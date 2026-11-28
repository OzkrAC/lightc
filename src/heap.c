#include <lightc/heap.h>
#include <lightc/memory.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/thread.h>
#include <lightc/fault.h>    /* lc_chaos_point, in the faultinject build */
#include <stdatomic.h>
#include "kernel.h"          /* ARCH_SET_FS */
#include "internal.h"        /* LC_SLOT_END */

/*
 * Block header — sits right before the pointer we return.
 */
typedef struct {
    size_t   size;
    uint32_t bucket;
    uint32_t magic;
} lc_heap_header;

#define HEAP_MAGIC      0x4C434845
#define HEAP_MAGIC_FREED 0xDEADBEEF
#define HEADER_SIZE     sizeof(lc_heap_header)

/*
 * Large (mmap-backed) blocks prefix the standard header with the mapping's true
 * page count. Best-fit reuse can hand out a cached mapping larger than the
 * request, so the page count must travel with the block itself — recomputing it
 * from the requested size would under-count and leak the tail pages on free.
 * The prefix is padded to 16 bytes so the returned pointer keeps the heap's
 * 16-byte alignment (block is page-aligned; block + LARGE_HEADER_SIZE stays so).
 */
#define LARGE_PREHEADER_SIZE ((size_t)16)
#define LARGE_HEADER_SIZE    (LARGE_PREHEADER_SIZE + HEADER_SIZE)

/*
 * Debug safety checks — enabled when LC_HEAP_DEBUG is defined.
 * In release builds these are compiled out for maximum performance.
 *
 * When enabled:
 *   - Double-free detection via magic poisoning
 *   - Heap corruption detection via magic validation
 *   - Use-after-free detection via 0xCC fill
 */
#ifndef LC_HEAP_DEBUG
# ifdef NDEBUG
#  define LC_HEAP_DEBUG 0
# else
#  define LC_HEAP_DEBUG 1
# endif
#endif

/*
 * Size classes — 29 buckets with ~1.25x geometric spacing.
 *
 * Every block size MUST be a multiple of 16: a block starts at a 16-aligned
 * offset within a 64 KiB-aligned page, and the user pointer is block + the
 * 16-byte header, so a non-16-multiple stride would hand out 8-mod-16 addresses
 * for odd block indices — violating the documented 16-byte ABI alignment
 * (a movaps / _Atomic __int128 on such a block would fault or tear).
 *
 * The smallest group [32,64) only contains two distinct 16-multiples (32, 48),
 * so the 1.25x/1.75x steps that would otherwise give 40 and 56 are rounded up
 * to 48 and 64. That leaves one redundant 48 slot (index 2 is never selected,
 * since find_bucket returns the first class >= the request) — a negligible cost
 * for universal 16-alignment.
 */
#define BUCKET_COUNT    29
#define BUCKET_LARGE    0xFFFFFFFF
#define BUCKET_NONE     0xFFFFFFFE

static const size_t bucket_block_sizes[BUCKET_COUNT] = {
                 32,  48,  48,  64,   /* was 32,40,48,56 — 40/56 not 16-aligned */
                 64,  80,  96, 112,
                128, 160, 192, 224,
                256, 320, 384, 448,
                512, 640, 768, 896,
               1024, 1280, 1536, 1792,
               2048, 2560, 3072, 3584, 4096
};

/*
 * Large block cache — avoids mmap/munmap syscalls for repeated
 * large allocations. Freed large blocks are cached here and reused
 * on subsequent allocations of equal or smaller size.
 */
#define LARGE_CACHE_SIZE 16

typedef struct {
    void  *ptr;    /* mmap'd region (includes header) */
    size_t pages;  /* number of pages in the mapping */
} lc_large_cache_entry;

static lc_large_cache_entry large_cache[LARGE_CACHE_SIZE];
static uint32_t large_cache_count = 0;
static lc_spinlock large_cache_lock = LC_SPINLOCK_INIT;

/*
 * Remote free node — stored inside a freed block when a foreign
 * thread frees it. Forms a lock-free singly-linked list per page.
 */
typedef struct lc_free_node {
    struct lc_free_node *next;
} lc_free_node;

/*
 * Segment/Page architecture with thread-local heaps.
 *
 * Each thread owns its pages exclusively:
 *   - Owner allocates from bitmap: NO LOCK
 *   - Owner frees to bitmap: NO LOCK
 *   - Foreign thread frees: atomic CAS push to remote_free_list (NO LOCK)
 *   - Owner drains remote_free_list before allocating (one atomic exchange)
 *
 * Global lock only for: creating segments, assigning fresh pages, and the
 * pages of threads that have exited (see Page states below).
 */

#define SEGMENT_SIZE        (4 * 1024 * 1024)
#define SEGMENT_ALIGN_MASK  (~(uintptr_t)(SEGMENT_SIZE - 1))
#define ALLOC_PAGE_SIZE     (64 * 1024)
#define PAGES_PER_SEGMENT   ((SEGMENT_SIZE / ALLOC_PAGE_SIZE) - 1)
#define BITMAP_MAX_WORDS    32

/*
 * Page states.
 *
 * Every page of a segment is in one state at a time, which `state` records.
 * A page in use belongs to one size class, its bucket; a free one to none.
 *
 *   PAGE_FREE       In its segment, held by no class: bucket is BUCKET_NONE,
 *                   and its memory has been given back with MADV_DONTNEED,
 *                   unless it was never used.
 *   PAGE_OWNED      On its owner's list for its class, local->pages[bucket],
 *                   with owner_tid the owner's thread id. Only the owner
 *                   allocates from it, frees into its bitmap and drains its
 *                   remote frees, without a lock. Another thread that frees
 *                   one of its blocks pushes the block onto remote_free_list.
 *   PAGE_FULL       Not used yet, but for a later change: an owned page
 *                   that the allocation walk found full, moved to the
 *                   owner's full list for its class so that the walk does
 *                   not visit it again. The owner's free into it moves it
 *                   back to the head of its class; a remote free into it
 *                   queues it once on the owner's atomic collect list, which
 *                   the owner drains before it takes a fresh page.
 *   PAGE_RESERVE    Not used yet, but for a later change: an emptied page
 *                   that is not the head of its class, kept by its owner, at
 *                   most two per class, instead of going back to its segment
 *                   at once. The purge delay counts operations, not time: each
 *                   allocation and free of the owner counts one, and a
 *                   reserve page that LC_HEAP_PURGE_OPS of them (4096 to
 *                   start with) have passed without reusing it is released,
 *                   madvised while it is still private to the owner and only
 *                   then marked free under heap_global_lock. A clock would
 *                   cost a system call on the free path.
 *   PAGE_ABANDONED  Its owner exited while it still held a block. It is on
 *                   abandoned_room[bucket] when at least a quarter of its
 *                   blocks are free, and on abandoned_full[bucket] otherwise.
 *                   owner_tid is HEAP_NO_OWNER, which no thread id equals,
 *                   and remote_free_list holds PAGE_ABANDONED_MARK, so a free
 *                   into it pushes nothing onto the page: it is an orphan
 *                   free (below). Only a holder of heap_global_lock touches
 *                   its bitmap, used_count, state or links.
 *
 * The transitions, and what guards each:
 *
 *   FREE -> OWNED       get_fresh_page, for an allocation that finds no page
 *                       with a free block in its own list and no abandoned
 *                       page of its class to adopt. heap_global_lock.
 *   OWNED -> FREE       the owner's free that empties a page other than the
 *                       only one of its class (maybe_reclaim_page), and the
 *                       owner's exit, for each of its pages that is empty once
 *                       its remote frees are drained. The owner unlinks the
 *                       page from its list; page_reclaim gives it back to its
 *                       segment under heap_global_lock. The exit madvises
 *                       those pages before it takes the lock, as no other
 *                       thread can touch an empty page.
 *   OWNED -> ABANDONED  the owner's exit, for each of its pages that still
 *                       holds a block once its remote frees are drained:
 *                       under heap_global_lock it swaps the page's remote list
 *                       for PAGE_ABANDONED_MARK, frees what the list held, and
 *                       links the page onto the room or the full list.
 *   ABANDONED -> OWNED  adoption: an allocation that finds no page with a
 *                       free block in its own list takes the first page of
 *                       abandoned_room[bucket], before it takes a fresh page,
 *                       and clears the mark. heap_global_lock. A page with
 *                       fewer than a quarter of its blocks free is not
 *                       adopted: it would give the adopter a few blocks and
 *                       then sit full in its list, where every allocation walk
 *                       that finds no room before it visits it again.
 *   ABANDONED -> FREE   when the free of its last block is applied, by the
 *                       thread that freed it, or by the thread that held the
 *                       lock then (below). heap_global_lock.
 *
 * The full and reserve states add OWNED <-> FULL, OWNED -> RESERVE, and
 * RESERVE -> OWNED or FREE, and the exit is to treat full and reserve pages
 * as it treats owned ones.
 *
 * Orphan frees. A free into an abandoned page finds PAGE_ABANDONED_MARK on
 * the page's remote list. When heap_global_lock is free, the freeing thread
 * takes it and applies the free itself (orphan_apply): it clears the block's
 * bit, moves the page from the full list to the room list once a quarter of
 * its blocks are free, and gives the page back to its segment when the block
 * was its last. When the lock is taken, the thread pushes the block onto
 * orphan_frees, a lock-free list of blocks, and tries the lock once more.
 * Every holder of the lock applies the list (orphan_collect) before it
 * releases the lock, and looks at it again after (heap_global_unlock), so a
 * free that found the lock taken is applied by the time the thread that
 * held it has released it, with no thread exit or later free needed. No
 * work depends on how many pages are abandoned: an exit costs its own pages
 * and the orphan frees, and an adoption takes the head of a list.
 *
 * Why no free is lost to an abandoned page: the exiting owner's swap of the
 * remote list for the mark is one atomic exchange. A push either completed
 * before it, and the exchange returned the block, or comes after it, and
 * then its load or its failed compare-and-swap sees the mark. Pushes never
 * replace the mark, and only adoption clears it, under the lock.
 *
 * Why a free cannot race with a reclaim: a block counts in its page's
 * used_count until its bit is cleared, by the page's owner draining its
 * remote list or by orphan_apply. A thread that is freeing a block holds
 * one whose bit is still set, so its page is not empty, nobody reclaims it
 * and its segment stays mapped; once its push onto the page's list has
 * succeeded, or it has seen the mark, it does not touch the page again.
 * A block on orphan_frees keeps its bit too, so its page is not reclaimed
 * before orphan_apply reaches it; if the page has been adopted meanwhile,
 * orphan_apply pushes the block onto its remote list for the new owner. A
 * page whose used_count is 0 has no block for any thread to free.
 *
 * Lock order: heap_local_table_lock, then heap_global_lock, then
 * large_cache_lock. A path that holds more than one takes them in that
 * order; none of the heap's paths holds two at once now, and a fork hook
 * that takes all three, so that the child of a fork finds none held, takes
 * them in that order. The abandoned lists are under heap_global_lock, which
 * an allocation takes anyway for a fresh page, so they add no lock, and
 * orphan_frees takes none.
 */
enum {
    PAGE_FREE = 0,
    PAGE_OWNED,
    PAGE_FULL,        /* not used yet */
    PAGE_RESERVE,     /* not used yet */
    PAGE_ABANDONED,
};

/* The owner_tid of a page no thread owns: free or abandoned. The kernel
 * gives no thread id 0, and get_local refuses one, so no thread takes such
 * a page for its own. */
#define HEAP_NO_OWNER 0

/* What the remote list of an abandoned page holds instead of a list: no
 * block is at address 1. */
#define PAGE_ABANDONED_MARK ((lc_free_node *)1)

/*
 * owner_tid is read by every thread that frees a block of the page, to tell
 * a free of its own from a remote one, and written under heap_global_lock
 * when the page changes hands. Relaxed loads and stores are enough on both
 * arches, because a thread only compares it with its own id: it reads its
 * own tid there only if it wrote it itself (page_init_for_bucket or
 * page_adopt, earlier in its own program order), or if a thread of the same
 * id did, before it exited and the kernel gave the id out again. That
 * thread's exit wrote HEAP_NO_OWNER, or its pages were reclaimed, before the
 * kernel freed the id, and the kernel's locks around freeing and handing
 * out an id order that store before the new thread's first instruction.
 */
typedef struct lc_heap_page {
    struct lc_heap_page *next;
    struct lc_heap_page *prev;    /* on an abandoned list only */
    uint8_t             *data_start;
    uint32_t             bucket;
    uint16_t             block_count;
    uint16_t             used_count;
    _Atomic(int32_t)     owner_tid;
    uint32_t             state;   /* PAGE_FREE, PAGE_OWNED or PAGE_ABANDONED */
    _Atomic(lc_free_node *) remote_free_list;
    uint64_t             bitmap[BITMAP_MAX_WORDS];
} lc_heap_page;

typedef struct lc_heap_segment {
    struct lc_heap_segment *next;
    uint32_t page_count;
    uint32_t used_page_count;
    lc_heap_page pages[PAGES_PER_SEGMENT];
} lc_heap_segment;

/*
 * Thread-local heap — each thread has one.
 * Stored in a CPU register for zero-cost access:
 *   x86_64:  FS register (set via arch_prctl, read via %fs:0)
 *   aarch64: TPIDR_EL0 register (read/write from userspace)
 *
 * First alloc per thread: one gettid + one register setup (slow path).
 * All subsequent allocs/frees: single instruction to read (fast path).
 */
typedef struct lc_heap_local {
    struct lc_heap_local *self;      /* offset 0: self-pointer (x86_64 TLS read) */
    int32_t tid;
    struct lc_heap_local *next;      /* hash chain, or the spare list; under heap_local_table_lock */
    lc_heap_page *pages[BUCKET_COUNT]; /* per-bucket page list */
    void *tls_slots[LC_SLOT_END];    /* lc_tls_get/set's slots, then the library's (internal.h) */
} lc_heap_local;

/* Hash table for thread-local heaps */
#define HEAP_LOCAL_TABLE_SIZE 64
/* Chained hash table keyed by tid. A thread's exit unlinks its local and
 * unmaps it, so the chains are walked, and changed, only under
 * heap_local_table_lock: a walk without it could follow `next` into a local
 * that has just been unmapped. The lock also guards spare_locals: the locals
 * whose munmap failed at their thread's exit, which get_local hands out
 * again before it maps a new one. */
static lc_heap_local *heap_local_table[HEAP_LOCAL_TABLE_SIZE];
static lc_heap_local *spare_locals;
static lc_spinlock heap_local_table_lock = LC_SPINLOCK_INIT;

/* Global state */
static lc_heap_segment *all_segments = NULL;
static lc_spinlock heap_global_lock = LC_SPINLOCK_INIT;

/* Every holder of heap_global_lock releases it with this, which applies
 * the orphan frees first (see Orphan frees). */
static void heap_global_unlock(void);

/* The abandoned pages of each size class (see Page states), doubly linked
 * through their `next` and `prev`: those with at least a quarter of their
 * blocks free, which adoption takes, and the others. Under heap_global_lock,
 * with the pages' bitmaps, used_counts, states and links. */
static lc_heap_page *abandoned_room[BUCKET_COUNT];
static lc_heap_page *abandoned_full[BUCKET_COUNT];

/* Blocks freed into abandoned pages while heap_global_lock was taken,
 * linked through their first word, for orphan_collect to apply. Pushed with
 * a release compare-and-swap; taken whole, under the lock, with an acquire
 * exchange. */
static _Atomic(lc_free_node *) orphan_frees;

/* Statistics counters — compiled out when LC_STATS=0 */
#if LC_STATS
static _Atomic(uint64_t) stat_total_allocs = 0;
static _Atomic(uint64_t) stat_total_frees = 0;
static _Atomic(uint64_t) stat_total_bytes_allocated = 0;
static _Atomic(uint64_t) stat_total_bytes_freed = 0;
static _Atomic(uint64_t) stat_active_allocs = 0;
static _Atomic(uint64_t) stat_active_bytes = 0;
static _Atomic(uint64_t) stat_peak_allocs = 0;
static _Atomic(uint64_t) stat_peak_bytes = 0;
static _Atomic(uint64_t) stat_large_allocs = 0;
static _Atomic(uint64_t) stat_large_cache_hits = 0;
static _Atomic(uint64_t) stat_pages_reclaimed = 0;

static void stat_track_alloc(size_t size) {
    atomic_fetch_add_explicit(&stat_total_allocs, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&stat_total_bytes_allocated, size, memory_order_relaxed);
    uint64_t active = atomic_fetch_add_explicit(&stat_active_allocs, 1, memory_order_relaxed) + 1;
    uint64_t bytes = atomic_fetch_add_explicit(&stat_active_bytes, size, memory_order_relaxed) + size;
    /* Update peaks (relaxed compare-exchange loop) */
    uint64_t peak = atomic_load_explicit(&stat_peak_allocs, memory_order_relaxed);
    while (active > peak) {
        if (atomic_compare_exchange_weak_explicit(&stat_peak_allocs, &peak, active,
                memory_order_relaxed, memory_order_relaxed)) break;
    }
    peak = atomic_load_explicit(&stat_peak_bytes, memory_order_relaxed);
    while (bytes > peak) {
        if (atomic_compare_exchange_weak_explicit(&stat_peak_bytes, &peak, bytes,
                memory_order_relaxed, memory_order_relaxed)) break;
    }
}

static void stat_track_free(size_t size) {
    atomic_fetch_add_explicit(&stat_total_frees, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&stat_total_bytes_freed, size, memory_order_relaxed);
    atomic_fetch_sub_explicit(&stat_active_allocs, 1, memory_order_relaxed);
    atomic_fetch_sub_explicit(&stat_active_bytes, size, memory_order_relaxed);
}
static inline void stat_track_page_reclaim(void) {
    atomic_fetch_add_explicit(&stat_pages_reclaimed, 1, memory_order_relaxed);
}
#else
static inline void stat_track_alloc(size_t size) { (void)size; }
static inline void stat_track_free(size_t size)  { (void)size; }
static inline void stat_track_page_reclaim(void) {}
#endif

/* --- Helpers --- */

/* Stop the program when the heap finds its own state broken: a page in a
 * state that the transition it is in cannot start from. Debug builds only;
 * the messages are as those of lc_heap_free's checks. */
#if LC_HEAP_DEBUG
[[gnu::cold, noreturn]]
static void heap_state_broken(const char *message, size_t length) {
    lc_kernel_write_bytes(2, message, length);
    lc_kernel_exit(134);
}
#define HEAP_CHECK_STATE(cond, message) \
    do { if (!(cond)) heap_state_broken(message, sizeof(message) - 1); } while (0)
#else
#define HEAP_CHECK_STATE(cond, message) ((void)0)
#endif

[[gnu::const]]
static uint8_t *segment_data_start(lc_heap_segment *seg) {
    return (uint8_t *)seg + ALLOC_PAGE_SIZE;
}

static lc_heap_page *page_from_pointer(void *ptr) {
    lc_heap_segment *seg = (lc_heap_segment *)((uintptr_t)ptr & SEGMENT_ALIGN_MASK);
    size_t offset = (uint8_t *)ptr - segment_data_start(seg);
    uint32_t page_idx = offset / ALLOC_PAGE_SIZE;
    return &seg->pages[page_idx];
}

[[gnu::const, gnu::hot]]
static uint32_t find_bucket(size_t size) {
    /* size + HEADER_SIZE must not wrap: a size near SIZE_MAX would otherwise
     * land in bucket 0 and hand out a 32-byte block claiming SIZE_MAX bytes.
     * Route it to BUCKET_LARGE, whose path rejects oversized requests. */
    if (size > SIZE_MAX - HEADER_SIZE) return BUCKET_LARGE;
    size_t needed = size + HEADER_SIZE;
    if (needed <= 32) return 0;
    if (needed > 4096) return BUCKET_LARGE;

    uint32_t bit = 63 - (uint32_t)__builtin_clzll(needed);
    uint32_t group = bit - 5;
    uint32_t base = group * 4;

    for (uint32_t i = base; i < BUCKET_COUNT; i++) {
        if (needed <= bucket_block_sizes[i]) return i;
    }
    return BUCKET_LARGE;
}

[[gnu::const]]
static size_t bucket_usable_size(uint32_t bucket) {
    return bucket_block_sizes[bucket] - HEADER_SIZE;
}

/* --- Bitmap operations --- */

[[gnu::hot]]
static void *bitmap_allocate_block(lc_heap_page *page) {
    size_t block_size = bucket_block_sizes[page->bucket];
    uint32_t words = (page->block_count + 63) / 64;

    for (uint32_t w = 0; w < words; w++) {
        if (page->bitmap[w] != UINT64_MAX) {
            uint32_t bit = (uint32_t)__builtin_ctzll(~page->bitmap[w]);
            page->bitmap[w] |= (1ULL << bit);
            page->used_count++;
            uint32_t block_idx = w * 64 + bit;
            return page->data_start + block_idx * block_size;
        }
    }
    return NULL;
}

[[gnu::hot]]
static void bitmap_free_block(lc_heap_page *page, void *block_ptr) {
    size_t block_size = bucket_block_sizes[page->bucket];
    uint32_t block_idx = (uint32_t)((uint8_t *)block_ptr - page->data_start) / (uint32_t)block_size;
    uint32_t word_idx = block_idx / 64;
    uint32_t bit_idx = block_idx % 64;
    page->bitmap[word_idx] &= ~(1ULL << bit_idx);
    page->used_count--;
}

/* --- Remote free list (lock-free CAS push) --- */

/* Push a block onto its page's remote-free list. Returns false, having
 * pushed nothing, when the list holds PAGE_ABANDONED_MARK: the page is
 * abandoned, and the block is for orphan_free instead. A failed CAS reloads
 * the head, so a push that loaded the head before the owner's exit swapped
 * in the mark sees the mark on its retry. */
static bool remote_free_push(lc_heap_page *page, void *block_ptr) {
    lc_free_node *node = (lc_free_node *)block_ptr;
    lc_free_node *old_head = atomic_load_explicit(&page->remote_free_list, memory_order_relaxed);
    do {
#ifdef LC_FAULTINJECT
        /* A chaos point between the load of the head and the CAS, so that
         * the owner's drain, its exit and other remote frees come in
         * between. */
        lc_chaos_point();
#endif
        if (old_head == PAGE_ABANDONED_MARK) return false;
        node->next = old_head;
    } while (!atomic_compare_exchange_weak_explicit(
        &page->remote_free_list, &old_head, node,
        memory_order_release, memory_order_relaxed));
    return true;
}

/* Clear the bits of the blocks of `list`, a remote-free list of `page`. */
static void drain_list(lc_heap_page *page, lc_free_node *list) {
    size_t block_size = bucket_block_sizes[page->bucket];
    while (list != NULL) {
        lc_free_node *next = list->next;
        uint32_t block_idx = (uint32_t)((uint8_t *)list - page->data_start) / (uint32_t)block_size;
        uint32_t word_idx = block_idx / 64;
        uint32_t bit_idx = block_idx % 64;
        page->bitmap[word_idx] &= ~(1ULL << bit_idx);
        page->used_count--;
        list = next;
    }
}

/* Drain all remote frees back into the bitmap. Called by owner only; an
 * owned page's list never holds the mark. */
static void drain_remote_frees(lc_heap_page *page) {
    drain_list(page, atomic_exchange_explicit(&page->remote_free_list, NULL,
                                              memory_order_acquire));
}

/* --- TLS register access (zero-cost thread-local storage) --- */

/*
 * x86_64: FS register points to lc_heap_local, %fs:0 reads self-pointer.
 * aarch64: TPIDR_EL0 holds lc_heap_local pointer directly.
 */

/*
 * Read/write the per-thread TLS pointer.
 *
 * x86_64: FS register. We set FS base to &local, and local->self = &local,
 *   so %fs:0 gives us the pointer. Before first setup, FS base is 0 and
 *   %fs:0 would segfault, so we set up a null sentinel at program init.
 *
 * aarch64: TPIDR_EL0 holds the lc_heap_local pointer directly. We read its
 *   ->self field (like x86_64's %fs:0) so the null sentinel — whose address
 *   is installed in TPIDR_EL0 but whose self = NULL — reports as uninitialized
 *   rather than being mistaken for a real per-thread heap.
 */

/* Null sentinel: FS/TPIDR points here before a thread sets up TLS.
 * self = NULL tells get_local() this thread is uninitialized.
 * On x86_64, we point FS here so %fs:0 reads NULL instead of segfaulting. */
static lc_heap_local tls_null_sentinel = { .self = NULL };

static inline lc_heap_local *tls_read(void) {
#if defined(__x86_64__)
    lc_heap_local *local;
    __asm__ volatile("movq %%fs:0, %0" : "=r"(local));
    return local;
#elif defined(__aarch64__)
    /* TPIDR_EL0 holds the lc_heap_local pointer directly. Dereference ->self
     * so the sentinel (self == NULL) is reported as uninitialized, mirroring
     * x86_64's %fs:0 read. A zero register (TLS never set) is treated as NULL
     * too, so this never faults. */
    lc_heap_local *base;
    __asm__ volatile("mrs %0, tpidr_el0" : "=r"(base));
    if (base == NULL) return NULL;
    return base->self;
#endif
}

/* Point this thread's TLS register at `local`. False when it still points
 * where it did: on x86_64 when arch_prctl fails, which only a fault rule
 * makes it do for a mapped address. */
static inline bool tls_write(lc_heap_local *local) {
#if defined(__x86_64__)
    return lc_syscall2(SYS_arch_prctl, ARCH_SET_FS, (int64_t)local) == 0;
#elif defined(__aarch64__)
    __asm__ volatile("msr tpidr_el0, %0" :: "r"(local));
    return true;
#endif
}

/* --- Thread-local heap lookup --- */

/*
 * Fast path: read TLS register (1 instruction, ~1 cycle).
 * Slow path: first call per thread — gettid + mmap + arch_prctl.
 */
/*
 * On x86_64, reading %fs:0 with FS base = 0 segfaults. New threads
 * start with FS = 0 (clone doesn't set it). We export lc_heap_tls_sentinel()
 * so the thread module can set FS to the null sentinel via CLONE_SETTLS.
 */
void *lc_heap_tls_sentinel(void) {
    return &tls_null_sentinel;
}

/* Forward declaration — defined below */
static lc_heap_local *get_local(void);

/* Return per-thread TLS slot array. Used by lc_tls_get/set in thread.c.
 * Creates the thread-local heap if it doesn't exist yet. */
void **lc_heap_tls_slots(void) {
    lc_heap_local *local = get_local();
    if (local == NULL) return NULL;
    return local->tls_slots;
}

/*
 * Point the main thread's TLS register at the null sentinel. Called once from
 * _start (x86_64), before main() and therefore before any other thread can
 * exist. This is what makes the very first `%fs:0` read on the main thread
 * safe. It replaces a former lazy, process-global "ready" flag whose fatal flaw
 * was that ANY thread's first allocation flipped it: a child that allocated
 * before main did would set the flag, so main's first read then skipped its own
 * FS setup and faulted on FS base 0. Doing it eagerly while single-threaded
 * removes the race entirely — every thread that ever calls get_local() now has
 * a valid TLS register (main via this call, children via CLONE_SETTLS).
 *
 * On aarch64 TPIDR_EL0 starts at 0 and tls_read() already treats 0 as
 * "uninitialized" without faulting, so no eager setup is required there.
 */
void lc_heap_bootstrap_main_tls(void) {
#if defined(__x86_64__)
    (void)tls_write(&tls_null_sentinel);
#endif
}

static lc_heap_local *get_local(void) {
    /* Fast path: read TLS register (1 instruction). Non-NULL means this thread
     * already has a local heap. NULL means TLS points at tls_null_sentinel
     * (self == NULL) — i.e. this is the thread's first allocation. The main
     * thread's FS is set up in lc_heap_bootstrap_main_tls() at startup and
     * children get theirs via CLONE_SETTLS, so this read never faults. */
    lc_heap_local *local = tls_read();
    if (local != NULL) return local;

    /* Slow path: first alloc on this thread. A thread id is above 0, which
     * is HEAP_NO_OWNER; only a fault rule makes gettid return less. */
    int32_t tid = lc_kernel_get_thread_id();
    if (tid <= HEAP_NO_OWNER) return NULL;
    uint32_t slot = (uint32_t)tid % HEAP_LOCAL_TABLE_SIZE;

    lc_spinlock_acquire(&heap_local_table_lock);
    for (local = heap_local_table[slot]; local != NULL; local = local->next) {
        if (local->tid == tid) {
            /* The local of a thread of this id that ended without
             * lc_heap_thread_exit, as one that made its own SYS_exit does:
             * this thread takes over its pages, but starts with every
             * lc_tls_* slot NULL, not with the dead one's values. */
            lc_bytes_fill(local->tls_slots, 0, sizeof(local->tls_slots));
            lc_spinlock_release(&heap_local_table_lock);
            (void)tls_write(local);
            return local;
        }
    }
    /* A local whose unmap failed at its thread's exit, before a new one. */
    local = spare_locals;
    if (local != NULL) spare_locals = local->next;
    lc_spinlock_release(&heap_local_table_lock);

    /* Create new local heap via raw mmap (can't use lc_heap_allocate) */
    if (local == NULL) {
        local = (lc_heap_local *)lc_kernel_map_memory(
            NULL, sizeof(lc_heap_local),
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (local == MAP_FAILED) return NULL;
    }

    /* A spare local holds what its last thread left: no page, but its slots. */
    lc_bytes_fill(local, 0, sizeof(*local));
    local->self = local;  /* self-pointer for x86_64 %fs:0 read */
    local->tid = tid;

    /* No other live thread has this tid, so none can have inserted a local
     * for it since the walk above. */
    lc_spinlock_acquire(&heap_local_table_lock);
    local->next = heap_local_table[slot];
    heap_local_table[slot] = local;
    lc_spinlock_release(&heap_local_table_lock);

    /* Set TLS register — all future reads are free */
    (void)tls_write(local);

    return local;
}

/* --- Segment lifecycle --- */

/* Unmap a segment whose every page is free. Caller holds heap_global_lock.
 * The segment leaves all_segments only once its munmap has succeeded: when
 * munmap fails, as it can with ENOMEM at the limit on mappings, the segment
 * stays on the list, every page free, for get_fresh_page to use again,
 * instead of staying mapped where nothing can reach it. */
static void segment_release(lc_heap_segment *seg) {
    lc_heap_segment **pp = &all_segments;
    while (*pp != NULL && *pp != seg) pp = &(*pp)->next;
    lc_heap_segment *next = seg->next;   /* read while it is mapped */
    if (lc_kernel_unmap_memory(seg, SEGMENT_SIZE) < 0) return;
    if (*pp == seg) *pp = next;
}

/* Return an emptied slab page to its segment. Caller must hold heap_global_lock
 * (touches seg->used_page_count and all_segments) and must have already
 * unlinked the page from the owner's thread-local list, or from its
 * abandoned list. `advised` says the caller has already given the page's
 * memory back with MADV_DONTNEED, as an exiting thread does with its empty
 * pages before it takes the lock. */
[[gnu::cold]]
static void page_reclaim(lc_heap_page *page, bool advised) {
    HEAP_CHECK_STATE(page->state == PAGE_OWNED || page->state == PAGE_ABANDONED,
                     "lightc: heap page reclaimed twice\n");
    HEAP_CHECK_STATE(page->used_count == 0, "lightc: heap page reclaimed with a block in use\n");
    if (!advised) lc_kernel_advise_memory(page->data_start, ALLOC_PAGE_SIZE, MADV_DONTNEED);
    page->bucket = BUCKET_NONE;
    page->block_count = 0;
    page->used_count = 0;
    page->state = PAGE_FREE;
    atomic_store_explicit(&page->owner_tid, HEAP_NO_OWNER, memory_order_relaxed);
    page->next = NULL;
    page->prev = NULL;
    atomic_store_explicit(&page->remote_free_list, NULL, memory_order_relaxed);
    stat_track_page_reclaim();

    lc_heap_segment *seg = (lc_heap_segment *)((uintptr_t)page->data_start & SEGMENT_ALIGN_MASK);
    seg->used_page_count--;

    if (seg->used_page_count == 0) {
        segment_release(seg);
    }
}

[[gnu::cold]]
static lc_heap_segment *segment_create(void) {
    void *raw = lc_kernel_map_memory(NULL, SEGMENT_SIZE * 2,
                                     PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (raw == MAP_FAILED) return NULL;

    uintptr_t addr = (uintptr_t)raw;
    uintptr_t aligned = (addr + SEGMENT_SIZE - 1) & SEGMENT_ALIGN_MASK;

    if (aligned > addr)
        lc_kernel_unmap_memory((void *)addr, aligned - addr);

    uintptr_t seg_end = aligned + SEGMENT_SIZE;
    uintptr_t raw_end = addr + SEGMENT_SIZE * 2;
    if (seg_end < raw_end)
        lc_kernel_unmap_memory((void *)seg_end, raw_end - seg_end);

    lc_heap_segment *seg = (lc_heap_segment *)aligned;
    seg->next = all_segments;
    all_segments = seg;
    seg->page_count = PAGES_PER_SEGMENT;
    seg->used_page_count = 0;

    uint8_t *data_base = segment_data_start(seg);
    for (uint32_t i = 0; i < PAGES_PER_SEGMENT; i++) {
        seg->pages[i].next = NULL;
        seg->pages[i].prev = NULL;
        seg->pages[i].data_start = data_base + i * ALLOC_PAGE_SIZE;
        seg->pages[i].bucket = BUCKET_NONE;
        seg->pages[i].block_count = 0;
        seg->pages[i].used_count = 0;
        seg->pages[i].state = PAGE_FREE;
        atomic_init(&seg->pages[i].owner_tid, HEAP_NO_OWNER);
        atomic_init(&seg->pages[i].remote_free_list, NULL);
    }

    return seg;
}

/* Initialize a page for a bucket. Does NOT add to any list. */
static lc_heap_page *page_init_for_bucket(lc_heap_segment *seg, uint32_t page_idx,
                                          uint32_t bucket, int32_t owner_tid) {
    lc_heap_page *page = &seg->pages[page_idx];
    size_t block_size = bucket_block_sizes[bucket];

    page->bucket = bucket;
    page->block_count = (uint16_t)(ALLOC_PAGE_SIZE / block_size);
    page->used_count = 0;
    page->state = PAGE_OWNED;
    atomic_store_explicit(&page->owner_tid, owner_tid, memory_order_relaxed);
    atomic_store_explicit(&page->remote_free_list, NULL, memory_order_relaxed);

    uint32_t words_needed = (page->block_count + 63) / 64;
    for (uint32_t i = 0; i < words_needed; i++)
        page->bitmap[i] = 0;

    uint32_t remainder = page->block_count % 64;
    if (remainder != 0)
        page->bitmap[words_needed - 1] = ~((1ULL << remainder) - 1);

    for (uint32_t i = words_needed; i < BITMAP_MAX_WORDS; i++)
        page->bitmap[i] = UINT64_MAX;

    return page;
}

/* Get a fresh page from any segment. Caller holds heap_global_lock. */
static lc_heap_page *get_fresh_page(uint32_t bucket, int32_t owner_tid) {
    for (lc_heap_segment *seg = all_segments; seg != NULL; seg = seg->next) {
        if (seg->used_page_count < seg->page_count) {
            for (uint32_t i = 0; i < seg->page_count; i++) {
                if (seg->pages[i].bucket == BUCKET_NONE) {
                    seg->used_page_count++;
                    return page_init_for_bucket(seg, i, bucket, owner_tid);
                }
            }
        }
    }

    lc_heap_segment *seg = segment_create();
    if (seg == NULL) return NULL;

    seg->used_page_count++;
    return page_init_for_bucket(seg, 0, bucket, owner_tid);
}

/* Remove a page from a thread-local page list. */
static void remove_page_from_list(lc_heap_page **list_head, lc_heap_page *page) {
    lc_heap_page **pp = list_head;
    while (*pp != NULL) {
        if (*pp == page) {
            *pp = page->next;
            page->next = NULL;
            return;
        }
        pp = &(*pp)->next;
    }
}

/*
 * After a same-thread free, hand an emptied page back to its segment so the
 * heap footprint can shrink instead of growing monotonically.
 *
 * Safe because used_count == 0 means every block handed out from this page has
 * been freed-and-accounted: a pending remote free would still be counted (it is
 * only decremented on drain), so used_count == 0 implies the remote list is
 * empty too, and with no live blocks no other thread can push a new one. The
 * owner alone reaches this path, so unlinking from its own list needs no lock;
 * page_reclaim touches shared segment state under heap_global_lock.
 *
 * One page per bucket is kept resident so a tight alloc/free loop on a single
 * page does not madvise-and-refault (or unmap/remap a whole segment) every
 * iteration.
 */
static void maybe_reclaim_page(lc_heap_local *local, lc_heap_page *page) {
    if (page->used_count != 0) return;

    uint32_t bucket = page->bucket;
    lc_heap_page *head = local->pages[bucket];
    if (head == page && page->next == NULL) return;  /* keep the sole page */

    remove_page_from_list(&local->pages[bucket], page);
    lc_spinlock_acquire(&heap_global_lock);
    page_reclaim(page, false);
    heap_global_unlock();
}

/* --- The pages of threads that have exited --- */

/* Whether `page` has a real share of its blocks free, at least a quarter,
 * which makes it worth adopting. Every class has at least 16 blocks a page,
 * so a page with one block in use always has room. */
static bool page_has_room(const lc_heap_page *page) {
    return (uint32_t)(page->block_count - page->used_count) >= (uint32_t)page->block_count / 4;
}

/* The abandoned list `page` belongs on, by its free blocks now. Caller holds
 * heap_global_lock. */
static lc_heap_page **abandoned_list(const lc_heap_page *page) {
    return page_has_room(page) ? &abandoned_room[page->bucket] : &abandoned_full[page->bucket];
}

static void abandoned_link(lc_heap_page **head, lc_heap_page *page) {
    page->prev = NULL;
    page->next = *head;
    if (*head != NULL) (*head)->prev = page;
    *head = page;
}

static void abandoned_unlink(lc_heap_page **head, lc_heap_page *page) {
    if (page->prev != NULL) page->prev->next = page->next;
    else *head = page->next;
    if (page->next != NULL) page->next->prev = page->prev;
    page->next = NULL;
    page->prev = NULL;
}

/* OWNED -> ABANDONED: the owner is exiting and `page` still holds a block,
 * and its remote list holds the mark. Caller holds heap_global_lock, and has
 * unlinked the page from its owner's list. */
static void page_abandon(lc_heap_page *page) {
    HEAP_CHECK_STATE(page->state == PAGE_OWNED, "lightc: heap page abandoned twice\n");
    page->state = PAGE_ABANDONED;
    atomic_store_explicit(&page->owner_tid, HEAP_NO_OWNER, memory_order_relaxed);
    abandoned_link(abandoned_list(page), page);
}

/*
 * ABANDONED -> OWNED: an abandoned page of `bucket` with room, now owned by
 * thread `tid`, or NULL when there is none. Caller holds heap_global_lock and
 * links the page into its own list. Clearing the mark makes later frees of
 * other threads push onto the page's remote list again, for its new owner to
 * drain; a free that saw the mark before has gone to orphan_frees, whose
 * collect pushes it there too.
 */
static lc_heap_page *page_adopt(uint32_t bucket, int32_t tid) {
    lc_heap_page *page = abandoned_room[bucket];
    if (page == NULL) return NULL;
    abandoned_unlink(&abandoned_room[bucket], page);
    page->state = PAGE_OWNED;
    atomic_store_explicit(&page->owner_tid, tid, memory_order_relaxed);
    atomic_store_explicit(&page->remote_free_list, NULL, memory_order_relaxed);
    return page;
}

/*
 * Apply the free of `node`, a block of a page that was abandoned when its
 * free saw the mark. Caller holds heap_global_lock. The block still counts
 * in its page, so the page is in use. Abandoned still, the page gets the
 * block's bit cleared here, moves to the room list once it has room, and
 * goes back to its segment when that was its last block. Adopted since, it
 * gets the block pushed onto its remote list, for its owner to drain; an
 * owned page's list never holds the mark.
 */
static void orphan_apply(lc_free_node *node) {
    lc_heap_page *page = page_from_pointer(node);
    if (page->state == PAGE_ABANDONED) {
        lc_heap_page **list = abandoned_list(page);
        bitmap_free_block(page, node);
        if (page->used_count == 0) {
            abandoned_unlink(list, page);
            page_reclaim(page, false);   /* may unmap the segment */
        } else if (abandoned_list(page) != list) {
            abandoned_unlink(list, page);
            abandoned_link(abandoned_list(page), page);
        }
        return;
    }
    HEAP_CHECK_STATE(page->state == PAGE_OWNED, "lightc: heap block freed into a free page\n");
    if (!remote_free_push(page, node))
        HEAP_CHECK_STATE(false, "lightc: heap page owned and abandoned at once\n");
}

/* Apply every free on orphan_frees. Caller holds heap_global_lock, which
 * makes it the one thread that takes the list. */
static void orphan_collect(void) {
    if (atomic_load_explicit(&orphan_frees, memory_order_relaxed) == NULL) return;
    lc_free_node *list = atomic_exchange_explicit(&orphan_frees, NULL, memory_order_acquire);
    while (list != NULL) {
        lc_free_node *next = list->next;   /* orphan_apply may reuse the block's first word */
        orphan_apply(list);
        list = next;
    }
}

/*
 * Release heap_global_lock, having applied the orphan frees, and apply any
 * that a thread pushed meanwhile because it found the lock taken.
 *
 * The check after the release and the retry after a push in orphan_free
 * are the two halves of a store-buffer pattern: a pusher stores to
 * orphan_frees and then reads the lock (its try-acquire), and a holder
 * stores to the lock (its release) and then reads orphan_frees. Without a
 * fence between each store and the read after it, both reads may miss the
 * other's store, on x86_64 too, where a store can wait in the store buffer
 * past a later load, and on aarch64, where nothing orders a plain load
 * after a release store. The pusher would then see the lock still taken
 * and leave, and the holder would see the list empty and leave, and the
 * free would wait for some later holder. The two seq_cst fences forbid
 * that: of the two fences, the one later in their total order makes its
 * thread's read see the other thread's store. So either the holder finds
 * the block, or the pusher finds the lock released, or held by a later
 * holder, which then does the same check when it releases.
 */
static void heap_global_unlock(void) {
    for (;;) {
        orphan_collect();
        lc_spinlock_release(&heap_global_lock);
        atomic_thread_fence(memory_order_seq_cst);
        if (atomic_load_explicit(&orphan_frees, memory_order_relaxed) == NULL) return;
        if (!lc_spinlock_try_acquire(&heap_global_lock)) return;
    }
}

/*
 * Free `node`, a block whose page's remote list held the mark. With
 * heap_global_lock free, apply it now, and the orphan frees others left;
 * with the lock taken, push it onto orphan_frees and try the lock once more,
 * so that it is applied by the time the thread that holds the lock has
 * released it, rather than wait for it. The block is still in use until
 * applied, so the push touches only the block and the list head.
 */
static void orphan_free(lc_free_node *node) {
    if (lc_spinlock_try_acquire(&heap_global_lock)) {
        orphan_apply(node);
        heap_global_unlock();
        return;
    }
    lc_free_node *old_head = atomic_load_explicit(&orphan_frees, memory_order_relaxed);
    do {
        node->next = old_head;
#ifdef LC_FAULTINJECT
        lc_chaos_point();   /* as in remote_free_push */
#endif
    } while (!atomic_compare_exchange_weak_explicit(&orphan_frees, &old_head, node,
                                                    memory_order_release, memory_order_relaxed));
    atomic_thread_fence(memory_order_seq_cst);   /* see heap_global_unlock */
    if (lc_spinlock_try_acquire(&heap_global_lock)) heap_global_unlock();
}

/* A block freed by a thread that is not its page's owner. */
static void remote_free(lc_heap_page *page, lc_heap_header *header) {
    if (!remote_free_push(page, header)) orphan_free((lc_free_node *)header);
}

/*
 * Give back the heap state of the calling thread, which is exiting: the
 * thread entry of thread.c calls this after the thread's function returns,
 * as the thread's last heap call before SYS_exit. It drains the remote
 * frees of every page of the thread and madvises the pages that are then
 * empty, without a lock, as they are its own. Under heap_global_lock, it
 * returns those to their segments, swaps the remote list of each other page
 * for the mark, frees what it held, and abandons the page for an allocation
 * of another thread to adopt, then applies the orphan frees. Then it takes
 * the local out of the table, points the TLS register at the sentinel, and
 * unmaps the local. The thread must make no heap or lc_tls_* call after it:
 * one would start a new local, which no exit would give back. A thread that
 * never used the heap has no local, and this does nothing: no orphan free
 * waits for an exit, as heap_global_unlock says.
 *
 * When munmap fails, the local goes to spare_locals for the next thread
 * that needs one. When arch_prctl fails, so that FS still points at the
 * local, it stays mapped, with no page and out of the table.
 */
void lc_heap_thread_exit(void) {
    lc_heap_local *local = tls_read();
    if (local == NULL) return;

    /* As the owner still, without a lock: drain each page, and sort it. No
     * thread can free into a page that is empty, so its memory can go back
     * before the lock. */
    lc_heap_page *empty = NULL, *held = NULL;
    for (uint32_t bucket = 0; bucket < BUCKET_COUNT; bucket++) {
        lc_heap_page *page = local->pages[bucket];
        local->pages[bucket] = NULL;
        while (page != NULL) {
            lc_heap_page *next = page->next;
            drain_remote_frees(page);
            if (page->used_count == 0) {
                lc_kernel_advise_memory(page->data_start, ALLOC_PAGE_SIZE, MADV_DONTNEED);
                page->next = empty;
                empty = page;
            } else {
                page->next = held;
                held = page;
            }
            page = next;
        }
    }

    /* A free of another thread can still push into a held page after its
     * drain above, until the exchange below swaps in the mark: the exchange
     * returns those blocks, and every later free of the page is an orphan
     * free. A page they empty goes back to its segment instead. */
    lc_spinlock_acquire(&heap_global_lock);
    while (empty != NULL) {
        lc_heap_page *next = empty->next;
        page_reclaim(empty, true);   /* may unmap the segment: `next` is read */
        empty = next;
    }
    while (held != NULL) {
        lc_heap_page *next = held->next;
        drain_list(held, atomic_exchange_explicit(&held->remote_free_list, PAGE_ABANDONED_MARK,
                                                  memory_order_acquire));
        if (held->used_count == 0) page_reclaim(held, false);
        else page_abandon(held);
        held = next;
    }
    heap_global_unlock();

    uint32_t slot = (uint32_t)local->tid % HEAP_LOCAL_TABLE_SIZE;
    lc_spinlock_acquire(&heap_local_table_lock);
    for (lc_heap_local **link = &heap_local_table[slot]; *link != NULL; link = &(*link)->next) {
        if (*link == local) {
            *link = local->next;
            break;
        }
    }
    lc_spinlock_release(&heap_local_table_lock);

    if (!tls_write(&tls_null_sentinel)) return;
    if (lc_kernel_unmap_memory(local, sizeof(lc_heap_local)) < 0) {
        lc_spinlock_acquire(&heap_local_table_lock);
        local->next = spare_locals;
        spare_locals = local;
        lc_spinlock_release(&heap_local_table_lock);
    }
}

/* --- Public API --- */

lc_result_ptr lc_heap_allocate(size_t size) {
    if (size == 0) size = 1;

    uint32_t bucket = find_bucket(size);

    if (__builtin_expect(bucket == BUCKET_LARGE, 0)) {
        /* Both size + LARGE_HEADER_SIZE and the page round-up below must not wrap. */
        if (size > SIZE_MAX - LARGE_HEADER_SIZE - (LC_PAGE_SIZE - 1))
            return lc_err_ptr(LC_ERR_NOMEM);
        size_t total = size + LARGE_HEADER_SIZE;
        size_t pages = (total + LC_PAGE_SIZE - 1) / LC_PAGE_SIZE;

        /* Try the large block cache first */
        uint8_t *block = NULL;
        size_t block_pages = pages;  /* actual page count of the mapping we use */
        lc_spinlock_acquire(&large_cache_lock);
        uint32_t best = UINT32_MAX;
        size_t best_pages = (size_t)-1;
        for (uint32_t i = 0; i < large_cache_count; i++) {
            if (large_cache[i].pages >= pages && large_cache[i].pages < best_pages) {
                best = i;
                best_pages = large_cache[i].pages;
                if (best_pages == pages) break; /* exact match */
            }
        }
        if (best != UINT32_MAX) {
            block = large_cache[best].ptr;
            block_pages = large_cache[best].pages;  /* the mapping's TRUE size */
            /* Remove from cache by swapping with last */
            large_cache[best] = large_cache[--large_cache_count];
        }
        lc_spinlock_release(&large_cache_lock);

        if (block != NULL) {
#if LC_STATS
            atomic_fetch_add_explicit(&stat_large_cache_hits, 1, memory_order_relaxed);
#endif
        } else {
            lc_result_ptr page_alloc = lc_allocate_pages(pages);
            if (__builtin_expect(lc_ptr_is_err(page_alloc), 0)) return lc_err_ptr(LC_ERR_NOMEM);
            block = page_alloc.value;
            block_pages = pages;
        }

#if LC_STATS
        atomic_fetch_add_explicit(&stat_large_allocs, 1, memory_order_relaxed);
#endif
        stat_track_alloc(size);

        /* Record the mapping's TRUE page count in the prefix so free releases
         * the whole mapping — otherwise a reused oversized block leaks its
         * tail pages, since free would recompute a smaller count from `size`. */
        *(size_t *)block = block_pages;
        lc_heap_header *header = (lc_heap_header *)(block + LARGE_PREHEADER_SIZE);
        header->size = size;
        header->bucket = BUCKET_LARGE;
        header->magic = HEAP_MAGIC;
        return lc_ok_ptr((uint8_t *)header + HEADER_SIZE);
    }

    lc_heap_local *local = get_local();
    if (__builtin_expect(local == NULL, 0)) return lc_err_ptr(LC_ERR_NOMEM);

    /* Walk our page list, drain remote frees, find a page with free blocks */
    lc_heap_page *page = local->pages[bucket];
    while (page != NULL) {
        drain_remote_frees(page);
        if (page->used_count < page->block_count)
            break;
        page = page->next;
    }

    /* No page with free blocks — adopt an abandoned page of this class that
     * has room, or else get a fresh one */
    if (__builtin_expect(page == NULL, 0)) {
        lc_spinlock_acquire(&heap_global_lock);
        orphan_collect();   /* so that adoption sees the room they make */
        page = page_adopt(bucket, local->tid);
        if (page == NULL) page = get_fresh_page(bucket, local->tid);
        heap_global_unlock();

        if (page == NULL) return lc_err_ptr(LC_ERR_NOMEM);

        /* Prepend to our page list */
        page->next = local->pages[bucket];
        local->pages[bucket] = page;
    }

    /* Allocate from bitmap — no lock, we own this page */
    void *block = bitmap_allocate_block(page);
    if (block == NULL) return lc_err_ptr(LC_ERR_NOMEM);

    lc_heap_header *header = (lc_heap_header *)block;
    header->size = size;
    header->bucket = bucket;
    header->magic = HEAP_MAGIC;

    stat_track_alloc(size);
    return lc_ok_ptr((uint8_t *)block + HEADER_SIZE);
}

lc_result_ptr lc_heap_allocate_zeroed(size_t size) {
    lc_result_ptr alloc = lc_heap_allocate(size);
    if (lc_ptr_is_ok(alloc))
        lc_bytes_fill(alloc.value, 0, size);
    return alloc;
}

void lc_heap_free(void *ptr) {
    if (ptr == NULL) return;

    lc_heap_header *header = (lc_heap_header *)((uint8_t *)ptr - HEADER_SIZE);

    /* Validate magic FIRST (debug) — before dereferencing header->size for
     * stats — so a double-free or corrupt header is reported rather than read. */
#if LC_HEAP_DEBUG
    if (header->magic == HEAP_MAGIC_FREED) {
        lc_kernel_write_bytes(2, "lightc: double-free detected\n", 29);
        lc_kernel_exit(134);
    }
    if (header->magic != HEAP_MAGIC) {
        lc_kernel_write_bytes(2, "lightc: heap corruption detected\n", 33);
        lc_kernel_exit(134);
    }
#endif

    stat_track_free(header->size);

    if (header->bucket == BUCKET_LARGE) {
        /* The mapping base and its TRUE page count live in the large prefix,
         * ahead of the header — use them verbatim so the whole mapping is
         * released, including any tail beyond the requested size. */
        uint8_t *block = (uint8_t *)header - LARGE_PREHEADER_SIZE;
        size_t pages = *(size_t *)block;
#if LC_HEAP_DEBUG
        header->magic = HEAP_MAGIC_FREED;
#endif
        /* Try to cache for reuse instead of unmapping */
        lc_spinlock_acquire(&large_cache_lock);
        if (large_cache_count < LARGE_CACHE_SIZE) {
            large_cache[large_cache_count].ptr = block;
            large_cache[large_cache_count].pages = pages;
            large_cache_count++;
            lc_spinlock_release(&large_cache_lock);
        } else {
            /* Cache full — evict the smallest entry if ours is larger */
            uint32_t smallest = 0;
            for (uint32_t i = 1; i < LARGE_CACHE_SIZE; i++) {
                if (large_cache[i].pages < large_cache[smallest].pages)
                    smallest = i;
            }
            if (pages > large_cache[smallest].pages) {
                void *evict_ptr = large_cache[smallest].ptr;
                size_t evict_pages = large_cache[smallest].pages;
                large_cache[smallest].ptr = block;
                large_cache[smallest].pages = pages;
                lc_spinlock_release(&large_cache_lock);
                lc_free_pages(evict_ptr, evict_pages);
            } else {
                lc_spinlock_release(&large_cache_lock);
                lc_free_pages(block, pages);
            }
        }
        return;
    }

    lc_heap_page *page = page_from_pointer(ptr);
    lc_heap_local *local = get_local(); /* TLS read: 1 instruction */

    if (local == NULL) {
        /* Can't determine ownership — use remote free as fallback */
#if LC_HEAP_DEBUG
        header->magic = HEAP_MAGIC_FREED;
        lc_bytes_fill((uint8_t *)header + HEADER_SIZE, 0xCC, header->size);
#endif
        remote_free(page, header);
        return;
    }

    if (atomic_load_explicit(&page->owner_tid, memory_order_relaxed) == local->tid) {
        /* Same thread — free directly to bitmap, no lock */
        HEAP_CHECK_STATE(page->state == PAGE_OWNED,
                         "lightc: heap block freed as its own into a page the thread does not own\n");
        bitmap_free_block(page, header);
#if LC_HEAP_DEBUG
        header->magic = HEAP_MAGIC_FREED;
        lc_bytes_fill((uint8_t *)header + HEADER_SIZE, 0xCC, header->size);
#endif
        /* Return the page to its segment if it just emptied (keeps footprint
         * bounded; previously these pages were never reclaimed). */
        maybe_reclaim_page(local, page);
    } else {
        /* Different thread — CAS push to remote free list, no lock, or an
         * orphan free when the page is abandoned */
#if LC_HEAP_DEBUG
        header->magic = HEAP_MAGIC_FREED;
        lc_bytes_fill((uint8_t *)header + HEADER_SIZE, 0xCC, header->size);
#endif
        remote_free(page, header);
    }
}

lc_result_ptr lc_heap_reallocate(void *ptr, size_t new_size) {
    if (ptr == NULL) return lc_heap_allocate(new_size);
    if (new_size == 0) {
        lc_heap_free(ptr);
        return lc_ok_ptr(NULL);
    }

    lc_heap_header *header = (lc_heap_header *)((uint8_t *)ptr - HEADER_SIZE);

    /* Validate magic (debug) before trusting the header — matches lc_heap_free. */
#if LC_HEAP_DEBUG
    if (header->magic == HEAP_MAGIC_FREED) {
        lc_kernel_write_bytes(2, "lightc: realloc of freed block\n", 31);
        lc_kernel_exit(134);
    }
    if (header->magic != HEAP_MAGIC) {
        lc_kernel_write_bytes(2, "lightc: heap corruption detected\n", 33);
        lc_kernel_exit(134);
    }
#endif

    size_t old_size = header->size;

    if (header->bucket != BUCKET_LARGE) {
        size_t usable = bucket_usable_size(header->bucket);
        if (new_size <= usable) {
#if LC_STATS
            /* Adjust active bytes for the size change */
            if (new_size > old_size) {
                atomic_fetch_add_explicit(&stat_active_bytes, new_size - old_size, memory_order_relaxed);
                atomic_fetch_add_explicit(&stat_total_bytes_allocated, new_size - old_size, memory_order_relaxed);
            } else {
                atomic_fetch_sub_explicit(&stat_active_bytes, old_size - new_size, memory_order_relaxed);
                atomic_fetch_add_explicit(&stat_total_bytes_freed, old_size - new_size, memory_order_relaxed);
            }
#endif
            header->size = new_size;
            return lc_ok_ptr(ptr);
        }
    }

    /* Large block: resize in place when the new size still belongs to the large
     * class and fits the mapping's capacity — avoids an mmap + copy + munmap for
     * a large-to-large shrink (or a grow that still fits). A shrink into the
     * small-class range falls through to allocate-and-copy, reclaiming memory. */
    if (header->bucket == BUCKET_LARGE) {
        uint8_t *block = (uint8_t *)header - LARGE_PREHEADER_SIZE;
        size_t block_pages = *(size_t *)block;
        size_t capacity = block_pages * LC_PAGE_SIZE - LARGE_HEADER_SIZE;
        if (new_size <= capacity && find_bucket(new_size) == BUCKET_LARGE) {
#if LC_STATS
            if (new_size > old_size) {
                atomic_fetch_add_explicit(&stat_active_bytes, new_size - old_size, memory_order_relaxed);
                atomic_fetch_add_explicit(&stat_total_bytes_allocated, new_size - old_size, memory_order_relaxed);
            } else {
                atomic_fetch_sub_explicit(&stat_active_bytes, old_size - new_size, memory_order_relaxed);
                atomic_fetch_add_explicit(&stat_total_bytes_freed, old_size - new_size, memory_order_relaxed);
            }
#endif
            header->size = new_size;
            return lc_ok_ptr(ptr);
        }
    }

    lc_result_ptr alloc = lc_heap_allocate(new_size);
    if (lc_ptr_is_err(alloc)) return alloc;

    size_t copy_size = old_size < new_size ? old_size : new_size;
    lc_bytes_copy(alloc.value, ptr, copy_size);
    lc_heap_free(ptr);

    return alloc;
}

/* --- Statistics --- */

void lc_heap_get_stats(lc_heap_stats *stats) {
#if LC_STATS
    stats->total_allocations     = atomic_load_explicit(&stat_total_allocs, memory_order_relaxed);
    stats->total_frees           = atomic_load_explicit(&stat_total_frees, memory_order_relaxed);
    stats->total_bytes_allocated = atomic_load_explicit(&stat_total_bytes_allocated, memory_order_relaxed);
    stats->total_bytes_freed     = atomic_load_explicit(&stat_total_bytes_freed, memory_order_relaxed);
    stats->active_allocations    = atomic_load_explicit(&stat_active_allocs, memory_order_relaxed);
    stats->active_bytes          = atomic_load_explicit(&stat_active_bytes, memory_order_relaxed);
    stats->peak_active_allocations = atomic_load_explicit(&stat_peak_allocs, memory_order_relaxed);
    stats->peak_active_bytes     = atomic_load_explicit(&stat_peak_bytes, memory_order_relaxed);
    stats->large_allocations     = atomic_load_explicit(&stat_large_allocs, memory_order_relaxed);
    stats->large_cache_hits      = atomic_load_explicit(&stat_large_cache_hits, memory_order_relaxed);
    stats->heap_pages_reclaimed  = atomic_load_explicit(&stat_pages_reclaimed, memory_order_relaxed);

    /* Read current large cache count under lock */
    lc_spinlock_acquire(&large_cache_lock);
    stats->large_cache_count = large_cache_count;
    lc_spinlock_release(&large_cache_lock);
#else
    lc_bytes_fill(stats, 0, sizeof(*stats));
    /* Still report large cache count — it's always available */
    lc_spinlock_acquire(&large_cache_lock);
    stats->large_cache_count = large_cache_count;
    lc_spinlock_release(&large_cache_lock);
#endif
}

void lc_heap_reset_stats(void) {
#if LC_STATS
    atomic_store_explicit(&stat_total_allocs, 0, memory_order_relaxed);
    atomic_store_explicit(&stat_total_frees, 0, memory_order_relaxed);
    atomic_store_explicit(&stat_total_bytes_allocated, 0, memory_order_relaxed);
    atomic_store_explicit(&stat_total_bytes_freed, 0, memory_order_relaxed);
    atomic_store_explicit(&stat_peak_allocs,
        atomic_load_explicit(&stat_active_allocs, memory_order_relaxed),
        memory_order_relaxed);
    atomic_store_explicit(&stat_peak_bytes,
        atomic_load_explicit(&stat_active_bytes, memory_order_relaxed),
        memory_order_relaxed);
    atomic_store_explicit(&stat_large_allocs, 0, memory_order_relaxed);
    atomic_store_explicit(&stat_large_cache_hits, 0, memory_order_relaxed);
    atomic_store_explicit(&stat_pages_reclaimed, 0, memory_order_relaxed);
#endif
}

#ifndef LIGHTC_HEAP_H
#define LIGHTC_HEAP_H

#include "types.h"

/*
 * Heap allocator — individual alloc/free.
 *
 * Small allocations (block ≤ 4096 bytes): a per-thread bucket allocator over
 * 64 KiB slab pages carved from 4 MiB segments, with lock-free cross-thread
 * frees. Large allocations: a dedicated mmap, kept in a small reuse cache on
 * free rather than unmapped immediately. Every allocation is 16-byte aligned.
 *
 * Each allocation has a hidden 16-byte header storing its size class, so
 * lc_heap_free knows how to return it. See docs/heap.html for the full design.
 *
 * A thread of lc_thread_create gives its heap state back when its function
 * returns. The blocks it allocated stay valid, and any thread may free them.
 * A page of such a thread is adopted by another thread's allocation of its
 * size class once a quarter of its blocks are free, and goes back to its
 * segment when its last block is freed: at that free, or, when another
 * thread holds the heap's lock just then, by the time that thread has
 * released it, as each holder of the lock applies such frees before it
 * releases it and looks for more after.
 */

/* Allocate `size` bytes. */
[[nodiscard, gnu::hot]]
lc_result_ptr lc_heap_allocate(size_t size);

/* Allocate `size` bytes, zero-filled. */
[[nodiscard, gnu::hot]]
lc_result_ptr lc_heap_allocate_zeroed(size_t size);

/* Resize an allocation. On failure, original is untouched. */
[[nodiscard, gnu::hot]]
lc_result_ptr lc_heap_reallocate(void *ptr, size_t new_size);

/* Free a heap allocation. NULL is safe to pass. */
[[gnu::hot]]
void lc_heap_free(void *ptr);

/* --- Statistics ---
 *
 * Controlled by LC_STATS. Default: on in debug (no NDEBUG), off in release.
 * Override: compile with -DLC_STATS=1 to force on, -DLC_STATS=0 to force off.
 *
 * When disabled, lc_heap_get_stats() zeroes the struct and
 * lc_heap_reset_stats() is a no-op. Zero overhead on the hot path.
 */

#ifndef LC_STATS
# ifdef NDEBUG
#  define LC_STATS 0
# else
#  define LC_STATS 1
# endif
#endif

typedef struct {
    /* Counters — cumulative since process start */
    uint64_t total_allocations;       /* total calls to allocate */
    uint64_t total_frees;             /* total calls to free */
    uint64_t total_bytes_allocated;   /* sum of all requested sizes */
    uint64_t total_bytes_freed;       /* sum of all freed sizes */

    /* Current state */
    uint64_t active_allocations;      /* allocations not yet freed */
    uint64_t active_bytes;            /* bytes currently in use (requested sizes) */

    /* Peak */
    uint64_t peak_active_allocations; /* max active_allocations ever seen */
    uint64_t peak_active_bytes;       /* max active_bytes ever seen */

    /* Large allocation tracking */
    uint64_t large_allocations;       /* total large (mmap) allocations */
    uint64_t large_cache_hits;        /* large allocs served from cache */
    uint64_t large_cache_count;       /* current entries in the large cache */

    /* Small-slab reclamation */
    uint64_t heap_pages_reclaimed;    /* cumulative emptied slab pages returned */
} lc_heap_stats;

/* Collect current heap statistics (zeroes struct when LC_STATS=0). */
void lc_heap_get_stats(lc_heap_stats *stats);

/* Reset cumulative counters (no-op when LC_STATS=0). */
void lc_heap_reset_stats(void);

/* Return per-thread TLS slot array (used by lc_tls_get/set). A thread's
 * slots start NULL, also when it reuses the id of a thread that has exited.
 * The array is valid until the thread exits: a thread of lc_thread_create
 * gives it back, with the rest of its heap state, when its function
 * returns, so no other thread may keep a pointer into it past then. */
void **lc_heap_tls_slots(void);

#endif /* LIGHTC_HEAP_H */

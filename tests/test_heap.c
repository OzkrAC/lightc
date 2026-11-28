/*
 * test_heap.c — tests for lightc heap allocator.
 */

#include "test.h"
#include <lightc/heap.h>
#include <lightc/signal.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

/* ===== lc_heap_allocate — basic ===== */

static void test_heap_allocate_basic(void) {
    lc_result_ptr r = lc_heap_allocate(64);
    TEST_ASSERT_PTR_OK(r);
    void *ptr = r.value;

    /* Write and read back */
    uint8_t *bytes = (uint8_t *)ptr;
    for (int i = 0; i < 64; i++) {
        bytes[i] = (uint8_t)(i * 3);
    }
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQ(bytes[i], (uint8_t)(i * 3));
    }

    lc_heap_free(ptr);
}

/* ===== lc_heap_allocate with size 0 ===== */

static void test_heap_allocate_zero(void) {
    lc_result_ptr r = lc_heap_allocate(0);
    TEST_ASSERT_PTR_OK(r);
    lc_heap_free(r.value);
}

/* ===== lc_heap_allocate_zeroed ===== */

static void test_heap_allocate_zeroed(void) {
    lc_result_ptr r = lc_heap_allocate_zeroed(128);
    TEST_ASSERT_PTR_OK(r);
    void *ptr = r.value;

    uint8_t *bytes = (uint8_t *)ptr;
    for (int i = 0; i < 128; i++) {
        TEST_ASSERT_EQ(bytes[i], (uint8_t)0);
    }

    lc_heap_free(ptr);
}

/* ===== lc_heap_free — free and re-allocate ===== */

static void test_heap_free_and_realloc(void) {
    lc_result_ptr r = lc_heap_allocate(64);
    TEST_ASSERT_PTR_OK(r);
    lc_heap_free(r.value);

    /* Re-allocate — should not crash */
    lc_result_ptr r2 = lc_heap_allocate(64);
    TEST_ASSERT_PTR_OK(r2);
    lc_heap_free(r2.value);
}

/* ===== lc_heap_free(NULL) ===== */

static void test_heap_free_null(void) {
    /* Should not crash */
    lc_heap_free(NULL);
}

/* ===== lc_heap_reallocate — grow ===== */

static void test_heap_reallocate_grow(void) {
    lc_result_ptr r = lc_heap_allocate(16);
    TEST_ASSERT_PTR_OK(r);
    void *ptr = r.value;

    /* Write data */
    uint8_t *bytes = (uint8_t *)ptr;
    for (int i = 0; i < 16; i++) {
        bytes[i] = (uint8_t)(i + 1);
    }

    /* Grow */
    lc_result_ptr r2 = lc_heap_reallocate(ptr, 256);
    TEST_ASSERT_PTR_OK(r2);
    void *ptr2 = r2.value;

    /* Original data preserved */
    uint8_t *bytes2 = (uint8_t *)ptr2;
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQ(bytes2[i], (uint8_t)(i + 1));
    }

    lc_heap_free(ptr2);
}

/* ===== lc_heap_reallocate — shrink ===== */

static void test_heap_reallocate_shrink(void) {
    lc_result_ptr r = lc_heap_allocate(256);
    TEST_ASSERT_PTR_OK(r);
    void *ptr = r.value;

    /* Write data */
    uint8_t *bytes = (uint8_t *)ptr;
    for (int i = 0; i < 256; i++) {
        bytes[i] = (uint8_t)(i & 0xFF);
    }

    /* Shrink */
    lc_result_ptr r2 = lc_heap_reallocate(ptr, 32);
    TEST_ASSERT_PTR_OK(r2);
    void *ptr2 = r2.value;

    /* First 32 bytes preserved */
    uint8_t *bytes2 = (uint8_t *)ptr2;
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQ(bytes2[i], (uint8_t)(i & 0xFF));
    }

    lc_heap_free(ptr2);
}

/* ===== lc_heap_reallocate(NULL, size) — acts as allocate ===== */

static void test_heap_reallocate_null(void) {
    lc_result_ptr r = lc_heap_reallocate(NULL, 64);
    TEST_ASSERT_PTR_OK(r);
    void *ptr = r.value;

    /* Write and read */
    uint8_t *bytes = (uint8_t *)ptr;
    bytes[0] = 0xAA;
    bytes[63] = 0xBB;
    TEST_ASSERT_EQ(bytes[0], (uint8_t)0xAA);
    TEST_ASSERT_EQ(bytes[63], (uint8_t)0xBB);

    lc_heap_free(ptr);
}

/* ===== lc_heap_reallocate(ptr, 0) — acts as free, returns NULL ===== */

static void test_heap_reallocate_to_zero(void) {
    lc_result_ptr r = lc_heap_allocate(64);
    TEST_ASSERT_PTR_OK(r);

    lc_result_ptr r2 = lc_heap_reallocate(r.value, 0);
    TEST_ASSERT_NULL(r2.value);
}

/* ===== Various bucket sizes ===== */

static void test_heap_bucket_sizes(void) {
    static const size_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096};

    for (int i = 0; i < 8; i++) {
        size_t sz = sizes[i];
        lc_result_ptr r = lc_heap_allocate(sz);
        TEST_ASSERT_PTR_OK(r);
        void *ptr = r.value;

        /* Write first and last byte */
        uint8_t *bytes = (uint8_t *)ptr;
        bytes[0] = 0xDE;
        bytes[sz - 1] = 0xAD;
        TEST_ASSERT_EQ(bytes[0], (uint8_t)0xDE);
        TEST_ASSERT_EQ(bytes[sz - 1], (uint8_t)0xAD);

        lc_heap_free(ptr);
    }
}

/* ===== Large allocation (mmap path) ===== */

static void test_heap_large_allocation(void) {
    /* Allocate > 4096 to trigger the BUCKET_LARGE / mmap path */
    size_t sz = 8192;
    lc_result_ptr r = lc_heap_allocate(sz);
    TEST_ASSERT_PTR_OK(r);
    void *ptr = r.value;

    /* Write and read across the allocation */
    uint8_t *bytes = (uint8_t *)ptr;
    for (size_t i = 0; i < sz; i++) {
        bytes[i] = (uint8_t)(i & 0xFF);
    }
    for (size_t i = 0; i < sz; i++) {
        TEST_ASSERT_EQ(bytes[i], (uint8_t)(i & 0xFF));
    }

    lc_heap_free(ptr);
}

/* ===== H2 regression: large cache must not leak a reused mapping's tail ===== */

/*
 * Best-fit reuse can serve a large request from a bigger cached mapping. The
 * bug: the header recorded only the requested size, so free recomputed a
 * smaller page count and orphaned (leaked) the mapping's tail pages — the
 * mapping could then never again satisfy a request as large as its true size.
 *
 * Detected by pointer identity (build-independent): a cached mapping is reused
 * at the same base address. We cache one big mapping, reuse it for a smaller
 * request, free it, then re-request the original big size. If the true size
 * survived in the cache the same mapping comes back; if the tail leaked, the
 * cache entry shrank, the big request can't reuse it, and a different mapping
 * is returned. Sizes are chosen (33 vs 17 pages) so nothing else the suite
 * leaves cached (<= a few pages) interferes with best-fit selection.
 */
static void test_heap_large_cache_no_tail_leak(void) {
    const size_t big   = 32 * 4096;  /* needs 33 pages including the header */
    const size_t small = 16 * 4096;  /* needs 17 pages including the header */

    /* Cache a big mapping. */
    lc_result_ptr a = lc_heap_allocate(big);
    TEST_ASSERT_PTR_OK(a);
    void *base = a.value;
    lc_heap_free(a.value);

    /* Reuse it for a smaller request; same base confirms the size-mismatched
     * reuse actually happened (precondition for the leak to matter). */
    lc_result_ptr b = lc_heap_allocate(small);
    TEST_ASSERT_PTR_OK(b);
    TEST_ASSERT_EQ((uintptr_t)b.value, (uintptr_t)base);
    lc_heap_free(b.value);

    /* Re-request the original big size. With the tail intact the same mapping
     * is reused; if it leaked, a fresh (different) mapping comes back. */
    lc_result_ptr c = lc_heap_allocate(big);
    TEST_ASSERT_PTR_OK(c);
    TEST_ASSERT_EQ((uintptr_t)c.value, (uintptr_t)base);

    /* The full big extent must be writable — proves the mapping is really big. */
    uint8_t *bytes = (uint8_t *)c.value;
    bytes[0] = 0x11;
    bytes[big - 1] = 0x22;
    TEST_ASSERT_EQ(bytes[0], (uint8_t)0x11);
    TEST_ASSERT_EQ(bytes[big - 1], (uint8_t)0x22);

    lc_heap_free(c.value);
}

/* ===== Integer-overflow hardening (H3) ===== */

static void test_heap_allocate_size_overflow(void) {
    /* size + HEADER_SIZE wraps: an unchecked find_bucket maps SIZE_MAX to
     * bucket 0 and returns a 32-byte block claiming SIZE_MAX usable bytes —
     * a guaranteed heap overflow at first use. Must fail cleanly. */
    lc_result_ptr r = lc_heap_allocate(SIZE_MAX);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_NOMEM);

    /* Wraps only after adding the header — same failure mode. */
    r = lc_heap_allocate(SIZE_MAX - 8);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_NOMEM);

    /* Huge but non-wrapping: takes the large path, page round-up must not
     * wrap either; mmap rejects it and the error propagates cleanly. */
    r = lc_heap_allocate(SIZE_MAX / 2);
    TEST_ASSERT_PTR_ERR(r);
    TEST_ASSERT_EQ(r.error, LC_ERR_NOMEM);

    /* Reallocate to a wrapping size must fail and leave the block intact. */
    lc_result_ptr small = lc_heap_allocate(64);
    TEST_ASSERT_PTR_OK(small);
    uint8_t *bytes = (uint8_t *)small.value;
    bytes[0] = 0xAB;
    bytes[63] = 0xCD;

    lc_result_ptr grown = lc_heap_reallocate(small.value, SIZE_MAX);
    TEST_ASSERT_PTR_ERR(grown);
    TEST_ASSERT_EQ(bytes[0], (uint8_t)0xAB);
    TEST_ASSERT_EQ(bytes[63], (uint8_t)0xCD);

    lc_heap_free(small.value);
}

/* ===== M8: every allocation is 16-byte aligned ===== */

/*
 * The heap documents 16-byte alignment (SSE / _Atomic __int128). A block starts
 * 16-aligned and the user pointer is block + 16-byte header, so the stride (the
 * bucket size) must be a multiple of 16 or odd-indexed blocks come back
 * 8-mod-16. Allocate many blocks of each small size consecutively (filling
 * successive indices in a page, so odd indices are exercised) and require every
 * returned pointer to be 16-aligned. Sizes 24 and 40 map to the buckets whose
 * strides were formerly 40 and 56 — the ones that misaligned.
 */
#define ALIGN_BATCH 64
static void *align_ptrs[ALIGN_BATCH];

static void test_heap_block_alignment(void) {
    static const size_t sizes[] = { 1, 8, 16, 24, 32, 40, 48, 56, 64, 100, 200, 256 };
    const size_t nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t s = 0; s < nsizes; s++) {
        for (int i = 0; i < ALIGN_BATCH; i++) {
            lc_result_ptr r = lc_heap_allocate(sizes[s]);
            TEST_ASSERT_PTR_OK(r);
            align_ptrs[i] = r.value;
            TEST_ASSERT_EQ(((uintptr_t)r.value) & 15u, (uintptr_t)0);
        }
        for (int i = 0; i < ALIGN_BATCH; i++) {
            lc_heap_free(align_ptrs[i]);
        }
    }
}

/* ===== M7: emptied slab pages are reclaimed ===== */

/*
 * Fill several 64 KiB slab pages of one bucket, then free everything. Each page
 * that empties (and is not the last one kept resident per bucket) is returned
 * to its segment — previously page_reclaim was dead code and the footprint grew
 * monotonically. We assert the reclaim counter advanced, and that the heap is
 * still fully usable afterwards (reclaimed pages/segments get reused cleanly).
 */
#define RECLAIM_BLOCKS 20000
static void *reclaim_ptrs[RECLAIM_BLOCKS];

static void test_heap_page_reclaim(void) {
    lc_heap_stats before, after;
    lc_heap_get_stats(&before);

    /* Hold the allocations so the pages genuinely fill (used_count stays high)
     * and span multiple slab pages. */
    for (int i = 0; i < RECLAIM_BLOCKS; i++) {
        lc_result_ptr r = lc_heap_allocate(48);
        TEST_ASSERT_PTR_OK(r);
        reclaim_ptrs[i] = r.value;
    }
    for (int i = 0; i < RECLAIM_BLOCKS; i++) {
        lc_heap_free(reclaim_ptrs[i]);
    }

    lc_heap_get_stats(&after);

#if LC_STATS
    /* The spanned pages emptied; all but one-per-bucket must have been reclaimed. */
    TEST_ASSERT(after.heap_pages_reclaimed > before.heap_pages_reclaimed);
#endif

    /* Heap still works: allocate and use a block (exercises reuse of reclaimed
     * pages / a freshly re-created segment). */
    lc_result_ptr r = lc_heap_allocate(48);
    TEST_ASSERT_PTR_OK(r);
    uint8_t *b = (uint8_t *)r.value;
    b[0] = 0x7E;
    b[47] = 0x7F;
    TEST_ASSERT_EQ(b[0], (uint8_t)0x7E);
    TEST_ASSERT_EQ(b[47], (uint8_t)0x7F);
    lc_heap_free(r.value);
}

/* ===== Alloc/free cycles ===== */

static void test_heap_alloc_free_cycles(void) {
    for (int i = 0; i < 100; i++) {
        lc_result_ptr r = lc_heap_allocate(64);
        TEST_ASSERT_PTR_OK(r);
        void *ptr = r.value;

        /* Touch the memory */
        uint8_t *bytes = (uint8_t *)ptr;
        bytes[0] = (uint8_t)i;

        lc_heap_free(ptr);
    }
}

/* ===== Heap stress — rapid alloc/free of many small blocks ===== */

static void test_heap_stress(void) {
    /* Allocate many blocks, then free them all */
    #define STRESS_COUNT 200
    void *ptrs[STRESS_COUNT];

    for (int i = 0; i < STRESS_COUNT; i++) {
        lc_result_ptr r = lc_heap_allocate(16);
        TEST_ASSERT_PTR_OK(r);
        ptrs[i] = r.value;

        /* Write a marker */
        uint8_t *bytes = (uint8_t *)ptrs[i];
        bytes[0] = (uint8_t)(i & 0xFF);
    }

    /* Verify markers */
    for (int i = 0; i < STRESS_COUNT; i++) {
        uint8_t *bytes = (uint8_t *)ptrs[i];
        TEST_ASSERT_EQ(bytes[0], (uint8_t)(i & 0xFF));
    }

    /* Free all */
    for (int i = 0; i < STRESS_COUNT; i++) {
        lc_heap_free(ptrs[i]);
    }

    /* Allocate again to verify allocator still works */
    for (int i = 0; i < STRESS_COUNT; i++) {
        lc_result_ptr r = lc_heap_allocate(16);
        TEST_ASSERT_PTR_OK(r);
        ptrs[i] = r.value;
    }
    for (int i = 0; i < STRESS_COUNT; i++) {
        lc_heap_free(ptrs[i]);
    }

    #undef STRESS_COUNT
}

/* ===== Statistics ===== */

static void test_heap_stats(void) {
    lc_heap_stats after;

    /* API should work regardless of LC_STATS */
    lc_heap_reset_stats();

    void *a = lc_heap_allocate(100).value;
    void *b = lc_heap_allocate(200).value;
    void *c = lc_heap_allocate(300).value;

#if LC_STATS
    lc_heap_get_stats(&after);
    TEST_ASSERT(after.total_allocations >= 3);
    TEST_ASSERT(after.active_allocations >= 3);
    TEST_ASSERT(after.active_bytes >= 600);
    TEST_ASSERT(after.total_bytes_allocated >= 600);

    lc_heap_free(b);
    lc_heap_get_stats(&after);
    TEST_ASSERT(after.total_frees >= 1);
    TEST_ASSERT(after.active_bytes >= 400);

    /* Peak should reflect the 3-allocation high point */
    TEST_ASSERT(after.peak_active_allocations >= 3);
    TEST_ASSERT(after.peak_active_bytes >= 600);

    /* Large allocation tracking */
    void *big = lc_heap_allocate(16384).value;
    lc_heap_get_stats(&after);
    TEST_ASSERT(after.large_allocations >= 1);
    lc_heap_free(big);

    /* Cache hit on second large alloc of same size */
    void *big2 = lc_heap_allocate(16384).value;
    lc_heap_get_stats(&after);
    TEST_ASSERT(after.large_cache_hits >= 1);
    lc_heap_free(big2);
#else
    /* When disabled, get_stats should still work (zeroed counters) */
    lc_heap_get_stats(&after);
    TEST_ASSERT_EQ(after.total_allocations, (uint64_t)0);
    TEST_ASSERT_EQ(after.active_allocations, (uint64_t)0);
    lc_heap_free(b);
#endif

    lc_heap_free(a);
    lc_heap_free(c);
}

/* ===== main ===== */

/* A large block shrunk to a still-large size is resized in place — same pointer,
 * no mmap/copy/munmap — and its contents are preserved. */
static void test_heap_reallocate_large_inplace(void) {
    lc_result_ptr a = lc_heap_allocate(100000);   /* large: block > 4096 */
    TEST_ASSERT_PTR_OK(a);
    void *orig = a.value;
    uint8_t *p = (uint8_t *)orig;
    p[0] = 0xA5;
    p[60000] = 0x5A;

    lc_result_ptr s = lc_heap_reallocate(orig, 70000);   /* still large, fits */
    TEST_ASSERT_PTR_OK(s);
    TEST_ASSERT_EQ((uintptr_t)s.value, (uintptr_t)orig);  /* in place */
    uint8_t *q = (uint8_t *)s.value;
    TEST_ASSERT_EQ(q[0], (uint8_t)0xA5);
    TEST_ASSERT_EQ(q[60000], (uint8_t)0x5A);

    lc_heap_free(s.value);
}

/* ===== Thread exit: the pages of threads that have exited ===== */

/*
 * A thread that exits holding blocks abandons their pages, and another
 * thread's allocation of the same size class adopts such a page before it
 * takes a fresh one. A page is 64 KiB and aligned to its size, so two
 * blocks share a page when their addresses agree above the low 16 bits.
 * Each test uses a size class of its own, which no other test of this
 * suite allocates from a thread, so the abandoned pages it sees are its own.
 */
#define EXIT_PAGE_SHIFT 16

static bool same_page(const void *a, const void *b) {
    return ((uintptr_t)a >> EXIT_PAGE_SHIFT) == ((uintptr_t)b >> EXIT_PAGE_SHIFT);
}

typedef struct {
    size_t   size;
    uint32_t count;     /* how many blocks to allocate, up to EXIT_MAX_BLOCKS */
    bool     free_them; /* free them before returning, or leave them to main */
    void    *blocks[64];
    uint32_t got;
} exit_work;

#define EXIT_MAX_BLOCKS 64

static int32_t exit_worker(void *arg) {
    exit_work *w = arg;
    w->got = 0;
    for (uint32_t i = 0; i < w->count && i < EXIT_MAX_BLOCKS; i++) {
        lc_result_ptr r = lc_heap_allocate(w->size);
        if (lc_ptr_is_err(r)) break;
        w->blocks[i] = r.value;
        ((uint8_t *)r.value)[0] = (uint8_t)i;
        w->got++;
    }
    if (w->free_them) {
        for (uint32_t i = 0; i < w->got; i++) lc_heap_free(w->blocks[i]);
    }
    return 0;
}

/* Runs exit_worker on `w` in a thread, and returns when it has exited. */
static bool run_exit_worker(exit_work *w) {
    lc_thread thread;
    if (lc_is_err(lc_thread_create(&thread, exit_worker, w))) return false;
    lc_thread_join(&thread);
    return w->got == w->count;
}

static void test_heap_exit_adopts_abandoned_page(void) {
    exit_work first = { .size = 3000, .count = 1, .free_them = false };
    TEST_REQUIRE(run_exit_worker(&first));
    exit_work second = { .size = 3000, .count = 1, .free_them = false };
    TEST_REQUIRE(run_exit_worker(&second));

    /* The second thread allocated from the page the first abandoned. */
    TEST_ASSERT(first.blocks[0] != second.blocks[0]);
    TEST_ASSERT(same_page(first.blocks[0], second.blocks[0]));
    TEST_ASSERT_EQ(((uint8_t *)first.blocks[0])[0], (uint8_t)0);

    /* Freed from here, a thread that does not own the page. */
    lc_heap_free(first.blocks[0]);
    lc_heap_free(second.blocks[0]);
}

/*
 * 65536 / 1536 is 42: a thread that allocates 42 blocks of 1500 bytes, of
 * the 1536-byte class, from a fresh page fills it. Abandoned full, it is
 * adopted only once a quarter of its blocks, 10, are free: with one free
 * block, the next thread of the class takes a fresh page, as adopting it
 * would give that thread one block and a full page to walk past. With 10
 * free, the next thread adopts it, and gets the lowest of them.
 */
#define FULL_SIZE    1500
#define FULL_BLOCKS  42
#define FULL_QUARTER (FULL_BLOCKS / 4)
#define FULL_FIRST   21   /* the blocks main frees start here */

static void test_heap_exit_adopts_only_a_page_with_room(void) {
    exit_work filler = { .size = FULL_SIZE, .count = FULL_BLOCKS, .free_them = false };
    TEST_REQUIRE(run_exit_worker(&filler));
    for (uint32_t i = 1; i < FULL_BLOCKS; i++)
        TEST_REQUIRE(same_page(filler.blocks[0], filler.blocks[i]));

    lc_heap_free(filler.blocks[FULL_FIRST]);   /* one block free: no room */
    exit_work other = { .size = FULL_SIZE, .count = 1, .free_them = true };
    TEST_REQUIRE(run_exit_worker(&other));
    TEST_ASSERT(!same_page(filler.blocks[0], other.blocks[0]));

    for (uint32_t i = FULL_FIRST + 1; i < FULL_FIRST + FULL_QUARTER; i++)
        lc_heap_free(filler.blocks[i]);        /* a quarter free: room */
    exit_work adopter = { .size = FULL_SIZE, .count = 1, .free_them = false };
    TEST_REQUIRE(run_exit_worker(&adopter));
    TEST_ASSERT_EQ(adopter.blocks[0], filler.blocks[FULL_FIRST]);

    for (uint32_t i = 0; i < FULL_BLOCKS; i++)
        if (i < FULL_FIRST || i >= FULL_FIRST + FULL_QUARTER) lc_heap_free(filler.blocks[i]);
    lc_heap_free(adopter.blocks[0]);
}

/* A thread's exit returns its empty pages, the one it keeps resident per
 * class too. Frees from another thread that empty an abandoned page return
 * it at the last of them, here from main, which takes no exit. Only builds
 * with LC_STATS count the pages. */
static void test_heap_exit_reclaims_pages(void) {
    lc_heap_stats before, after;
    lc_heap_get_stats(&before);
    exit_work clean = { .size = 700, .count = 8, .free_them = true };
    TEST_REQUIRE(run_exit_worker(&clean));
    lc_heap_get_stats(&after);
#if LC_STATS
    TEST_ASSERT(after.heap_pages_reclaimed >= before.heap_pages_reclaimed + 1);
#endif

    exit_work held = { .size = 700, .count = 8, .free_them = false };
    TEST_REQUIRE(run_exit_worker(&held));
    lc_heap_get_stats(&before);
    for (uint32_t i = 0; i < held.got; i++) lc_heap_free(held.blocks[i]);
    lc_heap_get_stats(&after);
#if LC_STATS
    TEST_ASSERT(after.heap_pages_reclaimed >= before.heap_pages_reclaimed + 1);
#endif

    /* A page that was given back is used again. */
    exit_work again = { .size = 700, .count = 8, .free_them = true };
    TEST_ASSERT(run_exit_worker(&again));
}

/*
 * A free into an abandoned page that finds the heap's lock taken is applied
 * by the time the thread that held the lock has released it, with no thread
 * exit and no later free. The test holds the lock in a known state: a busy
 * thread takes it over and over, in the 3584-byte class (each round
 * allocates 19 blocks, so that the 19th takes a fresh page, and frees them,
 * so that the page it filled goes back: one reclaimed page a round), and a
 * SIGUSR1, which only that thread does not block, stops it in a handler
 * that waits until main lets it go. Each attempt prepares 8 abandoned pages
 * of the 4096-byte class with one block left in each, stops the busy
 * thread, and frees the 8 blocks from main. As no other thread runs then,
 * either all 8 frees found the lock free and their pages came back at once,
 * and the attempt is made again, or the busy thread stopped holding the
 * lock and none did: then main lets it go and stops it after its round,
 * and the 8 pages must be back, besides the at most one page of its own
 * that the round returned. Only builds with LC_STATS count the pages; under
 * the faultinject build's chaos points the busy thread holds the lock
 * longer.
 */
#if LC_STATS
#define WAIT_SIGNAL    10      /* SIGUSR1, on both arches */
#define PROBE_SIZE     4000
#define PROBE_PAGES    8
#define BUSY_SIZE      3500
#define BUSY_BLOCKS    19
#define WAIT_ATTEMPTS  5000    /* a few percent of attempts stop it in the lock */
#define WAIT_TOTAL_MS  30000   /* and a bound on them all, for a slow emulator */
#define WAIT_LIMIT_MS  10000

static void *probe_blocks[PROBE_PAGES];
static _Atomic(int32_t) busy_frozen;     /* the busy thread is in the handler */
static _Atomic(int32_t) busy_thaw;       /* main lets it go */
static _Atomic(int32_t) busy_stop;       /* main asks it to stop after its round */
static _Atomic(int32_t) busy_stopped;    /* it has, and waits for busy_exit */
static _Atomic(int32_t) busy_exit;

static bool wait_flag(_Atomic(int32_t) *flag, int32_t value) {
    int64_t start = lc_time_start_timer();
    while (atomic_load_explicit(flag, memory_order_acquire) != value) {
        if (lc_time_elapsed_milliseconds(start) > WAIT_LIMIT_MS) return false;
        (void)lc_syscall0(SYS_sched_yield);
    }
    return true;
}

static void busy_freeze(int signo) {
    (void)signo;
    atomic_store_explicit(&busy_frozen, 1, memory_order_release);
    (void)wait_flag(&busy_thaw, 1);
    atomic_store_explicit(&busy_frozen, 0, memory_order_release);
}

static int32_t busy_locker(void *arg) {
    (void)arg;
    void *blocks[BUSY_BLOCKS];
    if (lc_is_err(lc_signal_unblock(WAIT_SIGNAL))) return 1;
    while (atomic_load_explicit(&busy_stop, memory_order_acquire) == 0) {
        for (int i = 0; i < BUSY_BLOCKS; i++) {
            lc_result_ptr r = lc_heap_allocate(BUSY_SIZE);
            blocks[i] = lc_ptr_is_ok(r) ? r.value : NULL;
        }
        for (int i = 0; i < BUSY_BLOCKS; i++) lc_heap_free(blocks[i]);
    }
    atomic_store_explicit(&busy_stopped, 1, memory_order_release);
    (void)wait_flag(&busy_exit, 1);
    return 0;
}

/* Leaves PROBE_PAGES abandoned pages of the 4096-byte class with one block
 * each in probe_blocks: 16 blocks fill a page, and it frees 15 of each. */
static int32_t probe_holder(void *arg) {
    int32_t *ok = arg;
    static void *blocks[PROBE_PAGES][16];
    *ok = 1;
    for (int p = 0; p < PROBE_PAGES; p++) {
        for (int i = 0; i < 16; i++) {
            lc_result_ptr r = lc_heap_allocate(PROBE_SIZE);
            blocks[p][i] = lc_ptr_is_ok(r) ? r.value : NULL;
            if (blocks[p][i] == NULL || !same_page(blocks[p][i], blocks[p][0])) *ok = 0;
        }
    }
    for (int p = 0; p < PROBE_PAGES; p++) {
        probe_blocks[p] = blocks[p][0];
        for (int i = 1; i < 16; i++) lc_heap_free(blocks[p][i]);
    }
    return 0;
}

#endif /* LC_STATS */

static void test_heap_waiting_free_is_applied_without_an_exit(void) {
#if LC_STATS
    TEST_REQUIRE_OK(lc_signal_block(WAIT_SIGNAL));   /* every thread but the busy one */
    TEST_REQUIRE_OK(lc_signal_handle(WAIT_SIGNAL, busy_freeze));
    atomic_store(&busy_stop, 0);
    atomic_store(&busy_stopped, 0);
    atomic_store(&busy_exit, 0);
    lc_thread busy;
    TEST_REQUIRE_OK(lc_thread_create(&busy, busy_locker, NULL));

    int attempts = 0, waited = 0;
    uint64_t at_once = 0, afterwards = 0;
    bool consistent = true;
    int64_t started = lc_time_start_timer();
    while (attempts < WAIT_ATTEMPTS && !waited &&
           lc_time_elapsed_milliseconds(started) < WAIT_TOTAL_MS) {
        attempts++;
        int32_t ok = 0;
        lc_thread holder;
        TEST_REQUIRE_OK(lc_thread_create(&holder, probe_holder, &ok));
        lc_thread_join(&holder);
        TEST_REQUIRE(ok);

        atomic_store(&busy_thaw, 0);
        TEST_REQUIRE(lc_kernel_send_signal(lc_kernel_get_process_id(), WAIT_SIGNAL) == 0);
        TEST_REQUIRE(wait_flag(&busy_frozen, 1));
        lc_heap_stats s0, s1, s2;
        lc_heap_get_stats(&s0);
        for (int p = 0; p < PROBE_PAGES; p++) lc_heap_free(probe_blocks[p]);
        lc_heap_get_stats(&s1);
        at_once = s1.heap_pages_reclaimed - s0.heap_pages_reclaimed;
        if (at_once != 0) {
            /* The lock was free: the busy thread stopped outside it. */
            if (at_once != PROBE_PAGES) consistent = false;
            atomic_store_explicit(&busy_thaw, 1, memory_order_release);
            TEST_REQUIRE(wait_flag(&busy_frozen, 0));
            continue;
        }
        waited = 1;
        atomic_store_explicit(&busy_stop, 1, memory_order_release);
        atomic_store_explicit(&busy_thaw, 1, memory_order_release);
        TEST_REQUIRE(wait_flag(&busy_stopped, 1));
        lc_heap_get_stats(&s2);
        afterwards = s2.heap_pages_reclaimed - s1.heap_pages_reclaimed;
    }
    atomic_store_explicit(&busy_stop, 1, memory_order_release);
    atomic_store_explicit(&busy_thaw, 1, memory_order_release);
    TEST_ASSERT(wait_flag(&busy_stopped, 1));
    atomic_store_explicit(&busy_exit, 1, memory_order_release);
    lc_thread_join(&busy);
    TEST_ASSERT_OK(lc_signal_reset(WAIT_SIGNAL));
    TEST_ASSERT_OK(lc_signal_unblock(WAIT_SIGNAL));

    _test_print_text("  ");
    lc_print_signed(TEST_FD, attempts);
    _test_print_text(waited ? " attempts until the 8 frees waited; then "
                            : " attempts, and the 8 frees never waited; ");
    lc_print_unsigned(TEST_FD, afterwards);
    _test_print_text(" pages came back by the time the busy thread let the lock go\n");
    TEST_ASSERT(consistent);
    TEST_ASSERT(waited);
    TEST_ASSERT(afterwards >= PROBE_PAGES && afterwards <= PROBE_PAGES + 1);
#endif
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    test_chaos_init(envp);   /* LC_CHAOS, in the faultinject build */

    /* lc_heap_allocate — basic */
    TEST_RUN(test_heap_allocate_basic);

    /* lc_heap_allocate with size 0 */
    TEST_RUN(test_heap_allocate_zero);

    /* lc_heap_allocate_zeroed */
    TEST_RUN(test_heap_allocate_zeroed);

    /* lc_heap_free — free and re-allocate */
    TEST_RUN(test_heap_free_and_realloc);

    /* lc_heap_free(NULL) */
    TEST_RUN(test_heap_free_null);

    /* lc_heap_reallocate — grow */
    TEST_RUN(test_heap_reallocate_grow);

    /* lc_heap_reallocate — shrink */
    TEST_RUN(test_heap_reallocate_shrink);

    /* lc_heap_reallocate(NULL, size) — acts as allocate */
    TEST_RUN(test_heap_reallocate_null);

    /* lc_heap_reallocate(ptr, 0) — acts as free */
    TEST_RUN(test_heap_reallocate_to_zero);

    /* Various bucket sizes */
    TEST_RUN(test_heap_bucket_sizes);

    /* Large allocation (mmap path) */
    TEST_RUN(test_heap_large_allocation);

    /* large-cache tail-leak regression (H2) */
    TEST_RUN(test_heap_large_cache_no_tail_leak);

    /* integer-overflow hardening (H3) */
    TEST_RUN(test_heap_allocate_size_overflow);

    /* Alloc/free cycles */
    TEST_RUN(test_heap_alloc_free_cycles);

    /* Heap stress */
    TEST_RUN(test_heap_stress);

    /* M8: every allocation is 16-byte aligned */
    TEST_RUN(test_heap_block_alignment);

    /* M7: emptied slab pages are reclaimed */
    TEST_RUN(test_heap_page_reclaim);
    TEST_RUN(test_heap_reallocate_large_inplace);

    /* Statistics */
    TEST_RUN(test_heap_stats);

    /* Thread exit: abandoned pages, adoption and reclaim */
    TEST_RUN(test_heap_exit_adopts_abandoned_page);
    TEST_RUN(test_heap_exit_adopts_only_a_page_with_room);
    TEST_RUN(test_heap_exit_reclaims_pages);
    TEST_RUN(test_heap_waiting_free_is_applied_without_an_exit);

    return test_main();
}

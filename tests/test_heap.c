/*
 * test_heap.c — tests for lightc heap allocator.
 */

#include "test.h"
#include <lightc/heap.h>
#include <lightc/string.h>

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

int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

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

    /* Statistics */
    TEST_RUN(test_heap_stats);

    return test_main();
}

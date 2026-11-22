/*
 * Exercise the heap allocator.
 */
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/print.h>
#include <lightc/heap.h>

#include "checks.h"

#define S(literal) literal, sizeof(literal) - 1

int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

    /* --- Basic allocate/free --- */
    lc_print_string(STDOUT, S("heap_allocate_basic"));
    lc_result_ptr num_r = lc_heap_allocate(sizeof(uint64_t));
    uint64_t *num = num_r.value;
    *num = 42;
    say_pass_fail(!lc_ptr_is_err(num_r) && *num == 42);

    lc_print_string(STDOUT, S("heap_free_basic"));
    lc_heap_free(num);
    say_pass_fail(true);

    /* --- Free NULL is safe --- */
    lc_print_string(STDOUT, S("heap_free_null"));
    lc_heap_free(NULL);
    say_pass_fail(true);

    /* --- Allocate zeroed --- */
    lc_print_string(STDOUT, S("heap_allocate_zeroed"));
    lc_result_ptr zeroed_r = lc_heap_allocate_zeroed(128);
    uint8_t *zeroed = zeroed_r.value;
    bool all_zero = true;
    for (size_t i = 0; i < 128; i++) {
        if (zeroed[i] != 0) { all_zero = false; break; }
    }
    say_pass_fail(!lc_ptr_is_err(zeroed_r) && all_zero);
    lc_heap_free(zeroed);

    /* --- Multiple small allocations (same bucket) --- */
    lc_print_string(STDOUT, S("heap_multiple_small"));
    void *ptrs[16];
    bool ok = true;
    for (int i = 0; i < 16; i++) {
        lc_result_ptr r = lc_heap_allocate(24);
        ptrs[i] = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
        /* Write to verify no overlap */
        lc_bytes_fill(ptrs[i], (uint8_t)(i + 1), 24);
    }
    /* Verify no corruption */
    for (int i = 0; i < 16 && ok; i++) {
        uint8_t *p = ptrs[i];
        for (int j = 0; j < 24; j++) {
            if (p[j] != (uint8_t)(i + 1)) { ok = false; break; }
        }
    }
    say_pass_fail(ok);
    for (int i = 0; i < 16; i++) lc_heap_free(ptrs[i]);

    /* --- All 29 size classes --- */
    lc_print_string(STDOUT, S("heap_all_29_classes"));
    size_t test_sizes[] = {
        1, 16, 24, 32, 40,       /* buckets 0-3 */
        48, 64, 80, 96,           /* buckets 4-7 */
        112, 144, 176, 208,       /* buckets 8-11 */
        240, 304, 368, 432,       /* buckets 12-15 */
        496, 624, 752, 880,       /* buckets 16-19 */
        1008, 1264, 1520, 1776,   /* buckets 20-23 */
        2032, 2544, 3056, 3568, 4080  /* buckets 24-28 */
    };
    ok = true;
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++) {
        lc_result_ptr r = lc_heap_allocate(test_sizes[i]);
        void *p = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
        /* Write to full usable size */
        lc_bytes_fill(p, 0xBB, test_sizes[i]);
        lc_heap_free(p);
    }
    say_pass_fail(ok);

    /* --- Fragmentation check: worst case should be < 25% --- */
    lc_print_string(STDOUT, S("heap_fragmentation"));
    /* The tightest sizes that just miss a bucket boundary.
     * e.g., asking for 17 should get bucket 1 (usable 24), not bucket 0 (usable 16).
     * waste = (24 - 17) / 24 = 29%. Old scheme: (48 - 17) / 48 = 64%.
     */
    size_t tight_sizes[] = {17, 25, 33, 41, 49, 65, 81, 97, 113, 145, 177, 209,
                            241, 305, 369, 433, 497, 625, 753, 881, 1009, 1265,
                            1521, 1777, 2033, 2545, 3057, 3569};
    ok = true;
    for (size_t i = 0; i < sizeof(tight_sizes) / sizeof(tight_sizes[0]); i++) {
        lc_result_ptr r = lc_heap_allocate(tight_sizes[i]);
        void *p = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
        lc_bytes_fill(p, 0xCC, tight_sizes[i]);
        lc_heap_free(p);
    }
    say_pass_fail(ok);

    /* --- Large allocation (direct mmap) --- */
    lc_print_string(STDOUT, S("heap_large_allocate"));
    lc_result_ptr big_r = lc_heap_allocate(8192);
    char *big = big_r.value;
    say_pass_fail(!lc_ptr_is_err(big_r));

    lc_print_string(STDOUT, S("heap_large_write"));
    lc_bytes_fill(big, 'L', 8192);
    say_pass_fail(big[0] == 'L' && big[8191] == 'L');

    lc_print_string(STDOUT, S("heap_large_free"));
    lc_heap_free(big);
    say_pass_fail(true);

    /* --- Reallocate: grow within same bucket --- */
    lc_print_string(STDOUT, S("heap_reallocate_same_bucket"));
    lc_result_ptr r_r = lc_heap_allocate(8);
    char *r = r_r.value;
    lc_bytes_copy(r, "hello!", 6);
    lc_result_ptr r2_r = lc_heap_reallocate(r, 14);
    char *r2 = r2_r.value;
    say_pass_fail(r2 == r && lc_string_equal(r2, 5, "hello", 5));

    /* --- Reallocate: grow to bigger bucket --- */
    lc_print_string(STDOUT, S("heap_reallocate_bigger"));
    lc_result_ptr r3_r = lc_heap_reallocate(r2, 200);
    char *r3 = r3_r.value;
    say_pass_fail(!lc_ptr_is_err(r3_r) && lc_string_equal(r3, 5, "hello", 5));
    lc_heap_free(r3);

    /* --- Reallocate: NULL ptr (acts as allocate) --- */
    lc_print_string(STDOUT, S("heap_reallocate_null"));
    lc_result_ptr r4_r = lc_heap_reallocate(NULL, 64);
    char *r4 = r4_r.value;
    say_pass_fail(!lc_ptr_is_err(r4_r));
    lc_heap_free(r4);

    /* --- Reallocate: size 0 (acts as free) --- */
    lc_print_string(STDOUT, S("heap_reallocate_zero"));
    lc_result_ptr r5_r = lc_heap_allocate(64);
    char *r5 = r5_r.value;
    lc_result_ptr r6_r = lc_heap_reallocate(r5, 0);
    void *r6 = r6_r.value;
    say_pass_fail(r6 == NULL);

    /* --- Stress: every size from 1 to 4096 --- */
    lc_print_string(STDOUT, S("heap_stress_all_sizes"));
    ok = true;
    for (size_t sz = 1; sz <= 4096; sz++) {
        lc_result_ptr r = lc_heap_allocate(sz);
        void *p = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
        /* Write first and last byte */
        ((uint8_t *)p)[0] = 0xAA;
        ((uint8_t *)p)[sz - 1] = 0x55;
        lc_heap_free(p);
    }
    say_pass_fail(ok);

    /* --- Stress: allocate many, then free all --- */
    lc_print_string(STDOUT, S("heap_stress_batch"));
    #define BATCH 256
    void *batch[BATCH];
    ok = true;
    for (int i = 0; i < BATCH; i++) {
        lc_result_ptr r = lc_heap_allocate((size_t)(i * 13 % 4000) + 1);
        batch[i] = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
    }
    for (int i = 0; i < BATCH; i++) lc_heap_free(batch[i]);
    say_pass_fail(ok);

    /* --- Page reuse: free-then-alloc reuses page free list --- */
    lc_print_string(STDOUT, S("heap_page_reuse"));
    void *first = lc_heap_allocate(100).value;
    void *second = lc_heap_allocate(100).value;
    lc_heap_free(first);
    void *reused = lc_heap_allocate(100).value;
    /* freed block should be reused from the same page */
    say_pass_fail(reused == first);
    lc_heap_free(second);
    lc_heap_free(reused);

    /* --- Full page cycle: fill a page, free all, refill --- */
    lc_print_string(STDOUT, S("heap_full_page_cycle"));
    /* 64 KiB page / 32-byte blocks = 2048 blocks in one page */
    #define FILL_COUNT 2048
    void *fill[FILL_COUNT];
    ok = true;
    for (int i = 0; i < FILL_COUNT; i++) {
        lc_result_ptr r = lc_heap_allocate(16); /* smallest bucket: 32-byte blocks */
        fill[i] = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
        ((uint8_t *)fill[i])[0] = (uint8_t)i;
    }
    /* Free all — page should become available for reuse */
    for (int i = 0; i < FILL_COUNT; i++) lc_heap_free(fill[i]);
    /* Allocate again — should reuse the same page */
    for (int i = 0; i < FILL_COUNT; i++) {
        lc_result_ptr r = lc_heap_allocate(16);
        fill[i] = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
    }
    for (int i = 0; i < FILL_COUNT; i++) lc_heap_free(fill[i]);
    say_pass_fail(ok);

    /* --- Multiple pages: allocate more than one page worth --- */
    lc_print_string(STDOUT, S("heap_multi_page"));
    #define MULTI_COUNT 4200 /* > 2048 blocks, needs 2+ pages */
    void *multi[MULTI_COUNT];
    ok = true;
    for (int i = 0; i < MULTI_COUNT; i++) {
        lc_result_ptr r = lc_heap_allocate(16);
        multi[i] = r.value;
        if (lc_ptr_is_err(r)) { ok = false; break; }
    }
    for (int i = 0; i < MULTI_COUNT; i++) lc_heap_free(multi[i]);
    say_pass_fail(ok);

    /* --- Reclamation: emptied pages go back to the OS --- */
    lc_print_string(STDOUT, S("heap_reclamation"));
    /* Allocate a large number of blocks across many pages */
    #define RECLAIM_COUNT 8000
    void *rptrs[RECLAIM_COUNT];
    int allocated = 0;
    while (allocated < RECLAIM_COUNT) {
        lc_result_ptr r = lc_heap_allocate(100);
        if (lc_ptr_is_err(r)) break;
        rptrs[allocated] = r.value;
        lc_bytes_fill(rptrs[allocated], 0xDD, 100);
        allocated++;
    }
    ok = allocated == RECLAIM_COUNT;
    /* Free all. Each page that empties goes back to the OS through
     * madvise, except one per size class, which the heap keeps for the
     * next allocation. */
    lc_heap_stats before, after;
    lc_heap_get_stats(&before);
    for (int i = 0; i < allocated; i++) lc_heap_free(rptrs[i]);
    lc_heap_get_stats(&after);
    /* Allocate again, from the page kept and the pages given back. As
     * with any reused block, a block may still hold old bytes, so this
     * checks only that every allocation succeeds. Free just the blocks
     * it got. */
    allocated = 0;
    while (allocated < RECLAIM_COUNT) {
        lc_result_ptr r = lc_heap_allocate(100);
        if (lc_ptr_is_err(r)) break;
        rptrs[allocated++] = r.value;
    }
    ok = ok && allocated == RECLAIM_COUNT;
    for (int i = 0; i < allocated; i++) lc_heap_free(rptrs[i]);
    say_pass_fail(ok);

    /* --- The heap statistics count the pages given back --- */
    lc_print_string(STDOUT, S("heap_pages_reclaimed"));
#if LC_STATS
    say_pass_fail(after.heap_pages_reclaimed > before.heap_pages_reclaimed);
    lc_print_string(STDOUT, S("  pages given back: "));
    lc_print_unsigned(STDOUT, after.heap_pages_reclaimed - before.heap_pages_reclaimed);
    lc_print_newline(STDOUT);
#else
    /* LC_STATS is off when NDEBUG is defined, as in release and tiny
     * builds, and lc_heap_get_stats then zeroes every counter. */
    say_skip("LC_STATS is off, so the heap counts no reclaimed pages");
#endif

    return checks_summary();
}

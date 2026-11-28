/*
 * ARENA-aligned-math — lc_arena_allocate_aligned must align the address it
 * returns, and refuse an alignment it cannot honour.
 *
 * It aligned the offset into the arena, not the address. The arena's base
 * is aligned only to the page, so an alignment above the page size was not
 * honoured: an arena's first 65536-aligned allocation was 4096-aligned at
 * best. An alignment of 0 rounded the offset down to the last allocation's,
 * and returned memory already handed out; one that is not a power of two,
 * such as 24, gave a misaligned pointer. It now aligns the address, and
 * returns NULL for 0 and for a non-power of two.
 */

#include "test.h"
#include <lightc/memory.h>
#include <lightc/string.h>

/* 64 fresh arenas: each first 65536-aligned allocation is 65536-aligned. */
static void test_alignment_above_the_page(void) {
    int misaligned = 0;
    for (int i = 0; i < 64; i++) {
        lc_arena a = lc_arena_create(1 << 20);
        TEST_REQUIRE_NOT_NULL(a.base);
        void *p = lc_arena_allocate_aligned(&a, 100, 65536);
        if (p == NULL || ((uintptr_t)p & 65535) != 0) misaligned++;
        lc_arena_destroy(&a);
    }
    TEST_ASSERT_EQ(misaligned, 0);
}

/* Alignment 0 returns NULL and hands nothing out twice. */
static void test_alignment_zero(void) {
    lc_arena a = lc_arena_create(4096);
    TEST_REQUIRE_NOT_NULL(a.base);
    char *x = lc_arena_allocate(&a, 64);
    TEST_REQUIRE_NOT_NULL(x);
    size_t used = lc_arena_get_used(&a);
    TEST_ASSERT_NULL(lc_arena_allocate_aligned(&a, 64, 0));
    TEST_ASSERT_EQ(lc_arena_get_used(&a), used);
    lc_arena_destroy(&a);
}

/* An alignment that is not a power of two returns NULL. */
static void test_alignment_not_a_power_of_two(void) {
    lc_arena a = lc_arena_create(4096);
    TEST_REQUIRE_NOT_NULL(a.base);
    TEST_REQUIRE_NOT_NULL(lc_arena_allocate(&a, 1));
    TEST_ASSERT_NULL(lc_arena_allocate_aligned(&a, 16, 24));
    TEST_ASSERT_NULL(lc_arena_allocate_aligned(&a, 16, 3));
    lc_arena_destroy(&a);
}

/* Small powers of two still work, one after another. */
static void test_small_alignments(void) {
    lc_arena a = lc_arena_create(4096);
    TEST_REQUIRE_NOT_NULL(a.base);
    for (size_t align = 1; align <= 256; align *= 2) {
        TEST_REQUIRE_NOT_NULL(lc_arena_allocate(&a, 3));
        void *p = lc_arena_allocate_aligned(&a, 5, align);
        TEST_ASSERT_NOT_NULL(p);
        TEST_ASSERT_EQ((uintptr_t)p % align, (uintptr_t)0);
    }
    lc_arena_destroy(&a);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_alignment_above_the_page);
    TEST_RUN(test_alignment_zero);
    TEST_RUN(test_alignment_not_a_power_of_two);
    TEST_RUN(test_small_alignments);
    return test_main();
}

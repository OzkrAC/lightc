/*
 * ARENA-create-wrap — lc_arena_create must fail for a size it cannot round
 * up to a page.
 *
 * It rounded the size up to a whole page with (size + page - 1) & ~(page -
 * 1), which wraps for a size within a page of SIZE_MAX: SIZE_MAX - 100 came
 * out as 0 pages, then 1, and the call returned a one-page arena where it
 * should have failed. It now returns an arena with no base.
 */

#include "test.h"
#include <lightc/memory.h>

static void test_sizes_near_size_max_fail(void) {
    size_t sizes[] = { SIZE_MAX, SIZE_MAX - 100, SIZE_MAX - (LC_PAGE_SIZE - 2) };
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        lc_arena a = lc_arena_create(sizes[i]);
        TEST_ASSERT_NULL(a.base);
        TEST_ASSERT_EQ(a.capacity, (size_t)0);
        if (a.base) lc_arena_destroy(&a);
    }
}

static void test_ordinary_size_works(void) {
    lc_arena a = lc_arena_create(10000);
    TEST_REQUIRE_NOT_NULL(a.base);
    TEST_ASSERT(a.capacity >= 10000);
    TEST_ASSERT_EQ(a.capacity % LC_PAGE_SIZE, (size_t)0);
    lc_arena_destroy(&a);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_sizes_near_size_max_fail);
    TEST_RUN(test_ordinary_size_works);
    return test_main();
}

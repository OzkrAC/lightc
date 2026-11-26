/*
 * DS-array-self-alias — lc_array_push and lc_array_insert must copy the
 * right value when the element points into the array itself.
 *
 * push(&a, lc_array_get(&a, 0)) on a full array grew it first: the reserve
 * moved the elements to a new buffer and freed the old one, which the copy
 * then read, freed memory or an unmapped page. insert(&a, 0, lc_array_get(&a,
 * 1)) shifted the tail right before copying, so it copied the element that
 * had moved into place, the wrong one. Both now find the element again after
 * the reserve or the shift.
 */

#include "test.h"
#include <lightdata/array.h>

/* push of an element of the array, when the push grows it */
static void test_push_across_growth(void) {
    lc_array a = lc_array_create(sizeof(int64_t));
    for (int64_t i = 0; i < 8; i++) {
        int64_t v = 1000 + i;
        TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, &v)));
    }
    TEST_REQUIRE_EQ(a.count, a.capacity);
    TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, lc_array_get(&a, 0))));
    TEST_ASSERT_EQ(*(int64_t *)lc_array_get(&a, 8), (int64_t)1000);
    lc_array_destroy(&a);
}

/* The same with a buffer large enough to be a mapping of its own, which
 * the growth unmaps. */
static void test_push_across_growth_of_a_large_buffer(void) {
    typedef struct { int64_t a, b; } pair;
    lc_array a = lc_array_create(sizeof(pair));
    TEST_REQUIRE(!lc_is_err(lc_array_reserve(&a, 512)));
    for (int64_t i = 0; i < 512; i++) {
        pair e = { i, -i };
        TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, &e)));
    }
    TEST_REQUIRE_EQ(a.count, a.capacity);
    TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, lc_array_get(&a, 5))));
    const pair *last = lc_array_get(&a, 512);
    TEST_ASSERT_EQ(last->a, (int64_t)5);
    TEST_ASSERT_EQ(last->b, (int64_t)-5);
    lc_array_destroy(&a);
}

/* insert of an element of the array, with room to spare: no growth */
static void test_insert_of_a_shifted_element(void) {
    lc_array a = lc_array_create(sizeof(int64_t));
    TEST_REQUIRE(!lc_is_err(lc_array_reserve(&a, 16)));
    int64_t x = 10, y = 20;
    TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, &x)));
    TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, &y)));
    TEST_REQUIRE(!lc_ptr_is_err(lc_array_insert(&a, 0, lc_array_get(&a, 1))));
    TEST_ASSERT_EQ(*(int64_t *)lc_array_get(&a, 0), (int64_t)20);
    TEST_ASSERT_EQ(*(int64_t *)lc_array_get(&a, 1), (int64_t)10);
    TEST_ASSERT_EQ(*(int64_t *)lc_array_get(&a, 2), (int64_t)20);
    lc_array_destroy(&a);
}

/* insert of an element of the array, when the insert grows it */
static void test_insert_across_growth(void) {
    lc_array a = lc_array_create(sizeof(int64_t));
    for (int64_t i = 0; i < 8; i++) {
        int64_t v = 100 + i;
        TEST_REQUIRE(!lc_ptr_is_err(lc_array_push(&a, &v)));
    }
    TEST_REQUIRE_EQ(a.count, a.capacity);
    TEST_REQUIRE(!lc_ptr_is_err(lc_array_insert(&a, 3, lc_array_get(&a, 6))));
    TEST_ASSERT_EQ(*(int64_t *)lc_array_get(&a, 3), (int64_t)106);
    TEST_ASSERT_EQ(*(int64_t *)lc_array_get(&a, 7), (int64_t)106);
    TEST_ASSERT_EQ(a.count, (size_t)9);
    lc_array_destroy(&a);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_push_across_growth);
    TEST_RUN(test_push_across_growth_of_a_large_buffer);
    TEST_RUN(test_insert_of_a_shifted_element);
    TEST_RUN(test_insert_across_growth);
    return test_main();
}

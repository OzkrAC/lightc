/*
 * DS-ringbuf-overflow — lc_ringbuf_create must fail when the buffer it
 * would need cannot be sized.
 *
 * It rounded the capacity up to a power of two and allocated capacity *
 * element_size bytes, with neither step checked. 2^60 elements of 16 bytes
 * is 2^64 bytes, which wraps to 0: lc_heap_allocate(0) gave a 16-byte
 * block behind a ring that claimed a capacity of 2^60, and each push wrote
 * past it into the next allocation. A capacity above 2^63 made the round-up
 * wrap to 0 too. Both now give a ring with no buffer, whose pushes fail.
 */

#include "test.h"
#include <lightc/heap.h>
#include <lightc/string.h>
#include <lightdata/ringbuf.h>

/* A byte size that wraps: the ring fails and the neighbouring block keeps
 * its bytes. */
static void test_byte_size_that_wraps_fails(void) {
    lc_result_ptr a = lc_heap_allocate(8);
    lc_ringbuf ring = lc_ringbuf_create(16, (size_t)1 << 60);
    lc_result_ptr b = lc_heap_allocate(8);
    TEST_REQUIRE(!lc_ptr_is_err(a) && !lc_ptr_is_err(b));
    lc_bytes_fill(b.value, 0x11, 8);

    /* Checked first: with the defect a push writes past the block. */
    TEST_REQUIRE_NULL(ring.data);
    TEST_ASSERT_EQ(ring.capacity, (size_t)0);
    uint8_t element[16];
    lc_bytes_fill(element, 0xab, sizeof element);
    TEST_ASSERT_EQ(lc_ringbuf_push(&ring, element).error, LC_ERR_FULL);

    for (int i = 0; i < 8; i++) TEST_ASSERT_EQ(((uint8_t *)b.value)[i], (uint8_t)0x11);
    lc_ringbuf_destroy(&ring);
    lc_heap_free(b.value);
    lc_heap_free(a.value);
}

/* A capacity whose round-up to a power of two wraps fails too. */
static void test_capacity_that_wraps_fails(void) {
    lc_ringbuf ring = lc_ringbuf_create(1, ((size_t)1 << 63) + 1);
    TEST_ASSERT_NULL(ring.data);
    TEST_ASSERT_EQ(ring.capacity, (size_t)0);
    lc_ringbuf_destroy(&ring);
}

/* A ring that fits still works. */
static void test_small_ring_works(void) {
    lc_ringbuf ring = lc_ringbuf_create(sizeof(int64_t), 5);
    TEST_REQUIRE_NOT_NULL(ring.data);
    TEST_ASSERT_EQ(ring.capacity, (size_t)8);
    int64_t v = 42, out = 0;
    TEST_ASSERT_EQ(lc_ringbuf_push(&ring, &v).error, 0);
    TEST_ASSERT(lc_ringbuf_pop(&ring, &out));
    TEST_ASSERT_EQ(out, (int64_t)42);
    lc_ringbuf_destroy(&ring);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_byte_size_that_wraps_fails);
    TEST_RUN(test_capacity_that_wraps_fails);
    TEST_RUN(test_small_ring_works);
    return test_main();
}

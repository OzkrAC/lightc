/*
 * async-2 — lc_async_peek must see the completions the kernel held back
 * while the CQ was full, and every one after them.
 *
 * With the CQ full, the kernel keeps further completions in an overflow
 * list and sets IORING_SQ_CQ_OVERFLOW, and only an io_uring_enter with
 * GETEVENTS moves them into the CQ. lc_async_peek made no enter, so a loop
 * that only peeked never saw them, nor any later completion, which queues
 * behind them. lc_async_peek now makes that enter while the flag is set.
 *
 * A ring of 1 has an SQ of 1 and a CQ of 2: of four NOPs, 3 and 4 overflow.
 * The program needs io_uring, which qemu-user lacks, and skips there.
 */

#include "test.h"
#include <lightc/async.h>

#define IORING_OP_NOP 0

static bool io_uring_works;

/* Submits a NOP tagged `tag`, and flushes it. */
static bool nop(lc_async_ring *ring, uint64_t tag) {
    if (lc_is_err(lc_async_submit_raw(ring, IORING_OP_NOP, -1, 0, 0, 0, 0, tag))) return false;
    return lc_async_flush(ring) == 1;
}

/* Peeks up to 100 times; returns the mask of the tags 1 to 31 seen. */
static uint32_t peeked(lc_async_ring *ring) {
    uint32_t seen = 0;
    for (int i = 0; i < 100; i++) {
        lc_async_result r[4];
        uint32_t n = lc_async_peek(ring, r, 4);
        for (uint32_t j = 0; j < n; j++)
            if (r[j].tag >= 1 && r[j].tag <= 31 && r[j].result == 0) seen |= 1u << (r[j].tag - 1);
    }
    return seen;
}

static void test_peek_sees_overflowed_completions(void) {
    if (!io_uring_works) TEST_SKIP("io_uring not available");
    lc_result_ptr rp = lc_async_ring_create(1);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;

    for (uint64_t tag = 1; tag <= 4; tag++) TEST_REQUIRE(nop(ring, tag));
    TEST_ASSERT_EQ(peeked(ring), (uint32_t)0xf);

    /* One more, after the overflow: it is seen too. */
    TEST_REQUIRE(nop(ring, 5));
    TEST_ASSERT_EQ(peeked(ring), (uint32_t)0x10);
    lc_async_ring_destroy(ring);
}

/* The same with a bigger ring, as the review found it: 12 NOPs, a CQ of 8. */
static void test_peek_sees_overflow_of_a_bigger_ring(void) {
    if (!io_uring_works) TEST_SKIP("io_uring not available");
    lc_result_ptr rp = lc_async_ring_create(4);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;

    for (uint64_t tag = 1; tag <= 12; tag++) TEST_REQUIRE(nop(ring, tag));
    TEST_ASSERT_EQ(peeked(ring), (uint32_t)0xfff);
    TEST_REQUIRE(nop(ring, 13));
    TEST_ASSERT_EQ(peeked(ring), (uint32_t)0x1000);
    lc_async_ring_destroy(ring);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    lc_result_ptr probe = lc_async_ring_create(4);
    io_uring_works = lc_ptr_is_ok(probe);
    if (io_uring_works) lc_async_ring_destroy(probe.value);
    TEST_RUN(test_peek_sees_overflowed_completions);
    TEST_RUN(test_peek_sees_overflow_of_a_bigger_ring);
    return test_main();
}

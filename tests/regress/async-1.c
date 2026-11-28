/*
 * async-1 — an SQE that a failed io_uring_enter did not take must reach the
 * kernel with a later flush or wait.
 *
 * lc_async_flush published the staged SQEs, cleared its count of them and
 * asked the kernel for that many. An enter that failed with EAGAIN or EBUSY
 * took none, and nothing offered them again: the next flush asked only for
 * what it had staged itself, which the kernel took from the oldest, and a
 * wait asked for none. Operations lagged forever, and with the SQ full the
 * ring wedged. Each flush and wait now offers every SQE the kernel has not
 * taken.
 *
 * tests/uring_fault.h fails the enter with EAGAIN, as the kernel does when
 * it cannot allocate a request. Each case runs in a child under a 3 s
 * watchdog, as the wait of the wedged ring never returned. The program
 * needs io_uring and seccomp, which qemu-user lacks, and skips there.
 */

#include "test.h"
#include "uring_fault.h"
#include <lightc/async.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

/* Peeks for up to 2 s, until each of the tags 1 to `count` has completed
 * with a result of 1; returns the mask of those that did. */
static uint32_t completed_reads(lc_async_ring *ring, uint32_t count) {
    uint32_t all = (1u << count) - 1, seen = 0;
    for (int i = 0; i < 2000 && seen != all; i++) {
        lc_async_result r[8];
        uint32_t n = lc_async_peek(ring, r, 8);
        for (uint32_t j = 0; j < n; j++)
            if (r[j].tag >= 1 && r[j].tag <= count && r[j].result == 1)
                seen |= 1u << (r[j].tag - 1);
        if (seen != all) lc_time_sleep_milliseconds(1);
    }
    return seen;
}

/* A pipe that holds 8 bytes: each 1-byte read of it completes at once. */
static int32_t pipe_with_data(void) {
    int32_t p[2];
    if (lc_kernel_create_pipe(p, 0) < 0) return -1;
    if (lc_kernel_write_bytes(p[1], "abcdefgh", 8) != 8) return -1;
    return p[0];
}

/* Three reads whose flush fails, then a fourth with a flush that works:
 * that flush must submit all four, not the oldest one. */
static int32_t lag(void) {
    int32_t fd = pipe_with_data();
    if (fd < 0) return 10;
    lc_result_ptr rp = lc_async_ring_create(8);
    if (lc_ptr_is_err(rp)) return 11;
    lc_async_ring *ring = rp.value;
    if (!refuse_flushes_of(3)) return 12;

    char buf[4];
    for (uint64_t i = 0; i < 3; i++)
        if (lc_is_err(lc_async_submit_read(ring, fd, &buf[i], 1, (uint64_t)-1, 1 + i))) return 13;
    if (lc_async_flush(ring) != 0) return 14;   /* the filter refused the three */
    if (lc_is_err(lc_async_submit_read(ring, fd, &buf[3], 1, (uint64_t)-1, 4))) return 15;
    (void)lc_async_flush(ring);

    uint32_t seen = completed_reads(ring, 4);
    return seen == 0xf ? 0 : 20 + (int32_t)seen;   /* the defect: 21, tag 1 alone */
}

/* Four reads fill the SQ, and their flush fails: the wait must submit them,
 * or the ring is wedged and the wait never returns. */
static int32_t wedge(void) {
    int32_t fd = pipe_with_data();
    if (fd < 0) return 10;
    lc_result_ptr rp = lc_async_ring_create(4);
    if (lc_ptr_is_err(rp)) return 11;
    lc_async_ring *ring = rp.value;
    if (!refuse_flushes_of(4)) return 12;

    char buf[4];
    for (uint64_t i = 0; i < 4; i++)
        if (lc_is_err(lc_async_submit_read(ring, fd, &buf[i], 1, (uint64_t)-1, 1 + i))) return 13;
    if (lc_async_flush(ring) != 0) return 14;
    if (lc_async_get_free_slots(ring) != 0) return 15;   /* the four are still queued */

    lc_async_result r[8];
    uint32_t n = lc_async_wait(ring, r, 8);   /* the defect: never returns */
    uint32_t seen = 0;
    for (uint32_t j = 0; j < n; j++)
        if (r[j].tag >= 1 && r[j].tag <= 4 && r[j].result == 1) seen |= 1u << (r[j].tag - 1);
    seen |= completed_reads(ring, 4);
    if (seen != 0xf) return 20 + (int32_t)seen;
    return lc_async_get_free_slots(ring) == 4 ? 0 : 16;
}

static bool can_run;

static void test_refused_flush_lags_no_read(void) {
    if (!can_run) TEST_SKIP("needs io_uring and seccomp");
    TEST_ASSERT_EQ(in_child(lag, 3), 0);
}

static void test_refused_flush_of_a_full_sq_wedges_nothing(void) {
    if (!can_run) TEST_SKIP("needs io_uring and seccomp");
    TEST_ASSERT_EQ(in_child(wedge, 3), 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    can_run = uring_fault_works();
    TEST_RUN(test_refused_flush_lags_no_read);
    TEST_RUN(test_refused_flush_of_a_full_sq_wedges_nothing);
    return test_main();
}

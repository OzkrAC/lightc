/*
 * test_async.c — tests for lightc async I/O (io_uring).
 */

#include "test.h"
#include "uring_fault.h"
#include <lightc/async.h>
#include <lightc/signal.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/time.h>

/* Every test needs io_uring as lightc uses it, from Linux 5.10 on (see
 * docs/contracts.html). main probes for it once, and without it each test
 * skips. */
static bool io_uring_available;
#define NO_IO_URING "io_uring not available (Linux 5.10+ required)"

/* ===== lc_async_ring_create / destroy ===== */

static void test_async_ring_create_destroy(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    lc_result_ptr r = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(r);
    TEST_ASSERT_NOT_NULL(r.value);

    lc_async_ring *ring = (lc_async_ring *)r.value;
    lc_async_ring_destroy(ring);
}

/* ===== Submit read/write via pipe ===== */

static void test_async_submit_read_write(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    /* Create a pipe: fds[0] = read end, fds[1] = write end */
    int32_t fds[2];
    lc_sysret pipe_ret = lc_kernel_create_pipe(fds, 0);
    TEST_ASSERT(pipe_ret >= 0);

    /* Create ring */
    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(rp);
    lc_async_ring *ring = (lc_async_ring *)rp.value;

    /* Write data to the pipe synchronously */
    const char *msg = "hello async";
    size_t msg_len = lc_string_length(msg);
    lc_sysret wr = lc_kernel_write_bytes(fds[1], msg, msg_len);
    TEST_ASSERT_EQ(wr, (lc_sysret)msg_len);

    /* Submit async read from read end */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_result sr = lc_async_submit_read(ring, fds[0], buf, (uint32_t)msg_len, (uint64_t)-1, 42);
    TEST_ASSERT_OK(sr);

    /* Flush and wait for completion */
    lc_async_result results[4];
    uint32_t n = lc_async_wait(ring, results, 4);
    TEST_ASSERT(n >= 1);

    /* Verify data matches */
    TEST_ASSERT_EQ(results[0].result, (int32_t)msg_len);
    TEST_ASSERT_STR_EQ(buf, msg_len, msg, msg_len);

    /* Cleanup */
    lc_async_ring_destroy(ring);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

/* ===== Tag matching across multiple ops ===== */

static void test_async_tag_matching(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    int32_t fds1[2], fds2[2];
    lc_sysret r1 = lc_kernel_create_pipe(fds1, 0);
    lc_sysret r2 = lc_kernel_create_pipe(fds2, 0);
    TEST_ASSERT(r1 >= 0);
    TEST_ASSERT(r2 >= 0);

    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(rp);
    lc_async_ring *ring = (lc_async_ring *)rp.value;

    /* Write to both pipes */
    lc_kernel_write_bytes(fds1[1], "aaa", 3);
    lc_kernel_write_bytes(fds2[1], "bbb", 3);

    /* Submit reads with different tags */
    char buf1[8], buf2[8];
    lc_bytes_fill(buf1, 0, sizeof(buf1));
    lc_bytes_fill(buf2, 0, sizeof(buf2));

    lc_result s1 = lc_async_submit_read(ring, fds1[0], buf1, 3, (uint64_t)-1, 100);
    lc_result s2 = lc_async_submit_read(ring, fds2[0], buf2, 3, (uint64_t)-1, 200);
    TEST_ASSERT_OK(s1);
    TEST_ASSERT_OK(s2);

    /* Wait for both completions */
    lc_async_result results[4];
    uint32_t total = 0;
    while (total < 2) {
        uint32_t n = lc_async_wait(ring, results + total, 4 - total);
        total += n;
    }

    /* Verify both tags appear (order may vary) */
    bool found_100 = false;
    bool found_200 = false;
    for (uint32_t i = 0; i < total; i++) {
        if (results[i].tag == 100) found_100 = true;
        if (results[i].tag == 200) found_200 = true;
    }
    TEST_ASSERT(found_100);
    TEST_ASSERT(found_200);

    lc_async_ring_destroy(ring);
    lc_kernel_close_file(fds1[0]);
    lc_kernel_close_file(fds1[1]);
    lc_kernel_close_file(fds2[0]);
    lc_kernel_close_file(fds2[1]);
}

/* ===== Peek with no pending ops returns 0 ===== */

static void test_async_peek_empty(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(rp);
    lc_async_ring *ring = (lc_async_ring *)rp.value;

    lc_async_result results[4];
    uint32_t n = lc_async_peek(ring, results, 4);
    TEST_ASSERT_EQ(n, (uint32_t)0);

    lc_async_ring_destroy(ring);
}

/* ===== Flush returns count of submitted ops ===== */

static void test_async_flush_count(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    int32_t fds[2];
    lc_sysret pr = lc_kernel_create_pipe(fds, 0);
    TEST_ASSERT(pr >= 0);

    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(rp);
    lc_async_ring *ring = (lc_async_ring *)rp.value;

    /* Write some data so reads will complete */
    lc_kernel_write_bytes(fds[1], "xxxxxx", 6);

    /* Submit 3 reads */
    char buf1[4], buf2[4], buf3[4];
    lc_result s1 = lc_async_submit_read(ring, fds[0], buf1, 2, (uint64_t)-1, 1);
    lc_result s2 = lc_async_submit_read(ring, fds[0], buf2, 2, (uint64_t)-1, 2);
    lc_result s3 = lc_async_submit_read(ring, fds[0], buf3, 2, (uint64_t)-1, 3);
    TEST_ASSERT_OK(s1);
    TEST_ASSERT_OK(s2);
    TEST_ASSERT_OK(s3);

    uint32_t flushed = lc_async_flush(ring);
    TEST_ASSERT_EQ(flushed, (uint32_t)3);

    /* Drain completions */
    lc_async_result results[4];
    uint32_t total = 0;
    while (total < 3) {
        uint32_t n = lc_async_wait(ring, results + total, 4 - total);
        total += n;
    }

    lc_async_ring_destroy(ring);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

/* ===== Free slots decrease after submissions ===== */

static void test_async_free_slots(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(rp);
    lc_async_ring *ring = (lc_async_ring *)rp.value;

    uint32_t initial_slots = lc_async_get_free_slots(ring);
    TEST_ASSERT(initial_slots > 0);

    /* Create a pipe for a valid fd target */
    int32_t fds[2];
    lc_sysret pr = lc_kernel_create_pipe(fds, 0);
    TEST_ASSERT(pr >= 0);

    /* Write data so read can succeed */
    lc_kernel_write_bytes(fds[1], "xx", 2);

    /* Submit one op */
    char buf[4];
    lc_result sr = lc_async_submit_read(ring, fds[0], buf, 2, (uint64_t)-1, 0);
    TEST_ASSERT_OK(sr);

    uint32_t after_slots = lc_async_get_free_slots(ring);
    TEST_ASSERT(after_slots < initial_slots);

    /* Flush and drain to leave ring clean */
    lc_async_result results[4];
    lc_async_wait(ring, results, 4);

    lc_async_ring_destroy(ring);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

/* ===== Multi-cycle flush accounting ===== */

/*
 * Stress the submission bookkeeping in lc_async_flush across many separate
 * flush cycles. Total ops (CYCLES*PER = 200) far exceed the ring size (8), so
 * sq_tail wraps repeatedly and sq_pending is published+reset on every flush.
 *
 * Each cycle submits PER reads, one flush per submit (so every flush must
 * report exactly 1 submitted), then fully drains PER completions and checks
 * each read returned 1 byte. A tail double-advance or a lost/duplicated entry
 * in flush would desync the ring: wrong submit counts, missing completions, or
 * a free-slot count that never returns to full. After each cycle the ring must
 * be fully reconciled (free slots back to the initial full count).
 */
static void test_async_multiflush_accounting(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);

    int32_t fds[2];
    lc_sysret pr = lc_kernel_create_pipe(fds, 0);
    TEST_ASSERT(pr >= 0);

    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_ASSERT_PTR_OK(rp);
    lc_async_ring *ring = (lc_async_ring *)rp.value;

    uint32_t initial_slots = lc_async_get_free_slots(ring);
    TEST_ASSERT(initial_slots > 0);

    const int CYCLES = 50;
    const int PER    = 4;
    char buf[8];

    for (int c = 0; c < CYCLES; c++) {
        for (int i = 0; i < PER; i++) {
            /* One byte in the pipe per read so the read completes immediately. */
            lc_sysret w = lc_kernel_write_bytes(fds[1], "x", 1);
            TEST_ASSERT_EQ(w, (lc_sysret)1);

            uint64_t tag = (uint64_t)(c * PER + i) + 1;  /* unique, non-zero */
            lc_result s = lc_async_submit_read(ring, fds[0], &buf[i], 1, (uint64_t)-1, tag);
            TEST_ASSERT_OK(s);

            uint32_t flushed = lc_async_flush(ring);
            TEST_ASSERT_EQ(flushed, (uint32_t)1);  /* exactly one submitted */
        }

        /* Drain exactly PER completions, bounded so a regression fails instead
         * of hanging CI. */
        int got = 0;
        int guard = 0;
        while (got < PER) {
            lc_async_result res[8];
            uint32_t n = lc_async_wait(ring, res, 8);
            for (uint32_t k = 0; k < n; k++) {
                TEST_ASSERT_EQ(res[k].result, (int32_t)1);  /* read one byte */
            }
            got += (int)n;
            TEST_ASSERT(++guard < 1000);  /* no infinite wait on regression */
        }

        /* Ring must be fully reconciled between cycles: everything submitted
         * (sq_head caught up to sq_tail) and nothing left pending. */
        TEST_ASSERT_EQ(lc_async_get_free_slots(ring), initial_slots);
    }

    lc_async_ring_destroy(ring);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

/* ===== lc_async_flush_ex / lc_async_wait_ex ===== */

#define IORING_OP_NOP 0

static bool queue_nop(lc_async_ring *ring, uint64_t tag) {
    return lc_is_ok(lc_async_submit_raw(ring, IORING_OP_NOP, -1, 0, 0, 0, 0, tag));
}

/* What has completed: the mask of tags 1 to 31, from up to 100 peeks that
 * flush what is still queued between them. */
static uint32_t drain(lc_async_ring *ring) {
    uint32_t seen = 0;
    for (int i = 0; i < 100; i++) {
        lc_async_result r[4];
        uint32_t n = lc_async_peek(ring, r, 4);
        for (uint32_t j = 0; j < n; j++)
            if (r[j].tag >= 1 && r[j].tag <= 31) seen |= 1u << (r[j].tag - 1);
        if (n == 0) (void)lc_async_flush_ex(ring);
    }
    return seen;
}

static void test_async_flush_ex_and_wait_ex(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    lc_result_ptr rp = lc_async_ring_create(8);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;

    lc_result none = lc_async_flush_ex(ring);
    TEST_ASSERT_EQ(none.value, (int64_t)0);
    TEST_ASSERT_EQ(none.error, 0);

    for (uint64_t tag = 1; tag <= 3; tag++) TEST_REQUIRE(queue_nop(ring, tag));
    lc_result f = lc_async_flush_ex(ring);
    TEST_ASSERT_EQ(f.value, (int64_t)3);
    TEST_ASSERT_EQ(f.error, 0);

    uint32_t seen = 0;
    for (int i = 0; i < 10 && seen != 7; i++) {
        lc_async_result r[8];
        lc_result w = lc_async_wait_ex(ring, r, 8);
        TEST_ASSERT_EQ(w.error, 0);
        for (int64_t j = 0; j < w.value; j++) seen |= 1u << (r[j].tag - 1);
    }
    TEST_ASSERT_EQ(seen, (uint32_t)7);
    lc_async_ring_destroy(ring);
}

/* A ring whose fd was closed by mistake: each enter fails with EBADF. The
 * _ex calls report it at once, and the wait does not block. */
static void test_async_ex_report_a_closed_ring(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    /* The ring's fd is the lowest free one when it is created. */
    lc_sysret probe = lc_kernel_open_file("/dev/null", O_RDONLY, 0);
    TEST_REQUIRE(probe >= 0);
    lc_kernel_close_file((int32_t)probe);
    lc_result_ptr rp = lc_async_ring_create(4);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;
    lc_kernel_close_file((int32_t)probe);

    TEST_REQUIRE(queue_nop(ring, 1));
    lc_result f = lc_async_flush_ex(ring);
    TEST_ASSERT_EQ(f.value, (int64_t)0);
    TEST_ASSERT_EQ(f.error, LC_ERR_BADF);

    lc_async_result r[4];
    int64_t start = lc_time_now_monotonic();
    lc_result w = lc_async_wait_ex(ring, r, 4);
    TEST_ASSERT_EQ(w.value, (int64_t)0);
    TEST_ASSERT_EQ(w.error, LC_ERR_BADF);

    /* lc_async_wait cannot report it: it returns 0, and keeps it. */
    TEST_ASSERT_EQ(lc_async_wait(ring, r, 4), (uint32_t)0);
    TEST_ASSERT(lc_time_now_monotonic() - start < 1000000000);
    lc_async_ring_destroy(ring);
}

/* An error that lc_async_flush could not report is reported by the next
 * _ex call, once, and the queued SQEs still reach the kernel. In a child,
 * which tests/uring_fault.h's filter refuses flushes of two SQEs in. */
static int32_t kept_error(void) {
    lc_result_ptr rp = lc_async_ring_create(4);
    if (lc_ptr_is_err(rp)) return 10;
    lc_async_ring *ring = rp.value;
    if (!refuse_flushes_of(2)) return 11;
    if (!queue_nop(ring, 1) || !queue_nop(ring, 2)) return 12;
    if (lc_async_flush(ring) != 0) return 13;

    lc_async_result r[4];
    lc_result w = lc_async_wait_ex(ring, r, 4);   /* reported, without waiting */
    if (w.error != LC_ERR_AGAIN || w.value != 0) return 14;
    uint32_t seen = 0;
    for (int i = 0; i < 10 && seen != 3; i++) {
        w = lc_async_wait_ex(ring, r, 4);         /* its wait gets past the filter */
        if (w.error != 0) return 15;
        for (int64_t j = 0; j < w.value; j++) seen |= 1u << (r[j].tag - 1);
    }
    if (seen != 3) return 16;

    if (!queue_nop(ring, 3) || !queue_nop(ring, 4)) return 17;
    lc_result f = lc_async_flush_ex(ring);
    if (f.error != LC_ERR_AGAIN || f.value != 0) return 18;
    f = lc_async_flush_ex(ring);                  /* refused again, and said so */
    if (f.error != LC_ERR_AGAIN) return 19;
    return 0;
}

static void test_async_ex_report_a_kept_error(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    if (!uring_fault_works()) TEST_SKIP("seccomp filters do not work here");
    TEST_ASSERT_EQ(in_child(kept_error, 3), 0);
}

/* A full CQ that nothing reaps: a flush returns at once, whether the kernel
 * queued the NOP behind the overflow or refused it with EBUSY, and every
 * completion arrives once reaped. */
static void test_async_flush_ex_on_a_full_cq_returns(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    lc_result_ptr rp = lc_async_ring_create(1);   /* SQ 1, CQ 2 */
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;

    for (uint64_t tag = 1; tag <= 2; tag++) {
        TEST_REQUIRE(queue_nop(ring, tag));
        TEST_REQUIRE_EQ(lc_async_flush_ex(ring).value, (int64_t)1);
    }
    for (uint64_t tag = 3; tag <= 4; tag++) {
        TEST_REQUIRE(queue_nop(ring, tag));
        int64_t start = lc_time_now_monotonic();
        lc_result f = lc_async_flush_ex(ring);
        TEST_ASSERT(lc_time_now_monotonic() - start < 10000000);   /* 10 ms */
        TEST_ASSERT((f.value == 1 && f.error == 0) || f.error == LC_ERR_BUSY);
    }
    TEST_ASSERT_EQ(drain(ring), (uint32_t)0xf);
    lc_async_ring_destroy(ring);
}

/* A signal ends lc_async_wait_ex's wait, with no error: a loop that waits
 * can then see what the handler did. */
static volatile int32_t alarms;
static void on_alarm(int signo) { (void)signo; alarms++; }

static void test_async_wait_ex_returns_on_a_signal(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    lc_result_ptr rp = lc_async_ring_create(4);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;
    TEST_REQUIRE(lc_is_ok(lc_signal_handle(LC_SIGALRM, on_alarm)));

    alarms = 0;
    int64_t timer[4] = { 0, 0, 0, 20000 };   /* struct itimerval: 20 ms, once */
    TEST_REQUIRE(lc_syscall3(SYS_setitimer, UF_ITIMER_REAL, (int64_t)timer, 0) == 0);
    lc_async_result r[4];
    lc_result w = lc_async_wait_ex(ring, r, 4);   /* nothing in flight: only the signal ends it */
    TEST_ASSERT_EQ(alarms, 1);
    TEST_ASSERT_EQ(w.error, 0);
    TEST_ASSERT_EQ(w.value, (int64_t)0);

    TEST_ASSERT(lc_is_ok(lc_signal_reset(LC_SIGALRM)));
    lc_async_ring_destroy(ring);
}

/* lc_async_wait waits again after a signal, where lc_async_wait_ex returns:
 * a timeout of 100 ms is queued and a SIGALRM comes at 20 ms, so the wait
 * must return the timeout's completion, -ETIME, after it has expired. */
#define IORING_OP_TIMEOUT 11

static void test_async_wait_retries_a_signal(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    lc_result_ptr rp = lc_async_ring_create(4);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    lc_async_ring *ring = rp.value;
    TEST_REQUIRE(lc_is_ok(lc_signal_handle(LC_SIGALRM, on_alarm)));

    static int64_t timeout[2] = { 0, 100000000 };   /* struct __kernel_timespec: 100 ms */
    TEST_REQUIRE(lc_is_ok(lc_async_submit_raw(ring, IORING_OP_TIMEOUT, -1, (uint64_t)(uintptr_t)timeout,
                                              1, 0, 0, 7)));
    alarms = 0;
    int64_t start = lc_time_now_monotonic();
    int64_t timer[4] = { 0, 0, 0, 20000 };   /* struct itimerval: 20 ms, once */
    TEST_REQUIRE(lc_syscall3(SYS_setitimer, UF_ITIMER_REAL, (int64_t)timer, 0) == 0);
    lc_async_result r[4] = {0};
    uint32_t n = lc_async_wait(ring, r, 4);
    int64_t waited = lc_time_now_monotonic() - start;

    TEST_ASSERT_EQ(alarms, 1);
    TEST_ASSERT_EQ(n, (uint32_t)1);
    TEST_ASSERT_EQ(r[0].tag, (uint64_t)7);
    TEST_ASSERT_EQ(r[0].result, -LC_ERR_TIME);
    TEST_ASSERT(waited >= 90000000);   /* the timeout's, not the signal's */
    TEST_ASSERT(lc_is_ok(lc_signal_reset(LC_SIGALRM)));
    lc_async_ring_destroy(ring);
}

/* The ring's fd is close-on-exec, as the kernel makes every io_uring fd:
 * the fd of /proc/self/fd that is anon_inode:[io_uring] after the create
 * and not before must have FD_CLOEXEC. */
#define UF_F_GETFD    1
#define UF_FD_CLOEXEC 1

static bool is_ring_fd(int64_t fd) {
    char path[32] = "/proc/self/fd/";
    size_t at = 14;
    char digits[20];
    size_t n = 0;
    do { digits[n++] = (char)('0' + fd % 10); fd /= 10; } while (fd > 0);
    while (n > 0) path[at++] = digits[--n];
    path[at] = 0;
    char target[64];
    lc_sysret len = lc_kernel_readlinkat(AT_FDCWD, path, target, sizeof target);
    return len > 0 && lc_string_equal(target, (size_t)len, "anon_inode:[io_uring]", 21);
}

static void test_async_ring_fd_close_on_exec(void) {
    if (!io_uring_available) TEST_SKIP(NO_IO_URING);
    static bool before[1024];
    for (int64_t fd = 0; fd < 1024; fd++) before[fd] = is_ring_fd(fd);
    lc_result_ptr rp = lc_async_ring_create(4);
    TEST_REQUIRE(lc_ptr_is_ok(rp));
    int32_t found = 0;
    for (int64_t fd = 0; fd < 1024; fd++) {
        if (before[fd] || !is_ring_fd(fd)) continue;
        found++;
        TEST_ASSERT((lc_syscall3(SYS_fcntl, fd, UF_F_GETFD, 0) & UF_FD_CLOEXEC) != 0);
    }
    TEST_ASSERT_EQ(found, 1);
    lc_async_ring_destroy(rp.value);
}

/* ===== main ===== */

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    lc_result_ptr probe = lc_async_ring_create(4);
    io_uring_available = lc_ptr_is_ok(probe);
    if (io_uring_available) lc_async_ring_destroy(probe.value);

    /* ring create/destroy */
    TEST_RUN(test_async_ring_create_destroy);

    /* submit read/write via pipe */
    TEST_RUN(test_async_submit_read_write);

    /* tag matching across multiple ops */
    TEST_RUN(test_async_tag_matching);

    /* peek with no pending ops */
    TEST_RUN(test_async_peek_empty);

    /* flush returns submitted count */
    TEST_RUN(test_async_flush_count);

    /* free slots decrease after submissions */
    TEST_RUN(test_async_free_slots);

    /* multi-cycle flush accounting (tail/pending bookkeeping) */
    TEST_RUN(test_async_multiflush_accounting);

    /* the _ex calls report what io_uring_enter did */
    TEST_RUN(test_async_flush_ex_and_wait_ex);
    TEST_RUN(test_async_ex_report_a_closed_ring);
    TEST_RUN(test_async_ex_report_a_kept_error);
    TEST_RUN(test_async_flush_ex_on_a_full_cq_returns);
    TEST_RUN(test_async_wait_ex_returns_on_a_signal);
    TEST_RUN(test_async_wait_retries_a_signal);
    TEST_RUN(test_async_ring_fd_close_on_exec);

    return test_main();
}

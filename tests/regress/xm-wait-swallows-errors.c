/*
 * xm-wait-swallows-errors — when io_uring_enter keeps failing, lio_loop_run
 * must return and say why, not spin.
 *
 * lc_async_wait retried EINTR and returned 0 on any other error, and
 * lio_loop_run took 0 for "nothing completed yet" and waited again. Once
 * the ring's fd was closed, by a stray close anywhere in the program, every
 * enter failed at once with EBADF, and the loop spun at full CPU forever,
 * reporting nothing. lio_loop_run now uses lc_async_flush_ex and
 * lc_async_wait_ex, and on an error but EINTR, EAGAIN or EBUSY it returns,
 * with the errno in lio_loop_error.
 *
 * A handler closes the ring's fd, the lowest free one when the loop was
 * created. The loop runs in a child under a 3 s watchdog, as with the
 * defect it never returned. The program needs io_uring, which qemu-user
 * lacks, and skips there.
 */

#include "test.h"
#include <lightc/async.h>
#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <lightio/lightio.h>

#define ITIMER_REAL 0
#define PORT        19940

static int32_t ring_fd = -1;

static void close_the_ring(lio_stream *stream) {
    lc_kernel_close_file(ring_fd);
    char buf[16];
    (void)lio_read(stream, buf, sizeof buf);   /* suspends: the loop must notice */
}

static int32_t client(void *arg) {
    (void)arg;
    lc_time_sleep_milliseconds(50);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, PORT);
    if (lc_is_err(c)) lc_kernel_exit(30);
    return 0;
}

/* Runs the loop; exits 0 if it returned with EBADF kept. */
static int32_t serve(void) {
    /* The ring's fd is the lowest free one when the loop is created. */
    lc_sysret probe = lc_kernel_open_file("/dev/null", O_RDONLY, 0);
    if (probe < 0) return 10;
    lc_kernel_close_file((int32_t)probe);
    ring_fd = (int32_t)probe;

    lio_loop *loop = lio_loop_create();
    if (!loop) return 11;
    if (lc_is_err(lio_tcp_serve(loop, PORT, close_the_ring))) return 12;
    lc_thread t;
    if (lc_is_err(lc_thread_create(&t, client, NULL))) return 13;

    lio_loop_run(loop);   /* the defect: never returns */
    return lio_loop_error(loop) == LC_ERR_BADF ? 0 : 20;
}

static void test_loop_returns_with_ebadf(void) {
    lc_result_ptr ring = lc_async_ring_create(4);
    if (lc_ptr_is_err(ring)) TEST_SKIP("io_uring not available");
    lc_async_ring_destroy(ring.value);

    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, 3, 0 };   /* struct itimerval: 3 s, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        lc_kernel_exit(serve());
    }
    TEST_REQUIRE(pid > 0);
    int32_t status = 0;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status & 0x7f, 0);          /* 14, SIGALRM: still spinning after 3 s */
    TEST_ASSERT_EQ((status >> 8) & 0xff, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_loop_returns_with_ebadf);
    return test_main();
}

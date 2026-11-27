/*
 * NET-uring-sigpipe — one client must not be able to end a lightio server
 * with SIGPIPE.
 *
 * lio_write, and httpd's response, wrote with IORING_OP_WRITE, which on a
 * socket whose peer has reset the connection raises SIGPIPE as write(2)
 * does: a client that sent a request, shut down its side and reset the
 * connection ended the server. They now send with IORING_OP_SEND and
 * MSG_NOSIGNAL, and the write fails with -EPIPE or -ECONNRESET.
 *
 * In a child, a lightio handler reads one byte from a client that then
 * shuts down its sending side and closes with SO_LINGER 0, as the review's
 * v_killer does. The FIN puts the server's end in CLOSE_WAIT, where the
 * reset makes a send fail with EPIPE, the error that raises SIGPIPE; the
 * reset of an established connection gives ECONNRESET, which does not.
 * The handler writes until a write fails with -EPIPE, and the child must
 * live. It runs under a 5 s watchdog. The program needs io_uring, which
 * qemu-user lacks, and skips there.
 */

#include "test.h"
#include <lightio/lightio.h>
#include <lightc/async.h>
#include <lightc/signal.h>
#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define PORT        19944
#define SO_LINGER   13
#define ITIMER_REAL 0

static lio_loop *loop;
static _Atomic(int32_t) last_write = 1;

static void write_after_reset(lio_stream *stream) {
    char byte;
    if (lio_read(stream, &byte, 1) != 1) return;
    lio_sleep(stream, 50);   /* the FIN and the reset arrive */
    int32_t n = 1;
    for (int i = 0; i < 100 && n != -LC_ERR_PIPE; i++) n = lio_write(stream, "response\n", 9);
    atomic_store(&last_write, n);
    lio_loop_stop(loop);
}

static int32_t reset_client(void *arg) {
    (void)arg;
    lc_time_sleep_milliseconds(50);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, PORT);
    if (lc_is_err(c)) return 1;
    int32_t fd = (int32_t)c.value;
    (void)lc_socket_send(fd, "x", 1);
    (void)lc_socket_shutdown(fd, LC_SHUT_WRITE);            /* FIN: the server's end in CLOSE_WAIT */
    struct { int32_t onoff, seconds; } linger = { 1, 0 };   /* close resets */
    (void)lc_socket_set_option(fd, LC_SOL_SOCKET, SO_LINGER, &linger, sizeof linger);
    lc_socket_close(fd);
    /* Wakes the loop once the handler has stopped it. */
    for (int i = 0; i < 400 && atomic_load(&last_write) > 0; i++) lc_time_sleep_milliseconds(10);
    lc_result wake = lc_socket_connect_to(127, 0, 0, 1, PORT);
    if (lc_is_ok(wake)) lc_socket_close((int32_t)wake.value);
    return 0;
}

static int32_t serve(void) {
    (void)lc_signal_reset(LC_SIGPIPE);   /* SIGPIPE's default action: end the process */
    loop = lio_loop_create();
    if (!loop || lc_is_err(lio_tcp_serve(loop, PORT, write_after_reset))) return 10;
    lc_thread client;
    if (lc_is_err(lc_thread_create(&client, reset_client, NULL))) return 11;
    lio_loop_run(loop);
    lc_thread_join(&client);
    return atomic_load(&last_write) == -LC_ERR_PIPE ? 0 : 12;
}

static void test_write_to_a_reset_client(void) {
    lc_result_ptr ring = lc_async_ring_create(4);
    if (lc_ptr_is_err(ring)) TEST_SKIP("io_uring not available");
    lc_async_ring_destroy(ring.value);

    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, 5, 0 };   /* struct itimerval: 5 s, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        lc_kernel_exit(serve());
    }
    TEST_REQUIRE(pid > 0);
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    TEST_ASSERT_EQ(status & 0x7f, 0);   /* the defect: 13, SIGPIPE */
    TEST_ASSERT_EQ(status, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_write_to_a_reset_client);
    return test_main();
}

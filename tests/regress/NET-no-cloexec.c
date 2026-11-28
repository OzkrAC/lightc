/*
 * NET-no-cloexec — every socket lightc opens or accepts must be
 * close-on-exec.
 *
 * An earlier fix made lc_socket_create and lc_socket_accept open their
 * sockets close-on-exec, but lc_socket_tcp6, lc_socket_udp6 and
 * lc_socket_accept6 passed no SOCK_CLOEXEC, nor did lightio's accepts, so
 * a program those made exec'd held the sockets open. Each now has the flag,
 * as lc_socket_open and lc_socket_accept_flags give it unless asked not to.
 *
 * FD_CLOEXEC is read back with fcntl for the sockets of each. The IPv6
 * listener is a dual-stack one on ::, which an IPv4 connection reaches, so
 * the host needs no IPv6 address; it skips only where the kernel has no
 * IPv6 at all. lightio's check needs io_uring, which qemu-user lacks, and
 * skips there.
 */

#include "test.h"
#include <lightio/lightio.h>
#include <lightc/async.h>
#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define F_GETFD    1
#define FD_CLOEXEC 1
#define PORT       19943

static bool close_on_exec(int32_t fd) {
    return (lc_syscall3(SYS_fcntl, fd, F_GETFD, 0) & FD_CLOEXEC) != 0;
}

static void test_ipv4_sockets(void) {
    lc_result t = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    lc_result u = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    TEST_REQUIRE_OK(t);
    TEST_REQUIRE_OK(u);
    TEST_ASSERT(close_on_exec((int32_t)t.value));
    TEST_ASSERT(close_on_exec((int32_t)u.value));
    lc_socket_close((int32_t)t.value);
    lc_socket_close((int32_t)u.value);

    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 0);
    lc_result l = lc_socket_listen_on(&addr, 1);
    TEST_REQUIRE_OK(l);
    lc_socket_address bound;
    uint32_t size = sizeof bound;
    TEST_REQUIRE(lc_kernel_getsockname((int32_t)l.value, &bound, &size) == 0);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, lc_network_to_host_16(bound.port));
    TEST_REQUIRE_OK(c);
    lc_result a = lc_socket_accept_flags((int32_t)l.value, NULL, NULL, 0);
    TEST_REQUIRE_OK(a);
    TEST_ASSERT(close_on_exec((int32_t)l.value));
    TEST_ASSERT(close_on_exec((int32_t)c.value));
    TEST_ASSERT(close_on_exec((int32_t)a.value));
    lc_socket_close((int32_t)a.value);
    lc_socket_close((int32_t)c.value);
    lc_socket_close((int32_t)l.value);
}

static void test_ipv6_sockets(void) {
    lc_result t = lc_socket_tcp6();
    if (t.error == LC_ERR_AFNOSUPPORT) TEST_SKIP("no IPv6");
    TEST_REQUIRE_OK(t);
    lc_result u = lc_socket_udp6();
    TEST_REQUIRE_OK(u);
    TEST_ASSERT(close_on_exec((int32_t)t.value));   /* the defect: not close-on-exec */
    TEST_ASSERT(close_on_exec((int32_t)u.value));
    lc_socket_close((int32_t)u.value);

    /* A dual-stack listener on ::, which takes IPv4 connections as mapped
     * addresses, so the accept needs no IPv6 address on the host. */
    int32_t listener = (int32_t)t.value;
    lc_socket_address6 addr = lc_socket_address6_any(0);
    TEST_REQUIRE_OK(lc_socket_bind6(listener, &addr));
    TEST_REQUIRE_OK(lc_socket_listen6(listener, 1));
    lc_socket_address6 bound;
    uint32_t size = sizeof bound;
    TEST_REQUIRE(lc_kernel_getsockname(listener, &bound, &size) == 0);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, lc_network_to_host_16(bound.port));
    TEST_REQUIRE_OK(c);
    lc_socket_address6 peer;
    lc_result a = lc_socket_accept6(listener, &peer);
    TEST_REQUIRE_OK(a);
    TEST_ASSERT(close_on_exec((int32_t)a.value));   /* the defect: not close-on-exec */
    lc_socket_close((int32_t)a.value);
    lc_socket_close((int32_t)c.value);
    lc_socket_close(listener);
}

static _Atomic(int32_t) handled;
static bool accepted_close_on_exec;

static void check_stream(lio_stream *stream) {
    accepted_close_on_exec = close_on_exec(lio_stream_fd(stream));
    atomic_store(&handled, 1);
}

static int32_t serve(void *loop) {
    lio_loop_run(loop);
    return 0;
}

static void test_lightio_accepted_socket(void) {
    lc_result_ptr ring = lc_async_ring_create(4);
    if (lc_ptr_is_err(ring)) TEST_SKIP("io_uring not available");
    lc_async_ring_destroy(ring.value);

    lio_loop *loop = lio_loop_create();
    TEST_REQUIRE_NOT_NULL(loop);
    TEST_REQUIRE_OK(lio_tcp_serve(loop, PORT, check_stream));
    lc_thread server;
    TEST_REQUIRE_OK(lc_thread_create(&server, serve, loop));
    lc_time_sleep_milliseconds(50);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, PORT);
    for (int i = 0; i < 500 && !atomic_load(&handled); i++) lc_time_sleep_milliseconds(10);
    if (lc_is_ok(c)) lc_socket_close((int32_t)c.value);
    lio_loop_stop(loop);
    lc_result wake = lc_socket_connect_to(127, 0, 0, 1, PORT);
    if (lc_is_ok(wake)) lc_socket_close((int32_t)wake.value);
    lc_thread_join(&server);
    lio_loop_destroy(loop);

    TEST_ASSERT_EQ(atomic_load(&handled), 1);
    TEST_ASSERT(accepted_close_on_exec);   /* the defect: not close-on-exec */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_ipv4_sockets);
    TEST_RUN(test_ipv6_sockets);
    TEST_RUN(test_lightio_accepted_socket);
    return test_main();
}

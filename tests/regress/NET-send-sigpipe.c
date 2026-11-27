/*
 * NET-send-sigpipe — a send to a peer that has closed must fail with
 * EPIPE, not end the process with SIGPIPE.
 *
 * lc_socket_send_to passed no flags to sendto, so on a stream socket whose
 * peer had reset the connection the kernel raised SIGPIPE, whose default
 * action ends the process; lc_socket_send had the same defect until an
 * earlier fix. Every send now passes MSG_NOSIGNAL, and the error comes back
 * as LC_ERR_PIPE.
 *
 * A child connects over loopback to a listener, whose end of the
 * connection then closes with SO_LINGER 0, which resets it. The child sends
 * until a send fails: it must get EPIPE, perhaps after the reset's
 * ECONNRESET, and live. Once each with lc_socket_send_to,
 * lc_socket_send_to_sockaddr and lc_socket_send, whose MSG_NOSIGNAL an
 * earlier fix added.
 */

#include "test.h"
#include <lightc/signal.h>
#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

#define SO_LINGER 13

/* A connected socket whose peer has reset the connection, or -1. */
static int32_t reset_connection(void) {
    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 0);
    lc_result l = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(l)) return -1;
    int32_t listener = (int32_t)l.value;
    lc_socket_address bound;
    uint32_t size = sizeof bound;
    if (lc_is_err(lc_socket_bind(listener, &addr)) || lc_is_err(lc_socket_listen(listener, 1)) ||
        lc_kernel_getsockname(listener, &bound, &size) < 0)
        return -1;

    lc_result c = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(c) || lc_is_err(lc_socket_connect((int32_t)c.value, &bound))) return -1;
    lc_result a = lc_socket_accept_flags(listener, NULL, NULL, 0);
    if (lc_is_err(a)) return -1;

    struct { int32_t onoff, seconds; } linger = { 1, 0 };   /* close resets */
    if (lc_is_err(lc_socket_set_option((int32_t)a.value, LC_SOL_SOCKET, SO_LINGER, &linger, sizeof linger)))
        return -1;
    lc_socket_close((int32_t)a.value);
    lc_socket_close(listener);
    lc_time_sleep_milliseconds(20);
    return (int32_t)c.value;
}

enum { SEND_TO, SEND_TO_SOCKADDR, SEND };

static int32_t send_until_it_fails(int32_t how) {
    (void)lc_signal_reset(LC_SIGPIPE);   /* SIGPIPE's default action: end the process */
    int32_t fd = reset_connection();
    if (fd < 0) return 10;
    for (int i = 0; i < 100; i++) {
        lc_result r = how == SEND_TO          ? lc_socket_send_to(fd, "x", 1, NULL)
                    : how == SEND_TO_SOCKADDR ? lc_socket_send_to_sockaddr(fd, "x", 1, 0, NULL, 0)
                    :                           lc_socket_send(fd, "x", 1);
        if (r.error == LC_ERR_PIPE) return 0;
        if (r.error != 0 && r.error != LC_ERR_CONNRESET) return 20;
    }
    return 11;   /* no send failed */
}

/* The exit status of a child that runs send_until_it_fails. */
static int32_t in_child(int32_t how) {
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) lc_kernel_exit(send_until_it_fails(how));
    if (pid < 0) return -1;
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    return status;
}

static void test_send_to_a_reset_peer(void) {
    int32_t status = in_child(SEND_TO);
    TEST_ASSERT_EQ(status & 0x7f, 0);   /* the defect: 13, SIGPIPE */
    TEST_ASSERT_EQ(status, 0);
}

static void test_send_to_sockaddr_to_a_reset_peer(void) {
    int32_t status = in_child(SEND_TO_SOCKADDR);
    TEST_ASSERT_EQ(status & 0x7f, 0);
    TEST_ASSERT_EQ(status, 0);
}

static void test_send_to_a_reset_peer_without_an_address(void) {
    int32_t status = in_child(SEND);
    TEST_ASSERT_EQ(status & 0x7f, 0);
    TEST_ASSERT_EQ(status, 0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_send_to_a_reset_peer);
    TEST_RUN(test_send_to_sockaddr_to_a_reset_peer);
    TEST_RUN(test_send_to_a_reset_peer_without_an_address);
    return test_main();
}

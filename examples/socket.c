/*
 * Exercise the socket networking layer: TCP client/server, UDP, convenience functions.
 */
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/print.h>
#include <lightc/socket.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#include "checks.h"

#define S(literal) literal, sizeof(literal) - 1

/* ================================================================
 * Free ports and bounded waits, for all three exchanges
 *
 * Each server binds port 0, so the kernel picks a free port, and reads
 * that port back with getsockname. It stores the port, then sets its ready
 * flag to 1, or sets the flag to -1 if it cannot start. Its client waits
 * for the flag at most WAIT_MS, and accept and receive give up after the
 * same time, so no thread waits forever.
 * ================================================================ */

#define WAIT_MS 5000

/* SO_RCVTIMEO from the kernel's <asm-generic/socket.h>, which x86_64 and
 * aarch64 both use. It takes a struct timeval, two 64-bit fields on both. */
#define LINUX_SO_RCVTIMEO 20

/* Make accept and receive on fd fail with EAGAIN after WAIT_MS. A socket
 * that accept returns inherits this from the listening socket. */
static bool limit_wait(int32_t fd) {
    struct { int64_t seconds; int64_t microseconds; } timeout = { WAIT_MS / 1000, 0 };
    return lc_syscall5(SYS_setsockopt, fd, LC_SOL_SOCKET, LINUX_SO_RCVTIMEO,
                       (int64_t)&timeout, sizeof(timeout)) == 0;
}

/* The port fd is bound to, in host byte order, or 0 if getsockname fails. */
static uint16_t bound_port(int32_t fd) {
    lc_socket_address addr;
    uint32_t addr_length = sizeof(addr);
    if (lc_kernel_getsockname(fd, &addr, &addr_length) != 0) return 0;
    return lc_network_to_host_16(addr.port);
}

/* Close fd, if the server has one yet, and set *ready to -1, so the client
 * stops waiting. Returns the server thread's exit code. */
static int32_t server_failed(int32_t fd, _Atomic(int32_t) *ready) {
    if (fd >= 0) lc_socket_close(fd);
    atomic_store_explicit(ready, -1, memory_order_release);
    return 1;
}

/* Wait at most WAIT_MS for a server to set *ready. True once it is ready,
 * false if it failed or the time ran out. */
static bool wait_for_server(_Atomic(int32_t) *ready) {
    int64_t start = lc_time_start_timer();
    for (;;) {
        int32_t state = atomic_load_explicit(ready, memory_order_acquire);
        if (state != 0) return state == 1;
        if (lc_time_elapsed_milliseconds(start) >= WAIT_MS) return false;
        lc_time_sleep_microseconds(100);
    }
}

/* ================================================================
 * TCP test — server/client exchange via threads
 * ================================================================ */

/* Server state: 0 = starting, 1 = listening on tcp_port, -1 = failed */
static _Atomic(int32_t) server_ready = 0;
static uint16_t tcp_port;   /* set before server_ready = 1 */

/* Server thread: accept one connection, receive, send back */
static int32_t tcp_server_thread(void *arg) {
    int32_t *ok = (int32_t *)arg;
    *ok = 0;

    /* Create, bind to port 0 (any free port), listen */
    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(r)) return server_failed(-1, &server_ready);
    int32_t server_fd = (int32_t)r.value;

    if (lc_is_err(lc_socket_set_reuse_address(server_fd)))
        return server_failed(server_fd, &server_ready);

    lc_socket_address addr = lc_socket_address_any(0);
    if (lc_is_err(lc_socket_bind(server_fd, &addr)))
        return server_failed(server_fd, &server_ready);

    if (lc_is_err(lc_socket_listen(server_fd, 1)))
        return server_failed(server_fd, &server_ready);

    /* Read back the port the kernel picked, and bound accept and receive */
    tcp_port = bound_port(server_fd);
    if (tcp_port == 0 || !limit_wait(server_fd))
        return server_failed(server_fd, &server_ready);

    /* Signal that we are listening */
    atomic_store_explicit(&server_ready, 1, memory_order_release);

    /* Accept one connection */
    lc_socket_address client_addr;
    uint32_t client_addr_size = sizeof client_addr;
    r = lc_socket_accept_flags(server_fd, &client_addr, &client_addr_size, 0);
    if (lc_is_err(r)) {
        lc_socket_close(server_fd);
        return 1;
    }
    int32_t client_fd = (int32_t)r.value;

    /* Receive message from client */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_result rr = lc_socket_receive(client_fd, buf, sizeof(buf));
    if (lc_is_err(rr) || rr.value <= 0) {
        lc_socket_close(client_fd);
        lc_socket_close(server_fd);
        return 1;
    }

    /* Verify message */
    const char *expected = "hello from client";
    size_t expected_len = 17;
    if (!lc_string_equal(buf, (size_t)rr.value, expected, expected_len)) {
        lc_socket_close(client_fd);
        lc_socket_close(server_fd);
        return 1;
    }

    /* Send response */
    const char *response = "hello from server";
    size_t response_len = 17;
    lc_result sr = lc_socket_send(client_fd, response, response_len);
    if (lc_is_err(sr) || sr.value != (int64_t)response_len) {
        lc_socket_close(client_fd);
        lc_socket_close(server_fd);
        return 1;
    }

    lc_socket_close(client_fd);
    lc_socket_close(server_fd);
    *ok = 1;
    return 0;
}

/* Client thread: connect, send, receive */
static int32_t tcp_client_thread(void *arg) {
    int32_t *ok = (int32_t *)arg;
    *ok = 0;

    /* Wait for server to be ready, or to fail */
    if (!wait_for_server(&server_ready)) return 1;

    /* Connect to server */
    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(r)) return 1;
    int32_t fd = (int32_t)r.value;

    if (!limit_wait(fd)) {
        lc_socket_close(fd);
        return 1;
    }

    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, tcp_port);
    if (lc_is_err(lc_socket_connect(fd, &addr))) {
        lc_socket_close(fd);
        return 1;
    }

    /* Send message */
    const char *msg = "hello from client";
    size_t msg_len = 17;
    lc_result sr = lc_socket_send(fd, msg, msg_len);
    if (lc_is_err(sr) || sr.value != (int64_t)msg_len) {
        lc_socket_close(fd);
        return 1;
    }

    /* Receive response */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_result rr = lc_socket_receive(fd, buf, sizeof(buf));
    if (lc_is_err(rr) || rr.value <= 0) {
        lc_socket_close(fd);
        return 1;
    }

    /* Verify response */
    const char *expected = "hello from server";
    size_t expected_len = 17;
    if (!lc_string_equal(buf, (size_t)rr.value, expected, expected_len)) {
        lc_socket_close(fd);
        return 1;
    }

    lc_socket_close(fd);
    *ok = 1;
    return 0;
}

/* ================================================================
 * UDP test — send/receive datagram on loopback
 * ================================================================ */

/* Server state: 0 = starting, 1 = bound to udp_port, -1 = failed */
static _Atomic(int32_t) udp_server_ready = 0;
static uint16_t udp_port;   /* set before udp_server_ready = 1 */

static int32_t udp_server_thread(void *arg) {
    int32_t *ok = (int32_t *)arg;
    *ok = 0;

    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    if (lc_is_err(r)) return server_failed(-1, &udp_server_ready);
    int32_t fd = (int32_t)r.value;

    /* Bind to port 0 (any free port). No SO_REUSEADDR: UDP sockets that all
     * set it can be given the same port by a bind to port 0, and then one
     * of them gets the datagrams meant for another. */
    lc_socket_address addr = lc_socket_address_any(0);
    if (lc_is_err(lc_socket_bind(fd, &addr)))
        return server_failed(fd, &udp_server_ready);

    /* Read back the port the kernel picked, and bound receive */
    udp_port = bound_port(fd);
    if (udp_port == 0 || !limit_wait(fd))
        return server_failed(fd, &udp_server_ready);

    /* Signal that we are bound */
    atomic_store_explicit(&udp_server_ready, 1, memory_order_release);

    /* Receive a datagram */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_socket_address sender;
    lc_result rr = lc_socket_receive_from(fd, buf, sizeof(buf), &sender);
    if (lc_is_err(rr) || rr.value <= 0) {
        lc_socket_close(fd);
        return 1;
    }

    /* Verify */
    const char *expected = "udp hello";
    size_t expected_len = 9;
    if (!lc_string_equal(buf, (size_t)rr.value, expected, expected_len)) {
        lc_socket_close(fd);
        return 1;
    }

    /* Send reply back to sender */
    const char *reply = "udp reply";
    size_t reply_len = 9;
    lc_result sr = lc_socket_send_to(fd, reply, reply_len, &sender);
    if (lc_is_err(sr) || sr.value != (int64_t)reply_len) {
        lc_socket_close(fd);
        return 1;
    }

    lc_socket_close(fd);
    *ok = 1;
    return 0;
}

static int32_t udp_client_thread(void *arg) {
    int32_t *ok = (int32_t *)arg;
    *ok = 0;

    /* Wait for server to be ready, or to fail */
    if (!wait_for_server(&udp_server_ready)) return 1;

    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    if (lc_is_err(r)) return 1;
    int32_t fd = (int32_t)r.value;

    if (!limit_wait(fd)) {
        lc_socket_close(fd);
        return 1;
    }

    /* Send datagram to server */
    lc_socket_address dest = lc_socket_address_create(127, 0, 0, 1, udp_port);
    const char *msg = "udp hello";
    size_t msg_len = 9;
    lc_result sr = lc_socket_send_to(fd, msg, msg_len, &dest);
    if (lc_is_err(sr) || sr.value != (int64_t)msg_len) {
        lc_socket_close(fd);
        return 1;
    }

    /* Receive reply */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_socket_address sender;
    lc_result rr = lc_socket_receive_from(fd, buf, sizeof(buf), &sender);
    if (lc_is_err(rr) || rr.value <= 0) {
        lc_socket_close(fd);
        return 1;
    }

    const char *expected = "udp reply";
    size_t expected_len = 9;
    if (!lc_string_equal(buf, (size_t)rr.value, expected, expected_len)) {
        lc_socket_close(fd);
        return 1;
    }

    lc_socket_close(fd);
    *ok = 1;
    return 0;
}

/* ================================================================
 * Convenience functions test
 * ================================================================ */

/* Server state: 0 = starting, 1 = listening on conv_port, -1 = failed */
static _Atomic(int32_t) conv_server_ready = 0;
static uint16_t conv_port;   /* set before conv_server_ready = 1 */

static int32_t conv_server_thread(void *arg) {
    int32_t *ok = (int32_t *)arg;
    *ok = 0;

    /* Use convenience function: create + bind to port 0 of loopback + listen */
    lc_socket_address listen_addr = lc_socket_address_create(127, 0, 0, 1, 0);
    lc_result r = lc_socket_listen_on(&listen_addr, 1);
    if (lc_is_err(r)) return server_failed(-1, &conv_server_ready);
    int32_t server_fd = (int32_t)r.value;

    /* Read back the port the kernel picked, and bound accept and receive */
    conv_port = bound_port(server_fd);
    if (conv_port == 0 || !limit_wait(server_fd))
        return server_failed(server_fd, &conv_server_ready);

    /* Signal that we are listening */
    atomic_store_explicit(&conv_server_ready, 1, memory_order_release);

    /* Accept one connection */
    r = lc_socket_accept_flags(server_fd, NULL, NULL, 0);
    if (lc_is_err(r)) {
        lc_socket_close(server_fd);
        return 1;
    }
    int32_t client_fd = (int32_t)r.value;

    /* Receive and echo back */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_result rr = lc_socket_receive(client_fd, buf, sizeof(buf));
    if (lc_is_err(rr) || rr.value <= 0) {
        lc_socket_close(client_fd);
        lc_socket_close(server_fd);
        return 1;
    }

    lc_result sr = lc_socket_send(client_fd, buf, (size_t)rr.value);
    if (lc_is_err(sr) || sr.value != rr.value) {
        lc_socket_close(client_fd);
        lc_socket_close(server_fd);
        return 1;
    }

    lc_socket_close(client_fd);
    lc_socket_close(server_fd);
    *ok = 1;
    return 0;
}

static int32_t conv_client_thread(void *arg) {
    int32_t *ok = (int32_t *)arg;
    *ok = 0;

    /* Wait for server to be ready, or to fail */
    if (!wait_for_server(&conv_server_ready)) return 1;

    /* Use convenience function: create + connect */
    lc_result r = lc_socket_connect_to(127, 0, 0, 1, conv_port);
    if (lc_is_err(r)) return 1;
    int32_t fd = (int32_t)r.value;

    if (!limit_wait(fd)) {
        lc_socket_close(fd);
        return 1;
    }

    /* Send a message */
    const char *msg = "convenience test";
    size_t msg_len = 16;
    lc_result sr = lc_socket_send(fd, msg, msg_len);
    if (lc_is_err(sr) || sr.value != (int64_t)msg_len) {
        lc_socket_close(fd);
        return 1;
    }

    /* Receive echo */
    char buf[64];
    lc_bytes_fill(buf, 0, sizeof(buf));
    lc_result rr = lc_socket_receive(fd, buf, sizeof(buf));
    if (lc_is_err(rr) || rr.value <= 0) {
        lc_socket_close(fd);
        return 1;
    }

    if (!lc_string_equal(buf, (size_t)rr.value, msg, msg_len)) {
        lc_socket_close(fd);
        return 1;
    }

    lc_socket_close(fd);
    *ok = 1;
    return 0;
}

/* ================================================================
 * Main
 * ================================================================ */

int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

    bool ok;

    /* --- TCP server/client exchange --- */
    lc_print_string(STDOUT, S("tcp_exchange"));
    {
        atomic_store(&server_ready, 0);
        int32_t server_ok = 0, client_ok = 0;
        lc_thread server, client;

        bool server_started = !lc_is_err(lc_thread_create(&server, tcp_server_thread, &server_ok));
        bool client_started = server_started
            && !lc_is_err(lc_thread_create(&client, tcp_client_thread, &client_ok));
        /* Join each thread that started. A server whose client did not
         * start gives up waiting for it after WAIT_MS. */
        if (client_started) lc_thread_join(&client);
        if (server_started) lc_thread_join(&server);
        ok = client_started && server_ok && client_ok;
    }
    say_pass_fail(ok);

    /* --- UDP send/receive --- */
    lc_print_string(STDOUT, S("udp_exchange"));
    {
        atomic_store(&udp_server_ready, 0);
        int32_t server_ok = 0, client_ok = 0;
        lc_thread server, client;

        bool server_started = !lc_is_err(lc_thread_create(&server, udp_server_thread, &server_ok));
        bool client_started = server_started
            && !lc_is_err(lc_thread_create(&client, udp_client_thread, &client_ok));
        /* Join each thread that started. A server whose client did not
         * start gives up waiting for it after WAIT_MS. */
        if (client_started) lc_thread_join(&client);
        if (server_started) lc_thread_join(&server);
        ok = client_started && server_ok && client_ok;
    }
    say_pass_fail(ok);

    /* --- Convenience functions (listen_on + connect_to) --- */
    lc_print_string(STDOUT, S("convenience_functions"));
    {
        atomic_store(&conv_server_ready, 0);
        int32_t server_ok = 0, client_ok = 0;
        lc_thread server, client;

        bool server_started = !lc_is_err(lc_thread_create(&server, conv_server_thread, &server_ok));
        bool client_started = server_started
            && !lc_is_err(lc_thread_create(&client, conv_client_thread, &client_ok));
        /* Join each thread that started. A server whose client did not
         * start gives up waiting for it after WAIT_MS. */
        if (client_started) lc_thread_join(&client);
        if (server_started) lc_thread_join(&server);
        ok = client_started && server_ok && client_ok;
    }
    say_pass_fail(ok);

    /* --- Shutdown test --- */
    lc_print_string(STDOUT, S("socket_shutdown"));
    {
        lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
        ok = !lc_is_err(r);
        if (ok) {
            int32_t fd = (int32_t)r.value;
            /* Shutdown on an unconnected socket should fail gracefully */
            lc_result shut_r = lc_socket_shutdown(fd, LC_SHUT_BOTH);
            /* It's expected to fail (ENOTCONN), that's fine — just verify no crash */
            (void)shut_r;
            lc_socket_close(fd);
        }
    }
    say_pass_fail(ok);

    /* --- Address helper test --- */
    lc_print_string(STDOUT, S("address_helpers"));
    {
        lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 8080);
        ok = addr.family == LC_AF_INET;
        ok = ok && addr.port == lc_host_to_network_16(8080);
        /* 127.0.0.1 in little-endian memory = 0x0100007F, but stored as bytes: 127,0,0,1 */
        uint8_t *ip = (uint8_t *)&addr.addr;
        ok = ok && ip[0] == 127 && ip[1] == 0 && ip[2] == 0 && ip[3] == 1;

        lc_socket_address any = lc_socket_address_any(9090);
        ok = ok && any.family == LC_AF_INET;
        ok = ok && any.port == lc_host_to_network_16(9090);
        ok = ok && any.addr == 0;
    }
    say_pass_fail(ok);

    /* --- Byte order test --- */
    lc_print_string(STDOUT, S("byte_order"));
    {
        uint16_t h16 = 0x1234;
        uint16_t n16 = lc_host_to_network_16(h16);
        ok = lc_network_to_host_16(n16) == h16;

        uint32_t h32 = 0x12345678;
        uint32_t n32 = lc_host_to_network_32(h32);
        ok = ok && lc_network_to_host_32(n32) == h32;

        /* Verify actual byte swap on little-endian */
        ok = ok && n16 == 0x3412;
        ok = ok && n32 == 0x78563412;
    }
    say_pass_fail(ok);

    return checks_summary();
}

/*
 * test_socket.c — tests for lightc socket networking.
 */

#include "test.h"
#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/string.h>
#include <lightc/time.h>
#include <stdatomic.h>

/* ===== lc_socket_address_create ===== */

static void test_socket_address_create(void) {
    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 8080);

    TEST_ASSERT_EQ(addr.family, (uint16_t)LC_AF_INET);
    TEST_ASSERT_EQ(addr.port, lc_host_to_network_16(8080));

    /* 127.0.0.1 in network byte order = 0x7F000001 big-endian */
    TEST_ASSERT_EQ(addr.addr, lc_host_to_network_32(0x7F000001));
}

/* ===== lc_socket_address_any ===== */

static void test_socket_address_any(void) {
    lc_socket_address addr = lc_socket_address_any(9090);

    TEST_ASSERT_EQ(addr.family, (uint16_t)LC_AF_INET);
    TEST_ASSERT_EQ(addr.port, lc_host_to_network_16(9090));
    TEST_ASSERT_EQ(addr.addr, lc_host_to_network_32(LC_INADDR_ANY));
}

/* ===== Byte order round-trip ===== */

static void test_byte_order(void) {
    /* 16-bit round-trip */
    uint16_t val16 = 0x1234;
    uint16_t net16 = lc_host_to_network_16(val16);
    uint16_t host16 = lc_network_to_host_16(net16);
    TEST_ASSERT_EQ(host16, val16);

    /* 32-bit round-trip */
    uint32_t val32 = 0xDEADBEEF;
    uint32_t net32 = lc_host_to_network_32(val32);
    uint32_t host32 = lc_network_to_host_32(net32);
    TEST_ASSERT_EQ(host32, val32);

    /* Network byte order should be big-endian */
    uint16_t test16 = lc_host_to_network_16(0x0102);
    uint8_t *bytes16 = (uint8_t *)&test16;
    TEST_ASSERT_EQ(bytes16[0], (uint8_t)0x01);
    TEST_ASSERT_EQ(bytes16[1], (uint8_t)0x02);
}

/* ===== Socket create and close ===== */

static void test_socket_create_close(void) {
    /* TCP socket */
    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    TEST_ASSERT_OK(r);
    TEST_ASSERT(r.value >= 0);
    lc_socket_close((int32_t)r.value);

    /* UDP socket */
    lc_result r2 = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    TEST_ASSERT_OK(r2);
    TEST_ASSERT(r2.value >= 0);
    lc_socket_close((int32_t)r2.value);
}

/* ===== Servers on background threads =====
 *
 * Each server binds port 0 unless a test gives it a port, so the kernel
 * gives it a free port and two runs on one machine cannot collide, and
 * reads that port back with getsockname. It stores the port and then
 * sets its ready flag to 1, or sets the flag to -1 if it cannot start.
 * The test waits for the flag at most SOCKET_WAIT_MS. Accept and receive
 * give up after the same time, so a server whose client never comes still
 * ends and can be joined.
 */

#define SOCKET_WAIT_MS 5000

/* SO_RCVTIMEO from the UAPI <asm-generic/socket.h>, which x86_64 and aarch64
 * both use. It takes a struct timeval, two 64-bit fields on both. */
#define LINUX_SO_RCVTIMEO 20

/* Make accept and receive on fd fail with EAGAIN after SOCKET_WAIT_MS.
 * A socket that accept returns inherits this from the listening socket. */
static bool limit_wait(int32_t fd) {
    struct { int64_t seconds; int64_t microseconds; } timeout = { SOCKET_WAIT_MS / 1000, 0 };
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

/* Close fd, if the server has one yet, and set *ready to -1, so the test
 * stops waiting. Returns the server thread's exit code. */
static int32_t server_failed(int32_t fd, _Atomic(int32_t) *ready) {
    if (fd >= 0) lc_socket_close(fd);
    atomic_store(ready, -1);
    return 1;
}

/* Wait at most SOCKET_WAIT_MS for *ready to leave 0, and return it:
 * 1 = the server is up, -1 = it failed, 0 = neither happened in time. */
static int32_t wait_for_server(_Atomic(int32_t) *ready) {
    int64_t start = lc_time_start_timer();
    int32_t state;
    while ((state = atomic_load(ready)) == 0
           && lc_time_elapsed_milliseconds(start) < SOCKET_WAIT_MS) {
        lc_time_sleep_microseconds(100);
    }
    return state;
}

/* ===== TCP exchange with background thread ===== */

static _Atomic(int32_t) tcp_server_ready;  /* 0 = starting, 1 = listening, -1 = failed */
static _Atomic(int32_t) tcp_server_ok;
static uint16_t         tcp_server_port;   /* the port it listens on, set before ready = 1 */

/* arg is the port to listen on, as (void *)(uintptr_t)port; 0 = any free port */
static int32_t tcp_server_thread(void *arg) {
    uint16_t port = (uint16_t)(uintptr_t)arg;

    /* Create server socket */
    lc_result sr = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(sr)) return server_failed(-1, &tcp_server_ready);
    int32_t server_fd = (int32_t)sr.value;

    if (lc_is_err(lc_socket_set_reuse_address(server_fd)))
        return server_failed(server_fd, &tcp_server_ready);

    lc_socket_address addr = lc_socket_address_any(port);
    lc_result br = lc_socket_bind(server_fd, &addr);
    if (lc_is_err(br)) return server_failed(server_fd, &tcp_server_ready);

    lc_result lr = lc_socket_listen(server_fd, 1);
    if (lc_is_err(lr)) return server_failed(server_fd, &tcp_server_ready);

    /* Read back the port bind picked, and bound accept and receive */
    tcp_server_port = bound_port(server_fd);
    if (tcp_server_port == 0 || !limit_wait(server_fd))
        return server_failed(server_fd, &tcp_server_ready);

    /* Signal that server is ready */
    atomic_store(&tcp_server_ready, 1);

    /* Accept one connection */
    lc_socket_address client_addr;
    uint32_t client_addr_size = sizeof client_addr;
    lc_result ar = lc_socket_accept_flags(server_fd, &client_addr, &client_addr_size, 0);
    if (lc_is_err(ar)) { lc_socket_close(server_fd); atomic_store(&tcp_server_ok, 0); return 1; }
    int32_t client_fd = (int32_t)ar.value;

    /* Read data */
    char buf[64];
    lc_result rr = lc_socket_receive(client_fd, buf, sizeof(buf));
    if (lc_is_err(rr) || rr.value == 0) {
        lc_socket_close(client_fd);
        lc_socket_close(server_fd);
        atomic_store(&tcp_server_ok, 0);
        return 1;
    }

    /* Echo it back */
    lc_result er = lc_socket_send(client_fd, buf, (size_t)rr.value);

    lc_socket_close(client_fd);
    lc_socket_close(server_fd);
    atomic_store(&tcp_server_ok, lc_is_ok(er) && er.value == rr.value);
    return 0;
}

/* The client half of the exchange, with a server listening on port. */
static void tcp_client_exchange(uint16_t port) {
    /* Connect as client */
    lc_result cr = lc_socket_connect_to(127, 0, 0, 1, port);
    TEST_REQUIRE_OK(cr);
    int32_t client_fd = (int32_t)cr.value;
    /* Without the timeout the receive below could wait forever. */
    bool limited = limit_wait(client_fd);
    if (!limited) {
        lc_socket_close(client_fd);
    }
    TEST_REQUIRE(limited);

    /* Send data */
    const char *msg = "tcp test";
    size_t msg_len = lc_string_length(msg);
    lc_result sr = lc_socket_send(client_fd, msg, msg_len);
    TEST_ASSERT_OK(sr);
    TEST_ASSERT_EQ(sr.value, (int64_t)msg_len);

    /* Receive echo */
    char buf[64];
    lc_result rr = lc_socket_receive(client_fd, buf, sizeof(buf));
    TEST_ASSERT_OK(rr);
    TEST_ASSERT_EQ(rr.value, (int64_t)msg_len);
    TEST_ASSERT_STR_EQ(buf, (size_t)rr.value, msg, msg_len);

    lc_socket_close(client_fd);
}

static void test_socket_tcp_exchange(void) {
    atomic_store(&tcp_server_ready, 0);
    atomic_store(&tcp_server_ok, 0);

    /* Start server thread on any free port */
    lc_thread t;
    TEST_REQUIRE_OK(lc_thread_create(&t, tcp_server_thread, (void *)(uintptr_t)0));

    /* Wait for server to be ready */
    int32_t ready = wait_for_server(&tcp_server_ready);
    TEST_ASSERT_EQ(ready, 1);
    if (ready == 1) tcp_client_exchange(tcp_server_port);

    lc_thread_join(&t);

    TEST_ASSERT_EQ(atomic_load(&tcp_server_ok), 1);
}

/* ===== Server whose port is taken ===== */

/* Start the TCP server on held_port, which another socket listens on. */
static void start_server_on_held_port(uint16_t held_port) {
    TEST_REQUIRE_NEQ(held_port, 0);
    atomic_store(&tcp_server_ready, 0);
    atomic_store(&tcp_server_ok, 0);

    int64_t start = lc_time_start_timer();
    lc_thread t;
    TEST_REQUIRE_OK(lc_thread_create(&t, tcp_server_thread, (void *)(uintptr_t)held_port));

    /* bind fails, and the server says so at once */
    TEST_ASSERT_EQ(wait_for_server(&tcp_server_ready), -1);
    lc_thread_join(&t);
    TEST_ASSERT(lc_time_elapsed_milliseconds(start) < SOCKET_WAIT_MS);
    TEST_ASSERT_EQ(atomic_load(&tcp_server_ok), 0);
}

static void test_socket_server_port_in_use(void) {
    /* Hold a free port: no other socket can bind a port that one listens on */
    lc_socket_address any = lc_socket_address_any(0);
    lc_result hr = lc_socket_listen_on(&any, 1);
    TEST_REQUIRE_OK(hr);
    int32_t holder_fd = (int32_t)hr.value;

    start_server_on_held_port(bound_port(holder_fd));

    lc_socket_close(holder_fd);
}

/* ===== UDP exchange with background thread ===== */

static _Atomic(int32_t) udp_server_ready;  /* 0 = starting, 1 = bound, -1 = failed */
static _Atomic(int32_t) udp_server_ok;
static uint16_t         udp_server_port;   /* the port it is bound to, set before ready = 1 */

static int32_t udp_server_thread(void *arg) {
    (void)arg;

    /* Create UDP socket */
    lc_result sr = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    if (lc_is_err(sr)) return server_failed(-1, &udp_server_ready);
    int32_t server_fd = (int32_t)sr.value;

    /* No SO_REUSEADDR: UDP sockets that all set it can be given the same
     * port by a bind to port 0, and then one of them gets the datagrams
     * meant for another. */
    lc_socket_address addr = lc_socket_address_any(0);
    lc_result br = lc_socket_bind(server_fd, &addr);
    if (lc_is_err(br)) return server_failed(server_fd, &udp_server_ready);

    /* Read back the port bind picked, and bound receive */
    udp_server_port = bound_port(server_fd);
    if (udp_server_port == 0 || !limit_wait(server_fd))
        return server_failed(server_fd, &udp_server_ready);

    /* Signal that server is ready */
    atomic_store(&udp_server_ready, 1);

    /* Receive one datagram */
    char buf[64];
    lc_socket_address sender;
    lc_result rr = lc_socket_receive_from(server_fd, buf, sizeof(buf), &sender);
    if (lc_is_err(rr) || rr.value == 0) {
        lc_socket_close(server_fd);
        atomic_store(&udp_server_ok, 0);
        return 1;
    }

    /* Echo it back to sender */
    lc_result er = lc_socket_send_to(server_fd, buf, (size_t)rr.value, &sender);

    lc_socket_close(server_fd);
    atomic_store(&udp_server_ok, lc_is_ok(er) && er.value == rr.value);
    return 0;
}

/* The client half of the exchange, with a server bound to port. */
static void udp_client_exchange(uint16_t port) {
    /* Create client UDP socket */
    lc_result cr = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    TEST_REQUIRE_OK(cr);
    int32_t client_fd = (int32_t)cr.value;
    /* Without the timeout the receive below could wait forever. */
    bool limited = limit_wait(client_fd);
    if (!limited) {
        lc_socket_close(client_fd);
    }
    TEST_REQUIRE(limited);

    /* Send datagram to server */
    lc_socket_address dest = lc_socket_address_create(127, 0, 0, 1, port);
    const char *msg = "udp test";
    size_t msg_len = lc_string_length(msg);
    lc_result sr = lc_socket_send_to(client_fd, msg, msg_len, &dest);
    TEST_ASSERT_OK(sr);
    TEST_ASSERT_EQ(sr.value, (int64_t)msg_len);

    /* Receive echo */
    char buf[64];
    lc_socket_address sender;
    lc_result rr = lc_socket_receive_from(client_fd, buf, sizeof(buf), &sender);
    TEST_ASSERT_OK(rr);
    TEST_ASSERT_EQ(rr.value, (int64_t)msg_len);
    TEST_ASSERT_STR_EQ(buf, (size_t)rr.value, msg, msg_len);

    lc_socket_close(client_fd);
}

static void test_socket_udp_exchange(void) {
    atomic_store(&udp_server_ready, 0);
    atomic_store(&udp_server_ok, 0);

    /* Start server thread */
    lc_thread t;
    TEST_REQUIRE_OK(lc_thread_create(&t, udp_server_thread, NULL));

    /* Wait for server to be ready */
    int32_t ready = wait_for_server(&udp_server_ready);
    TEST_ASSERT_EQ(ready, 1);
    if (ready == 1) udp_client_exchange(udp_server_port);

    lc_thread_join(&t);

    TEST_ASSERT_EQ(atomic_load(&udp_server_ok), 1);
}

/* ===== main ===== */

/* Byte-order helpers: round-trip is identity; on a little-endian host (x86_64,
 * aarch64/Linux) host<->network swaps. */
static void test_socket_byte_order(void) {
    TEST_ASSERT_EQ(lc_network_to_host_16(lc_host_to_network_16(0x1234)), (uint16_t)0x1234);
    TEST_ASSERT_EQ(lc_network_to_host_32(lc_host_to_network_32(0x12345678u)), (uint32_t)0x12345678u);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    TEST_ASSERT_EQ(lc_host_to_network_16(0x0102), (uint16_t)0x0201);
    TEST_ASSERT_EQ(lc_host_to_network_32(0x01020304u), (uint32_t)0x04030201u);
#endif
}

/* ===== The complete wrappers: lc_socket_open and the rest ===== */

#define F_GETFD     1
#define F_GETFL     3
#define FD_CLOEXEC  1

static bool close_on_exec(int32_t fd) {
    return (lc_syscall3(SYS_fcntl, fd, F_GETFD, 0) & FD_CLOEXEC) != 0;
}

/* A listener on 127.0.0.1 and a port the kernel picks; sets *port. */
static int32_t loopback_listener(uint16_t *port) {
    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 0);
    lc_result r = lc_socket_listen_on(&addr, 4);
    if (lc_is_err(r)) return -1;
    lc_socket_address bound;
    uint32_t size = sizeof bound;
    if (lc_kernel_getsockname((int32_t)r.value, &bound, &size) < 0) return -1;
    *port = lc_network_to_host_16(bound.port);
    return (int32_t)r.value;
}

/* lc_socket_listen_on binds the address it is given: 127.0.0.1 here. */
static void test_socket_listen_on_an_address(void) {
    lc_socket_address addr = lc_socket_address_create(127, 0, 0, 1, 0);
    lc_result r = lc_socket_listen_on(&addr, 1);
    TEST_REQUIRE_OK(r);
    lc_socket_address bound;
    uint32_t size = sizeof bound;
    TEST_REQUIRE(lc_kernel_getsockname((int32_t)r.value, &bound, &size) == 0);
    TEST_ASSERT_EQ(bound.family, (uint16_t)LC_AF_INET);
    TEST_ASSERT_EQ(lc_network_to_host_32(bound.addr), (uint32_t)LC_INADDR_LOOPBACK);
    TEST_ASSERT(close_on_exec((int32_t)r.value));
    lc_socket_close((int32_t)r.value);
}

/* A netlink socket: an rtnetlink dump of the links, sent and answered
 * through the sockaddr forms with the kernel's address, a sockaddr_nl. */
static void test_socket_open_netlink(void) {
    lc_result r = lc_socket_open(LC_AF_NETLINK, LC_SOCK_RAW, 0 /* NETLINK_ROUTE */);
    TEST_REQUIRE_OK(r);
    int32_t fd = (int32_t)r.value;
    TEST_ASSERT(close_on_exec(fd));

    struct { uint16_t family, pad; uint32_t pid, groups; } kernel = { LC_AF_NETLINK, 0, 0, 0 };
    struct {
        uint32_t len; uint16_t type, flags; uint32_t seq, pid;   /* struct nlmsghdr */
        uint8_t  rtgen_family, pad[3];                           /* struct rtgenmsg */
    } request = { 20, 18 /* RTM_GETLINK */, 0x301 /* NLM_F_REQUEST | NLM_F_DUMP */, 1, 0, 0, {0} };
    lc_result s = lc_socket_send_to_sockaddr(fd, &request, sizeof request, 0, &kernel, sizeof kernel);
    TEST_ASSERT_OK(s);
    TEST_ASSERT_EQ(s.value, (int64_t)sizeof request);

    static uint8_t reply[32768];
    struct { uint16_t family, pad; uint32_t pid, groups; } sender;
    uint32_t sender_size = sizeof sender;
    lc_result g = lc_socket_receive_from_sockaddr(fd, reply, sizeof reply, 0, &sender, &sender_size);
    TEST_ASSERT_OK(g);
    TEST_ASSERT(g.value >= 16);
    uint16_t type;
    lc_bytes_copy(&type, reply + 4, sizeof type);
    TEST_ASSERT_EQ(type, (uint16_t)16);   /* RTM_NEWLINK: there is always lo */
    TEST_ASSERT_EQ(sender_size, (uint32_t)sizeof sender);
    TEST_ASSERT_EQ(sender.family, (uint16_t)LC_AF_NETLINK);
    TEST_ASSERT_EQ(sender.pid, (uint32_t)0);   /* the kernel */
    lc_socket_close(fd);
}

/* accept4's flags: LC_SOCK_NONBLOCK makes the accepted socket
 * non-blocking, and it is close-on-exec. */
static void test_socket_accept_flags_nonblocking(void) {
    uint16_t port = 0;
    int32_t listener = loopback_listener(&port);
    TEST_REQUIRE(listener >= 0);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, port);
    TEST_REQUIRE_OK(c);

    lc_socket_address peer;
    uint32_t peer_size = sizeof peer;
    lc_result a = lc_socket_accept_flags(listener, &peer, &peer_size, LC_SOCK_NONBLOCK);
    TEST_REQUIRE_OK(a);
    int32_t fd = (int32_t)a.value;
    TEST_ASSERT_EQ(peer_size, (uint32_t)sizeof peer);
    TEST_ASSERT_EQ(lc_network_to_host_32(peer.addr), (uint32_t)LC_INADDR_LOOPBACK);
    TEST_ASSERT((lc_syscall3(SYS_fcntl, fd, F_GETFL, 0) & O_NONBLOCK) != 0);
    TEST_ASSERT(close_on_exec(fd));
    char byte;
    TEST_ASSERT_EQ(lc_socket_receive(fd, &byte, 1).error, LC_ERR_AGAIN);   /* nothing sent yet */

    lc_socket_close(fd);
    lc_socket_close((int32_t)c.value);
    lc_socket_close(listener);
}

/* LC_SOCK_INHERIT: the socket is not close-on-exec, and the flag never
 * reaches the kernel, which would refuse a type it does not know. */
static void test_socket_inherit(void) {
    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM | LC_SOCK_INHERIT, 0);
    TEST_REQUIRE_OK(r);
    TEST_ASSERT(!close_on_exec((int32_t)r.value));
    lc_socket_close((int32_t)r.value);

    uint16_t port = 0;
    int32_t listener = loopback_listener(&port);
    TEST_REQUIRE(listener >= 0);
    lc_result c = lc_socket_connect_to(127, 0, 0, 1, port);
    TEST_REQUIRE_OK(c);
    lc_result a = lc_socket_accept_flags(listener, NULL, NULL, LC_SOCK_INHERIT);
    TEST_REQUIRE_OK(a);
    TEST_ASSERT(!close_on_exec((int32_t)a.value));
    lc_socket_close((int32_t)a.value);
    lc_socket_close((int32_t)c.value);
    lc_socket_close(listener);
}

/* SO_ERROR through lc_socket_get_option: a UDP socket connected to a port
 * nobody listens on gets ECONNREFUSED from the ICMP reply to its first
 * datagram, and reading the option clears it. */
static void test_socket_so_error_refused(void) {
    /* A free port: bound, then closed. */
    lc_result h = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    TEST_REQUIRE_OK(h);
    lc_socket_address any = lc_socket_address_create(127, 0, 0, 1, 0);
    TEST_REQUIRE_OK(lc_socket_bind((int32_t)h.value, &any));
    lc_socket_address bound;
    uint32_t size = sizeof bound;
    TEST_REQUIRE(lc_kernel_getsockname((int32_t)h.value, &bound, &size) == 0);
    lc_socket_close((int32_t)h.value);

    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_DGRAM, 0);
    TEST_REQUIRE_OK(r);
    int32_t fd = (int32_t)r.value;
    TEST_REQUIRE_OK(lc_socket_connect(fd, &bound));
    TEST_REQUIRE_OK(lc_socket_send(fd, "x", 1));

    int32_t error = 0;
    for (int i = 0; i < 100 && error == 0; i++) {
        uint32_t length = sizeof error;
        TEST_REQUIRE_OK(lc_socket_get_option(fd, LC_SOL_SOCKET, LC_SO_ERROR, &error, &length));
        TEST_ASSERT_EQ(length, (uint32_t)sizeof error);
        if (error == 0) lc_time_sleep_milliseconds(10);
    }
    TEST_ASSERT_EQ(error, LC_ERR_CONNREFUSED);
    uint32_t length = sizeof error;
    TEST_ASSERT_OK(lc_socket_get_option(fd, LC_SOL_SOCKET, LC_SO_ERROR, &error, &length));
    TEST_ASSERT_EQ(error, 0);   /* read once, cleared */

    int32_t one = 1;
    TEST_ASSERT_OK(lc_socket_set_option(fd, LC_SOL_SOCKET, LC_SO_REUSEADDR, &one, sizeof one));
    lc_socket_close(fd);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    /* address create */
    TEST_RUN(test_socket_address_create);

    /* address any */
    TEST_RUN(test_socket_address_any);

    /* byte order round-trip */
    TEST_RUN(test_byte_order);

    /* socket create/close */
    TEST_RUN(test_socket_create_close);

    /* TCP echo exchange */
    TEST_RUN(test_socket_byte_order);
    TEST_RUN(test_socket_tcp_exchange);

    /* server on a port that is already taken */
    TEST_RUN(test_socket_server_port_in_use);

    /* UDP echo exchange */
    TEST_RUN(test_socket_udp_exchange);

    /* the complete wrappers */
    TEST_RUN(test_socket_listen_on_an_address);
    TEST_RUN(test_socket_open_netlink);
    TEST_RUN(test_socket_accept_flags_nonblocking);
    TEST_RUN(test_socket_inherit);
    TEST_RUN(test_socket_so_error_refused);

    return test_main();
}

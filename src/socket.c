#include <lightc/socket.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include "kernel.h"          /* SOCK_CLOEXEC, MSG_NOSIGNAL */

_Static_assert(sizeof(lc_socket_address) == 16, "lc_socket_address must be 16 bytes (sockaddr_in)");

/* --- Address helpers --- */

lc_socket_address lc_socket_address_create(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port) {
    lc_socket_address addr;
    lc_bytes_fill(&addr, 0, sizeof(addr));
    addr.family = LC_AF_INET;
    addr.port   = lc_host_to_network_16(port);
    addr.addr   = lc_host_to_network_32(((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d);
    return addr;
}

lc_socket_address lc_socket_address_any(uint16_t port) {
    lc_socket_address addr;
    lc_bytes_fill(&addr, 0, sizeof(addr));
    addr.family = LC_AF_INET;
    addr.port   = lc_host_to_network_16(port);
    addr.addr   = lc_host_to_network_32(LC_INADDR_ANY);
    return addr;
}

/* --- Socket operations --- */

/* The kernel's flags for `flags`, the type flags of lc_socket_open or the
 * flags of lc_socket_accept_flags: SOCK_CLOEXEC unless LC_SOCK_INHERIT is
 * there, which goes no further. */
static int64_t kernel_socket_flags(int32_t flags) {
    if (flags & LC_SOCK_INHERIT) return flags & ~LC_SOCK_INHERIT;
    return flags | SOCK_CLOEXEC;
}

lc_result lc_socket_open(int32_t family, int32_t type, int32_t protocol) {
    return lc_result_from_sysret(lc_syscall3(SYS_socket, family, kernel_socket_flags(type), protocol));
}

lc_result lc_socket_create(int32_t type) {
    return lc_socket_open(LC_AF_INET, type, 0);
}

void lc_socket_close(int32_t socket_fd) {
    lc_kernel_close_file(socket_fd);
}

lc_result lc_socket_set_reuse_address(int32_t socket_fd) {
    int32_t val = 1;
    return lc_socket_set_option(socket_fd, LC_SOL_SOCKET, LC_SO_REUSEADDR, &val, sizeof(val));
}

lc_result lc_socket_bind(int32_t socket_fd, const lc_socket_address *addr) {
    return lc_result_from_sysret(lc_syscall3(SYS_bind, socket_fd, (int64_t)addr, sizeof(lc_socket_address)));
}

lc_result lc_socket_listen(int32_t socket_fd, int32_t backlog) {
    return lc_result_from_sysret(lc_syscall2(SYS_listen, socket_fd, backlog));
}

lc_result lc_socket_accept_flags(int32_t socket_fd, void *addr, uint32_t *addr_size, int32_t flags) {
    return lc_result_from_sysret(lc_syscall4(SYS_accept4, socket_fd, (int64_t)addr,
                                             (int64_t)addr_size, kernel_socket_flags(flags)));
}

lc_result lc_socket_accept(int32_t socket_fd, lc_socket_address *client_addr) {
    uint32_t addrlen = sizeof(lc_socket_address);
    return lc_socket_accept_flags(socket_fd, client_addr, client_addr ? &addrlen : NULL, 0);
}

lc_result lc_socket_connect(int32_t socket_fd, const lc_socket_address *addr) {
    return lc_result_from_sysret(lc_syscall3(SYS_connect, socket_fd, (int64_t)addr, sizeof(lc_socket_address)));
}

lc_result lc_socket_send(int32_t socket_fd, const void *buf, size_t count) {
    return lc_result_from_sysret(lc_syscall6(SYS_sendto, socket_fd, (int64_t)buf, (int64_t)count,
                       MSG_NOSIGNAL, 0, 0));
}

lc_result lc_socket_receive(int32_t socket_fd, void *buf, size_t count) {
    return lc_result_from_sysret(lc_syscall6(SYS_recvfrom, socket_fd, (int64_t)buf, (int64_t)count,
                       0, 0, 0));
}

lc_result lc_socket_send_to(int32_t socket_fd, const void *buf, size_t count,
                          const lc_socket_address *dest) {
    return lc_socket_send_to_sockaddr(socket_fd, buf, count, 0, dest, sizeof(lc_socket_address));
}

lc_result lc_socket_receive_from(int32_t socket_fd, void *buf, size_t count,
                               lc_socket_address *sender) {
    uint32_t addrlen = sizeof(lc_socket_address);
    return lc_socket_receive_from_sockaddr(socket_fd, buf, count, 0, sender, sender ? &addrlen : NULL);
}

lc_result lc_socket_send_to_sockaddr(int32_t socket_fd, const void *buf, size_t count,
                                     int32_t flags, const void *dest, uint32_t dest_size) {
    return lc_result_from_sysret(lc_syscall6(SYS_sendto, socket_fd, (int64_t)buf, (int64_t)count,
                                             flags | MSG_NOSIGNAL, (int64_t)dest, dest_size));
}

lc_result lc_socket_receive_from_sockaddr(int32_t socket_fd, void *buf, size_t count,
                                          int32_t flags, void *sender, uint32_t *sender_size) {
    return lc_result_from_sysret(lc_syscall6(SYS_recvfrom, socket_fd, (int64_t)buf, (int64_t)count,
                                             flags, (int64_t)sender, (int64_t)sender_size));
}

lc_result lc_socket_get_option(int32_t socket_fd, int32_t level, int32_t name,
                               void *value, uint32_t *length) {
    return lc_result_from_sysret(lc_syscall5(SYS_getsockopt, socket_fd, level, name,
                                             (int64_t)value, (int64_t)length));
}

lc_result lc_socket_set_option(int32_t socket_fd, int32_t level, int32_t name,
                               const void *value, uint32_t length) {
    return lc_result_from_sysret(lc_syscall5(SYS_setsockopt, socket_fd, level, name,
                                             (int64_t)value, length));
}

lc_result lc_socket_shutdown(int32_t socket_fd, int32_t how) {
    return lc_result_from_sysret(lc_syscall2(SYS_shutdown, socket_fd, how));
}

/* --- Convenience functions --- */

lc_result lc_socket_listen_on(const lc_socket_address *addr, int32_t backlog) {
    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(r)) return r;
    int32_t fd = (int32_t)r.value;

    r = lc_socket_set_reuse_address(fd);
    if (lc_is_err(r)) { lc_socket_close(fd); return r; }

    r = lc_socket_bind(fd, addr);
    if (lc_is_err(r)) { lc_socket_close(fd); return r; }

    r = lc_socket_listen(fd, backlog);
    if (lc_is_err(r)) { lc_socket_close(fd); return r; }

    return lc_ok(fd);
}

lc_result lc_socket_connect_to(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port) {
    lc_result r = lc_socket_open(LC_AF_INET, LC_SOCK_STREAM, 0);
    if (lc_is_err(r)) return r;
    int32_t fd = (int32_t)r.value;

    lc_socket_address addr = lc_socket_address_create(a, b, c, d, port);
    r = lc_socket_connect(fd, &addr);
    if (lc_is_err(r)) { lc_socket_close(fd); return r; }

    return lc_ok(fd);
}

/* ------------------------------------------------------------------ */
/* IPv6                                                               */
/* ------------------------------------------------------------------ */

_Static_assert(sizeof(lc_socket_address6) == 28, "lc_socket_address6 must be 28 bytes (sockaddr_in6)");

lc_result lc_socket_tcp6(void) {
    return lc_socket_open(LC_AF_INET6, LC_SOCK_STREAM, 0);
}

lc_result lc_socket_udp6(void) {
    return lc_socket_open(LC_AF_INET6, LC_SOCK_DGRAM, 0);
}

lc_socket_address6 lc_socket_address6_create(const uint8_t addr[16], uint16_t port) {
    lc_socket_address6 sa;
    lc_bytes_fill(&sa, 0, sizeof(sa));
    sa.family = LC_AF_INET6;
    sa.port   = lc_host_to_network_16(port);
    lc_bytes_copy(sa.addr, addr, 16);
    return sa;
}

lc_socket_address6 lc_socket_address6_loopback(uint16_t port) {
    uint8_t addr[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
    return lc_socket_address6_create(addr, port);
}

lc_socket_address6 lc_socket_address6_any(uint16_t port) {
    uint8_t addr[16] = {0};
    return lc_socket_address6_create(addr, port);
}

lc_result lc_socket_bind6(int32_t fd, const lc_socket_address6 *addr) {
    return lc_result_from_sysret(lc_syscall3(SYS_bind, fd, (int64_t)addr, sizeof(lc_socket_address6)));
}

lc_result lc_socket_listen6(int32_t fd, int32_t backlog) {
    return lc_result_from_sysret(lc_syscall2(SYS_listen, fd, backlog));
}

lc_result lc_socket_accept6(int32_t fd, lc_socket_address6 *client_addr) {
    uint32_t addrlen = sizeof(lc_socket_address6);
    return lc_socket_accept_flags(fd, client_addr, client_addr ? &addrlen : NULL, 0);
}

lc_result lc_socket_connect6(int32_t fd, const lc_socket_address6 *addr) {
    return lc_result_from_sysret(lc_syscall3(SYS_connect, fd, (int64_t)addr, sizeof(lc_socket_address6)));
}

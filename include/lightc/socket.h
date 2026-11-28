#ifndef LIGHTC_SOCKET_H
#define LIGHTC_SOCKET_H

#include "types.h"

/*
 * Socket networking — built on raw syscalls, no libc.
 *
 * TCP and UDP over IPv4 and IPv6 have helpers of their own, and
 * lc_socket_open opens a socket of any family, such as a netlink one.
 *
 * Every socket lightc opens or accepts is close-on-exec, so a program it
 * execs inherits none, unless the type or flags have LC_SOCK_INHERIT. Every
 * send passes MSG_NOSIGNAL, so a peer that has closed gives LC_ERR_PIPE
 * instead of a SIGPIPE that would end the process.
 */

/* Address families */
#define LC_AF_UNIX      1
#define LC_AF_INET      2
#define LC_AF_NETLINK   16

/* Socket types */
#define LC_SOCK_STREAM  1    /* TCP */
#define LC_SOCK_DGRAM   2    /* UDP */
#define LC_SOCK_RAW     3

/* Flags for the type of lc_socket_open and the flags of
 * lc_socket_accept_flags. LC_SOCK_NONBLOCK is the kernel's SOCK_NONBLOCK.
 * LC_SOCK_INHERIT is lightc's own and never reaches the kernel: the socket
 * is then not close-on-exec, for a program that hands it to one it execs. */
#define LC_SOCK_NONBLOCK 04000
#define LC_SOCK_INHERIT  0x40000000

/* Protocol */
#define LC_IPPROTO_TCP  6
#define LC_IPPROTO_UDP  17

/* Shutdown modes */
#define LC_SHUT_READ    0
#define LC_SHUT_WRITE   1
#define LC_SHUT_BOTH    2

/* Socket options */
#define LC_SOL_SOCKET       1
#define LC_SO_REUSEADDR     2
#define LC_SO_ERROR         4    /* the pending error, as an int32_t errno; reading it clears it */
#define LC_SO_REUSEPORT     15

/* Special addresses */
#define LC_INADDR_ANY       0x00000000
#define LC_INADDR_LOOPBACK  0x7F000001

/* IPv4 address (network byte order) */
typedef struct {
    uint16_t family;      /* LC_AF_INET */
    uint16_t port;        /* port in network byte order */
    uint32_t addr;        /* IPv4 address in network byte order */
    uint8_t  _pad[8];     /* padding to 16 bytes (sockaddr_in size) */
} lc_socket_address;

/* --- Byte order helpers ---
 *
 * Network byte order is big-endian. These convert between host and network
 * order correctly on either endianness: on a big-endian host they are the
 * identity, on a little-endian host they swap. (x86_64 and aarch64/Linux are
 * little-endian, so the generated code is a byte swap there.) */

/* A 16-bit value in network byte order. */
[[gnu::const]]
static inline uint16_t lc_host_to_network_16(uint16_t value) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return value;
#else
    return __builtin_bswap16(value);
#endif
}

/* A 32-bit value in network byte order. */
[[gnu::const]]
static inline uint32_t lc_host_to_network_32(uint32_t value) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return value;
#else
    return __builtin_bswap32(value);
#endif
}

/* A 16-bit value from network byte order. */
[[gnu::const]]
static inline uint16_t lc_network_to_host_16(uint16_t value) {
    return lc_host_to_network_16(value);
}

/* A 32-bit value from network byte order. */
[[gnu::const]]
static inline uint32_t lc_network_to_host_32(uint32_t value) {
    return lc_host_to_network_32(value);
}

/* --- Address helpers --- */

/* Create an address from IPv4 components and port.
 * Example: lc_socket_address_create(127, 0, 0, 1, 8080) */
lc_socket_address lc_socket_address_create(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);

/* Create an address for INADDR_ANY with the given port. */
lc_socket_address lc_socket_address_any(uint16_t port);

/* --- Socket operations --- */

/* Open a socket of any family, type and protocol. value = fd on success.
 * It is close-on-exec unless type has LC_SOCK_INHERIT, and non-blocking when
 * type has LC_SOCK_NONBLOCK. For example lc_socket_open(LC_AF_INET,
 * LC_SOCK_STREAM, 0) for TCP, or lc_socket_open(LC_AF_NETLINK, LC_SOCK_RAW,
 * 0) for rtnetlink. */
[[nodiscard]] lc_result lc_socket_open(int32_t family, int32_t type, int32_t protocol);

/* Create an IPv4 socket, close-on-exec. value = fd on success.
 * Deprecated: lc_socket_open(LC_AF_INET, type, 0) does the same. */
[[deprecated("use lc_socket_open(LC_AF_INET, type, 0)"), nodiscard]]
lc_result lc_socket_create(int32_t type);

/* Close a socket. */
void lc_socket_close(int32_t socket_fd);

/* Set socket option to reuse address. */
[[nodiscard]] lc_result lc_socket_set_reuse_address(int32_t socket_fd);

/* Bind socket to an address. */
[[nodiscard]] lc_result lc_socket_bind(int32_t socket_fd, const lc_socket_address *addr);

/* Start listening for connections (TCP). */
[[nodiscard]] lc_result lc_socket_listen(int32_t socket_fd, int32_t backlog);

/* Accept a connection, with accept4's flags. value = new fd on success.
 * The new socket is close-on-exec unless flags has LC_SOCK_INHERIT, and
 * non-blocking when flags has LC_SOCK_NONBLOCK. addr, of any family, and
 * addr_size may be NULL; else *addr_size is the room at addr on the way in
 * and the length of the peer's address on the way out. */
[[nodiscard]] lc_result lc_socket_accept_flags(int32_t socket_fd, void *addr, uint32_t *addr_size,
                                               int32_t flags);

/* Accept an IPv4 connection (TCP). value = new fd, close-on-exec; fills
 * client_addr if non-NULL. Deprecated: lc_socket_accept_flags does the same
 * with flags 0, and takes an address of any family. */
[[deprecated("use lc_socket_accept_flags(socket_fd, addr, &addr_size, 0)"), nodiscard]]
lc_result lc_socket_accept(int32_t socket_fd, lc_socket_address *client_addr);

/* Connect to a remote address (TCP client). */
[[nodiscard]] lc_result lc_socket_connect(int32_t socket_fd, const lc_socket_address *addr);

/* Send data on a connected socket. value = bytes sent. A peer that closed
 * gives LC_ERR_PIPE instead of a SIGPIPE. */
[[nodiscard]] lc_result lc_socket_send(int32_t socket_fd, const void *buf, size_t count);

/* Receive data from a connected socket. value = bytes received (0 on close). */
[[nodiscard]] lc_result lc_socket_receive(int32_t socket_fd, void *buf, size_t count);

/* Send data to a specific address (UDP). value = bytes sent. A connected
 * stream socket's peer that closed gives LC_ERR_PIPE instead of a SIGPIPE. */
[[nodiscard]] lc_result lc_socket_send_to(int32_t socket_fd, const void *buf, size_t count,
                          const lc_socket_address *dest);

/* Receive data and get sender's address (UDP). value = bytes received. */
[[nodiscard]] lc_result lc_socket_receive_from(int32_t socket_fd, void *buf, size_t count,
                               lc_socket_address *sender);

/* Send data to an address of any family, with sendto's flags. value = bytes
 * sent. MSG_NOSIGNAL is always added, so a peer that closed gives
 * LC_ERR_PIPE instead of a SIGPIPE. dest may be NULL, with dest_size 0, on
 * a connected socket. */
[[nodiscard]] lc_result lc_socket_send_to_sockaddr(int32_t socket_fd, const void *buf, size_t count,
                                                   int32_t flags, const void *dest, uint32_t dest_size);

/* Receive data and the sender's address of any family, with recvfrom's
 * flags. value = bytes received. sender and sender_size may be NULL; else
 * *sender_size is the room at sender on the way in and the length of the
 * sender's address on the way out, which is longer than the room when the
 * address did not fit. */
[[nodiscard]] lc_result lc_socket_receive_from_sockaddr(int32_t socket_fd, void *buf, size_t count,
                                                        int32_t flags, void *sender, uint32_t *sender_size);

/* Read a socket option. *length is the room at value on the way in and
 * the option's length on the way out. LC_SO_ERROR at LC_SOL_SOCKET reads,
 * and clears, the error of a non-blocking connect. */
[[nodiscard]] lc_result lc_socket_get_option(int32_t socket_fd, int32_t level, int32_t name,
                                             void *value, uint32_t *length);

/* Set a socket option to the `length` bytes at value. */
[[nodiscard]] lc_result lc_socket_set_option(int32_t socket_fd, int32_t level, int32_t name,
                                             const void *value, uint32_t length);

/* Shutdown part of a connection. */
[[nodiscard]] lc_result lc_socket_shutdown(int32_t socket_fd, int32_t how);

/* --- Convenience: TCP server --- */

/* Create a TCP socket, reuse its address, bind it to addr and listen, in
 * one call. value = socket fd. lc_socket_address_any(port) listens on every
 * interface, and lc_socket_address_create(127, 0, 0, 1, port) on loopback
 * only. */
[[nodiscard]] lc_result lc_socket_listen_on(const lc_socket_address *addr, int32_t backlog);

/* --- Convenience: TCP client --- */

/* Create and connect in one call. value = socket fd. */
[[nodiscard]] lc_result lc_socket_connect_to(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);

/* ------------------------------------------------------------------ */
/* IPv6                                                               */
/* ------------------------------------------------------------------ */

#define LC_AF_INET6 10

/* IPv6 address — matches struct sockaddr_in6 layout */
typedef struct {
    uint16_t family;     /* LC_AF_INET6 */
    uint16_t port;       /* network byte order */
    uint32_t flowinfo;
    uint8_t  addr[16];   /* 128-bit IPv6 address */
    uint32_t scope_id;
} lc_socket_address6;

/* Create an IPv6 TCP socket, close-on-exec. */
[[nodiscard]] lc_result lc_socket_tcp6(void);

/* Create an IPv6 UDP socket, close-on-exec. */
[[nodiscard]] lc_result lc_socket_udp6(void);

/* Create an IPv6 address from its 16 bytes and a port. */
lc_socket_address6 lc_socket_address6_create(const uint8_t addr[16], uint16_t port);

/* Create the loopback address, ::1, with a port. */
lc_socket_address6 lc_socket_address6_loopback(uint16_t port);

/* Create the any-address, ::, with a port. */
lc_socket_address6 lc_socket_address6_any(uint16_t port);

/* Bind an IPv6 socket to an address. */
[[nodiscard]] lc_result lc_socket_bind6(int32_t fd, const lc_socket_address6 *addr);

/* Start listening for connections on an IPv6 socket. */
[[nodiscard]] lc_result lc_socket_listen6(int32_t fd, int32_t backlog);

/* Accept a connection on an IPv6 socket. The new socket is close-on-exec. */
[[nodiscard]] lc_result lc_socket_accept6(int32_t fd, lc_socket_address6 *client_addr);

/* Connect an IPv6 socket to a remote address. */
[[nodiscard]] lc_result lc_socket_connect6(int32_t fd, const lc_socket_address6 *addr);

#endif /* LIGHTC_SOCKET_H */

/*
 * NET-sockaddr-endian — lc_socket_address_create must store the IPv4
 * address in network byte order on a big-endian target too.
 *
 * It built the address as a | b << 8 | c << 16 | d << 24, which has the
 * bytes a, b, c, d in that order in memory only on a little-endian target:
 * on a big-endian one, 127.0.0.1 became 1.0.0.127. It now builds the
 * host-order value and converts it with lc_host_to_network_32.
 *
 * x86_64 and aarch64 Linux are little-endian, where both give the same
 * bytes, and test_socket checks those. So this program compiles its own
 * copy of src/socket.c with __BYTE_ORDER__ set to big-endian, so that
 * lc_host_to_network_* are the identity, as in a big-endian build, and it
 * uses that copy instead of the library's. There an address's value is
 * what its bytes in memory are, read big-endian: 127.0.0.1 must be
 * 0x7F000001.
 */

#pragma push_macro("__BYTE_ORDER__")
#undef __BYTE_ORDER__
#define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
#include "../../src/socket.c"
#pragma pop_macro("__BYTE_ORDER__")

#include "test.h"

static void test_address_create_is_big_endian(void) {
    lc_socket_address loopback = lc_socket_address_create(127, 0, 0, 1, 8080);
    _test_print_text("  127.0.0.1 as a big-endian build stores it: ");
    lc_print_hex(TEST_FD, loopback.addr);
    _test_print_text("\n");
    TEST_ASSERT_EQ(loopback.addr, (uint32_t)0x7F000001);
    TEST_ASSERT_EQ(loopback.port, (uint16_t)8080);
    TEST_ASSERT_EQ(loopback.family, (uint16_t)LC_AF_INET);

    lc_socket_address other = lc_socket_address_create(192, 168, 1, 20, 0);
    TEST_ASSERT_EQ(other.addr, (uint32_t)0xC0A80114);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_address_create_is_big_endian);
    return test_main();
}

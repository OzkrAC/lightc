/*
 * IO-sysfs-readall — lc_file_read_all must read a /sys attribute whole.
 *
 * read_all took the size lseek(SEEK_END) reported, when it was not 0, for
 * the exact size of the file: it read that many bytes and failed with EIO
 * when fewer came. Every sysfs attribute reports a page, 4096 bytes or
 * more, whatever it holds, so read_all failed on each of them. It now
 * reads until EOF, and the size is only a hint.
 *
 * /sys/devices/system/cpu/online lists the CPUs online, such as "0-3\n",
 * and is there in every Linux with sysfs mounted. read_all must give what
 * plain reads of the file give.
 */

#include "test.h"
#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/syscall.h>

#define ATTRIBUTE "/sys/devices/system/cpu/online"

static void test_read_all_reads_a_sysfs_attribute(void) {
    lc_sysret fd = lc_kernel_open_file(ATTRIBUTE, O_RDONLY | O_CLOEXEC, 0);
    if (fd < 0) TEST_SKIP(ATTRIBUTE " cannot be opened: is /sys mounted?");
    lc_sysret reported = lc_kernel_seek_position((int32_t)fd, 0, SEEK_END);
    TEST_REQUIRE_EQ(lc_kernel_seek_position((int32_t)fd, 0, SEEK_SET), 0);
    static char expected[4096];
    lc_sysret length = lc_kernel_read_all((int32_t)fd, expected, sizeof(expected));
    lc_kernel_close_file((int32_t)fd);
    TEST_REQUIRE(length > 0);

    uint8_t *data = NULL;
    size_t size = 0;
    lc_result r = lc_file_read_all(ATTRIBUTE, &data, &size);

    _test_print_text("  lseek(SEEK_END) reports ");
    lc_print_signed(TEST_FD, reported);
    _test_print_text(", the file holds ");
    lc_print_signed(TEST_FD, length);
    _test_print_text(" bytes; read_all: value ");
    lc_print_signed(TEST_FD, r.value);
    _test_print_text(", error ");
    _test_print_text(lc_error_name(r.error));
    _test_print_text("\n");

    TEST_REQUIRE_OK(r);
    TEST_ASSERT_EQ(r.value, length);
    TEST_ASSERT_EQ(size, (size_t)length);
    TEST_REQUIRE_NOT_NULL(data);
    TEST_ASSERT_STR_EQ((const char *)data, size, expected, (size_t)length);
    lc_heap_free(data);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_read_all_reads_a_sysfs_attribute);
    return test_main();
}

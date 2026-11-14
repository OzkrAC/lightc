/*
 * FMT-print-double-oob — lc_print_double must print exactly the text
 * lc_format_add_double makes, however long it is.
 *
 * lc_print_double formatted into a 32-byte stack buffer, then wrote as many
 * bytes as lc_format_finish said the text needed. Doubles of 2^64 and up
 * print as their exact integer, so from about 1e24 (or -1e23) the text no
 * longer fit: up to 32 characters it printed a NUL in place of the last
 * one, and past that it also printed the stack memory after the buffer,
 * 276 bytes of it for 1e300.
 *
 * Each value is printed to a pipe and must come back as the text
 * lc_format_add_double makes in a buffer big enough for any double, of the
 * length given, so the check cannot pass on short text.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/print.h>
#include <lightc/syscall.h>

/* lc_print_double writes value to a pipe; this reads back what it wrote. */
static size_t print_to_pipe(double value, char *buf, size_t size) {
    int32_t fds[2];
    if (lc_kernel_create_pipe(fds, 0) < 0) return 0;
    lc_print_double(fds[1], value);
    lc_kernel_close_file(fds[1]);
    size_t total = 0;
    while (total < size) {
        lc_sysret n = lc_kernel_read_bytes(fds[0], buf + total, size - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    lc_kernel_close_file(fds[0]);
    return total;
}

static void check_print_double(double value, const char *name, size_t expected_len) {
    char want[512];
    lc_format fmt = lc_format_start(want, sizeof(want));
    lc_format_add_double(&fmt, value);
    size_t want_len = lc_format_finish(&fmt);
    TEST_REQUIRE_EQ(want_len, expected_len);

    char got[512];
    size_t got_len = print_to_pipe(value, got, sizeof(got));
    size_t same = 0, nuls = 0;
    while (same < got_len && same < want_len && got[same] == want[same]) same++;
    for (size_t i = 0; i < got_len; i++)
        if (got[i] == '\0') nuls++;
    if (same < got_len || same < want_len) {
        _test_print_text("  lc_print_double(");
        _test_print_text(name);
        _test_print_text(") printed ");
        lc_print_unsigned(TEST_FD, got_len);
        _test_print_text(" bytes, the first ");
        lc_print_unsigned(TEST_FD, same);
        _test_print_text(" right and ");
        lc_print_unsigned(TEST_FD, nuls);
        _test_print_text(" of them NUL\n");
    }
    TEST_ASSERT_EQ(got_len, want_len);
    TEST_ASSERT_STR_EQ(got, got_len, want, want_len);
}

static void test_print_double_prints_long_values_whole(void) {
    check_print_double(-1e24, "-1e24", 32);     /* '-' + 24 integer digits + ".000000" */
    check_print_double(1e25, "1e25", 33);       /* 26 integer digits + ".000000" */
    check_print_double(1e300, "1e300", 308);    /* 301 integer digits + ".000000" */
    check_print_double(-1e308, "-1e308", 317);  /* '-' + 309 integer digits + ".000000" */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_print_double_prints_long_values_whole);
    return test_main();
}

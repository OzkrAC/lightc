/*
 * FMT-zero-pad-sign — lc_format_add_signed_padded with pad '0' must put a
 * negative number's sign before the zeros, as printf's %06d does.
 *
 * It rendered "-42" and then padded the whole of it on the left, so -42 in
 * a field of 6 came out as "000-42". With pad '0' and a negative value the
 * sign now comes first, then the zeros, then the digits. Other pads still
 * go before the sign, as with printf's %6d.
 *
 * Each expected string is what glibc 2.41's printf("%0*lld") prints for
 * the width and value (printf("%*lld") for the space pad), copied here
 * from a hosted program.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/print.h>
#include <lightc/string.h>

static void check(int64_t value, uint32_t width, char pad, const char *want) {
    char got[64];
    lc_format fmt = lc_format_start(got, sizeof(got));
    lc_format_add_signed_padded(&fmt, value, width, pad);
    size_t got_len = lc_format_finish(&fmt);
    size_t want_len = lc_string_length(want);
    if (!lc_string_equal(got, got_len, want, want_len)) {
        _test_print_text("  ");
        lc_print_signed(TEST_FD, value);
        _test_print_text(" in ");
        lc_print_unsigned(TEST_FD, width);
        _test_print_text(pad == '0' ? " with '0'" : " with ' '");
        _test_print_text(" printed \"");
        _test_print_text(got);
        _test_print_text("\", printf prints \"");
        _test_print_text(want);
        _test_print_text("\"\n");
    }
    TEST_ASSERT_STR_EQ(got, got_len, want, want_len);
}

static void test_negative_zero_padded(void) {
    check(-42, 6, '0', "-00042");
    check(-42, 4, '0', "-042");         /* one wider than "-42" */
    check(-42, 3, '0', "-42");          /* as wide as "-42" */
    check(-42, 2, '0', "-42");          /* narrower */
    check(-42, 0, '0', "-42");
    check(-1, 5, '0', "-0001");
}

static void test_int64_extremes(void) {
    check(INT64_MIN, 0, '0', "-9223372036854775808");
    check(INT64_MIN, 20, '0', "-9223372036854775808");    /* as wide */
    check(INT64_MIN, 21, '0', "-09223372036854775808");
    check(INT64_MIN, 25, '0', "-000009223372036854775808");
    check(INT64_MAX, 19, '0', "9223372036854775807");
    check(INT64_MAX, 21, '0', "009223372036854775807");
}

/* What did not change: non-negative values, and a space pad. */
static void test_unchanged(void) {
    check(42, 6, '0', "000042");
    check(7, 3, '0', "007");
    check(0, 4, '0', "0000");
    check(-42, 6, ' ', "   -42");
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_negative_zero_padded);
    TEST_RUN(test_int64_extremes);
    TEST_RUN(test_unchanged);
    return test_main();
}

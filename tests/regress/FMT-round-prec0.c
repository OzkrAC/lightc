/*
 * FMT-round-prec0 — lc_format_add_double_precision with precision 0 must
 * print the integer nearest the value, and for a tie the even one, as
 * printf's %.0f does.
 *
 * It printed (uint64_t)(value + 0.5), and the sum is rounded before the
 * cast truncates it. 0.49999999999999994 + 0.5 rounds to 1.0, so the
 * largest double below one half printed "1". From 2^52 to 2^53 doubles are
 * one apart, so value + 0.5 is a tie that rounds to even, and every odd
 * integer there printed as the integer above it: 4503599627370497 as
 * 4503599627370498. A tie such as 2.5 printed 3 where printf prints 2.
 *
 * Each expected string is what glibc 2.41's printf("%.0f") prints for the
 * value, copied here from a hosted program. The literals that are not
 * exact are the value printed with %.17g, which reads back as that double.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/string.h>

static void check(double value, const char *text, const char *want) {
    char got[64];
    lc_format fmt = lc_format_start(got, sizeof(got));
    lc_format_add_double_precision(&fmt, value, 0);
    size_t got_len = lc_format_finish(&fmt);
    size_t want_len = lc_string_length(want);
    if (!lc_string_equal(got, got_len, want, want_len)) {
        _test_print_text("  ");
        _test_print_text(text);
        _test_print_text(" printed \"");
        _test_print_text(got);
        _test_print_text("\", printf's %.0f prints \"");
        _test_print_text(want);
        _test_print_text("\"\n");
    }
    TEST_ASSERT_STR_EQ(got, got_len, want, want_len);
}

#define CHECK(value, want) check(value, #value, want)

static void test_ties_round_to_even(void) {
    CHECK(0.5, "0");
    CHECK(1.5, "2");
    CHECK(2.5, "2");
    CHECK(3.5, "4");
    CHECK(-0.5, "-0");
    CHECK(-1.5, "-2");
    CHECK(-2.5, "-2");
    CHECK(-3.5, "-4");
    CHECK(1000000000000000.5, "1000000000000000");
    CHECK(1000000000000001.5, "1000000000000002");
    CHECK(4503599627370494.5, "4503599627370494");       /* 2^52 - 1.5 */
    CHECK(4503599627370495.5, "4503599627370496");       /* 2^52 - 0.5 */
    CHECK(-4503599627370495.5, "-4503599627370496");
}

/* The doubles just below and just above each half. */
static void test_either_side_of_a_half(void) {
    CHECK(0.49999999999999994, "0");
    CHECK(0.50000000000000011, "1");
    CHECK(1.4999999999999998, "1");
    CHECK(1.5000000000000002, "2");
    CHECK(2.4999999999999996, "2");
    CHECK(2.5000000000000004, "3");
    CHECK(-0.49999999999999994, "-0");
    CHECK(-0.50000000000000011, "-1");
    CHECK(0.99999999999999989, "1");
}

/* From 2^52 on every double is an integer and prints as itself. */
static void test_integers_near_2_52_and_2_53(void) {
    CHECK(4503599627370495.0, "4503599627370495");       /* 2^52 - 1 */
    CHECK(4503599627370496.0, "4503599627370496");       /* 2^52 */
    CHECK(4503599627370497.0, "4503599627370497");       /* 2^52 + 1 */
    CHECK(4503599627370499.0, "4503599627370499");       /* 2^52 + 3 */
    CHECK(6755399441055745.0, "6755399441055745");       /* 3 * 2^51 + 1 */
    CHECK(9007199254740991.0, "9007199254740991");       /* 2^53 - 1 */
    CHECK(9007199254740992.0, "9007199254740992");       /* 2^53 */
    CHECK(9007199254740994.0, "9007199254740994");       /* 2^53 + 2 */
    CHECK(-4503599627370497.0, "-4503599627370497");
    CHECK(-9007199254740991.0, "-9007199254740991");
}

/* Zeros, the smallest doubles, and either side of 2^64, where printing
 * switches to the exact-integer path. */
static void test_zeros_and_extremes(void) {
    CHECK(0.0, "0");
    CHECK(-0.0, "-0");
    CHECK(4.9406564584124654e-324, "0");
    CHECK(-4.9406564584124654e-324, "-0");
    CHECK(-0.25, "-0");
    CHECK(9223372036854775808.0, "9223372036854775808");      /* 2^63 */
    CHECK(18446744073709549568.0, "18446744073709549568");    /* below 2^64 */
    CHECK(-18446744073709549568.0, "-18446744073709549568");
    CHECK(18446744073709551616.0, "18446744073709551616");    /* 2^64 */
    CHECK(1e20, "100000000000000000000");
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_ties_round_to_even);
    TEST_RUN(test_either_side_of_a_half);
    TEST_RUN(test_integers_near_2_52_and_2_53);
    TEST_RUN(test_zeros_and_extremes);
    return test_main();
}

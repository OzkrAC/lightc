/*
 * TIME-neg-year — lc_time_format, lc_time_format_date and
 * lc_time_format_iso8601 must print a year before 0 as a minus sign and
 * four digits or more.
 *
 * All three cast the year to uint64_t and printed it with
 * lc_format_add_unsigned_padded, so year -1, the second before
 * 0000-01-01T00:00:00Z, printed as 18446744073709551615-12-31T23:59:59Z.
 * They now print it with lc_format_add_signed_padded, in a field of 4, or
 * of 5 with the sign: -0001-12-31T23:59:59Z.
 *
 * Each date's fields are what glibc 2.41's gmtime_r gives for the epoch
 * second, and each expected string is those fields printed by glibc with
 * "%0*d-%02d-%02dT%02d:%02d:%02dZ", the year's width 5 when it is below 0
 * and 4 otherwise. A hosted program printed them, and they are copied here.
 * For year -1 glibc's strftime "%05Y-%m-%dT%H:%M:%SZ" prints the same.
 */

#include "test.h"
#include <lightc/string.h>
#include <lightc/time.h>

typedef size_t (*formatter)(const lc_date_time *dt, char *buf, size_t buf_size);

static void check(formatter format, const char *name, const lc_date_time *dt,
                  const char *want) {
    char got[64];
    size_t got_len = format(dt, got, sizeof(got));
    size_t want_len = lc_string_length(want);
    if (!lc_string_equal(got, got_len, want, want_len)) {
        _test_print_text("  ");
        _test_print_text(name);
        _test_print_text(" printed \"");
        _test_print_text(got);
        _test_print_text("\", glibc's fields give \"");
        _test_print_text(want);
        _test_print_text("\"\n");
    }
    TEST_ASSERT_STR_EQ(got, got_len, want, want_len);
}

static void check_iso8601(int64_t epoch_seconds, const char *want) {
    lc_date_time dt = lc_time_from_unix(epoch_seconds);
    check(lc_time_format_iso8601, "lc_time_format_iso8601", &dt, want);
}

static void test_year_minus_1_in_every_formatter(void) {
    lc_date_time dt = lc_time_from_unix(-62167219201);
    check(lc_time_format_iso8601, "lc_time_format_iso8601", &dt, "-0001-12-31T23:59:59Z");
    check(lc_time_format, "lc_time_format", &dt, "-0001-12-31 23:59:59");
    check(lc_time_format_date, "lc_time_format_date", &dt, "-0001-12-31");

    dt = lc_time_from_unix(-62182839233);
    check(lc_time_format_iso8601, "lc_time_format_iso8601", &dt, "-0001-07-04T05:06:07Z");
    check(lc_time_format, "lc_time_format", &dt, "-0001-07-04 05:06:07");
    check(lc_time_format_date, "lc_time_format_date", &dt, "-0001-07-04");
}

static void test_years_far_before_0(void) {
    check_iso8601(-377705116800, "-9999-01-01T00:00:00Z");
    check_iso8601(-377705116801, "-10000-12-31T23:59:59Z");
    check_iso8601(-451737734400, "-12345-01-01T00:00:00Z");

    /* The formatters take any year an lc_date_time holds. */
    lc_date_time dt = { .year = INT32_MIN, .month = 1, .day = 1 };
    check(lc_time_format_date, "lc_time_format_date", &dt, "-2147483648-01-01");
}

/* What did not change: years from 0 on. */
static void test_years_from_0(void) {
    check_iso8601(-62167219200, "0000-01-01T00:00:00Z");
    check_iso8601(-60825840833, "0042-07-04T05:06:07Z");
    check_iso8601(-30625844033, "0999-07-04T05:06:07Z");
    check_iso8601(0, "1970-01-01T00:00:00Z");
    check_iso8601(1710592245, "2024-03-16T12:30:45Z");
    check_iso8601(253402300799, "9999-12-31T23:59:59Z");
    check_iso8601(253402300800, "10000-01-01T00:00:00Z");

    lc_date_time dt = lc_time_from_unix(-62167219200);
    check(lc_time_format, "lc_time_format", &dt, "0000-01-01 00:00:00");
    check(lc_time_format_date, "lc_time_format_date", &dt, "0000-01-01");
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_year_minus_1_in_every_formatter);
    TEST_RUN(test_years_far_before_0);
    TEST_RUN(test_years_from_0);
    return test_main();
}

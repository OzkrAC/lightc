/*
 * TIME-format-retlen — a caller must be able to tell how much of a
 * formatter's text its buffer holds.
 *
 * lc_time_format and its kin return lc_format_finish's length: that of the
 * whole text, which is more than the buffer holds when the buffer is too
 * small, as snprintf's return is. time.h called it the length written, so
 * a caller that wrote that many bytes read past the text, into whatever
 * followed the buffer. time.h and format.h now say it is the whole text's
 * length, and lc_format_length gives what a builder's buffer holds, the
 * length to write.
 *
 * 2026-03-15 21:30:00 formatted into 8 bytes must return 19, with the
 * buffer holding "2026-03" and a NUL; a builder that formats the same into
 * 8 bytes must report 7 from lc_format_length and 19 from
 * lc_format_finish. lc_format_length is the fix, so with the defect this
 * program does not build.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/string.h>
#include <lightc/time.h>

static const lc_date_time when = { 2026, 3, 15, 21, 30, 0, 0, 73 };

static void test_time_format_returns_the_whole_length(void) {
    char buf[8];
    lc_bytes_fill(buf, 'x', sizeof buf);
    size_t whole = lc_time_format(&when, buf, sizeof buf);
    TEST_ASSERT_EQ(whole, (size_t)19);
    TEST_ASSERT(whole >= sizeof buf);   /* says the text was cut */
    TEST_ASSERT_STR_EQ(buf, 7, "2026-03", 7);
    TEST_ASSERT_EQ(buf[7], '\0');

    char date[4], clock[4], iso[4];
    TEST_ASSERT_EQ(lc_time_format_date(&when, date, sizeof date), (size_t)10);
    TEST_ASSERT_EQ(lc_time_format_time(&when, clock, sizeof clock), (size_t)8);
    TEST_ASSERT_EQ(lc_time_format_iso8601(&when, iso, sizeof iso), (size_t)20);
    TEST_ASSERT_EQ(date[3], '\0');
    TEST_ASSERT_EQ(clock[3], '\0');
    TEST_ASSERT_EQ(iso[3], '\0');
}

static void test_format_length_is_what_the_buffer_holds(void) {
    char buf[8];
    lc_format fmt = lc_format_start(buf, sizeof buf);
    lc_format_add_text(&fmt, "2026-03-15 21:30:00");
    TEST_ASSERT_EQ(lc_format_finish(&fmt), (size_t)19);
    TEST_ASSERT_EQ(lc_format_length(&fmt), (size_t)7);
    TEST_ASSERT(lc_format_has_overflow(&fmt));
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_time_format_returns_the_whole_length);
    TEST_RUN(test_format_length_is_what_the_buffer_holds);
    return test_main();
}

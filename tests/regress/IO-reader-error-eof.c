/*
 * IO-reader-error-eof — lc_reader must not take EAGAIN for the end of the
 * stream, and must tell a read error from the end of file.
 *
 * reader_refill ended the stream for good on any read that returned 0 or
 * less. On a non-blocking fd with no data yet, read returns -EAGAIN, so the
 * first read that found no data ended the stream, and the data that came
 * later was never read. A read error, such as EISDIR or ECONNRESET, looked
 * like a clean end of file. Now EAGAIN leaves the stream open, with error
 * set to LC_ERR_AGAIN, and a read error ends it with error set to the
 * errno, which stays. The tests check the error field that lightc 0.2.0
 * added, so this does not build against an earlier lightc.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/syscall.h>

static void test_again_is_not_the_end(void) {
    int32_t fds[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(fds, O_NONBLOCK | O_CLOEXEC), 0);
    lc_reader r = lc_reader_create(fds[0], 64);
    TEST_REQUIRE_NOT_NULL(r.buffer);
    char line[64];

    /* No data yet: no line, but not the end either. */
    TEST_ASSERT_EQ(lc_reader_read_line(&r, line, sizeof(line)), -1);
    TEST_ASSERT_EQ(r.error, LC_ERR_AGAIN);
    TEST_ASSERT(!lc_reader_is_end(&r));

    /* The line that comes later is read. */
    TEST_REQUIRE_EQ(lc_kernel_write_all(fds[1], "hello world!\n", 13), 13);
    int64_t n = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(n, 12);
    TEST_ASSERT_STR_EQ(line, n > 0 ? (size_t)n : 0, "hello world!", 12);
    TEST_ASSERT_EQ(r.error, 0);

    /* EAGAIN again between two lines, and the next one still comes. */
    TEST_ASSERT_EQ(lc_reader_read_byte(&r), -1);
    TEST_ASSERT_EQ(r.error, LC_ERR_AGAIN);
    TEST_REQUIRE_EQ(lc_kernel_write_all(fds[1], "line2\n", 6), 6);
    n = lc_reader_read_line(&r, line, sizeof(line));
    TEST_ASSERT_EQ(n, 5);
    TEST_ASSERT_STR_EQ(line, n > 0 ? (size_t)n : 0, "line2", 5);

    /* The real end, once the write end is closed: error 0. */
    lc_kernel_close_file(fds[1]);
    TEST_ASSERT_EQ(lc_reader_read_line(&r, line, sizeof(line)), -1);
    TEST_ASSERT(lc_reader_is_end(&r));
    TEST_ASSERT_EQ(r.error, 0);

    lc_reader_destroy(&r);
    lc_kernel_close_file(fds[0]);
}

static void test_read_error_is_not_eof(void) {
    /* read on a directory fails with EISDIR. */
    lc_sysret fd = lc_kernel_open_file(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
    TEST_REQUIRE(fd >= 0);
    lc_reader r = lc_reader_create((int32_t)fd, 64);
    TEST_REQUIRE_NOT_NULL(r.buffer);

    TEST_ASSERT_EQ(lc_reader_read_byte(&r), -1);
    _test_print_text("  read_byte on a directory: error ");
    _test_print_text(lc_error_name(r.error));
    _test_print_text("\n");
    TEST_ASSERT_EQ(r.error, LC_ERR_ISDIR);
    TEST_ASSERT(lc_reader_is_end(&r));

    /* The error ended the stream, and it stays. */
    char buf[4];
    TEST_ASSERT_EQ(lc_reader_read_bytes(&r, buf, sizeof(buf)), (size_t)0);
    TEST_ASSERT_EQ(r.error, LC_ERR_ISDIR);

    lc_reader_destroy(&r);
    lc_kernel_close_file((int32_t)fd);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_again_is_not_the_end);
    TEST_RUN(test_read_error_is_not_eof);
    return test_main();
}

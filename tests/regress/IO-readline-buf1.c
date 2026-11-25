/*
 * IO-readline-buf1 — lc_reader_read_line with a buf_size of 1 must return
 * -1 at the end of the stream.
 *
 * A buf_size of 1 leaves room for the terminator only, so read_line never
 * ran its main loop, the one place it returned -1. It went on to skip the
 * rest of the line and returned 0, an empty line, also when no line was
 * left, so a loop that reads lines until -1 never ended. A buf_size of 1
 * still stores no byte, but read_line returns -1 once the stream has
 * ended. Each loop here stops after MAX_CALLS calls, so with the defect
 * the test fails instead of hanging.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/syscall.h>

#define MAX_CALLS 16

/* The number of lines read_line with buf_size 1 finds in `input` before it
 * returns -1: MAX_CALLS + 1 if it never does, and -1 if a call returned
 * another value than 0 and -1. */
static int32_t count_lines(const char *input, size_t length) {
    int32_t fds[2];
    if (lc_kernel_create_pipe(fds, O_CLOEXEC) < 0) return -1;
    lc_sysret written = lc_kernel_write_all(fds[1], input, length);
    lc_kernel_close_file(fds[1]);
    if (written != (lc_sysret)length) {
        lc_kernel_close_file(fds[0]);
        return -1;
    }

    lc_reader r = lc_reader_create(fds[0], 64);
    char buf[1];
    int32_t lines = 0;
    for (;;) {
        int64_t n = lc_reader_read_line(&r, buf, sizeof(buf));
        if (n == -1) break;
        if (n != 0 || buf[0] != '\0') { lines = -1; break; }
        if (++lines > MAX_CALLS) break;
    }
    lc_reader_destroy(&r);
    lc_kernel_close_file(fds[0]);
    return lines;
}

static void test_buf_size_1_ends(void) {
    TEST_ASSERT_EQ(count_lines("", 0), 0);
    TEST_ASSERT_EQ(count_lines("ab\n", 3), 1);
    TEST_ASSERT_EQ(count_lines("a\n\nbc", 5), 3);   /* "a", "", and "bc" at EOF */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_buf_size_1_ends);
    return test_main();
}

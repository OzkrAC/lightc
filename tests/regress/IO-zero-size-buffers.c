/*
 * IO-zero-size-buffers — a writer or a reader created with a buffer size of
 * 0 must still pass every byte on.
 *
 * lc_heap_allocate(0) succeeds, so lc_writer_create(fd, 0) gave a writer
 * with a buffer of capacity 0. put_string wrote only through the buffer,
 * so it returned at once and dropped its bytes, while put_byte worked:
 * put_line wrote only the newline. A writer whose allocation failed
 * dropped every put_string the same way. Both writers now write straight
 * to the fd. lc_reader_create(fd, 0) gave a reader whose first read asked
 * for 0 bytes, got 0 and took that for the end of the stream; a size of 0
 * now gives a 4096-byte buffer.
 *
 * The tests check what reaches the pipe, not what the puts return, so
 * this builds against lightc 0.1.x too, where the puts return nothing.
 */

#include "test.h"
#include <lightc/io.h>
#include <lightc/syscall.h>

#define EXPECTED "helloworld\n12345X"

/* Put the same text with each kind of put, through a writer of
 * `buffer_size` bytes, and check what comes out of the pipe. */
static void check_writer(size_t buffer_size) {
    int32_t fds[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(fds, O_CLOEXEC), 0);

    lc_writer w = lc_writer_create(fds[1], buffer_size);
    TEST_ASSERT_EQ(w.capacity, (size_t)0);
    lc_writer_put_string(&w, "hello", 5);
    lc_writer_put_line(&w, "world", 5);
    lc_writer_put_unsigned(&w, 12345);
    lc_writer_put_char(&w, 'X');
    (void)lc_writer_destroy(&w);
    lc_kernel_close_file(fds[1]);

    char buf[64];
    lc_sysret n = lc_kernel_read_all(fds[0], buf, sizeof(buf));
    lc_kernel_close_file(fds[0]);
    TEST_REQUIRE(n >= 0);
    _test_print_text("  the pipe got ");
    lc_print_signed(TEST_FD, n);
    _test_print_text(" of 17 bytes\n");
    TEST_ASSERT_STR_EQ(buf, (size_t)n, EXPECTED, sizeof(EXPECTED) - 1);
}

static void test_writer_of_size_0_writes_every_byte(void) {
    check_writer(0);
}

static void test_writer_without_buffer_writes_every_byte(void) {
    check_writer((size_t)-1);           /* SIZE_MAX: the allocation fails */
}

static void test_reader_of_size_0_reads(void) {
    int32_t fds[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(fds, O_CLOEXEC), 0);
    TEST_REQUIRE_EQ(lc_kernel_write_all(fds[1], "abc\n", 4), 4);
    lc_kernel_close_file(fds[1]);

    lc_reader r = lc_reader_create(fds[0], 0);
    TEST_ASSERT_EQ(r.capacity, (size_t)4096);
    char buf[8];
    size_t n = lc_reader_read_bytes(&r, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, (size_t)4);
    TEST_ASSERT_STR_EQ(buf, n, "abc\n", 4);
    TEST_ASSERT(lc_reader_is_end(&r));

    lc_reader_destroy(&r);
    lc_kernel_close_file(fds[0]);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_writer_of_size_0_writes_every_byte);
    TEST_RUN(test_writer_without_buffer_writes_every_byte);
    TEST_RUN(test_reader_of_size_0_reads);
    return test_main();
}

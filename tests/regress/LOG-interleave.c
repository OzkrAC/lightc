/*
 * LOG-interleave — lines that two processes log to one O_APPEND file must
 * not mix.
 *
 * A text line went out in three writes: the prefix, the message and the
 * newline. A lock kept the threads of one process apart, but nothing kept
 * another process that shares the log fd, a forked child or a worker
 * writing to the same file, from writing in between, so lines came out
 * cut up and run together. A line now goes out in one writev, and Linux
 * does not mix another write to an O_APPEND file into it.
 *
 * The parent and a forked child each log LINES lines to one O_APPEND file
 * at once. Then every line of the file must be a whole line of one of
 * them, and each must have LINES. The file is in build/regress, with the
 * process id in its name, so two runs at once do not share it.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/heap.h>
#include <lightc/io.h>
#include <lightc/log.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

#define LINES          20000
#define WAIT_MS        60000    /* for the child: a bound for a slow emulator */
#define PARENT_MESSAGE "a line from the parent"
#define CHILD_MESSAGE  "a line from the child"

/* The child's exit code when its lines are logged. */
#define CHILD_DONE 0

/* Sets line to "[INFO  tid:<tid>] <message>" and returns its length. */
static size_t expected_line(char *line, size_t size, int32_t tid, const char *message) {
    lc_format f = lc_format_start(line, size);
    lc_format_add_text(&f, "[INFO  tid:");
    lc_format_add_signed(&f, tid);
    lc_format_add_text(&f, "] ");
    lc_format_add_text(&f, message);
    return lc_format_finish(&f);
}

/* Prints the bytes of line, with each byte below space as '.'. */
static void print_line(const uint8_t *line, size_t length) {
    for (size_t i = 0; i < length; i++)
        lc_print_char(TEST_FD, line[i] < ' ' ? '.' : (char)line[i]);
}

static void test_two_processes_keep_lines_whole(void) {
    char path[64];
    lc_format f = lc_format_start(path, sizeof(path));
    lc_format_add_text(&f, "build/regress/LOG-interleave.");
    lc_format_add_signed(&f, lc_kernel_get_process_id());
    lc_format_add_text(&f, ".log");
    lc_format_finish(&f);

    lc_sysret fd = lc_kernel_open_file(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_CLOEXEC,
                                       0600);
    TEST_REQUIRE(fd >= 0);
    lc_log_set_output((int32_t)fd);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);

    lc_sysret pid = lc_kernel_fork();
    TEST_REQUIRE(pid >= 0);
    bool child = pid == 0;
    for (int32_t i = 0; i < LINES; i++)
        lc_log(LC_LOG_INFO, child ? CHILD_MESSAGE : PARENT_MESSAGE);
    if (child) lc_kernel_exit(CHILD_DONE);

    int32_t status = 0;
    lc_sysret reaped;
    int64_t start = lc_time_start_timer();
    while ((reaped = lc_kernel_wait_for_child((int32_t)pid, &status, WNOHANG)) == 0
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);
    if (reaped == 0) {
        _test_print_text("  the child still runs after 60 s; killing it\n");
        lc_kernel_send_signal((int32_t)pid, SIGKILL);
        lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    }
    lc_log_set_output(STDERR);
    lc_kernel_close_file((int32_t)fd);
    TEST_ASSERT_EQ(reaped, pid);
    TEST_ASSERT_EQ(status, CHILD_DONE);

    uint8_t *data = NULL;
    size_t size = 0;
    lc_result loaded = lc_file_read_all(path, &data, &size);
    (void)lc_kernel_unlinkat(AT_FDCWD, path, 0);
    TEST_REQUIRE_OK(loaded);

    /* The main thread's id is its process's id, so each process's lines
     * carry its pid. */
    char parent_line[96], child_line[96];
    size_t parent_length = expected_line(parent_line, sizeof(parent_line),
                                         lc_kernel_get_process_id(), PARENT_MESSAGE);
    size_t child_length = expected_line(child_line, sizeof(child_line),
                                        (int32_t)pid, CHILD_MESSAGE);

    int64_t from_parent = 0, from_child = 0, malformed = 0;
    size_t first_malformed = size;
    size_t at = 0;
    while (at < size) {
        size_t end = at;
        while (end < size && data[end] != '\n') end++;
        const char *line = (const char *)data + at;
        size_t length = end - at;
        if (end == size)    /* no newline: the last line was cut off */
            malformed++;
        else if (lc_string_equal(line, length, parent_line, parent_length))
            from_parent++;
        else if (lc_string_equal(line, length, child_line, child_length))
            from_child++;
        else
            malformed++;
        if (malformed == 1 && first_malformed == size) first_malformed = at;
        at = end + 1;
    }

    _test_print_text("  ");
    lc_print_signed(TEST_FD, from_parent);
    _test_print_text(" whole lines from the parent, ");
    lc_print_signed(TEST_FD, from_child);
    _test_print_text(" from the child, ");
    lc_print_signed(TEST_FD, malformed);
    _test_print_text(" malformed\n");
    if (malformed > 0) {
        size_t end = first_malformed;
        while (end < size && data[end] != '\n') end++;
        _test_print_text("  the first malformed line: ");
        print_line(data + first_malformed, end - first_malformed);
        lc_print_newline(TEST_FD);
    }
    lc_heap_free(data);
    TEST_ASSERT_EQ(malformed, 0);
    TEST_ASSERT_EQ(from_parent, LINES);
    TEST_ASSERT_EQ(from_child, LINES);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_two_processes_keep_lines_whole);
    return test_main();
}

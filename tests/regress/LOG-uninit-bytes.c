/*
 * LOG-uninit-bytes — lc_log_commit must write nothing for an entry that
 * lc_log_begin filtered, even if the level has been lowered since.
 *
 * lc_log_begin gives an entry below the minimum level a builder with no
 * buffer, which stores nothing but still counts the bytes added to it.
 * lc_log_commit checked the level again rather than what begin had done,
 * so when the level had been lowered in between, by another thread or a
 * handler that turns on debug logging, it wrote that many bytes of the
 * entry's buffer, which nothing had written: whatever the stack held
 * there before, such as another function's secrets. Commit now writes
 * nothing for an entry that begin filtered.
 *
 * The log goes to a non-blocking pipe, and before each entry the stack
 * under it is filled with MARKER, so that what a commit writes can be
 * read back and its bytes show where they came from.
 */

#include "test.h"
#include <lightc/format.h>
#include <lightc/log.h>
#include <lightc/string.h>
#include <lightc/syscall.h>

#define MARKER     "SECRET-KEY-"
#define STACK_FILL 4096
#define TEXT       "user=alice action=login ok"

static int32_t fds[2];

/* Leaves MARKER all over the stack that log_entry's frame takes next. */
[[gnu::noinline]] static void fill_stack(void) {
    volatile char junk[STACK_FILL];
    for (size_t i = 0; i < sizeof(junk); i++) junk[i] = MARKER[i % (sizeof(MARKER) - 1)];
    (void)junk[0];
}

/* Begins an entry at level, adds TEXT, sets the minimum level to
 * level_at_commit and commits the entry. */
[[gnu::noinline]] static void log_entry(lc_log_level level, lc_log_level level_at_commit) {
    lc_log_entry entry;
    lc_log_begin(&entry, level);
    lc_format_add_text(&entry.fmt, TEXT);
    lc_log_set_level(level_at_commit);
    lc_log_commit(&entry);
}

/* Reads what the pipe holds into buf; returns the byte count. */
static size_t read_log(char *buf, size_t size) {
    size_t total = 0;
    while (total < size) {
        lc_sysret n = lc_kernel_read_bytes(fds[0], buf + total, size - total);
        if (n == -LC_ERR_INTR) continue;
        if (n <= 0) break;
        total += (size_t)n;
    }
    return total;
}

/* Prints what a commit wrote, with each byte below space as '.'. */
static void print_written(const char *buf, size_t length) {
    _test_print_text("  the commit wrote ");
    lc_print_unsigned(TEST_FD, length);
    _test_print_text(" bytes: ");
    for (size_t i = 0; i < length; i++)
        lc_print_char(TEST_FD, (uint8_t)buf[i] < ' ' ? '.' : buf[i]);
    lc_print_newline(TEST_FD);
}

/* Sends the log to a new non-blocking pipe. */
static bool open_log(void) {
    if (lc_kernel_create_pipe(fds, O_NONBLOCK | O_CLOEXEC) != 0) return false;
    lc_log_set_output(fds[1]);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);
    return true;
}

static void close_log(void) {
    lc_log_set_output(STDERR);
    lc_log_set_level(LC_LOG_DEBUG);
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

/* The defect: the level is lowered between begin and commit. */
static void test_filtered_entry_writes_nothing_after_level_lowered(void) {
    TEST_REQUIRE(open_log());
    lc_log_set_level(LC_LOG_WARN);
    fill_stack();
    log_entry(LC_LOG_DEBUG, LC_LOG_DEBUG);

    char buf[2 * LC_LOG_BUFFER_SIZE];
    size_t n = read_log(buf, sizeof(buf));
    if (n > 0) print_written(buf, n);
    close_log();
    TEST_ASSERT_EQ(n, (size_t)0);
}

/* The same with the level left as it was. */
static void test_filtered_entry_writes_nothing(void) {
    TEST_REQUIRE(open_log());
    lc_log_set_level(LC_LOG_WARN);
    fill_stack();
    log_entry(LC_LOG_DEBUG, LC_LOG_WARN);

    char buf[2 * LC_LOG_BUFFER_SIZE];
    size_t n = read_log(buf, sizeof(buf));
    if (n > 0) print_written(buf, n);
    close_log();
    TEST_ASSERT_EQ(n, (size_t)0);
}

/* An entry begin let through is still written, whole. */
static void test_enabled_entry_is_written(void) {
    TEST_REQUIRE(open_log());
    lc_log_set_level(LC_LOG_DEBUG);
    fill_stack();
    log_entry(LC_LOG_WARN, LC_LOG_DEBUG);

    char buf[2 * LC_LOG_BUFFER_SIZE];
    size_t n = read_log(buf, sizeof(buf));
    close_log();

    char line[96];
    lc_format f = lc_format_start(line, sizeof(line));
    lc_format_add_text(&f, "[WARN  tid:");
    lc_format_add_signed(&f, lc_kernel_get_thread_id());
    lc_format_add_text(&f, "] " TEXT "\n");
    size_t length = lc_format_finish(&f);
    TEST_ASSERT_STR_EQ(buf, n, line, length);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_filtered_entry_writes_nothing_after_level_lowered);
    TEST_RUN(test_filtered_entry_writes_nothing);
    TEST_RUN(test_enabled_entry_is_written);
    return test_main();
}

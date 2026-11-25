/*
 * test_log.c — tests for lightc logging module.
 */

#include "test.h"
#include <lightc/log.h>
#include <lightc/signal.h>
#include <lightc/syscall.h>
#include <lightc/string.h>
#include <lightc/thread.h>

/* ===== Helper: create pipe and redirect log output ===== */

static void setup_log_pipe(int32_t fds[2]) {
    lc_sysret r = lc_kernel_create_pipe(fds, O_NONBLOCK);
    /* fds[0] = read end, fds[1] = write end */
    (void)r;
    lc_log_set_output(fds[1]);
}

static void teardown_log_pipe(int32_t fds[2]) {
    lc_log_set_output(2); /* restore stderr */
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);
}

/*
 * Read whatever is available from the pipe into buf.
 * Returns the number of bytes read.
 */
static size_t drain_pipe(int32_t read_fd, char *buf, size_t buf_size) {
    size_t total = 0;
    while (total < buf_size) {
        lc_sysret n = lc_kernel_read_bytes(read_fd, buf + total, buf_size - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    return total;
}

/* ===== lc_log_is_enabled / lc_log_set_level ===== */

static void test_log_set_level(void) {
    /* Set level to WARN */
    lc_log_set_level(LC_LOG_WARN);

    TEST_ASSERT(!lc_log_is_enabled(LC_LOG_DEBUG));
    TEST_ASSERT(!lc_log_is_enabled(LC_LOG_INFO));
    TEST_ASSERT(lc_log_is_enabled(LC_LOG_WARN));
    TEST_ASSERT(lc_log_is_enabled(LC_LOG_ERROR));

    /* Set level to DEBUG — everything enabled */
    lc_log_set_level(LC_LOG_DEBUG);

    TEST_ASSERT(lc_log_is_enabled(LC_LOG_DEBUG));
    TEST_ASSERT(lc_log_is_enabled(LC_LOG_INFO));
    TEST_ASSERT(lc_log_is_enabled(LC_LOG_WARN));
    TEST_ASSERT(lc_log_is_enabled(LC_LOG_ERROR));

    /* Set level to ERROR — only ERROR enabled */
    lc_log_set_level(LC_LOG_ERROR);

    TEST_ASSERT(!lc_log_is_enabled(LC_LOG_DEBUG));
    TEST_ASSERT(!lc_log_is_enabled(LC_LOG_INFO));
    TEST_ASSERT(!lc_log_is_enabled(LC_LOG_WARN));
    TEST_ASSERT(lc_log_is_enabled(LC_LOG_ERROR));

    /* Restore default */
    lc_log_set_level(LC_LOG_DEBUG);
}

/* ===== Level filtering: below-level messages produce no output ===== */

static void test_log_level_filtering(void) {
    int32_t fds[2];
    setup_log_pipe(fds);

    /* Set level to WARN */
    lc_log_set_level(LC_LOG_WARN);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);

    /* DEBUG and INFO should be suppressed */
    lc_log(LC_LOG_DEBUG, "debug message");
    lc_log(LC_LOG_INFO, "info message");

    char buf[512];
    size_t n = drain_pipe(fds[0], buf, sizeof(buf));
    TEST_ASSERT_EQ(n, (size_t)0);

    /* WARN and ERROR should produce output */
    lc_log(LC_LOG_WARN, "warn message");
    lc_log(LC_LOG_ERROR, "error message");

    n = drain_pipe(fds[0], buf, sizeof(buf));
    TEST_ASSERT(n > 0);

    /* Verify WARN output present */
    TEST_ASSERT(lc_string_contains(buf, n, "warn message", 12));

    /* Verify ERROR output present */
    TEST_ASSERT(lc_string_contains(buf, n, "error message", 13));

    /* Restore default */
    lc_log_set_level(LC_LOG_DEBUG);
    teardown_log_pipe(fds);
}

/* ===== Text format contains expected markers ===== */

static void test_log_text_format(void) {
    int32_t fds[2];
    setup_log_pipe(fds);

    lc_log_set_level(LC_LOG_DEBUG);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);

    lc_log(LC_LOG_INFO, "server started");

    char buf[512];
    size_t n = drain_pipe(fds[0], buf, sizeof(buf));
    TEST_ASSERT(n > 0);

    /* Should contain the level tag */
    TEST_ASSERT(lc_string_contains(buf, n, "[INFO", 5));

    /* Should contain the message text */
    TEST_ASSERT(lc_string_contains(buf, n, "server started", 14));

    /* Should contain tid: marker */
    TEST_ASSERT(lc_string_contains(buf, n, "tid:", 4));

    teardown_log_pipe(fds);
}

/* ===== JSON format contains expected fields ===== */

static void test_log_json_format(void) {
    int32_t fds[2];
    setup_log_pipe(fds);

    lc_log_set_level(LC_LOG_DEBUG);
    lc_log_set_format(LC_LOG_FORMAT_JSON);

    lc_log(LC_LOG_INFO, "json test msg");

    char buf[1024];
    size_t n = drain_pipe(fds[0], buf, sizeof(buf));
    TEST_ASSERT(n > 0);

    /* Should contain level field */
    TEST_ASSERT(lc_string_contains(buf, n, "\"level\":\"INFO\"", 14));

    /* Should contain the message field */
    TEST_ASSERT(lc_string_contains(buf, n, "\"msg\":\"json test msg\"", 21));

    /* Should contain ts field */
    TEST_ASSERT(lc_string_contains(buf, n, "\"ts\":", 5));

    /* Should contain tid field */
    TEST_ASSERT(lc_string_contains(buf, n, "\"tid\":", 6));

    /* Restore text format */
    lc_log_set_format(LC_LOG_FORMAT_TEXT);
    teardown_log_pipe(fds);
}

/* ===== A short write: the rest of the line follows, or the line stops ===== */

/* Longer than a pipe holds: 16 pages by default, 1 MiB with 64 KiB pages. */
#define LONG_MESSAGE (2u << 20)

static char    long_message[LONG_MESSAGE];
static char    long_output[LONG_MESSAGE + 64];
static size_t  long_output_length;
static int32_t long_pipe[2];
static bool    reader_blocked_signal;
static volatile int32_t short_write_signals;

static void short_write_handler(int signo) { (void)signo; short_write_signals++; }

static void fill_long_message(void) {
    for (size_t i = 0; i < LONG_MESSAGE; i++) long_message[i] = (char)('a' + i % 26);
}

/* Length of the "[INFO  tid:<tid>] " prefix that out starts with, or 0. */
static size_t info_prefix_length(const char *out, size_t length) {
    if (!lc_string_starts_with(out, length, "[INFO  tid:", 11)) return 0;
    for (size_t i = 11; i + 1 < length; i++)
        if (out[i] == ']' && out[i + 1] == ' ') return i + 2;
    return 0;
}

/*
 * Once it has read the first byte of the line, the main thread is in its
 * writev, which cannot finish while nothing reads: the line is longer than
 * the pipe holds. This thread blocks SIGUSR1, so the SIGUSR1 it then sends
 * goes to the main thread and cuts the writev short. It reads the rest
 * until the write end is closed.
 */
static int32_t read_long_line(void *arg) {
    (void)arg;
    reader_blocked_signal = lc_is_ok(lc_signal_block(SIGUSR1));
    lc_sysret n = lc_kernel_read_retry(long_pipe[0], long_output, 1);
    if (n != 1) return 1;
    long_output_length = 1;
    if (reader_blocked_signal)
        (void)lc_kernel_send_signal(lc_kernel_get_process_id(), SIGUSR1);
    for (;;) {
        n = lc_kernel_read_retry(long_pipe[0], long_output + long_output_length,
                                 sizeof(long_output) - long_output_length);
        if (n <= 0) return 0;
        long_output_length += (size_t)n;
    }
}

/* A signal cuts the writev of a line short: the rest must follow, and the
 * line come out whole. */
static void test_log_short_write_finishes_line(void) {
    fill_long_message();
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(long_pipe, O_CLOEXEC), 0);
    short_write_signals = 0;
    TEST_REQUIRE_OK(lc_signal_handle(SIGUSR1, short_write_handler));
    lc_log_set_level(LC_LOG_DEBUG);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);
    lc_log_set_output(long_pipe[1]);

    lc_thread reader;
    TEST_REQUIRE_OK(lc_thread_create(&reader, read_long_line, NULL));
    lc_log_string(LC_LOG_INFO, long_message, LONG_MESSAGE);
    lc_log_set_output(2);
    lc_kernel_close_file(long_pipe[1]);     /* the reader reads end of file */
    lc_thread_join(&reader);
    lc_kernel_close_file(long_pipe[0]);
    TEST_ASSERT_OK(lc_signal_reset(SIGUSR1));

    TEST_ASSERT(reader_blocked_signal);
    TEST_ASSERT_EQ(short_write_signals, 1);  /* the signal came during the writev */
    size_t prefix = info_prefix_length(long_output, long_output_length);
    TEST_ASSERT(prefix > 0);
    TEST_ASSERT_EQ(long_output_length, prefix + LONG_MESSAGE + 1);
    if (prefix > 0 && long_output_length == prefix + LONG_MESSAGE + 1) {
        TEST_ASSERT_EQ(lc_bytes_compare(long_output + prefix, long_message, LONG_MESSAGE), 0);
        TEST_ASSERT_EQ(long_output[long_output_length - 1], '\n');
    }
}

/* On a non-blocking pipe the writev stops once the pipe is full, and the
 * write of the rest fails at once. The line stops there, with no newline,
 * and the next line, which the pipe cannot take, is dropped. */
static void test_log_short_write_error_stops_line(void) {
    fill_long_message();
    int32_t fds[2];
    setup_log_pipe(fds);
    lc_log_set_level(LC_LOG_DEBUG);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);

    lc_log_string(LC_LOG_INFO, long_message, LONG_MESSAGE);
    lc_log(LC_LOG_INFO, "next");

    size_t n = drain_pipe(fds[0], long_output, sizeof(long_output));
    teardown_log_pipe(fds);

    size_t prefix = info_prefix_length(long_output, n);
    TEST_ASSERT(prefix > 0);
    TEST_ASSERT(n > prefix && n < prefix + LONG_MESSAGE);
    if (prefix > 0 && n > prefix && n < prefix + LONG_MESSAGE)  /* the message's start */
        TEST_ASSERT_EQ(lc_bytes_compare(long_output + prefix, long_message, n - prefix), 0);
}

/* ===== main ===== */

/* Control bytes (< 0x20) must be escaped as \u00XX so the JSON stays valid, and
 * the line must still end with a well-formed "}\n". */
static void test_log_json_control_escape(void) {
    int32_t fds[2];
    setup_log_pipe(fds);
    lc_log_set_level(LC_LOG_DEBUG);
    lc_log_set_format(LC_LOG_FORMAT_JSON);

    lc_log_string(LC_LOG_INFO, "a\x01\x1fz", 4);   /* 0x01 and 0x1f are control bytes */

    char buf[1024];
    size_t n = drain_pipe(fds[0], buf, sizeof(buf));
    TEST_ASSERT(n >= 2);
    TEST_ASSERT(lc_string_contains(buf, n, "\\u0001", 6));
    TEST_ASSERT(lc_string_contains(buf, n, "\\u001f", 6));
    TEST_ASSERT_EQ(buf[n - 1], '\n');              /* well-formed line ending */
    TEST_ASSERT_EQ(buf[n - 2], '}');

    lc_log_set_format(LC_LOG_FORMAT_TEXT);
    teardown_log_pipe(fds);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    /* lc_log_set_level / lc_log_is_enabled */
    TEST_RUN(test_log_set_level);

    /* level filtering */
    TEST_RUN(test_log_level_filtering);

    /* text format output */
    TEST_RUN(test_log_text_format);

    /* JSON format output */
    TEST_RUN(test_log_json_format);
    TEST_RUN(test_log_json_control_escape);

    /* a short write */
    TEST_RUN(test_log_short_write_finishes_line);
    TEST_RUN(test_log_short_write_error_stops_line);

    return test_main();
}

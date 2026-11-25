/*
 * LOG-spin-across-write — threads that log while the log fd blocks must
 * wait without using the CPU.
 *
 * Each log line took a spinlock and held it across its write syscalls.
 * When the fd stopped taking bytes, as a pipe does once it is full and
 * its reader stalls, the writer blocked in write() with the lock held, and
 * every other thread that logged spun on the lock at full speed until the
 * reader came back. There is no lock now: each thread blocks in its own
 * writev.
 *
 * The log goes to a pipe that is full before the test starts and that
 * nothing reads for WINDOW_MS. THREADS threads each log one line: the first
 * to write blocks, and the others log behind it. The process's CPU time
 * over the window, while the main thread sleeps, must stay under
 * MAX_CPU_MS_PER_S per second. With the defect each thread behind the
 * first spins, at about 1000 ms per second when it has a CPU to itself;
 * with no lock the window takes well under 1 ms. The process's CPU time
 * leaves out other processes on the machine, and the window starts
 * SETTLE_MS after every thread has started, so an emulator's start-up
 * work is not in it either. Then the pipe is drained, and each thread's
 * line must come out whole.
 */

#include "test.h"
#include <lightc/log.h>
#include <lightc/string.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

/* clock_gettime's clock of the CPU time of the calling process: every
 * thread's, user and system (<linux/time.h>, the same on both arches). */
#define CLOCK_PROCESS_CPUTIME_ID 2
/* ioctl that sets or clears O_NONBLOCK on an fd from an int: 0 clears it
 * (asm-generic/ioctls.h, the same on both arches). */
#define FIONBIO 0x5421

#define THREADS          3
#define SETTLE_MS        100     /* for the threads to reach their writes */
#define WINDOW_MS        500
#define MAX_CPU_MS_PER_S 50
#define WAIT_MS          5000    /* for the threads to finish once drained */
#define CHUNK            4096
#define MESSAGE          "a line behind a blocked writer"

static _Atomic(int32_t) started;
static _Atomic(int32_t) finished;

static int32_t log_one_line(void *arg) {
    (void)arg;
    atomic_fetch_add(&started, 1);
    lc_log(LC_LOG_INFO, MESSAGE);
    atomic_fetch_add(&finished, 1);
    return 0;
}

static int64_t process_cpu_ns(void) {
    lc_timespec now = { 0, 0 };
    if (lc_kernel_get_clock(CLOCK_PROCESS_CPUTIME_ID, &now) != 0) return -1;
    return now.seconds * 1000000000 + now.nanoseconds;
}

/* Writes CHUNK bytes at a time to the non-blocking fd until it would
 * block; returns the bytes written. A pipe write of at most PIPE_BUF bytes
 * goes in whole or not at all, so the pipe is then full. */
static size_t fill_pipe(int32_t fd) {
    static uint8_t chunk[CHUNK];
    lc_bytes_fill(chunk, 'x', sizeof(chunk));
    size_t total = 0;
    for (;;) {
        lc_sysret n = lc_kernel_write_bytes(fd, chunk, sizeof(chunk));
        if (n == -LC_ERR_INTR) continue;
        if (n <= 0) return total;
        total += (size_t)n;
    }
}

/* What the pipe gave after the prefill: the threads' lines. */
static char   out[THREADS * 128];
static size_t out_length;
static size_t prefill_left;         /* bytes of the prefill not read yet */
static bool   out_overflow;

/* Reads what the non-blocking fd holds now, and keeps what comes after
 * the prefill in out. False on an error or end of file. */
static bool drain(int32_t fd) {
    uint8_t chunk[CHUNK];
    for (;;) {
        lc_sysret n = lc_kernel_read_bytes(fd, chunk, sizeof(chunk));
        if (n == -LC_ERR_AGAIN) return true;
        if (n == -LC_ERR_INTR) continue;
        if (n <= 0) return false;
        size_t got = (size_t)n, skip = got < prefill_left ? got : prefill_left;
        prefill_left -= skip;
        for (size_t i = skip; i < got; i++) {
            if (out_length < sizeof(out)) out[out_length++] = (char)chunk[i];
            else out_overflow = true;
        }
    }
}

static void test_blocked_loggers_do_not_spin(void) {
    int32_t fds[2];
    TEST_REQUIRE_EQ(lc_kernel_create_pipe(fds, O_NONBLOCK | O_CLOEXEC), 0);
    prefill_left = fill_pipe(fds[1]);
    TEST_REQUIRE(prefill_left > 0);
    TEST_REQUIRE_EQ(lc_kernel_write_bytes(fds[1], "x", 1), -LC_ERR_AGAIN);
    /* The write end blocks from now on; the read end stays non-blocking. */
    int32_t off = 0;
    TEST_REQUIRE_EQ(lc_kernel_device_control(fds[1], FIONBIO, &off), 0);
    lc_log_set_output(fds[1]);
    lc_log_set_format(LC_LOG_FORMAT_TEXT);

    lc_thread threads[THREADS];
    for (int32_t i = 0; i < THREADS; i++)
        TEST_REQUIRE_OK(lc_thread_create(&threads[i], log_one_line, NULL));
    int64_t start = lc_time_start_timer();
    while (atomic_load(&started) < THREADS && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);
    TEST_REQUIRE_EQ(atomic_load(&started), THREADS);
    lc_time_sleep_milliseconds(SETTLE_MS);

    int64_t cpu_start = process_cpu_ns();
    int64_t window_start = lc_time_start_timer();
    lc_time_sleep_milliseconds(WINDOW_MS);
    int64_t cpu_end = process_cpu_ns();
    int64_t window_ns = lc_time_elapsed_nanoseconds(window_start);
    int32_t done_in_window = atomic_load(&finished);

    /* Drain the pipe until every thread has written its line. Each thread
     * counts itself finished once its whole line is in the pipe, so once
     * all have, one more drain takes the rest of their lines. */
    start = lc_time_start_timer();
    while (drain(fds[0]) && atomic_load(&finished) < THREADS
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);
    int32_t done = atomic_load(&finished);
    if (done == THREADS) {
        (void)drain(fds[0]);
        for (int32_t i = 0; i < THREADS; i++) lc_thread_join(&threads[i]);
    }
    lc_log_set_output(STDERR);

    TEST_REQUIRE(cpu_start >= 0 && cpu_end >= 0 && window_ns > 0);
    int64_t cpu_ms_per_s = (cpu_end - cpu_start) * 1000 / window_ns;
    _test_print_text("  ");
    lc_print_signed(TEST_FD, THREADS);
    _test_print_text(" threads blocked on the full pipe took ");
    lc_print_signed(TEST_FD, cpu_end - cpu_start);
    _test_print_text(" ns of CPU in ");
    lc_print_signed(TEST_FD, window_ns / 1000000);
    _test_print_text(" ms, ");
    lc_print_signed(TEST_FD, cpu_ms_per_s);
    _test_print_text(" ms per s\n");
    TEST_ASSERT(cpu_ms_per_s < MAX_CPU_MS_PER_S);
    TEST_ASSERT_EQ(done_in_window, 0);  /* the pipe really blocked them */

    if (done != THREADS) {
        _test_print_text("  the threads still had not written their lines 5 s after the"
                         " pipe was drained\n");
        /* They are blocked or spinning: leave them, the process exits. */
        TEST_ASSERT_EQ(done, THREADS);
        return;
    }
    lc_kernel_close_file(fds[0]);
    lc_kernel_close_file(fds[1]);

    /* Each line is "[INFO  tid:<tid>] MESSAGE". */
    int32_t lines = 0, whole = 0;
    size_t at = 0;
    while (at < out_length) {
        size_t end = at;
        while (end < out_length && out[end] != '\n') end++;
        if (end == out_length) break;
        lines++;
        if (lc_string_starts_with(out + at, end - at, "[INFO  tid:", 11) &&
            lc_string_ends_with(out + at, end - at, "] " MESSAGE, sizeof("] " MESSAGE) - 1))
            whole++;
        at = end + 1;
    }
    TEST_ASSERT(!out_overflow);
    TEST_ASSERT_EQ(at, out_length);     /* the output ends with a newline */
    TEST_ASSERT_EQ(lines, THREADS);
    TEST_ASSERT_EQ(whole, THREADS);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_blocked_loggers_do_not_spin);
    return test_main();
}

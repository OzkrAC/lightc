/*
 * LOG-fork-deadlock — a child forked while another thread logs must be
 * able to log.
 *
 * Each log line took a process-wide spinlock and held it across its write
 * syscalls, and fork copies memory as it is. So a child forked while
 * another thread of the parent was inside a log call started with the
 * lock taken and no thread to release it, and its first lc_log spun for
 * ever. A line now goes out with one syscall and no lock.
 *
 * A thread logs to /dev/null without a pause while the test forks
 * CHILDREN children, FORK_GAP_US apart, and each child logs a line and
 * exits 0. With the defect most of them hang. Each child must have exited
 * 0 within WAIT_MS of the last fork: they normally take a few ms, and a
 * child that hangs never exits, so the bound only has to be safe for a
 * slow emulator. A child still running then is killed. The test also
 * prints how many children still ran REPORT_MS after the last fork.
 */

#include "test.h"
#include <lightc/log.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define CHILDREN    40
#define FORK_GAP_US 200
#define REPORT_MS   500
#define WAIT_MS     5000

static _Atomic(int32_t) stop;
static _Atomic(int64_t) parent_lines;   /* lines the thread has logged */

static int32_t keep_logging(void *arg) {
    (void)arg;
    while (!atomic_load(&stop)) {
        lc_log(LC_LOG_INFO, "a line from the parent's logging thread");
        atomic_fetch_add(&parent_lines, 1);
    }
    return 0;
}

static void test_forked_child_can_log(void) {
    lc_sysret null_fd = lc_kernel_open_file("/dev/null", O_WRONLY | O_CLOEXEC, 0);
    TEST_REQUIRE(null_fd >= 0);
    lc_log_set_output((int32_t)null_fd);

    lc_thread thread;
    TEST_REQUIRE_OK(lc_thread_create(&thread, keep_logging, NULL));
    int64_t start = lc_time_start_timer();
    while (atomic_load(&parent_lines) == 0 && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);

    int32_t pids[CHILDREN];
    int32_t forked = 0;
    int64_t lines_before = atomic_load(&parent_lines);
    while (forked < CHILDREN) {
        lc_sysret pid = lc_kernel_fork();
        if (pid == 0) {
            lc_log(LC_LOG_INFO, "a line from a forked child");
            lc_kernel_exit(0);
        }
        if (pid < 0) break;
        pids[forked++] = (int32_t)pid;
        lc_time_sleep_microseconds(FORK_GAP_US);
    }
    int64_t lines_during = atomic_load(&parent_lines) - lines_before;

    /* Reap the children as they exit, until all have or WAIT_MS is over. */
    int32_t status[CHILDREN] = { 0 };
    bool reaped[CHILDREN] = { false };
    int32_t running = forked;
    int32_t running_at_report = -1;     /* still running REPORT_MS in */
    start = lc_time_start_timer();
    while (running > 0 && lc_time_elapsed_milliseconds(start) < WAIT_MS) {
        if (running_at_report < 0 && lc_time_elapsed_milliseconds(start) >= REPORT_MS)
            running_at_report = running;
        for (int32_t i = 0; i < forked; i++) {
            if (reaped[i]) continue;
            if (lc_kernel_wait_for_child(pids[i], &status[i], WNOHANG) == pids[i]) {
                reaped[i] = true;
                running--;
            }
        }
        if (running > 0) lc_time_sleep_milliseconds(1);
    }
    int64_t waited = lc_time_elapsed_milliseconds(start);
    if (running_at_report < 0) running_at_report = running;

    int32_t failed = 0;
    for (int32_t i = 0; i < forked; i++) {
        if (!reaped[i]) {
            lc_kernel_send_signal(pids[i], SIGKILL);
            lc_kernel_wait_for_child(pids[i], &status[i], 0);
        } else if (status[i] != 0) {
            failed++;
        }
    }

    atomic_store(&stop, 1);
    lc_thread_join(&thread);
    lc_log_set_output(STDERR);
    lc_kernel_close_file((int32_t)null_fd);

    _test_print_text("  the thread logged ");
    lc_print_signed(TEST_FD, lines_during);
    _test_print_text(" lines while the test forked\n  ");
    lc_print_signed(TEST_FD, running_at_report);
    _test_print_text(" of ");
    lc_print_signed(TEST_FD, forked);
    _test_print_text(" children still ran 500 ms after the last fork\n");
    if (running > 0) {
        _test_print_text("  ");
        lc_print_signed(TEST_FD, running);
        _test_print_text(" of ");
        lc_print_signed(TEST_FD, forked);
        _test_print_text(" children still ran 5 s after the last fork; killed them\n");
    } else {
        _test_print_text("  all ");
        lc_print_signed(TEST_FD, forked);
        _test_print_text(" children exited within ");
        lc_print_signed(TEST_FD, waited);
        _test_print_text(" ms of the last fork\n");
    }
    TEST_ASSERT_EQ(forked, CHILDREN);
    TEST_ASSERT_EQ(running, 0);         /* no child hung */
    TEST_ASSERT_EQ(failed, 0);          /* and each exited 0 */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_forked_child_can_log);
    return test_main();
}

/*
 * RT-main-exit — returning from main must end every thread.
 *
 * _start ended with SYS_exit when main returned. That ends only the calling
 * thread, so a process with another thread still running did not exit
 * until that thread ended, or ever if it never did. Returning from main
 * means exit(), which ends the whole process: exit_group.
 *
 * The test forks a child that re-executes this program (/proc/self/exe)
 * with CHILD_MODE as argv[1]. In that mode main starts a thread that sleeps
 * for an hour at a time, waits until it runs and returns CHILD_STATUS. The
 * child must exit with that status within 5 s. With the defect the sleeping
 * thread keeps it running: the test then kills it and fails.
 */

#include "test.h"
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

#define CHILD_MODE      "--return-with-a-sleeping-thread"
#define CHILD_STATUS    7
#define CHILD_NO_THREAD 99      /* the child could not start its thread */
#define CHILD_NO_EXEC   127     /* the forked child could not exec this program */
#define WAIT_MS         5000

static _Atomic(int32_t) worker_running;

static int32_t sleeping_worker(void *arg) {
    (void)arg;
    atomic_store(&worker_running, 1);
    lc_timespec hour = { .seconds = 3600, .nanoseconds = 0 };
    for (;;) lc_kernel_sleep(&hour);
    return 0;
}

/* main in CHILD_MODE: return while the worker sleeps. */
static int child_main(void) {
    lc_thread worker;
    if (lc_thread_create(&worker, sleeping_worker, NULL).error)
        return CHILD_NO_THREAD;
    int64_t start = lc_time_start_timer();
    while (atomic_load(&worker_running) == 0 && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);
    return atomic_load(&worker_running) ? CHILD_STATUS : CHILD_NO_THREAD;
}

static void test_main_return_ends_every_thread(void) {
    lc_sysret pid = lc_kernel_fork();
    TEST_REQUIRE(pid >= 0);
    if (pid == 0) {
        char *argv[] = { "/proc/self/exe", CHILD_MODE, NULL };
        char *envp[] = { NULL };
        lc_sysret ret = lc_kernel_execute("/proc/self/exe", argv, envp);
        _test_print_text("  execve(/proc/self/exe) = ");
        lc_print_signed(TEST_FD, ret);
        lc_print_newline(TEST_FD);
        lc_kernel_exit(CHILD_NO_EXEC);
    }

    int32_t status = 0;
    lc_sysret reaped;
    int64_t start = lc_time_start_timer();
    while ((reaped = lc_kernel_wait_for_child((int32_t)pid, &status, WNOHANG)) == 0
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(5);

    if (reaped == 0) {
        _test_print_text("  the child still runs 5 s after its main returned; killing it\n");
        lc_kernel_send_signal((int32_t)pid, SIGKILL);
        lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    }
    TEST_ASSERT_EQ(reaped, pid);                    /* the child ended within 5 s */
    TEST_ASSERT_EQ(status, CHILD_STATUS << 8);      /* with main's return value */
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    if (argc > 1 && lc_string_equal(argv[1], lc_string_length(argv[1]),
                                    CHILD_MODE, sizeof(CHILD_MODE) - 1))
        return child_main();

    test_init(argc, argv);
    TEST_RUN(test_main_return_ends_every_thread);
    return test_main();
}

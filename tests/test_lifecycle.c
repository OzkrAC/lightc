/*
 * test_lifecycle.c — tests for lightc atexit registration, lc_exit and
 * returning from main.
 *
 * lc_exit and returning from main end the process, so those tests run in
 * a child: a forked one for lc_exit, a re-executed one for main.
 */

#include "test.h"
#include <lightc/lifecycle.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <lightc/time.h>
#include <stdatomic.h>

/* ===== M3: lc_exit reentrancy guard =====
 *
 * lc_exit terminates the process, so we observe handler execution from a forked
 * child through a MAP_SHARED page, then reap the child and inspect the counts.
 *
 * The handler re-enters lc_exit exactly once. With the reentrancy guard the
 * re-entrant call terminates immediately, so the handler runs once (count == 1).
 * Without it, the re-entrant lc_exit re-runs the whole handler chain from the
 * top, so the handler runs twice (count == 2) — the double-close/double-free bug.
 */
static volatile int32_t *exit_counter;  /* [0] = executions, [1] = re-enter latch */

static void reentrant_exit_handler(void) {
    exit_counter[0]++;
    if (exit_counter[1] == 0) {
        exit_counter[1] = 1;
        lc_exit(0);  /* re-enter — noreturn */
    }
}

static void test_exit_reentrancy_guard(void) {
    void *page = lc_kernel_map_memory(NULL, 4096, PROT_READ | PROT_WRITE,
                                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    TEST_ASSERT(page != MAP_FAILED);
    exit_counter = (volatile int32_t *)page;
    exit_counter[0] = 0;
    exit_counter[1] = 0;

    lc_sysret pid = lc_kernel_fork();
    TEST_ASSERT(pid >= 0);
    if (pid == 0) {
        /* Child: register the re-entering handler and exit. lc_exit is noreturn,
         * so the child never falls back into the test harness. */
        (void)lc_atexit(reentrant_exit_handler);
        lc_exit(0);
    }

    /* Parent: reap the child, then check how many times the handler ran. */
    int32_t status = 0;
    lc_kernel_wait_for_child((int32_t)pid, &status, 0);

    TEST_ASSERT_EQ(exit_counter[0], 1);  /* handler ran exactly once */

    lc_kernel_unmap_memory(page, 4096);
}

/* ===== test_atexit_register ===== */

static void dummy_atexit_handler(void) {
    /* intentionally empty */
}

static void test_atexit_register(void) {
    TEST_ASSERT_OK(lc_atexit(dummy_atexit_handler));
}

/* ===== test_atexit_overflow ===== */

static void overflow_handler_0(void) {}
static void overflow_handler_1(void) {}

static void test_atexit_overflow(void) {
    /*
     * One handler was already registered in test_atexit_register above.
     * Fill remaining slots. We alternate between two different function
     * pointers to avoid any dedup the implementation might do.
     */
    int registered = 1; /* account for the one already registered */

    for (; registered < LC_MAX_ATEXIT_HANDLERS; registered++) {
        lc_atexit_func fn = (registered % 2 == 0)
            ? overflow_handler_0
            : overflow_handler_1;
        lc_result r = lc_atexit(fn);
        if (r.error != 0) {
            /* Registration failed before we expected — report it */
            TEST_ASSERT_OK(r);
            return;
        }
    }

    /* Table should now be full — next registration must fail */
    TEST_ASSERT_ERR(lc_atexit(dummy_atexit_handler));
}

/* ===== returning from main ends the whole process =====
 *
 * Returning from main must end every thread, as exit_group does. The test
 * re-executes this binary (/proc/self/exe) with MAIN_RETURN_MODE as argv[1].
 * In that mode main starts a worker that sleeps for a minute, waits until the
 * worker runs, and returns MAIN_RETURN_STATUS. The child must exit with that
 * status within 1 s. If _start ended only the main thread (SYS_exit), the
 * sleeping worker would keep the process alive: the parent then kills the
 * child after 1 s and the test fails.
 *
 * Under qemu-user without a binfmt handler for the guest arch, the host
 * kernel cannot exec the binary and execve fails with ENOEXEC: the child
 * prints SKIP: then. Natively, re-executing the running binary cannot fail
 * that way, and any other exec failure is a test failure.
 */
#define MAIN_RETURN_MODE      "--main-returns-with-sleeping-thread"
#define MAIN_RETURN_STATUS    7
#define MAIN_RETURN_SKIPPED   77    /* exec failed with ENOEXEC */
#define MAIN_RETURN_NO_THREAD 99    /* child could not start its worker */
#define MAIN_RETURN_NO_EXEC   127   /* forked child could not exec the suite */
#define ERRNO_NOEXEC          8     /* ENOEXEC: exec format error */

static _Atomic(int32_t) main_return_worker_running;

static int32_t main_return_sleeping_worker(void *arg) {
    (void)arg;
    atomic_store(&main_return_worker_running, 1);
    lc_timespec minute = { .seconds = 60, .nanoseconds = 0 };
    lc_kernel_sleep(&minute);
    return 0;
}

/* main in MAIN_RETURN_MODE: return while the worker sleeps. */
static int main_return_child(void) {
    lc_thread worker;
    if (lc_thread_create(&worker, main_return_sleeping_worker, NULL).error)
        return MAIN_RETURN_NO_THREAD;
    while (atomic_load(&main_return_worker_running) == 0)
        lc_time_sleep_milliseconds(1);
    return MAIN_RETURN_STATUS;
}

static void test_main_return_ends_process(void) {
    lc_sysret pid = lc_kernel_fork();
    TEST_ASSERT(pid >= 0);
    if (pid < 0) return;
    if (pid == 0) {
        char *argv[] = { "/proc/self/exe", MAIN_RETURN_MODE, NULL };
        char *envp[] = { NULL };
        lc_sysret ret = lc_kernel_execute("/proc/self/exe", argv, envp);
        if (ret == -ERRNO_NOEXEC) {
            _test_print_text("SKIP: cannot re-execute the suite binary, execve(/proc/self/exe) = -8 "
                             "(ENOEXEC: no binfmt handler for this arch?)\n");
            lc_kernel_exit(MAIN_RETURN_SKIPPED);
        }
        _test_print_text("  execve(/proc/self/exe) = ");
        lc_print_signed(TEST_FD, ret);
        lc_print_newline(TEST_FD);
        lc_kernel_exit(MAIN_RETURN_NO_EXEC);
    }

    int32_t status = 0;
    lc_sysret reaped;
    int64_t start = lc_time_start_timer();
    while ((reaped = lc_kernel_wait_for_child((int32_t)pid, &status, WNOHANG)) == 0
           && lc_time_elapsed_milliseconds(start) < 1000)
        lc_time_sleep_milliseconds(5);

    if (reaped == 0) {
        _test_print_text("  child still running 1 s after main returned; killing it\n");
        lc_kernel_send_signal((int32_t)pid, SIGKILL);
        lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    } else if (status == (MAIN_RETURN_SKIPPED << 8)) {
        return;
    } else if (status != (MAIN_RETURN_STATUS << 8)) {
        _test_print_text("  child wait status = ");
        lc_print_signed(TEST_FD, status);
        lc_print_newline(TEST_FD);
    }

    TEST_ASSERT_EQ(reaped, pid);                          /* exited within 1 s */
    TEST_ASSERT_EQ(status, MAIN_RETURN_STATUS << 8);      /* with main's value */
}

/* ===== main ===== */

int main(int argc, char **argv, char **envp) {
    (void)envp;

    if (argc > 1 && lc_string_equal(argv[1], lc_string_length(argv[1]),
                                    MAIN_RETURN_MODE, sizeof(MAIN_RETURN_MODE) - 1))
        return main_return_child();

    test_init(argc, argv);

    /* Run first, while the atexit table still has room for the child to register
     * its handler (test_atexit_overflow fills the table afterwards). */
    TEST_RUN(test_exit_reentrancy_guard);

    TEST_RUN(test_atexit_register);
    TEST_RUN(test_atexit_overflow);
    TEST_RUN(test_main_return_ends_process);

    return test_main();
}

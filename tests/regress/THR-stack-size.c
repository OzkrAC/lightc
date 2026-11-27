/*
 * THR-stack-size — lc_thread_create_with_stack must refuse a stack it
 * cannot run a thread on.
 *
 * It took any size: 0 or 8 gave a mapping that the guard page filled, so
 * the new thread's first push faulted and killed the process, and a size
 * near SIZE_MAX wrapped, with the guard page added, to a small one that did
 * the same. It now fails with LC_ERR_INVAL for a size below
 * LC_THREAD_MIN_STACK_SIZE or one whose total with the guard page does not
 * fit in a size_t, with the handle all zeros, and rounds the total up to a
 * whole number of pages.
 *
 * Each refused size is tried in a child, under a 3 s watchdog, as with
 * the defect the thread takes the process down. A size of 20001, which is
 * not a whole number of pages, must run a thread.
 */

#include "test.h"
#include <lightc/memory.h>
#include <lightc/syscall.h>
#include <lightc/thread.h>
#include <stdatomic.h>

#define ITIMER_REAL 0

static size_t size_under_test;
static volatile int32_t ran;

static int32_t mark_ran(void *arg) {
    (void)arg;
    ran = 1;
    return 0;
}

static int32_t create_with_size(void) {
    lc_thread t;
    lc_result r = lc_thread_create_with_stack(&t, mark_ran, NULL, size_under_test);
    if (lc_is_ok(r)) {
        lc_thread_join(&t);
        return 20;   /* it should have refused */
    }
    if (r.error != LC_ERR_INVAL) return 21;
    if (t.tid != 0 || t.stack_base != NULL || t.stack_size != 0 || atomic_load(&t.alive) != 0)
        return 22;
    return 0;
}

/* The exit status of a child that tries `size`. */
static int32_t try_in_child(size_t size) {
    size_under_test = size;
    lc_sysret pid = lc_kernel_fork();
    if (pid == 0) {
        int64_t watchdog[4] = { 0, 0, 3, 0 };   /* struct itimerval: 3 s, once */
        (void)lc_syscall3(SYS_setitimer, ITIMER_REAL, (int64_t)watchdog, 0);
        lc_kernel_exit(create_with_size());
    }
    if (pid < 0) return -1;
    int32_t status = -1;
    (void)lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    return status;
}

static void test_sizes_too_small_or_too_large_are_refused(void) {
    const size_t sizes[] = { 0, 8, LC_THREAD_MIN_STACK_SIZE - 1, SIZE_MAX - 100, SIZE_MAX };
    for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        int32_t status = try_in_child(sizes[i]);
        if (status != 0) {
            _test_print_text("  stack size ");
            lc_print_unsigned(TEST_FD, sizes[i]);
            _test_print_text(": child status ");
            lc_print_signed(TEST_FD, status);
            _test_print_text("\n");
        }
        TEST_ASSERT_EQ(status, 0);
    }
}

static void test_odd_size_runs(void) {
    ran = 0;
    lc_thread t;
    lc_result r = lc_thread_create_with_stack(&t, mark_ran, NULL, 20001);
    TEST_REQUIRE(lc_is_ok(r));
    lc_thread_join(&t);
    TEST_ASSERT_EQ(ran, 1);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_sizes_too_small_or_too_large_are_refused);
    TEST_RUN(test_odd_size_runs);
    return test_main();
}

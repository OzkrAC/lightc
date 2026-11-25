/*
 * CORO-sched-tls — lc_scheduler_run must leave the thread's current
 * scheduler as it found it.
 *
 * lc_scheduler_run made its scheduler the thread's current one, which
 * lc_coroutine_yield switches through, and never set it back, and
 * lc_scheduler_destroy did not clear it either. So after a run:
 *
 *   - a yield outside any coroutine, documented as a no-op, found the
 *     finished scheduler and switched to its main_context, into the frame
 *     of the lc_scheduler_run that had already returned;
 *   - when a helper had made, run and destroyed a scheduler in its own
 *     frame, that yield read whatever had since reused the frame;
 *   - a coroutine that ran a scheduler of its own left the inner one
 *     current, so its next yield went to the finished inner scheduler
 *     instead of to the coroutine's siblings.
 *
 * lc_scheduler_run now makes the scheduler that was current when it was
 * called current again whenever it returns, and lc_scheduler_destroy
 * clears the current scheduler when it destroys it.
 *
 * With the defect these crash, hang or return into a frame that is gone,
 * so each case runs in a child process, which must exit 0 within 5 s.
 */

#include "test.h"
#include <lightc/coroutine.h>
#include <lightc/syscall.h>
#include <lightc/time.h>

#define WAIT_MS 5000

/* The child's exit codes when a check fails. */
#define CHILD_NO_COROUTINE   10  /* lc_coroutine_create failed */
#define CHILD_YIELD_SKIPPED  11  /* the code after the yield did not run */
#define CHILD_RUN_REENTERED  12  /* lc_scheduler_run returned a second time */
#define CHILD_WRONG_ORDER    13  /* the coroutines ran in the wrong order */

/* wait4's status: the signal that killed the child in bits 0 to 6, or 0
 * and the child's exit code in bits 8 to 15. */
#define STATUS_SIGNAL(status) ((status) & 0x7f)
#define STATUS_EXIT(status)   (((status) >> 8) & 0xff)

/* Runs scenario in a child process and checks that it exits 0. */
static void expect_child_exits_0(int32_t (*scenario)(void)) {
    lc_sysret pid = lc_kernel_fork();
    TEST_REQUIRE(pid >= 0);
    if (pid == 0) lc_kernel_exit(scenario());

    int32_t status = 0;
    lc_sysret reaped;
    int64_t start = lc_time_start_timer();
    while ((reaped = lc_kernel_wait_for_child((int32_t)pid, &status, WNOHANG)) == 0
           && lc_time_elapsed_milliseconds(start) < WAIT_MS)
        lc_time_sleep_milliseconds(1);

    if (reaped == 0) {
        _test_print_text("  the child still runs after 5 s; killing it\n");
        lc_kernel_send_signal((int32_t)pid, SIGKILL);
        lc_kernel_wait_for_child((int32_t)pid, &status, 0);
    } else if (STATUS_SIGNAL(status) != 0) {
        _test_print_text("  the child was killed by signal ");
        lc_print_signed(TEST_FD, STATUS_SIGNAL(status));
        lc_print_newline(TEST_FD);
    } else if (status != 0) {
        _test_print_text("  the child exited with ");
        lc_print_signed(TEST_FD, STATUS_EXIT(status));
        lc_print_newline(TEST_FD);
    }
    TEST_ASSERT_EQ(reaped, pid);    /* the child ended within 5 s */
    TEST_ASSERT_EQ(status, 0);      /* by exiting 0 */
}

static void do_nothing(void *arg) {
    (void)arg;
}

static volatile int32_t yield_returned;

/* Yields from plain code, in a frame of its own, as a program would long
 * after a run. */
[[gnu::noinline]] static void yield_outside_any_coroutine(void) {
    lc_coroutine_yield();
    yield_returned = 1;
}

/* --- A yield after a run --- */

static lc_scheduler kept;               /* run, and not destroyed yet */
static volatile int32_t runs_returned;

static int32_t yield_after_run(void) {
    kept = lc_scheduler_create_with_capacity(1);
    if (lc_coroutine_create(&kept, do_nothing, NULL) == NULL) return CHILD_NO_COROUTINE;
    lc_scheduler_run(&kept);
    runs_returned = runs_returned + 1;
    if (runs_returned != 1) return CHILD_RUN_REENTERED;
    yield_outside_any_coroutine();
    if (!yield_returned) return CHILD_YIELD_SKIPPED;
    lc_scheduler_destroy(&kept);
    return 0;
}

static void test_yield_after_run_is_a_no_op(void) {
    expect_child_exits_0(yield_after_run);
}

/* --- A yield after a helper ran and destroyed a scheduler of its own --- */

/* run_a_batch runs GAP bytes further down the stack than the calls of the
 * yield below reach, so the junk reuse_the_stack leaves in its old frame is
 * still there when that yield reads the scheduler the frame held. */
#define GAP 4096

[[gnu::noinline]] static int32_t run_a_batch(void) {
    lc_scheduler batch = lc_scheduler_create_with_capacity(1);
    if (lc_coroutine_create(&batch, do_nothing, NULL) == NULL) return CHILD_NO_COROUTINE;
    lc_scheduler_run(&batch);
    lc_scheduler_destroy(&batch);
    return 0;
}

[[gnu::noinline]] static int32_t run_a_batch_further_down(void) {
    volatile uint8_t gap[GAP];
    gap[0] = 0;
    int32_t failed = run_a_batch();
    (void)gap[0];
    return failed;
}

/* Writes over the stack that run_a_batch's frame took, as later calls do. */
[[gnu::noinline]] static void reuse_the_stack(void) {
    volatile uint8_t junk[4 * GAP];
    for (size_t i = 0; i < sizeof(junk); i++) junk[i] = 0x41;
    (void)junk[0];
}

static int32_t yield_after_a_helper_scheduler(void) {
    int32_t failed = run_a_batch_further_down();
    if (failed) return failed;
    reuse_the_stack();
    yield_outside_any_coroutine();
    return yield_returned ? 0 : CHILD_YIELD_SKIPPED;
}

static void test_yield_after_a_destroyed_scheduler_is_a_no_op(void) {
    expect_child_exits_0(yield_after_a_helper_scheduler);
}

/* --- A coroutine that runs a scheduler of its own --- */

#define STEPS 6
static const int32_t expected_order[STEPS] = { 1, 100, 2, 10, 3, 11 };
static int32_t order[STEPS];
static uint32_t steps;
static bool inner_failed;

static void step(int32_t id) {
    if (steps < STEPS) order[steps] = id;
    steps++;
}

static void inner_body(void *arg) {
    (void)arg;
    step(100);
}

/* Runs an inner scheduler, then yields, which must go to outer_b. */
static void outer_a(void *arg) {
    (void)arg;
    step(1);
    lc_scheduler inner = lc_scheduler_create_with_capacity(1);
    if (lc_coroutine_create(&inner, inner_body, NULL) == NULL) inner_failed = true;
    lc_scheduler_run(&inner);
    lc_scheduler_destroy(&inner);
    step(2);
    lc_coroutine_yield();
    step(3);
}

static void outer_b(void *arg) {
    (void)arg;
    step(10);
    lc_coroutine_yield();
    step(11);
}

static int32_t nested_run(void) {
    lc_scheduler outer = lc_scheduler_create_with_capacity(2);
    if (lc_coroutine_create(&outer, outer_a, NULL) == NULL
        || lc_coroutine_create(&outer, outer_b, NULL) == NULL)
        return CHILD_NO_COROUTINE;
    lc_scheduler_run(&outer);
    lc_scheduler_destroy(&outer);
    if (inner_failed) return CHILD_NO_COROUTINE;

    bool same = steps == STEPS;
    for (uint32_t i = 0; i < STEPS && i < steps; i++) same = same && order[i] == expected_order[i];
    if (same) return 0;
    _test_print_text("  the steps ran in the order");
    for (uint32_t i = 0; i < STEPS && i < steps; i++) {
        _test_print_text(" ");
        lc_print_signed(TEST_FD, order[i]);
    }
    _test_print_text(", not 1 100 2 10 3 11\n");
    return CHILD_WRONG_ORDER;
}

static void test_nested_run_resumes_the_outer_scheduler(void) {
    expect_child_exits_0(nested_run);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_yield_after_run_is_a_no_op);
    TEST_RUN(test_yield_after_a_destroyed_scheduler_is_a_no_op);
    TEST_RUN(test_nested_run_resumes_the_outer_scheduler);
    return test_main();
}

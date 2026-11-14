/*
 * test_harness.c — tests for the test harness itself (test.h).
 *
 * Most tests run a few demo tests in a forked child whose stdout is a pipe,
 * then check what the child printed and how it exited. The demos fail and
 * skip on purpose, so their output is checked here and not passed through:
 * while every check holds, this suite prints no FAIL: or SKIP: line, on any
 * machine or under qemu-user with a binfmt handler for the guest arch. After a
 * failed check, the test prints the child's output with each line behind
 * "  | ".
 */

#include "test.h"
#include <lightc/syscall.h>

/* ===== Running demos in a child ===== */

#define DEMO_OUTPUT_MAX 8192

typedef struct {
    char    text[DEMO_OUTPUT_MAX];
    size_t  length;
    int32_t status;     /* the child's wait status, or -1 if it did not run */
} demo_run;

static demo_run run;

/* Run body in a forked child with its stdout on a pipe. Collects what it
 * printed and its wait status; body's return value is its exit code. */
static void run_demo(int (*body)(void)) {
    run.length = 0;
    run.status = -1;

    int32_t fds[2];
    if (lc_kernel_create_pipe(fds, 0) < 0) return;
    lc_sysret pid = lc_kernel_fork();
    if (pid < 0) {
        lc_kernel_close_file(fds[0]);
        lc_kernel_close_file(fds[1]);
        return;
    }
    if (pid == 0) {
        lc_kernel_close_file(fds[0]);
        if (lc_kernel_duplicate_fd(fds[1], TEST_FD, 0) < 0) lc_kernel_exit(127);
        lc_kernel_close_file(fds[1]);
        lc_kernel_exit(body());
    }

    lc_kernel_close_file(fds[1]);
    char spill[512];
    for (;;) {
        /* Read to the end even when the buffer is full, so the child never
         * blocks on a full pipe. */
        size_t room = DEMO_OUTPUT_MAX - run.length;
        lc_sysret n = room > 0 ? lc_kernel_read_bytes(fds[0], run.text + run.length, room)
                               : lc_kernel_read_bytes(fds[0], spill, sizeof(spill));
        if (n <= 0) break;
        if (room > 0) run.length += (size_t)n;
    }
    lc_kernel_close_file(fds[0]);

    int32_t status = 0;
    if (lc_kernel_wait_for_child((int32_t)pid, &status, 0) == pid) run.status = status;
}

static bool printed(const char *text) {
    return lc_string_contains(run.text, run.length, text, lc_string_length(text));
}

static int64_t times_printed(const char *text) {
    size_t length = lc_string_length(text);
    int64_t count = 0;
    size_t at = 0;
    for (;;) {
        int64_t found = lc_string_find_substring(run.text + at, run.length - at, text, length);
        if (found < 0) return count;
        count++;
        at += (size_t)found + length;
    }
}

/* After a failed check, show the child's output. Each line goes behind
 * "  | ", so none of them reads as a SKIP: or RESULT: line of this suite. */
static void show_output_if_failed(void) {
    if (_test_current_failed == 0) return;
    _test_print_text("  child's output:\n");
    size_t start = 0;
    for (size_t i = 0; i < run.length; i++) {
        if (run.text[i] != '\n' && i + 1 < run.length) continue;
        _test_print_text("  | ");
        lc_print_string(TEST_FD, run.text + start, i - start);
        lc_print_newline(TEST_FD);
        start = i + 1;
    }
}

/* ===== TEST_REQUIRE_* end their own test only =====
 *
 * Each demo fails one TEST_REQUIRE_* and then prints AFTER_REQUIRE, which must
 * not appear. demo_require_ptr_ok goes on to write through the NULL value, as
 * a test trusting the check would, so a TEST_REQUIRE_PTR_OK that did not
 * return would kill the child with SIGSEGV. The demo after them must still
 * run, and so must the one whose checks all hold.
 */

#define AFTER_REQUIRE "  reached the line after a failed TEST_REQUIRE\n"

static int32_t seven = 7;
static int     an_int;
static int    *some_int = &an_int;  /* a variable, so == NULL is not folded */

static void demo_require(void) {
    TEST_REQUIRE(seven == 8);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_eq(void) {
    TEST_REQUIRE_EQ(seven, 8);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_neq(void) {
    TEST_REQUIRE_NEQ(seven, 7);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_null(void) {
    TEST_REQUIRE_NULL(some_int);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_not_null(void) {
    int *nothing = NULL;
    TEST_REQUIRE_NOT_NULL(nothing);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_str_eq(void) {
    TEST_REQUIRE_STR_EQ("abc", 3, "abd", 3);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_ok(void) {
    TEST_REQUIRE_OK(lc_err(LC_ERR_INVAL));
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_err(void) {
    TEST_REQUIRE_ERR(lc_ok(1));
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_ptr_ok(void) {
    lc_result_ptr r = lc_err_ptr(LC_ERR_NOMEM);
    TEST_REQUIRE_PTR_OK(r);
    _test_print_text(AFTER_REQUIRE);
    *(volatile int *)r.value = 1;
}

static void demo_require_ptr_err(void) {
    TEST_REQUIRE_PTR_ERR(lc_ok_ptr(some_int));
    _test_print_text(AFTER_REQUIRE);
}

static void demo_requires_hold(void) {
    int *nothing = NULL;
    TEST_REQUIRE(seven == 7);
    TEST_REQUIRE_EQ(seven, 7);
    TEST_REQUIRE_NEQ(seven, 8);
    TEST_REQUIRE_NULL(nothing);
    TEST_REQUIRE_NOT_NULL(some_int);
    TEST_REQUIRE_STR_EQ("abc", 3, "abc", 3);
    TEST_REQUIRE_OK(lc_ok(1));
    TEST_REQUIRE_ERR(lc_err(LC_ERR_INVAL));
    TEST_REQUIRE_PTR_OK(lc_ok_ptr(some_int));
    TEST_REQUIRE_PTR_ERR(lc_err_ptr(LC_ERR_NOMEM));
    _test_print_text("  every TEST_REQUIRE held\n");
}

static int demo_requires(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_require);
    TEST_RUN(demo_require_eq);
    TEST_RUN(demo_require_neq);
    TEST_RUN(demo_require_null);
    TEST_RUN(demo_require_not_null);
    TEST_RUN(demo_require_str_eq);
    TEST_RUN(demo_require_ok);
    TEST_RUN(demo_require_err);
    TEST_RUN(demo_require_ptr_ok);
    TEST_RUN(demo_require_ptr_err);
    TEST_RUN(demo_requires_hold);
    return test_main();
}

static void test_require_ends_only_its_test(void) {
    run_demo(demo_requires);
    TEST_ASSERT_EQ(run.status, 1 << 8);     /* exit 1, not killed by SIGSEGV */
    TEST_ASSERT_EQ(times_printed("RUN  "), 11);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 10);
    TEST_ASSERT(!printed(AFTER_REQUIRE));
    TEST_ASSERT(printed("FAIL: r is ok\nRUN  demo_require_ptr_err\n"));
    TEST_ASSERT(printed("RUN  demo_requires_hold\n  every TEST_REQUIRE held\n  OK\n"));
    TEST_ASSERT(printed("Tests run:     11\nTests passed:  1\nTests failed:  10\n"));
    TEST_ASSERT(printed("RESULT: FAIL\n"));
    show_output_if_failed();
}

/* ===== TEST_ASSERT_EQ and TEST_ASSERT_NEQ print both values ===== */

static void demo_eq_values(void) {
    uint64_t most = UINT64_MAX;
    int64_t minus_five = -5;
    bool yes = true;
    bool no = false;
    char letter = 'a';
    double half = 0.5;
    int *pointer = some_int;

    TEST_ASSERT_EQ(seven, 8);
    TEST_ASSERT_EQ(most, (uint64_t)1);
    TEST_ASSERT_EQ(minus_five, (int64_t)5);
    TEST_ASSERT_EQ(yes, no);
    TEST_ASSERT_EQ(letter, 'b');
    TEST_ASSERT_EQ(half, 0.25);
    TEST_ASSERT_EQ(pointer, NULL);
    TEST_ASSERT_NEQ(seven, 7);
    TEST_ASSERT_EQ(seven, 7);               /* holds, so prints nothing */
}

static int demo_eq(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_eq_values);
    return test_main();
}

static void test_eq_prints_both_values(void) {
    run_demo(demo_eq);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 8);
    TEST_ASSERT(printed("  " __FILE__ ":"));
    TEST_ASSERT(printed(": FAIL: seven == 8 (7 vs 8)\n"));
    TEST_ASSERT(printed(": FAIL: most == (uint64_t)1 (18446744073709551615 vs 1)\n"));
    TEST_ASSERT(printed(": FAIL: minus_five == (int64_t)5 (-5 vs 5)\n"));
    TEST_ASSERT(printed(": FAIL: yes == no (true vs false)\n"));
    TEST_ASSERT(printed(": FAIL: letter == 'b' (97 'a' vs 98)\n"));
    TEST_ASSERT(printed(": FAIL: half == 0.25 (0.500000 vs 0.250000)\n"));
    TEST_ASSERT(printed(": FAIL: pointer == NULL (0x"));
    TEST_ASSERT(printed(" vs NULL)\n"));
    TEST_ASSERT(printed(": FAIL: seven != 7 (7 vs 7)\n"));
    TEST_ASSERT(printed("Tests run:     1\nTests passed:  0\nTests failed:  1\n"));
    show_output_if_failed();
}

/* aarch64's long double is quad precision, done by libgcc routines that
 * lightc does not link, so only x86_64 compares long doubles. */
#ifdef __x86_64__
static void demo_eq_long_double_values(void) {
    long double half = 0.5L;
    TEST_ASSERT_EQ(half, 0.25L);
    TEST_ASSERT_EQ(half, 0.5L);             /* holds, so prints nothing */
}

static int demo_eq_long_double(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_eq_long_double_values);
    return test_main();
}

static void test_eq_prints_long_double_values(void) {
    run_demo(demo_eq_long_double);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 1);
    TEST_ASSERT(printed(": FAIL: half == 0.25L (0.500000 vs 0.250000)\n"));
    show_output_if_failed();
}
#endif

static void test_eq_evaluates_operands_once(void) {
    int calls = 0;
    TEST_ASSERT_EQ(++calls, 1);
    TEST_ASSERT_NEQ(++calls, 1);
    TEST_REQUIRE_EQ(++calls, 3);
    TEST_ASSERT_EQ(calls, 3);
}

/* Operands convert as for == itself, and an unsigned operand compared with a
 * non-negative constant builds without -Wsign-compare warnings. */
static void test_eq_compares_as_c_does(void) {
    uint8_t byte = 200;
    size_t length = 3;
    const char *text = "x";
    char *same = (char *)text;
    TEST_ASSERT_EQ(byte, 200);
    TEST_ASSERT_EQ(length, 3);
    TEST_ASSERT_EQ(text, same);
    TEST_ASSERT_EQ(true, 1);
    TEST_ASSERT_EQ(0.5f, 0.5);
    TEST_ASSERT_NEQ(byte, 0);
}

/* ===== TEST_SKIP is counted ===== */

#define AFTER_SKIP "  reached the line after TEST_SKIP\n"

static void demo_skips(void) {
    TEST_SKIP("demo reason");
    _test_print_text(AFTER_SKIP);
}

static void demo_passes(void) {
    TEST_ASSERT(seven == 7);
}

static int demo_skip(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_skips);
    TEST_RUN(demo_passes);
    return test_main();
}

static void test_skip_is_counted(void) {
    run_demo(demo_skip);
    TEST_ASSERT_EQ(run.status, 0);          /* a skip does not fail the suite */
    TEST_ASSERT(!printed(AFTER_SKIP));
    TEST_ASSERT(printed("RUN  demo_skips\n  SKIP: demo reason\nRUN  demo_passes\n  OK\n"));
    TEST_ASSERT(printed("Tests run:     2\nTests passed:  1\nTests failed:  0\n"
                        "Tests skipped: 1\n"));
    TEST_ASSERT(printed("RESULT: PASS\n"));
    show_output_if_failed();
}

static void demo_fails_then_skips(void) {
    TEST_ASSERT(seven == 8);
    TEST_SKIP("after a failure");
}

static int demo_skip_after_failure(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_fails_then_skips);
    return test_main();
}

static void test_skip_after_failure_counts_as_failure(void) {
    run_demo(demo_skip_after_failure);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT(printed("Tests failed:  1\nTests skipped: 0\n"));
    TEST_ASSERT(printed("RESULT: FAIL\n"));
    show_output_if_failed();
}

/* ===== Names on the command line pick the tests ===== */

static void demo_alpha(void) {}
static void demo_beta(void) {}
static void demo_gamma(void) {}

static int demo_three_tests(int argc, char **argv) {
    test_init(argc, argv);
    TEST_RUN(demo_alpha);
    TEST_RUN(demo_beta);
    TEST_RUN(demo_gamma);
    return test_main();
}

static int demo_name_beta(void) {
    char *argv[] = { "test_harness", "demo_beta", NULL };
    return demo_three_tests(2, argv);
}

static int demo_name_gamma_alpha(void) {
    char *argv[] = { "test_harness", "demo_gamma", "demo_alpha", NULL };
    return demo_three_tests(3, argv);
}

static int demo_name_unknown(void) {
    char *argv[] = { "test_harness", "demo_beta", "demo_delta", NULL };
    return demo_three_tests(3, argv);
}

static int demo_too_many_names(void) {
    char *argv[TEST_MAX_NAMES + 3];
    argv[0] = "test_harness";
    for (int i = 1; i <= TEST_MAX_NAMES + 1; i++) argv[i] = "demo_beta";
    argv[TEST_MAX_NAMES + 2] = NULL;
    return demo_three_tests(TEST_MAX_NAMES + 2, argv);
}

static void test_one_name_runs_one_test(void) {
    run_demo(demo_name_beta);
    TEST_ASSERT_EQ(run.status, 0);
    TEST_ASSERT_EQ(times_printed("RUN  "), 1);
    TEST_ASSERT(printed("RUN  demo_beta\n  OK\n"));
    TEST_ASSERT(printed("Tests run:     1\nTests passed:  1\n"));
    show_output_if_failed();
}

/* The demos above hand test_init an argv of their own. Here the child
 * re-executes this binary (/proc/self/exe) with one test's name as its only
 * argument, so the name reaches test_init through main, as it does for
 * `build/test_harness NAME`. Under qemu-user without a binfmt handler for the
 * guest arch, the host kernel cannot exec the binary and execve fails with
 * ENOEXEC: the test skips then, as test_lifecycle's re-executing test does. */
#define REEXEC_NAME     "test_eq_evaluates_operands_once"
#define REEXEC_MARK     "LIGHTC_TEST_HARNESS_REEXECUTED=1"  /* in the child's envp */
#define REEXEC_SKIPPED  77      /* execve failed with ENOEXEC */
#define REEXEC_FAILED   127     /* execve failed otherwise */
#define ERRNO_NOEXEC    8       /* ENOEXEC: exec format error */

static bool reexecuted;         /* set by main when envp holds REEXEC_MARK */

static int reexec_with_one_name(void) {
    char *argv[] = { "/proc/self/exe", REEXEC_NAME, NULL };
    char *envp[] = { REEXEC_MARK, NULL };
    lc_sysret ret = lc_kernel_execute("/proc/self/exe", argv, envp);
    if (ret == -ERRNO_NOEXEC) return REEXEC_SKIPPED;
    _test_print_text("execve(/proc/self/exe) = ");
    lc_print_signed(TEST_FD, ret);
    lc_print_newline(TEST_FD);
    return REEXEC_FAILED;
}

static void test_command_line_name_runs_one_test(void) {
    /* This test runs in the re-executed child only if the name did not reach
     * test_init. Fail there rather than re-execute again, and again. */
    TEST_REQUIRE(!reexecuted);
    run_demo(reexec_with_one_name);
    if (run.status == REEXEC_SKIPPED << 8)
        TEST_SKIP("cannot re-execute the suite binary, execve(/proc/self/exe) = -8 "
                  "(ENOEXEC: no binfmt handler for this arch?)");
    TEST_ASSERT_EQ(run.status, 0);
    TEST_ASSERT_EQ(times_printed("RUN  "), 1);
    TEST_ASSERT(printed("RUN  " REEXEC_NAME "\n  OK\n"));
    TEST_ASSERT(printed("Tests run:     1\nTests passed:  1\nTests failed:  0\n"));
    show_output_if_failed();
}

static void test_names_run_in_suite_order(void) {
    run_demo(demo_name_gamma_alpha);
    TEST_ASSERT_EQ(run.status, 0);
    TEST_ASSERT_EQ(times_printed("RUN  "), 2);
    TEST_ASSERT(printed("RUN  demo_alpha\n  OK\nRUN  demo_gamma\n  OK\n"));
    TEST_ASSERT(printed("Tests run:     2\nTests passed:  2\n"));
    show_output_if_failed();
}

static void test_unknown_name_fails(void) {
    run_demo(demo_name_unknown);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT(printed("RUN  demo_beta\n  OK\nNo test named demo_delta\n"));
    TEST_ASSERT(printed("Tests run:     1\nTests passed:  1\nTests failed:  0\n"));
    TEST_ASSERT(printed("RESULT: FAIL\n"));
    show_output_if_failed();
}

static void test_too_many_names_fail(void) {
    run_demo(demo_too_many_names);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed("RUN  "), 1);
    TEST_ASSERT(printed("Too many test names, at most 64: demo_beta\n"));
    TEST_ASSERT(printed("RESULT: FAIL\n"));
    show_output_if_failed();
}

/* ===== main ===== */

int main(int argc, char **argv, char **envp) {
    for (char **e = envp; *e != NULL; e++)
        if (lc_string_equal(*e, lc_string_length(*e), REEXEC_MARK, sizeof(REEXEC_MARK) - 1))
            reexecuted = true;
    test_init(argc, argv);

    TEST_RUN(test_require_ends_only_its_test);
    TEST_RUN(test_eq_prints_both_values);
#ifdef __x86_64__
    TEST_RUN(test_eq_prints_long_double_values);
#endif
    TEST_RUN(test_eq_evaluates_operands_once);
    TEST_RUN(test_eq_compares_as_c_does);
    TEST_RUN(test_skip_is_counted);
    TEST_RUN(test_skip_after_failure_counts_as_failure);
    TEST_RUN(test_one_name_runs_one_test);
    TEST_RUN(test_command_line_name_runs_one_test);
    TEST_RUN(test_names_run_in_suite_order);
    TEST_RUN(test_unknown_name_fails);
    TEST_RUN(test_too_many_names_fail);

    return test_main();
}

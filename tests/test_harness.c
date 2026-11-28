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

/* ===== Floating-point assertions =====
 *
 * The values are made from their bits, so each test states the exact values
 * it compares, NaN payloads included.
 */

static float float_from_bits(uint32_t bits) {
    union { uint32_t u; float f; } pun = { .u = bits };
    return pun.f;
}

static double double_from_bits(uint64_t bits) {
    union { uint64_t u; double d; } pun = { .u = bits };
    return pun.d;
}

/* The ulp distance between the floats or the doubles of these bits, found
 * the same both ways round, or NO_DISTANCE when they have none. */
#define NO_DISTANCE UINT64_MAX

static uint64_t ulps_of(_test_fp a, _test_fp b) {
    _test_fp_result there = _test_fp_check(_test_fp_bound_ulp(UINT64_MAX), a, b);
    _test_fp_result back = _test_fp_check(_test_fp_bound_ulp(UINT64_MAX), b, a);
    TEST_ASSERT_EQ(there.ok, back.ok);
    TEST_ASSERT_EQ(there.ulps, back.ulps);
    return there.ok ? there.ulps : NO_DISTANCE;
}

static uint64_t float_ulps(uint32_t a, uint32_t b) {
    return ulps_of(_test_fp_float(float_from_bits(a)), _test_fp_float(float_from_bits(b)));
}

static uint64_t double_ulps(uint64_t a, uint64_t b) {
    return ulps_of(_test_fp_double(double_from_bits(a)), _test_fp_double(double_from_bits(b)));
}

static void test_ulp_distance(void) {
    /* Zeros, and across zero. */
    TEST_ASSERT_EQ(float_ulps(0x00000000, 0x80000000), 0);      /* +0, -0 */
    TEST_ASSERT_EQ(float_ulps(0x00000000, 0x00000001), 1);      /* +0, smallest subnormal */
    TEST_ASSERT_EQ(float_ulps(0x80000000, 0x00000001), 1);      /* -0, smallest subnormal */
    TEST_ASSERT_EQ(float_ulps(0x80000001, 0x00000001), 2);      /* the smallest of each sign */
    TEST_ASSERT_EQ(float_ulps(0x80000003, 0x00000005), 8);
    TEST_ASSERT_EQ(float_ulps(0x807fffff, 0x007fffff), 0xfffffe);
    TEST_ASSERT_EQ(float_ulps(0xbf800000, 0x3f800000), 2 * 0x3f800000u);   /* -1, 1 */
    TEST_ASSERT_EQ(float_ulps(0xff7fffff, 0x7f7fffff), 2 * 0x7f7fffffu);   /* -max, max */

    /* Subnormals, and from them to the normal values. */
    TEST_ASSERT_EQ(float_ulps(0x00000003, 0x00000010), 13);
    TEST_ASSERT_EQ(float_ulps(0x80000010, 0x80000003), 13);
    TEST_ASSERT_EQ(float_ulps(0x007fffff, 0x00800000), 1);      /* largest subnormal, smallest normal */
    TEST_ASSERT_EQ(float_ulps(0x00000001, 0x00800001), 0x800000);

    /* Normal values, across a power of two too. */
    TEST_ASSERT_EQ(float_ulps(0x3f800000, 0x3f800000), 0);
    TEST_ASSERT_EQ(float_ulps(0x3f800000, 0x3f800001), 1);
    TEST_ASSERT_EQ(float_ulps(0x3f7fffff, 0x3f800001), 2);
    TEST_ASSERT_EQ(float_ulps(0x7f7ffffe, 0x7f7fffff), 1);

    /* Infinities and NaNs. */
    TEST_ASSERT_EQ(float_ulps(0x7f800000, 0x7f800000), 0);      /* inf, inf */
    TEST_ASSERT_EQ(float_ulps(0xff800000, 0xff800000), 0);      /* -inf, -inf */
    TEST_ASSERT_EQ(float_ulps(0x7f7fffff, 0x7f800000), NO_DISTANCE);   /* max, inf */
    TEST_ASSERT_EQ(float_ulps(0xff800000, 0x7f800000), NO_DISTANCE);   /* -inf, inf */
    TEST_ASSERT_EQ(float_ulps(0x7fc00000, 0xffc00001), 0);      /* NaNs of either sign */
    TEST_ASSERT_EQ(float_ulps(0x7f800001, 0x7fc00000), 0);      /* signalling, quiet */
    TEST_ASSERT_EQ(float_ulps(0x7fc00000, 0x3f800000), NO_DISTANCE);   /* NaN, 1 */
    TEST_ASSERT_EQ(float_ulps(0x7fc00000, 0x7f800000), NO_DISTANCE);   /* NaN, inf */

    /* Doubles, by the same rules. */
    TEST_ASSERT_EQ(double_ulps(0x0000000000000000, 0x8000000000000000), 0);
    TEST_ASSERT_EQ(double_ulps(0x8000000000000001, 0x0000000000000001), 2);
    TEST_ASSERT_EQ(double_ulps(0x0000000000000003, 0x0000000000000010), 13);
    TEST_ASSERT_EQ(double_ulps(0x000fffffffffffff, 0x0010000000000000), 1);
    TEST_ASSERT_EQ(double_ulps(0x3ff0000000000000, 0x3ff0000000000001), 1);
    TEST_ASSERT_EQ(double_ulps(0xffefffffffffffff, 0x7fefffffffffffff), 0xffdffffffffffffe);
    TEST_ASSERT_EQ(double_ulps(0x7fefffffffffffff, 0x7ff0000000000000), NO_DISTANCE);
    TEST_ASSERT_EQ(double_ulps(0x7ff8000000000000, 0xfff8000000000001), 0);
    TEST_ASSERT_EQ(double_ulps(0x7ff8000000000000, 0x3ff0000000000000), NO_DISTANCE);
}

/* Each float assertion holds where it should, for floats and doubles. */
static void test_float_asserts_hold(void) {
    float one = 1.0f;
    float one_up = float_from_bits(0x3f800001);
    float zero = 0.0f;
    float minus_zero = float_from_bits(0x80000000);
    float nan = float_from_bits(0x7fc00000);
    float other_nan = float_from_bits(0xffc00123);
    float signalling_nan = float_from_bits(0x7f800001);
    float inf = float_from_bits(0x7f800000);
    float minus_inf = float_from_bits(0xff800000);
    float tiny = float_from_bits(0x00000001);
    float minus_tiny = float_from_bits(0x80000001);
    double d_one = 1.0;
    double d_nan = double_from_bits(0x7ff8000000000000);
    double d_other_nan = double_from_bits(0xfff0000000000001);
    double d_inf = double_from_bits(0x7ff0000000000000);

    TEST_ASSERT_EXACT(one, 1.0f);
    TEST_ASSERT_EXACT(minus_zero, -0.0f);
    TEST_ASSERT_EXACT(nan, other_nan);
    TEST_ASSERT_EXACT(signalling_nan, nan);
    TEST_ASSERT_EXACT(inf, inf);
    TEST_ASSERT_EXACT(minus_inf, -inf);
    TEST_ASSERT_EXACT(d_one, 1.0);
    TEST_ASSERT_EXACT(d_nan, d_other_nan);
    TEST_ASSERT_EXACT(d_inf, d_inf);

    TEST_ASSERT_ULP(one, one, 0);
    TEST_ASSERT_ULP(one, one_up, 1);
    TEST_ASSERT_ULP(zero, minus_zero, 0);
    TEST_ASSERT_ULP(minus_tiny, tiny, 2);
    TEST_ASSERT_ULP(nan, other_nan, 0);
    TEST_ASSERT_ULP(inf, inf, 0);
    TEST_ASSERT_ULP(d_one, double_from_bits(0x3feffffffffffffe), 2);
    TEST_ASSERT_ULP(d_nan, d_other_nan, 0);

    TEST_ASSERT_NEAR(one, one_up, 0, 0.0000002);
    TEST_ASSERT_NEAR(one, 1.25f, 0.25, 0);                  /* at the bound */
    TEST_ASSERT_NEAR(1000.0f, 1001.0f, 0, 0.001);           /* 1 <= 0.001 * 1001 */
    TEST_ASSERT_NEAR(1000.0f, 1001.0f, 1, 0.0001);          /* the larger tolerance counts */
    TEST_ASSERT_NEAR(zero, minus_zero, 0, 0);
    TEST_ASSERT_NEAR(nan, other_nan, 0, 0);
    TEST_ASSERT_NEAR(inf, inf, 0, 0);
    TEST_ASSERT_NEAR(d_one, 1.5, 0.5, 0);
    TEST_ASSERT_NEAR(d_nan, d_other_nan, 0, 0);

    const float want[4] = { 1.0f, 0.0f, 3.0f, 4.0f };
    float got[4] = { 1.0f, 0.0f, 3.0f, 4.0f };
    got[2] = float_from_bits(0x40400001);                   /* 3 and 1 ulp */
    got[3] = nan;
    float want_nan[4] = { 1.0f, 0.0f, 3.0f, 4.0f };
    want_nan[2] = got[2];
    want_nan[3] = other_nan;
    TEST_ASSERT_EXACT_ARRAY(got, want_nan, 4);
    TEST_ASSERT_EXACT_ARRAY(got, want, 2);
    TEST_ASSERT_ULP_ARRAY(got, want_nan, 4, 0);
    TEST_ASSERT_ULP_ARRAY(got, want, 3, 1);
    TEST_ASSERT_NEAR_ARRAY(got, want, 3, 0.000001, 0);
    double d_got[2] = { 1.0, 2.0 };
    double d_want[2] = { 1.0, 2.0 };
    TEST_ASSERT_EXACT_ARRAY(d_got, d_want, 2);
    TEST_ASSERT_ULP_ARRAY(d_got, d_want, 2, 0);
    TEST_ASSERT_NEAR_ARRAY(d_got, d_want, 2, 0, 0);
    const float *none = NULL;                               /* a count of 0 reads nothing */
    TEST_ASSERT_EXACT_ARRAY(none, none, 0);
    TEST_ASSERT_ULP_ARRAY(none, none, 0, 0);
    TEST_ASSERT_NEAR_ARRAY(none, none, 0, 0, 0);

    TEST_REQUIRE_EXACT(nan, other_nan);
    TEST_REQUIRE_ULP(one, one_up, 1);
    TEST_REQUIRE_NEAR(one, 1.25f, 0.25, 0);
    TEST_REQUIRE_EXACT_ARRAY(got, want_nan, 4);
    TEST_REQUIRE_ULP_ARRAY(got, want, 3, 1);
    TEST_REQUIRE_NEAR_ARRAY(d_got, d_want, 2, 0, 0);
}

/* Each argument is evaluated once, the arrays' too. */
static int fp_calls;
static const float ones[2] = { 1.0f, 1.0f };

static float next_one(void) { fp_calls++; return 1.0f; }
static const float *next_ones(void) { fp_calls++; return ones; }
static size_t next_count(void) { fp_calls++; return 2; }
static uint64_t next_ulps(void) { fp_calls++; return 0; }
static double next_tolerance(void) { fp_calls++; return 0.0; }

static void test_float_asserts_evaluate_once(void) {
    fp_calls = 0;
    TEST_ASSERT_EXACT(next_one(), next_one());
    TEST_ASSERT_ULP(next_one(), next_one(), next_ulps());
    TEST_ASSERT_NEAR(next_one(), next_one(), next_tolerance(), next_tolerance());
    TEST_ASSERT_EQ(fp_calls, 9);
    TEST_ASSERT_EXACT_ARRAY(next_ones(), next_ones(), next_count());
    TEST_ASSERT_ULP_ARRAY(next_ones(), next_ones(), next_count(), next_ulps());
    TEST_ASSERT_NEAR_ARRAY(next_ones(), next_ones(), next_count(), next_tolerance(),
                           next_tolerance());
    TEST_REQUIRE_EXACT(next_one(), next_one());
    TEST_ASSERT_EQ(fp_calls, 9 + 3 + 4 + 5 + 2);
}

/* ===== The float assertions fail, and say why ===== */

static void demo_exact_values(void) {
    float one = 1.0f;
    float one_up = float_from_bits(0x3f800001);
    float zero = 0.0f;
    float minus_zero = float_from_bits(0x80000000);
    float nan = float_from_bits(0x7fc00000);
    float largest = float_from_bits(0x7f7fffff);
    float inf = float_from_bits(0x7f800000);
    double d_one = 1.0;
    double d_one_up = double_from_bits(0x3ff0000000000001);

    TEST_ASSERT_EXACT(one, one_up);
    TEST_ASSERT_EXACT(zero, minus_zero);
    TEST_ASSERT_EXACT(nan, one);
    TEST_ASSERT_EXACT(largest, inf);
    TEST_ASSERT_EXACT(d_one, d_one_up);
    TEST_ASSERT_EXACT(one, one);            /* holds, so prints nothing */
}

static int demo_exact(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_exact_values);
    return test_main();
}

static void test_exact_fails(void) {
    run_demo(demo_exact);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 5);
    TEST_ASSERT(printed(": FAIL: exact(one, one_up) (1.00000000e+00 0x3f800000 vs "
                        "1.00000012e+00 0x3f800001, 1 ulp apart)\n"));
    TEST_ASSERT(printed(": FAIL: exact(zero, minus_zero) (0.00000000e+00 0x00000000 vs "
                        "-0.00000000e+00 0x80000000, 0 ulp apart)\n"));
    TEST_ASSERT(printed(": FAIL: exact(nan, one) (nan 0x7fc00000 vs "
                        "1.00000000e+00 0x3f800000, only one is a NaN)\n"));
    TEST_ASSERT(printed(": FAIL: exact(largest, inf) (3.40282347e+38 0x7f7fffff vs "
                        "inf 0x7f800000, an infinity matches only itself)\n"));
    TEST_ASSERT(printed(": FAIL: exact(d_one, d_one_up) (1.00000000e+00 0x3ff0000000000000 vs "
                        "1.00000000e+00 0x3ff0000000000001, 1 ulp apart)\n"));
    show_output_if_failed();
}

static void demo_ulp_values(void) {
    float one = 1.0f;
    float three_up = float_from_bits(0x3f800003);
    float tiny = float_from_bits(0x00000001);
    float minus_tiny = float_from_bits(0x80000001);
    float sub3 = float_from_bits(0x00000003);
    float sub16 = float_from_bits(0x00000010);
    float largest_sub = float_from_bits(0x007fffff);
    float smallest_normal = float_from_bits(0x00800000);
    float nan = float_from_bits(0x7fc00000);
    float largest = float_from_bits(0x7f7fffff);
    float inf = float_from_bits(0x7f800000);
    double d_tiny = double_from_bits(0x0000000000000001);
    double d_minus_tiny = double_from_bits(0x8000000000000001);
    uint64_t bound = 1;

    TEST_ASSERT_ULP(one, three_up, 1);
    TEST_ASSERT_ULP(minus_tiny, tiny, 1);                   /* across zero */
    TEST_ASSERT_ULP(sub3, sub16, 12);                       /* between subnormals */
    TEST_ASSERT_ULP(largest_sub, smallest_normal, 0);
    TEST_ASSERT_ULP(nan, one, 1000);
    TEST_ASSERT_ULP(largest, inf, 1000);
    TEST_ASSERT_ULP(d_minus_tiny, d_tiny, bound);
    TEST_ASSERT_ULP(sub3, sub16, 13);                       /* holds, so prints nothing */
}

static int demo_ulp(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_ulp_values);
    return test_main();
}

static void test_ulp_fails(void) {
    run_demo(demo_ulp);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 7);
    TEST_ASSERT(printed(": FAIL: ulp(one, three_up) <= 1 (1.00000000e+00 0x3f800000 vs "
                        "1.00000036e+00 0x3f800003, 3 ulp apart, bound 1)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(minus_tiny, tiny) <= 1 (-1.40129846e-45 0x80000001 vs "
                        "1.40129846e-45 0x00000001, 2 ulp apart, bound 1)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(sub3, sub16) <= 12 (4.20389539e-45 0x00000003 vs "
                        "2.24207754e-44 0x00000010, 13 ulp apart, bound 12)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(largest_sub, smallest_normal) <= 0 (1.17549421e-38 "
                        "0x007fffff vs 1.17549435e-38 0x00800000, 1 ulp apart, bound 0)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(nan, one) <= 1000 (nan 0x7fc00000 vs "
                        "1.00000000e+00 0x3f800000, only one is a NaN)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(largest, inf) <= 1000 (3.40282347e+38 0x7f7fffff vs "
                        "inf 0x7f800000, an infinity matches only itself)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(d_minus_tiny, d_tiny) <= bound (-4.94065646e-324 "
                        "0x8000000000000001 vs 4.94065646e-324 0x0000000000000001, "
                        "2 ulp apart, bound 1)\n"));
    show_output_if_failed();
}

static void demo_near_values(void) {
    float one = 1.0f;
    float nan = float_from_bits(0x7fc00000);
    float largest = float_from_bits(0x7f7fffff);
    float inf = float_from_bits(0x7f800000);
    float minus_inf = float_from_bits(0xff800000);
    double d_one = 1.0;
    double d_tolerance = 0.25;

    TEST_ASSERT_NEAR(one, 1.1f, 0.01, 0);                   /* absolute */
    TEST_ASSERT_NEAR(one, 1.00001f, 1e-6, 0);               /* a small bound reads as one */
    TEST_ASSERT_NEAR(1000.0f, 1002.0f, 0, 0.001);           /* relative */
    TEST_ASSERT_NEAR(nan, one, 1, 1);
    TEST_ASSERT_NEAR(inf, largest, 0, 1);                   /* rel * inf would be inf */
    TEST_ASSERT_NEAR(minus_inf, inf, 0, 1);
    TEST_ASSERT_NEAR(d_one, 1.5, d_tolerance, 0);
    TEST_ASSERT_NEAR(d_one, 1.5, 0.5, 0);                   /* holds, so prints nothing */
}

static int demo_near(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_near_values);
    return test_main();
}

static void test_near_fails(void) {
    run_demo(demo_near);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 7);
    TEST_ASSERT(printed(": FAIL: near(one, 1.1f, 0.01, 0) (1.00000000e+00 0x3f800000 vs "
                        "1.10000002e+00 0x3f8ccccd, error 1.00000024e-01, "
                        "bound 1.00000000e-02)\n"));
    TEST_ASSERT(printed(": FAIL: near(one, 1.00001f, 1e-6, 0) (1.00000000e+00 0x3f800000 vs "
                        "1.00001001e+00 0x3f800054, error 1.00135803e-05, "
                        "bound 1.00000000e-06)\n"));
    TEST_ASSERT(printed(": FAIL: near(1000.0f, 1002.0f, 0, 0.001) (1.00000000e+03 0x447a0000 vs "
                        "1.00200000e+03 0x447a8000, error 2.00000000e+00, "
                        "bound 1.00200000e+00)\n"));
    TEST_ASSERT(printed(": FAIL: near(nan, one, 1, 1) (nan 0x7fc00000 vs "
                        "1.00000000e+00 0x3f800000, only one is a NaN)\n"));
    TEST_ASSERT(printed(": FAIL: near(inf, largest, 0, 1) (inf 0x7f800000 vs "
                        "3.40282347e+38 0x7f7fffff, an infinity matches only itself)\n"));
    TEST_ASSERT(printed(": FAIL: near(minus_inf, inf, 0, 1) (-inf 0xff800000 vs "
                        "inf 0x7f800000, an infinity matches only itself)\n"));
    TEST_ASSERT(printed(": FAIL: near(d_one, 1.5, d_tolerance, 0) (1.00000000e+00 "
                        "0x3ff0000000000000 vs 1.50000000e+00 0x3ff8000000000000, "
                        "error 5.00000000e-01, bound 2.50000000e-01)\n"));
    show_output_if_failed();
}

/* ===== The array forms report the worst element ===== */

static void demo_array_values(void) {
    /* 3 fails by 2 ulps first, and 6 by 9 ulps later: 6 is the worst. */
    const float want[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    float got[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    got[1] = float_from_bits(0x40000001);   /* 1 ulp, within the bound */
    got[2] = float_from_bits(0x40400002);
    got[5] = float_from_bits(0x40c00009);
    got[6] = float_from_bits(0x40e00003);
    TEST_ASSERT_ULP_ARRAY(got, want, 8, 1);

    /* Of two equally far, the first is the worst. */
    float tied[3] = { 1.0f, 2.0f, 3.0f };
    tied[0] = float_from_bits(0x3f800003);
    tied[2] = float_from_bits(0x40400003);
    TEST_ASSERT_ULP_ARRAY(tied, want, 3, 0);

    /* A NaN against a number is worse than any distance. */
    float with_nan[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    with_nan[1] = float_from_bits(0x40000000 + 1000);
    with_nan[3] = float_from_bits(0x7fc00000);
    TEST_ASSERT_ULP_ARRAY(with_nan, want, 4, 1);

    /* +0 against -0 fails, 0 ulps apart; two NaNs match. */
    float exact_got[5] = { 1.0f, 0.0f, 1.0f, 0.0f, 5.0f };
    float exact_want[5] = { 1.0f, 0.0f, 1.0f, 0.0f, 5.0f };
    exact_got[1] = float_from_bits(0x80000000);
    exact_got[2] = float_from_bits(0x3f800001);
    exact_got[3] = float_from_bits(0x7fc00000);
    exact_want[3] = float_from_bits(0xffc00001);
    TEST_ASSERT_EXACT_ARRAY(exact_got, exact_want, 5);

    /* The worst is the largest multiple of its bound, not the largest error:
     * 1002 is 2 away but within 1.002 of 1000, 1.1 is 0.1 away but within
     * 0.01 of 1. */
    const float near_want[4] = { 1000.0f, 1.0f, 2.0f, 3.0f };
    const float near_got[4] = { 1002.0f, 1.1f, 2.0f, 3.05f };
    TEST_ASSERT_NEAR_ARRAY(near_got, near_want, 4, 0.01, 0.001);

    /* With a bound of 0 every error is an infinite multiple of it, and the
     * larger error is the worse. */
    const float zero_want[3] = { 1.0f, 2.0f, 8.0f };
    const float zero_got[3] = { 1.5f, 2.0f, 10.0f };
    TEST_ASSERT_NEAR_ARRAY(zero_got, zero_want, 3, 0, 0);

    /* Doubles; an infinity against a number is the worst. */
    double d_want[3] = { 1.0, 1.0, 0.0 };
    double d_got[3] = { 1.0, 0.0, 0.0 };
    d_want[2] = double_from_bits(0x0000000000000004);
    d_got[1] = double_from_bits(0x3ff0000000000002);
    d_got[2] = double_from_bits(0x8000000000000003);
    TEST_ASSERT_ULP_ARRAY(d_got, d_want, 3, 1);
    double inf_want[3] = { 1.0, 2.0, 0.0 };
    double inf_got[3] = { 1.0, 2.5, 0.0 };
    inf_want[2] = double_from_bits(0x7fefffffffffffff);
    inf_got[2] = double_from_bits(0x7ff0000000000000);
    TEST_ASSERT_NEAR_ARRAY(inf_got, inf_want, 3, 0.1, 0);

    TEST_ASSERT_ULP_ARRAY(got, want, 8, 9);     /* holds, so prints nothing */
}

static int demo_array(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_array_values);
    return test_main();
}

static void test_array_reports_worst(void) {
    run_demo(demo_array);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 8);
    TEST_ASSERT(printed(": FAIL: ulp(got[i], want[i]) <= 1 for i < 8 (3 of 8 failed, "
                        "the first at [2], the worst at [5]: 6.00000429e+00 0x40c00009 vs "
                        "6.00000000e+00 0x40c00000, 9 ulp apart, bound 1)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(tied[i], want[i]) <= 0 for i < 3 (2 of 3 failed, "
                        "the first at [0], the worst at [0]: 1.00000036e+00 0x3f800003 vs "
                        "1.00000000e+00 0x3f800000, 3 ulp apart, bound 0)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(with_nan[i], want[i]) <= 1 for i < 4 (2 of 4 failed, "
                        "the first at [1], the worst at [3]: nan 0x7fc00000 vs "
                        "4.00000000e+00 0x40800000, only one is a NaN)\n"));
    TEST_ASSERT(printed(": FAIL: exact(exact_got[i], exact_want[i]) for i < 5 (2 of 5 failed, "
                        "the first at [1], the worst at [2]: 1.00000012e+00 0x3f800001 vs "
                        "1.00000000e+00 0x3f800000, 1 ulp apart)\n"));
    TEST_ASSERT(printed(": FAIL: near(near_got[i], near_want[i], 0.01, 0.001) for i < 4 "
                        "(3 of 4 failed, the first at [0], the worst at [1]: 1.10000002e+00 "
                        "0x3f8ccccd vs 1.00000000e+00 0x3f800000, error 1.00000024e-01, "
                        "bound 1.00000000e-02)\n"));
    TEST_ASSERT(printed(": FAIL: near(zero_got[i], zero_want[i], 0, 0) for i < 3 "
                        "(2 of 3 failed, the first at [0], the worst at [2]: 1.00000000e+01 "
                        "0x41200000 vs 8.00000000e+00 0x41000000, error 2.00000000e+00, "
                        "bound 0.00000000e+00)\n"));
    TEST_ASSERT(printed(": FAIL: ulp(d_got[i], d_want[i]) <= 1 for i < 3 (2 of 3 failed, "
                        "the first at [1], the worst at [2]: -1.48219694e-323 "
                        "0x8000000000000003 vs 1.97626258e-323 0x0000000000000004, "
                        "7 ulp apart, bound 1)\n"));
    TEST_ASSERT(printed(": FAIL: near(inf_got[i], inf_want[i], 0.1, 0) for i < 3 "
                        "(2 of 3 failed, the first at [1], the worst at [2]: inf "
                        "0x7ff0000000000000 vs 1.79769313e+308 0x7fefffffffffffff, "
                        "an infinity matches only itself)\n"));
    show_output_if_failed();
}

/* ===== The float TEST_REQUIRE_* end their own test only ===== */

static float half = 0.5f;
static const float halves[2] = { 0.5f, 0.5f };
static const float quarters[2] = { 0.25f, 0.25f };

static void demo_require_exact(void) {
    TEST_REQUIRE_EXACT(half, 0.25f);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_ulp(void) {
    TEST_REQUIRE_ULP(half, 0.25f, 1);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_near(void) {
    TEST_REQUIRE_NEAR(half, 0.25f, 0.1, 0);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_exact_array(void) {
    TEST_REQUIRE_EXACT_ARRAY(halves, quarters, 2);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_ulp_array(void) {
    TEST_REQUIRE_ULP_ARRAY(halves, quarters, 2, 1);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_require_near_array(void) {
    TEST_REQUIRE_NEAR_ARRAY(halves, quarters, 2, 0.1, 0);
    _test_print_text(AFTER_REQUIRE);
}

static void demo_float_requires_hold(void) {
    TEST_REQUIRE_EXACT(half, 0.5f);
    TEST_REQUIRE_ULP(half, 0.5f, 0);
    TEST_REQUIRE_NEAR(half, 0.25f, 0.25, 0);
    TEST_REQUIRE_EXACT_ARRAY(halves, halves, 2);
    TEST_REQUIRE_ULP_ARRAY(halves, halves, 2, 0);
    TEST_REQUIRE_NEAR_ARRAY(halves, quarters, 2, 0.25, 0);
    _test_print_text("  every float TEST_REQUIRE held\n");
}

static int demo_float_requires(void) {
    char *argv[] = { "test_harness", NULL };
    test_init(1, argv);
    TEST_RUN(demo_require_exact);
    TEST_RUN(demo_require_ulp);
    TEST_RUN(demo_require_near);
    TEST_RUN(demo_require_exact_array);
    TEST_RUN(demo_require_ulp_array);
    TEST_RUN(demo_require_near_array);
    TEST_RUN(demo_float_requires_hold);
    return test_main();
}

static void test_float_require_ends_only_its_test(void) {
    run_demo(demo_float_requires);
    TEST_ASSERT_EQ(run.status, 1 << 8);
    TEST_ASSERT_EQ(times_printed("RUN  "), 7);
    TEST_ASSERT_EQ(times_printed(": FAIL: "), 6);
    TEST_ASSERT(!printed(AFTER_REQUIRE));
    TEST_ASSERT(printed(": FAIL: exact(half, 0.25f) ("));
    TEST_ASSERT(printed(": FAIL: ulp(half, 0.25f) <= 1 ("));
    TEST_ASSERT(printed(": FAIL: near(half, 0.25f, 0.1, 0) ("));
    TEST_ASSERT(printed(": FAIL: exact(halves[i], quarters[i]) for i < 2 (2 of 2 failed, "));
    TEST_ASSERT(printed(": FAIL: ulp(halves[i], quarters[i]) <= 1 for i < 2 (2 of 2 failed, "));
    TEST_ASSERT(printed(": FAIL: near(halves[i], quarters[i], 0.1, 0) for i < 2 (2 of 2 failed, "));
    TEST_ASSERT(printed("RUN  demo_float_requires_hold\n  every float TEST_REQUIRE held\n  OK\n"));
    TEST_ASSERT(printed("Tests run:     7\nTests passed:  1\nTests failed:  6\n"));
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
    TEST_RUN(test_ulp_distance);
    TEST_RUN(test_float_asserts_hold);
    TEST_RUN(test_float_asserts_evaluate_once);
    TEST_RUN(test_exact_fails);
    TEST_RUN(test_ulp_fails);
    TEST_RUN(test_near_fails);
    TEST_RUN(test_array_reports_worst);
    TEST_RUN(test_float_require_ends_only_its_test);

    return test_main();
}

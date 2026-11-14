/*
 * test.h — minimal self-hosted test harness for lightc.
 *
 * Freestanding: uses only lightc's own facilities for output.
 * No libc, no assert.h, no stdio.h.
 *
 * Usage:
 *   #include "test.h"
 *
 *   void test_something(void) {
 *       lc_result_ptr r = lc_heap_allocate(16);
 *       TEST_REQUIRE_PTR_OK(r);
 *       TEST_ASSERT(1 + 1 == 2);
 *       TEST_ASSERT_EQ(42, 42);
 *       TEST_ASSERT_STR_EQ("hello", 5, "hello", 5);
 *   }
 *
 *   int main(int argc, char **argv, char **envp) {
 *       (void)envp;
 *       test_init(argc, argv);
 *       TEST_RUN(test_something);
 *       return test_main();
 *   }
 *
 * A failed TEST_ASSERT_* records the failure and the test goes on. Each has a
 * TEST_REQUIRE_* twin with the same arguments that also returns from the
 * function it is in, so the next TEST_RUN still runs: use it where the rest
 * of the test needs the check to hold, such as a pointer it goes on to use.
 * Use them in void functions only. In a helper that a test calls, they
 * return from the helper only.
 *
 * TEST_ASSERT_EQ and TEST_ASSERT_NEQ evaluate each operand once, compare them
 * as == and != would, and on failure print both values by type: integers in
 * decimal, chars also as the character, bools as true or false, floating
 * values, and pointers in hex. Their operands are integers (enums too),
 * bools, chars, floats, doubles, long doubles and object pointers. Compare a
 * pointer with NULL, not 0, which is an int there. Copy a bit-field into a
 * variable, or cast it, first; for a function pointer or any other type, use
 * TEST_ASSERT(a == b). On aarch64, long double arithmetic needs libgcc,
 * which lightc does not link, so only x86_64 tests compare long doubles.
 *
 * TEST_SKIP(reason) prints "SKIP: reason" and returns. The test counts as
 * skipped, or as failed if it already failed. Skipped tests do not fail the
 * suite.
 *
 * Arguments name the tests to run: `build/test_heap test_a test_b` runs only
 * those two, in the suite's order. A name that matches no test fails the
 * suite. Some tests rely on what earlier tests in their suite did, so a test
 * run alone can fail where the whole suite passes.
 */

#ifndef LIGHTC_TEST_H
#define LIGHTC_TEST_H

#include <lightc/print.h>
#include <lightc/string.h>
#include <lightc/types.h>

/* --- Globals for tracking test results --- */

static int _test_total   = 0;
static int _test_passed  = 0;
static int _test_failed  = 0;
static int _test_skipped = 0;
static int _test_current_failed  = 0;
static int _test_current_skipped = 0;

/* Test names from the command line; with none, every test runs. */
#define TEST_MAX_NAMES 64
static char **_test_names      = NULL;
static int    _test_name_count = 0;
static bool   _test_name_matched[TEST_MAX_NAMES];

/* File descriptor for output (stdout = 1) */
#define TEST_FD 1

/* --- Internal helpers --- */

static inline void _test_print_text(const char *s) {
    lc_print_string(TEST_FD, s, lc_string_length(s));
}

static inline void _test_print_location(const char *file, int line) {
    _test_print_text("  ");
    _test_print_text(file);
    _test_print_text(":");
    lc_print_signed(TEST_FD, line);
    _test_print_text(": ");
}

/* Print the FAIL line up to the expression, without the newline. */
static inline void _test_fail_start(const char *file, int line, const char *expr) {
    _test_print_location(file, line);
    _test_print_text("FAIL: ");
    _test_print_text(expr);
    _test_current_failed++;
}

static inline void _test_fail(const char *file, int line, const char *expr) {
    _test_fail_start(file, line, expr);
    lc_print_newline(TEST_FD);
}

static inline void _test_skip(const char *reason) {
    _test_print_text("  SKIP: ");
    _test_print_text(reason);
    lc_print_newline(TEST_FD);
    _test_current_skipped = 1;
}

/* Whether TEST_RUN runs test `name`, which it does when no names were given. */
static inline bool _test_selected(const char *name) {
    if (_test_name_count == 0) return true;
    bool selected = false;
    size_t length = lc_string_length(name);
    for (int i = 0; i < _test_name_count && i < TEST_MAX_NAMES; i++) {
        if (lc_string_equal(name, length, _test_names[i], lc_string_length(_test_names[i]))) {
            _test_name_matched[i] = true;
            selected = true;
        }
    }
    return selected;
}

/* --- Printing the operands of TEST_ASSERT_EQ and TEST_ASSERT_NEQ --- */

static inline void _test_print_value_signed(int64_t value) {
    lc_print_signed(TEST_FD, value);
}

static inline void _test_print_value_unsigned(uint64_t value) {
    lc_print_unsigned(TEST_FD, value);
}

static inline void _test_print_value_bool(bool value) {
    _test_print_text(value ? "true" : "false");
}

/* A char prints as its value, then as 'c' when it is printable ASCII. */
static inline void _test_print_value_char(char value) {
    lc_print_signed(TEST_FD, value);
    if (value >= ' ' && value <= '~') {
        _test_print_text(" '");
        lc_print_char(TEST_FD, value);
        _test_print_text("'");
    }
}

static inline void _test_print_value_double(double value) {
    lc_print_double(TEST_FD, value);
}

static inline void _test_print_value_long_double(long double value) {
    lc_print_double(TEST_FD, (double)value);
}

static inline void _test_print_value_pointer(const volatile void *value) {
    if (value == NULL) _test_print_text("NULL");
    else lc_print_hex(TEST_FD, (uint64_t)(uintptr_t)value);
}

#define _TEST_PRINT_VALUE(x)                                        \
    _Generic((x),                                                   \
        bool:               _test_print_value_bool,                 \
        char:               _test_print_value_char,                 \
        signed char:        _test_print_value_signed,               \
        short:              _test_print_value_signed,               \
        int:                _test_print_value_signed,               \
        long:               _test_print_value_signed,               \
        long long:          _test_print_value_signed,               \
        unsigned char:      _test_print_value_unsigned,             \
        unsigned short:     _test_print_value_unsigned,             \
        unsigned int:       _test_print_value_unsigned,             \
        unsigned long:      _test_print_value_unsigned,             \
        unsigned long long: _test_print_value_unsigned,             \
        float:              _test_print_value_double,               \
        double:             _test_print_value_double,               \
        long double:        _test_print_value_long_double,          \
        default:            _test_print_value_pointer)(x)

/* --- Checks shared by TEST_ASSERT_* (on_fail does nothing) and
 *     TEST_REQUIRE_* (on_fail returns) --- */

#define _TEST_CHECK(cond, text, on_fail)                            \
    do {                                                            \
        if (!(cond)) {                                              \
            _test_fail(__FILE__, __LINE__, text);                   \
            on_fail;                                                \
        }                                                           \
    } while (0)

/*
 * Each operand is evaluated once into a variable of its own type, which is
 * what gets printed. The comparison converts both to the type `1 ? a : b`
 * has, as `a op b` itself would, so it gives the same result. The casts keep
 * an unsigned operand and a non-negative constant, no longer a constant once
 * copied, from tripping -Wsign-compare. The `a op b` that never runs gives the
 * warnings `a op b` itself would, such as -Wsign-compare for two variables of
 * mixed signedness.
 */
#define _TEST_COMPARE(a, b, op, text, on_fail)                      \
    do {                                                            \
        if (0) (void)((a) op (b));                                  \
        typedef __typeof__(1 ? (a) : (b)) _test_common;             \
        __auto_type _test_a = (a);                                  \
        __auto_type _test_b = (b);                                  \
        if (!((_test_common)_test_a op (_test_common)_test_b)) {    \
            _test_fail_start(__FILE__, __LINE__, text);             \
            _test_print_text(" (");                                 \
            _TEST_PRINT_VALUE(_test_a);                             \
            _test_print_text(" vs ");                               \
            _TEST_PRINT_VALUE(_test_b);                             \
            _test_print_text(")\n");                                \
            on_fail;                                                \
        }                                                           \
    } while (0)

#define _TEST_CHECK_RESULT(type, result, ok, text, on_fail)         \
    do {                                                            \
        type _r = (result);                                         \
        _TEST_CHECK((_r.error == 0) == (ok), text, on_fail);        \
    } while (0)

/* --- Assertion macros: on failure, the test goes on --- */

#define TEST_ASSERT(expr)                                           \
    _TEST_CHECK(expr, #expr, (void)0)

#define TEST_ASSERT_EQ(a, b)                                        \
    _TEST_COMPARE(a, b, ==, #a " == " #b, (void)0)

#define TEST_ASSERT_NEQ(a, b)                                       \
    _TEST_COMPARE(a, b, !=, #a " != " #b, (void)0)

#define TEST_ASSERT_NULL(ptr)                                       \
    _TEST_CHECK((ptr) == NULL, #ptr " == NULL", (void)0)

#define TEST_ASSERT_NOT_NULL(ptr)                                   \
    _TEST_CHECK((ptr) != NULL, #ptr " != NULL", (void)0)

#define TEST_ASSERT_STR_EQ(a, a_len, b, b_len)                      \
    _TEST_CHECK(lc_string_equal((a), (a_len), (b), (b_len)),        \
                "string_equal(" #a ", " #b ")", (void)0)

/* --- Result assertion macros --- */

#define TEST_ASSERT_OK(result)                                      \
    _TEST_CHECK_RESULT(lc_result, result, true, #result " is ok", (void)0)

#define TEST_ASSERT_ERR(result)                                     \
    _TEST_CHECK_RESULT(lc_result, result, false, #result " is err", (void)0)

#define TEST_ASSERT_PTR_OK(result)                                  \
    _TEST_CHECK_RESULT(lc_result_ptr, result, true, #result " is ok", (void)0)

#define TEST_ASSERT_PTR_ERR(result)                                 \
    _TEST_CHECK_RESULT(lc_result_ptr, result, false, #result " is err", (void)0)

/* --- Requirement macros: on failure, return from the current function --- */

#define TEST_REQUIRE(expr)                                          \
    _TEST_CHECK(expr, #expr, return)

#define TEST_REQUIRE_EQ(a, b)                                       \
    _TEST_COMPARE(a, b, ==, #a " == " #b, return)

#define TEST_REQUIRE_NEQ(a, b)                                      \
    _TEST_COMPARE(a, b, !=, #a " != " #b, return)

#define TEST_REQUIRE_NULL(ptr)                                      \
    _TEST_CHECK((ptr) == NULL, #ptr " == NULL", return)

#define TEST_REQUIRE_NOT_NULL(ptr)                                  \
    _TEST_CHECK((ptr) != NULL, #ptr " != NULL", return)

#define TEST_REQUIRE_STR_EQ(a, a_len, b, b_len)                     \
    _TEST_CHECK(lc_string_equal((a), (a_len), (b), (b_len)),        \
                "string_equal(" #a ", " #b ")", return)

#define TEST_REQUIRE_OK(result)                                     \
    _TEST_CHECK_RESULT(lc_result, result, true, #result " is ok", return)

#define TEST_REQUIRE_ERR(result)                                    \
    _TEST_CHECK_RESULT(lc_result, result, false, #result " is err", return)

#define TEST_REQUIRE_PTR_OK(result)                                 \
    _TEST_CHECK_RESULT(lc_result_ptr, result, true, #result " is ok", return)

#define TEST_REQUIRE_PTR_ERR(result)                                \
    _TEST_CHECK_RESULT(lc_result_ptr, result, false, #result " is err", return)

/* --- Skipping: print the reason and return from the current function --- */

#define TEST_SKIP(reason)                                           \
    do {                                                            \
        _test_skip(reason);                                         \
        return;                                                     \
    } while (0)

/* --- Test runner --- */

/* Start a run: clear the counts and take the names of the tests to run from
 * argv[1..argc-1]. Without it, or with no names, every test runs. */
static inline void test_init(int argc, char **argv) {
    _test_total   = 0;
    _test_passed  = 0;
    _test_failed  = 0;
    _test_skipped = 0;
    _test_names      = argv + 1;
    _test_name_count = argc > 1 ? argc - 1 : 0;
    for (int i = 0; i < TEST_MAX_NAMES; i++) _test_name_matched[i] = false;
}

#define TEST_RUN(test_fn)                                           \
    do {                                                            \
        if (!_test_selected(#test_fn)) break;                       \
        _test_current_failed = 0;                                   \
        _test_current_skipped = 0;                                  \
        _test_total++;                                              \
        _test_print_text("RUN  ");                                  \
        _test_print_text(#test_fn);                                 \
        lc_print_newline(TEST_FD);                                  \
        test_fn();                                                  \
        if (_test_current_failed > 0) {                             \
            _test_failed++;                                         \
        } else if (_test_current_skipped) {                         \
            _test_skipped++;                                        \
        } else {                                                    \
            _test_passed++;                                         \
            _test_print_text("  OK\n");                             \
        }                                                           \
    } while (0)

/* --- Summary --- */

static inline int test_main(void) {
    /* A misspelt name must not pass by running nothing. */
    int unknown_names = 0;
    for (int i = 0; i < _test_name_count; i++) {
        if (i >= TEST_MAX_NAMES) {
            _test_print_text("Too many test names, at most ");
            lc_print_signed(TEST_FD, TEST_MAX_NAMES);
            _test_print_text(": ");
        } else if (!_test_name_matched[i]) {
            _test_print_text("No test named ");
        } else {
            continue;
        }
        _test_print_text(_test_names[i]);
        lc_print_newline(TEST_FD);
        unknown_names++;
    }

    lc_print_newline(TEST_FD);
    _test_print_text("========================================\n");
    _test_print_text("Tests run:     ");
    lc_print_signed(TEST_FD, _test_total);
    lc_print_newline(TEST_FD);
    _test_print_text("Tests passed:  ");
    lc_print_signed(TEST_FD, _test_passed);
    lc_print_newline(TEST_FD);
    _test_print_text("Tests failed:  ");
    lc_print_signed(TEST_FD, _test_failed);
    lc_print_newline(TEST_FD);
    _test_print_text("Tests skipped: ");
    lc_print_signed(TEST_FD, _test_skipped);
    lc_print_newline(TEST_FD);
    _test_print_text("========================================\n");

    if (_test_failed > 0 || unknown_names > 0) {
        _test_print_text("RESULT: FAIL\n");
        return 1;
    }
    _test_print_text("RESULT: PASS\n");
    return 0;
}

#endif /* LIGHTC_TEST_H */

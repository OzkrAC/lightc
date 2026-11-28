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
 * TEST_ASSERT_EXACT, TEST_ASSERT_ULP and TEST_ASSERT_NEAR compare two floats
 * or two doubles: bit for bit, by ulp distance, or within a tolerance. Their
 * _ARRAY forms compare two arrays element by element and report the worst
 * element. The comment above them says what each holds for and prints.
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

/* --- Floating-point assertions ---
 *
 * TEST_ASSERT_EXACT(a, b), TEST_ASSERT_ULP(a, b, n) and
 * TEST_ASSERT_NEAR(a, b, abs, rel) compare two floats or two doubles. a and b
 * have the same type, float or double, or the check does not compile, so a
 * float is never compared with a double, nor with an int: write 1.0f, not 1.0
 * or 1, beside a float. Each argument is evaluated once. Each has its
 * TEST_REQUIRE_* twin, as the assertions above do.
 *
 * - TEST_ASSERT_EXACT holds when a and b have the same bits, or are both
 *   NaNs, whatever the sign and payload of each. +0 and -0 differ.
 * - TEST_ASSERT_ULP holds when a and b are at most n ulps apart, by the
 *   distance below. n is a whole number of ulps, 0 or more.
 * - TEST_ASSERT_NEAR holds when |a - b| <= max(abs, rel * max(|a|, |b|)),
 *   computed in double. Pass 0 for the tolerance not wanted: (a, b, 1e-6, 0)
 *   is an absolute bound and (a, b, 0, 1e-6) a relative one. abs and rel are
 *   converted to double, and are 0 or more. Two NaNs are near, and a NaN and
 *   a number are not. An infinity is near only the same infinity, whatever
 *   the tolerances.
 *
 * The ulp distance of a and b is the number of steps from one to the other
 * through the finite values of their type, in order from the most negative
 * to the most positive, with +0 and -0 as one value:
 * - two values of the same sign are as many ulps apart as their bits differ
 *   by, without the sign bit. So neighbours are 1 apart, subnormals too, and
 *   so are the largest subnormal and the smallest normal value;
 * - across zero, the distance is the sum of the distances of each from zero:
 *   the smallest subnormals of the two signs are 2 apart;
 * - +0 and -0 are 0 apart, so TEST_ASSERT_ULP(a, b, 0) holds for them, while
 *   TEST_ASSERT_EXACT tells them apart;
 * - an infinity is 0 from the same infinity, and two NaNs are 0 apart;
 * - an infinity has no distance from any other value, not even from the
 *   largest finite one, nor has a NaN from a value that is not a NaN, so
 *   TEST_ASSERT_ULP fails for them whatever n is: a result that should be an
 *   infinity passes only when it is that infinity.
 *
 * TEST_ASSERT_EXACT_ARRAY(a, b, count), TEST_ASSERT_ULP_ARRAY(a, b, count, n)
 * and TEST_ASSERT_NEAR_ARRAY(a, b, count, abs, rel) check a[i] against b[i]
 * as the forms above do, for each i below count. a and b point to elements
 * of the same type, float or double. With a count of 0 they hold, and read
 * neither array.
 *
 * A failure prints one FAIL line. It gives each value as a decimal of 9
 * significant digits, which is enough to tell floats apart, and its bits in
 * hex, which tell doubles apart too; then how far apart the two are: their
 * ulp distance, or the error |a - b| and the bound. A failed array form also
 * gives how many elements failed, the index of the first, and the index and
 * values of the worst over the whole array:
 * - for EXACT and ULP, the one with the largest ulp distance;
 * - for NEAR, the one whose error is the largest multiple of its bound, and
 *   of two that are the same multiple, such as two errors over a bound of
 *   0, the one with the larger error;
 * - a NaN against a value that is not a NaN, and an infinity against another
 *   value, are worse than any distance or error;
 * - of equally bad elements, the first.
 *
 * Whether a value is a NaN or an infinity is read from its bits, so these
 * checks still tell them from other values under -ffinite-math-only, which
 * -ffast-math turns on.
 */

/* A float or double operand, taken apart. */
typedef struct {
    double   value;         /* the operand; a double holds a float exactly */
    uint64_t bits;          /* its bits */
    uint64_t magnitude;     /* its bits without the sign bit */
    bool     negative;      /* its sign bit is set */
    bool     nan;
    bool     infinite;
    int      hex_digits;    /* to print its bits: 8 for a float, 16 for a double */
} _test_fp;

static inline _test_fp _test_fp_float(float x) {
    union { float f; uint32_t u; } pun = { .f = x };
    uint32_t magnitude = pun.u & 0x7fffffffu;
    return (_test_fp){
        .value      = (double)x,
        .bits       = pun.u,
        .magnitude  = magnitude,
        .negative   = (pun.u >> 31) != 0,
        .nan        = magnitude > 0x7f800000u,
        .infinite   = magnitude == 0x7f800000u,
        .hex_digits = 8,
    };
}

static inline _test_fp _test_fp_double(double x) {
    union { double d; uint64_t u; } pun = { .d = x };
    uint64_t magnitude = pun.u & 0x7fffffffffffffffull;
    return (_test_fp){
        .value      = x,
        .bits       = pun.u,
        .magnitude  = magnitude,
        .negative   = (pun.u >> 63) != 0,
        .nan        = magnitude > 0x7ff0000000000000ull,
        .infinite   = magnitude == 0x7ff0000000000000ull,
        .hex_digits = 16,
    };
}

/* Element i of an array of floats (size 4) or of doubles (size 8). */
static inline _test_fp _test_fp_at(const void *array, size_t i, size_t size) {
    if (size == sizeof(double)) return _test_fp_double(((const double *)array)[i]);
    return _test_fp_float(((const float *)array)[i]);
}

enum { _TEST_FP_EXACT, _TEST_FP_ULP, _TEST_FP_NEAR };

/* What a check compares, and its bound. */
typedef struct {
    int      how;           /* _TEST_FP_EXACT, _TEST_FP_ULP or _TEST_FP_NEAR */
    uint64_t ulps;          /* _TEST_FP_ULP: the most ulps apart allowed */
    double   abs_tolerance; /* _TEST_FP_NEAR: abs and rel */
    double   rel_tolerance;
} _test_fp_bound;

static inline _test_fp_bound _test_fp_bound_exact(void) {
    return (_test_fp_bound){ .how = _TEST_FP_EXACT };
}

static inline _test_fp_bound _test_fp_bound_ulp(uint64_t ulps) {
    return (_test_fp_bound){ .how = _TEST_FP_ULP, .ulps = ulps };
}

static inline _test_fp_bound _test_fp_bound_near(double abs_tolerance, double rel_tolerance) {
    return (_test_fp_bound){
        .how = _TEST_FP_NEAR, .abs_tolerance = abs_tolerance, .rel_tolerance = rel_tolerance,
    };
}

/* Why two values fail whatever the bound. */
enum { _TEST_FP_NAN_ALONE = 1, _TEST_FP_INFINITY_ALONE };

/* What a check found of two values. */
typedef struct {
    bool     ok;
    int      special;   /* 0, or why they fail whatever the bound */
    uint64_t ulps;      /* exact and ulp: the ulp distance, when special is 0 */
    double   error;     /* near: |a - b| and the most allowed, when special is 0 */
    double   bound;
} _test_fp_result;

static inline _test_fp_result _test_fp_check(_test_fp_bound bound, _test_fp a, _test_fp b) {
    _test_fp_result r = { .ok = false };
    if (a.nan || b.nan) {
        r.ok = a.nan && b.nan;
        if (!r.ok) r.special = _TEST_FP_NAN_ALONE;
        return r;
    }
    if (a.infinite || b.infinite) {
        r.ok = a.bits == b.bits;
        if (!r.ok) r.special = _TEST_FP_INFINITY_ALONE;
        return r;
    }
    if (bound.how == _TEST_FP_NEAR) {
        double abs_a = __builtin_fabs(a.value);
        double abs_b = __builtin_fabs(b.value);
        r.error = __builtin_fabs(a.value - b.value);
        r.bound = bound.rel_tolerance * (abs_a > abs_b ? abs_a : abs_b);
        if (bound.abs_tolerance > r.bound) r.bound = bound.abs_tolerance;
        r.ok = r.error <= r.bound;
        return r;
    }
    if (a.negative != b.negative) r.ulps = a.magnitude + b.magnitude;
    else if (a.magnitude > b.magnitude) r.ulps = a.magnitude - b.magnitude;
    else r.ulps = b.magnitude - a.magnitude;
    r.ok = bound.how == _TEST_FP_EXACT ? a.bits == b.bits : r.ulps <= bound.ulps;
    return r;
}

/* Whether failure r is worse than failure worst, by the order above. */
static inline bool _test_fp_worse(_test_fp_bound bound, _test_fp_result r, _test_fp_result worst) {
    if (worst.special != 0) return false;
    if (r.special != 0) return true;
    if (bound.how != _TEST_FP_NEAR) return r.ulps > worst.ulps;
    double times = r.error / r.bound;   /* a bound of 0 gives infinity */
    double worst_times = worst.error / worst.bound;
    if (times > worst_times) return true;
    if (times < worst_times) return false;
    return r.error > worst.error;
}

/* value * 10^power, by exact powers of ten, each 1e22 at most. */
static inline double _test_scale10(double value, int power) {
    while (power > 22) { value *= 1e22; power -= 22; }
    while (power < -22) { value /= 1e22; power += 22; }
    double ten_to_power = 1.0;
    for (int i = 0; i < (power < 0 ? -power : power); i++) ten_to_power *= 10.0;
    return power < 0 ? value / ten_to_power : value * ten_to_power;
}

/* Print value as d.dddddddde+dd, with 9 significant digits, as printf's %.8e
 * does; nan, inf or -inf for those. It is worked out in double, so the last
 * digit can be one off when the value is all but halfway between two. */
static inline void _test_print_decimal(double value) {
    union { double d; uint64_t u; } pun = { .d = value };
    uint64_t magnitude = pun.u & 0x7fffffffffffffffull;
    if (magnitude > 0x7ff0000000000000ull) {
        _test_print_text("nan");
        return;
    }
    if (pun.u >> 63) {
        _test_print_text("-");
        value = -value;
    }
    if (magnitude == 0x7ff0000000000000ull) {
        _test_print_text("inf");
        return;
    }

    int exponent = 0;
    uint64_t digits = 0;
    if (magnitude != 0) {
        /* A first guess from the binary exponent, then put the value scaled
         * to 9 digits in [99999999.5, 999999999.5), so it rounds to them. */
        exponent = ((int)(magnitude >> 52) - 1023) * 30103 / 100000;
        double scaled = _test_scale10(value, 8 - exponent);
        while (scaled >= 999999999.5) scaled = _test_scale10(value, 8 - ++exponent);
        while (scaled < 99999999.5) scaled = _test_scale10(value, 8 - --exponent);
        digits = (uint64_t)(scaled + 0.5);
    }

    char text[16];  /* d.dddddddde-ddd */
    for (int i = 9; i >= 2; i--) {
        text[i] = (char)('0' + digits % 10);
        digits /= 10;
    }
    text[0] = (char)('0' + digits);
    text[1] = '.';
    text[10] = 'e';
    text[11] = exponent < 0 ? '-' : '+';
    int e = exponent < 0 ? -exponent : exponent;
    size_t length = 12;
    if (e >= 100) text[length++] = (char)('0' + e / 100);
    text[length++] = (char)('0' + e / 10 % 10);
    text[length++] = (char)('0' + e % 10);
    lc_print_string(TEST_FD, text, length);
}

/* Print bits as 0x and `digits` hex digits, leading zeros included. */
static inline void _test_print_bits(uint64_t bits, int digits) {
    char text[18];
    text[0] = '0';
    text[1] = 'x';
    for (int i = 0; i < digits; i++)
        text[2 + i] = "0123456789abcdef"[(bits >> (4 * (digits - 1 - i))) & 0xf];
    lc_print_string(TEST_FD, text, (size_t)(2 + digits));
}

/* Print "A BITS vs B BITS, " and how far apart they are. */
static inline void _test_fp_print_pair(_test_fp_bound bound, _test_fp a, _test_fp b,
                                       _test_fp_result r) {
    _test_print_decimal(a.value);
    _test_print_text(" ");
    _test_print_bits(a.bits, a.hex_digits);
    _test_print_text(" vs ");
    _test_print_decimal(b.value);
    _test_print_text(" ");
    _test_print_bits(b.bits, b.hex_digits);
    _test_print_text(", ");
    if (r.special == _TEST_FP_NAN_ALONE) {
        _test_print_text("only one is a NaN");
    } else if (r.special == _TEST_FP_INFINITY_ALONE) {
        _test_print_text("an infinity matches only itself");
    } else if (bound.how == _TEST_FP_NEAR) {
        _test_print_text("error ");
        _test_print_decimal(r.error);
        _test_print_text(", bound ");
        _test_print_decimal(r.bound);
    } else {
        lc_print_unsigned(TEST_FD, r.ulps);
        _test_print_text(" ulp apart");
        if (bound.how == _TEST_FP_ULP) {
            _test_print_text(", bound ");
            lc_print_unsigned(TEST_FD, bound.ulps);
        }
    }
}

/* Check a against b; on failure, print the FAIL line. Returns whether it held. */
static inline bool _test_fp_assert(const char *file, int line, const char *text,
                                   _test_fp_bound bound, _test_fp a, _test_fp b) {
    _test_fp_result r = _test_fp_check(bound, a, b);
    if (r.ok) return true;
    _test_fail_start(file, line, text);
    _test_print_text(" (");
    _test_fp_print_pair(bound, a, b, r);
    _test_print_text(")\n");
    return false;
}

/* Check a[i] against b[i] for each i below count; on failure, print the FAIL
 * line with the worst element. size is that of an element: 4 or 8. */
static inline bool _test_fp_assert_array(const char *file, int line, const char *text,
                                         _test_fp_bound bound, const void *a, const void *b,
                                         size_t count, size_t size) {
    size_t failed = 0, first = 0, worst = 0;
    _test_fp worst_a = { 0 }, worst_b = { 0 };
    _test_fp_result worst_r = { 0 };
    for (size_t i = 0; i < count; i++) {
        _test_fp x = _test_fp_at(a, i, size);
        _test_fp y = _test_fp_at(b, i, size);
        _test_fp_result r = _test_fp_check(bound, x, y);
        if (r.ok) continue;
        if (failed == 0) first = i;
        if (failed == 0 || _test_fp_worse(bound, r, worst_r)) {
            worst = i;
            worst_a = x;
            worst_b = y;
            worst_r = r;
        }
        failed++;
    }
    if (failed == 0) return true;
    _test_fail_start(file, line, text);
    _test_print_text(" (");
    lc_print_unsigned(TEST_FD, failed);
    _test_print_text(" of ");
    lc_print_unsigned(TEST_FD, count);
    _test_print_text(" failed, the first at [");
    lc_print_unsigned(TEST_FD, first);
    _test_print_text("], the worst at [");
    lc_print_unsigned(TEST_FD, worst);
    _test_print_text("]: ");
    _test_fp_print_pair(bound, worst_a, worst_b, worst_r);
    _test_print_text(")\n");
    return false;
}

/* The operand taken apart, by its type: float or double, nothing else. */
#define _TEST_FP(x)                                                 \
    _Generic((x), float: _test_fp_float, double: _test_fp_double)(x)

#define _TEST_FP_SIZE(x)                                            \
    _Generic((x), float: sizeof(float), double: sizeof(double))

#define _TEST_FP_SAME_TYPE(a, b)                                    \
    _Static_assert(__builtin_types_compatible_p(__typeof__(a), __typeof__(b)), \
                   "the float assertions compare two floats or two doubles")

#define _TEST_FP_COMPARE(a, b, bound, text, on_fail)                \
    do {                                                            \
        _TEST_FP_SAME_TYPE(a, b);                                   \
        _test_fp _test_fa = _TEST_FP(a);                            \
        _test_fp _test_fb = _TEST_FP(b);                            \
        if (!_test_fp_assert(__FILE__, __LINE__, text, bound,       \
                             _test_fa, _test_fb)) {                 \
            on_fail;                                                \
        }                                                           \
    } while (0)

#define _TEST_FP_COMPARE_ARRAY(a, b, count, bound, text, on_fail)   \
    do {                                                            \
        _TEST_FP_SAME_TYPE((a)[0], (b)[0]);                         \
        if (!_test_fp_assert_array(__FILE__, __LINE__, text, bound, \
                                   (a), (b), (count),               \
                                   _TEST_FP_SIZE((a)[0]))) {        \
            on_fail;                                                \
        }                                                           \
    } while (0)

#define TEST_ASSERT_EXACT(a, b)                                     \
    _TEST_FP_COMPARE(a, b, _test_fp_bound_exact(),                  \
                     "exact(" #a ", " #b ")", (void)0)

#define TEST_ASSERT_ULP(a, b, n)                                    \
    _TEST_FP_COMPARE(a, b, _test_fp_bound_ulp((uint64_t)(n)),       \
                     "ulp(" #a ", " #b ") <= " #n, (void)0)

#define TEST_ASSERT_NEAR(a, b, abs, rel)                            \
    _TEST_FP_COMPARE(a, b, _test_fp_bound_near((double)(abs), (double)(rel)), \
                     "near(" #a ", " #b ", " #abs ", " #rel ")", (void)0)

#define TEST_ASSERT_EXACT_ARRAY(a, b, count)                        \
    _TEST_FP_COMPARE_ARRAY(a, b, count, _test_fp_bound_exact(),     \
                           "exact(" #a "[i], " #b "[i]) for i < " #count, (void)0)

#define TEST_ASSERT_ULP_ARRAY(a, b, count, n)                       \
    _TEST_FP_COMPARE_ARRAY(a, b, count, _test_fp_bound_ulp((uint64_t)(n)), \
                           "ulp(" #a "[i], " #b "[i]) <= " #n " for i < " #count, (void)0)

#define TEST_ASSERT_NEAR_ARRAY(a, b, count, abs, rel)               \
    _TEST_FP_COMPARE_ARRAY(a, b, count, _test_fp_bound_near((double)(abs), (double)(rel)), \
                           "near(" #a "[i], " #b "[i], " #abs ", " #rel ") for i < " #count, \
                           (void)0)

#define TEST_REQUIRE_EXACT(a, b)                                    \
    _TEST_FP_COMPARE(a, b, _test_fp_bound_exact(),                  \
                     "exact(" #a ", " #b ")", return)

#define TEST_REQUIRE_ULP(a, b, n)                                   \
    _TEST_FP_COMPARE(a, b, _test_fp_bound_ulp((uint64_t)(n)),       \
                     "ulp(" #a ", " #b ") <= " #n, return)

#define TEST_REQUIRE_NEAR(a, b, abs, rel)                           \
    _TEST_FP_COMPARE(a, b, _test_fp_bound_near((double)(abs), (double)(rel)), \
                     "near(" #a ", " #b ", " #abs ", " #rel ")", return)

#define TEST_REQUIRE_EXACT_ARRAY(a, b, count)                       \
    _TEST_FP_COMPARE_ARRAY(a, b, count, _test_fp_bound_exact(),     \
                           "exact(" #a "[i], " #b "[i]) for i < " #count, return)

#define TEST_REQUIRE_ULP_ARRAY(a, b, count, n)                      \
    _TEST_FP_COMPARE_ARRAY(a, b, count, _test_fp_bound_ulp((uint64_t)(n)), \
                           "ulp(" #a "[i], " #b "[i]) <= " #n " for i < " #count, return)

#define TEST_REQUIRE_NEAR_ARRAY(a, b, count, abs, rel)              \
    _TEST_FP_COMPARE_ARRAY(a, b, count, _test_fp_bound_near((double)(abs), (double)(rel)), \
                           "near(" #a "[i], " #b "[i], " #abs ", " #rel ") for i < " #count, \
                           return)

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

/* --- Chaos points, in the faultinject build ---
 *
 * test_chaos_init(envp) seeds lightc's chaos points (<lightc/fault.h>) with
 * the value of LC_CHAOS in `envp`, main's third argument: a decimal number
 * below 2^64. It prints the seed, so a run that fails can be run again with
 * it, as in LC_CHAOS=12345 build/test_fault. Unset, empty or 0 turns the
 * points off. Any other value is a mistake that would quietly test less, so
 * the suite then exits with status 2. In another build it does nothing.
 */

#include <lightc/fault.h>
#include <lightc/syscall.h>

static inline void test_chaos_init(char **envp) {
#ifdef LC_FAULTINJECT
    static const char name[] = "LC_CHAOS=";
    const size_t name_length = sizeof name - 1;
    const char *value = NULL;
    for (char **e = envp; e != NULL && *e != NULL; e++) {
        if (lc_string_length(*e) >= name_length &&
            lc_string_equal(*e, name_length, name, name_length)) {
            value = *e + name_length;
            break;
        }
    }
    uint64_t seed = 0;
    for (const char *c = value; c != NULL && *c != '\0'; c++) {
        uint64_t digit = (uint64_t)(*c - '0');
        if (*c < '0' || *c > '9' || seed > (UINT64_MAX - digit) / 10) {
            _test_print_text("LC_CHAOS=");
            _test_print_text(value);
            _test_print_text(": not a decimal number below 2^64\n");
            lc_kernel_exit(2);
        }
        seed = seed * 10 + digit;
    }
    lc_chaos_seed(seed);
    if (seed != 0) {
        _test_print_text("Chaos points seeded with LC_CHAOS=");
        lc_print_unsigned(TEST_FD, seed);
        lc_print_newline(TEST_FD);
    }
#else
    (void)envp;
#endif
}

#endif /* LIGHTC_TEST_H */

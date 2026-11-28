/*
 * math_cases.h — a math suite's hard cases, checked in both forms of each
 * function.
 *
 * Each math function of <lightc/math.h> has two forms: the lc_ one, which
 * a program calls, inline over the GCC builtin, and the one of the
 * standard name, such as floorf, which liblightc.a defines for the calls
 * GCC makes when it cannot inline the builtin (decision N-a). A suite of
 * such functions, as tests/test_math_exact.c is, defines before it
 * includes this:
 *
 *   MATH_FUNCTIONS(X)  its functions, X(KIND, NAME, ULPS, RULE), as the
 *                      table of tools/math-sweep.c lists them;
 *   MATH_CASES_FILE    its case file, as a string, such as
 *                      "data/math_exact_cases.h", which
 *                      "build/math-sweep -c NAME..." writes;
 *
 * and declares the standard name of each function, as no lightc header
 * does; -ffreestanding implies -fno-builtin, so GCC takes each for a plain
 * function. This then defines test_NAME for each function, which checks
 * NAME and lc_NAME on every case of NAME in the case file, and fails when
 * there is none; MATH_RUN_TESTS() runs them all, in the order of
 * MATH_FUNCTIONS.
 *
 * A case passes when the result is at most ULPS ulps from the expected
 * one, by the distance of tests/test.h's TEST_ASSERT_ULP: so with the same
 * bits for 0. An expected NaN is matched by any quiet NaN, but under the
 * rule NAN_BITS, which only its own bits match. The other rules, MIN, MAX
 * and SATURATE, are the sweep's: the case file already holds lightc's
 * result where they stand for the oracle's. A failure prints the form, the
 * arguments' bits, the result and the expected one, the first few of each
 * test.
 *
 * KIND is the signature: F_F float (float), D_D double (double), L_F long
 * (float), F_FE float (float, int *), F_FI float (float, int), F_FF float
 * (float, float), D_DD double (double, double), F_FFF float (float, float,
 * float), D_DDD double (double, double, double).
 */

#ifndef LIGHTC_TESTS_MATH_CASES_H
#define LIGHTC_TESTS_MATH_CASES_H

#include "test.h"
#include <lightc/math.h>

#if !defined(MATH_FUNCTIONS) || !defined(MATH_CASES_FILE)
#error "define MATH_FUNCTIONS and MATH_CASES_FILE before including math_cases.h"
#endif

enum { NAN_QUIET, NAN_BITS, MIN, MAX, SATURATE };

/* --- The lc_ forms, as functions --- */

#define MATH_FORM_F_F(name)   [[maybe_unused]] static float lc_form_##name(float x) { return lc_##name(x); }
#define MATH_FORM_D_D(name)   [[maybe_unused]] static double lc_form_##name(double x) { return lc_##name(x); }
#define MATH_FORM_L_F(name)   [[maybe_unused]] static long lc_form_##name(float x) { return lc_##name(x); }
#define MATH_FORM_F_FE(name)  [[maybe_unused]] static float lc_form_##name(float x, int *e) { return lc_##name(x, e); }
#define MATH_FORM_F_FI(name)  [[maybe_unused]] static float lc_form_##name(float x, int n) { return lc_##name(x, n); }
#define MATH_FORM_F_FF(name)  [[maybe_unused]] static float lc_form_##name(float x, float y) { return lc_##name(x, y); }
#define MATH_FORM_D_DD(name)  [[maybe_unused]] static double lc_form_##name(double x, double y) { return lc_##name(x, y); }
#define MATH_FORM_F_FFF(name) [[maybe_unused]] static float lc_form_##name(float x, float y, float z) { \
                                  return lc_##name(x, y, z); }
#define MATH_FORM_D_DDD(name) [[maybe_unused]] static double lc_form_##name(double x, double y, double z) { \
                                  return lc_##name(x, y, z); }
#define MATH_FORM(kind, name, ulps, rule) MATH_FORM_##kind(name)
MATH_FUNCTIONS(MATH_FORM)

/* Each function's bound and rule, as enum constants. */
#define MATH_BOUND(kind, name, ulps, rule) math_ulps_##name = ulps, math_rule_##name = rule,
enum { MATH_FUNCTIONS(MATH_BOUND) };

/* --- The cases --- */

/* What every case has besides its arguments and result: the function in
 * each form, by the pointer that picks its cases, its bound and its rule.
 * Each array ends with a case whose lib is NULL. */
#define MATH_CASE_HEAD(name) .lib = name, .lc = lc_form_##name, .ulps = math_ulps_##name, \
                             .rule = math_rule_##name

typedef struct { float (*lib)(float); float (*lc)(float); int ulps, rule; uint32_t x, r; } math_case_f_f;
typedef struct { double (*lib)(double); double (*lc)(double); int ulps, rule; uint64_t x, r; } math_case_d_d;
typedef struct { long (*lib)(float); long (*lc)(float); int ulps, rule; uint32_t x; long r; } math_case_l_f;
typedef struct {
    float (*lib)(float, int *); float (*lc)(float, int *); int ulps, rule; uint32_t x, r; int e;
} math_case_f_fe;
typedef struct {
    float (*lib)(float, int); float (*lc)(float, int); int ulps, rule; uint32_t x; int n; uint32_t r;
} math_case_f_fi;
typedef struct {
    float (*lib)(float, float); float (*lc)(float, float); int ulps, rule; uint32_t x, y, r;
} math_case_f_ff;
typedef struct {
    double (*lib)(double, double); double (*lc)(double, double); int ulps, rule; uint64_t x, y, r;
} math_case_d_dd;
typedef struct {
    float (*lib)(float, float, float); float (*lc)(float, float, float); int ulps, rule;
    uint32_t x, y, z, r;
} math_case_f_fff;
typedef struct {
    double (*lib)(double, double, double); double (*lc)(double, double, double); int ulps, rule;
    uint64_t x, y, z, r;
} math_case_d_ddd;

/* The case file is read once per kind, with only that kind's macro making
 * an element. */
#define CASE_F_F(...)
#define CASE_D_D(...)
#define CASE_L_F(...)
#define CASE_F_FE(...)
#define CASE_F_FI(...)
#define CASE_F_FF(...)
#define CASE_D_DD(...)
#define CASE_F_FFF(...)
#define CASE_D_DDD(...)

#undef CASE_F_F
#define CASE_F_F(name, x_, r_) { MATH_CASE_HEAD(name), .x = x_, .r = r_ },
static const math_case_f_f math_f_f_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_F_F
#define CASE_F_F(...)

#undef CASE_D_D
#define CASE_D_D(name, x_, r_) { MATH_CASE_HEAD(name), .x = x_, .r = r_ },
static const math_case_d_d math_d_d_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_D_D
#define CASE_D_D(...)

#undef CASE_L_F
#define CASE_L_F(name, x_, r_) { MATH_CASE_HEAD(name), .x = x_, .r = r_ },
static const math_case_l_f math_l_f_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_L_F
#define CASE_L_F(...)

#undef CASE_F_FE
#define CASE_F_FE(name, x_, r_, e_) { MATH_CASE_HEAD(name), .x = x_, .r = r_, .e = e_ },
static const math_case_f_fe math_f_fe_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_F_FE
#define CASE_F_FE(...)

#undef CASE_F_FI
#define CASE_F_FI(name, x_, n_, r_) { MATH_CASE_HEAD(name), .x = x_, .n = n_, .r = r_ },
static const math_case_f_fi math_f_fi_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_F_FI
#define CASE_F_FI(...)

#undef CASE_F_FF
#define CASE_F_FF(name, x_, y_, r_) { MATH_CASE_HEAD(name), .x = x_, .y = y_, .r = r_ },
static const math_case_f_ff math_f_ff_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_F_FF
#define CASE_F_FF(...)

#undef CASE_D_DD
#define CASE_D_DD(name, x_, y_, r_) { MATH_CASE_HEAD(name), .x = x_, .y = y_, .r = r_ },
static const math_case_d_dd math_d_dd_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_D_DD
#define CASE_D_DD(...)

#undef CASE_F_FFF
#define CASE_F_FFF(name, x_, y_, z_, r_) { MATH_CASE_HEAD(name), .x = x_, .y = y_, .z = z_, .r = r_ },
static const math_case_f_fff math_f_fff_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_F_FFF
#define CASE_F_FFF(...)

#undef CASE_D_DDD
#define CASE_D_DDD(name, x_, y_, z_, r_) { MATH_CASE_HEAD(name), .x = x_, .y = y_, .z = z_, .r = r_ },
static const math_case_d_ddd math_d_ddd_cases[] = {
#include MATH_CASES_FILE
    { .lib = NULL },
};
#undef CASE_D_DDD
#define CASE_D_DDD(...)

/* --- Checking a result --- */

/* Whether `got` passes for `want` by the bound and the rule. */
static inline bool math_float_passes(int ulps, int rule, uint32_t got, uint32_t want) {
    uint32_t ag = got & 0x7fffffffu, aw = want & 0x7fffffffu;
    if (rule == NAN_BITS) return got == want;
    if (ag > 0x7f800000u || aw > 0x7f800000u) return ag > 0x7f800000u && aw > 0x7f800000u && (got & 0x00400000u);
    if (got == want) return true;
    if (ag == 0x7f800000u || aw == 0x7f800000u) return false;
    uint64_t distance = (got >> 31) != (want >> 31) ? (uint64_t)ag + aw : ag > aw ? ag - aw : aw - ag;
    return ulps > 0 && distance <= (uint64_t)ulps;    /* ±0 are 0 apart, but not the same bits */
}

static inline bool math_double_passes(int ulps, int rule, uint64_t got, uint64_t want) {
    uint64_t ag = got & 0x7fffffffffffffffu, aw = want & 0x7fffffffffffffffu;
    if (rule == NAN_BITS) return got == want;
    if (ag > 0x7ff0000000000000u || aw > 0x7ff0000000000000u)
        return ag > 0x7ff0000000000000u && aw > 0x7ff0000000000000u && (got & 0x0008000000000000u);
    if (got == want) return true;
    if (ag == 0x7ff0000000000000u || aw == 0x7ff0000000000000u) return false;
    uint64_t distance = (got >> 63) != (want >> 63) ? ag + aw : ag > aw ? ag - aw : aw - ag;
    return ulps > 0 && distance <= (uint64_t)ulps;
}

/* At most this many failed cases are printed for each test. */
#define MATH_SHOWN 8
static int math_shown;

/* Prints a failed case: the form, its arguments' bits, what it gave and
 * what it should have given. */
static void math_print_failure(const char *name, bool lc, const uint64_t *in, int count,
                               uint64_t got, uint64_t want) {
    if (math_shown++ >= MATH_SHOWN) return;
    _test_print_text(lc ? "  lc_" : "  ");
    _test_print_text(name);
    _test_print_text("(");
    for (int i = 0; i < count; i++) {
        if (i) _test_print_text(", ");
        lc_print_hex(TEST_FD, in[i]);
    }
    _test_print_text(") gave ");
    lc_print_hex(TEST_FD, got);
    _test_print_text(", want ");
    lc_print_hex(TEST_FD, want);
    lc_print_newline(TEST_FD);
}

/* Checks a case's two results, given as bits, and prints those that fail. */
static void math_check_float(const char *name, const uint64_t *in, int count, int ulps, int rule,
                             uint32_t a, uint32_t b, uint32_t want) {
    bool a_ok = math_float_passes(ulps, rule, a, want), b_ok = math_float_passes(ulps, rule, b, want);
    if (!a_ok) math_print_failure(name, false, in, count, a, want);
    if (!b_ok) math_print_failure(name, true, in, count, b, want);
    TEST_ASSERT(a_ok && b_ok);
}

static void math_check_double(const char *name, const uint64_t *in, int count, int ulps, int rule,
                              uint64_t a, uint64_t b, uint64_t want) {
    bool a_ok = math_double_passes(ulps, rule, a, want), b_ok = math_double_passes(ulps, rule, b, want);
    if (!a_ok) math_print_failure(name, false, in, count, a, want);
    if (!b_ok) math_print_failure(name, true, in, count, b, want);
    TEST_ASSERT(a_ok && b_ok);
}

/*
 * Each runs every case of the function `lib` of its kind, in both forms,
 * reading each argument through a volatile so that GCC cannot fold the
 * call, and fails when there is no case, so that a case file without the
 * function's cases fails too.
 */

[[maybe_unused]] static void math_cases_f_f(const char *name, float (*lib)(float)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_f_f *c = math_f_f_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint32_t x = c->x;
        float v = lc_float_from_bits(x);
        uint64_t in[1] = { c->x };
        math_check_float(name, in, 1, c->ulps, c->rule, lc_float_to_bits(c->lib(v)),
                         lc_float_to_bits(c->lc(v)), c->r);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_d_d(const char *name, double (*lib)(double)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_d_d *c = math_d_d_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint64_t x = c->x;
        double v = lc_double_from_bits(x);
        uint64_t in[1] = { c->x };
        math_check_double(name, in, 1, c->ulps, c->rule, lc_double_to_bits(c->lib(v)),
                          lc_double_to_bits(c->lc(v)), c->r);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_l_f(const char *name, long (*lib)(float)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_l_f *c = math_l_f_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint32_t x = c->x;
        float v = lc_float_from_bits(x);
        long a = c->lib(v), b = c->lc(v);
        uint64_t in[1] = { c->x };
        if (a != c->r) math_print_failure(name, false, in, 1, (uint64_t)a, (uint64_t)c->r);
        if (b != c->r) math_print_failure(name, true, in, 1, (uint64_t)b, (uint64_t)c->r);
        TEST_ASSERT(a == c->r && b == c->r);
    }
    TEST_ASSERT(found > 0);
}

/* The result and the exponent, printed in one word: the exponent above. */
[[maybe_unused]] static void math_cases_f_fe(const char *name, float (*lib)(float, int *)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_f_fe *c = math_f_fe_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint32_t x = c->x;
        float v = lc_float_from_bits(x);
        int ea = 99, eb = 99;
        uint32_t a = lc_float_to_bits(c->lib(v, &ea)), b = lc_float_to_bits(c->lc(v, &eb));
        uint64_t in[1] = { c->x };
        bool a_ok = math_float_passes(c->ulps, c->rule, a, c->r) && ea == c->e;
        bool b_ok = math_float_passes(c->ulps, c->rule, b, c->r) && eb == c->e;
        uint64_t want = (uint64_t)c->r | (uint64_t)(uint32_t)c->e << 32;
        if (!a_ok) math_print_failure(name, false, in, 1, (uint64_t)a | (uint64_t)(uint32_t)ea << 32, want);
        if (!b_ok) math_print_failure(name, true, in, 1, (uint64_t)b | (uint64_t)(uint32_t)eb << 32, want);
        TEST_ASSERT(a_ok && b_ok);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_f_fi(const char *name, float (*lib)(float, int)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_f_fi *c = math_f_fi_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint32_t x = c->x;
        volatile int n = c->n;
        float v = lc_float_from_bits(x);
        uint64_t in[2] = { c->x, (uint64_t)(int64_t)c->n };
        math_check_float(name, in, 2, c->ulps, c->rule, lc_float_to_bits(c->lib(v, n)),
                         lc_float_to_bits(c->lc(v, n)), c->r);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_f_ff(const char *name, float (*lib)(float, float)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_f_ff *c = math_f_ff_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint32_t x = c->x, y = c->y;
        float v = lc_float_from_bits(x), w = lc_float_from_bits(y);
        uint64_t in[2] = { c->x, c->y };
        math_check_float(name, in, 2, c->ulps, c->rule, lc_float_to_bits(c->lib(v, w)),
                         lc_float_to_bits(c->lc(v, w)), c->r);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_d_dd(const char *name, double (*lib)(double, double)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_d_dd *c = math_d_dd_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint64_t x = c->x, y = c->y;
        double v = lc_double_from_bits(x), w = lc_double_from_bits(y);
        uint64_t in[2] = { c->x, c->y };
        math_check_double(name, in, 2, c->ulps, c->rule, lc_double_to_bits(c->lib(v, w)),
                          lc_double_to_bits(c->lc(v, w)), c->r);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_f_fff(const char *name, float (*lib)(float, float, float)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_f_fff *c = math_f_fff_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint32_t x = c->x, y = c->y, z = c->z;
        float u = lc_float_from_bits(x), v = lc_float_from_bits(y), w = lc_float_from_bits(z);
        uint64_t in[3] = { c->x, c->y, c->z };
        math_check_float(name, in, 3, c->ulps, c->rule, lc_float_to_bits(c->lib(u, v, w)),
                         lc_float_to_bits(c->lc(u, v, w)), c->r);
    }
    TEST_ASSERT(found > 0);
}

[[maybe_unused]] static void math_cases_d_ddd(const char *name, double (*lib)(double, double, double)) {
    int found = 0;
    math_shown = 0;
    for (const math_case_d_ddd *c = math_d_ddd_cases; c->lib; c++) {
        if (c->lib != lib) continue;
        found++;
        volatile uint64_t x = c->x, y = c->y, z = c->z;
        double u = lc_double_from_bits(x), v = lc_double_from_bits(y), w = lc_double_from_bits(z);
        uint64_t in[3] = { c->x, c->y, c->z };
        math_check_double(name, in, 3, c->ulps, c->rule, lc_double_to_bits(c->lib(u, v, w)),
                          lc_double_to_bits(c->lc(u, v, w)), c->r);
    }
    TEST_ASSERT(found > 0);
}

/* --- One test per function: test_floorf checks floorf and lc_floorf --- */

#define MATH_TEST(kind, name, ulps, rule) \
    static void test_##name(void) { math_cases_##kind(#name, name); }
#define math_cases_F_F   math_cases_f_f
#define math_cases_D_D   math_cases_d_d
#define math_cases_L_F   math_cases_l_f
#define math_cases_F_FE  math_cases_f_fe
#define math_cases_F_FI  math_cases_f_fi
#define math_cases_F_FF  math_cases_f_ff
#define math_cases_D_DD  math_cases_d_dd
#define math_cases_F_FFF math_cases_f_fff
#define math_cases_D_DDD math_cases_d_ddd
MATH_FUNCTIONS(MATH_TEST)

#define MATH_RUN(kind, name, ulps, rule) TEST_RUN(test_##name);
#define MATH_RUN_TESTS() MATH_FUNCTIONS(MATH_RUN)

#endif /* LIGHTC_TESTS_MATH_CASES_H */

/*
 * test_math_exact — the exact functions of <lightc/math.h>, in both their
 * forms, on their hard cases.
 *
 * Each function has two forms: the lc_ one, inline over the GCC builtin,
 * which a program calls, and the one of the standard name, such as floorf,
 * which liblightc.a defines for the calls GCC makes when it cannot inline
 * the builtin (decision N-a). tests/math_cases.h makes a test of each
 * function, test_floorf and the others, which checks both forms on every
 * case of the function in tests/data/math_exact_cases.h. "build/math-sweep
 * -c" wrote that file from the host libm, with lightc's rule where C
 * leaves the result open: the special values, the values on each side of
 * each rounding boundary, the halfway cases, 2^23 and 2^52 and the largest
 * values below them that are not integers, subnormals and their edges, the
 * range of long for lrintf, and for fma the ties that rounding twice gets
 * wrong. Each function here is exact, so a case passes when the result has
 * the expected bits; an expected NaN is matched by any quiet NaN, but for
 * fabsf and copysignf, which must give the NaN's own bits but for the
 * sign. The cases are the same on both arches, so on each this checks the
 * bits that tools/math-sweep.c checks over every float on x86_64.
 *
 * The tests after those check what <lightc/math.h> says lightc gives where
 * C leaves the result open, also on constants, which GCC folds by its own
 * rules.
 */

#include "test.h"
#include <lightc/math.h>

/* The standard names, which no lightc header declares. */
float sqrtf(float);
double sqrt(double);
float floorf(float);
double floor(double);
float ceilf(float);
double ceil(double);
float truncf(float);
double trunc(double);
float roundf(float);
double round(double);
float rintf(float);
double rint(double);
float nearbyintf(float);
double nearbyint(double);
long lrintf(float);
float fmaf(float, float, float);
double fma(double, double, double);
float fminf(float, float);
double fmin(double, double);
float fmaxf(float, float);
double fmax(double, double);
float fabsf(float);
float copysignf(float, float);
float frexpf(float, int *);
float ldexpf(float, int);
float scalbnf(float, int);

/* The functions, X(KIND, NAME, ULPS, RULE), as tools/math-sweep.c lists
 * them, and their cases: tests/math_cases.h makes a test of each. */
#define MATH_FUNCTIONS(X)                   \
    X(F_F,   sqrtf,      0, NAN_QUIET)      \
    X(F_F,   floorf,     0, NAN_QUIET)      \
    X(F_F,   ceilf,      0, NAN_QUIET)      \
    X(F_F,   truncf,     0, NAN_QUIET)      \
    X(F_F,   roundf,     0, NAN_QUIET)      \
    X(F_F,   rintf,      0, NAN_QUIET)      \
    X(F_F,   nearbyintf, 0, NAN_QUIET)      \
    X(F_F,   fabsf,      0, NAN_BITS)       \
    X(L_F,   lrintf,     0, SATURATE)       \
    X(F_FE,  frexpf,     0, NAN_QUIET)      \
    X(F_FI,  ldexpf,     0, NAN_QUIET)      \
    X(F_FI,  scalbnf,    0, NAN_QUIET)      \
    X(F_FF,  copysignf,  0, NAN_BITS)       \
    X(F_FF,  fminf,      0, MIN)            \
    X(F_FF,  fmaxf,      0, MAX)            \
    X(F_FFF, fmaf,       0, NAN_QUIET)      \
    X(D_D,   sqrt,       0, NAN_QUIET)      \
    X(D_D,   floor,      0, NAN_QUIET)      \
    X(D_D,   ceil,       0, NAN_QUIET)      \
    X(D_D,   trunc,      0, NAN_QUIET)      \
    X(D_D,   round,      0, NAN_QUIET)      \
    X(D_D,   rint,       0, NAN_QUIET)      \
    X(D_D,   nearbyint,  0, NAN_QUIET)      \
    X(D_DD,  fmin,       0, MIN)            \
    X(D_DD,  fmax,       0, MAX)            \
    X(D_DDD, fma,        0, NAN_QUIET)
#define MATH_CASES_FILE "data/math_exact_cases.h"
#include "math_cases.h"

#define COUNT(a) (sizeof (a) / sizeof (a)[0])

/* --- Where C leaves the result open --- */

static volatile float  zero_f = 0.0f, negative_zero_f = -0.0f;
static volatile double zero_d = 0.0, negative_zero_d = -0.0;

/* fmin of two zeros is -0 if either is, fmax +0 if either is, in either
 * order, in both forms, and on constants. */
static void test_min_max_of_zeros(void) {
    float pz = zero_f, nz = negative_zero_f;
    TEST_ASSERT_EQ(lc_float_to_bits(lc_fminf(nz, pz)), 0x80000000u);
    TEST_ASSERT_EQ(lc_float_to_bits(lc_fminf(pz, nz)), 0x80000000u);
    TEST_ASSERT_EQ(lc_float_to_bits(lc_fmaxf(nz, pz)), 0u);
    TEST_ASSERT_EQ(lc_float_to_bits(lc_fmaxf(pz, nz)), 0u);
    TEST_ASSERT_EQ(lc_float_to_bits(fminf(pz, nz)), 0x80000000u);
    TEST_ASSERT_EQ(lc_float_to_bits(fmaxf(nz, pz)), 0u);
    TEST_ASSERT_EQ(lc_float_to_bits(lc_fminf(0.0f, -0.0f)), 0x80000000u);
    TEST_ASSERT_EQ(lc_float_to_bits(lc_fmaxf(-0.0f, 0.0f)), 0u);
    double dp = zero_d, dn = negative_zero_d;
    TEST_ASSERT_EQ(lc_double_to_bits(lc_fmin(dp, dn)), 0x8000000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(lc_fmax(dn, dp)), 0u);
    TEST_ASSERT_EQ(lc_double_to_bits(fmin(dn, dp)), 0x8000000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(fmax(dp, dn)), 0u);
    TEST_ASSERT_EQ(lc_double_to_bits(lc_fmin(0.0, -0.0)), 0x8000000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(lc_fmax(-0.0, 0.0)), 0u);
}

/* A NaN and a number give the number; a signalling NaN gives a quiet NaN. */
static void test_min_max_of_nans(void) {
    volatile uint32_t quiet = 0x7fc00000u, signalling = 0x7fa00000u;
    float q = lc_float_from_bits(quiet), s = lc_float_from_bits(signalling), one = zero_f + 1.0f;
    TEST_ASSERT_EXACT(lc_fminf(q, one), 1.0f);
    TEST_ASSERT_EXACT(lc_fmaxf(one, q), 1.0f);
    uint32_t r = lc_float_to_bits(lc_fminf(s, one));
    TEST_ASSERT(lc_float_isnan(lc_float_from_bits(r)) && (r & 0x00400000u));
    r = lc_float_to_bits(fmaxf(one, s));
    TEST_ASSERT(lc_float_isnan(lc_float_from_bits(r)) && (r & 0x00400000u));
    r = lc_float_to_bits(lc_fmaxf(q, s));
    TEST_ASSERT(lc_float_isnan(lc_float_from_bits(r)) && (r & 0x00400000u));
}

static volatile uint32_t lrint_inputs[] = {
    0x7fc00000u, 0xffc00000u, 0x7f800001u, 0x7f800000u, 0xff800000u,
    0x5f000000u, 0xdf000000u, 0x5f000001u, 0xdf000001u, 0x7f7fffffu, 0xff7fffffu,
    0x5effffffu, 0xdeffffffu,
};
static const long lrint_results[] = {
    0, 0, 0, __LONG_MAX__, -__LONG_MAX__ - 1,
    __LONG_MAX__, -__LONG_MAX__ - 1, __LONG_MAX__, -__LONG_MAX__ - 1, __LONG_MAX__, -__LONG_MAX__ - 1,
    0x7fffff8000000000, -0x7fffff8000000000,
};

/* lrintf saturates: 0 for a NaN, LONG_MAX from 2^63 up, LONG_MIN below
 * -2^63, -2^63 itself exactly; in both forms, and on constants. */
static void test_lrintf_saturates(void) {
    for (size_t i = 0; i < COUNT(lrint_inputs); i++) {
        float x = lc_float_from_bits(lrint_inputs[i]);
        TEST_ASSERT_EQ(lc_lrintf(x), lrint_results[i]);
        TEST_ASSERT_EQ(lrintf(x), lrint_results[i]);
    }
    TEST_ASSERT_EQ(lc_lrintf(LC_NAN), 0L);
    TEST_ASSERT_EQ(lc_lrintf(LC_INFINITY), __LONG_MAX__);
    TEST_ASSERT_EQ(lc_lrintf(-LC_INFINITY), -__LONG_MAX__ - 1);
    TEST_ASSERT_EQ(lc_lrintf(1e30f), __LONG_MAX__);
    TEST_ASSERT_EQ(lc_lrintf(-1e30f), -__LONG_MAX__ - 1);
    TEST_ASSERT_EQ(lc_lrintf(-0x1p63f), -__LONG_MAX__ - 1);
    TEST_ASSERT_EQ(lc_lrintf(2.5f), 2L);
    TEST_ASSERT_EQ(lc_lrintf(-3.5f), -4L);
}

/* frexpf stores 0 for ±0, an infinity and a NaN. */
static void test_frexpf_exponent_of_specials(void) {
    static volatile uint32_t inputs[] = { 0x00000000u, 0x80000000u, 0x7f800000u, 0xff800000u,
                                          0x7fc00000u, 0x7fa00000u };
    for (size_t i = 0; i < COUNT(inputs); i++) {
        int a = 99, b = 99;
        float x = lc_float_from_bits(inputs[i]);
        (void)lc_frexpf(x, &a);
        (void)frexpf(x, &b);
        TEST_ASSERT_EQ(a, 0);
        TEST_ASSERT_EQ(b, 0);
    }
}

/* rint and nearbyint are one function: ties to even, both forms. */
static void test_rint_is_nearbyint(void) {
    static volatile float inputs[] = { 0.5f, 1.5f, 2.5f, -0.5f, -2.5f, 8388607.5f, 0.49999997f };
    for (size_t i = 0; i < COUNT(inputs); i++) {
        float x = inputs[i];
        TEST_ASSERT_EXACT(lc_rintf(x), lc_nearbyintf(x));
        TEST_ASSERT_EXACT(rintf(x), nearbyintf(x));
        TEST_ASSERT_EXACT(lc_rint((double)x), lc_nearbyint((double)x));
        TEST_ASSERT_EXACT(rint((double)x), nearbyint((double)x));
    }
    TEST_ASSERT_EXACT(lc_rintf(2.5f), 2.0f);
    TEST_ASSERT_EXACT(lc_rintf(-0.5f), -0.0f);
}

/* A finite product beyond a float's range plus an infinity is that
 * infinity, not a NaN: fma rounds once, at the end. */
static void test_fma_rounds_once(void) {
    volatile float big = 0x1p127f, inf = LC_INFINITY, tiny = 0x1p-149f, three = 3.0f;
    volatile float odd = 8388609.0f;
    TEST_ASSERT_EXACT(lc_fmaf(big, big, -inf), -LC_INFINITY);
    TEST_ASSERT_EXACT(fmaf(big, 2.0f, -big), 0x1p127f);    /* 2^128 - 2^127 */
    /* 3 * (2^23 + 1) is halfway between two floats; the smallest float
     * decides the tie, which rounding the sum to double first loses. */
    TEST_ASSERT_EXACT(lc_fmaf(three, odd, tiny), 25165828.0f);
    TEST_ASSERT_EXACT(fmaf(three, odd, -tiny), 25165826.0f);
    volatile double dbig = 0x1p1023, dinf = LC_INFINITY, dtiny = 0x1p-1074, dthree = 3.0;
    volatile double dodd = 4503599627370497.0;
    TEST_ASSERT_EXACT(lc_fma(dbig, dbig, -dinf), -(double)LC_INFINITY);
    TEST_ASSERT_EXACT(lc_fma(dthree, dodd, dtiny), 13510798882111492.0);
    TEST_ASSERT_EXACT(fma(dthree, dodd, -dtiny), 13510798882111490.0);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    MATH_RUN_TESTS()

    TEST_RUN(test_min_max_of_zeros);
    TEST_RUN(test_min_max_of_nans);
    TEST_RUN(test_lrintf_saturates);
    TEST_RUN(test_frexpf_exponent_of_specials);
    TEST_RUN(test_rint_is_nearbyint);
    TEST_RUN(test_fma_rounds_once);
    return test_main();
}

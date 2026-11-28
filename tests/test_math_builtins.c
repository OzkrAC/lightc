/*
 * test_math_builtins — every math builtin that GCC may turn into a call
 * links against liblightc.a, and gives its result.
 *
 * GCC turns a builtin into a call to its C name, __builtin_floorf into
 * floorf, when the target has no instruction for it or when errno would
 * have to be set, and liblightc.a defines those names (decision N-a). This
 * program calls each of them, so a builtin that became a call to a name
 * liblightc.a lacks fails its link. configure builds it four times: at -O0
 * and at -O2, each with -fno-math-errno, as every compile of the tree and
 * lightc.pc's Cflags give it, and with -fmath-errno, under which GCC calls
 * sqrtf for a negative argument and lrintf. ci/consumer.sh builds it the
 * same four ways with the flags of an installed lightc.pc.
 *
 * Each argument is read through a volatile, so that GCC cannot fold a
 * call; each result is a value that every implementation gives, so a
 * name that linked to the wrong function fails too. <lightc/math.h>'s
 * lc_ functions are tested in test_math_exact.
 */

#include "test.h"
#include <lightc/math.h>

static volatile float  f_quarter = 0.25f, f_four = 4.0f, f_minus_one = -1.0f;
static volatile float  f_pos = 2.5f, f_neg = -2.5f, f_half = 0.5f, f_two = 2.0f;
static volatile float  f_negative_zero = -0.0f;
static volatile double d_quarter = 2.25, d_pos = 2.5, d_neg = -2.5, d_half = 0.5, d_two = 2.0;
static volatile double d_minus_one = -1.0;
static volatile int    i_three = 3, i_minus_one = -1;

static void test_sqrt(void) {
    TEST_ASSERT_EXACT(__builtin_sqrtf(f_four), 2.0f);
    TEST_ASSERT_EXACT(__builtin_sqrtf(f_quarter), 0.5f);
    TEST_ASSERT(lc_isnan(__builtin_sqrtf(f_minus_one)));
    TEST_ASSERT_EXACT(__builtin_sqrt(d_quarter), 1.5);
    TEST_ASSERT(lc_isnan(__builtin_sqrt(d_minus_one)));
}

static void test_floor_ceil_trunc(void) {
    TEST_ASSERT_EXACT(__builtin_floorf(f_neg), -3.0f);
    TEST_ASSERT_EXACT(__builtin_ceilf(f_neg), -2.0f);
    TEST_ASSERT_EXACT(__builtin_truncf(f_neg), -2.0f);
    TEST_ASSERT_EXACT(__builtin_floor(d_neg), -3.0);
    TEST_ASSERT_EXACT(__builtin_ceil(d_neg), -2.0);
    TEST_ASSERT_EXACT(__builtin_trunc(d_neg), -2.0);
}

static void test_round_rint(void) {
    TEST_ASSERT_EXACT(__builtin_roundf(f_neg), -3.0f);
    TEST_ASSERT_EXACT(__builtin_rintf(f_neg), -2.0f);
    TEST_ASSERT_EXACT(__builtin_nearbyintf(f_pos), 2.0f);
    TEST_ASSERT_EXACT(__builtin_round(d_pos), 3.0);
    TEST_ASSERT_EXACT(__builtin_rint(d_pos), 2.0);
    TEST_ASSERT_EXACT(__builtin_nearbyint(d_neg), -2.0);
    TEST_ASSERT_EQ(__builtin_lrintf(f_pos), 2L);
    TEST_ASSERT_EQ(__builtin_lrintf(f_neg), -2L);
}

static void test_fma(void) {
    TEST_ASSERT_EXACT(__builtin_fmaf(f_pos, f_two, f_half), 5.5f);
    TEST_ASSERT_EXACT(__builtin_fma(d_pos, d_two, d_half), 5.5);
}

static void test_min_max(void) {
    TEST_ASSERT_EXACT(__builtin_fminf(f_pos, f_neg), -2.5f);
    TEST_ASSERT_EXACT(__builtin_fmaxf(f_pos, f_neg), 2.5f);
    TEST_ASSERT_EXACT(__builtin_fmin(d_pos, d_neg), -2.5);
    TEST_ASSERT_EXACT(__builtin_fmax(d_pos, d_neg), 2.5);
}

static void test_sign_and_scale(void) {
    TEST_ASSERT_EXACT(__builtin_fabsf(f_neg), 2.5f);
    TEST_ASSERT_EXACT(__builtin_copysignf(f_pos, f_negative_zero), -2.5f);
    int exponent = 0;
    TEST_ASSERT_EXACT(__builtin_frexpf(f_pos, &exponent), 0.625f);
    TEST_ASSERT_EQ(exponent, 2);
    TEST_ASSERT_EXACT(__builtin_ldexpf(f_pos, i_three), 20.0f);
    TEST_ASSERT_EXACT(__builtin_scalbnf(f_pos, i_minus_one), 1.25f);
}

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);
    TEST_RUN(test_sqrt);
    TEST_RUN(test_floor_ceil_trunc);
    TEST_RUN(test_round_rint);
    TEST_RUN(test_fma);
    TEST_RUN(test_min_max);
    TEST_RUN(test_sign_and_scale);
    return test_main();
}

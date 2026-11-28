#ifndef LIGHTC_MATH_H
#define LIGHTC_MATH_H

#include "types.h"

/*
 * Floating-point math without libm. <math.h> is libc's, so a program on
 * lightc includes this header instead. <float.h> is the compiler's, and
 * still gives FLT_MAX, FLT_EPSILON, DBL_MANT_DIG and the rest.
 *
 * float is IEEE 754 binary32 and double is binary64, on both arches. This
 * header declares only lc_ names, and each part of it has a section of its
 * own. lightc has no libm, and nothing here needs one.
 */

/* --- The bits of a value --- */

/* The bits of `x`: the sign in bit 31, the exponent in bits 30 to 23 and
 * the fraction in bits 22 to 0. */
[[gnu::const]]
static inline uint32_t lc_float_to_bits(float x) {
    union { float value; uint32_t bits; } u = { .value = x };
    return u.bits;
}

/* The float whose bits are `bits`, laid out as lc_float_to_bits gives them.
 * Every pattern is a float, a signalling NaN too, and none is changed. */
[[gnu::const]]
static inline float lc_float_from_bits(uint32_t bits) {
    union { uint32_t bits; float value; } u = { .bits = bits };
    return u.value;
}

/* The bits of `x`: the sign in bit 63, the exponent in bits 62 to 52 and
 * the fraction in bits 51 to 0. */
[[gnu::const]]
static inline uint64_t lc_double_to_bits(double x) {
    union { double value; uint64_t bits; } u = { .value = x };
    return u.bits;
}

/* The double whose bits are `bits`, laid out as lc_double_to_bits gives
 * them. Every pattern is a double, a signalling NaN too, and none is
 * changed. */
[[gnu::const]]
static inline double lc_double_from_bits(uint64_t bits) {
    union { uint64_t bits; double value; } u = { .bits = bits };
    return u.value;
}

/* --- Classification --- */

/*
 * lc_isnan(x), lc_isinf(x), lc_isfinite(x) and lc_signbit(x) classify a
 * float or a double as <math.h>'s isnan, isinf, isfinite and signbit do,
 * and give a bool:
 *
 *   lc_isnan     x is a NaN, quiet or signalling, of either sign.
 *   lc_isinf     x is +infinity or -infinity.
 *   lc_isfinite  x is neither: a zero, a subnormal or a normal number.
 *   lc_signbit   the sign bit of x is set: for -0, a negative number,
 *                -infinity, and a NaN whose sign bit is set.
 *
 * Each is a type-generic macro that calls the function below for the type
 * of `x`: lc_float_isnan for a float, lc_double_isnan for a double, and so
 * on. `x` is evaluated once and is not converted, so a float is tested as
 * a float. Any other type, long double and the integers among them, does
 * not compile.
 *
 * They test the bits of `x`, not with __builtin_isnan and the like, so
 * that GCC's flags do not change their answers. Under -ffinite-math-only,
 * which -ffast-math sets, GCC folds __builtin_isnan and __builtin_isinf to
 * false and __builtin_isfinite to true, and under -fno-signed-zeros, which
 * -ffast-math sets too, it folds __builtin_signbit(x) to x < 0, which is
 * false for -0 and for every NaN. These still give the class of the value
 * that `x` holds, at every optimization level, with or without
 * -ffast-math. That is checked with GCC, not with clang. Under
 * -ffast-math, though, the compiler also rewrites arithmetic in ways
 * IEEE 754 does not allow, as when it folds x / x to 1 or fuses a * b - c
 * into one operation, so an expression that gives a NaN, an infinity or
 * -0 under IEEE 754 can give another value there: what these test is the
 * value the compiled code produced.
 */
#define lc_isnan(x) \
    _Generic((x), float: lc_float_isnan, double: lc_double_isnan)(x)
#define lc_isinf(x) \
    _Generic((x), float: lc_float_isinf, double: lc_double_isinf)(x)
#define lc_isfinite(x) \
    _Generic((x), float: lc_float_isfinite, double: lc_double_isfinite)(x)
#define lc_signbit(x) \
    _Generic((x), float: lc_float_signbit, double: lc_double_signbit)(x)

/* Whether `x` is a NaN: lc_isnan for a float. */
[[gnu::const]]
static inline bool lc_float_isnan(float x) {
    return (lc_float_to_bits(x) & 0x7fffffffu) > 0x7f800000u;
}

/* Whether `x` is an infinity: lc_isinf for a float. */
[[gnu::const]]
static inline bool lc_float_isinf(float x) {
    return (lc_float_to_bits(x) & 0x7fffffffu) == 0x7f800000u;
}

/* Whether `x` is neither a NaN nor an infinity: lc_isfinite for a float. */
[[gnu::const]]
static inline bool lc_float_isfinite(float x) {
    return (lc_float_to_bits(x) & 0x7fffffffu) < 0x7f800000u;
}

/* Whether the sign bit of `x` is set: lc_signbit for a float. */
[[gnu::const]]
static inline bool lc_float_signbit(float x) {
    return (lc_float_to_bits(x) >> 31) != 0;
}

/* Whether `x` is a NaN: lc_isnan for a double. */
[[gnu::const]]
static inline bool lc_double_isnan(double x) {
    return (lc_double_to_bits(x) & 0x7fffffffffffffffu) > 0x7ff0000000000000u;
}

/* Whether `x` is an infinity: lc_isinf for a double. */
[[gnu::const]]
static inline bool lc_double_isinf(double x) {
    return (lc_double_to_bits(x) & 0x7fffffffffffffffu) == 0x7ff0000000000000u;
}

/* Whether `x` is neither a NaN nor an infinity: lc_isfinite for a double. */
[[gnu::const]]
static inline bool lc_double_isfinite(double x) {
    return (lc_double_to_bits(x) & 0x7fffffffffffffffu) < 0x7ff0000000000000u;
}

/* Whether the sign bit of `x` is set: lc_signbit for a double. */
[[gnu::const]]
static inline bool lc_double_signbit(double x) {
    return (lc_double_to_bits(x) >> 63) != 0;
}

/* --- Constants --- */

/*
 * Positive infinity and a quiet NaN, as floats, as <math.h>'s INFINITY and
 * NAN are. A float converts to a double exactly, class and sign kept, so
 * they serve for doubles too. LC_NAN's bits are 0x7fc00000: sign clear,
 * the quiet bit set and no payload. Both are constant expressions, which
 * static initializers can use. Under -ffinite-math-only they keep their
 * bits, and lc_isinf and lc_isnan still recognize them, but the compiler
 * then assumes no value is either, so arithmetic and comparisons on them
 * can give anything.
 */
#define LC_INFINITY (__builtin_inff())
#define LC_NAN      (__builtin_nanf(""))

/*
 * π, e and ln 2, correctly rounded: LC_PI, LC_E and LC_LN2 are doubles,
 * and LC_PI_F, LC_E_F and LC_LN2_F floats. Each is a decimal literal with
 * 36 digits after the point, far more than either type holds, which the
 * compiler rounds once, to the nearest value of its type: each float is
 * the nearest float to its constant, not the double rounded again.
 */
#define LC_PI    3.141592653589793238462643383279502884
#define LC_PI_F  3.141592653589793238462643383279502884f
#define LC_E     2.718281828459045235360287471352662498
#define LC_E_F   2.718281828459045235360287471352662498f
#define LC_LN2   0.693147180559945309417232121458176568
#define LC_LN2_F 0.693147180559945309417232121458176568f

/* --- Exact functions --- */

/*
 * lc_sqrtf, lc_floorf and the functions below are <math.h>'s functions of
 * the same names without lc_: each for floats, and each but lc_lrintf,
 * lc_fabsf, lc_copysignf, lc_frexpf, lc_ldexpf and lc_scalbnf for doubles
 * too. Each is exact: its result is the correctly rounded one, bit for
 * bit, with C23 Annex F's special values, and the same bits on x86_64 and
 * aarch64. No errno is set and no FP flag is read, and the rounding mode
 * is always to nearest, ties to even, as lightc never changes it. A NaN
 * in gives a quiet NaN out, whose sign and payload are not specified;
 * lc_fabsf and lc_copysignf alone keep a NaN as it is but for its sign
 * bit, a signalling one too, as IEEE 754 says they do.
 *
 * Where C leaves the result open, lightc gives:
 *
 *   lc_rintf, lc_nearbyintf   the same function: the nearest integer, ties
 *                             to even; and so lc_rint and lc_nearbyint;
 *   lc_fminf, lc_fmaxf        -0 for lc_fminf of -0 and +0, either way
 *                             round, and +0 for lc_fmaxf of them; a NaN
 *                             and a number give the number, and a
 *                             signalling NaN gives a quiet NaN, as
 *                             aarch64's fminnm and fmaxnm do; and so the
 *                             double forms;
 *   lc_lrintf                 the nearest long, ties to even, saturated:
 *                             0 for a NaN, LONG_MAX for 2^63 and above,
 *                             LONG_MIN below -2^63;
 *   lc_frexpf                 0 in *exponent for ±0, an infinity or a NaN.
 *
 * Each is a static inline function over the GCC builtin of its name, such
 * as __builtin_floorf, so it compiles to one instruction where the target
 * has one, also in a function with a target("...") attribute: sqrtss,
 * roundss (SSE4.1) and vfmadd (FMA) on x86_64; fsqrt, frintm and the other
 * frint, fmadd, fminnm and fmaxnm on aarch64. Elsewhere GCC calls the
 * function of the standard name, floorf or fmaf, which liblightc.a defines
 * and no lightc header declares, so a program calls only the lc_ names: on
 * baseline x86_64, the rounding functions, fma, fmin, fmax, frexp, ldexp
 * and scalbn are calls.
 *
 * lc_sqrtf and lc_sqrt are one instruction under -fno-math-errno, which
 * lightc.pc's Cflags give. Without it GCC also tests for a negative
 * argument and then calls sqrtf, which gives the same result. Under
 * -ffast-math, though, GCC may rewrite these as it rewrites the program's
 * own arithmetic: it may give +0 for -0, or the other argument of lc_fminf
 * for a NaN. And when it folds a call on constants it takes a signalling
 * NaN for a quiet one.
 */

/* The square root of `x`, correctly rounded: -0 for -0, and a NaN for any
 * x below 0. */
[[gnu::const]]
static inline float lc_sqrtf(float x) {
    return __builtin_sqrtf(x);
}

/* The square root of `x`, correctly rounded: lc_sqrtf for a double. */
[[gnu::const]]
static inline double lc_sqrt(double x) {
    return __builtin_sqrt(x);
}

/* The largest integer not above `x`, as a float: -1 for -0.5, and `x`
 * itself for ±0, an infinity and any float of 2^23 or more. */
[[gnu::const]]
static inline float lc_floorf(float x) {
    return __builtin_floorf(x);
}

/* The largest integer not above `x`: lc_floorf for a double, where every
 * double of 2^52 or more is an integer. */
[[gnu::const]]
static inline double lc_floor(double x) {
    return __builtin_floor(x);
}

/* The smallest integer not below `x`, as a float: -0 for -0.5. */
[[gnu::const]]
static inline float lc_ceilf(float x) {
    return __builtin_ceilf(x);
}

/* The smallest integer not below `x`: lc_ceilf for a double. */
[[gnu::const]]
static inline double lc_ceil(double x) {
    return __builtin_ceil(x);
}

/* `x` rounded toward zero to an integer, as a float, with the sign of `x`:
 * -0 for -0.5. */
[[gnu::const]]
static inline float lc_truncf(float x) {
    return __builtin_truncf(x);
}

/* `x` rounded toward zero to an integer: lc_truncf for a double. */
[[gnu::const]]
static inline double lc_trunc(double x) {
    return __builtin_trunc(x);
}

/* The integer nearest `x`, as a float, ties away from zero: 3 for 2.5 and
 * -3 for -2.5, with the sign of `x`. */
[[gnu::const]]
static inline float lc_roundf(float x) {
    return __builtin_roundf(x);
}

/* The integer nearest `x`, ties away from zero: lc_roundf for a double. */
[[gnu::const]]
static inline double lc_round(double x) {
    return __builtin_round(x);
}

/* The integer nearest `x`, as a float, ties to even: 2 for 2.5 and for
 * 1.5, with the sign of `x`. It is lc_nearbyintf: the rounding mode is
 * always to nearest, and no flag is read. */
[[gnu::const]]
static inline float lc_rintf(float x) {
    return __builtin_nearbyintf(x);
}

/* The integer nearest `x`, ties to even: lc_rintf for a double. */
[[gnu::const]]
static inline double lc_rint(double x) {
    return __builtin_nearbyint(x);
}

/* The integer nearest `x`, as a float, ties to even: lc_rintf. */
[[gnu::const]]
static inline float lc_nearbyintf(float x) {
    return __builtin_nearbyintf(x);
}

/* The integer nearest `x`, ties to even: lc_rint. */
[[gnu::const]]
static inline double lc_nearbyint(double x) {
    return __builtin_nearbyint(x);
}

/*
 * The long nearest `x`, ties to even, saturated: 0 for a NaN, LONG_MAX for
 * 2^63, +infinity and every float between, and LONG_MIN for -infinity and
 * every float below -2^63; -2^63 itself is LONG_MIN, exactly. On x86_64
 * this is cvtss2si after a test of the range, as that instruction gives
 * LONG_MIN for each of those; aarch64's fcvtzs saturates as this does, so
 * the test there is only for a constant `x`, which GCC folds by its own
 * rules.
 */
[[gnu::const]]
static inline long lc_lrintf(float x) {
#if defined(__aarch64__)
    if (!__builtin_constant_p(x)) return __builtin_lrintf(x);
#endif
    uint32_t magnitude = lc_float_to_bits(x) & 0x7fffffffu;
    if (magnitude >= 0x5f000000u) {             /* 2^63 or more, or a NaN */
        if (magnitude > 0x7f800000u) return 0;
        return lc_float_signbit(x) ? -__LONG_MAX__ - 1 : __LONG_MAX__;
    }
    return __builtin_lrintf(x);
}

/* `x` times `y`, plus `z`, rounded once: the exact x * y + z, correctly
 * rounded. A NaN for an infinity times 0, and for an infinite product
 * plus the opposite infinity, but not for a finite product too large for a
 * float plus an infinity, which gives that infinity. */
[[gnu::const]]
static inline float lc_fmaf(float x, float y, float z) {
    return __builtin_fmaf(x, y, z);
}

/* `x` times `y`, plus `z`, rounded once: lc_fmaf for a double. */
[[gnu::const]]
static inline double lc_fma(double x, double y, double z) {
    return __builtin_fma(x, y, z);
}

/* The smaller of `x` and `y`: the number when one is a NaN, -0 for -0 and
 * +0 in either order, and a quiet NaN when both are NaNs or either is a
 * signalling NaN. */
[[gnu::const]]
static inline float lc_fminf(float x, float y) {
    return __builtin_fminf(x, y);
}

/* The smaller of `x` and `y`: lc_fminf for a double. */
[[gnu::const]]
static inline double lc_fmin(double x, double y) {
    return __builtin_fmin(x, y);
}

/* The larger of `x` and `y`: the number when one is a NaN, +0 for -0 and
 * +0 in either order, and a quiet NaN when both are NaNs or either is a
 * signalling NaN. */
[[gnu::const]]
static inline float lc_fmaxf(float x, float y) {
    return __builtin_fmaxf(x, y);
}

/* The larger of `x` and `y`: lc_fmaxf for a double. */
[[gnu::const]]
static inline double lc_fmax(double x, double y) {
    return __builtin_fmax(x, y);
}

/* `x` with its sign bit clear: a NaN keeps its other bits, a signalling
 * one too. */
[[gnu::const]]
static inline float lc_fabsf(float x) {
    return __builtin_fabsf(x);
}

/* `x` with the sign bit of `y`, whatever `y` is, a NaN too: a NaN `x`
 * keeps its other bits. */
[[gnu::const]]
static inline float lc_copysignf(float x, float y) {
    return __builtin_copysignf(x, y);
}

/*
 * `x` split into a fraction and a power of 2: the result has the sign of
 * `x` and a magnitude in [0.5, 1), and `x` is the result times 2 to the
 * power stored in *exponent. A subnormal `x` gives a normal fraction. For
 * ±0 and an infinity the result is `x`, for a NaN a quiet NaN, and for
 * each of them *exponent is 0.
 */
static inline float lc_frexpf(float x, int *exponent) {
    return __builtin_frexpf(x, exponent);
}

/* `x` times 2 to the power `n`, rounded once: exact unless the result is
 * subnormal, and an infinity or ±0, of the sign of `x`, when it is too
 * large or too small for a float. Any int `n` is taken. */
[[gnu::const]]
static inline float lc_ldexpf(float x, int n) {
    return __builtin_ldexpf(x, n);
}

/* `x` times 2 to the power `n`, rounded once: lc_ldexpf, as a float's
 * radix is 2. */
[[gnu::const]]
static inline float lc_scalbnf(float x, int n) {
    return __builtin_scalbnf(x, n);
}

#endif /* LIGHTC_MATH_H */

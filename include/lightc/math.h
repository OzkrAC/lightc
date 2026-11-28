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

#endif /* LIGHTC_MATH_H */

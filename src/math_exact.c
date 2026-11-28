/*
 * math_exact.c — the standard names of <lightc/math.h>'s exact functions.
 *
 * GCC turns a math builtin, such as __builtin_floorf, into a call to the
 * function of its C name when the target has no instruction for it, or
 * when errno would have to be set. lightc has no libm, so this file defines
 * those names, as src/mem.c defines memcpy: sqrt, floor, ceil, trunc,
 * round, rint, nearbyint, fma, fmin and fmax, each for float and double,
 * and lrintf, fabsf, copysignf, frexpf, ldexpf and scalbnf. No lightc
 * header declares them (decision N-a): a program calls the lc_ functions
 * of <lightc/math.h>, which are inline over the builtins, and GCC calls
 * these when it cannot inline one. Each gives what its lc_ function's
 * comment says, bit for bit, but for the sign and payload of a NaN.
 *
 * On aarch64 each but frexpf, ldexpf and scalbnf is the builtin, which is
 * one instruction there: fsqrt, frintm, frintp, frintz, frinta, frintx,
 * frinti, fmadd, fminnm and fmaxnm, and frintx then fcvtzs for lrintf.
 * Baseline x86_64 has only sqrtss, sqrtsd and cvtss2si of these, so the
 * rounding functions work on the bits there, fmin and fmax compare, fmaf
 * rounds once through a double, and fma multiplies the significands in
 * integers. A builtin used here must never fall back to a call, which
 * would be a call to the function it is in: tools/math-objects.sh checks
 * that the object calls no name it defines, and configure runs that check
 * with every build. frexpf, ldexpf and scalbnf work on the bits, or in
 * double, on both arches.
 *
 * configure builds every source with -fno-math-errno, so that sqrtf is
 * sqrtss alone and never calls itself for a negative argument, and with
 * -ffp-contract=off, so that GCC fuses no a * b + c on aarch64 that it
 * would not fuse on x86_64. Nothing here reads or sets the FP flags.
 */

#include <lightc/math.h>

#define F_SIGN      0x80000000u
#define F_INFINITY  0x7f800000u
#define F_QUIET     0x00400000u
#define F_FRACTION  0x007fffffu

#define D_SIGN      0x8000000000000000u
#define D_INFINITY  0x7ff0000000000000u
#define D_QUIET     0x0008000000000000u
#define D_FRACTION  0x000fffffffffffffu
#define D_IMPLICIT  0x0010000000000000u

/* The NaN with the bits `bits`, quiet: what an arithmetic operation of
 * either arch gives for a NaN operand. */
static inline float quiet_float(uint32_t bits) {
    return lc_float_from_bits(bits | F_QUIET);
}

static inline double quiet_double(uint64_t bits) {
    return lc_double_from_bits(bits | D_QUIET);
}

/* --- Square root --- */

/* sqrtss and sqrtsd on x86_64, fsqrt on aarch64: under -fno-math-errno
 * GCC adds no call for a negative argument. */
float sqrtf(float x) {
    return __builtin_sqrtf(x);
}

double sqrt(double x) {
    return __builtin_sqrt(x);
}

/* --- Rounding to an integer --- */

#if defined(__aarch64__)

float floorf(float x) { return __builtin_floorf(x); }
double floor(double x) { return __builtin_floor(x); }
float ceilf(float x) { return __builtin_ceilf(x); }
double ceil(double x) { return __builtin_ceil(x); }
float truncf(float x) { return __builtin_truncf(x); }
double trunc(double x) { return __builtin_trunc(x); }
float roundf(float x) { return __builtin_roundf(x); }
double round(double x) { return __builtin_round(x); }
float rintf(float x) { return __builtin_rintf(x); }
double rint(double x) { return __builtin_rint(x); }
float nearbyintf(float x) { return __builtin_nearbyintf(x); }
double nearbyint(double x) { return __builtin_nearbyint(x); }

#else

/*
 * Each works on the bits. A float of biased exponent e, between 127 and
 * 149, has 150 - e bits below its units place, which the mask
 * F_FRACTION >> (e - 127) covers; one of 150 and above is an integer, an
 * infinity or a NaN, and one below 127 is less than 1 in magnitude. Adding
 * to the bits carries into the exponent as the value does, so 1.5 rounded
 * up is 2. The sign bit is never touched, so each result has the sign of
 * `x`, as C asks: floor(-0.5) is -1, ceil(-0.5) is -0. The doubles are
 * the same with an exponent of 1023 and 52 fraction bits.
 */

float floorf(float x) {
    uint32_t bits = lc_float_to_bits(x);
    uint32_t e = (bits >> 23) & 0xff;
    if (e >= 150) return e == 0xff && (bits & F_FRACTION) ? quiet_float(bits) : x;
    if (e < 127) {
        if ((bits & ~F_SIGN) == 0) return x;
        return lc_float_from_bits(bits & F_SIGN ? 0xbf800000u : 0);   /* -1 or +0 */
    }
    uint32_t mask = F_FRACTION >> (e - 127);
    if ((bits & mask) == 0) return x;
    if (bits & F_SIGN) bits += mask;
    return lc_float_from_bits(bits & ~mask);
}

double floor(double x) {
    uint64_t bits = lc_double_to_bits(x);
    uint64_t e = (bits >> 52) & 0x7ff;
    if (e >= 1075) return e == 0x7ff && (bits & D_FRACTION) ? quiet_double(bits) : x;
    if (e < 1023) {
        if ((bits & ~D_SIGN) == 0) return x;
        return lc_double_from_bits(bits & D_SIGN ? 0xbff0000000000000u : 0);
    }
    uint64_t mask = D_FRACTION >> (e - 1023);
    if ((bits & mask) == 0) return x;
    if (bits & D_SIGN) bits += mask;
    return lc_double_from_bits(bits & ~mask);
}

float ceilf(float x) {
    uint32_t bits = lc_float_to_bits(x);
    uint32_t e = (bits >> 23) & 0xff;
    if (e >= 150) return e == 0xff && (bits & F_FRACTION) ? quiet_float(bits) : x;
    if (e < 127) {
        if ((bits & ~F_SIGN) == 0) return x;
        return lc_float_from_bits(bits & F_SIGN ? F_SIGN : 0x3f800000u);    /* -0 or 1 */
    }
    uint32_t mask = F_FRACTION >> (e - 127);
    if ((bits & mask) == 0) return x;
    if (!(bits & F_SIGN)) bits += mask;
    return lc_float_from_bits(bits & ~mask);
}

double ceil(double x) {
    uint64_t bits = lc_double_to_bits(x);
    uint64_t e = (bits >> 52) & 0x7ff;
    if (e >= 1075) return e == 0x7ff && (bits & D_FRACTION) ? quiet_double(bits) : x;
    if (e < 1023) {
        if ((bits & ~D_SIGN) == 0) return x;
        return lc_double_from_bits(bits & D_SIGN ? D_SIGN : 0x3ff0000000000000u);
    }
    uint64_t mask = D_FRACTION >> (e - 1023);
    if ((bits & mask) == 0) return x;
    if (!(bits & D_SIGN)) bits += mask;
    return lc_double_from_bits(bits & ~mask);
}

float truncf(float x) {
    uint32_t bits = lc_float_to_bits(x);
    uint32_t e = (bits >> 23) & 0xff;
    if (e >= 150) return e == 0xff && (bits & F_FRACTION) ? quiet_float(bits) : x;
    if (e < 127) return lc_float_from_bits(bits & F_SIGN);
    return lc_float_from_bits(bits & ~(F_FRACTION >> (e - 127)));
}

double trunc(double x) {
    uint64_t bits = lc_double_to_bits(x);
    uint64_t e = (bits >> 52) & 0x7ff;
    if (e >= 1075) return e == 0x7ff && (bits & D_FRACTION) ? quiet_double(bits) : x;
    if (e < 1023) return lc_double_from_bits(bits & D_SIGN);
    return lc_double_from_bits(bits & ~(D_FRACTION >> (e - 1023)));
}

/* Ties away from zero: half a unit added to the magnitude, then the
 * fraction cleared. */
float roundf(float x) {
    uint32_t bits = lc_float_to_bits(x);
    uint32_t e = (bits >> 23) & 0xff;
    if (e >= 150) return e == 0xff && (bits & F_FRACTION) ? quiet_float(bits) : x;
    if (e < 127) {
        /* 0.5 up to 1 rounds to 1, anything smaller to 0. */
        return lc_float_from_bits((bits & F_SIGN) | (e == 126 ? 0x3f800000u : 0));
    }
    uint32_t mask = F_FRACTION >> (e - 127);
    if ((bits & mask) == 0) return x;
    bits += (mask >> 1) + 1;
    return lc_float_from_bits(bits & ~mask);
}

double round(double x) {
    uint64_t bits = lc_double_to_bits(x);
    uint64_t e = (bits >> 52) & 0x7ff;
    if (e >= 1075) return e == 0x7ff && (bits & D_FRACTION) ? quiet_double(bits) : x;
    if (e < 1023)
        return lc_double_from_bits((bits & D_SIGN) | (e == 1022 ? 0x3ff0000000000000u : 0));
    uint64_t mask = D_FRACTION >> (e - 1023);
    if ((bits & mask) == 0) return x;
    bits += (mask >> 1) + 1;
    return lc_double_from_bits(bits & ~mask);
}

/*
 * Ties to even. `unit` is the bit of the units place, which for 1 up to 2
 * is the low bit of the exponent: 127 and 1023 are odd, as 1 is. The
 * fraction is compared with half a unit, and a tie goes up only when the
 * integer part is odd.
 */
float rintf(float x) {
    uint32_t bits = lc_float_to_bits(x);
    uint32_t e = (bits >> 23) & 0xff;
    if (e >= 150) return e == 0xff && (bits & F_FRACTION) ? quiet_float(bits) : x;
    if (e < 126) return lc_float_from_bits(bits & F_SIGN);       /* below 0.5 */
    if (e == 126) {                                               /* 0.5 up to 1 */
        bool half = (bits & F_FRACTION) == 0;
        return lc_float_from_bits((bits & F_SIGN) | (half ? 0 : 0x3f800000u));
    }
    uint32_t mask = F_FRACTION >> (e - 127);
    uint32_t unit = mask + 1;
    uint32_t fraction = bits & mask;
    uint32_t half = unit >> 1;
    bits &= ~mask;
    if (fraction > half || (fraction == half && (bits & unit))) bits += unit;
    return lc_float_from_bits(bits);
}

double rint(double x) {
    uint64_t bits = lc_double_to_bits(x);
    uint64_t e = (bits >> 52) & 0x7ff;
    if (e >= 1075) return e == 0x7ff && (bits & D_FRACTION) ? quiet_double(bits) : x;
    if (e < 1022) return lc_double_from_bits(bits & D_SIGN);
    if (e == 1022) {
        bool half = (bits & D_FRACTION) == 0;
        return lc_double_from_bits((bits & D_SIGN) | (half ? 0 : 0x3ff0000000000000u));
    }
    uint64_t mask = D_FRACTION >> (e - 1023);
    uint64_t unit = mask + 1;
    uint64_t fraction = bits & mask;
    uint64_t half = unit >> 1;
    bits &= ~mask;
    if (fraction > half || (fraction == half && (bits & unit))) bits += unit;
    return lc_double_from_bits(bits);
}

/* The rounding mode is always to nearest and no flag is read, so
 * nearbyint is rint. */
[[gnu::alias("rintf")]] float nearbyintf(float x);
[[gnu::alias("rint")]] double nearbyint(double x);

#endif

/* --- lrintf --- */

/* Saturated, as <lightc/math.h> says lc_lrintf is: fcvtzs does that on
 * aarch64, and on x86_64 cvtss2si gives LONG_MIN for each value out of
 * range, so the range is tested first there. */
long lrintf(float x) {
#if !defined(__aarch64__)
    uint32_t magnitude = lc_float_to_bits(x) & ~F_SIGN;
    if (magnitude >= 0x5f000000u) {             /* 2^63 or more, or a NaN */
        if (magnitude > F_INFINITY) return 0;
        return lc_float_signbit(x) ? -__LONG_MAX__ - 1 : __LONG_MAX__;
    }
#endif
    return __builtin_lrintf(x);
}

/* --- Fused multiply-add --- */

#if defined(__aarch64__)

float fmaf(float x, float y, float z) { return __builtin_fmaf(x, y, z); }
double fma(double x, double y, double z) { return __builtin_fma(x, y, z); }

#else

/*
 * The product of two floats is exact in a double, which holds 53 bits to
 * their 48, and as no float is near a double's range it neither overflows
 * nor underflows there. Adding z rounds, and the float then rounds again,
 * which could round a sum just off a float's halfway point onto it, and
 * then to even, away from the exact result. So the sum is rounded to odd
 * instead: when it is inexact, of the two doubles around the exact sum the
 * one whose last bit is 1. A double rounded to odd, with 53 bits to the
 * float's 24 or fewer, rounds to the float as the exact value would
 * (Boldo and Melquiond, "Emulation of FMA and correctly rounded sums:
 * proved algorithms using rounding to odd", 2008).
 *
 * The sum's error comes from Knuth's two-sum, exact for any two doubles
 * that do not overflow. A finite nonzero sum of these is at least 2^-298 in
 * magnitude, so it rounds to 0 only when it is 0, and then exactly.
 * Infinities and NaNs come through the arithmetic as fma's: an infinite
 * product or z, or a NaN, gives the same in the sum.
 */
float fmaf(float x, float y, float z) {
    double product = (double)x * (double)y;
    double addend = z;
    double sum = product + addend;
    if (lc_double_isfinite(sum)) {
        double addend_part = sum - product;
        double error = (product - (sum - addend_part)) + (addend - addend_part);
        uint64_t bits = lc_double_to_bits(sum);
        if (error != 0.0 && (bits & 1) == 0) {
            /* Even and inexact: step to the odd neighbour toward the
             * exact sum, up in magnitude when the error has the sum's
             * sign. sum is not 0 here, so the step stays on its side. */
            bits += (error > 0.0) == (sum > 0.0) ? 1 : (uint64_t)-1;
            sum = lc_double_from_bits(bits);
        }
    }
    return (float)sum;
}

/*
 * fma for doubles, in integers. The significands multiply to 106 bits,
 * exactly, in a 128-bit pair; z's significand and the product are each
 * shifted so that their top bit is bit 125, the one of smaller exponent is
 * shifted right to align with the other, with each bit shifted out kept
 * as a sticky bit 0, and the two are added or subtracted. That leaves two
 * bits above for a carry. The sum is then rounded once to nearest, ties to
 * even, to 53 bits, or to fewer when it is subnormal.
 *
 * A shift right loses bits only when the exponents differ by 20 or more,
 * as the product's low 19 bits and z's low 72 are zeros; then the larger
 * has its top bit at 125 and the smaller is below 2^106, so the result
 * keeps its top bit at 124 or above, and its rounding point, 52 bits
 * below that, is far above the sticky bit. When nothing is lost the
 * difference is exact, and only then can it cancel to 0.
 */

typedef struct {
    uint64_t hi;
    uint64_t lo;
} u128;

/* GCC's 128-bit integer, for its 64 by 64 bit multiply alone: a shift of
 * one by a variable count is a call to libgcc at -Os on aarch64. */
__extension__ typedef unsigned __int128 wide_product;

static inline u128 u128_multiply(uint64_t a, uint64_t b) {
    wide_product p = (wide_product)a * b;
    return (u128){ (uint64_t)(p >> 64), (uint64_t)p };
}

/* The number of zero bits above the top 1 of `a`, which is not 0. */
static inline int u128_leading_zeros(u128 a) {
    return a.hi ? __builtin_clzll(a.hi) : 64 + __builtin_clzll(a.lo);
}

/* `a` shifted left by `n`, from 0 to 127. */
static inline u128 u128_shift_left(u128 a, int n) {
    if (n == 0) return a;
    if (n >= 64) return (u128){ a.lo << (n - 64), 0 };
    return (u128){ (a.hi << n) | (a.lo >> (64 - n)), a.lo << n };
}

/* `a` shifted right by `n`, 0 or more, with bit 0 set when a 1 was
 * shifted out. */
static inline u128 u128_shift_right_sticky(u128 a, int n) {
    if (n == 0) return a;
    if (n >= 128) return (u128){ 0, (a.hi | a.lo) != 0 };
    u128 r;
    uint64_t lost;
    if (n >= 64) {
        int m = n - 64;
        lost = a.lo | (m ? a.hi << (64 - m) : 0);
        r = (u128){ 0, a.hi >> m };
    } else {
        lost = a.lo << (64 - n);
        r = (u128){ a.hi >> n, (a.lo >> n) | (a.hi << (64 - n)) };
    }
    r.lo |= lost != 0;
    return r;
}

/* Bit `n`, from 0 to 127, of `a`. */
static inline uint64_t u128_bit(u128 a, int n) {
    return (n >= 64 ? a.hi >> (n - 64) : a.lo >> n) & 1;
}

/* Whether a bit of `a` below bit `n`, from 0 to 128, is 1. */
static inline bool u128_any_below(u128 a, int n) {
    if (n == 0) return false;
    if (n < 64) return (a.lo << (64 - n)) != 0;
    if (n == 64) return a.lo != 0;
    return a.lo != 0 || (n < 128 ? a.hi << (128 - n) : a.hi) != 0;
}

static inline bool u128_less(u128 a, u128 b) {
    return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
}

static inline u128 u128_add(u128 a, u128 b) {
    uint64_t lo = a.lo + b.lo;
    return (u128){ a.hi + b.hi + (lo < a.lo), lo };
}

static inline u128 u128_subtract(u128 a, u128 b) {
    return (u128){ a.hi - b.hi - (a.lo < b.lo), a.lo - b.lo };
}

/* The significand of a finite nonzero double of magnitude bits `bits`,
 * and in *exponent the power of 2 of its last bit: the value is the
 * significand times 2^*exponent. */
static inline uint64_t significand(uint64_t bits, int *exponent) {
    int e = (int)(bits >> 52);
    uint64_t m = bits & D_FRACTION;
    if (e == 0) e = 1;
    else m |= D_IMPLICIT;
    *exponent = e - 1075;
    return m;
}

double fma(double x, double y, double z) {
    uint64_t ux = lc_double_to_bits(x), uy = lc_double_to_bits(y), uz = lc_double_to_bits(z);
    uint64_t ax = ux & ~D_SIGN, ay = uy & ~D_SIGN, az = uz & ~D_SIGN;

    /* An infinite or NaN factor, or a NaN z: the operations give fma's
     * result, as no finite product is rounded on the way. */
    if (ax >= D_INFINITY || ay >= D_INFINITY || az > D_INFINITY) return x * y + z;
    /* The product is finite, however large, so an infinite z wins. */
    if (az == D_INFINITY) return z;
    /* An exact zero product, whose sum with z follows the rules of zeros. */
    if (ax == 0 || ay == 0) return x * y + z;
    /* The product is not 0, so z = ±0 changes nothing, and the product
     * rounded once is the result, with its sign also when it underflows. */
    if (az == 0) return x * y;

    int ex, ey, ez;
    uint64_t mx = significand(ax, &ex), my = significand(ay, &ey), mz = significand(az, &ez);
    uint64_t product_sign = (ux ^ uy) & D_SIGN, z_sign = uz & D_SIGN;

    u128 p = u128_multiply(mx, my);
    int ep = ex + ey;
    int shift = u128_leading_zeros(p) - 2;
    p = u128_shift_left(p, shift);
    ep -= shift;

    u128 q = { 0, mz };
    int eq = ez;
    shift = u128_leading_zeros(q) - 2;
    q = u128_shift_left(q, shift);
    eq -= shift;

    /* a is the larger in magnitude. */
    u128 a, b;
    uint64_t sign, b_sign;
    int e, distance;
    if (ep > eq || (ep == eq && !u128_less(p, q))) {
        a = p; sign = product_sign; b = q; b_sign = z_sign; e = ep; distance = ep - eq;
    } else {
        a = q; sign = z_sign; b = p; b_sign = product_sign; e = eq; distance = eq - ep;
    }
    b = u128_shift_right_sticky(b, distance);
    u128 r;
    if (sign == b_sign) {
        r = u128_add(a, b);
    } else {
        r = u128_subtract(a, b);
        if (r.hi == 0 && r.lo == 0) return 0.0;     /* exact cancellation: +0 */
    }

    /* r times 2^e is the exact result, but for the sticky bit. Keep its top
     * 53 bits, or, when the last of them would be below 2^-1074, the bits
     * from 2^-1074 up. */
    int top = 127 - u128_leading_zeros(r);
    int cut = top - 52;
    if (e + cut < -1074) cut = -1074 - e;
    uint64_t m;
    if (cut <= 0) {
        m = r.lo << -cut;                           /* r < 2^53: exact */
    } else if (cut >= 128) {
        m = 0;                                      /* below half of 2^-1074 */
    } else {
        m = cut >= 64 ? r.hi >> (cut - 64) : (r.lo >> cut) | (r.hi << (64 - cut));
        if (u128_bit(r, cut - 1) && (u128_any_below(r, cut - 1) || (m & 1))) m++;
    }

    /* m is the significand, with its implicit bit, of weight 2^(e + cut)
     * for its last bit; adding it to the exponent field carries into the
     * exponent, so 2^53 after rounding up, or 2^52 from a subnormal, comes
     * out right. */
    int last = e + cut + 1074;                      /* 0 for a subnormal */
    if (last > 2045) return lc_double_from_bits(sign | D_INFINITY);
    uint64_t bits = ((uint64_t)last << 52) + m;
    if (bits >= D_INFINITY) bits = D_INFINITY;
    return lc_double_from_bits(sign | bits);
}

#endif

/* --- Minimum and maximum --- */

#if defined(__aarch64__)

float fminf(float x, float y) { return __builtin_fminf(x, y); }
double fmin(double x, double y) { return __builtin_fmin(x, y); }
float fmaxf(float x, float y) { return __builtin_fmaxf(x, y); }
double fmax(double x, double y) { return __builtin_fmax(x, y); }

#else

/*
 * As fminnm and fmaxnm do on aarch64: a signalling NaN gives itself,
 * quieted, the first one when both are; else a quiet NaN gives the other
 * operand, and two give the first. Of two zeros fmin gives -0 if either
 * is, fmax +0 if either is. The rest compare as numbers.
 */

float fminf(float x, float y) {
    uint32_t ux = lc_float_to_bits(x), uy = lc_float_to_bits(y);
    uint32_t ax = ux & ~F_SIGN, ay = uy & ~F_SIGN;
    if (ax > F_INFINITY || ay > F_INFINITY) {
        if (ax > F_INFINITY && !(ux & F_QUIET)) return quiet_float(ux);
        if (ay > F_INFINITY && !(uy & F_QUIET)) return quiet_float(uy);
        return ax > F_INFINITY ? (ay > F_INFINITY ? x : y) : x;
    }
    if ((ax | ay) == 0) return lc_float_from_bits(ux | uy);
    return x < y ? x : y;
}

float fmaxf(float x, float y) {
    uint32_t ux = lc_float_to_bits(x), uy = lc_float_to_bits(y);
    uint32_t ax = ux & ~F_SIGN, ay = uy & ~F_SIGN;
    if (ax > F_INFINITY || ay > F_INFINITY) {
        if (ax > F_INFINITY && !(ux & F_QUIET)) return quiet_float(ux);
        if (ay > F_INFINITY && !(uy & F_QUIET)) return quiet_float(uy);
        return ax > F_INFINITY ? (ay > F_INFINITY ? x : y) : x;
    }
    if ((ax | ay) == 0) return lc_float_from_bits(ux & uy);
    return x > y ? x : y;
}

double fmin(double x, double y) {
    uint64_t ux = lc_double_to_bits(x), uy = lc_double_to_bits(y);
    uint64_t ax = ux & ~D_SIGN, ay = uy & ~D_SIGN;
    if (ax > D_INFINITY || ay > D_INFINITY) {
        if (ax > D_INFINITY && !(ux & D_QUIET)) return quiet_double(ux);
        if (ay > D_INFINITY && !(uy & D_QUIET)) return quiet_double(uy);
        return ax > D_INFINITY ? (ay > D_INFINITY ? x : y) : x;
    }
    if ((ax | ay) == 0) return lc_double_from_bits(ux | uy);
    return x < y ? x : y;
}

double fmax(double x, double y) {
    uint64_t ux = lc_double_to_bits(x), uy = lc_double_to_bits(y);
    uint64_t ax = ux & ~D_SIGN, ay = uy & ~D_SIGN;
    if (ax > D_INFINITY || ay > D_INFINITY) {
        if (ax > D_INFINITY && !(ux & D_QUIET)) return quiet_double(ux);
        if (ay > D_INFINITY && !(uy & D_QUIET)) return quiet_double(uy);
        return ax > D_INFINITY ? (ay > D_INFINITY ? x : y) : x;
    }
    if ((ax | ay) == 0) return lc_double_from_bits(ux & uy);
    return x > y ? x : y;
}

#endif

/* --- Sign, exponent and scale: the same code on both arches --- */

float fabsf(float x) {
    return lc_float_from_bits(lc_float_to_bits(x) & ~F_SIGN);
}

float copysignf(float x, float y) {
    return lc_float_from_bits((lc_float_to_bits(x) & ~F_SIGN) | (lc_float_to_bits(y) & F_SIGN));
}

float frexpf(float x, int *exponent) {
    uint32_t bits = lc_float_to_bits(x);
    uint32_t e = (bits >> 23) & 0xff;
    *exponent = 0;
    if (e == 0xff) return bits & F_FRACTION ? quiet_float(bits) : x;
    if (e == 0) {
        uint32_t fraction = bits & F_FRACTION;
        if (fraction == 0) return x;
        /* A subnormal: move its top 1 to the implicit bit, bit 23. */
        int shift = __builtin_clz(fraction) - 8;
        fraction <<= shift;
        *exponent = -125 - shift;
        return lc_float_from_bits((bits & F_SIGN) | (126u << 23) | (fraction & F_FRACTION));
    }
    *exponent = (int)e - 126;
    return lc_float_from_bits((bits & (F_SIGN | F_FRACTION)) | (126u << 23));
}

/*
 * In double, where x times 2^n is exact for every float x and every n from
 * -300 to 300, and the conversion to float rounds it once. An n beyond
 * that range gives the same as ±300: an infinity for any x but ±0, or ±0,
 * as a float is at least 2^-149 and below 2^128 in magnitude.
 */
float ldexpf(float x, int n) {
    if (n > 300) n = 300;
    if (n < -300) n = -300;
    return (float)((double)x * lc_double_from_bits((uint64_t)(n + 1023) << 52));
}

/* A float's radix is 2, so scalbn is ldexp. */
[[gnu::alias("ldexpf")]] float scalbnf(float x, int n);

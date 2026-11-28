/*
 * math-sweep — lightc's math functions against the host's libm.
 *
 * For each function of the table below, this compares lightc's results
 * with the oracle's, the host libm's function of the same name (decision
 * N-d: this tool, unlike the library and its tests, links libc and libm).
 * It compares two forms of each: the library's function of the standard
 * name, floorf, as GCC calls it, and the lc_ function of <lightc/math.h>,
 * lc_floorf, as a program on lightc compiles it. A float function of one
 * argument is checked on all 2^32 floats. One of two arguments is checked
 * on every float as the first, each paired with two second arguments,
 * one of a list of special values and one drawn from the float. fmaf, and
 * every double function, are checked on random samples, drawn from kinds
 * of input that reach their hard cases, the same samples whatever the
 * threads.
 *
 * ninja math-sweep builds it, as build/math-sweep, from this file twice
 * and the library's math objects, which tools/math-objects.sh renames:
 * floorf becomes lightc_floorf, so that libm's floorf stays the oracle.
 * Compiled with -DMATH_SWEEP_LC and the library's flags, this file is
 * freestanding, and wraps each lc_ function in an extern sweep_lc_NAME;
 * its calls to standard names are renamed too, so that a builtin that GCC
 * did not inline lands in lightc's function, not in libm's. Compiled
 * without it, the rest of this file is the tool, a hosted program. Neither
 * ninja, ninja test nor the library builds it.
 *
 * What is compared, by the RULE of a function's line:
 *
 *   NAN_QUIET  the bits, but a NaN result matches any NaN of the oracle's,
 *              and lightc's must be quiet;
 *   NAN_BITS   the bits, NaNs too: fabsf and copysignf only change the
 *              sign bit;
 *   MIN, MAX   NAN_QUIET, but for two zeros lightc's rule stands for the
 *              oracle's, which C leaves open: -0 from fmin if either is -0,
 *              +0 from fmax if either is +0;
 *   SATURATE   lrintf: for a NaN 0, and for 2^63 and beyond LONG_MAX or
 *              LONG_MIN, lightc's rule, where C gives no value;
 *
 * and ULPS, the most ulps lightc may be from the oracle, is 0 for the
 * exact functions, which then must give the oracle's bits. The ulp distance
 * is test.h's: the steps between two finite values of a type, +0 and -0
 * being one; an infinity matches only itself. frexpf's exponent must be
 * the oracle's too.
 *
 * Each line reports, for a function, the inputs checked, the mismatches of
 * each form, how many lightc's rule decided, the largest ulp distance, a
 * hash of the library form's results and the time. The hash is the same
 * on any machine that gives the same results, with every NaN counted as
 * one value, as NaN payloads differ between the arches: nightly runs
 * compare it between x86_64 and aarch64. A mismatch prints its inputs and
 * results, the first few of each function. The exit status is 1 when any
 * function had a mismatch.
 *
 * Usage:
 *   build/math-sweep [-t THREADS] [-n LOG2] [-e LOG2] [-s SEED] [NAME...]
 *       Sweep each NAME, or every function. THREADS defaults to the online
 *       CPUs. Random samples are 2^32 of fmaf and 2^30 of each double
 *       function, or 2^LOG2 of each with -n, drawn from SEED (default 1).
 *       -e takes 2^LOG2 first arguments, spread over all the floats,
 *       instead of every one (default 32), for a quick run.
 *   build/math-sweep -l
 *       List the functions, their kinds and their rules.
 *   build/math-sweep -c [NAME...] > FILE
 *       Print the hard cases of each NAME, or of every function, as a
 *       header of lines that tests/test_math_exact.c includes: the inputs
 *       of each kind's list below, each with the oracle's result, or
 *       lightc's rule where that stands for it, and NaNs written as the
 *       default quiet NaN but for NAN_BITS. tests/data/math_exact_cases.h
 *       is the output of "build/math-sweep -c" on x86_64.
 *
 * A new function, of a kind below, takes a line in SWEEP_FUNCTIONS: its
 * standard name, which must be both libm's and the one lightc defines, its
 * bound and its rule. Its lc_ form must be lc_NAME, and its object one
 * that configure's sweep edges rename. A new kind takes a wrapper, a
 * declaration, a sweep and a case printer below.
 */

/* --- The functions: X(KIND, NAME, ULPS, RULE) ---
 *
 * The kinds, by their result and arguments:
 *   F_F   float (float)            D_D    double (double)
 *   L_F   long (float)             F_FE   float (float, int *)
 *   F_FI  float (float, int)       F_FF   float (float, float)
 *   D_DD  double (double, double)  F_FFF  float (float, float, float)
 *   D_DDD double (double, double, double)
 */
#define SWEEP_FUNCTIONS(X)                  \
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

#if defined(MATH_SWEEP_LC)

/* --- The lc_ forms, compiled as a program on lightc compiles them --- */

#include <lightc/math.h>

#define WRAP_F_F(name)   float sweep_lc_##name(float x) { return lc_##name(x); }
#define WRAP_D_D(name)   double sweep_lc_##name(double x) { return lc_##name(x); }
#define WRAP_L_F(name)   long sweep_lc_##name(float x) { return lc_##name(x); }
#define WRAP_F_FE(name)  float sweep_lc_##name(float x, int *e) { return lc_##name(x, e); }
#define WRAP_F_FI(name)  float sweep_lc_##name(float x, int n) { return lc_##name(x, n); }
#define WRAP_F_FF(name)  float sweep_lc_##name(float x, float y) { return lc_##name(x, y); }
#define WRAP_D_DD(name)  double sweep_lc_##name(double x, double y) { return lc_##name(x, y); }
#define WRAP_F_FFF(name) float sweep_lc_##name(float x, float y, float z) { return lc_##name(x, y, z); }
#define WRAP_D_DDD(name) double sweep_lc_##name(double x, double y, double z) { return lc_##name(x, y, z); }

#define WRAP(kind, name, ulps, rule) WRAP_##kind(name)
SWEEP_FUNCTIONS(WRAP)

#else

/* --- The tool --- */

#include <gnu/libc-version.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* lightc's two forms of each: lightc_NAME is the library's NAME, renamed,
 * and sweep_lc_NAME wraps lc_NAME. */
#define DECLARE_F_F(name)   float lightc_##name(float); float sweep_lc_##name(float);
#define DECLARE_D_D(name)   double lightc_##name(double); double sweep_lc_##name(double);
#define DECLARE_L_F(name)   long lightc_##name(float); long sweep_lc_##name(float);
#define DECLARE_F_FE(name)  float lightc_##name(float, int *); float sweep_lc_##name(float, int *);
#define DECLARE_F_FI(name)  float lightc_##name(float, int); float sweep_lc_##name(float, int);
#define DECLARE_F_FF(name)  float lightc_##name(float, float); float sweep_lc_##name(float, float);
#define DECLARE_D_DD(name)  double lightc_##name(double, double); double sweep_lc_##name(double, double);
#define DECLARE_F_FFF(name) float lightc_##name(float, float, float); \
                            float sweep_lc_##name(float, float, float);
#define DECLARE_D_DDD(name) double lightc_##name(double, double, double); \
                            double sweep_lc_##name(double, double, double);
#define DECLARE(kind, name, ulps, rule) DECLARE_##kind(name)
SWEEP_FUNCTIONS(DECLARE)

enum { F_F, D_D, L_F, F_FE, F_FI, F_FF, D_DD, F_FFF, D_DDD };
enum { NAN_QUIET, NAN_BITS, MIN, MAX, SATURATE };

static const char *const kind_names[] = {
    "float(float)", "double(double)", "long(float)", "float(float, int *)",
    "float(float, int)", "float(float, float)", "double(double, double)",
    "float(float, float, float)", "double(double, double, double)",
};
static const char *const rule_names[] = { "NAN_QUIET", "NAN_BITS", "MIN", "MAX", "SATURATE" };

typedef void (*any_function)(void);

typedef struct {
    const char  *name;
    int          kind;
    unsigned     ulps;
    int          rule;
    any_function lightc;    /* the library's function of the name */
    any_function lc;        /* the lc_ function */
    any_function oracle;    /* libm's */
} function;

#define ENTRY(kind_, name_, ulps_, rule_)                                   \
    { .name = #name_, .kind = kind_, .ulps = ulps_, .rule = rule_,          \
      .lightc = (any_function)lightc_##name_,                               \
      .lc = (any_function)sweep_lc_##name_, .oracle = (any_function)name_ },
static const function functions[] = { SWEEP_FUNCTIONS(ENTRY) };
#define FUNCTION_COUNT (sizeof functions / sizeof functions[0])

/* --- Bits --- */

static uint32_t fbits(float x) { uint32_t u; memcpy(&u, &x, 4); return u; }
static float ffrom(uint32_t u) { float x; memcpy(&x, &u, 4); return x; }
static uint64_t dbits(double x) { uint64_t u; memcpy(&u, &x, 8); return u; }
static double dfrom(uint64_t u) { double x; memcpy(&x, &u, 8); return x; }

static bool fnan(uint32_t u) { return (u & 0x7fffffffu) > 0x7f800000u; }
static bool dnan(uint64_t u) { return (u & 0x7fffffffffffffffu) > 0x7ff0000000000000u; }

/* splitmix64: a sample's inputs come from its own seed alone. */
static uint64_t next_random(uint64_t *state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15u);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9u;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebu;
    return z ^ (z >> 31);
}

static uint64_t mix(uint64_t x) {
    uint64_t state = x;
    return next_random(&state);
}

/* A result's part of a function's hash: the sum of these over its inputs,
 * which does not depend on the order the threads take them in. */
static inline uint64_t hash_of(const uint64_t in[3], uint64_t n, uint64_t out) {
    uint64_t h = ((in[0] * 0x9e3779b97f4a7c15u + in[1]) * 0xc2b2ae3d27d4eb4fu + in[2] + n) ^ out;
    h *= 0xbf58476d1ce4e5b9u;
    return h ^ (h >> 31);
}

/* --- What one chunk of a sweep found --- */

#define KEPT 4

typedef struct {
    uint64_t in[3];     /* the arguments' bits */
    int      n;         /* the int argument of F_FI */
    int      form;      /* 0: the library's function, 1: the lc_ one */
    uint64_t got, want;
    int      got_e, want_e;
} mismatch;

typedef struct {
    uint64_t inputs;
    uint64_t mismatches[2];
    uint64_t by_rule;       /* inputs where lightc's rule stood for the oracle */
    uint64_t max_ulps;
    uint64_t hash;
    int      kept;
    mismatch first[KEPT];
} tally;

/* Distance in ulps between two finite values of the same width, given as
 * magnitudes and signs. */
static uint64_t ulp_distance(uint64_t a, bool a_negative, uint64_t b, bool b_negative) {
    if (a_negative != b_negative) return a + b;
    return a > b ? a - b : b - a;
}

/* Whether a float result `got` passes against `want`, by the function's
 * rule and bound; *ulps is their distance when both are finite. */
static bool float_passes(const function *f, uint32_t got, uint32_t want, uint64_t *ulps) {
    *ulps = 0;
    if (f->rule == NAN_BITS) return got == want;
    if (fnan(got) || fnan(want)) return fnan(got) && fnan(want) && (got & 0x00400000u);
    if (got == want) return true;
    uint32_t ag = got & 0x7fffffffu, aw = want & 0x7fffffffu;
    if (ag == 0x7f800000u || aw == 0x7f800000u) return false;
    *ulps = ulp_distance(ag, got >> 31, aw, want >> 31);
    return *ulps != 0 ? *ulps <= f->ulps : f->ulps > 0;   /* ±0: 0 apart, but not the same bits */
}

static bool double_passes(const function *f, uint64_t got, uint64_t want, uint64_t *ulps) {
    *ulps = 0;
    if (f->rule == NAN_BITS) return got == want;
    if (dnan(got) || dnan(want)) return dnan(got) && dnan(want) && (got & 0x0008000000000000u);
    if (got == want) return true;
    uint64_t ag = got & 0x7fffffffffffffffu, aw = want & 0x7fffffffffffffffu;
    if (ag == 0x7ff0000000000000u || aw == 0x7ff0000000000000u) return false;
    *ulps = ulp_distance(ag, got >> 63, aw, want >> 63);
    return *ulps != 0 ? *ulps <= f->ulps : f->ulps > 0;
}

/* The bits a hash takes of a result: every NaN as one. */
static uint64_t float_canonical(uint32_t u) { return fnan(u) ? 0x7fc00000u : u; }
static uint64_t double_canonical(uint64_t u) { return dnan(u) ? 0x7ff8000000000000u : u; }

static void record(tally *t, int form, const uint64_t in[3], int n,
                   uint64_t got, uint64_t want, int got_e, int want_e) {
    t->mismatches[form]++;
    if (t->kept < KEPT) {
        mismatch *m = &t->first[t->kept++];
        memcpy(m->in, in, sizeof m->in);
        m->n = n;
        m->form = form;
        m->got = got;
        m->want = want;
        m->got_e = got_e;
        m->want_e = want_e;
    }
}

/* Checks the two forms' float results against the expected one. The
 * common case, both forms giving the expected bits and that not being a
 * signalling NaN, which no form passes for, is decided inline. */
[[gnu::noinline, gnu::cold]]
static void check_float_slowly(const function *f, tally *t, const uint64_t in[3], int n,
                               uint32_t want, uint32_t got0, uint32_t got1) {
    uint64_t ulps;
    if (!float_passes(f, got0, want, &ulps)) record(t, 0, in, n, got0, want, 0, 0);
    else if (ulps > t->max_ulps) t->max_ulps = ulps;
    if (!float_passes(f, got1, want, &ulps)) record(t, 1, in, n, got1, want, 0, 0);
    else if (ulps > t->max_ulps) t->max_ulps = ulps;
}

[[gnu::always_inline]]
static inline void check_float(const function *f, tally *t, const uint64_t in[3], int n,
                               uint32_t want, uint32_t got0, uint32_t got1) {
    t->inputs++;
    t->hash += hash_of(in, (uint64_t)n, float_canonical(got0));
    bool signalling = (want & 0x7fc00000u) == 0x7f800000u && (want & 0x003fffffu);
    if (__builtin_expect(got0 == want && got1 == want && !signalling, 1)) return;
    check_float_slowly(f, t, in, n, want, got0, got1);
}

[[gnu::noinline, gnu::cold]]
static void check_double_slowly(const function *f, tally *t, const uint64_t in[3],
                                uint64_t want, uint64_t got0, uint64_t got1) {
    uint64_t ulps;
    if (!double_passes(f, got0, want, &ulps)) record(t, 0, in, 0, got0, want, 0, 0);
    else if (ulps > t->max_ulps) t->max_ulps = ulps;
    if (!double_passes(f, got1, want, &ulps)) record(t, 1, in, 0, got1, want, 0, 0);
    else if (ulps > t->max_ulps) t->max_ulps = ulps;
}

[[gnu::always_inline]]
static inline void check_double(const function *f, tally *t, const uint64_t in[3],
                                uint64_t want, uint64_t got0, uint64_t got1) {
    t->inputs++;
    t->hash += hash_of(in, 0, double_canonical(got0));
    bool signalling = (want & 0x7ff8000000000000u) == 0x7ff0000000000000u && (want & 0x0007ffffffffffffu);
    if (__builtin_expect(got0 == want && got1 == want && !signalling, 1)) return;
    check_double_slowly(f, t, in, want, got0, got1);
}

/* --- The expected results: the oracle's, or lightc's rule --- */

/* The expected result of a pair: the oracle's, `result`, but where
 * lightc's rule stands for it. */
static inline uint32_t expect_float_pair(const function *f, uint32_t x, uint32_t y, uint32_t result,
                                         tally *t) {
    if ((f->rule == MIN || f->rule == MAX) && ((x | y) & 0x7fffffffu) == 0) {
        t->by_rule++;
        return f->rule == MIN ? (x | y) : (x & y);
    }
    return result;
}

static inline uint64_t expect_double_pair(const function *f, uint64_t x, uint64_t y, uint64_t result,
                                          tally *t) {
    if ((f->rule == MIN || f->rule == MAX) && ((x | y) & 0x7fffffffffffffffu) == 0) {
        t->by_rule++;
        return f->rule == MIN ? (x | y) : (x & y);
    }
    return result;
}

static inline long expect_long(const function *f, uint32_t x, long result, tally *t) {
    uint32_t magnitude = x & 0x7fffffffu;
    if (f->rule == SATURATE && magnitude >= 0x5f000000u) {
        t->by_rule++;
        if (magnitude > 0x7f800000u) return 0;
        return x >> 31 ? LONG_MIN : LONG_MAX;
    }
    return result;
}

/* --- Inputs --- */

/*
 * The second arguments of a float pair: each float, as the first, is
 * paired with two. One is of the kind its hash picks, of six: +0, -0, a
 * quiet NaN with its sign bit set, a signalling NaN, x itself and -x;
 * the other is drawn from it. Over every x that makes each pair of a zero
 * or a NaN with each class of float, hundreds of millions of times, and
 * every sign of y, which is what fminf, fmaxf and copysignf decide on
 * besides comparing. tests/data/math_exact_cases.h has every pair of the
 * special values.
 */
static const uint32_t float_seconds[] = { 0x00000000u, 0x80000000u, 0xffc00000u, 0x7fa00000u };
#define FLOAT_SECONDS (sizeof float_seconds / sizeof float_seconds[0])
#define FLOAT_PAIRS 2

static uint32_t float_second(uint32_t x, unsigned j) {
    uint64_t r = mix(x);
    if (j == 1) return (uint32_t)(r >> 32);
    unsigned kind = (unsigned)(r % (FLOAT_SECONDS + 2));
    if (kind < FLOAT_SECONDS) return float_seconds[kind];
    return kind == FLOAT_SECONDS ? x : x ^ 0x80000000u;
}

/*
 * The powers of 2 of ldexpf and scalbnf: each float is paired with two.
 * One is of this list, picked by its hash: the ends of int and ±300, where
 * lightc clamps; where the largest float becomes subnormal and the
 * smallest infinite; where floats near 1 become subnormal or infinite;
 * small ones. The other is drawn from it, from -330 to 330.
 */
static const int float_scales[] = {
    -2147483647 - 1, -300, -278, -277, -150, -149, -126, -1, 0, 1, 127, 128, 277, 300, 2147483647,
};
#define FLOAT_SCALES (sizeof float_scales / sizeof float_scales[0])

static int float_scale(uint32_t x, unsigned j) {
    uint64_t r = mix(x);
    if (j == 1) return (int)((r >> 32) % 661) - 330;
    return float_scales[r % FLOAT_SCALES];
}

/* A random double of sign, biased exponent in [lo, hi] and fraction drawn. */
static uint64_t random_double(uint64_t *s, int lo, int hi) {
    uint64_t r = next_random(s);
    uint64_t e = (uint64_t)lo + next_random(s) % (uint64_t)(hi - lo + 1);
    return (r & 0x800fffffffffffffu) | (e << 52);
}

static uint32_t random_float(uint64_t *s, int lo, int hi) {
    uint64_t r = next_random(s);
    uint32_t e = (uint32_t)lo + (uint32_t)(next_random(s) % (uint64_t)(hi - lo + 1));
    return ((uint32_t)r & 0x807fffffu) | (e << 23);
}

/* A random odd integer of exactly `bits` bits. */
static uint64_t random_odd(uint64_t *s, int bits) {
    uint64_t r = next_random(s);
    if (bits < 64) r &= (UINT64_C(1) << bits) - 1;
    return r | 1 | (UINT64_C(1) << (bits - 1));
}

static int bit_length(uint64_t x) { return x ? 64 - __builtin_clzll(x) : 0; }

/* A double sample of a function of one argument, of kind i % 4: any bits;
 * a magnitude from 1/16 to 2^56, where rounding to an integer has work to
 * do; an integer plus a half, give or take up to 2 ulps; an integer, give
 * or take up to 2 ulps. */
static uint64_t double_sample(uint64_t *s, uint64_t i) {
    switch (i % 4) {
    case 0:  return next_random(s);
    case 1:  return random_double(s, 1019, 1079);
    default: {
        int bits = 1 + (int)(next_random(s) % 53);
        uint64_t k = next_random(s) >> (64 - bits);
        double v = (double)k + (i % 4 == 2 && bits < 53 ? 0.5 : 0.0);
        uint64_t u = dbits(v) + (uint64_t)((int64_t)(next_random(s) % 5) - 2);
        if (v == 0.0) u = next_random(s) % 5;
        return u | (next_random(s) & 0x8000000000000000u);
    }
    }
}

static const uint64_t double_specials[] = {
    0x0000000000000000u, 0x8000000000000000u, 0x0000000000000001u, 0x3ff0000000000000u,
    0xbff0000000000000u, 0x7fefffffffffffffu, 0x7ff0000000000000u, 0xfff0000000000000u,
    0x7ff8000000000000u, 0xfff8000000000000u, 0x7ff4000000000000u, 0x0010000000000000u,
};
#define DOUBLE_SPECIALS (sizeof double_specials / sizeof double_specials[0])

/* A pair of doubles of kind i % 4: any bits; any first and a special
 * second; a first and its neighbour, or its negation; two specials. */
static void double_pair_sample(uint64_t *s, uint64_t i, uint64_t *x, uint64_t *y) {
    *x = next_random(s);
    switch (i % 4) {
    case 0: *y = next_random(s); break;
    case 1: *y = double_specials[next_random(s) % DOUBLE_SPECIALS]; break;
    case 2: {
        uint64_t r = next_random(s) % 6;
        *y = r == 5 ? *x ^ 0x8000000000000000u : *x + r - 2;
        break;
    }
    default:
        *x = double_specials[next_random(s) % DOUBLE_SPECIALS];
        *y = double_specials[next_random(s) % DOUBLE_SPECIALS];
        break;
    }
}

/*
 * A triple for fmaf, of kind i % 6:
 *   0  any bits;
 *   1  factors near 1 and an addend near their product, which cancel;
 *   2  a product that is a tie: two odd integers whose product has
 *      exactly 25 bits, halfway between two floats, scaled; the addend 0,
 *      ±2^-149 or a float far below an ulp of the product, which decides
 *      the tie, the case that rounding twice gets wrong;
 *   3  the addend minus the product rounded to a float, give or take up to
 *      2 ulps: the result is the product's rounding error;
 *   4  a product of odd integers below 2^24, scaled to a multiple of
 *      2^-150, a tie at a subnormal's last bit, and a small addend;
 *   5  factors and addend near the largest float.
 */
static void float_triple_sample(uint64_t *s, uint64_t i, uint32_t in[3]) {
    switch (i % 6) {
    case 0:
        in[0] = (uint32_t)next_random(s);
        in[1] = (uint32_t)next_random(s);
        in[2] = (uint32_t)next_random(s);
        return;
    case 1: {
        in[0] = random_float(s, 107, 147);
        in[1] = random_float(s, 107, 147);
        int e = (int)((in[0] >> 23) & 0xff) + (int)((in[1] >> 23) & 0xff) - 127;
        int lo = e - 30 < 1 ? 1 : e - 30, hi = e + 5 > 254 ? 254 : e + 5;
        in[2] = random_float(s, lo, hi);
        return;
    }
    case 2: {
        uint64_t a, b;
        do {
            int bits_a = 2 + (int)(next_random(s) % 22);
            a = random_odd(s, bits_a);
            b = random_odd(s, 26 - bits_a - (int)(next_random(s) % 2));
        } while (bit_length(a * b) != 25 || bit_length(b) > 24);
        int sa = (int)(next_random(s) % 81) - 40, sb = (int)(next_random(s) % 81) - 40;
        float x = ldexpf((float)a, sa), y = ldexpf((float)b, sb);
        double p = (double)x * (double)y;
        int ep = ilogb(p);
        float z;
        switch (next_random(s) % 4) {
        case 0:  z = 0.0f; break;
        case 1:  z = ldexpf(1.0f, -149); break;
        case 2:  z = ldexpf(1.0f + (float)(next_random(s) % 1024) / 1024.0f,
                            ep - 26 - (int)(next_random(s) % 60)); break;
        default: z = ldexpf(1.0f, ep - 24); break;
        }
        if (next_random(s) & 1) z = -z;
        if (next_random(s) & 1) x = -x;
        in[0] = fbits(x); in[1] = fbits(y); in[2] = fbits(z);
        return;
    }
    case 3: {
        in[0] = random_float(s, 64, 190);
        in[1] = random_float(s, 64, 190);
        float p = (float)((double)ffrom(in[0]) * (double)ffrom(in[1]));
        in[2] = (fbits(-p) + (uint32_t)(next_random(s) % 5)) - 2;
        return;
    }
    case 4: {
        uint64_t a, b;
        do {
            a = random_odd(s, 1 + (int)(next_random(s) % 12));
            b = random_odd(s, 1 + (int)(next_random(s) % 12));
        } while (a * b >= (UINT64_C(1) << 24));
        int k = (int)(next_random(s) % 41) - 20;
        float x = ldexpf((float)a, -75 + k), y = ldexpf((float)b, -75 - k);
        uint32_t z = (uint32_t)(next_random(s) % 8);
        if (next_random(s) & 1) z |= 0x80000000u;
        if (next_random(s) & 1) x = -x;
        in[0] = fbits(x); in[1] = fbits(y); in[2] = z;
        return;
    }
    default:
        in[0] = random_float(s, 180, 254);
        in[1] = random_float(s, 60, 127);
        in[2] = random_float(s, 200, 254);
        return;
    }
}

/* A triple for fma, of the same kinds as fmaf's, with 54-bit ties, ties
 * at 2^-1075, and a double's ranges. */
static void double_triple_sample(uint64_t *s, uint64_t i, uint64_t in[3]) {
    switch (i % 6) {
    case 0:
        in[0] = next_random(s);
        in[1] = next_random(s);
        in[2] = next_random(s);
        return;
    case 1: {
        in[0] = random_double(s, 1023 - 300, 1023 + 300);
        in[1] = random_double(s, 1023 - 300, 1023 + 300);
        int e = (int)((in[0] >> 52) & 0x7ff) + (int)((in[1] >> 52) & 0x7ff) - 1023;
        int lo = e - 110 < 1 ? 1 : e - 110, hi = e + 5 > 2046 ? 2046 : e + 5;
        in[2] = random_double(s, lo, hi);
        return;
    }
    case 2: {
        uint64_t a, b;
        unsigned __int128 p;
        do {
            int bits_a = 2 + (int)(next_random(s) % 51);
            a = random_odd(s, bits_a);
            b = random_odd(s, 55 - bits_a - (int)(next_random(s) % 2));
            p = (unsigned __int128)a * b;
        } while (bit_length((uint64_t)(p >> 1)) != 53 || bit_length(b) > 53);
        int sa = (int)(next_random(s) % 801) - 400, sb = (int)(next_random(s) % 801) - 400;
        double x = ldexp((double)a, sa), y = ldexp((double)b, sb);
        int ep = sa + sb + 53;
        double z;
        switch (next_random(s) % 4) {
        case 0:  z = 0.0; break;
        case 1:  z = ldexp(1.0, -1074); break;
        case 2:  z = ldexp(1.0 + (double)(next_random(s) % 1024) / 1024.0,
                           ep - 55 - (int)(next_random(s) % 200)); break;
        default: z = ldexp(1.0, ep - 53); break;
        }
        if (next_random(s) & 1) z = -z;
        if (next_random(s) & 1) x = -x;
        in[0] = dbits(x); in[1] = dbits(y); in[2] = dbits(z);
        return;
    }
    case 3: {
        in[0] = random_double(s, 1023 - 500, 1023 + 500);
        in[1] = random_double(s, 1023 - 500, 1023 + 500);
        double p = dfrom(in[0]) * dfrom(in[1]);
        in[2] = (dbits(-p) + next_random(s) % 5) - 2;
        return;
    }
    case 4: {
        uint64_t a, b;
        do {
            a = random_odd(s, 1 + (int)(next_random(s) % 26));
            b = random_odd(s, 1 + (int)(next_random(s) % 26));
        } while (bit_length(a) + bit_length(b) > 53 || a * b >= (UINT64_C(1) << 52));
        int k = (int)(next_random(s) % 201) - 100;
        double x = ldexp((double)a, -537 + k), y = ldexp((double)b, -538 - k);
        uint64_t z = next_random(s) % 8;
        if (next_random(s) & 1) z |= 0x8000000000000000u;
        if (next_random(s) & 1) x = -x;
        in[0] = dbits(x); in[1] = dbits(y); in[2] = z;
        return;
    }
    default:
        in[0] = random_double(s, 1800, 2046);
        in[1] = random_double(s, 900, 1023);
        in[2] = random_double(s, 1900, 2046);
        return;
    }
}

/* --- Sweeps: one chunk of a function's inputs --- */

#define CHUNK_BITS 22
#define CHUNK (UINT64_C(1) << CHUNK_BITS)
#define SAMPLE_CHUNK (UINT64_C(1) << 16)
#define BLOCK 512

/* Random samples of each function of a random kind: of a float function
 * and of a double one. */
static uint64_t float_samples = UINT64_C(1) << 32;
static uint64_t double_samples = UINT64_C(1) << 30;

static uint64_t samples_of(const function *f) {
    return f->kind == F_FFF ? float_samples : double_samples;
}

/* The floats a first argument takes: all 2^32, or 2^float_log2 of them
 * spread over every exponent, for a quicker run. */
static int float_log2 = 32;

static uint32_t float_input(uint64_t index) {
    return float_log2 == 32 ? (uint32_t)index : (uint32_t)index * 0x9e3779b1u;
}

static uint64_t float_chunk_size(void) {
    return float_log2 >= CHUNK_BITS ? CHUNK : UINT64_C(1) << float_log2;
}
static uint64_t seed = 1;

static uint64_t chunk_count(const function *f) {
    switch (f->kind) {
    case F_F: case L_F: case F_FE: case F_FI: case F_FF:
        return float_log2 >= CHUNK_BITS ? UINT64_C(1) << (float_log2 - CHUNK_BITS) : 1;
    default:
        return (samples_of(f) + SAMPLE_CHUNK - 1) / SAMPLE_CHUNK;
    }
}

static void sweep_chunk(const function *f, uint64_t chunk, tally *t) {
    uint64_t in[3] = { 0, 0, 0 };
    uint64_t first = chunk << CHUNK_BITS;
    uint64_t state = mix(seed * 0x2545f4914f6cdd1du + (uint64_t)(f - functions) * 0x9e3779b97f4a7c15u
                         + chunk);
    /* The exhaustive kinds go through their inputs a block at a time, with
     * each function called in a loop of its own over the block. */
    uint64_t size = float_chunk_size();
    float v[BLOCK], w[BLOCK], ra[BLOCK], rb[BLOCK], ro[BLOCK];
    switch (f->kind) {
    case F_F: {
        float (*a)(float) = (float (*)(float))f->lightc;
        float (*b)(float) = (float (*)(float))f->lc;
        float (*o)(float) = (float (*)(float))f->oracle;
        for (uint64_t base = 0; base < size; base += BLOCK) {
            unsigned m = size - base < BLOCK ? (unsigned)(size - base) : BLOCK;
            for (unsigned k = 0; k < m; k++) v[k] = ffrom(float_input(first + base + k));
            for (unsigned k = 0; k < m; k++) ro[k] = o(v[k]);
            for (unsigned k = 0; k < m; k++) ra[k] = a(v[k]);
            for (unsigned k = 0; k < m; k++) rb[k] = b(v[k]);
            for (unsigned k = 0; k < m; k++) {
                in[0] = fbits(v[k]);
                check_float(f, t, in, 0, fbits(ro[k]), fbits(ra[k]), fbits(rb[k]));
            }
        }
        return;
    }
    case L_F: {
        long (*a)(float) = (long (*)(float))f->lightc;
        long (*b)(float) = (long (*)(float))f->lc;
        long (*o)(float) = (long (*)(float))f->oracle;
        long la[BLOCK], lb[BLOCK], lo[BLOCK];
        for (uint64_t base = 0; base < size; base += BLOCK) {
            unsigned m = size - base < BLOCK ? (unsigned)(size - base) : BLOCK;
            for (unsigned k = 0; k < m; k++) v[k] = ffrom(float_input(first + base + k));
            for (unsigned k = 0; k < m; k++) lo[k] = o(v[k]);
            for (unsigned k = 0; k < m; k++) la[k] = a(v[k]);
            for (unsigned k = 0; k < m; k++) lb[k] = b(v[k]);
            for (unsigned k = 0; k < m; k++) {
                uint32_t x = fbits(v[k]);
                long want = expect_long(f, x, lo[k], t);
                in[0] = x;
                t->inputs++;
                if (la[k] != want) record(t, 0, in, 0, (uint64_t)la[k], (uint64_t)want, 0, 0);
                if (lb[k] != want) record(t, 1, in, 0, (uint64_t)lb[k], (uint64_t)want, 0, 0);
                t->hash += hash_of(in, 0, (uint64_t)la[k]);
            }
        }
        return;
    }
    case F_FE: {
        float (*a)(float, int *) = (float (*)(float, int *))f->lightc;
        float (*b)(float, int *) = (float (*)(float, int *))f->lc;
        float (*o)(float, int *) = (float (*)(float, int *))f->oracle;
        int ea[BLOCK], eb[BLOCK], eo[BLOCK];
        for (uint64_t base = 0; base < size; base += BLOCK) {
            unsigned m = size - base < BLOCK ? (unsigned)(size - base) : BLOCK;
            for (unsigned k = 0; k < m; k++) v[k] = ffrom(float_input(first + base + k));
            for (unsigned k = 0; k < m; k++) { eo[k] = 0; ro[k] = o(v[k], &eo[k]); }
            for (unsigned k = 0; k < m; k++) { ea[k] = 0; ra[k] = a(v[k], &ea[k]); }
            for (unsigned k = 0; k < m; k++) { eb[k] = 0; rb[k] = b(v[k], &eb[k]); }
            for (unsigned k = 0; k < m; k++) {
                uint32_t want = fbits(ro[k]), got0 = fbits(ra[k]), got1 = fbits(rb[k]);
                uint64_t ulps;
                in[0] = fbits(v[k]);
                t->inputs++;
                if (!float_passes(f, got0, want, &ulps) || ea[k] != eo[k])
                    record(t, 0, in, 0, got0, want, ea[k], eo[k]);
                if (!float_passes(f, got1, want, &ulps) || eb[k] != eo[k])
                    record(t, 1, in, 0, got1, want, eb[k], eo[k]);
                t->hash += hash_of(in, (uint64_t)(uint32_t)ea[k], float_canonical(got0));
            }
        }
        return;
    }
    case F_FI: {
        float (*a)(float, int) = (float (*)(float, int))f->lightc;
        float (*b)(float, int) = (float (*)(float, int))f->lc;
        float (*o)(float, int) = (float (*)(float, int))f->oracle;
        int n[BLOCK];
        for (uint64_t base = 0; base < size; base += BLOCK) {
            unsigned m = size - base < BLOCK ? (unsigned)(size - base) : BLOCK;
            for (unsigned k = 0; k < m; k++) v[k] = ffrom(float_input(first + base + k));
            for (unsigned j = 0; j < 2; j++) {
                for (unsigned k = 0; k < m; k++) n[k] = float_scale(fbits(v[k]), j);
                for (unsigned k = 0; k < m; k++) ro[k] = o(v[k], n[k]);
                for (unsigned k = 0; k < m; k++) ra[k] = a(v[k], n[k]);
                for (unsigned k = 0; k < m; k++) rb[k] = b(v[k], n[k]);
                for (unsigned k = 0; k < m; k++) {
                    in[0] = fbits(v[k]);
                    check_float(f, t, in, n[k], fbits(ro[k]), fbits(ra[k]), fbits(rb[k]));
                }
            }
        }
        return;
    }
    case F_FF: {
        float (*a)(float, float) = (float (*)(float, float))f->lightc;
        float (*b)(float, float) = (float (*)(float, float))f->lc;
        float (*o)(float, float) = (float (*)(float, float))f->oracle;
        for (uint64_t base = 0; base < size; base += BLOCK) {
            unsigned m = size - base < BLOCK ? (unsigned)(size - base) : BLOCK;
            for (unsigned k = 0; k < m; k++) v[k] = ffrom(float_input(first + base + k));
            for (unsigned j = 0; j < FLOAT_PAIRS; j++) {
                for (unsigned k = 0; k < m; k++) w[k] = ffrom(float_second(fbits(v[k]), j));
                for (unsigned k = 0; k < m; k++) ro[k] = o(v[k], w[k]);
                for (unsigned k = 0; k < m; k++) ra[k] = a(v[k], w[k]);
                for (unsigned k = 0; k < m; k++) rb[k] = b(v[k], w[k]);
                for (unsigned k = 0; k < m; k++) {
                    in[0] = fbits(v[k]);
                    in[1] = fbits(w[k]);
                    uint32_t want = expect_float_pair(f, (uint32_t)in[0], (uint32_t)in[1], fbits(ro[k]), t);
                    check_float(f, t, in, 0, want, fbits(ra[k]), fbits(rb[k]));
                }
            }
        }
        return;
    }
    case F_FFF: {
        float (*a)(float, float, float) = (float (*)(float, float, float))f->lightc;
        float (*b)(float, float, float) = (float (*)(float, float, float))f->lc;
        float (*o)(float, float, float) = (float (*)(float, float, float))f->oracle;
        for (uint64_t i = 0; i < SAMPLE_CHUNK && chunk * SAMPLE_CHUNK + i < samples_of(f); i++) {
            uint32_t u[3];
            float_triple_sample(&state, i, u);
            float x = ffrom(u[0]), y = ffrom(u[1]), z = ffrom(u[2]);
            in[0] = u[0]; in[1] = u[1]; in[2] = u[2];
            check_float(f, t, in, 0, fbits(o(x, y, z)), fbits(a(x, y, z)), fbits(b(x, y, z)));
        }
        return;
    }
    case D_D: {
        double (*a)(double) = (double (*)(double))f->lightc;
        double (*b)(double) = (double (*)(double))f->lc;
        double (*o)(double) = (double (*)(double))f->oracle;
        for (uint64_t i = 0; i < SAMPLE_CHUNK && chunk * SAMPLE_CHUNK + i < samples_of(f); i++) {
            uint64_t x = double_sample(&state, i);
            double v = dfrom(x);
            in[0] = x;
            check_double(f, t, in, dbits(o(v)), dbits(a(v)), dbits(b(v)));
        }
        return;
    }
    case D_DD: {
        double (*a)(double, double) = (double (*)(double, double))f->lightc;
        double (*b)(double, double) = (double (*)(double, double))f->lc;
        double (*o)(double, double) = (double (*)(double, double))f->oracle;
        for (uint64_t i = 0; i < SAMPLE_CHUNK && chunk * SAMPLE_CHUNK + i < samples_of(f); i++) {
            uint64_t x, y;
            double_pair_sample(&state, i, &x, &y);
            double v = dfrom(x), w = dfrom(y);
            in[0] = x; in[1] = y;
            check_double(f, t, in, expect_double_pair(f, x, y, dbits(o(v, w)), t), dbits(a(v, w)), dbits(b(v, w)));
        }
        return;
    }
    case D_DDD: {
        double (*a)(double, double, double) = (double (*)(double, double, double))f->lightc;
        double (*b)(double, double, double) = (double (*)(double, double, double))f->lc;
        double (*o)(double, double, double) = (double (*)(double, double, double))f->oracle;
        for (uint64_t i = 0; i < SAMPLE_CHUNK && chunk * SAMPLE_CHUNK + i < samples_of(f); i++) {
            uint64_t u[3];
            double_triple_sample(&state, i, u);
            double x = dfrom(u[0]), y = dfrom(u[1]), z = dfrom(u[2]);
            in[0] = u[0]; in[1] = u[1]; in[2] = u[2];
            check_double(f, t, in, dbits(o(x, y, z)), dbits(a(x, y, z)), dbits(b(x, y, z)));
        }
        return;
    }
    }
}

/* --- Threads --- */

typedef struct {
    const function *f;
    uint64_t        chunks;
    atomic_uint_fast64_t next;
    tally          *tallies;    /* one per chunk */
} work;

static void *worker(void *arg) {
    work *w = arg;
    for (;;) {
        uint64_t chunk = atomic_fetch_add(&w->next, 1);
        if (chunk >= w->chunks) return NULL;
        sweep_chunk(w->f, chunk, &w->tallies[chunk]);
    }
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void print_value(int kind, uint64_t bits, int argument) {
    bool is_double = kind == D_D || kind == D_DD || kind == D_DDD;
    if (kind == L_F && argument < 0) {
        printf("%ld", (long)bits);
    } else if (is_double) {
        printf("0x%016llx (%a)", (unsigned long long)bits, dfrom(bits));
    } else {
        printf("0x%08x (%a)", (unsigned)bits, (double)ffrom((uint32_t)bits));
    }
}

static int argument_count(int kind) {
    switch (kind) {
    case F_FF: case D_DD: return 2;
    case F_FFF: case D_DDD: return 3;
    default: return 1;
    }
}

static void print_mismatch(const function *f, const mismatch *m) {
    printf("    %s%s(", m->form ? "lc_" : "", f->name);
    for (int k = 0; k < argument_count(f->kind); k++) {
        if (k) printf(", ");
        print_value(f->kind, m->in[k], k);
    }
    if (f->kind == F_FI) printf(", %d", m->n);
    printf("): ");
    print_value(f->kind, m->got, -1);
    if (f->kind == F_FE) printf(" exponent %d", m->got_e);
    printf(", want ");
    print_value(f->kind, m->want, -1);
    if (f->kind == F_FE) printf(" exponent %d", m->want_e);
    printf("\n");
}

/* Sweeps one function; whether it had no mismatch. */
static bool sweep(const function *f, int threads) {
    work w = { .f = f, .chunks = chunk_count(f) };
    atomic_init(&w.next, 0);
    w.tallies = calloc(w.chunks, sizeof *w.tallies);
    if (!w.tallies) { perror("calloc"); exit(2); }
    double start = now();
    pthread_t *ids = calloc((size_t)threads, sizeof *ids);
    if (!ids) { perror("calloc"); exit(2); }
    for (int i = 0; i < threads; i++)
        if (pthread_create(&ids[i], NULL, worker, &w) != 0) { perror("pthread_create"); exit(2); }
    for (int i = 0; i < threads; i++) pthread_join(ids[i], NULL);
    double seconds = now() - start;

    tally all = { 0 };
    int shown = 0;
    for (uint64_t c = 0; c < w.chunks; c++) {
        tally *t = &w.tallies[c];
        all.inputs += t->inputs;
        all.mismatches[0] += t->mismatches[0];
        all.mismatches[1] += t->mismatches[1];
        all.by_rule += t->by_rule;
        all.hash += t->hash;
        if (t->max_ulps > all.max_ulps) all.max_ulps = t->max_ulps;
        for (int k = 0; k < t->kept && shown < 8; k++, shown++) {
            if (!shown) printf("  %s: mismatches, the first in input order:\n", f->name);
            print_mismatch(f, &t->first[k]);
        }
    }
    printf("%-11s %12llu inputs  %llu mismatches, lc_%s %llu", f->name,
           (unsigned long long)all.inputs, (unsigned long long)all.mismatches[0], f->name,
           (unsigned long long)all.mismatches[1]);
    if (all.by_rule) printf("  (%llu by lightc's rule)", (unsigned long long)all.by_rule);
    if (f->ulps) printf("  max %llu ulp", (unsigned long long)all.max_ulps);
    printf("  hash %016llx  %.1f s\n", (unsigned long long)all.hash, seconds);
    fflush(stdout);
    free(ids);
    free(w.tallies);
    return all.mismatches[0] == 0 && all.mismatches[1] == 0;
}

/* --- Hard cases --- */

/* Floats, each also negated: zeros, infinities and NaNs; subnormals and
 * the edge of the normals; around 0.5 and 1; halfway cases of each parity
 * and their neighbours; the last floats with a fraction, below 2^23, and
 * the first integers above; the edges of int and long; the largest
 * floats; squares and their neighbours. */
static const uint32_t float_cases[] = {
    0x00000000u, 0x7f800000u, 0x7fc00000u, 0x7fc00001u, 0x7fffffffu, 0x7f800001u, 0x7fbfffffu,
    0x00000001u, 0x00000002u, 0x00000003u, 0x003fffffu, 0x00400000u, 0x007fffffu,
    0x00800000u, 0x00800001u, 0x00ffffffu, 0x01000000u,
    0x3e800000u, 0x3effffffu, 0x3f000000u, 0x3f000001u, 0x3f3fffffu, 0x3f400000u,
    0x3f7fffffu, 0x3f800000u, 0x3f800001u,
    0x3fbfffffu, 0x3fc00000u, 0x3fc00001u, 0x40000000u, 0x401fffffu, 0x40200000u, 0x40200001u,
    0x40600000u, 0x40900000u, 0x41280000u, 0x41380000u,
    0x4a7fffffu, 0x4a800000u, 0x4a800001u, 0x4a800003u, 0x4afffffdu, 0x4afffffeu, 0x4affffffu,
    0x4b000000u, 0x4b000001u, 0x4b7fffffu, 0x4b800000u, 0x4b800001u,
    0x4effffffu, 0x4f000000u, 0x5effffffu, 0x5f000000u, 0x5f000001u, 0x7f000000u, 0x7f7fffffu,
    0x407fffffu, 0x40800000u, 0x40800001u, 0x41100000u, 0x3e800001u,
};

static const uint64_t double_cases[] = {
    0x0000000000000000u, 0x7ff0000000000000u, 0x7ff8000000000000u, 0x7ff8000000000001u,
    0x7fffffffffffffffu, 0x7ff0000000000001u, 0x7ff7ffffffffffffu,
    0x0000000000000001u, 0x0000000000000002u, 0x0000000000000003u, 0x0007ffffffffffffu,
    0x0008000000000000u, 0x000fffffffffffffu, 0x0010000000000000u, 0x0010000000000001u,
    0x3fd0000000000000u, 0x3fdfffffffffffffu, 0x3fe0000000000000u, 0x3fe0000000000001u,
    0x3fe7ffffffffffffu, 0x3fe8000000000000u, 0x3fefffffffffffffu, 0x3ff0000000000000u,
    0x3ff0000000000001u,
    0x3ff7ffffffffffffu, 0x3ff8000000000000u, 0x3ff8000000000001u, 0x4000000000000000u,
    0x4003ffffffffffffu, 0x4004000000000000u, 0x4004000000000001u, 0x400c000000000000u,
    0x4012000000000000u, 0x4025000000000000u, 0x4027000000000000u,
    0x431fffffffffffffu, 0x4320000000000000u, 0x4320000000000001u, 0x4320000000000003u,
    0x432ffffffffffffdu, 0x432ffffffffffffeu, 0x432fffffffffffffu, 0x4330000000000000u,
    0x4330000000000001u, 0x433fffffffffffffu, 0x4340000000000000u, 0x4340000000000001u,
    0x43dfffffffffffffu, 0x43e0000000000000u, 0x43e0000000000001u, 0x7fe0000000000000u,
    0x7fefffffffffffffu,
    0x400fffffffffffffu, 0x4010000000000000u, 0x4010000000000001u, 0x4022000000000000u,
};

/* Pairs: each of these with each. */
static const uint32_t float_pair_cases[] = {
    0x00000000u, 0x80000000u, 0x00000001u, 0x3f800000u, 0xbf800000u, 0x3fc00000u,
    0x7f7fffffu, 0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u, 0x7fa00000u,
};

static const uint64_t double_pair_cases[] = {
    0x0000000000000000u, 0x8000000000000000u, 0x0000000000000001u, 0x3ff0000000000000u,
    0xbff0000000000000u, 0x3ff8000000000000u, 0x7fefffffffffffffu, 0x7ff0000000000000u,
    0xfff0000000000000u, 0x7ff8000000000000u, 0xfff8000000000000u, 0x7ff4000000000000u,
};

/* ldexpf and scalbnf: each float with each power. */
static const uint32_t float_scale_cases[] = {
    0x00000000u, 0x80000000u, 0x00000001u, 0x00000003u, 0x007fffffu, 0x00800000u,
    0x00c00000u, 0x3f800000u, 0x3f800001u, 0xbfc00000u, 0x7f7fffffu, 0x7f800000u,
    0xff800000u, 0x7fc00000u, 0x7fa00000u,
};

static const int scale_cases[] = {
    -2147483647 - 1, -1000, -300, -278, -277, -276, -254, -253, -150, -149, -148, -127,
    -126, -125, -24, -1, 0, 1, 23, 127, 128, 253, 254, 276, 277, 278, 300, 1000, 2147483647,
};

/* fmaf: infinities and NaNs, which an infinite product or z decides; the
 * signs of zero results, also of a product that underflows; a product
 * beyond a float's range, with z that brings it back or an infinity;
 * ties, at the 25th bit and at a subnormal's last, with z that decides
 * them; and cancellations that leave the product's rounding error. */
static const uint32_t float_triple_cases[][3] = {
    { 0x7f800000u, 0x00000000u, 0x3f800000u }, { 0x00000000u, 0x7f800000u, 0x3f800000u },
    { 0x7f800000u, 0x3f800000u, 0xff800000u }, { 0x7f800000u, 0x3f800000u, 0x7f800000u },
    { 0xff800000u, 0x3f800000u, 0x7f800000u }, { 0x7f800000u, 0xbf800000u, 0x3f800000u },
    { 0x7fc00000u, 0x3f800000u, 0x3f800000u }, { 0x3f800000u, 0x7fc00000u, 0x3f800000u },
    { 0x3f800000u, 0x3f800000u, 0x7fc00000u }, { 0x7fa00000u, 0x3f800000u, 0x3f800000u },
    { 0x7f800000u, 0x00000000u, 0x7fc00000u }, { 0x3f800000u, 0x3f800000u, 0x7fa00000u },
    { 0x00000000u, 0x3f800000u, 0x00000000u }, { 0x80000000u, 0x3f800000u, 0x00000000u },
    { 0x80000000u, 0x3f800000u, 0x80000000u }, { 0x00000000u, 0xbf800000u, 0x80000000u },
    { 0x3f800000u, 0x3f800000u, 0xbf800000u }, { 0xbf800000u, 0x3f800000u, 0x3f800000u },
    { 0x0d800000u, 0x0d800000u, 0x00000000u }, { 0x8d800000u, 0x0d800000u, 0x00000000u },
    { 0x0d800000u, 0x8d800000u, 0x80000000u }, { 0x1a000000u, 0x1a000000u, 0x00000000u },
    { 0x1a000000u, 0x1a400000u, 0x00000000u }, { 0x1a000000u, 0x1a000000u, 0x00000001u },
    { 0x1a000000u, 0x1a000000u, 0x80000001u }, { 0x1a400000u, 0x1a400000u, 0x80000001u },
    { 0x7f7fffffu, 0x40000000u, 0xff800000u }, { 0x7f7fffffu, 0x7f7fffffu, 0x00000000u },
    { 0x7f7fffffu, 0x40000000u, 0xff7fffffu }, { 0x7f7fffffu, 0xc0000000u, 0x7f7fffffu },
    { 0x7f7fffffu, 0x3f800001u, 0x00000000u }, { 0x5f800000u, 0x5f800000u, 0xff000000u },
    { 0x40400000u, 0x4b000001u, 0x00000000u }, { 0x40400000u, 0x4b000001u, 0x00000001u },
    { 0x40400000u, 0x4b000001u, 0x80000001u }, { 0x40400000u, 0x4b000001u, 0x0d800000u },
    { 0x40400000u, 0x4b000001u, 0x8d800000u }, { 0xc0400000u, 0x4b000001u, 0x00000001u },
    { 0x40400000u, 0x4b000003u, 0x00000001u }, { 0x40400000u, 0x4b000003u, 0x80000001u },
    { 0x40a00000u, 0x4acccccdu, 0x00000000u }, { 0x40a00000u, 0x4acccccdu, 0x00000001u },
    { 0x3f800001u, 0x3f7fffffu, 0xbf800000u }, { 0x3f800001u, 0x3f800001u, 0xbf800002u },
    { 0x3f8ccccdu, 0x3f8ccccdu, 0xbf9ae148u }, { 0x3dcccccdu, 0x41200000u, 0xbf800000u },
    { 0x3eaaaaabu, 0x40400000u, 0xbf800000u }, { 0x00000003u, 0x3f000000u, 0x00000000u },
    { 0x00000001u, 0x3f000000u, 0x00000000u }, { 0x00000001u, 0x3f400000u, 0x80000000u },
    { 0x00000003u, 0x3f000000u, 0x00000001u }, { 0x00800000u, 0x3f7fffffu, 0x00000000u },
    { 0x00800001u, 0x3f000000u, 0x00000000u }, { 0x3f800000u, 0x3f800000u, 0x33800000u },
    { 0x3f800000u, 0x3f800000u, 0x33800001u }, { 0x3f800000u, 0x3f800000u, 0xb3800000u },
    { 0x3f800001u, 0x3f800000u, 0xb3800000u }, { 0x3f800001u, 0x3f800000u, 0x33800000u },
};

static const uint64_t double_triple_cases[][3] = {
    { 0x7ff0000000000000u, 0x0000000000000000u, 0x3ff0000000000000u },
    { 0x7ff0000000000000u, 0x3ff0000000000000u, 0xfff0000000000000u },
    { 0x7ff0000000000000u, 0x3ff0000000000000u, 0x7ff0000000000000u },
    { 0xfff0000000000000u, 0x3ff0000000000000u, 0x7ff0000000000000u },
    { 0x7ff8000000000000u, 0x3ff0000000000000u, 0x3ff0000000000000u },
    { 0x3ff0000000000000u, 0x3ff0000000000000u, 0x7ff8000000000000u },
    { 0x7ff4000000000000u, 0x3ff0000000000000u, 0x3ff0000000000000u },
    { 0x0000000000000000u, 0x3ff0000000000000u, 0x0000000000000000u },
    { 0x8000000000000000u, 0x3ff0000000000000u, 0x0000000000000000u },
    { 0x8000000000000000u, 0x3ff0000000000000u, 0x8000000000000000u },
    { 0x3ff0000000000000u, 0x3ff0000000000000u, 0xbff0000000000000u },
    { 0x2000000000000000u, 0x2000000000000000u, 0x0000000000000000u },
    { 0xa000000000000000u, 0x2000000000000000u, 0x0000000000000000u },
    { 0x2000000000000000u, 0xa000000000000000u, 0x8000000000000000u },
    { 0x1fe0000000000000u, 0x1fe0000000000000u, 0x0000000000000000u },
    { 0x1fe0000000000000u, 0x1fe8000000000000u, 0x0000000000000000u },
    { 0x1fe0000000000000u, 0x1fe0000000000000u, 0x0000000000000001u },
    { 0x1fe0000000000000u, 0x1fe0000000000000u, 0x8000000000000001u },
    { 0x7fefffffffffffffu, 0x4000000000000000u, 0xfff0000000000000u },
    { 0x7fefffffffffffffu, 0x7fefffffffffffffu, 0x0000000000000000u },
    { 0x7fefffffffffffffu, 0x4000000000000000u, 0xffefffffffffffffu },
    { 0x7fefffffffffffffu, 0xc000000000000000u, 0x7fefffffffffffffu },
    { 0x7fefffffffffffffu, 0x3ff0000000000001u, 0x0000000000000000u },
    { 0x4008000000000000u, 0x4330000000000001u, 0x0000000000000000u },
    { 0x4008000000000000u, 0x4330000000000001u, 0x0000000000000001u },
    { 0x4008000000000000u, 0x4330000000000001u, 0x8000000000000001u },
    { 0xc008000000000000u, 0x4330000000000001u, 0x0000000000000001u },
    { 0x4008000000000000u, 0x4330000000000003u, 0x0000000000000001u },
    { 0x4008000000000000u, 0x4330000000000003u, 0x8000000000000001u },
    { 0x3ff0000000000001u, 0x3fefffffffffffffu, 0xbff0000000000000u },
    { 0x3ff0000000000001u, 0x3ff0000000000001u, 0xbff0000000000002u },
    { 0x3ff199999999999au, 0x3ff199999999999au, 0xbff35c28f5c28f5cu },
    { 0x3fb999999999999au, 0x4024000000000000u, 0xbff0000000000000u },
    { 0x0000000000000003u, 0x3fe0000000000000u, 0x0000000000000000u },
    { 0x0000000000000001u, 0x3fe0000000000000u, 0x0000000000000000u },
    { 0x0000000000000001u, 0x3fe8000000000000u, 0x8000000000000000u },
    { 0x0000000000000003u, 0x3fe0000000000000u, 0x0000000000000001u },
    { 0x0010000000000000u, 0x3fefffffffffffffu, 0x0000000000000000u },
    { 0x3ff0000000000000u, 0x3ff0000000000000u, 0x3ca0000000000000u },
    { 0x3ff0000000000000u, 0x3ff0000000000000u, 0x3ca0000000000001u },
    { 0x3ff0000000000001u, 0x3ff0000000000000u, 0xbca0000000000000u },
    { 0x3ff0000000000001u, 0x3ff0000000000000u, 0x3ca0000000000000u },
};

#define COUNT(a) (sizeof a / sizeof a[0])

static uint32_t float_case_result(const function *f, uint32_t r) {
    return f->rule != NAN_BITS && fnan(r) ? 0x7fc00000u : r;
}

static uint64_t double_case_result(const function *f, uint64_t r) {
    return f->rule != NAN_BITS && dnan(r) ? 0x7ff8000000000000u : r;
}

static void print_cases(const function *f) {
    tally ignored = { 0 };
    printf("\n/* %s: %s, %s */\n", f->name, kind_names[f->kind], rule_names[f->rule]);
    switch (f->kind) {
    case F_F: case L_F: case F_FE:
        for (size_t i = 0; i < 2 * COUNT(float_cases); i++) {
            uint32_t x = float_cases[i / 2] ^ (i % 2 ? 0x80000000u : 0);
            float v = ffrom(x);
            if (f->kind == F_F) {
                uint32_t r = float_case_result(f, fbits(((float (*)(float))f->oracle)(v)));
                printf("CASE_F_F(%s, 0x%08x, 0x%08x) /* %.9g: %.9g */\n", f->name, x, r,
                       (double)v, (double)ffrom(r));
            } else if (f->kind == L_F) {
                long r = expect_long(f, x, ((long (*)(float))f->oracle)(v), &ignored);
                /* LONG_MIN is no decimal literal of a long. */
                if (r == LONG_MIN)
                    printf("CASE_L_F(%s, 0x%08x, (-%ldL - 1)) /* %.9g */\n", f->name, x, LONG_MAX, (double)v);
                else
                    printf("CASE_L_F(%s, 0x%08x, %ldL) /* %.9g */\n", f->name, x, r, (double)v);
            } else {
                int e = 0;
                uint32_t r = float_case_result(f, fbits(((float (*)(float, int *))f->oracle)(v, &e)));
                printf("CASE_F_FE(%s, 0x%08x, 0x%08x, %d) /* %.9g: %.9g, %d */\n", f->name, x, r, e,
                       (double)v, (double)ffrom(r), e);
            }
        }
        return;
    case D_D:
        for (size_t i = 0; i < 2 * COUNT(double_cases); i++) {
            uint64_t x = double_cases[i / 2] ^ (i % 2 ? 0x8000000000000000u : 0);
            uint64_t r = double_case_result(f, dbits(((double (*)(double))f->oracle)(dfrom(x))));
            printf("CASE_D_D(%s, 0x%016llx, 0x%016llx) /* %.17g: %.17g */\n", f->name,
                   (unsigned long long)x, (unsigned long long)r, dfrom(x), dfrom(r));
        }
        return;
    case F_FI:
        for (size_t i = 0; i < COUNT(float_scale_cases); i++)
            for (size_t j = 0; j < COUNT(scale_cases); j++) {
                uint32_t x = float_scale_cases[i];
                int n = scale_cases[j];
                uint32_t r = float_case_result(f, fbits(((float (*)(float, int))f->oracle)(ffrom(x), n)));
                printf("CASE_F_FI(%s, 0x%08x, %d, 0x%08x) /* %.9g, %d: %.9g */\n", f->name, x, n, r,
                       (double)ffrom(x), n, (double)ffrom(r));
            }
        return;
    case F_FF:
        for (size_t i = 0; i < COUNT(float_pair_cases); i++)
            for (size_t j = 0; j < COUNT(float_pair_cases); j++) {
                uint32_t x = float_pair_cases[i], y = float_pair_cases[j];
                float o = ((float (*)(float, float))f->oracle)(ffrom(x), ffrom(y));
                uint32_t r = float_case_result(f, expect_float_pair(f, x, y, fbits(o), &ignored));
                printf("CASE_F_FF(%s, 0x%08x, 0x%08x, 0x%08x) /* %.9g, %.9g: %.9g */\n", f->name,
                       x, y, r, (double)ffrom(x), (double)ffrom(y), (double)ffrom(r));
            }
        return;
    case D_DD:
        for (size_t i = 0; i < COUNT(double_pair_cases); i++)
            for (size_t j = 0; j < COUNT(double_pair_cases); j++) {
                uint64_t x = double_pair_cases[i], y = double_pair_cases[j];
                double o = ((double (*)(double, double))f->oracle)(dfrom(x), dfrom(y));
                uint64_t r = double_case_result(f, expect_double_pair(f, x, y, dbits(o), &ignored));
                printf("CASE_D_DD(%s, 0x%016llx, 0x%016llx, 0x%016llx) /* %.17g, %.17g: %.17g */\n",
                       f->name, (unsigned long long)x, (unsigned long long)y, (unsigned long long)r,
                       dfrom(x), dfrom(y), dfrom(r));
            }
        return;
    case F_FFF:
        for (size_t i = 0; i < COUNT(float_triple_cases); i++) {
            const uint32_t *u = float_triple_cases[i];
            float x = ffrom(u[0]), y = ffrom(u[1]), z = ffrom(u[2]);
            uint32_t r = float_case_result(f, fbits(((float (*)(float, float, float))f->oracle)(x, y, z)));
            printf("CASE_F_FFF(%s, 0x%08x, 0x%08x, 0x%08x, 0x%08x) /* %.9g, %.9g, %.9g: %.9g */\n",
                   f->name, u[0], u[1], u[2], r, (double)x, (double)y, (double)z, (double)ffrom(r));
        }
        return;
    case D_DDD:
        for (size_t i = 0; i < COUNT(double_triple_cases); i++) {
            const uint64_t *u = double_triple_cases[i];
            double x = dfrom(u[0]), y = dfrom(u[1]), z = dfrom(u[2]);
            uint64_t r = double_case_result(f, dbits(((double (*)(double, double, double))f->oracle)(x, y, z)));
            printf("CASE_D_DDD(%s, 0x%016llx, 0x%016llx, 0x%016llx, 0x%016llx) /* %.17g, %.17g, %.17g: %.17g */\n",
                   f->name, (unsigned long long)u[0], (unsigned long long)u[1], (unsigned long long)u[2],
                   (unsigned long long)r, x, y, z, dfrom(r));
        }
        return;
    }
}

/* --- main --- */

static const function *find(const char *name) {
    for (size_t i = 0; i < FUNCTION_COUNT; i++)
        if (strcmp(functions[i].name, name) == 0) return &functions[i];
    return NULL;
}

static void usage(void) {
    fprintf(stderr, "usage: math-sweep [-t THREADS] [-n LOG2] [-e LOG2] [-s SEED] [NAME...]\n"
                    "       math-sweep -l\n"
                    "       math-sweep -c [NAME...]\n");
    exit(2);
}

int main(int argc, char **argv) {
    long threads = sysconf(_SC_NPROCESSORS_ONLN);
    bool list = false, cases = false;
    int opt;
    while ((opt = getopt(argc, argv, "t:n:e:s:lc")) != -1) {
        switch (opt) {
        case 't': threads = strtol(optarg, NULL, 10); break;
        case 'n': {
            long log2 = strtol(optarg, NULL, 10);
            if (log2 < 0 || log2 > 40) usage();
            float_samples = double_samples = UINT64_C(1) << log2;
            break;
        }
        case 'e':
            float_log2 = (int)strtol(optarg, NULL, 10);
            if (float_log2 < 0 || float_log2 > 32) usage();
            break;
        case 's': seed = strtoull(optarg, NULL, 0); break;
        case 'l': list = true; break;
        case 'c': cases = true; break;
        default: usage();
        }
    }
    if (threads < 1) threads = 1;

    const function *chosen[FUNCTION_COUNT];
    size_t count = 0;
    if (optind == argc) {
        for (size_t i = 0; i < FUNCTION_COUNT; i++) chosen[count++] = &functions[i];
    } else {
        for (int i = optind; i < argc; i++) {
            const function *f = find(argv[i]);
            if (!f) { fprintf(stderr, "math-sweep: no function %s; -l lists them\n", argv[i]); return 2; }
            if (count < FUNCTION_COUNT) chosen[count++] = f;
        }
    }

    if (list) {
        for (size_t i = 0; i < FUNCTION_COUNT; i++)
            printf("%-11s %-31s %u ulp  %s\n", functions[i].name, kind_names[functions[i].kind],
                   functions[i].ulps, rule_names[functions[i].rule]);
        return 0;
    }

    if (cases) {
        printf("/*\n"
               " * Hard cases of lightc's math functions, which tests/test_math_exact.c\n"
               " * includes, written by \"build/math-sweep -c\": do not edit, regenerate.\n"
               " * Each line is CASE_<kind>(name, inputs..., result), the bits of each\n"
               " * value, and the result is the host libm's, glibc %s here, or\n"
               " * lightc's rule where C leaves it open; a NaN is written as the default\n"
               " * quiet NaN, which stands for any quiet NaN, but for fabsf and\n"
               " * copysignf. tools/math-sweep.c lists the inputs and the rules.\n"
               " */\n", gnu_get_libc_version());
        for (size_t i = 0; i < count; i++) print_cases(chosen[i]);
        return 0;
    }

    printf("math-sweep: %zu functions, %ld threads, 2^%d floats, 2^%d samples of fmaf and 2^%d of each "
           "double function, seed %llu, against glibc %s\n", count, threads, float_log2,
           __builtin_ctzll(float_samples), __builtin_ctzll(double_samples), (unsigned long long)seed,
           gnu_get_libc_version());
    double start = now();
    size_t failed = 0;
    for (size_t i = 0; i < count; i++)
        if (!sweep(chosen[i], (int)threads)) failed++;
    printf("%s: %zu functions, %zu with mismatches, %.1f s\n", failed ? "FAIL" : "PASS", count, failed,
           now() - start);
    return failed ? 1 : 0;
}

#endif

/*
 * test_math — <lightc/math.h>: the bits of a value, the classification
 * macros and the constants.
 *
 * Each classification macro, and the function it calls for each type, is
 * checked on the special values of float and of double: both zeros, the
 * smallest and largest subnormals, the smallest normal, ±1, the largest
 * finite values, both infinities, and quiet and signalling NaNs of both
 * signs, with the smallest and largest payloads. Each value is made from
 * its bits, read through a volatile so that the compiler cannot fold the
 * test, and the constants are also tested as constants, which it folds.
 * The bits of π, e and ln 2 that the tests expect were computed apart
 * from any C compiler, by rounding 70-digit expansions of each to the
 * nearest double and float with exact rational arithmetic.
 *
 * configure builds this file twice: as test_math, with the build's flags,
 * and as test_math_fast, with -O3 -ffast-math added, under which the
 * header says the macros give the same answers. There the tests whose code
 * does arithmetic on a NaN, an infinity or a signed zero, or converts one,
 * are left out, since -ffast-math lets the compiler assume no such value
 * arises; every value the other tests classify is made from its bits, or
 * is a constant.
 *
 * Linking at all is a check too: lightc has no libm, so a call that the
 * header made the compiler emit to one of libm's functions, at -O0 in the
 * debug build or at -O3 in test_math_fast, would fail the link.
 */

#include "test.h"
#include <float.h>
#include <lightc/math.h>

/* Whether this build keeps IEEE 754's rules for NaNs, infinities and
 * signed zeros, as test_math does and test_math_fast does not. */
#if defined(__FAST_MATH__) || __FINITE_MATH_ONLY__ || defined(__NO_SIGNED_ZEROS__)
#define IEEE_RULES 0
#else
#define IEEE_RULES 1
#endif

/* --- The types of the macros --- */

_Static_assert(_Generic(LC_INFINITY, float: 1, default: 0), "LC_INFINITY is a float");
_Static_assert(_Generic(LC_NAN, float: 1, default: 0), "LC_NAN is a float");
_Static_assert(_Generic(LC_PI, double: 1, default: 0), "LC_PI is a double");
_Static_assert(_Generic(LC_E, double: 1, default: 0), "LC_E is a double");
_Static_assert(_Generic(LC_LN2, double: 1, default: 0), "LC_LN2 is a double");
_Static_assert(_Generic(LC_PI_F, float: 1, default: 0), "LC_PI_F is a float");
_Static_assert(_Generic(LC_E_F, float: 1, default: 0), "LC_E_F is a float");
_Static_assert(_Generic(LC_LN2_F, float: 1, default: 0), "LC_LN2_F is a float");
_Static_assert(_Generic(lc_isnan(1.0f), bool: 1, default: 0), "lc_isnan gives a bool");
_Static_assert(_Generic(lc_isinf(1.0), bool: 1, default: 0), "lc_isinf gives a bool");
_Static_assert(_Generic(lc_isfinite(1.0f), bool: 1, default: 0), "lc_isfinite gives a bool");
_Static_assert(_Generic(lc_signbit(1.0), bool: 1, default: 0), "lc_signbit gives a bool");

/* --- The special values --- */

#define NAN_ (1 << 0)
#define INF_ (1 << 1)
#define FIN_ (1 << 2)
#define NEG_ (1 << 3)

typedef struct {
    uint32_t bits;
    int      class;   /* NAN_, INF_ or FIN_, and NEG_ when the sign is set */
} float_case;

typedef struct {
    uint64_t bits;
    int      class;
} double_case;

static const float_case float_cases[] = {
    { 0x00000000u, FIN_ },          /* +0 */
    { 0x80000000u, FIN_ | NEG_ },   /* -0 */
    { 0x00000001u, FIN_ },          /* smallest subnormal */
    { 0x80000001u, FIN_ | NEG_ },
    { 0x007fffffu, FIN_ },          /* largest subnormal */
    { 0x807fffffu, FIN_ | NEG_ },
    { 0x00800000u, FIN_ },          /* smallest normal */
    { 0x3f800000u, FIN_ },          /* 1 */
    { 0xbf800000u, FIN_ | NEG_ },   /* -1 */
    { 0x7f7fffffu, FIN_ },          /* largest finite */
    { 0xff7fffffu, FIN_ | NEG_ },
    { 0x7f800000u, INF_ },          /* +infinity */
    { 0xff800000u, INF_ | NEG_ },   /* -infinity */
    { 0x7fc00000u, NAN_ },          /* quiet NaN, no payload */
    { 0xffc00000u, NAN_ | NEG_ },
    { 0x7fffffffu, NAN_ },          /* quiet NaN, largest payload */
    { 0xffffffffu, NAN_ | NEG_ },
    { 0x7f800001u, NAN_ },          /* signalling NaN, smallest payload */
    { 0xff800001u, NAN_ | NEG_ },
    { 0x7fbfffffu, NAN_ },          /* signalling NaN, largest payload */
    { 0xffbfffffu, NAN_ | NEG_ },
};

static const double_case double_cases[] = {
    { 0x0000000000000000u, FIN_ },          /* +0 */
    { 0x8000000000000000u, FIN_ | NEG_ },   /* -0 */
    { 0x0000000000000001u, FIN_ },          /* smallest subnormal */
    { 0x8000000000000001u, FIN_ | NEG_ },
    { 0x000fffffffffffffu, FIN_ },          /* largest subnormal */
    { 0x800fffffffffffffu, FIN_ | NEG_ },
    { 0x0010000000000000u, FIN_ },          /* smallest normal */
    { 0x3ff0000000000000u, FIN_ },          /* 1 */
    { 0xbff0000000000000u, FIN_ | NEG_ },   /* -1 */
    { 0x7fefffffffffffffu, FIN_ },          /* largest finite */
    { 0xffefffffffffffffu, FIN_ | NEG_ },
    { 0x7ff0000000000000u, INF_ },          /* +infinity */
    { 0xfff0000000000000u, INF_ | NEG_ },   /* -infinity */
    { 0x7ff8000000000000u, NAN_ },          /* quiet NaN, no payload */
    { 0xfff8000000000000u, NAN_ | NEG_ },
    { 0x7fffffffffffffffu, NAN_ },          /* quiet NaN, largest payload */
    { 0xffffffffffffffffu, NAN_ | NEG_ },
    { 0x7ff0000000000001u, NAN_ },          /* signalling NaN, smallest payload */
    { 0xfff0000000000001u, NAN_ | NEG_ },
    { 0x7ff7ffffffffffffu, NAN_ },          /* signalling NaN, largest payload */
    { 0xfff7ffffffffffffu, NAN_ | NEG_ },
};

#define FLOAT_CASES  (sizeof float_cases / sizeof float_cases[0])
#define DOUBLE_CASES (sizeof double_cases / sizeof double_cases[0])

/* The class the four macros give `x`, in the bits above. */
static int float_class(float x) {
    return (lc_isnan(x) ? NAN_ : 0) | (lc_isinf(x) ? INF_ : 0) |
           (lc_isfinite(x) ? FIN_ : 0) | (lc_signbit(x) ? NEG_ : 0);
}

static int double_class(double x) {
    return (lc_isnan(x) ? NAN_ : 0) | (lc_isinf(x) ? INF_ : 0) |
           (lc_isfinite(x) ? FIN_ : 0) | (lc_signbit(x) ? NEG_ : 0);
}

/* The class the functions for one type give `x`, which must be the
 * macros' own. */
static int float_functions_class(float x) {
    return (lc_float_isnan(x) ? NAN_ : 0) | (lc_float_isinf(x) ? INF_ : 0) |
           (lc_float_isfinite(x) ? FIN_ : 0) | (lc_float_signbit(x) ? NEG_ : 0);
}

static int double_functions_class(double x) {
    return (lc_double_isnan(x) ? NAN_ : 0) | (lc_double_isinf(x) ? INF_ : 0) |
           (lc_double_isfinite(x) ? FIN_ : 0) | (lc_double_signbit(x) ? NEG_ : 0);
}

/* Prints the bits of the case a failure is about. */
static void print_case(const char *what, uint64_t bits) {
    _test_print_text("  ");
    _test_print_text(what);
    _test_print_text(" ");
    lc_print_hex(TEST_FD, bits);
    lc_print_newline(TEST_FD);
}

/* --- The bits of a value --- */

static void test_float_bits_layout(void) {
    TEST_ASSERT_EQ(lc_float_to_bits(1.0f), 0x3f800000u);
    TEST_ASSERT_EQ(lc_float_to_bits(-2.0f), 0xc0000000u);
    TEST_ASSERT_EQ(lc_float_to_bits(0.5f), 0x3f000000u);
    TEST_ASSERT(lc_float_from_bits(0x3f800000u) == 1.0f);
    TEST_ASSERT(lc_float_from_bits(0xc0000000u) == -2.0f);
    /* <float.h> is the compiler's, and works beside this header. */
    TEST_ASSERT_EQ(lc_float_to_bits(FLT_MAX), 0x7f7fffffu);
    TEST_ASSERT_EQ(lc_float_to_bits(FLT_MIN), 0x00800000u);
    TEST_ASSERT_EQ(lc_float_to_bits(FLT_TRUE_MIN), 0x00000001u);
    TEST_ASSERT_EQ(lc_float_to_bits(FLT_EPSILON), 0x34000000u);
}

static void test_double_bits_layout(void) {
    TEST_ASSERT_EQ(lc_double_to_bits(1.0), 0x3ff0000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(-2.0), 0xc000000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(0.5), 0x3fe0000000000000u);
    TEST_ASSERT(lc_double_from_bits(0x3ff0000000000000u) == 1.0);
    TEST_ASSERT(lc_double_from_bits(0xc000000000000000u) == -2.0);
    TEST_ASSERT_EQ(lc_double_to_bits(DBL_MAX), 0x7fefffffffffffffu);
    TEST_ASSERT_EQ(lc_double_to_bits(DBL_MIN), 0x0010000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(DBL_TRUE_MIN), 0x0000000000000001u);
    TEST_ASSERT_EQ(lc_double_to_bits(DBL_EPSILON), 0x3cb0000000000000u);
}

/* Every special value comes back from its bits unchanged, a signalling
 * NaN too: nothing on the way does arithmetic, which would quiet it. */
static void test_float_bits_round_trip(void) {
    for (size_t i = 0; i < FLOAT_CASES; i++) {
        volatile uint32_t bits = float_cases[i].bits;
        volatile float x = lc_float_from_bits(bits);
        uint32_t back = lc_float_to_bits(x);
        if (back != float_cases[i].bits) print_case("float", float_cases[i].bits);
        TEST_ASSERT_EQ(back, float_cases[i].bits);
    }
}

static void test_double_bits_round_trip(void) {
    for (size_t i = 0; i < DOUBLE_CASES; i++) {
        volatile uint64_t bits = double_cases[i].bits;
        volatile double x = lc_double_from_bits(bits);
        uint64_t back = lc_double_to_bits(x);
        if (back != double_cases[i].bits) print_case("double", double_cases[i].bits);
        TEST_ASSERT_EQ(back, double_cases[i].bits);
    }
}

/* --- Classification --- */

static void test_classify_float(void) {
    for (size_t i = 0; i < FLOAT_CASES; i++) {
        volatile uint32_t bits = float_cases[i].bits;
        float x = lc_float_from_bits(bits);
        int macros = float_class(x), functions = float_functions_class(x);
        if (macros != float_cases[i].class || functions != float_cases[i].class)
            print_case("float", float_cases[i].bits);
        TEST_ASSERT_EQ(macros, float_cases[i].class);
        TEST_ASSERT_EQ(functions, float_cases[i].class);
    }
}

static void test_classify_double(void) {
    for (size_t i = 0; i < DOUBLE_CASES; i++) {
        volatile uint64_t bits = double_cases[i].bits;
        double x = lc_double_from_bits(bits);
        int macros = double_class(x), functions = double_functions_class(x);
        if (macros != double_cases[i].class || functions != double_cases[i].class)
            print_case("double", double_cases[i].bits);
        TEST_ASSERT_EQ(macros, double_cases[i].class);
        TEST_ASSERT_EQ(functions, double_cases[i].class);
    }
}

/* The same values as constants, which the compiler can fold. */
static void test_classify_constants(void) {
    TEST_ASSERT(lc_isnan(LC_NAN));
    TEST_ASSERT(!lc_isinf(LC_NAN));
    TEST_ASSERT(!lc_isfinite(LC_NAN));
    TEST_ASSERT(!lc_signbit(LC_NAN));
    TEST_ASSERT(lc_isinf(LC_INFINITY));
    TEST_ASSERT(!lc_isnan(LC_INFINITY));
    TEST_ASSERT(!lc_isfinite(LC_INFINITY));
    TEST_ASSERT(!lc_signbit(LC_INFINITY));
    TEST_ASSERT(lc_isnan(lc_float_from_bits(0x7f800001u)));
    TEST_ASSERT(lc_isnan(lc_float_from_bits(0xffc00000u)));
    TEST_ASSERT(lc_signbit(lc_float_from_bits(0xffc00000u)));
    TEST_ASSERT(lc_signbit(lc_float_from_bits(0x80000000u)));
    TEST_ASSERT(lc_isfinite(lc_float_from_bits(0x80000000u)));
    TEST_ASSERT(lc_isinf(lc_float_from_bits(0xff800000u)));
    TEST_ASSERT(lc_isfinite(FLT_MAX));
    TEST_ASSERT(lc_isfinite(FLT_TRUE_MIN));
    TEST_ASSERT(lc_isnan(lc_double_from_bits(0x7ff0000000000001u)));
    TEST_ASSERT(lc_isnan(lc_double_from_bits(0xfff8000000000000u)));
    TEST_ASSERT(lc_signbit(lc_double_from_bits(0xfff8000000000000u)));
    TEST_ASSERT(lc_signbit(lc_double_from_bits(0x8000000000000000u)));
    TEST_ASSERT(lc_isinf(lc_double_from_bits(0x7ff0000000000000u)));
    TEST_ASSERT(lc_isfinite(DBL_MAX));
    TEST_ASSERT(lc_isfinite(DBL_TRUE_MIN));
    TEST_ASSERT(!lc_isfinite(lc_double_from_bits(0x7ff0000000000000u)));
}

/* Each macro evaluates `x` once, as each case below steps p by one. */
static void test_classify_evaluates_once(void) {
    float floats[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    double doubles[4] = { 1.0, 2.0, 3.0, 4.0 };
    const float *p = floats;
    const double *q = doubles;
    TEST_ASSERT(!lc_isnan(*p++));
    TEST_ASSERT(!lc_isinf(*p++));
    TEST_ASSERT(lc_isfinite(*p++));
    TEST_ASSERT(!lc_signbit(*p++));
    TEST_ASSERT(p == floats + 4);
    TEST_ASSERT(!lc_isnan(*q++));
    TEST_ASSERT(!lc_isinf(*q++));
    TEST_ASSERT(lc_isfinite(*q++));
    TEST_ASSERT(!lc_signbit(*q++));
    TEST_ASSERT(q == doubles + 4);
}

/* A const or volatile operand picks the function of its type. The negative
 * zero is made from bits the compiler cannot see: under -ffast-math, which
 * test_math_fast is built with, GCC may store a folded -0.0 as +0.0, as it
 * does on aarch64 with the zero register. */
static void test_classify_qualified(void) {
    const float infinity = lc_float_from_bits(0x7f800000u);
    volatile double nan = lc_double_from_bits(0xfff8000000000000u);
    volatile uint32_t negative_zero_bits = 0x80000000u;
    const volatile float negative_zero = lc_float_from_bits(negative_zero_bits);
    TEST_ASSERT(lc_isinf(infinity));
    TEST_ASSERT(lc_isnan(nan));
    TEST_ASSERT(lc_signbit(nan));
    TEST_ASSERT(lc_signbit(negative_zero));
    TEST_ASSERT(lc_isfinite(negative_zero));
}

#if IEEE_RULES
/* Values the hardware makes at run time from operands the compiler cannot
 * see. The sign of the NaN that 0 / 0 makes is the arch's: x86_64 sets it
 * and aarch64 does not, so it is not checked. */
static void test_classify_arithmetic(void) {
    volatile float fzero = 0.0f, fone = 1.0f, fbig = 1e30f, ftiny = 1e-30f;
    volatile float f;
    f = fzero / fzero;
    TEST_ASSERT(lc_isnan(f));
    f = fone / fzero;
    TEST_ASSERT(lc_isinf(f) && !lc_signbit(f));
    f = -fone / fzero;
    TEST_ASSERT(lc_isinf(f) && lc_signbit(f));
    f = fbig * fbig;
    TEST_ASSERT(lc_isinf(f) && !lc_isfinite(f));
    volatile float g = f;
    f = f - g;
    TEST_ASSERT(lc_isnan(f));
    f = -ftiny * ftiny;
    TEST_ASSERT(lc_signbit(f) && lc_isfinite(f) && lc_float_to_bits(f) == 0x80000000u);
    f = -fone * fzero;
    TEST_ASSERT(lc_signbit(f) && lc_float_to_bits(f) == 0x80000000u);

    volatile double dzero = 0.0, done = 1.0, dbig = 1e300, dtiny = 1e-300;
    volatile double d;
    d = dzero / dzero;
    TEST_ASSERT(lc_isnan(d));
    d = done / dzero;
    TEST_ASSERT(lc_isinf(d) && !lc_signbit(d));
    d = -done / dzero;
    TEST_ASSERT(lc_isinf(d) && lc_signbit(d));
    d = dbig * dbig;
    TEST_ASSERT(lc_isinf(d) && !lc_isfinite(d));
    volatile double e = d;
    d = d - e;
    TEST_ASSERT(lc_isnan(d));
    d = -dtiny * dtiny;
    TEST_ASSERT(lc_signbit(d) && lc_isfinite(d) && lc_double_to_bits(d) == 0x8000000000000000u);
}

/* A float converts to a double of the same class and sign, as the header
 * says LC_INFINITY and LC_NAN do. */
static void test_classify_converted(void) {
    for (size_t i = 0; i < FLOAT_CASES; i++) {
        volatile uint32_t bits = float_cases[i].bits;
        double x = (double)lc_float_from_bits(bits);
        int class = double_class(x);
        if (class != float_cases[i].class) print_case("float to double", float_cases[i].bits);
        TEST_ASSERT_EQ(class, float_cases[i].class);
    }
}
#endif

/* --- Constants --- */

static void test_constant_bits(void) {
    TEST_ASSERT_EQ(lc_float_to_bits(LC_INFINITY), 0x7f800000u);
    TEST_ASSERT_EQ(lc_float_to_bits(LC_NAN), 0x7fc00000u);
    TEST_ASSERT_EQ(lc_double_to_bits(LC_PI), 0x400921fb54442d18u);
    TEST_ASSERT_EQ(lc_double_to_bits(LC_E), 0x4005bf0a8b145769u);
    TEST_ASSERT_EQ(lc_double_to_bits(LC_LN2), 0x3fe62e42fefa39efu);
    TEST_ASSERT_EQ(lc_float_to_bits(LC_PI_F), 0x40490fdbu);
    TEST_ASSERT_EQ(lc_float_to_bits(LC_E_F), 0x402df854u);
    TEST_ASSERT_EQ(lc_float_to_bits(LC_LN2_F), 0x3f317218u);
}

/* The constants are constant expressions, which a static initializer can
 * use, with the same bits. */
static const float static_floats[] = { LC_INFINITY, LC_NAN, LC_PI_F, LC_E_F, LC_LN2_F };
static const double static_doubles[] = { LC_INFINITY, LC_NAN, LC_PI, LC_E, LC_LN2 };

static void test_constant_static(void) {
    TEST_ASSERT_EQ(lc_float_to_bits(static_floats[0]), 0x7f800000u);
    TEST_ASSERT_EQ(lc_float_to_bits(static_floats[1]), 0x7fc00000u);
    TEST_ASSERT_EQ(lc_float_to_bits(static_floats[2]), 0x40490fdbu);
    TEST_ASSERT_EQ(lc_float_to_bits(static_floats[3]), 0x402df854u);
    TEST_ASSERT_EQ(lc_float_to_bits(static_floats[4]), 0x3f317218u);
    TEST_ASSERT_EQ(lc_double_to_bits(static_doubles[0]), 0x7ff0000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(static_doubles[1]), 0x7ff8000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits(static_doubles[2]), 0x400921fb54442d18u);
    TEST_ASSERT_EQ(lc_double_to_bits(static_doubles[3]), 0x4005bf0a8b145769u);
    TEST_ASSERT_EQ(lc_double_to_bits(static_doubles[4]), 0x3fe62e42fefa39efu);
}

#if IEEE_RULES
/* LC_INFINITY and LC_NAN as doubles, negated, and the arithmetic of the
 * constants, as the compiler folds it. */
static void test_constant_arithmetic(void) {
    TEST_ASSERT_EQ(lc_double_to_bits((double)LC_INFINITY), 0x7ff0000000000000u);
    TEST_ASSERT_EQ(lc_double_to_bits((double)LC_NAN), 0x7ff8000000000000u);
    TEST_ASSERT_EQ(lc_float_to_bits(-LC_INFINITY), 0xff800000u);
    TEST_ASSERT_EQ(lc_float_to_bits(-LC_NAN), 0xffc00000u);
    TEST_ASSERT(lc_isinf(-LC_INFINITY) && lc_signbit(-LC_INFINITY));
    TEST_ASSERT(lc_isnan(-LC_NAN) && lc_signbit(-LC_NAN));
    TEST_ASSERT(lc_isinf((double)LC_INFINITY));
    TEST_ASSERT(lc_isnan((double)LC_NAN));
    TEST_ASSERT(lc_signbit(-0.0f) && !lc_signbit(0.0f));
    TEST_ASSERT(lc_signbit(-0.0) && !lc_signbit(0.0));
    TEST_ASSERT(lc_isnan(LC_INFINITY - LC_INFINITY));
    TEST_ASSERT(lc_isnan(LC_NAN * 0.0f));
    TEST_ASSERT(lc_isinf(FLT_MAX * 2.0f));
    TEST_ASSERT(LC_NAN != LC_NAN);
    TEST_ASSERT(LC_INFINITY > FLT_MAX);
}
#endif

int main(int argc, char **argv, char **envp) {
    (void)envp;
    test_init(argc, argv);

    /* The bits of a value */
    TEST_RUN(test_float_bits_layout);
    TEST_RUN(test_double_bits_layout);
    TEST_RUN(test_float_bits_round_trip);
    TEST_RUN(test_double_bits_round_trip);

    /* Classification */
    TEST_RUN(test_classify_float);
    TEST_RUN(test_classify_double);
    TEST_RUN(test_classify_constants);
    TEST_RUN(test_classify_evaluates_once);
    TEST_RUN(test_classify_qualified);
#if IEEE_RULES
    TEST_RUN(test_classify_arithmetic);
    TEST_RUN(test_classify_converted);
#endif

    /* Constants */
    TEST_RUN(test_constant_bits);
    TEST_RUN(test_constant_static);
#if IEEE_RULES
    TEST_RUN(test_constant_arithmetic);
#endif

    return test_main();
}

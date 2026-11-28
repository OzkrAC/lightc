/*
 * Hard cases of lightc's math functions, which tests/test_math_exact.c
 * includes, written by "build/math-sweep -c": do not edit, regenerate.
 * Each line is CASE_<kind>(name, inputs..., result), the bits of each
 * value, and the result is the host libm's, glibc 2.41 here, or
 * lightc's rule where C leaves it open; a NaN is written as the default
 * quiet NaN, which stands for any quiet NaN, but for fabsf and
 * copysignf. tools/math-sweep.c lists the inputs and the rules.
 */

/* sqrtf: float(float), NAN_QUIET */
CASE_F_F(sqrtf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(sqrtf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(sqrtf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(sqrtf, 0xff800000, 0x7fc00000) /* -inf: nan */
CASE_F_F(sqrtf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(sqrtf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(sqrtf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(sqrtf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(sqrtf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(sqrtf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(sqrtf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(sqrtf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(sqrtf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(sqrtf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(sqrtf, 0x00000001, 0x1a3504f3) /* 1.40129846e-45: 3.74339207e-23 */
CASE_F_F(sqrtf, 0x80000001, 0x7fc00000) /* -1.40129846e-45: nan */
CASE_F_F(sqrtf, 0x00000002, 0x1a800000) /* 2.80259693e-45: 5.29395592e-23 */
CASE_F_F(sqrtf, 0x80000002, 0x7fc00000) /* -2.80259693e-45: nan */
CASE_F_F(sqrtf, 0x00000003, 0x1a9cc471) /* 4.20389539e-45: 6.4837456e-23 */
CASE_F_F(sqrtf, 0x80000003, 0x7fc00000) /* -4.20389539e-45: nan */
CASE_F_F(sqrtf, 0x003fffff, 0x1fb504f2) /* 5.87747035e-39: 7.66646631e-20 */
CASE_F_F(sqrtf, 0x803fffff, 0x7fc00000) /* -5.87747035e-39: nan */
CASE_F_F(sqrtf, 0x00400000, 0x1fb504f3) /* 5.87747175e-39: 7.66646695e-20 */
CASE_F_F(sqrtf, 0x80400000, 0x7fc00000) /* -5.87747175e-39: nan */
CASE_F_F(sqrtf, 0x007fffff, 0x1fffffff) /* 1.17549421e-38: 1.08420211e-19 */
CASE_F_F(sqrtf, 0x807fffff, 0x7fc00000) /* -1.17549421e-38: nan */
CASE_F_F(sqrtf, 0x00800000, 0x20000000) /* 1.17549435e-38: 1.08420217e-19 */
CASE_F_F(sqrtf, 0x80800000, 0x7fc00000) /* -1.17549435e-38: nan */
CASE_F_F(sqrtf, 0x00800001, 0x20000000) /* 1.17549449e-38: 1.08420217e-19 */
CASE_F_F(sqrtf, 0x80800001, 0x7fc00000) /* -1.17549449e-38: nan */
CASE_F_F(sqrtf, 0x00ffffff, 0x203504f3) /* 2.35098856e-38: 1.53329339e-19 */
CASE_F_F(sqrtf, 0x80ffffff, 0x7fc00000) /* -2.35098856e-38: nan */
CASE_F_F(sqrtf, 0x01000000, 0x203504f3) /* 2.3509887e-38: 1.53329339e-19 */
CASE_F_F(sqrtf, 0x81000000, 0x7fc00000) /* -2.3509887e-38: nan */
CASE_F_F(sqrtf, 0x3e800000, 0x3f000000) /* 0.25: 0.5 */
CASE_F_F(sqrtf, 0xbe800000, 0x7fc00000) /* -0.25: nan */
CASE_F_F(sqrtf, 0x3effffff, 0x3f3504f3) /* 0.49999997: 0.707106769 */
CASE_F_F(sqrtf, 0xbeffffff, 0x7fc00000) /* -0.49999997: nan */
CASE_F_F(sqrtf, 0x3f000000, 0x3f3504f3) /* 0.5: 0.707106769 */
CASE_F_F(sqrtf, 0xbf000000, 0x7fc00000) /* -0.5: nan */
CASE_F_F(sqrtf, 0x3f000001, 0x3f3504f4) /* 0.50000006: 0.707106829 */
CASE_F_F(sqrtf, 0xbf000001, 0x7fc00000) /* -0.50000006: nan */
CASE_F_F(sqrtf, 0x3f3fffff, 0x3f5db3d7) /* 0.74999994: 0.866025388 */
CASE_F_F(sqrtf, 0xbf3fffff, 0x7fc00000) /* -0.74999994: nan */
CASE_F_F(sqrtf, 0x3f400000, 0x3f5db3d7) /* 0.75: 0.866025388 */
CASE_F_F(sqrtf, 0xbf400000, 0x7fc00000) /* -0.75: nan */
CASE_F_F(sqrtf, 0x3f7fffff, 0x3f7fffff) /* 0.99999994: 0.99999994 */
CASE_F_F(sqrtf, 0xbf7fffff, 0x7fc00000) /* -0.99999994: nan */
CASE_F_F(sqrtf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(sqrtf, 0xbf800000, 0x7fc00000) /* -1: nan */
CASE_F_F(sqrtf, 0x3f800001, 0x3f800000) /* 1.00000012: 1 */
CASE_F_F(sqrtf, 0xbf800001, 0x7fc00000) /* -1.00000012: nan */
CASE_F_F(sqrtf, 0x3fbfffff, 0x3f9cc470) /* 1.49999988: 1.2247448 */
CASE_F_F(sqrtf, 0xbfbfffff, 0x7fc00000) /* -1.49999988: nan */
CASE_F_F(sqrtf, 0x3fc00000, 0x3f9cc471) /* 1.5: 1.22474492 */
CASE_F_F(sqrtf, 0xbfc00000, 0x7fc00000) /* -1.5: nan */
CASE_F_F(sqrtf, 0x3fc00001, 0x3f9cc471) /* 1.50000012: 1.22474492 */
CASE_F_F(sqrtf, 0xbfc00001, 0x7fc00000) /* -1.50000012: nan */
CASE_F_F(sqrtf, 0x40000000, 0x3fb504f3) /* 2: 1.41421354 */
CASE_F_F(sqrtf, 0xc0000000, 0x7fc00000) /* -2: nan */
CASE_F_F(sqrtf, 0x401fffff, 0x3fca62c1) /* 2.49999976: 1.58113873 */
CASE_F_F(sqrtf, 0xc01fffff, 0x7fc00000) /* -2.49999976: nan */
CASE_F_F(sqrtf, 0x40200000, 0x3fca62c2) /* 2.5: 1.58113885 */
CASE_F_F(sqrtf, 0xc0200000, 0x7fc00000) /* -2.5: nan */
CASE_F_F(sqrtf, 0x40200001, 0x3fca62c2) /* 2.50000024: 1.58113885 */
CASE_F_F(sqrtf, 0xc0200001, 0x7fc00000) /* -2.50000024: nan */
CASE_F_F(sqrtf, 0x40600000, 0x3fef7751) /* 3.5: 1.87082875 */
CASE_F_F(sqrtf, 0xc0600000, 0x7fc00000) /* -3.5: nan */
CASE_F_F(sqrtf, 0x40900000, 0x4007c3b6) /* 4.5: 2.12132025 */
CASE_F_F(sqrtf, 0xc0900000, 0x7fc00000) /* -4.5: nan */
CASE_F_F(sqrtf, 0x41280000, 0x404f623a) /* 10.5: 3.24037027 */
CASE_F_F(sqrtf, 0xc1280000, 0x7fc00000) /* -10.5: nan */
CASE_F_F(sqrtf, 0x41380000, 0x405908d9) /* 11.5: 3.39116502 */
CASE_F_F(sqrtf, 0xc1380000, 0x7fc00000) /* -11.5: nan */
CASE_F_F(sqrtf, 0x4a7fffff, 0x44ffffff) /* 4194303.75: 2047.99988 */
CASE_F_F(sqrtf, 0xca7fffff, 0x7fc00000) /* -4194303.75: nan */
CASE_F_F(sqrtf, 0x4a800000, 0x45000000) /* 4194304: 2048 */
CASE_F_F(sqrtf, 0xca800000, 0x7fc00000) /* -4194304: nan */
CASE_F_F(sqrtf, 0x4a800001, 0x45000000) /* 4194304.5: 2048 */
CASE_F_F(sqrtf, 0xca800001, 0x7fc00000) /* -4194304.5: nan */
CASE_F_F(sqrtf, 0x4a800003, 0x45000001) /* 4194305.5: 2048.00024 */
CASE_F_F(sqrtf, 0xca800003, 0x7fc00000) /* -4194305.5: nan */
CASE_F_F(sqrtf, 0x4afffffd, 0x453504f2) /* 8388606.5: 2896.30908 */
CASE_F_F(sqrtf, 0xcafffffd, 0x7fc00000) /* -8388606.5: nan */
CASE_F_F(sqrtf, 0x4afffffe, 0x453504f2) /* 8388607: 2896.30908 */
CASE_F_F(sqrtf, 0xcafffffe, 0x7fc00000) /* -8388607: nan */
CASE_F_F(sqrtf, 0x4affffff, 0x453504f3) /* 8388607.5: 2896.30933 */
CASE_F_F(sqrtf, 0xcaffffff, 0x7fc00000) /* -8388607.5: nan */
CASE_F_F(sqrtf, 0x4b000000, 0x453504f3) /* 8388608: 2896.30933 */
CASE_F_F(sqrtf, 0xcb000000, 0x7fc00000) /* -8388608: nan */
CASE_F_F(sqrtf, 0x4b000001, 0x453504f4) /* 8388609: 2896.30957 */
CASE_F_F(sqrtf, 0xcb000001, 0x7fc00000) /* -8388609: nan */
CASE_F_F(sqrtf, 0x4b7fffff, 0x457fffff) /* 16777215: 4095.99976 */
CASE_F_F(sqrtf, 0xcb7fffff, 0x7fc00000) /* -16777215: nan */
CASE_F_F(sqrtf, 0x4b800000, 0x45800000) /* 16777216: 4096 */
CASE_F_F(sqrtf, 0xcb800000, 0x7fc00000) /* -16777216: nan */
CASE_F_F(sqrtf, 0x4b800001, 0x45800000) /* 16777218: 4096 */
CASE_F_F(sqrtf, 0xcb800001, 0x7fc00000) /* -16777218: nan */
CASE_F_F(sqrtf, 0x4effffff, 0x473504f3) /* 2.14748352e+09: 46340.9492 */
CASE_F_F(sqrtf, 0xceffffff, 0x7fc00000) /* -2.14748352e+09: nan */
CASE_F_F(sqrtf, 0x4f000000, 0x473504f3) /* 2.14748365e+09: 46340.9492 */
CASE_F_F(sqrtf, 0xcf000000, 0x7fc00000) /* -2.14748365e+09: nan */
CASE_F_F(sqrtf, 0x5effffff, 0x4f3504f3) /* 9.22337149e+18: 3.03700045e+09 */
CASE_F_F(sqrtf, 0xdeffffff, 0x7fc00000) /* -9.22337149e+18: nan */
CASE_F_F(sqrtf, 0x5f000000, 0x4f3504f3) /* 9.22337204e+18: 3.03700045e+09 */
CASE_F_F(sqrtf, 0xdf000000, 0x7fc00000) /* -9.22337204e+18: nan */
CASE_F_F(sqrtf, 0x5f000001, 0x4f3504f4) /* 9.22337314e+18: 3.0370007e+09 */
CASE_F_F(sqrtf, 0xdf000001, 0x7fc00000) /* -9.22337314e+18: nan */
CASE_F_F(sqrtf, 0x7f000000, 0x5f3504f3) /* 1.70141183e+38: 1.30438176e+19 */
CASE_F_F(sqrtf, 0xff000000, 0x7fc00000) /* -1.70141183e+38: nan */
CASE_F_F(sqrtf, 0x7f7fffff, 0x5f7fffff) /* 3.40282347e+38: 1.8446743e+19 */
CASE_F_F(sqrtf, 0xff7fffff, 0x7fc00000) /* -3.40282347e+38: nan */
CASE_F_F(sqrtf, 0x407fffff, 0x3fffffff) /* 3.99999976: 1.99999988 */
CASE_F_F(sqrtf, 0xc07fffff, 0x7fc00000) /* -3.99999976: nan */
CASE_F_F(sqrtf, 0x40800000, 0x40000000) /* 4: 2 */
CASE_F_F(sqrtf, 0xc0800000, 0x7fc00000) /* -4: nan */
CASE_F_F(sqrtf, 0x40800001, 0x40000000) /* 4.00000048: 2 */
CASE_F_F(sqrtf, 0xc0800001, 0x7fc00000) /* -4.00000048: nan */
CASE_F_F(sqrtf, 0x41100000, 0x40400000) /* 9: 3 */
CASE_F_F(sqrtf, 0xc1100000, 0x7fc00000) /* -9: nan */
CASE_F_F(sqrtf, 0x3e800001, 0x3f000000) /* 0.25000003: 0.5 */
CASE_F_F(sqrtf, 0xbe800001, 0x7fc00000) /* -0.25000003: nan */

/* floorf: float(float), NAN_QUIET */
CASE_F_F(floorf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(floorf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(floorf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(floorf, 0xff800000, 0xff800000) /* -inf: -inf */
CASE_F_F(floorf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(floorf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(floorf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(floorf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(floorf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(floorf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(floorf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(floorf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(floorf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(floorf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(floorf, 0x00000001, 0x00000000) /* 1.40129846e-45: 0 */
CASE_F_F(floorf, 0x80000001, 0xbf800000) /* -1.40129846e-45: -1 */
CASE_F_F(floorf, 0x00000002, 0x00000000) /* 2.80259693e-45: 0 */
CASE_F_F(floorf, 0x80000002, 0xbf800000) /* -2.80259693e-45: -1 */
CASE_F_F(floorf, 0x00000003, 0x00000000) /* 4.20389539e-45: 0 */
CASE_F_F(floorf, 0x80000003, 0xbf800000) /* -4.20389539e-45: -1 */
CASE_F_F(floorf, 0x003fffff, 0x00000000) /* 5.87747035e-39: 0 */
CASE_F_F(floorf, 0x803fffff, 0xbf800000) /* -5.87747035e-39: -1 */
CASE_F_F(floorf, 0x00400000, 0x00000000) /* 5.87747175e-39: 0 */
CASE_F_F(floorf, 0x80400000, 0xbf800000) /* -5.87747175e-39: -1 */
CASE_F_F(floorf, 0x007fffff, 0x00000000) /* 1.17549421e-38: 0 */
CASE_F_F(floorf, 0x807fffff, 0xbf800000) /* -1.17549421e-38: -1 */
CASE_F_F(floorf, 0x00800000, 0x00000000) /* 1.17549435e-38: 0 */
CASE_F_F(floorf, 0x80800000, 0xbf800000) /* -1.17549435e-38: -1 */
CASE_F_F(floorf, 0x00800001, 0x00000000) /* 1.17549449e-38: 0 */
CASE_F_F(floorf, 0x80800001, 0xbf800000) /* -1.17549449e-38: -1 */
CASE_F_F(floorf, 0x00ffffff, 0x00000000) /* 2.35098856e-38: 0 */
CASE_F_F(floorf, 0x80ffffff, 0xbf800000) /* -2.35098856e-38: -1 */
CASE_F_F(floorf, 0x01000000, 0x00000000) /* 2.3509887e-38: 0 */
CASE_F_F(floorf, 0x81000000, 0xbf800000) /* -2.3509887e-38: -1 */
CASE_F_F(floorf, 0x3e800000, 0x00000000) /* 0.25: 0 */
CASE_F_F(floorf, 0xbe800000, 0xbf800000) /* -0.25: -1 */
CASE_F_F(floorf, 0x3effffff, 0x00000000) /* 0.49999997: 0 */
CASE_F_F(floorf, 0xbeffffff, 0xbf800000) /* -0.49999997: -1 */
CASE_F_F(floorf, 0x3f000000, 0x00000000) /* 0.5: 0 */
CASE_F_F(floorf, 0xbf000000, 0xbf800000) /* -0.5: -1 */
CASE_F_F(floorf, 0x3f000001, 0x00000000) /* 0.50000006: 0 */
CASE_F_F(floorf, 0xbf000001, 0xbf800000) /* -0.50000006: -1 */
CASE_F_F(floorf, 0x3f3fffff, 0x00000000) /* 0.74999994: 0 */
CASE_F_F(floorf, 0xbf3fffff, 0xbf800000) /* -0.74999994: -1 */
CASE_F_F(floorf, 0x3f400000, 0x00000000) /* 0.75: 0 */
CASE_F_F(floorf, 0xbf400000, 0xbf800000) /* -0.75: -1 */
CASE_F_F(floorf, 0x3f7fffff, 0x00000000) /* 0.99999994: 0 */
CASE_F_F(floorf, 0xbf7fffff, 0xbf800000) /* -0.99999994: -1 */
CASE_F_F(floorf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(floorf, 0xbf800000, 0xbf800000) /* -1: -1 */
CASE_F_F(floorf, 0x3f800001, 0x3f800000) /* 1.00000012: 1 */
CASE_F_F(floorf, 0xbf800001, 0xc0000000) /* -1.00000012: -2 */
CASE_F_F(floorf, 0x3fbfffff, 0x3f800000) /* 1.49999988: 1 */
CASE_F_F(floorf, 0xbfbfffff, 0xc0000000) /* -1.49999988: -2 */
CASE_F_F(floorf, 0x3fc00000, 0x3f800000) /* 1.5: 1 */
CASE_F_F(floorf, 0xbfc00000, 0xc0000000) /* -1.5: -2 */
CASE_F_F(floorf, 0x3fc00001, 0x3f800000) /* 1.50000012: 1 */
CASE_F_F(floorf, 0xbfc00001, 0xc0000000) /* -1.50000012: -2 */
CASE_F_F(floorf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(floorf, 0xc0000000, 0xc0000000) /* -2: -2 */
CASE_F_F(floorf, 0x401fffff, 0x40000000) /* 2.49999976: 2 */
CASE_F_F(floorf, 0xc01fffff, 0xc0400000) /* -2.49999976: -3 */
CASE_F_F(floorf, 0x40200000, 0x40000000) /* 2.5: 2 */
CASE_F_F(floorf, 0xc0200000, 0xc0400000) /* -2.5: -3 */
CASE_F_F(floorf, 0x40200001, 0x40000000) /* 2.50000024: 2 */
CASE_F_F(floorf, 0xc0200001, 0xc0400000) /* -2.50000024: -3 */
CASE_F_F(floorf, 0x40600000, 0x40400000) /* 3.5: 3 */
CASE_F_F(floorf, 0xc0600000, 0xc0800000) /* -3.5: -4 */
CASE_F_F(floorf, 0x40900000, 0x40800000) /* 4.5: 4 */
CASE_F_F(floorf, 0xc0900000, 0xc0a00000) /* -4.5: -5 */
CASE_F_F(floorf, 0x41280000, 0x41200000) /* 10.5: 10 */
CASE_F_F(floorf, 0xc1280000, 0xc1300000) /* -10.5: -11 */
CASE_F_F(floorf, 0x41380000, 0x41300000) /* 11.5: 11 */
CASE_F_F(floorf, 0xc1380000, 0xc1400000) /* -11.5: -12 */
CASE_F_F(floorf, 0x4a7fffff, 0x4a7ffffc) /* 4194303.75: 4194303 */
CASE_F_F(floorf, 0xca7fffff, 0xca800000) /* -4194303.75: -4194304 */
CASE_F_F(floorf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(floorf, 0xca800000, 0xca800000) /* -4194304: -4194304 */
CASE_F_F(floorf, 0x4a800001, 0x4a800000) /* 4194304.5: 4194304 */
CASE_F_F(floorf, 0xca800001, 0xca800002) /* -4194304.5: -4194305 */
CASE_F_F(floorf, 0x4a800003, 0x4a800002) /* 4194305.5: 4194305 */
CASE_F_F(floorf, 0xca800003, 0xca800004) /* -4194305.5: -4194306 */
CASE_F_F(floorf, 0x4afffffd, 0x4afffffc) /* 8388606.5: 8388606 */
CASE_F_F(floorf, 0xcafffffd, 0xcafffffe) /* -8388606.5: -8388607 */
CASE_F_F(floorf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(floorf, 0xcafffffe, 0xcafffffe) /* -8388607: -8388607 */
CASE_F_F(floorf, 0x4affffff, 0x4afffffe) /* 8388607.5: 8388607 */
CASE_F_F(floorf, 0xcaffffff, 0xcb000000) /* -8388607.5: -8388608 */
CASE_F_F(floorf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(floorf, 0xcb000000, 0xcb000000) /* -8388608: -8388608 */
CASE_F_F(floorf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(floorf, 0xcb000001, 0xcb000001) /* -8388609: -8388609 */
CASE_F_F(floorf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(floorf, 0xcb7fffff, 0xcb7fffff) /* -16777215: -16777215 */
CASE_F_F(floorf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(floorf, 0xcb800000, 0xcb800000) /* -16777216: -16777216 */
CASE_F_F(floorf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(floorf, 0xcb800001, 0xcb800001) /* -16777218: -16777218 */
CASE_F_F(floorf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(floorf, 0xceffffff, 0xceffffff) /* -2.14748352e+09: -2.14748352e+09 */
CASE_F_F(floorf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(floorf, 0xcf000000, 0xcf000000) /* -2.14748365e+09: -2.14748365e+09 */
CASE_F_F(floorf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(floorf, 0xdeffffff, 0xdeffffff) /* -9.22337149e+18: -9.22337149e+18 */
CASE_F_F(floorf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(floorf, 0xdf000000, 0xdf000000) /* -9.22337204e+18: -9.22337204e+18 */
CASE_F_F(floorf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(floorf, 0xdf000001, 0xdf000001) /* -9.22337314e+18: -9.22337314e+18 */
CASE_F_F(floorf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(floorf, 0xff000000, 0xff000000) /* -1.70141183e+38: -1.70141183e+38 */
CASE_F_F(floorf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(floorf, 0xff7fffff, 0xff7fffff) /* -3.40282347e+38: -3.40282347e+38 */
CASE_F_F(floorf, 0x407fffff, 0x40400000) /* 3.99999976: 3 */
CASE_F_F(floorf, 0xc07fffff, 0xc0800000) /* -3.99999976: -4 */
CASE_F_F(floorf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(floorf, 0xc0800000, 0xc0800000) /* -4: -4 */
CASE_F_F(floorf, 0x40800001, 0x40800000) /* 4.00000048: 4 */
CASE_F_F(floorf, 0xc0800001, 0xc0a00000) /* -4.00000048: -5 */
CASE_F_F(floorf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(floorf, 0xc1100000, 0xc1100000) /* -9: -9 */
CASE_F_F(floorf, 0x3e800001, 0x00000000) /* 0.25000003: 0 */
CASE_F_F(floorf, 0xbe800001, 0xbf800000) /* -0.25000003: -1 */

/* ceilf: float(float), NAN_QUIET */
CASE_F_F(ceilf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(ceilf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(ceilf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(ceilf, 0xff800000, 0xff800000) /* -inf: -inf */
CASE_F_F(ceilf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(ceilf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(ceilf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(ceilf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(ceilf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(ceilf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(ceilf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(ceilf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(ceilf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(ceilf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(ceilf, 0x00000001, 0x3f800000) /* 1.40129846e-45: 1 */
CASE_F_F(ceilf, 0x80000001, 0x80000000) /* -1.40129846e-45: -0 */
CASE_F_F(ceilf, 0x00000002, 0x3f800000) /* 2.80259693e-45: 1 */
CASE_F_F(ceilf, 0x80000002, 0x80000000) /* -2.80259693e-45: -0 */
CASE_F_F(ceilf, 0x00000003, 0x3f800000) /* 4.20389539e-45: 1 */
CASE_F_F(ceilf, 0x80000003, 0x80000000) /* -4.20389539e-45: -0 */
CASE_F_F(ceilf, 0x003fffff, 0x3f800000) /* 5.87747035e-39: 1 */
CASE_F_F(ceilf, 0x803fffff, 0x80000000) /* -5.87747035e-39: -0 */
CASE_F_F(ceilf, 0x00400000, 0x3f800000) /* 5.87747175e-39: 1 */
CASE_F_F(ceilf, 0x80400000, 0x80000000) /* -5.87747175e-39: -0 */
CASE_F_F(ceilf, 0x007fffff, 0x3f800000) /* 1.17549421e-38: 1 */
CASE_F_F(ceilf, 0x807fffff, 0x80000000) /* -1.17549421e-38: -0 */
CASE_F_F(ceilf, 0x00800000, 0x3f800000) /* 1.17549435e-38: 1 */
CASE_F_F(ceilf, 0x80800000, 0x80000000) /* -1.17549435e-38: -0 */
CASE_F_F(ceilf, 0x00800001, 0x3f800000) /* 1.17549449e-38: 1 */
CASE_F_F(ceilf, 0x80800001, 0x80000000) /* -1.17549449e-38: -0 */
CASE_F_F(ceilf, 0x00ffffff, 0x3f800000) /* 2.35098856e-38: 1 */
CASE_F_F(ceilf, 0x80ffffff, 0x80000000) /* -2.35098856e-38: -0 */
CASE_F_F(ceilf, 0x01000000, 0x3f800000) /* 2.3509887e-38: 1 */
CASE_F_F(ceilf, 0x81000000, 0x80000000) /* -2.3509887e-38: -0 */
CASE_F_F(ceilf, 0x3e800000, 0x3f800000) /* 0.25: 1 */
CASE_F_F(ceilf, 0xbe800000, 0x80000000) /* -0.25: -0 */
CASE_F_F(ceilf, 0x3effffff, 0x3f800000) /* 0.49999997: 1 */
CASE_F_F(ceilf, 0xbeffffff, 0x80000000) /* -0.49999997: -0 */
CASE_F_F(ceilf, 0x3f000000, 0x3f800000) /* 0.5: 1 */
CASE_F_F(ceilf, 0xbf000000, 0x80000000) /* -0.5: -0 */
CASE_F_F(ceilf, 0x3f000001, 0x3f800000) /* 0.50000006: 1 */
CASE_F_F(ceilf, 0xbf000001, 0x80000000) /* -0.50000006: -0 */
CASE_F_F(ceilf, 0x3f3fffff, 0x3f800000) /* 0.74999994: 1 */
CASE_F_F(ceilf, 0xbf3fffff, 0x80000000) /* -0.74999994: -0 */
CASE_F_F(ceilf, 0x3f400000, 0x3f800000) /* 0.75: 1 */
CASE_F_F(ceilf, 0xbf400000, 0x80000000) /* -0.75: -0 */
CASE_F_F(ceilf, 0x3f7fffff, 0x3f800000) /* 0.99999994: 1 */
CASE_F_F(ceilf, 0xbf7fffff, 0x80000000) /* -0.99999994: -0 */
CASE_F_F(ceilf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(ceilf, 0xbf800000, 0xbf800000) /* -1: -1 */
CASE_F_F(ceilf, 0x3f800001, 0x40000000) /* 1.00000012: 2 */
CASE_F_F(ceilf, 0xbf800001, 0xbf800000) /* -1.00000012: -1 */
CASE_F_F(ceilf, 0x3fbfffff, 0x40000000) /* 1.49999988: 2 */
CASE_F_F(ceilf, 0xbfbfffff, 0xbf800000) /* -1.49999988: -1 */
CASE_F_F(ceilf, 0x3fc00000, 0x40000000) /* 1.5: 2 */
CASE_F_F(ceilf, 0xbfc00000, 0xbf800000) /* -1.5: -1 */
CASE_F_F(ceilf, 0x3fc00001, 0x40000000) /* 1.50000012: 2 */
CASE_F_F(ceilf, 0xbfc00001, 0xbf800000) /* -1.50000012: -1 */
CASE_F_F(ceilf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(ceilf, 0xc0000000, 0xc0000000) /* -2: -2 */
CASE_F_F(ceilf, 0x401fffff, 0x40400000) /* 2.49999976: 3 */
CASE_F_F(ceilf, 0xc01fffff, 0xc0000000) /* -2.49999976: -2 */
CASE_F_F(ceilf, 0x40200000, 0x40400000) /* 2.5: 3 */
CASE_F_F(ceilf, 0xc0200000, 0xc0000000) /* -2.5: -2 */
CASE_F_F(ceilf, 0x40200001, 0x40400000) /* 2.50000024: 3 */
CASE_F_F(ceilf, 0xc0200001, 0xc0000000) /* -2.50000024: -2 */
CASE_F_F(ceilf, 0x40600000, 0x40800000) /* 3.5: 4 */
CASE_F_F(ceilf, 0xc0600000, 0xc0400000) /* -3.5: -3 */
CASE_F_F(ceilf, 0x40900000, 0x40a00000) /* 4.5: 5 */
CASE_F_F(ceilf, 0xc0900000, 0xc0800000) /* -4.5: -4 */
CASE_F_F(ceilf, 0x41280000, 0x41300000) /* 10.5: 11 */
CASE_F_F(ceilf, 0xc1280000, 0xc1200000) /* -10.5: -10 */
CASE_F_F(ceilf, 0x41380000, 0x41400000) /* 11.5: 12 */
CASE_F_F(ceilf, 0xc1380000, 0xc1300000) /* -11.5: -11 */
CASE_F_F(ceilf, 0x4a7fffff, 0x4a800000) /* 4194303.75: 4194304 */
CASE_F_F(ceilf, 0xca7fffff, 0xca7ffffc) /* -4194303.75: -4194303 */
CASE_F_F(ceilf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(ceilf, 0xca800000, 0xca800000) /* -4194304: -4194304 */
CASE_F_F(ceilf, 0x4a800001, 0x4a800002) /* 4194304.5: 4194305 */
CASE_F_F(ceilf, 0xca800001, 0xca800000) /* -4194304.5: -4194304 */
CASE_F_F(ceilf, 0x4a800003, 0x4a800004) /* 4194305.5: 4194306 */
CASE_F_F(ceilf, 0xca800003, 0xca800002) /* -4194305.5: -4194305 */
CASE_F_F(ceilf, 0x4afffffd, 0x4afffffe) /* 8388606.5: 8388607 */
CASE_F_F(ceilf, 0xcafffffd, 0xcafffffc) /* -8388606.5: -8388606 */
CASE_F_F(ceilf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(ceilf, 0xcafffffe, 0xcafffffe) /* -8388607: -8388607 */
CASE_F_F(ceilf, 0x4affffff, 0x4b000000) /* 8388607.5: 8388608 */
CASE_F_F(ceilf, 0xcaffffff, 0xcafffffe) /* -8388607.5: -8388607 */
CASE_F_F(ceilf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(ceilf, 0xcb000000, 0xcb000000) /* -8388608: -8388608 */
CASE_F_F(ceilf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(ceilf, 0xcb000001, 0xcb000001) /* -8388609: -8388609 */
CASE_F_F(ceilf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(ceilf, 0xcb7fffff, 0xcb7fffff) /* -16777215: -16777215 */
CASE_F_F(ceilf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(ceilf, 0xcb800000, 0xcb800000) /* -16777216: -16777216 */
CASE_F_F(ceilf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(ceilf, 0xcb800001, 0xcb800001) /* -16777218: -16777218 */
CASE_F_F(ceilf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(ceilf, 0xceffffff, 0xceffffff) /* -2.14748352e+09: -2.14748352e+09 */
CASE_F_F(ceilf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(ceilf, 0xcf000000, 0xcf000000) /* -2.14748365e+09: -2.14748365e+09 */
CASE_F_F(ceilf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(ceilf, 0xdeffffff, 0xdeffffff) /* -9.22337149e+18: -9.22337149e+18 */
CASE_F_F(ceilf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(ceilf, 0xdf000000, 0xdf000000) /* -9.22337204e+18: -9.22337204e+18 */
CASE_F_F(ceilf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(ceilf, 0xdf000001, 0xdf000001) /* -9.22337314e+18: -9.22337314e+18 */
CASE_F_F(ceilf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(ceilf, 0xff000000, 0xff000000) /* -1.70141183e+38: -1.70141183e+38 */
CASE_F_F(ceilf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(ceilf, 0xff7fffff, 0xff7fffff) /* -3.40282347e+38: -3.40282347e+38 */
CASE_F_F(ceilf, 0x407fffff, 0x40800000) /* 3.99999976: 4 */
CASE_F_F(ceilf, 0xc07fffff, 0xc0400000) /* -3.99999976: -3 */
CASE_F_F(ceilf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(ceilf, 0xc0800000, 0xc0800000) /* -4: -4 */
CASE_F_F(ceilf, 0x40800001, 0x40a00000) /* 4.00000048: 5 */
CASE_F_F(ceilf, 0xc0800001, 0xc0800000) /* -4.00000048: -4 */
CASE_F_F(ceilf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(ceilf, 0xc1100000, 0xc1100000) /* -9: -9 */
CASE_F_F(ceilf, 0x3e800001, 0x3f800000) /* 0.25000003: 1 */
CASE_F_F(ceilf, 0xbe800001, 0x80000000) /* -0.25000003: -0 */

/* truncf: float(float), NAN_QUIET */
CASE_F_F(truncf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(truncf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(truncf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(truncf, 0xff800000, 0xff800000) /* -inf: -inf */
CASE_F_F(truncf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(truncf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(truncf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(truncf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(truncf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(truncf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(truncf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(truncf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(truncf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(truncf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(truncf, 0x00000001, 0x00000000) /* 1.40129846e-45: 0 */
CASE_F_F(truncf, 0x80000001, 0x80000000) /* -1.40129846e-45: -0 */
CASE_F_F(truncf, 0x00000002, 0x00000000) /* 2.80259693e-45: 0 */
CASE_F_F(truncf, 0x80000002, 0x80000000) /* -2.80259693e-45: -0 */
CASE_F_F(truncf, 0x00000003, 0x00000000) /* 4.20389539e-45: 0 */
CASE_F_F(truncf, 0x80000003, 0x80000000) /* -4.20389539e-45: -0 */
CASE_F_F(truncf, 0x003fffff, 0x00000000) /* 5.87747035e-39: 0 */
CASE_F_F(truncf, 0x803fffff, 0x80000000) /* -5.87747035e-39: -0 */
CASE_F_F(truncf, 0x00400000, 0x00000000) /* 5.87747175e-39: 0 */
CASE_F_F(truncf, 0x80400000, 0x80000000) /* -5.87747175e-39: -0 */
CASE_F_F(truncf, 0x007fffff, 0x00000000) /* 1.17549421e-38: 0 */
CASE_F_F(truncf, 0x807fffff, 0x80000000) /* -1.17549421e-38: -0 */
CASE_F_F(truncf, 0x00800000, 0x00000000) /* 1.17549435e-38: 0 */
CASE_F_F(truncf, 0x80800000, 0x80000000) /* -1.17549435e-38: -0 */
CASE_F_F(truncf, 0x00800001, 0x00000000) /* 1.17549449e-38: 0 */
CASE_F_F(truncf, 0x80800001, 0x80000000) /* -1.17549449e-38: -0 */
CASE_F_F(truncf, 0x00ffffff, 0x00000000) /* 2.35098856e-38: 0 */
CASE_F_F(truncf, 0x80ffffff, 0x80000000) /* -2.35098856e-38: -0 */
CASE_F_F(truncf, 0x01000000, 0x00000000) /* 2.3509887e-38: 0 */
CASE_F_F(truncf, 0x81000000, 0x80000000) /* -2.3509887e-38: -0 */
CASE_F_F(truncf, 0x3e800000, 0x00000000) /* 0.25: 0 */
CASE_F_F(truncf, 0xbe800000, 0x80000000) /* -0.25: -0 */
CASE_F_F(truncf, 0x3effffff, 0x00000000) /* 0.49999997: 0 */
CASE_F_F(truncf, 0xbeffffff, 0x80000000) /* -0.49999997: -0 */
CASE_F_F(truncf, 0x3f000000, 0x00000000) /* 0.5: 0 */
CASE_F_F(truncf, 0xbf000000, 0x80000000) /* -0.5: -0 */
CASE_F_F(truncf, 0x3f000001, 0x00000000) /* 0.50000006: 0 */
CASE_F_F(truncf, 0xbf000001, 0x80000000) /* -0.50000006: -0 */
CASE_F_F(truncf, 0x3f3fffff, 0x00000000) /* 0.74999994: 0 */
CASE_F_F(truncf, 0xbf3fffff, 0x80000000) /* -0.74999994: -0 */
CASE_F_F(truncf, 0x3f400000, 0x00000000) /* 0.75: 0 */
CASE_F_F(truncf, 0xbf400000, 0x80000000) /* -0.75: -0 */
CASE_F_F(truncf, 0x3f7fffff, 0x00000000) /* 0.99999994: 0 */
CASE_F_F(truncf, 0xbf7fffff, 0x80000000) /* -0.99999994: -0 */
CASE_F_F(truncf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(truncf, 0xbf800000, 0xbf800000) /* -1: -1 */
CASE_F_F(truncf, 0x3f800001, 0x3f800000) /* 1.00000012: 1 */
CASE_F_F(truncf, 0xbf800001, 0xbf800000) /* -1.00000012: -1 */
CASE_F_F(truncf, 0x3fbfffff, 0x3f800000) /* 1.49999988: 1 */
CASE_F_F(truncf, 0xbfbfffff, 0xbf800000) /* -1.49999988: -1 */
CASE_F_F(truncf, 0x3fc00000, 0x3f800000) /* 1.5: 1 */
CASE_F_F(truncf, 0xbfc00000, 0xbf800000) /* -1.5: -1 */
CASE_F_F(truncf, 0x3fc00001, 0x3f800000) /* 1.50000012: 1 */
CASE_F_F(truncf, 0xbfc00001, 0xbf800000) /* -1.50000012: -1 */
CASE_F_F(truncf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(truncf, 0xc0000000, 0xc0000000) /* -2: -2 */
CASE_F_F(truncf, 0x401fffff, 0x40000000) /* 2.49999976: 2 */
CASE_F_F(truncf, 0xc01fffff, 0xc0000000) /* -2.49999976: -2 */
CASE_F_F(truncf, 0x40200000, 0x40000000) /* 2.5: 2 */
CASE_F_F(truncf, 0xc0200000, 0xc0000000) /* -2.5: -2 */
CASE_F_F(truncf, 0x40200001, 0x40000000) /* 2.50000024: 2 */
CASE_F_F(truncf, 0xc0200001, 0xc0000000) /* -2.50000024: -2 */
CASE_F_F(truncf, 0x40600000, 0x40400000) /* 3.5: 3 */
CASE_F_F(truncf, 0xc0600000, 0xc0400000) /* -3.5: -3 */
CASE_F_F(truncf, 0x40900000, 0x40800000) /* 4.5: 4 */
CASE_F_F(truncf, 0xc0900000, 0xc0800000) /* -4.5: -4 */
CASE_F_F(truncf, 0x41280000, 0x41200000) /* 10.5: 10 */
CASE_F_F(truncf, 0xc1280000, 0xc1200000) /* -10.5: -10 */
CASE_F_F(truncf, 0x41380000, 0x41300000) /* 11.5: 11 */
CASE_F_F(truncf, 0xc1380000, 0xc1300000) /* -11.5: -11 */
CASE_F_F(truncf, 0x4a7fffff, 0x4a7ffffc) /* 4194303.75: 4194303 */
CASE_F_F(truncf, 0xca7fffff, 0xca7ffffc) /* -4194303.75: -4194303 */
CASE_F_F(truncf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(truncf, 0xca800000, 0xca800000) /* -4194304: -4194304 */
CASE_F_F(truncf, 0x4a800001, 0x4a800000) /* 4194304.5: 4194304 */
CASE_F_F(truncf, 0xca800001, 0xca800000) /* -4194304.5: -4194304 */
CASE_F_F(truncf, 0x4a800003, 0x4a800002) /* 4194305.5: 4194305 */
CASE_F_F(truncf, 0xca800003, 0xca800002) /* -4194305.5: -4194305 */
CASE_F_F(truncf, 0x4afffffd, 0x4afffffc) /* 8388606.5: 8388606 */
CASE_F_F(truncf, 0xcafffffd, 0xcafffffc) /* -8388606.5: -8388606 */
CASE_F_F(truncf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(truncf, 0xcafffffe, 0xcafffffe) /* -8388607: -8388607 */
CASE_F_F(truncf, 0x4affffff, 0x4afffffe) /* 8388607.5: 8388607 */
CASE_F_F(truncf, 0xcaffffff, 0xcafffffe) /* -8388607.5: -8388607 */
CASE_F_F(truncf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(truncf, 0xcb000000, 0xcb000000) /* -8388608: -8388608 */
CASE_F_F(truncf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(truncf, 0xcb000001, 0xcb000001) /* -8388609: -8388609 */
CASE_F_F(truncf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(truncf, 0xcb7fffff, 0xcb7fffff) /* -16777215: -16777215 */
CASE_F_F(truncf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(truncf, 0xcb800000, 0xcb800000) /* -16777216: -16777216 */
CASE_F_F(truncf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(truncf, 0xcb800001, 0xcb800001) /* -16777218: -16777218 */
CASE_F_F(truncf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(truncf, 0xceffffff, 0xceffffff) /* -2.14748352e+09: -2.14748352e+09 */
CASE_F_F(truncf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(truncf, 0xcf000000, 0xcf000000) /* -2.14748365e+09: -2.14748365e+09 */
CASE_F_F(truncf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(truncf, 0xdeffffff, 0xdeffffff) /* -9.22337149e+18: -9.22337149e+18 */
CASE_F_F(truncf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(truncf, 0xdf000000, 0xdf000000) /* -9.22337204e+18: -9.22337204e+18 */
CASE_F_F(truncf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(truncf, 0xdf000001, 0xdf000001) /* -9.22337314e+18: -9.22337314e+18 */
CASE_F_F(truncf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(truncf, 0xff000000, 0xff000000) /* -1.70141183e+38: -1.70141183e+38 */
CASE_F_F(truncf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(truncf, 0xff7fffff, 0xff7fffff) /* -3.40282347e+38: -3.40282347e+38 */
CASE_F_F(truncf, 0x407fffff, 0x40400000) /* 3.99999976: 3 */
CASE_F_F(truncf, 0xc07fffff, 0xc0400000) /* -3.99999976: -3 */
CASE_F_F(truncf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(truncf, 0xc0800000, 0xc0800000) /* -4: -4 */
CASE_F_F(truncf, 0x40800001, 0x40800000) /* 4.00000048: 4 */
CASE_F_F(truncf, 0xc0800001, 0xc0800000) /* -4.00000048: -4 */
CASE_F_F(truncf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(truncf, 0xc1100000, 0xc1100000) /* -9: -9 */
CASE_F_F(truncf, 0x3e800001, 0x00000000) /* 0.25000003: 0 */
CASE_F_F(truncf, 0xbe800001, 0x80000000) /* -0.25000003: -0 */

/* roundf: float(float), NAN_QUIET */
CASE_F_F(roundf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(roundf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(roundf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(roundf, 0xff800000, 0xff800000) /* -inf: -inf */
CASE_F_F(roundf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(roundf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(roundf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(roundf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(roundf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(roundf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(roundf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(roundf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(roundf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(roundf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(roundf, 0x00000001, 0x00000000) /* 1.40129846e-45: 0 */
CASE_F_F(roundf, 0x80000001, 0x80000000) /* -1.40129846e-45: -0 */
CASE_F_F(roundf, 0x00000002, 0x00000000) /* 2.80259693e-45: 0 */
CASE_F_F(roundf, 0x80000002, 0x80000000) /* -2.80259693e-45: -0 */
CASE_F_F(roundf, 0x00000003, 0x00000000) /* 4.20389539e-45: 0 */
CASE_F_F(roundf, 0x80000003, 0x80000000) /* -4.20389539e-45: -0 */
CASE_F_F(roundf, 0x003fffff, 0x00000000) /* 5.87747035e-39: 0 */
CASE_F_F(roundf, 0x803fffff, 0x80000000) /* -5.87747035e-39: -0 */
CASE_F_F(roundf, 0x00400000, 0x00000000) /* 5.87747175e-39: 0 */
CASE_F_F(roundf, 0x80400000, 0x80000000) /* -5.87747175e-39: -0 */
CASE_F_F(roundf, 0x007fffff, 0x00000000) /* 1.17549421e-38: 0 */
CASE_F_F(roundf, 0x807fffff, 0x80000000) /* -1.17549421e-38: -0 */
CASE_F_F(roundf, 0x00800000, 0x00000000) /* 1.17549435e-38: 0 */
CASE_F_F(roundf, 0x80800000, 0x80000000) /* -1.17549435e-38: -0 */
CASE_F_F(roundf, 0x00800001, 0x00000000) /* 1.17549449e-38: 0 */
CASE_F_F(roundf, 0x80800001, 0x80000000) /* -1.17549449e-38: -0 */
CASE_F_F(roundf, 0x00ffffff, 0x00000000) /* 2.35098856e-38: 0 */
CASE_F_F(roundf, 0x80ffffff, 0x80000000) /* -2.35098856e-38: -0 */
CASE_F_F(roundf, 0x01000000, 0x00000000) /* 2.3509887e-38: 0 */
CASE_F_F(roundf, 0x81000000, 0x80000000) /* -2.3509887e-38: -0 */
CASE_F_F(roundf, 0x3e800000, 0x00000000) /* 0.25: 0 */
CASE_F_F(roundf, 0xbe800000, 0x80000000) /* -0.25: -0 */
CASE_F_F(roundf, 0x3effffff, 0x00000000) /* 0.49999997: 0 */
CASE_F_F(roundf, 0xbeffffff, 0x80000000) /* -0.49999997: -0 */
CASE_F_F(roundf, 0x3f000000, 0x3f800000) /* 0.5: 1 */
CASE_F_F(roundf, 0xbf000000, 0xbf800000) /* -0.5: -1 */
CASE_F_F(roundf, 0x3f000001, 0x3f800000) /* 0.50000006: 1 */
CASE_F_F(roundf, 0xbf000001, 0xbf800000) /* -0.50000006: -1 */
CASE_F_F(roundf, 0x3f3fffff, 0x3f800000) /* 0.74999994: 1 */
CASE_F_F(roundf, 0xbf3fffff, 0xbf800000) /* -0.74999994: -1 */
CASE_F_F(roundf, 0x3f400000, 0x3f800000) /* 0.75: 1 */
CASE_F_F(roundf, 0xbf400000, 0xbf800000) /* -0.75: -1 */
CASE_F_F(roundf, 0x3f7fffff, 0x3f800000) /* 0.99999994: 1 */
CASE_F_F(roundf, 0xbf7fffff, 0xbf800000) /* -0.99999994: -1 */
CASE_F_F(roundf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(roundf, 0xbf800000, 0xbf800000) /* -1: -1 */
CASE_F_F(roundf, 0x3f800001, 0x3f800000) /* 1.00000012: 1 */
CASE_F_F(roundf, 0xbf800001, 0xbf800000) /* -1.00000012: -1 */
CASE_F_F(roundf, 0x3fbfffff, 0x3f800000) /* 1.49999988: 1 */
CASE_F_F(roundf, 0xbfbfffff, 0xbf800000) /* -1.49999988: -1 */
CASE_F_F(roundf, 0x3fc00000, 0x40000000) /* 1.5: 2 */
CASE_F_F(roundf, 0xbfc00000, 0xc0000000) /* -1.5: -2 */
CASE_F_F(roundf, 0x3fc00001, 0x40000000) /* 1.50000012: 2 */
CASE_F_F(roundf, 0xbfc00001, 0xc0000000) /* -1.50000012: -2 */
CASE_F_F(roundf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(roundf, 0xc0000000, 0xc0000000) /* -2: -2 */
CASE_F_F(roundf, 0x401fffff, 0x40000000) /* 2.49999976: 2 */
CASE_F_F(roundf, 0xc01fffff, 0xc0000000) /* -2.49999976: -2 */
CASE_F_F(roundf, 0x40200000, 0x40400000) /* 2.5: 3 */
CASE_F_F(roundf, 0xc0200000, 0xc0400000) /* -2.5: -3 */
CASE_F_F(roundf, 0x40200001, 0x40400000) /* 2.50000024: 3 */
CASE_F_F(roundf, 0xc0200001, 0xc0400000) /* -2.50000024: -3 */
CASE_F_F(roundf, 0x40600000, 0x40800000) /* 3.5: 4 */
CASE_F_F(roundf, 0xc0600000, 0xc0800000) /* -3.5: -4 */
CASE_F_F(roundf, 0x40900000, 0x40a00000) /* 4.5: 5 */
CASE_F_F(roundf, 0xc0900000, 0xc0a00000) /* -4.5: -5 */
CASE_F_F(roundf, 0x41280000, 0x41300000) /* 10.5: 11 */
CASE_F_F(roundf, 0xc1280000, 0xc1300000) /* -10.5: -11 */
CASE_F_F(roundf, 0x41380000, 0x41400000) /* 11.5: 12 */
CASE_F_F(roundf, 0xc1380000, 0xc1400000) /* -11.5: -12 */
CASE_F_F(roundf, 0x4a7fffff, 0x4a800000) /* 4194303.75: 4194304 */
CASE_F_F(roundf, 0xca7fffff, 0xca800000) /* -4194303.75: -4194304 */
CASE_F_F(roundf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(roundf, 0xca800000, 0xca800000) /* -4194304: -4194304 */
CASE_F_F(roundf, 0x4a800001, 0x4a800002) /* 4194304.5: 4194305 */
CASE_F_F(roundf, 0xca800001, 0xca800002) /* -4194304.5: -4194305 */
CASE_F_F(roundf, 0x4a800003, 0x4a800004) /* 4194305.5: 4194306 */
CASE_F_F(roundf, 0xca800003, 0xca800004) /* -4194305.5: -4194306 */
CASE_F_F(roundf, 0x4afffffd, 0x4afffffe) /* 8388606.5: 8388607 */
CASE_F_F(roundf, 0xcafffffd, 0xcafffffe) /* -8388606.5: -8388607 */
CASE_F_F(roundf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(roundf, 0xcafffffe, 0xcafffffe) /* -8388607: -8388607 */
CASE_F_F(roundf, 0x4affffff, 0x4b000000) /* 8388607.5: 8388608 */
CASE_F_F(roundf, 0xcaffffff, 0xcb000000) /* -8388607.5: -8388608 */
CASE_F_F(roundf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(roundf, 0xcb000000, 0xcb000000) /* -8388608: -8388608 */
CASE_F_F(roundf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(roundf, 0xcb000001, 0xcb000001) /* -8388609: -8388609 */
CASE_F_F(roundf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(roundf, 0xcb7fffff, 0xcb7fffff) /* -16777215: -16777215 */
CASE_F_F(roundf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(roundf, 0xcb800000, 0xcb800000) /* -16777216: -16777216 */
CASE_F_F(roundf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(roundf, 0xcb800001, 0xcb800001) /* -16777218: -16777218 */
CASE_F_F(roundf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(roundf, 0xceffffff, 0xceffffff) /* -2.14748352e+09: -2.14748352e+09 */
CASE_F_F(roundf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(roundf, 0xcf000000, 0xcf000000) /* -2.14748365e+09: -2.14748365e+09 */
CASE_F_F(roundf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(roundf, 0xdeffffff, 0xdeffffff) /* -9.22337149e+18: -9.22337149e+18 */
CASE_F_F(roundf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(roundf, 0xdf000000, 0xdf000000) /* -9.22337204e+18: -9.22337204e+18 */
CASE_F_F(roundf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(roundf, 0xdf000001, 0xdf000001) /* -9.22337314e+18: -9.22337314e+18 */
CASE_F_F(roundf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(roundf, 0xff000000, 0xff000000) /* -1.70141183e+38: -1.70141183e+38 */
CASE_F_F(roundf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(roundf, 0xff7fffff, 0xff7fffff) /* -3.40282347e+38: -3.40282347e+38 */
CASE_F_F(roundf, 0x407fffff, 0x40800000) /* 3.99999976: 4 */
CASE_F_F(roundf, 0xc07fffff, 0xc0800000) /* -3.99999976: -4 */
CASE_F_F(roundf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(roundf, 0xc0800000, 0xc0800000) /* -4: -4 */
CASE_F_F(roundf, 0x40800001, 0x40800000) /* 4.00000048: 4 */
CASE_F_F(roundf, 0xc0800001, 0xc0800000) /* -4.00000048: -4 */
CASE_F_F(roundf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(roundf, 0xc1100000, 0xc1100000) /* -9: -9 */
CASE_F_F(roundf, 0x3e800001, 0x00000000) /* 0.25000003: 0 */
CASE_F_F(roundf, 0xbe800001, 0x80000000) /* -0.25000003: -0 */

/* rintf: float(float), NAN_QUIET */
CASE_F_F(rintf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(rintf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(rintf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(rintf, 0xff800000, 0xff800000) /* -inf: -inf */
CASE_F_F(rintf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(rintf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(rintf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(rintf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(rintf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(rintf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(rintf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(rintf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(rintf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(rintf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(rintf, 0x00000001, 0x00000000) /* 1.40129846e-45: 0 */
CASE_F_F(rintf, 0x80000001, 0x80000000) /* -1.40129846e-45: -0 */
CASE_F_F(rintf, 0x00000002, 0x00000000) /* 2.80259693e-45: 0 */
CASE_F_F(rintf, 0x80000002, 0x80000000) /* -2.80259693e-45: -0 */
CASE_F_F(rintf, 0x00000003, 0x00000000) /* 4.20389539e-45: 0 */
CASE_F_F(rintf, 0x80000003, 0x80000000) /* -4.20389539e-45: -0 */
CASE_F_F(rintf, 0x003fffff, 0x00000000) /* 5.87747035e-39: 0 */
CASE_F_F(rintf, 0x803fffff, 0x80000000) /* -5.87747035e-39: -0 */
CASE_F_F(rintf, 0x00400000, 0x00000000) /* 5.87747175e-39: 0 */
CASE_F_F(rintf, 0x80400000, 0x80000000) /* -5.87747175e-39: -0 */
CASE_F_F(rintf, 0x007fffff, 0x00000000) /* 1.17549421e-38: 0 */
CASE_F_F(rintf, 0x807fffff, 0x80000000) /* -1.17549421e-38: -0 */
CASE_F_F(rintf, 0x00800000, 0x00000000) /* 1.17549435e-38: 0 */
CASE_F_F(rintf, 0x80800000, 0x80000000) /* -1.17549435e-38: -0 */
CASE_F_F(rintf, 0x00800001, 0x00000000) /* 1.17549449e-38: 0 */
CASE_F_F(rintf, 0x80800001, 0x80000000) /* -1.17549449e-38: -0 */
CASE_F_F(rintf, 0x00ffffff, 0x00000000) /* 2.35098856e-38: 0 */
CASE_F_F(rintf, 0x80ffffff, 0x80000000) /* -2.35098856e-38: -0 */
CASE_F_F(rintf, 0x01000000, 0x00000000) /* 2.3509887e-38: 0 */
CASE_F_F(rintf, 0x81000000, 0x80000000) /* -2.3509887e-38: -0 */
CASE_F_F(rintf, 0x3e800000, 0x00000000) /* 0.25: 0 */
CASE_F_F(rintf, 0xbe800000, 0x80000000) /* -0.25: -0 */
CASE_F_F(rintf, 0x3effffff, 0x00000000) /* 0.49999997: 0 */
CASE_F_F(rintf, 0xbeffffff, 0x80000000) /* -0.49999997: -0 */
CASE_F_F(rintf, 0x3f000000, 0x00000000) /* 0.5: 0 */
CASE_F_F(rintf, 0xbf000000, 0x80000000) /* -0.5: -0 */
CASE_F_F(rintf, 0x3f000001, 0x3f800000) /* 0.50000006: 1 */
CASE_F_F(rintf, 0xbf000001, 0xbf800000) /* -0.50000006: -1 */
CASE_F_F(rintf, 0x3f3fffff, 0x3f800000) /* 0.74999994: 1 */
CASE_F_F(rintf, 0xbf3fffff, 0xbf800000) /* -0.74999994: -1 */
CASE_F_F(rintf, 0x3f400000, 0x3f800000) /* 0.75: 1 */
CASE_F_F(rintf, 0xbf400000, 0xbf800000) /* -0.75: -1 */
CASE_F_F(rintf, 0x3f7fffff, 0x3f800000) /* 0.99999994: 1 */
CASE_F_F(rintf, 0xbf7fffff, 0xbf800000) /* -0.99999994: -1 */
CASE_F_F(rintf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(rintf, 0xbf800000, 0xbf800000) /* -1: -1 */
CASE_F_F(rintf, 0x3f800001, 0x3f800000) /* 1.00000012: 1 */
CASE_F_F(rintf, 0xbf800001, 0xbf800000) /* -1.00000012: -1 */
CASE_F_F(rintf, 0x3fbfffff, 0x3f800000) /* 1.49999988: 1 */
CASE_F_F(rintf, 0xbfbfffff, 0xbf800000) /* -1.49999988: -1 */
CASE_F_F(rintf, 0x3fc00000, 0x40000000) /* 1.5: 2 */
CASE_F_F(rintf, 0xbfc00000, 0xc0000000) /* -1.5: -2 */
CASE_F_F(rintf, 0x3fc00001, 0x40000000) /* 1.50000012: 2 */
CASE_F_F(rintf, 0xbfc00001, 0xc0000000) /* -1.50000012: -2 */
CASE_F_F(rintf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(rintf, 0xc0000000, 0xc0000000) /* -2: -2 */
CASE_F_F(rintf, 0x401fffff, 0x40000000) /* 2.49999976: 2 */
CASE_F_F(rintf, 0xc01fffff, 0xc0000000) /* -2.49999976: -2 */
CASE_F_F(rintf, 0x40200000, 0x40000000) /* 2.5: 2 */
CASE_F_F(rintf, 0xc0200000, 0xc0000000) /* -2.5: -2 */
CASE_F_F(rintf, 0x40200001, 0x40400000) /* 2.50000024: 3 */
CASE_F_F(rintf, 0xc0200001, 0xc0400000) /* -2.50000024: -3 */
CASE_F_F(rintf, 0x40600000, 0x40800000) /* 3.5: 4 */
CASE_F_F(rintf, 0xc0600000, 0xc0800000) /* -3.5: -4 */
CASE_F_F(rintf, 0x40900000, 0x40800000) /* 4.5: 4 */
CASE_F_F(rintf, 0xc0900000, 0xc0800000) /* -4.5: -4 */
CASE_F_F(rintf, 0x41280000, 0x41200000) /* 10.5: 10 */
CASE_F_F(rintf, 0xc1280000, 0xc1200000) /* -10.5: -10 */
CASE_F_F(rintf, 0x41380000, 0x41400000) /* 11.5: 12 */
CASE_F_F(rintf, 0xc1380000, 0xc1400000) /* -11.5: -12 */
CASE_F_F(rintf, 0x4a7fffff, 0x4a800000) /* 4194303.75: 4194304 */
CASE_F_F(rintf, 0xca7fffff, 0xca800000) /* -4194303.75: -4194304 */
CASE_F_F(rintf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(rintf, 0xca800000, 0xca800000) /* -4194304: -4194304 */
CASE_F_F(rintf, 0x4a800001, 0x4a800000) /* 4194304.5: 4194304 */
CASE_F_F(rintf, 0xca800001, 0xca800000) /* -4194304.5: -4194304 */
CASE_F_F(rintf, 0x4a800003, 0x4a800004) /* 4194305.5: 4194306 */
CASE_F_F(rintf, 0xca800003, 0xca800004) /* -4194305.5: -4194306 */
CASE_F_F(rintf, 0x4afffffd, 0x4afffffc) /* 8388606.5: 8388606 */
CASE_F_F(rintf, 0xcafffffd, 0xcafffffc) /* -8388606.5: -8388606 */
CASE_F_F(rintf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(rintf, 0xcafffffe, 0xcafffffe) /* -8388607: -8388607 */
CASE_F_F(rintf, 0x4affffff, 0x4b000000) /* 8388607.5: 8388608 */
CASE_F_F(rintf, 0xcaffffff, 0xcb000000) /* -8388607.5: -8388608 */
CASE_F_F(rintf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(rintf, 0xcb000000, 0xcb000000) /* -8388608: -8388608 */
CASE_F_F(rintf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(rintf, 0xcb000001, 0xcb000001) /* -8388609: -8388609 */
CASE_F_F(rintf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(rintf, 0xcb7fffff, 0xcb7fffff) /* -16777215: -16777215 */
CASE_F_F(rintf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(rintf, 0xcb800000, 0xcb800000) /* -16777216: -16777216 */
CASE_F_F(rintf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(rintf, 0xcb800001, 0xcb800001) /* -16777218: -16777218 */
CASE_F_F(rintf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(rintf, 0xceffffff, 0xceffffff) /* -2.14748352e+09: -2.14748352e+09 */
CASE_F_F(rintf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(rintf, 0xcf000000, 0xcf000000) /* -2.14748365e+09: -2.14748365e+09 */
CASE_F_F(rintf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(rintf, 0xdeffffff, 0xdeffffff) /* -9.22337149e+18: -9.22337149e+18 */
CASE_F_F(rintf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(rintf, 0xdf000000, 0xdf000000) /* -9.22337204e+18: -9.22337204e+18 */
CASE_F_F(rintf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(rintf, 0xdf000001, 0xdf000001) /* -9.22337314e+18: -9.22337314e+18 */
CASE_F_F(rintf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(rintf, 0xff000000, 0xff000000) /* -1.70141183e+38: -1.70141183e+38 */
CASE_F_F(rintf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(rintf, 0xff7fffff, 0xff7fffff) /* -3.40282347e+38: -3.40282347e+38 */
CASE_F_F(rintf, 0x407fffff, 0x40800000) /* 3.99999976: 4 */
CASE_F_F(rintf, 0xc07fffff, 0xc0800000) /* -3.99999976: -4 */
CASE_F_F(rintf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(rintf, 0xc0800000, 0xc0800000) /* -4: -4 */
CASE_F_F(rintf, 0x40800001, 0x40800000) /* 4.00000048: 4 */
CASE_F_F(rintf, 0xc0800001, 0xc0800000) /* -4.00000048: -4 */
CASE_F_F(rintf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(rintf, 0xc1100000, 0xc1100000) /* -9: -9 */
CASE_F_F(rintf, 0x3e800001, 0x00000000) /* 0.25000003: 0 */
CASE_F_F(rintf, 0xbe800001, 0x80000000) /* -0.25000003: -0 */

/* nearbyintf: float(float), NAN_QUIET */
CASE_F_F(nearbyintf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(nearbyintf, 0x80000000, 0x80000000) /* -0: -0 */
CASE_F_F(nearbyintf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(nearbyintf, 0xff800000, 0xff800000) /* -inf: -inf */
CASE_F_F(nearbyintf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(nearbyintf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(nearbyintf, 0x7fc00001, 0x7fc00000) /* nan: nan */
CASE_F_F(nearbyintf, 0xffc00001, 0x7fc00000) /* -nan: nan */
CASE_F_F(nearbyintf, 0x7fffffff, 0x7fc00000) /* nan: nan */
CASE_F_F(nearbyintf, 0xffffffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(nearbyintf, 0x7f800001, 0x7fc00000) /* nan: nan */
CASE_F_F(nearbyintf, 0xff800001, 0x7fc00000) /* -nan: nan */
CASE_F_F(nearbyintf, 0x7fbfffff, 0x7fc00000) /* nan: nan */
CASE_F_F(nearbyintf, 0xffbfffff, 0x7fc00000) /* -nan: nan */
CASE_F_F(nearbyintf, 0x00000001, 0x00000000) /* 1.40129846e-45: 0 */
CASE_F_F(nearbyintf, 0x80000001, 0x80000000) /* -1.40129846e-45: -0 */
CASE_F_F(nearbyintf, 0x00000002, 0x00000000) /* 2.80259693e-45: 0 */
CASE_F_F(nearbyintf, 0x80000002, 0x80000000) /* -2.80259693e-45: -0 */
CASE_F_F(nearbyintf, 0x00000003, 0x00000000) /* 4.20389539e-45: 0 */
CASE_F_F(nearbyintf, 0x80000003, 0x80000000) /* -4.20389539e-45: -0 */
CASE_F_F(nearbyintf, 0x003fffff, 0x00000000) /* 5.87747035e-39: 0 */
CASE_F_F(nearbyintf, 0x803fffff, 0x80000000) /* -5.87747035e-39: -0 */
CASE_F_F(nearbyintf, 0x00400000, 0x00000000) /* 5.87747175e-39: 0 */
CASE_F_F(nearbyintf, 0x80400000, 0x80000000) /* -5.87747175e-39: -0 */
CASE_F_F(nearbyintf, 0x007fffff, 0x00000000) /* 1.17549421e-38: 0 */
CASE_F_F(nearbyintf, 0x807fffff, 0x80000000) /* -1.17549421e-38: -0 */
CASE_F_F(nearbyintf, 0x00800000, 0x00000000) /* 1.17549435e-38: 0 */
CASE_F_F(nearbyintf, 0x80800000, 0x80000000) /* -1.17549435e-38: -0 */
CASE_F_F(nearbyintf, 0x00800001, 0x00000000) /* 1.17549449e-38: 0 */
CASE_F_F(nearbyintf, 0x80800001, 0x80000000) /* -1.17549449e-38: -0 */
CASE_F_F(nearbyintf, 0x00ffffff, 0x00000000) /* 2.35098856e-38: 0 */
CASE_F_F(nearbyintf, 0x80ffffff, 0x80000000) /* -2.35098856e-38: -0 */
CASE_F_F(nearbyintf, 0x01000000, 0x00000000) /* 2.3509887e-38: 0 */
CASE_F_F(nearbyintf, 0x81000000, 0x80000000) /* -2.3509887e-38: -0 */
CASE_F_F(nearbyintf, 0x3e800000, 0x00000000) /* 0.25: 0 */
CASE_F_F(nearbyintf, 0xbe800000, 0x80000000) /* -0.25: -0 */
CASE_F_F(nearbyintf, 0x3effffff, 0x00000000) /* 0.49999997: 0 */
CASE_F_F(nearbyintf, 0xbeffffff, 0x80000000) /* -0.49999997: -0 */
CASE_F_F(nearbyintf, 0x3f000000, 0x00000000) /* 0.5: 0 */
CASE_F_F(nearbyintf, 0xbf000000, 0x80000000) /* -0.5: -0 */
CASE_F_F(nearbyintf, 0x3f000001, 0x3f800000) /* 0.50000006: 1 */
CASE_F_F(nearbyintf, 0xbf000001, 0xbf800000) /* -0.50000006: -1 */
CASE_F_F(nearbyintf, 0x3f3fffff, 0x3f800000) /* 0.74999994: 1 */
CASE_F_F(nearbyintf, 0xbf3fffff, 0xbf800000) /* -0.74999994: -1 */
CASE_F_F(nearbyintf, 0x3f400000, 0x3f800000) /* 0.75: 1 */
CASE_F_F(nearbyintf, 0xbf400000, 0xbf800000) /* -0.75: -1 */
CASE_F_F(nearbyintf, 0x3f7fffff, 0x3f800000) /* 0.99999994: 1 */
CASE_F_F(nearbyintf, 0xbf7fffff, 0xbf800000) /* -0.99999994: -1 */
CASE_F_F(nearbyintf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(nearbyintf, 0xbf800000, 0xbf800000) /* -1: -1 */
CASE_F_F(nearbyintf, 0x3f800001, 0x3f800000) /* 1.00000012: 1 */
CASE_F_F(nearbyintf, 0xbf800001, 0xbf800000) /* -1.00000012: -1 */
CASE_F_F(nearbyintf, 0x3fbfffff, 0x3f800000) /* 1.49999988: 1 */
CASE_F_F(nearbyintf, 0xbfbfffff, 0xbf800000) /* -1.49999988: -1 */
CASE_F_F(nearbyintf, 0x3fc00000, 0x40000000) /* 1.5: 2 */
CASE_F_F(nearbyintf, 0xbfc00000, 0xc0000000) /* -1.5: -2 */
CASE_F_F(nearbyintf, 0x3fc00001, 0x40000000) /* 1.50000012: 2 */
CASE_F_F(nearbyintf, 0xbfc00001, 0xc0000000) /* -1.50000012: -2 */
CASE_F_F(nearbyintf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(nearbyintf, 0xc0000000, 0xc0000000) /* -2: -2 */
CASE_F_F(nearbyintf, 0x401fffff, 0x40000000) /* 2.49999976: 2 */
CASE_F_F(nearbyintf, 0xc01fffff, 0xc0000000) /* -2.49999976: -2 */
CASE_F_F(nearbyintf, 0x40200000, 0x40000000) /* 2.5: 2 */
CASE_F_F(nearbyintf, 0xc0200000, 0xc0000000) /* -2.5: -2 */
CASE_F_F(nearbyintf, 0x40200001, 0x40400000) /* 2.50000024: 3 */
CASE_F_F(nearbyintf, 0xc0200001, 0xc0400000) /* -2.50000024: -3 */
CASE_F_F(nearbyintf, 0x40600000, 0x40800000) /* 3.5: 4 */
CASE_F_F(nearbyintf, 0xc0600000, 0xc0800000) /* -3.5: -4 */
CASE_F_F(nearbyintf, 0x40900000, 0x40800000) /* 4.5: 4 */
CASE_F_F(nearbyintf, 0xc0900000, 0xc0800000) /* -4.5: -4 */
CASE_F_F(nearbyintf, 0x41280000, 0x41200000) /* 10.5: 10 */
CASE_F_F(nearbyintf, 0xc1280000, 0xc1200000) /* -10.5: -10 */
CASE_F_F(nearbyintf, 0x41380000, 0x41400000) /* 11.5: 12 */
CASE_F_F(nearbyintf, 0xc1380000, 0xc1400000) /* -11.5: -12 */
CASE_F_F(nearbyintf, 0x4a7fffff, 0x4a800000) /* 4194303.75: 4194304 */
CASE_F_F(nearbyintf, 0xca7fffff, 0xca800000) /* -4194303.75: -4194304 */
CASE_F_F(nearbyintf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(nearbyintf, 0xca800000, 0xca800000) /* -4194304: -4194304 */
CASE_F_F(nearbyintf, 0x4a800001, 0x4a800000) /* 4194304.5: 4194304 */
CASE_F_F(nearbyintf, 0xca800001, 0xca800000) /* -4194304.5: -4194304 */
CASE_F_F(nearbyintf, 0x4a800003, 0x4a800004) /* 4194305.5: 4194306 */
CASE_F_F(nearbyintf, 0xca800003, 0xca800004) /* -4194305.5: -4194306 */
CASE_F_F(nearbyintf, 0x4afffffd, 0x4afffffc) /* 8388606.5: 8388606 */
CASE_F_F(nearbyintf, 0xcafffffd, 0xcafffffc) /* -8388606.5: -8388606 */
CASE_F_F(nearbyintf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(nearbyintf, 0xcafffffe, 0xcafffffe) /* -8388607: -8388607 */
CASE_F_F(nearbyintf, 0x4affffff, 0x4b000000) /* 8388607.5: 8388608 */
CASE_F_F(nearbyintf, 0xcaffffff, 0xcb000000) /* -8388607.5: -8388608 */
CASE_F_F(nearbyintf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(nearbyintf, 0xcb000000, 0xcb000000) /* -8388608: -8388608 */
CASE_F_F(nearbyintf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(nearbyintf, 0xcb000001, 0xcb000001) /* -8388609: -8388609 */
CASE_F_F(nearbyintf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(nearbyintf, 0xcb7fffff, 0xcb7fffff) /* -16777215: -16777215 */
CASE_F_F(nearbyintf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(nearbyintf, 0xcb800000, 0xcb800000) /* -16777216: -16777216 */
CASE_F_F(nearbyintf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(nearbyintf, 0xcb800001, 0xcb800001) /* -16777218: -16777218 */
CASE_F_F(nearbyintf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(nearbyintf, 0xceffffff, 0xceffffff) /* -2.14748352e+09: -2.14748352e+09 */
CASE_F_F(nearbyintf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(nearbyintf, 0xcf000000, 0xcf000000) /* -2.14748365e+09: -2.14748365e+09 */
CASE_F_F(nearbyintf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(nearbyintf, 0xdeffffff, 0xdeffffff) /* -9.22337149e+18: -9.22337149e+18 */
CASE_F_F(nearbyintf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(nearbyintf, 0xdf000000, 0xdf000000) /* -9.22337204e+18: -9.22337204e+18 */
CASE_F_F(nearbyintf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(nearbyintf, 0xdf000001, 0xdf000001) /* -9.22337314e+18: -9.22337314e+18 */
CASE_F_F(nearbyintf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(nearbyintf, 0xff000000, 0xff000000) /* -1.70141183e+38: -1.70141183e+38 */
CASE_F_F(nearbyintf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(nearbyintf, 0xff7fffff, 0xff7fffff) /* -3.40282347e+38: -3.40282347e+38 */
CASE_F_F(nearbyintf, 0x407fffff, 0x40800000) /* 3.99999976: 4 */
CASE_F_F(nearbyintf, 0xc07fffff, 0xc0800000) /* -3.99999976: -4 */
CASE_F_F(nearbyintf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(nearbyintf, 0xc0800000, 0xc0800000) /* -4: -4 */
CASE_F_F(nearbyintf, 0x40800001, 0x40800000) /* 4.00000048: 4 */
CASE_F_F(nearbyintf, 0xc0800001, 0xc0800000) /* -4.00000048: -4 */
CASE_F_F(nearbyintf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(nearbyintf, 0xc1100000, 0xc1100000) /* -9: -9 */
CASE_F_F(nearbyintf, 0x3e800001, 0x00000000) /* 0.25000003: 0 */
CASE_F_F(nearbyintf, 0xbe800001, 0x80000000) /* -0.25000003: -0 */

/* fabsf: float(float), NAN_BITS */
CASE_F_F(fabsf, 0x00000000, 0x00000000) /* 0: 0 */
CASE_F_F(fabsf, 0x80000000, 0x00000000) /* -0: 0 */
CASE_F_F(fabsf, 0x7f800000, 0x7f800000) /* inf: inf */
CASE_F_F(fabsf, 0xff800000, 0x7f800000) /* -inf: inf */
CASE_F_F(fabsf, 0x7fc00000, 0x7fc00000) /* nan: nan */
CASE_F_F(fabsf, 0xffc00000, 0x7fc00000) /* -nan: nan */
CASE_F_F(fabsf, 0x7fc00001, 0x7fc00001) /* nan: nan */
CASE_F_F(fabsf, 0xffc00001, 0x7fc00001) /* -nan: nan */
CASE_F_F(fabsf, 0x7fffffff, 0x7fffffff) /* nan: nan */
CASE_F_F(fabsf, 0xffffffff, 0x7fffffff) /* -nan: nan */
CASE_F_F(fabsf, 0x7f800001, 0x7f800001) /* nan: nan */
CASE_F_F(fabsf, 0xff800001, 0x7f800001) /* -nan: nan */
CASE_F_F(fabsf, 0x7fbfffff, 0x7fbfffff) /* nan: nan */
CASE_F_F(fabsf, 0xffbfffff, 0x7fbfffff) /* -nan: nan */
CASE_F_F(fabsf, 0x00000001, 0x00000001) /* 1.40129846e-45: 1.40129846e-45 */
CASE_F_F(fabsf, 0x80000001, 0x00000001) /* -1.40129846e-45: 1.40129846e-45 */
CASE_F_F(fabsf, 0x00000002, 0x00000002) /* 2.80259693e-45: 2.80259693e-45 */
CASE_F_F(fabsf, 0x80000002, 0x00000002) /* -2.80259693e-45: 2.80259693e-45 */
CASE_F_F(fabsf, 0x00000003, 0x00000003) /* 4.20389539e-45: 4.20389539e-45 */
CASE_F_F(fabsf, 0x80000003, 0x00000003) /* -4.20389539e-45: 4.20389539e-45 */
CASE_F_F(fabsf, 0x003fffff, 0x003fffff) /* 5.87747035e-39: 5.87747035e-39 */
CASE_F_F(fabsf, 0x803fffff, 0x003fffff) /* -5.87747035e-39: 5.87747035e-39 */
CASE_F_F(fabsf, 0x00400000, 0x00400000) /* 5.87747175e-39: 5.87747175e-39 */
CASE_F_F(fabsf, 0x80400000, 0x00400000) /* -5.87747175e-39: 5.87747175e-39 */
CASE_F_F(fabsf, 0x007fffff, 0x007fffff) /* 1.17549421e-38: 1.17549421e-38 */
CASE_F_F(fabsf, 0x807fffff, 0x007fffff) /* -1.17549421e-38: 1.17549421e-38 */
CASE_F_F(fabsf, 0x00800000, 0x00800000) /* 1.17549435e-38: 1.17549435e-38 */
CASE_F_F(fabsf, 0x80800000, 0x00800000) /* -1.17549435e-38: 1.17549435e-38 */
CASE_F_F(fabsf, 0x00800001, 0x00800001) /* 1.17549449e-38: 1.17549449e-38 */
CASE_F_F(fabsf, 0x80800001, 0x00800001) /* -1.17549449e-38: 1.17549449e-38 */
CASE_F_F(fabsf, 0x00ffffff, 0x00ffffff) /* 2.35098856e-38: 2.35098856e-38 */
CASE_F_F(fabsf, 0x80ffffff, 0x00ffffff) /* -2.35098856e-38: 2.35098856e-38 */
CASE_F_F(fabsf, 0x01000000, 0x01000000) /* 2.3509887e-38: 2.3509887e-38 */
CASE_F_F(fabsf, 0x81000000, 0x01000000) /* -2.3509887e-38: 2.3509887e-38 */
CASE_F_F(fabsf, 0x3e800000, 0x3e800000) /* 0.25: 0.25 */
CASE_F_F(fabsf, 0xbe800000, 0x3e800000) /* -0.25: 0.25 */
CASE_F_F(fabsf, 0x3effffff, 0x3effffff) /* 0.49999997: 0.49999997 */
CASE_F_F(fabsf, 0xbeffffff, 0x3effffff) /* -0.49999997: 0.49999997 */
CASE_F_F(fabsf, 0x3f000000, 0x3f000000) /* 0.5: 0.5 */
CASE_F_F(fabsf, 0xbf000000, 0x3f000000) /* -0.5: 0.5 */
CASE_F_F(fabsf, 0x3f000001, 0x3f000001) /* 0.50000006: 0.50000006 */
CASE_F_F(fabsf, 0xbf000001, 0x3f000001) /* -0.50000006: 0.50000006 */
CASE_F_F(fabsf, 0x3f3fffff, 0x3f3fffff) /* 0.74999994: 0.74999994 */
CASE_F_F(fabsf, 0xbf3fffff, 0x3f3fffff) /* -0.74999994: 0.74999994 */
CASE_F_F(fabsf, 0x3f400000, 0x3f400000) /* 0.75: 0.75 */
CASE_F_F(fabsf, 0xbf400000, 0x3f400000) /* -0.75: 0.75 */
CASE_F_F(fabsf, 0x3f7fffff, 0x3f7fffff) /* 0.99999994: 0.99999994 */
CASE_F_F(fabsf, 0xbf7fffff, 0x3f7fffff) /* -0.99999994: 0.99999994 */
CASE_F_F(fabsf, 0x3f800000, 0x3f800000) /* 1: 1 */
CASE_F_F(fabsf, 0xbf800000, 0x3f800000) /* -1: 1 */
CASE_F_F(fabsf, 0x3f800001, 0x3f800001) /* 1.00000012: 1.00000012 */
CASE_F_F(fabsf, 0xbf800001, 0x3f800001) /* -1.00000012: 1.00000012 */
CASE_F_F(fabsf, 0x3fbfffff, 0x3fbfffff) /* 1.49999988: 1.49999988 */
CASE_F_F(fabsf, 0xbfbfffff, 0x3fbfffff) /* -1.49999988: 1.49999988 */
CASE_F_F(fabsf, 0x3fc00000, 0x3fc00000) /* 1.5: 1.5 */
CASE_F_F(fabsf, 0xbfc00000, 0x3fc00000) /* -1.5: 1.5 */
CASE_F_F(fabsf, 0x3fc00001, 0x3fc00001) /* 1.50000012: 1.50000012 */
CASE_F_F(fabsf, 0xbfc00001, 0x3fc00001) /* -1.50000012: 1.50000012 */
CASE_F_F(fabsf, 0x40000000, 0x40000000) /* 2: 2 */
CASE_F_F(fabsf, 0xc0000000, 0x40000000) /* -2: 2 */
CASE_F_F(fabsf, 0x401fffff, 0x401fffff) /* 2.49999976: 2.49999976 */
CASE_F_F(fabsf, 0xc01fffff, 0x401fffff) /* -2.49999976: 2.49999976 */
CASE_F_F(fabsf, 0x40200000, 0x40200000) /* 2.5: 2.5 */
CASE_F_F(fabsf, 0xc0200000, 0x40200000) /* -2.5: 2.5 */
CASE_F_F(fabsf, 0x40200001, 0x40200001) /* 2.50000024: 2.50000024 */
CASE_F_F(fabsf, 0xc0200001, 0x40200001) /* -2.50000024: 2.50000024 */
CASE_F_F(fabsf, 0x40600000, 0x40600000) /* 3.5: 3.5 */
CASE_F_F(fabsf, 0xc0600000, 0x40600000) /* -3.5: 3.5 */
CASE_F_F(fabsf, 0x40900000, 0x40900000) /* 4.5: 4.5 */
CASE_F_F(fabsf, 0xc0900000, 0x40900000) /* -4.5: 4.5 */
CASE_F_F(fabsf, 0x41280000, 0x41280000) /* 10.5: 10.5 */
CASE_F_F(fabsf, 0xc1280000, 0x41280000) /* -10.5: 10.5 */
CASE_F_F(fabsf, 0x41380000, 0x41380000) /* 11.5: 11.5 */
CASE_F_F(fabsf, 0xc1380000, 0x41380000) /* -11.5: 11.5 */
CASE_F_F(fabsf, 0x4a7fffff, 0x4a7fffff) /* 4194303.75: 4194303.75 */
CASE_F_F(fabsf, 0xca7fffff, 0x4a7fffff) /* -4194303.75: 4194303.75 */
CASE_F_F(fabsf, 0x4a800000, 0x4a800000) /* 4194304: 4194304 */
CASE_F_F(fabsf, 0xca800000, 0x4a800000) /* -4194304: 4194304 */
CASE_F_F(fabsf, 0x4a800001, 0x4a800001) /* 4194304.5: 4194304.5 */
CASE_F_F(fabsf, 0xca800001, 0x4a800001) /* -4194304.5: 4194304.5 */
CASE_F_F(fabsf, 0x4a800003, 0x4a800003) /* 4194305.5: 4194305.5 */
CASE_F_F(fabsf, 0xca800003, 0x4a800003) /* -4194305.5: 4194305.5 */
CASE_F_F(fabsf, 0x4afffffd, 0x4afffffd) /* 8388606.5: 8388606.5 */
CASE_F_F(fabsf, 0xcafffffd, 0x4afffffd) /* -8388606.5: 8388606.5 */
CASE_F_F(fabsf, 0x4afffffe, 0x4afffffe) /* 8388607: 8388607 */
CASE_F_F(fabsf, 0xcafffffe, 0x4afffffe) /* -8388607: 8388607 */
CASE_F_F(fabsf, 0x4affffff, 0x4affffff) /* 8388607.5: 8388607.5 */
CASE_F_F(fabsf, 0xcaffffff, 0x4affffff) /* -8388607.5: 8388607.5 */
CASE_F_F(fabsf, 0x4b000000, 0x4b000000) /* 8388608: 8388608 */
CASE_F_F(fabsf, 0xcb000000, 0x4b000000) /* -8388608: 8388608 */
CASE_F_F(fabsf, 0x4b000001, 0x4b000001) /* 8388609: 8388609 */
CASE_F_F(fabsf, 0xcb000001, 0x4b000001) /* -8388609: 8388609 */
CASE_F_F(fabsf, 0x4b7fffff, 0x4b7fffff) /* 16777215: 16777215 */
CASE_F_F(fabsf, 0xcb7fffff, 0x4b7fffff) /* -16777215: 16777215 */
CASE_F_F(fabsf, 0x4b800000, 0x4b800000) /* 16777216: 16777216 */
CASE_F_F(fabsf, 0xcb800000, 0x4b800000) /* -16777216: 16777216 */
CASE_F_F(fabsf, 0x4b800001, 0x4b800001) /* 16777218: 16777218 */
CASE_F_F(fabsf, 0xcb800001, 0x4b800001) /* -16777218: 16777218 */
CASE_F_F(fabsf, 0x4effffff, 0x4effffff) /* 2.14748352e+09: 2.14748352e+09 */
CASE_F_F(fabsf, 0xceffffff, 0x4effffff) /* -2.14748352e+09: 2.14748352e+09 */
CASE_F_F(fabsf, 0x4f000000, 0x4f000000) /* 2.14748365e+09: 2.14748365e+09 */
CASE_F_F(fabsf, 0xcf000000, 0x4f000000) /* -2.14748365e+09: 2.14748365e+09 */
CASE_F_F(fabsf, 0x5effffff, 0x5effffff) /* 9.22337149e+18: 9.22337149e+18 */
CASE_F_F(fabsf, 0xdeffffff, 0x5effffff) /* -9.22337149e+18: 9.22337149e+18 */
CASE_F_F(fabsf, 0x5f000000, 0x5f000000) /* 9.22337204e+18: 9.22337204e+18 */
CASE_F_F(fabsf, 0xdf000000, 0x5f000000) /* -9.22337204e+18: 9.22337204e+18 */
CASE_F_F(fabsf, 0x5f000001, 0x5f000001) /* 9.22337314e+18: 9.22337314e+18 */
CASE_F_F(fabsf, 0xdf000001, 0x5f000001) /* -9.22337314e+18: 9.22337314e+18 */
CASE_F_F(fabsf, 0x7f000000, 0x7f000000) /* 1.70141183e+38: 1.70141183e+38 */
CASE_F_F(fabsf, 0xff000000, 0x7f000000) /* -1.70141183e+38: 1.70141183e+38 */
CASE_F_F(fabsf, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38: 3.40282347e+38 */
CASE_F_F(fabsf, 0xff7fffff, 0x7f7fffff) /* -3.40282347e+38: 3.40282347e+38 */
CASE_F_F(fabsf, 0x407fffff, 0x407fffff) /* 3.99999976: 3.99999976 */
CASE_F_F(fabsf, 0xc07fffff, 0x407fffff) /* -3.99999976: 3.99999976 */
CASE_F_F(fabsf, 0x40800000, 0x40800000) /* 4: 4 */
CASE_F_F(fabsf, 0xc0800000, 0x40800000) /* -4: 4 */
CASE_F_F(fabsf, 0x40800001, 0x40800001) /* 4.00000048: 4.00000048 */
CASE_F_F(fabsf, 0xc0800001, 0x40800001) /* -4.00000048: 4.00000048 */
CASE_F_F(fabsf, 0x41100000, 0x41100000) /* 9: 9 */
CASE_F_F(fabsf, 0xc1100000, 0x41100000) /* -9: 9 */
CASE_F_F(fabsf, 0x3e800001, 0x3e800001) /* 0.25000003: 0.25000003 */
CASE_F_F(fabsf, 0xbe800001, 0x3e800001) /* -0.25000003: 0.25000003 */

/* lrintf: long(float), SATURATE */
CASE_L_F(lrintf, 0x00000000, 0L) /* 0 */
CASE_L_F(lrintf, 0x80000000, 0L) /* -0 */
CASE_L_F(lrintf, 0x7f800000, 9223372036854775807L) /* inf */
CASE_L_F(lrintf, 0xff800000, (-9223372036854775807L - 1)) /* -inf */
CASE_L_F(lrintf, 0x7fc00000, 0L) /* nan */
CASE_L_F(lrintf, 0xffc00000, 0L) /* -nan */
CASE_L_F(lrintf, 0x7fc00001, 0L) /* nan */
CASE_L_F(lrintf, 0xffc00001, 0L) /* -nan */
CASE_L_F(lrintf, 0x7fffffff, 0L) /* nan */
CASE_L_F(lrintf, 0xffffffff, 0L) /* -nan */
CASE_L_F(lrintf, 0x7f800001, 0L) /* nan */
CASE_L_F(lrintf, 0xff800001, 0L) /* -nan */
CASE_L_F(lrintf, 0x7fbfffff, 0L) /* nan */
CASE_L_F(lrintf, 0xffbfffff, 0L) /* -nan */
CASE_L_F(lrintf, 0x00000001, 0L) /* 1.40129846e-45 */
CASE_L_F(lrintf, 0x80000001, 0L) /* -1.40129846e-45 */
CASE_L_F(lrintf, 0x00000002, 0L) /* 2.80259693e-45 */
CASE_L_F(lrintf, 0x80000002, 0L) /* -2.80259693e-45 */
CASE_L_F(lrintf, 0x00000003, 0L) /* 4.20389539e-45 */
CASE_L_F(lrintf, 0x80000003, 0L) /* -4.20389539e-45 */
CASE_L_F(lrintf, 0x003fffff, 0L) /* 5.87747035e-39 */
CASE_L_F(lrintf, 0x803fffff, 0L) /* -5.87747035e-39 */
CASE_L_F(lrintf, 0x00400000, 0L) /* 5.87747175e-39 */
CASE_L_F(lrintf, 0x80400000, 0L) /* -5.87747175e-39 */
CASE_L_F(lrintf, 0x007fffff, 0L) /* 1.17549421e-38 */
CASE_L_F(lrintf, 0x807fffff, 0L) /* -1.17549421e-38 */
CASE_L_F(lrintf, 0x00800000, 0L) /* 1.17549435e-38 */
CASE_L_F(lrintf, 0x80800000, 0L) /* -1.17549435e-38 */
CASE_L_F(lrintf, 0x00800001, 0L) /* 1.17549449e-38 */
CASE_L_F(lrintf, 0x80800001, 0L) /* -1.17549449e-38 */
CASE_L_F(lrintf, 0x00ffffff, 0L) /* 2.35098856e-38 */
CASE_L_F(lrintf, 0x80ffffff, 0L) /* -2.35098856e-38 */
CASE_L_F(lrintf, 0x01000000, 0L) /* 2.3509887e-38 */
CASE_L_F(lrintf, 0x81000000, 0L) /* -2.3509887e-38 */
CASE_L_F(lrintf, 0x3e800000, 0L) /* 0.25 */
CASE_L_F(lrintf, 0xbe800000, 0L) /* -0.25 */
CASE_L_F(lrintf, 0x3effffff, 0L) /* 0.49999997 */
CASE_L_F(lrintf, 0xbeffffff, 0L) /* -0.49999997 */
CASE_L_F(lrintf, 0x3f000000, 0L) /* 0.5 */
CASE_L_F(lrintf, 0xbf000000, 0L) /* -0.5 */
CASE_L_F(lrintf, 0x3f000001, 1L) /* 0.50000006 */
CASE_L_F(lrintf, 0xbf000001, -1L) /* -0.50000006 */
CASE_L_F(lrintf, 0x3f3fffff, 1L) /* 0.74999994 */
CASE_L_F(lrintf, 0xbf3fffff, -1L) /* -0.74999994 */
CASE_L_F(lrintf, 0x3f400000, 1L) /* 0.75 */
CASE_L_F(lrintf, 0xbf400000, -1L) /* -0.75 */
CASE_L_F(lrintf, 0x3f7fffff, 1L) /* 0.99999994 */
CASE_L_F(lrintf, 0xbf7fffff, -1L) /* -0.99999994 */
CASE_L_F(lrintf, 0x3f800000, 1L) /* 1 */
CASE_L_F(lrintf, 0xbf800000, -1L) /* -1 */
CASE_L_F(lrintf, 0x3f800001, 1L) /* 1.00000012 */
CASE_L_F(lrintf, 0xbf800001, -1L) /* -1.00000012 */
CASE_L_F(lrintf, 0x3fbfffff, 1L) /* 1.49999988 */
CASE_L_F(lrintf, 0xbfbfffff, -1L) /* -1.49999988 */
CASE_L_F(lrintf, 0x3fc00000, 2L) /* 1.5 */
CASE_L_F(lrintf, 0xbfc00000, -2L) /* -1.5 */
CASE_L_F(lrintf, 0x3fc00001, 2L) /* 1.50000012 */
CASE_L_F(lrintf, 0xbfc00001, -2L) /* -1.50000012 */
CASE_L_F(lrintf, 0x40000000, 2L) /* 2 */
CASE_L_F(lrintf, 0xc0000000, -2L) /* -2 */
CASE_L_F(lrintf, 0x401fffff, 2L) /* 2.49999976 */
CASE_L_F(lrintf, 0xc01fffff, -2L) /* -2.49999976 */
CASE_L_F(lrintf, 0x40200000, 2L) /* 2.5 */
CASE_L_F(lrintf, 0xc0200000, -2L) /* -2.5 */
CASE_L_F(lrintf, 0x40200001, 3L) /* 2.50000024 */
CASE_L_F(lrintf, 0xc0200001, -3L) /* -2.50000024 */
CASE_L_F(lrintf, 0x40600000, 4L) /* 3.5 */
CASE_L_F(lrintf, 0xc0600000, -4L) /* -3.5 */
CASE_L_F(lrintf, 0x40900000, 4L) /* 4.5 */
CASE_L_F(lrintf, 0xc0900000, -4L) /* -4.5 */
CASE_L_F(lrintf, 0x41280000, 10L) /* 10.5 */
CASE_L_F(lrintf, 0xc1280000, -10L) /* -10.5 */
CASE_L_F(lrintf, 0x41380000, 12L) /* 11.5 */
CASE_L_F(lrintf, 0xc1380000, -12L) /* -11.5 */
CASE_L_F(lrintf, 0x4a7fffff, 4194304L) /* 4194303.75 */
CASE_L_F(lrintf, 0xca7fffff, -4194304L) /* -4194303.75 */
CASE_L_F(lrintf, 0x4a800000, 4194304L) /* 4194304 */
CASE_L_F(lrintf, 0xca800000, -4194304L) /* -4194304 */
CASE_L_F(lrintf, 0x4a800001, 4194304L) /* 4194304.5 */
CASE_L_F(lrintf, 0xca800001, -4194304L) /* -4194304.5 */
CASE_L_F(lrintf, 0x4a800003, 4194306L) /* 4194305.5 */
CASE_L_F(lrintf, 0xca800003, -4194306L) /* -4194305.5 */
CASE_L_F(lrintf, 0x4afffffd, 8388606L) /* 8388606.5 */
CASE_L_F(lrintf, 0xcafffffd, -8388606L) /* -8388606.5 */
CASE_L_F(lrintf, 0x4afffffe, 8388607L) /* 8388607 */
CASE_L_F(lrintf, 0xcafffffe, -8388607L) /* -8388607 */
CASE_L_F(lrintf, 0x4affffff, 8388608L) /* 8388607.5 */
CASE_L_F(lrintf, 0xcaffffff, -8388608L) /* -8388607.5 */
CASE_L_F(lrintf, 0x4b000000, 8388608L) /* 8388608 */
CASE_L_F(lrintf, 0xcb000000, -8388608L) /* -8388608 */
CASE_L_F(lrintf, 0x4b000001, 8388609L) /* 8388609 */
CASE_L_F(lrintf, 0xcb000001, -8388609L) /* -8388609 */
CASE_L_F(lrintf, 0x4b7fffff, 16777215L) /* 16777215 */
CASE_L_F(lrintf, 0xcb7fffff, -16777215L) /* -16777215 */
CASE_L_F(lrintf, 0x4b800000, 16777216L) /* 16777216 */
CASE_L_F(lrintf, 0xcb800000, -16777216L) /* -16777216 */
CASE_L_F(lrintf, 0x4b800001, 16777218L) /* 16777218 */
CASE_L_F(lrintf, 0xcb800001, -16777218L) /* -16777218 */
CASE_L_F(lrintf, 0x4effffff, 2147483520L) /* 2.14748352e+09 */
CASE_L_F(lrintf, 0xceffffff, -2147483520L) /* -2.14748352e+09 */
CASE_L_F(lrintf, 0x4f000000, 2147483648L) /* 2.14748365e+09 */
CASE_L_F(lrintf, 0xcf000000, -2147483648L) /* -2.14748365e+09 */
CASE_L_F(lrintf, 0x5effffff, 9223371487098961920L) /* 9.22337149e+18 */
CASE_L_F(lrintf, 0xdeffffff, -9223371487098961920L) /* -9.22337149e+18 */
CASE_L_F(lrintf, 0x5f000000, 9223372036854775807L) /* 9.22337204e+18 */
CASE_L_F(lrintf, 0xdf000000, (-9223372036854775807L - 1)) /* -9.22337204e+18 */
CASE_L_F(lrintf, 0x5f000001, 9223372036854775807L) /* 9.22337314e+18 */
CASE_L_F(lrintf, 0xdf000001, (-9223372036854775807L - 1)) /* -9.22337314e+18 */
CASE_L_F(lrintf, 0x7f000000, 9223372036854775807L) /* 1.70141183e+38 */
CASE_L_F(lrintf, 0xff000000, (-9223372036854775807L - 1)) /* -1.70141183e+38 */
CASE_L_F(lrintf, 0x7f7fffff, 9223372036854775807L) /* 3.40282347e+38 */
CASE_L_F(lrintf, 0xff7fffff, (-9223372036854775807L - 1)) /* -3.40282347e+38 */
CASE_L_F(lrintf, 0x407fffff, 4L) /* 3.99999976 */
CASE_L_F(lrintf, 0xc07fffff, -4L) /* -3.99999976 */
CASE_L_F(lrintf, 0x40800000, 4L) /* 4 */
CASE_L_F(lrintf, 0xc0800000, -4L) /* -4 */
CASE_L_F(lrintf, 0x40800001, 4L) /* 4.00000048 */
CASE_L_F(lrintf, 0xc0800001, -4L) /* -4.00000048 */
CASE_L_F(lrintf, 0x41100000, 9L) /* 9 */
CASE_L_F(lrintf, 0xc1100000, -9L) /* -9 */
CASE_L_F(lrintf, 0x3e800001, 0L) /* 0.25000003 */
CASE_L_F(lrintf, 0xbe800001, 0L) /* -0.25000003 */

/* frexpf: float(float, int *), NAN_QUIET */
CASE_F_FE(frexpf, 0x00000000, 0x00000000, 0) /* 0: 0, 0 */
CASE_F_FE(frexpf, 0x80000000, 0x80000000, 0) /* -0: -0, 0 */
CASE_F_FE(frexpf, 0x7f800000, 0x7f800000, 0) /* inf: inf, 0 */
CASE_F_FE(frexpf, 0xff800000, 0xff800000, 0) /* -inf: -inf, 0 */
CASE_F_FE(frexpf, 0x7fc00000, 0x7fc00000, 0) /* nan: nan, 0 */
CASE_F_FE(frexpf, 0xffc00000, 0x7fc00000, 0) /* -nan: nan, 0 */
CASE_F_FE(frexpf, 0x7fc00001, 0x7fc00000, 0) /* nan: nan, 0 */
CASE_F_FE(frexpf, 0xffc00001, 0x7fc00000, 0) /* -nan: nan, 0 */
CASE_F_FE(frexpf, 0x7fffffff, 0x7fc00000, 0) /* nan: nan, 0 */
CASE_F_FE(frexpf, 0xffffffff, 0x7fc00000, 0) /* -nan: nan, 0 */
CASE_F_FE(frexpf, 0x7f800001, 0x7fc00000, 0) /* nan: nan, 0 */
CASE_F_FE(frexpf, 0xff800001, 0x7fc00000, 0) /* -nan: nan, 0 */
CASE_F_FE(frexpf, 0x7fbfffff, 0x7fc00000, 0) /* nan: nan, 0 */
CASE_F_FE(frexpf, 0xffbfffff, 0x7fc00000, 0) /* -nan: nan, 0 */
CASE_F_FE(frexpf, 0x00000001, 0x3f000000, -148) /* 1.40129846e-45: 0.5, -148 */
CASE_F_FE(frexpf, 0x80000001, 0xbf000000, -148) /* -1.40129846e-45: -0.5, -148 */
CASE_F_FE(frexpf, 0x00000002, 0x3f000000, -147) /* 2.80259693e-45: 0.5, -147 */
CASE_F_FE(frexpf, 0x80000002, 0xbf000000, -147) /* -2.80259693e-45: -0.5, -147 */
CASE_F_FE(frexpf, 0x00000003, 0x3f400000, -147) /* 4.20389539e-45: 0.75, -147 */
CASE_F_FE(frexpf, 0x80000003, 0xbf400000, -147) /* -4.20389539e-45: -0.75, -147 */
CASE_F_FE(frexpf, 0x003fffff, 0x3f7ffffc, -127) /* 5.87747035e-39: 0.999999762, -127 */
CASE_F_FE(frexpf, 0x803fffff, 0xbf7ffffc, -127) /* -5.87747035e-39: -0.999999762, -127 */
CASE_F_FE(frexpf, 0x00400000, 0x3f000000, -126) /* 5.87747175e-39: 0.5, -126 */
CASE_F_FE(frexpf, 0x80400000, 0xbf000000, -126) /* -5.87747175e-39: -0.5, -126 */
CASE_F_FE(frexpf, 0x007fffff, 0x3f7ffffe, -126) /* 1.17549421e-38: 0.999999881, -126 */
CASE_F_FE(frexpf, 0x807fffff, 0xbf7ffffe, -126) /* -1.17549421e-38: -0.999999881, -126 */
CASE_F_FE(frexpf, 0x00800000, 0x3f000000, -125) /* 1.17549435e-38: 0.5, -125 */
CASE_F_FE(frexpf, 0x80800000, 0xbf000000, -125) /* -1.17549435e-38: -0.5, -125 */
CASE_F_FE(frexpf, 0x00800001, 0x3f000001, -125) /* 1.17549449e-38: 0.50000006, -125 */
CASE_F_FE(frexpf, 0x80800001, 0xbf000001, -125) /* -1.17549449e-38: -0.50000006, -125 */
CASE_F_FE(frexpf, 0x00ffffff, 0x3f7fffff, -125) /* 2.35098856e-38: 0.99999994, -125 */
CASE_F_FE(frexpf, 0x80ffffff, 0xbf7fffff, -125) /* -2.35098856e-38: -0.99999994, -125 */
CASE_F_FE(frexpf, 0x01000000, 0x3f000000, -124) /* 2.3509887e-38: 0.5, -124 */
CASE_F_FE(frexpf, 0x81000000, 0xbf000000, -124) /* -2.3509887e-38: -0.5, -124 */
CASE_F_FE(frexpf, 0x3e800000, 0x3f000000, -1) /* 0.25: 0.5, -1 */
CASE_F_FE(frexpf, 0xbe800000, 0xbf000000, -1) /* -0.25: -0.5, -1 */
CASE_F_FE(frexpf, 0x3effffff, 0x3f7fffff, -1) /* 0.49999997: 0.99999994, -1 */
CASE_F_FE(frexpf, 0xbeffffff, 0xbf7fffff, -1) /* -0.49999997: -0.99999994, -1 */
CASE_F_FE(frexpf, 0x3f000000, 0x3f000000, 0) /* 0.5: 0.5, 0 */
CASE_F_FE(frexpf, 0xbf000000, 0xbf000000, 0) /* -0.5: -0.5, 0 */
CASE_F_FE(frexpf, 0x3f000001, 0x3f000001, 0) /* 0.50000006: 0.50000006, 0 */
CASE_F_FE(frexpf, 0xbf000001, 0xbf000001, 0) /* -0.50000006: -0.50000006, 0 */
CASE_F_FE(frexpf, 0x3f3fffff, 0x3f3fffff, 0) /* 0.74999994: 0.74999994, 0 */
CASE_F_FE(frexpf, 0xbf3fffff, 0xbf3fffff, 0) /* -0.74999994: -0.74999994, 0 */
CASE_F_FE(frexpf, 0x3f400000, 0x3f400000, 0) /* 0.75: 0.75, 0 */
CASE_F_FE(frexpf, 0xbf400000, 0xbf400000, 0) /* -0.75: -0.75, 0 */
CASE_F_FE(frexpf, 0x3f7fffff, 0x3f7fffff, 0) /* 0.99999994: 0.99999994, 0 */
CASE_F_FE(frexpf, 0xbf7fffff, 0xbf7fffff, 0) /* -0.99999994: -0.99999994, 0 */
CASE_F_FE(frexpf, 0x3f800000, 0x3f000000, 1) /* 1: 0.5, 1 */
CASE_F_FE(frexpf, 0xbf800000, 0xbf000000, 1) /* -1: -0.5, 1 */
CASE_F_FE(frexpf, 0x3f800001, 0x3f000001, 1) /* 1.00000012: 0.50000006, 1 */
CASE_F_FE(frexpf, 0xbf800001, 0xbf000001, 1) /* -1.00000012: -0.50000006, 1 */
CASE_F_FE(frexpf, 0x3fbfffff, 0x3f3fffff, 1) /* 1.49999988: 0.74999994, 1 */
CASE_F_FE(frexpf, 0xbfbfffff, 0xbf3fffff, 1) /* -1.49999988: -0.74999994, 1 */
CASE_F_FE(frexpf, 0x3fc00000, 0x3f400000, 1) /* 1.5: 0.75, 1 */
CASE_F_FE(frexpf, 0xbfc00000, 0xbf400000, 1) /* -1.5: -0.75, 1 */
CASE_F_FE(frexpf, 0x3fc00001, 0x3f400001, 1) /* 1.50000012: 0.75000006, 1 */
CASE_F_FE(frexpf, 0xbfc00001, 0xbf400001, 1) /* -1.50000012: -0.75000006, 1 */
CASE_F_FE(frexpf, 0x40000000, 0x3f000000, 2) /* 2: 0.5, 2 */
CASE_F_FE(frexpf, 0xc0000000, 0xbf000000, 2) /* -2: -0.5, 2 */
CASE_F_FE(frexpf, 0x401fffff, 0x3f1fffff, 2) /* 2.49999976: 0.62499994, 2 */
CASE_F_FE(frexpf, 0xc01fffff, 0xbf1fffff, 2) /* -2.49999976: -0.62499994, 2 */
CASE_F_FE(frexpf, 0x40200000, 0x3f200000, 2) /* 2.5: 0.625, 2 */
CASE_F_FE(frexpf, 0xc0200000, 0xbf200000, 2) /* -2.5: -0.625, 2 */
CASE_F_FE(frexpf, 0x40200001, 0x3f200001, 2) /* 2.50000024: 0.62500006, 2 */
CASE_F_FE(frexpf, 0xc0200001, 0xbf200001, 2) /* -2.50000024: -0.62500006, 2 */
CASE_F_FE(frexpf, 0x40600000, 0x3f600000, 2) /* 3.5: 0.875, 2 */
CASE_F_FE(frexpf, 0xc0600000, 0xbf600000, 2) /* -3.5: -0.875, 2 */
CASE_F_FE(frexpf, 0x40900000, 0x3f100000, 3) /* 4.5: 0.5625, 3 */
CASE_F_FE(frexpf, 0xc0900000, 0xbf100000, 3) /* -4.5: -0.5625, 3 */
CASE_F_FE(frexpf, 0x41280000, 0x3f280000, 4) /* 10.5: 0.65625, 4 */
CASE_F_FE(frexpf, 0xc1280000, 0xbf280000, 4) /* -10.5: -0.65625, 4 */
CASE_F_FE(frexpf, 0x41380000, 0x3f380000, 4) /* 11.5: 0.71875, 4 */
CASE_F_FE(frexpf, 0xc1380000, 0xbf380000, 4) /* -11.5: -0.71875, 4 */
CASE_F_FE(frexpf, 0x4a7fffff, 0x3f7fffff, 22) /* 4194303.75: 0.99999994, 22 */
CASE_F_FE(frexpf, 0xca7fffff, 0xbf7fffff, 22) /* -4194303.75: -0.99999994, 22 */
CASE_F_FE(frexpf, 0x4a800000, 0x3f000000, 23) /* 4194304: 0.5, 23 */
CASE_F_FE(frexpf, 0xca800000, 0xbf000000, 23) /* -4194304: -0.5, 23 */
CASE_F_FE(frexpf, 0x4a800001, 0x3f000001, 23) /* 4194304.5: 0.50000006, 23 */
CASE_F_FE(frexpf, 0xca800001, 0xbf000001, 23) /* -4194304.5: -0.50000006, 23 */
CASE_F_FE(frexpf, 0x4a800003, 0x3f000003, 23) /* 4194305.5: 0.500000179, 23 */
CASE_F_FE(frexpf, 0xca800003, 0xbf000003, 23) /* -4194305.5: -0.500000179, 23 */
CASE_F_FE(frexpf, 0x4afffffd, 0x3f7ffffd, 23) /* 8388606.5: 0.999999821, 23 */
CASE_F_FE(frexpf, 0xcafffffd, 0xbf7ffffd, 23) /* -8388606.5: -0.999999821, 23 */
CASE_F_FE(frexpf, 0x4afffffe, 0x3f7ffffe, 23) /* 8388607: 0.999999881, 23 */
CASE_F_FE(frexpf, 0xcafffffe, 0xbf7ffffe, 23) /* -8388607: -0.999999881, 23 */
CASE_F_FE(frexpf, 0x4affffff, 0x3f7fffff, 23) /* 8388607.5: 0.99999994, 23 */
CASE_F_FE(frexpf, 0xcaffffff, 0xbf7fffff, 23) /* -8388607.5: -0.99999994, 23 */
CASE_F_FE(frexpf, 0x4b000000, 0x3f000000, 24) /* 8388608: 0.5, 24 */
CASE_F_FE(frexpf, 0xcb000000, 0xbf000000, 24) /* -8388608: -0.5, 24 */
CASE_F_FE(frexpf, 0x4b000001, 0x3f000001, 24) /* 8388609: 0.50000006, 24 */
CASE_F_FE(frexpf, 0xcb000001, 0xbf000001, 24) /* -8388609: -0.50000006, 24 */
CASE_F_FE(frexpf, 0x4b7fffff, 0x3f7fffff, 24) /* 16777215: 0.99999994, 24 */
CASE_F_FE(frexpf, 0xcb7fffff, 0xbf7fffff, 24) /* -16777215: -0.99999994, 24 */
CASE_F_FE(frexpf, 0x4b800000, 0x3f000000, 25) /* 16777216: 0.5, 25 */
CASE_F_FE(frexpf, 0xcb800000, 0xbf000000, 25) /* -16777216: -0.5, 25 */
CASE_F_FE(frexpf, 0x4b800001, 0x3f000001, 25) /* 16777218: 0.50000006, 25 */
CASE_F_FE(frexpf, 0xcb800001, 0xbf000001, 25) /* -16777218: -0.50000006, 25 */
CASE_F_FE(frexpf, 0x4effffff, 0x3f7fffff, 31) /* 2.14748352e+09: 0.99999994, 31 */
CASE_F_FE(frexpf, 0xceffffff, 0xbf7fffff, 31) /* -2.14748352e+09: -0.99999994, 31 */
CASE_F_FE(frexpf, 0x4f000000, 0x3f000000, 32) /* 2.14748365e+09: 0.5, 32 */
CASE_F_FE(frexpf, 0xcf000000, 0xbf000000, 32) /* -2.14748365e+09: -0.5, 32 */
CASE_F_FE(frexpf, 0x5effffff, 0x3f7fffff, 63) /* 9.22337149e+18: 0.99999994, 63 */
CASE_F_FE(frexpf, 0xdeffffff, 0xbf7fffff, 63) /* -9.22337149e+18: -0.99999994, 63 */
CASE_F_FE(frexpf, 0x5f000000, 0x3f000000, 64) /* 9.22337204e+18: 0.5, 64 */
CASE_F_FE(frexpf, 0xdf000000, 0xbf000000, 64) /* -9.22337204e+18: -0.5, 64 */
CASE_F_FE(frexpf, 0x5f000001, 0x3f000001, 64) /* 9.22337314e+18: 0.50000006, 64 */
CASE_F_FE(frexpf, 0xdf000001, 0xbf000001, 64) /* -9.22337314e+18: -0.50000006, 64 */
CASE_F_FE(frexpf, 0x7f000000, 0x3f000000, 128) /* 1.70141183e+38: 0.5, 128 */
CASE_F_FE(frexpf, 0xff000000, 0xbf000000, 128) /* -1.70141183e+38: -0.5, 128 */
CASE_F_FE(frexpf, 0x7f7fffff, 0x3f7fffff, 128) /* 3.40282347e+38: 0.99999994, 128 */
CASE_F_FE(frexpf, 0xff7fffff, 0xbf7fffff, 128) /* -3.40282347e+38: -0.99999994, 128 */
CASE_F_FE(frexpf, 0x407fffff, 0x3f7fffff, 2) /* 3.99999976: 0.99999994, 2 */
CASE_F_FE(frexpf, 0xc07fffff, 0xbf7fffff, 2) /* -3.99999976: -0.99999994, 2 */
CASE_F_FE(frexpf, 0x40800000, 0x3f000000, 3) /* 4: 0.5, 3 */
CASE_F_FE(frexpf, 0xc0800000, 0xbf000000, 3) /* -4: -0.5, 3 */
CASE_F_FE(frexpf, 0x40800001, 0x3f000001, 3) /* 4.00000048: 0.50000006, 3 */
CASE_F_FE(frexpf, 0xc0800001, 0xbf000001, 3) /* -4.00000048: -0.50000006, 3 */
CASE_F_FE(frexpf, 0x41100000, 0x3f100000, 4) /* 9: 0.5625, 4 */
CASE_F_FE(frexpf, 0xc1100000, 0xbf100000, 4) /* -9: -0.5625, 4 */
CASE_F_FE(frexpf, 0x3e800001, 0x3f000001, -1) /* 0.25000003: 0.50000006, -1 */
CASE_F_FE(frexpf, 0xbe800001, 0xbf000001, -1) /* -0.25000003: -0.50000006, -1 */

/* ldexpf: float(float, int), NAN_QUIET */
CASE_F_FI(ldexpf, 0x00000000, -2147483648, 0x00000000) /* 0, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x00000000, -1000, 0x00000000) /* 0, -1000: 0 */
CASE_F_FI(ldexpf, 0x00000000, -300, 0x00000000) /* 0, -300: 0 */
CASE_F_FI(ldexpf, 0x00000000, -278, 0x00000000) /* 0, -278: 0 */
CASE_F_FI(ldexpf, 0x00000000, -277, 0x00000000) /* 0, -277: 0 */
CASE_F_FI(ldexpf, 0x00000000, -276, 0x00000000) /* 0, -276: 0 */
CASE_F_FI(ldexpf, 0x00000000, -254, 0x00000000) /* 0, -254: 0 */
CASE_F_FI(ldexpf, 0x00000000, -253, 0x00000000) /* 0, -253: 0 */
CASE_F_FI(ldexpf, 0x00000000, -150, 0x00000000) /* 0, -150: 0 */
CASE_F_FI(ldexpf, 0x00000000, -149, 0x00000000) /* 0, -149: 0 */
CASE_F_FI(ldexpf, 0x00000000, -148, 0x00000000) /* 0, -148: 0 */
CASE_F_FI(ldexpf, 0x00000000, -127, 0x00000000) /* 0, -127: 0 */
CASE_F_FI(ldexpf, 0x00000000, -126, 0x00000000) /* 0, -126: 0 */
CASE_F_FI(ldexpf, 0x00000000, -125, 0x00000000) /* 0, -125: 0 */
CASE_F_FI(ldexpf, 0x00000000, -24, 0x00000000) /* 0, -24: 0 */
CASE_F_FI(ldexpf, 0x00000000, -1, 0x00000000) /* 0, -1: 0 */
CASE_F_FI(ldexpf, 0x00000000, 0, 0x00000000) /* 0, 0: 0 */
CASE_F_FI(ldexpf, 0x00000000, 1, 0x00000000) /* 0, 1: 0 */
CASE_F_FI(ldexpf, 0x00000000, 23, 0x00000000) /* 0, 23: 0 */
CASE_F_FI(ldexpf, 0x00000000, 127, 0x00000000) /* 0, 127: 0 */
CASE_F_FI(ldexpf, 0x00000000, 128, 0x00000000) /* 0, 128: 0 */
CASE_F_FI(ldexpf, 0x00000000, 253, 0x00000000) /* 0, 253: 0 */
CASE_F_FI(ldexpf, 0x00000000, 254, 0x00000000) /* 0, 254: 0 */
CASE_F_FI(ldexpf, 0x00000000, 276, 0x00000000) /* 0, 276: 0 */
CASE_F_FI(ldexpf, 0x00000000, 277, 0x00000000) /* 0, 277: 0 */
CASE_F_FI(ldexpf, 0x00000000, 278, 0x00000000) /* 0, 278: 0 */
CASE_F_FI(ldexpf, 0x00000000, 300, 0x00000000) /* 0, 300: 0 */
CASE_F_FI(ldexpf, 0x00000000, 1000, 0x00000000) /* 0, 1000: 0 */
CASE_F_FI(ldexpf, 0x00000000, 2147483647, 0x00000000) /* 0, 2147483647: 0 */
CASE_F_FI(ldexpf, 0x80000000, -2147483648, 0x80000000) /* -0, -2147483648: -0 */
CASE_F_FI(ldexpf, 0x80000000, -1000, 0x80000000) /* -0, -1000: -0 */
CASE_F_FI(ldexpf, 0x80000000, -300, 0x80000000) /* -0, -300: -0 */
CASE_F_FI(ldexpf, 0x80000000, -278, 0x80000000) /* -0, -278: -0 */
CASE_F_FI(ldexpf, 0x80000000, -277, 0x80000000) /* -0, -277: -0 */
CASE_F_FI(ldexpf, 0x80000000, -276, 0x80000000) /* -0, -276: -0 */
CASE_F_FI(ldexpf, 0x80000000, -254, 0x80000000) /* -0, -254: -0 */
CASE_F_FI(ldexpf, 0x80000000, -253, 0x80000000) /* -0, -253: -0 */
CASE_F_FI(ldexpf, 0x80000000, -150, 0x80000000) /* -0, -150: -0 */
CASE_F_FI(ldexpf, 0x80000000, -149, 0x80000000) /* -0, -149: -0 */
CASE_F_FI(ldexpf, 0x80000000, -148, 0x80000000) /* -0, -148: -0 */
CASE_F_FI(ldexpf, 0x80000000, -127, 0x80000000) /* -0, -127: -0 */
CASE_F_FI(ldexpf, 0x80000000, -126, 0x80000000) /* -0, -126: -0 */
CASE_F_FI(ldexpf, 0x80000000, -125, 0x80000000) /* -0, -125: -0 */
CASE_F_FI(ldexpf, 0x80000000, -24, 0x80000000) /* -0, -24: -0 */
CASE_F_FI(ldexpf, 0x80000000, -1, 0x80000000) /* -0, -1: -0 */
CASE_F_FI(ldexpf, 0x80000000, 0, 0x80000000) /* -0, 0: -0 */
CASE_F_FI(ldexpf, 0x80000000, 1, 0x80000000) /* -0, 1: -0 */
CASE_F_FI(ldexpf, 0x80000000, 23, 0x80000000) /* -0, 23: -0 */
CASE_F_FI(ldexpf, 0x80000000, 127, 0x80000000) /* -0, 127: -0 */
CASE_F_FI(ldexpf, 0x80000000, 128, 0x80000000) /* -0, 128: -0 */
CASE_F_FI(ldexpf, 0x80000000, 253, 0x80000000) /* -0, 253: -0 */
CASE_F_FI(ldexpf, 0x80000000, 254, 0x80000000) /* -0, 254: -0 */
CASE_F_FI(ldexpf, 0x80000000, 276, 0x80000000) /* -0, 276: -0 */
CASE_F_FI(ldexpf, 0x80000000, 277, 0x80000000) /* -0, 277: -0 */
CASE_F_FI(ldexpf, 0x80000000, 278, 0x80000000) /* -0, 278: -0 */
CASE_F_FI(ldexpf, 0x80000000, 300, 0x80000000) /* -0, 300: -0 */
CASE_F_FI(ldexpf, 0x80000000, 1000, 0x80000000) /* -0, 1000: -0 */
CASE_F_FI(ldexpf, 0x80000000, 2147483647, 0x80000000) /* -0, 2147483647: -0 */
CASE_F_FI(ldexpf, 0x00000001, -2147483648, 0x00000000) /* 1.40129846e-45, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x00000001, -1000, 0x00000000) /* 1.40129846e-45, -1000: 0 */
CASE_F_FI(ldexpf, 0x00000001, -300, 0x00000000) /* 1.40129846e-45, -300: 0 */
CASE_F_FI(ldexpf, 0x00000001, -278, 0x00000000) /* 1.40129846e-45, -278: 0 */
CASE_F_FI(ldexpf, 0x00000001, -277, 0x00000000) /* 1.40129846e-45, -277: 0 */
CASE_F_FI(ldexpf, 0x00000001, -276, 0x00000000) /* 1.40129846e-45, -276: 0 */
CASE_F_FI(ldexpf, 0x00000001, -254, 0x00000000) /* 1.40129846e-45, -254: 0 */
CASE_F_FI(ldexpf, 0x00000001, -253, 0x00000000) /* 1.40129846e-45, -253: 0 */
CASE_F_FI(ldexpf, 0x00000001, -150, 0x00000000) /* 1.40129846e-45, -150: 0 */
CASE_F_FI(ldexpf, 0x00000001, -149, 0x00000000) /* 1.40129846e-45, -149: 0 */
CASE_F_FI(ldexpf, 0x00000001, -148, 0x00000000) /* 1.40129846e-45, -148: 0 */
CASE_F_FI(ldexpf, 0x00000001, -127, 0x00000000) /* 1.40129846e-45, -127: 0 */
CASE_F_FI(ldexpf, 0x00000001, -126, 0x00000000) /* 1.40129846e-45, -126: 0 */
CASE_F_FI(ldexpf, 0x00000001, -125, 0x00000000) /* 1.40129846e-45, -125: 0 */
CASE_F_FI(ldexpf, 0x00000001, -24, 0x00000000) /* 1.40129846e-45, -24: 0 */
CASE_F_FI(ldexpf, 0x00000001, -1, 0x00000000) /* 1.40129846e-45, -1: 0 */
CASE_F_FI(ldexpf, 0x00000001, 0, 0x00000001) /* 1.40129846e-45, 0: 1.40129846e-45 */
CASE_F_FI(ldexpf, 0x00000001, 1, 0x00000002) /* 1.40129846e-45, 1: 2.80259693e-45 */
CASE_F_FI(ldexpf, 0x00000001, 23, 0x00800000) /* 1.40129846e-45, 23: 1.17549435e-38 */
CASE_F_FI(ldexpf, 0x00000001, 127, 0x34800000) /* 1.40129846e-45, 127: 2.38418579e-07 */
CASE_F_FI(ldexpf, 0x00000001, 128, 0x35000000) /* 1.40129846e-45, 128: 4.76837158e-07 */
CASE_F_FI(ldexpf, 0x00000001, 253, 0x73800000) /* 1.40129846e-45, 253: 2.02824096e+31 */
CASE_F_FI(ldexpf, 0x00000001, 254, 0x74000000) /* 1.40129846e-45, 254: 4.05648192e+31 */
CASE_F_FI(ldexpf, 0x00000001, 276, 0x7f000000) /* 1.40129846e-45, 276: 1.70141183e+38 */
CASE_F_FI(ldexpf, 0x00000001, 277, 0x7f800000) /* 1.40129846e-45, 277: inf */
CASE_F_FI(ldexpf, 0x00000001, 278, 0x7f800000) /* 1.40129846e-45, 278: inf */
CASE_F_FI(ldexpf, 0x00000001, 300, 0x7f800000) /* 1.40129846e-45, 300: inf */
CASE_F_FI(ldexpf, 0x00000001, 1000, 0x7f800000) /* 1.40129846e-45, 1000: inf */
CASE_F_FI(ldexpf, 0x00000001, 2147483647, 0x7f800000) /* 1.40129846e-45, 2147483647: inf */
CASE_F_FI(ldexpf, 0x00000003, -2147483648, 0x00000000) /* 4.20389539e-45, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x00000003, -1000, 0x00000000) /* 4.20389539e-45, -1000: 0 */
CASE_F_FI(ldexpf, 0x00000003, -300, 0x00000000) /* 4.20389539e-45, -300: 0 */
CASE_F_FI(ldexpf, 0x00000003, -278, 0x00000000) /* 4.20389539e-45, -278: 0 */
CASE_F_FI(ldexpf, 0x00000003, -277, 0x00000000) /* 4.20389539e-45, -277: 0 */
CASE_F_FI(ldexpf, 0x00000003, -276, 0x00000000) /* 4.20389539e-45, -276: 0 */
CASE_F_FI(ldexpf, 0x00000003, -254, 0x00000000) /* 4.20389539e-45, -254: 0 */
CASE_F_FI(ldexpf, 0x00000003, -253, 0x00000000) /* 4.20389539e-45, -253: 0 */
CASE_F_FI(ldexpf, 0x00000003, -150, 0x00000000) /* 4.20389539e-45, -150: 0 */
CASE_F_FI(ldexpf, 0x00000003, -149, 0x00000000) /* 4.20389539e-45, -149: 0 */
CASE_F_FI(ldexpf, 0x00000003, -148, 0x00000000) /* 4.20389539e-45, -148: 0 */
CASE_F_FI(ldexpf, 0x00000003, -127, 0x00000000) /* 4.20389539e-45, -127: 0 */
CASE_F_FI(ldexpf, 0x00000003, -126, 0x00000000) /* 4.20389539e-45, -126: 0 */
CASE_F_FI(ldexpf, 0x00000003, -125, 0x00000000) /* 4.20389539e-45, -125: 0 */
CASE_F_FI(ldexpf, 0x00000003, -24, 0x00000000) /* 4.20389539e-45, -24: 0 */
CASE_F_FI(ldexpf, 0x00000003, -1, 0x00000002) /* 4.20389539e-45, -1: 2.80259693e-45 */
CASE_F_FI(ldexpf, 0x00000003, 0, 0x00000003) /* 4.20389539e-45, 0: 4.20389539e-45 */
CASE_F_FI(ldexpf, 0x00000003, 1, 0x00000006) /* 4.20389539e-45, 1: 8.40779079e-45 */
CASE_F_FI(ldexpf, 0x00000003, 23, 0x01400000) /* 4.20389539e-45, 23: 3.52648305e-38 */
CASE_F_FI(ldexpf, 0x00000003, 127, 0x35400000) /* 4.20389539e-45, 127: 7.15255737e-07 */
CASE_F_FI(ldexpf, 0x00000003, 128, 0x35c00000) /* 4.20389539e-45, 128: 1.43051147e-06 */
CASE_F_FI(ldexpf, 0x00000003, 253, 0x74400000) /* 4.20389539e-45, 253: 6.08472288e+31 */
CASE_F_FI(ldexpf, 0x00000003, 254, 0x74c00000) /* 4.20389539e-45, 254: 1.21694458e+32 */
CASE_F_FI(ldexpf, 0x00000003, 276, 0x7f800000) /* 4.20389539e-45, 276: inf */
CASE_F_FI(ldexpf, 0x00000003, 277, 0x7f800000) /* 4.20389539e-45, 277: inf */
CASE_F_FI(ldexpf, 0x00000003, 278, 0x7f800000) /* 4.20389539e-45, 278: inf */
CASE_F_FI(ldexpf, 0x00000003, 300, 0x7f800000) /* 4.20389539e-45, 300: inf */
CASE_F_FI(ldexpf, 0x00000003, 1000, 0x7f800000) /* 4.20389539e-45, 1000: inf */
CASE_F_FI(ldexpf, 0x00000003, 2147483647, 0x7f800000) /* 4.20389539e-45, 2147483647: inf */
CASE_F_FI(ldexpf, 0x007fffff, -2147483648, 0x00000000) /* 1.17549421e-38, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -1000, 0x00000000) /* 1.17549421e-38, -1000: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -300, 0x00000000) /* 1.17549421e-38, -300: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -278, 0x00000000) /* 1.17549421e-38, -278: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -277, 0x00000000) /* 1.17549421e-38, -277: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -276, 0x00000000) /* 1.17549421e-38, -276: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -254, 0x00000000) /* 1.17549421e-38, -254: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -253, 0x00000000) /* 1.17549421e-38, -253: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -150, 0x00000000) /* 1.17549421e-38, -150: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -149, 0x00000000) /* 1.17549421e-38, -149: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -148, 0x00000000) /* 1.17549421e-38, -148: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -127, 0x00000000) /* 1.17549421e-38, -127: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -126, 0x00000000) /* 1.17549421e-38, -126: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -125, 0x00000000) /* 1.17549421e-38, -125: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -24, 0x00000000) /* 1.17549421e-38, -24: 0 */
CASE_F_FI(ldexpf, 0x007fffff, -1, 0x00400000) /* 1.17549421e-38, -1: 5.87747175e-39 */
CASE_F_FI(ldexpf, 0x007fffff, 0, 0x007fffff) /* 1.17549421e-38, 0: 1.17549421e-38 */
CASE_F_FI(ldexpf, 0x007fffff, 1, 0x00fffffe) /* 1.17549421e-38, 1: 2.35098842e-38 */
CASE_F_FI(ldexpf, 0x007fffff, 23, 0x0bfffffe) /* 1.17549421e-38, 23: 9.86076014e-32 */
CASE_F_FI(ldexpf, 0x007fffff, 127, 0x3ffffffe) /* 1.17549421e-38, 127: 1.99999976 */
CASE_F_FI(ldexpf, 0x007fffff, 128, 0x407ffffe) /* 1.17549421e-38, 128: 3.99999952 */
CASE_F_FI(ldexpf, 0x007fffff, 253, 0x7efffffe) /* 1.17549421e-38, 253: 1.70141163e+38 */
CASE_F_FI(ldexpf, 0x007fffff, 254, 0x7f7ffffe) /* 1.17549421e-38, 254: 3.40282326e+38 */
CASE_F_FI(ldexpf, 0x007fffff, 276, 0x7f800000) /* 1.17549421e-38, 276: inf */
CASE_F_FI(ldexpf, 0x007fffff, 277, 0x7f800000) /* 1.17549421e-38, 277: inf */
CASE_F_FI(ldexpf, 0x007fffff, 278, 0x7f800000) /* 1.17549421e-38, 278: inf */
CASE_F_FI(ldexpf, 0x007fffff, 300, 0x7f800000) /* 1.17549421e-38, 300: inf */
CASE_F_FI(ldexpf, 0x007fffff, 1000, 0x7f800000) /* 1.17549421e-38, 1000: inf */
CASE_F_FI(ldexpf, 0x007fffff, 2147483647, 0x7f800000) /* 1.17549421e-38, 2147483647: inf */
CASE_F_FI(ldexpf, 0x00800000, -2147483648, 0x00000000) /* 1.17549435e-38, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x00800000, -1000, 0x00000000) /* 1.17549435e-38, -1000: 0 */
CASE_F_FI(ldexpf, 0x00800000, -300, 0x00000000) /* 1.17549435e-38, -300: 0 */
CASE_F_FI(ldexpf, 0x00800000, -278, 0x00000000) /* 1.17549435e-38, -278: 0 */
CASE_F_FI(ldexpf, 0x00800000, -277, 0x00000000) /* 1.17549435e-38, -277: 0 */
CASE_F_FI(ldexpf, 0x00800000, -276, 0x00000000) /* 1.17549435e-38, -276: 0 */
CASE_F_FI(ldexpf, 0x00800000, -254, 0x00000000) /* 1.17549435e-38, -254: 0 */
CASE_F_FI(ldexpf, 0x00800000, -253, 0x00000000) /* 1.17549435e-38, -253: 0 */
CASE_F_FI(ldexpf, 0x00800000, -150, 0x00000000) /* 1.17549435e-38, -150: 0 */
CASE_F_FI(ldexpf, 0x00800000, -149, 0x00000000) /* 1.17549435e-38, -149: 0 */
CASE_F_FI(ldexpf, 0x00800000, -148, 0x00000000) /* 1.17549435e-38, -148: 0 */
CASE_F_FI(ldexpf, 0x00800000, -127, 0x00000000) /* 1.17549435e-38, -127: 0 */
CASE_F_FI(ldexpf, 0x00800000, -126, 0x00000000) /* 1.17549435e-38, -126: 0 */
CASE_F_FI(ldexpf, 0x00800000, -125, 0x00000000) /* 1.17549435e-38, -125: 0 */
CASE_F_FI(ldexpf, 0x00800000, -24, 0x00000000) /* 1.17549435e-38, -24: 0 */
CASE_F_FI(ldexpf, 0x00800000, -1, 0x00400000) /* 1.17549435e-38, -1: 5.87747175e-39 */
CASE_F_FI(ldexpf, 0x00800000, 0, 0x00800000) /* 1.17549435e-38, 0: 1.17549435e-38 */
CASE_F_FI(ldexpf, 0x00800000, 1, 0x01000000) /* 1.17549435e-38, 1: 2.3509887e-38 */
CASE_F_FI(ldexpf, 0x00800000, 23, 0x0c000000) /* 1.17549435e-38, 23: 9.86076132e-32 */
CASE_F_FI(ldexpf, 0x00800000, 127, 0x40000000) /* 1.17549435e-38, 127: 2 */
CASE_F_FI(ldexpf, 0x00800000, 128, 0x40800000) /* 1.17549435e-38, 128: 4 */
CASE_F_FI(ldexpf, 0x00800000, 253, 0x7f000000) /* 1.17549435e-38, 253: 1.70141183e+38 */
CASE_F_FI(ldexpf, 0x00800000, 254, 0x7f800000) /* 1.17549435e-38, 254: inf */
CASE_F_FI(ldexpf, 0x00800000, 276, 0x7f800000) /* 1.17549435e-38, 276: inf */
CASE_F_FI(ldexpf, 0x00800000, 277, 0x7f800000) /* 1.17549435e-38, 277: inf */
CASE_F_FI(ldexpf, 0x00800000, 278, 0x7f800000) /* 1.17549435e-38, 278: inf */
CASE_F_FI(ldexpf, 0x00800000, 300, 0x7f800000) /* 1.17549435e-38, 300: inf */
CASE_F_FI(ldexpf, 0x00800000, 1000, 0x7f800000) /* 1.17549435e-38, 1000: inf */
CASE_F_FI(ldexpf, 0x00800000, 2147483647, 0x7f800000) /* 1.17549435e-38, 2147483647: inf */
CASE_F_FI(ldexpf, 0x00c00000, -2147483648, 0x00000000) /* 1.76324153e-38, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -1000, 0x00000000) /* 1.76324153e-38, -1000: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -300, 0x00000000) /* 1.76324153e-38, -300: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -278, 0x00000000) /* 1.76324153e-38, -278: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -277, 0x00000000) /* 1.76324153e-38, -277: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -276, 0x00000000) /* 1.76324153e-38, -276: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -254, 0x00000000) /* 1.76324153e-38, -254: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -253, 0x00000000) /* 1.76324153e-38, -253: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -150, 0x00000000) /* 1.76324153e-38, -150: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -149, 0x00000000) /* 1.76324153e-38, -149: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -148, 0x00000000) /* 1.76324153e-38, -148: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -127, 0x00000000) /* 1.76324153e-38, -127: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -126, 0x00000000) /* 1.76324153e-38, -126: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -125, 0x00000000) /* 1.76324153e-38, -125: 0 */
CASE_F_FI(ldexpf, 0x00c00000, -24, 0x00000001) /* 1.76324153e-38, -24: 1.40129846e-45 */
CASE_F_FI(ldexpf, 0x00c00000, -1, 0x00600000) /* 1.76324153e-38, -1: 8.81620763e-39 */
CASE_F_FI(ldexpf, 0x00c00000, 0, 0x00c00000) /* 1.76324153e-38, 0: 1.76324153e-38 */
CASE_F_FI(ldexpf, 0x00c00000, 1, 0x01400000) /* 1.76324153e-38, 1: 3.52648305e-38 */
CASE_F_FI(ldexpf, 0x00c00000, 23, 0x0c400000) /* 1.76324153e-38, 23: 1.4791142e-31 */
CASE_F_FI(ldexpf, 0x00c00000, 127, 0x40400000) /* 1.76324153e-38, 127: 3 */
CASE_F_FI(ldexpf, 0x00c00000, 128, 0x40c00000) /* 1.76324153e-38, 128: 6 */
CASE_F_FI(ldexpf, 0x00c00000, 253, 0x7f400000) /* 1.76324153e-38, 253: 2.55211775e+38 */
CASE_F_FI(ldexpf, 0x00c00000, 254, 0x7f800000) /* 1.76324153e-38, 254: inf */
CASE_F_FI(ldexpf, 0x00c00000, 276, 0x7f800000) /* 1.76324153e-38, 276: inf */
CASE_F_FI(ldexpf, 0x00c00000, 277, 0x7f800000) /* 1.76324153e-38, 277: inf */
CASE_F_FI(ldexpf, 0x00c00000, 278, 0x7f800000) /* 1.76324153e-38, 278: inf */
CASE_F_FI(ldexpf, 0x00c00000, 300, 0x7f800000) /* 1.76324153e-38, 300: inf */
CASE_F_FI(ldexpf, 0x00c00000, 1000, 0x7f800000) /* 1.76324153e-38, 1000: inf */
CASE_F_FI(ldexpf, 0x00c00000, 2147483647, 0x7f800000) /* 1.76324153e-38, 2147483647: inf */
CASE_F_FI(ldexpf, 0x3f800000, -2147483648, 0x00000000) /* 1, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -1000, 0x00000000) /* 1, -1000: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -300, 0x00000000) /* 1, -300: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -278, 0x00000000) /* 1, -278: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -277, 0x00000000) /* 1, -277: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -276, 0x00000000) /* 1, -276: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -254, 0x00000000) /* 1, -254: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -253, 0x00000000) /* 1, -253: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -150, 0x00000000) /* 1, -150: 0 */
CASE_F_FI(ldexpf, 0x3f800000, -149, 0x00000001) /* 1, -149: 1.40129846e-45 */
CASE_F_FI(ldexpf, 0x3f800000, -148, 0x00000002) /* 1, -148: 2.80259693e-45 */
CASE_F_FI(ldexpf, 0x3f800000, -127, 0x00400000) /* 1, -127: 5.87747175e-39 */
CASE_F_FI(ldexpf, 0x3f800000, -126, 0x00800000) /* 1, -126: 1.17549435e-38 */
CASE_F_FI(ldexpf, 0x3f800000, -125, 0x01000000) /* 1, -125: 2.3509887e-38 */
CASE_F_FI(ldexpf, 0x3f800000, -24, 0x33800000) /* 1, -24: 5.96046448e-08 */
CASE_F_FI(ldexpf, 0x3f800000, -1, 0x3f000000) /* 1, -1: 0.5 */
CASE_F_FI(ldexpf, 0x3f800000, 0, 0x3f800000) /* 1, 0: 1 */
CASE_F_FI(ldexpf, 0x3f800000, 1, 0x40000000) /* 1, 1: 2 */
CASE_F_FI(ldexpf, 0x3f800000, 23, 0x4b000000) /* 1, 23: 8388608 */
CASE_F_FI(ldexpf, 0x3f800000, 127, 0x7f000000) /* 1, 127: 1.70141183e+38 */
CASE_F_FI(ldexpf, 0x3f800000, 128, 0x7f800000) /* 1, 128: inf */
CASE_F_FI(ldexpf, 0x3f800000, 253, 0x7f800000) /* 1, 253: inf */
CASE_F_FI(ldexpf, 0x3f800000, 254, 0x7f800000) /* 1, 254: inf */
CASE_F_FI(ldexpf, 0x3f800000, 276, 0x7f800000) /* 1, 276: inf */
CASE_F_FI(ldexpf, 0x3f800000, 277, 0x7f800000) /* 1, 277: inf */
CASE_F_FI(ldexpf, 0x3f800000, 278, 0x7f800000) /* 1, 278: inf */
CASE_F_FI(ldexpf, 0x3f800000, 300, 0x7f800000) /* 1, 300: inf */
CASE_F_FI(ldexpf, 0x3f800000, 1000, 0x7f800000) /* 1, 1000: inf */
CASE_F_FI(ldexpf, 0x3f800000, 2147483647, 0x7f800000) /* 1, 2147483647: inf */
CASE_F_FI(ldexpf, 0x3f800001, -2147483648, 0x00000000) /* 1.00000012, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -1000, 0x00000000) /* 1.00000012, -1000: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -300, 0x00000000) /* 1.00000012, -300: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -278, 0x00000000) /* 1.00000012, -278: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -277, 0x00000000) /* 1.00000012, -277: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -276, 0x00000000) /* 1.00000012, -276: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -254, 0x00000000) /* 1.00000012, -254: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -253, 0x00000000) /* 1.00000012, -253: 0 */
CASE_F_FI(ldexpf, 0x3f800001, -150, 0x00000001) /* 1.00000012, -150: 1.40129846e-45 */
CASE_F_FI(ldexpf, 0x3f800001, -149, 0x00000001) /* 1.00000012, -149: 1.40129846e-45 */
CASE_F_FI(ldexpf, 0x3f800001, -148, 0x00000002) /* 1.00000012, -148: 2.80259693e-45 */
CASE_F_FI(ldexpf, 0x3f800001, -127, 0x00400000) /* 1.00000012, -127: 5.87747175e-39 */
CASE_F_FI(ldexpf, 0x3f800001, -126, 0x00800001) /* 1.00000012, -126: 1.17549449e-38 */
CASE_F_FI(ldexpf, 0x3f800001, -125, 0x01000001) /* 1.00000012, -125: 2.35098898e-38 */
CASE_F_FI(ldexpf, 0x3f800001, -24, 0x33800001) /* 1.00000012, -24: 5.96046519e-08 */
CASE_F_FI(ldexpf, 0x3f800001, -1, 0x3f000001) /* 1.00000012, -1: 0.50000006 */
CASE_F_FI(ldexpf, 0x3f800001, 0, 0x3f800001) /* 1.00000012, 0: 1.00000012 */
CASE_F_FI(ldexpf, 0x3f800001, 1, 0x40000001) /* 1.00000012, 1: 2.00000024 */
CASE_F_FI(ldexpf, 0x3f800001, 23, 0x4b000001) /* 1.00000012, 23: 8388609 */
CASE_F_FI(ldexpf, 0x3f800001, 127, 0x7f000001) /* 1.00000012, 127: 1.70141204e+38 */
CASE_F_FI(ldexpf, 0x3f800001, 128, 0x7f800000) /* 1.00000012, 128: inf */
CASE_F_FI(ldexpf, 0x3f800001, 253, 0x7f800000) /* 1.00000012, 253: inf */
CASE_F_FI(ldexpf, 0x3f800001, 254, 0x7f800000) /* 1.00000012, 254: inf */
CASE_F_FI(ldexpf, 0x3f800001, 276, 0x7f800000) /* 1.00000012, 276: inf */
CASE_F_FI(ldexpf, 0x3f800001, 277, 0x7f800000) /* 1.00000012, 277: inf */
CASE_F_FI(ldexpf, 0x3f800001, 278, 0x7f800000) /* 1.00000012, 278: inf */
CASE_F_FI(ldexpf, 0x3f800001, 300, 0x7f800000) /* 1.00000012, 300: inf */
CASE_F_FI(ldexpf, 0x3f800001, 1000, 0x7f800000) /* 1.00000012, 1000: inf */
CASE_F_FI(ldexpf, 0x3f800001, 2147483647, 0x7f800000) /* 1.00000012, 2147483647: inf */
CASE_F_FI(ldexpf, 0xbfc00000, -2147483648, 0x80000000) /* -1.5, -2147483648: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -1000, 0x80000000) /* -1.5, -1000: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -300, 0x80000000) /* -1.5, -300: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -278, 0x80000000) /* -1.5, -278: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -277, 0x80000000) /* -1.5, -277: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -276, 0x80000000) /* -1.5, -276: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -254, 0x80000000) /* -1.5, -254: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -253, 0x80000000) /* -1.5, -253: -0 */
CASE_F_FI(ldexpf, 0xbfc00000, -150, 0x80000001) /* -1.5, -150: -1.40129846e-45 */
CASE_F_FI(ldexpf, 0xbfc00000, -149, 0x80000002) /* -1.5, -149: -2.80259693e-45 */
CASE_F_FI(ldexpf, 0xbfc00000, -148, 0x80000003) /* -1.5, -148: -4.20389539e-45 */
CASE_F_FI(ldexpf, 0xbfc00000, -127, 0x80600000) /* -1.5, -127: -8.81620763e-39 */
CASE_F_FI(ldexpf, 0xbfc00000, -126, 0x80c00000) /* -1.5, -126: -1.76324153e-38 */
CASE_F_FI(ldexpf, 0xbfc00000, -125, 0x81400000) /* -1.5, -125: -3.52648305e-38 */
CASE_F_FI(ldexpf, 0xbfc00000, -24, 0xb3c00000) /* -1.5, -24: -8.94069672e-08 */
CASE_F_FI(ldexpf, 0xbfc00000, -1, 0xbf400000) /* -1.5, -1: -0.75 */
CASE_F_FI(ldexpf, 0xbfc00000, 0, 0xbfc00000) /* -1.5, 0: -1.5 */
CASE_F_FI(ldexpf, 0xbfc00000, 1, 0xc0400000) /* -1.5, 1: -3 */
CASE_F_FI(ldexpf, 0xbfc00000, 23, 0xcb400000) /* -1.5, 23: -12582912 */
CASE_F_FI(ldexpf, 0xbfc00000, 127, 0xff400000) /* -1.5, 127: -2.55211775e+38 */
CASE_F_FI(ldexpf, 0xbfc00000, 128, 0xff800000) /* -1.5, 128: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 253, 0xff800000) /* -1.5, 253: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 254, 0xff800000) /* -1.5, 254: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 276, 0xff800000) /* -1.5, 276: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 277, 0xff800000) /* -1.5, 277: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 278, 0xff800000) /* -1.5, 278: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 300, 0xff800000) /* -1.5, 300: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 1000, 0xff800000) /* -1.5, 1000: -inf */
CASE_F_FI(ldexpf, 0xbfc00000, 2147483647, 0xff800000) /* -1.5, 2147483647: -inf */
CASE_F_FI(ldexpf, 0x7f7fffff, -2147483648, 0x00000000) /* 3.40282347e+38, -2147483648: 0 */
CASE_F_FI(ldexpf, 0x7f7fffff, -1000, 0x00000000) /* 3.40282347e+38, -1000: 0 */
CASE_F_FI(ldexpf, 0x7f7fffff, -300, 0x00000000) /* 3.40282347e+38, -300: 0 */
CASE_F_FI(ldexpf, 0x7f7fffff, -278, 0x00000000) /* 3.40282347e+38, -278: 0 */
CASE_F_FI(ldexpf, 0x7f7fffff, -277, 0x00000001) /* 3.40282347e+38, -277: 1.40129846e-45 */
CASE_F_FI(ldexpf, 0x7f7fffff, -276, 0x00000002) /* 3.40282347e+38, -276: 2.80259693e-45 */
CASE_F_FI(ldexpf, 0x7f7fffff, -254, 0x00800000) /* 3.40282347e+38, -254: 1.17549435e-38 */
CASE_F_FI(ldexpf, 0x7f7fffff, -253, 0x00ffffff) /* 3.40282347e+38, -253: 2.35098856e-38 */
CASE_F_FI(ldexpf, 0x7f7fffff, -150, 0x347fffff) /* 3.40282347e+38, -150: 2.38418565e-07 */
CASE_F_FI(ldexpf, 0x7f7fffff, -149, 0x34ffffff) /* 3.40282347e+38, -149: 4.7683713e-07 */
CASE_F_FI(ldexpf, 0x7f7fffff, -148, 0x357fffff) /* 3.40282347e+38, -148: 9.5367426e-07 */
CASE_F_FI(ldexpf, 0x7f7fffff, -127, 0x3fffffff) /* 3.40282347e+38, -127: 1.99999988 */
CASE_F_FI(ldexpf, 0x7f7fffff, -126, 0x407fffff) /* 3.40282347e+38, -126: 3.99999976 */
CASE_F_FI(ldexpf, 0x7f7fffff, -125, 0x40ffffff) /* 3.40282347e+38, -125: 7.99999952 */
CASE_F_FI(ldexpf, 0x7f7fffff, -24, 0x737fffff) /* 3.40282347e+38, -24: 2.02824084e+31 */
CASE_F_FI(ldexpf, 0x7f7fffff, -1, 0x7effffff) /* 3.40282347e+38, -1: 1.70141173e+38 */
CASE_F_FI(ldexpf, 0x7f7fffff, 0, 0x7f7fffff) /* 3.40282347e+38, 0: 3.40282347e+38 */
CASE_F_FI(ldexpf, 0x7f7fffff, 1, 0x7f800000) /* 3.40282347e+38, 1: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 23, 0x7f800000) /* 3.40282347e+38, 23: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 127, 0x7f800000) /* 3.40282347e+38, 127: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 128, 0x7f800000) /* 3.40282347e+38, 128: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 253, 0x7f800000) /* 3.40282347e+38, 253: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 254, 0x7f800000) /* 3.40282347e+38, 254: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 276, 0x7f800000) /* 3.40282347e+38, 276: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 277, 0x7f800000) /* 3.40282347e+38, 277: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 278, 0x7f800000) /* 3.40282347e+38, 278: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 300, 0x7f800000) /* 3.40282347e+38, 300: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 1000, 0x7f800000) /* 3.40282347e+38, 1000: inf */
CASE_F_FI(ldexpf, 0x7f7fffff, 2147483647, 0x7f800000) /* 3.40282347e+38, 2147483647: inf */
CASE_F_FI(ldexpf, 0x7f800000, -2147483648, 0x7f800000) /* inf, -2147483648: inf */
CASE_F_FI(ldexpf, 0x7f800000, -1000, 0x7f800000) /* inf, -1000: inf */
CASE_F_FI(ldexpf, 0x7f800000, -300, 0x7f800000) /* inf, -300: inf */
CASE_F_FI(ldexpf, 0x7f800000, -278, 0x7f800000) /* inf, -278: inf */
CASE_F_FI(ldexpf, 0x7f800000, -277, 0x7f800000) /* inf, -277: inf */
CASE_F_FI(ldexpf, 0x7f800000, -276, 0x7f800000) /* inf, -276: inf */
CASE_F_FI(ldexpf, 0x7f800000, -254, 0x7f800000) /* inf, -254: inf */
CASE_F_FI(ldexpf, 0x7f800000, -253, 0x7f800000) /* inf, -253: inf */
CASE_F_FI(ldexpf, 0x7f800000, -150, 0x7f800000) /* inf, -150: inf */
CASE_F_FI(ldexpf, 0x7f800000, -149, 0x7f800000) /* inf, -149: inf */
CASE_F_FI(ldexpf, 0x7f800000, -148, 0x7f800000) /* inf, -148: inf */
CASE_F_FI(ldexpf, 0x7f800000, -127, 0x7f800000) /* inf, -127: inf */
CASE_F_FI(ldexpf, 0x7f800000, -126, 0x7f800000) /* inf, -126: inf */
CASE_F_FI(ldexpf, 0x7f800000, -125, 0x7f800000) /* inf, -125: inf */
CASE_F_FI(ldexpf, 0x7f800000, -24, 0x7f800000) /* inf, -24: inf */
CASE_F_FI(ldexpf, 0x7f800000, -1, 0x7f800000) /* inf, -1: inf */
CASE_F_FI(ldexpf, 0x7f800000, 0, 0x7f800000) /* inf, 0: inf */
CASE_F_FI(ldexpf, 0x7f800000, 1, 0x7f800000) /* inf, 1: inf */
CASE_F_FI(ldexpf, 0x7f800000, 23, 0x7f800000) /* inf, 23: inf */
CASE_F_FI(ldexpf, 0x7f800000, 127, 0x7f800000) /* inf, 127: inf */
CASE_F_FI(ldexpf, 0x7f800000, 128, 0x7f800000) /* inf, 128: inf */
CASE_F_FI(ldexpf, 0x7f800000, 253, 0x7f800000) /* inf, 253: inf */
CASE_F_FI(ldexpf, 0x7f800000, 254, 0x7f800000) /* inf, 254: inf */
CASE_F_FI(ldexpf, 0x7f800000, 276, 0x7f800000) /* inf, 276: inf */
CASE_F_FI(ldexpf, 0x7f800000, 277, 0x7f800000) /* inf, 277: inf */
CASE_F_FI(ldexpf, 0x7f800000, 278, 0x7f800000) /* inf, 278: inf */
CASE_F_FI(ldexpf, 0x7f800000, 300, 0x7f800000) /* inf, 300: inf */
CASE_F_FI(ldexpf, 0x7f800000, 1000, 0x7f800000) /* inf, 1000: inf */
CASE_F_FI(ldexpf, 0x7f800000, 2147483647, 0x7f800000) /* inf, 2147483647: inf */
CASE_F_FI(ldexpf, 0xff800000, -2147483648, 0xff800000) /* -inf, -2147483648: -inf */
CASE_F_FI(ldexpf, 0xff800000, -1000, 0xff800000) /* -inf, -1000: -inf */
CASE_F_FI(ldexpf, 0xff800000, -300, 0xff800000) /* -inf, -300: -inf */
CASE_F_FI(ldexpf, 0xff800000, -278, 0xff800000) /* -inf, -278: -inf */
CASE_F_FI(ldexpf, 0xff800000, -277, 0xff800000) /* -inf, -277: -inf */
CASE_F_FI(ldexpf, 0xff800000, -276, 0xff800000) /* -inf, -276: -inf */
CASE_F_FI(ldexpf, 0xff800000, -254, 0xff800000) /* -inf, -254: -inf */
CASE_F_FI(ldexpf, 0xff800000, -253, 0xff800000) /* -inf, -253: -inf */
CASE_F_FI(ldexpf, 0xff800000, -150, 0xff800000) /* -inf, -150: -inf */
CASE_F_FI(ldexpf, 0xff800000, -149, 0xff800000) /* -inf, -149: -inf */
CASE_F_FI(ldexpf, 0xff800000, -148, 0xff800000) /* -inf, -148: -inf */
CASE_F_FI(ldexpf, 0xff800000, -127, 0xff800000) /* -inf, -127: -inf */
CASE_F_FI(ldexpf, 0xff800000, -126, 0xff800000) /* -inf, -126: -inf */
CASE_F_FI(ldexpf, 0xff800000, -125, 0xff800000) /* -inf, -125: -inf */
CASE_F_FI(ldexpf, 0xff800000, -24, 0xff800000) /* -inf, -24: -inf */
CASE_F_FI(ldexpf, 0xff800000, -1, 0xff800000) /* -inf, -1: -inf */
CASE_F_FI(ldexpf, 0xff800000, 0, 0xff800000) /* -inf, 0: -inf */
CASE_F_FI(ldexpf, 0xff800000, 1, 0xff800000) /* -inf, 1: -inf */
CASE_F_FI(ldexpf, 0xff800000, 23, 0xff800000) /* -inf, 23: -inf */
CASE_F_FI(ldexpf, 0xff800000, 127, 0xff800000) /* -inf, 127: -inf */
CASE_F_FI(ldexpf, 0xff800000, 128, 0xff800000) /* -inf, 128: -inf */
CASE_F_FI(ldexpf, 0xff800000, 253, 0xff800000) /* -inf, 253: -inf */
CASE_F_FI(ldexpf, 0xff800000, 254, 0xff800000) /* -inf, 254: -inf */
CASE_F_FI(ldexpf, 0xff800000, 276, 0xff800000) /* -inf, 276: -inf */
CASE_F_FI(ldexpf, 0xff800000, 277, 0xff800000) /* -inf, 277: -inf */
CASE_F_FI(ldexpf, 0xff800000, 278, 0xff800000) /* -inf, 278: -inf */
CASE_F_FI(ldexpf, 0xff800000, 300, 0xff800000) /* -inf, 300: -inf */
CASE_F_FI(ldexpf, 0xff800000, 1000, 0xff800000) /* -inf, 1000: -inf */
CASE_F_FI(ldexpf, 0xff800000, 2147483647, 0xff800000) /* -inf, 2147483647: -inf */
CASE_F_FI(ldexpf, 0x7fc00000, -2147483648, 0x7fc00000) /* nan, -2147483648: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -1000, 0x7fc00000) /* nan, -1000: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -300, 0x7fc00000) /* nan, -300: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -278, 0x7fc00000) /* nan, -278: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -277, 0x7fc00000) /* nan, -277: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -276, 0x7fc00000) /* nan, -276: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -254, 0x7fc00000) /* nan, -254: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -253, 0x7fc00000) /* nan, -253: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -150, 0x7fc00000) /* nan, -150: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -149, 0x7fc00000) /* nan, -149: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -148, 0x7fc00000) /* nan, -148: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -127, 0x7fc00000) /* nan, -127: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -126, 0x7fc00000) /* nan, -126: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -125, 0x7fc00000) /* nan, -125: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -24, 0x7fc00000) /* nan, -24: nan */
CASE_F_FI(ldexpf, 0x7fc00000, -1, 0x7fc00000) /* nan, -1: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 0, 0x7fc00000) /* nan, 0: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 1, 0x7fc00000) /* nan, 1: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 23, 0x7fc00000) /* nan, 23: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 127, 0x7fc00000) /* nan, 127: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 128, 0x7fc00000) /* nan, 128: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 253, 0x7fc00000) /* nan, 253: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 254, 0x7fc00000) /* nan, 254: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 276, 0x7fc00000) /* nan, 276: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 277, 0x7fc00000) /* nan, 277: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 278, 0x7fc00000) /* nan, 278: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 300, 0x7fc00000) /* nan, 300: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 1000, 0x7fc00000) /* nan, 1000: nan */
CASE_F_FI(ldexpf, 0x7fc00000, 2147483647, 0x7fc00000) /* nan, 2147483647: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -2147483648, 0x7fc00000) /* nan, -2147483648: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -1000, 0x7fc00000) /* nan, -1000: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -300, 0x7fc00000) /* nan, -300: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -278, 0x7fc00000) /* nan, -278: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -277, 0x7fc00000) /* nan, -277: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -276, 0x7fc00000) /* nan, -276: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -254, 0x7fc00000) /* nan, -254: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -253, 0x7fc00000) /* nan, -253: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -150, 0x7fc00000) /* nan, -150: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -149, 0x7fc00000) /* nan, -149: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -148, 0x7fc00000) /* nan, -148: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -127, 0x7fc00000) /* nan, -127: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -126, 0x7fc00000) /* nan, -126: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -125, 0x7fc00000) /* nan, -125: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -24, 0x7fc00000) /* nan, -24: nan */
CASE_F_FI(ldexpf, 0x7fa00000, -1, 0x7fc00000) /* nan, -1: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 0, 0x7fc00000) /* nan, 0: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 1, 0x7fc00000) /* nan, 1: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 23, 0x7fc00000) /* nan, 23: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 127, 0x7fc00000) /* nan, 127: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 128, 0x7fc00000) /* nan, 128: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 253, 0x7fc00000) /* nan, 253: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 254, 0x7fc00000) /* nan, 254: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 276, 0x7fc00000) /* nan, 276: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 277, 0x7fc00000) /* nan, 277: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 278, 0x7fc00000) /* nan, 278: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 300, 0x7fc00000) /* nan, 300: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 1000, 0x7fc00000) /* nan, 1000: nan */
CASE_F_FI(ldexpf, 0x7fa00000, 2147483647, 0x7fc00000) /* nan, 2147483647: nan */

/* scalbnf: float(float, int), NAN_QUIET */
CASE_F_FI(scalbnf, 0x00000000, -2147483648, 0x00000000) /* 0, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x00000000, -1000, 0x00000000) /* 0, -1000: 0 */
CASE_F_FI(scalbnf, 0x00000000, -300, 0x00000000) /* 0, -300: 0 */
CASE_F_FI(scalbnf, 0x00000000, -278, 0x00000000) /* 0, -278: 0 */
CASE_F_FI(scalbnf, 0x00000000, -277, 0x00000000) /* 0, -277: 0 */
CASE_F_FI(scalbnf, 0x00000000, -276, 0x00000000) /* 0, -276: 0 */
CASE_F_FI(scalbnf, 0x00000000, -254, 0x00000000) /* 0, -254: 0 */
CASE_F_FI(scalbnf, 0x00000000, -253, 0x00000000) /* 0, -253: 0 */
CASE_F_FI(scalbnf, 0x00000000, -150, 0x00000000) /* 0, -150: 0 */
CASE_F_FI(scalbnf, 0x00000000, -149, 0x00000000) /* 0, -149: 0 */
CASE_F_FI(scalbnf, 0x00000000, -148, 0x00000000) /* 0, -148: 0 */
CASE_F_FI(scalbnf, 0x00000000, -127, 0x00000000) /* 0, -127: 0 */
CASE_F_FI(scalbnf, 0x00000000, -126, 0x00000000) /* 0, -126: 0 */
CASE_F_FI(scalbnf, 0x00000000, -125, 0x00000000) /* 0, -125: 0 */
CASE_F_FI(scalbnf, 0x00000000, -24, 0x00000000) /* 0, -24: 0 */
CASE_F_FI(scalbnf, 0x00000000, -1, 0x00000000) /* 0, -1: 0 */
CASE_F_FI(scalbnf, 0x00000000, 0, 0x00000000) /* 0, 0: 0 */
CASE_F_FI(scalbnf, 0x00000000, 1, 0x00000000) /* 0, 1: 0 */
CASE_F_FI(scalbnf, 0x00000000, 23, 0x00000000) /* 0, 23: 0 */
CASE_F_FI(scalbnf, 0x00000000, 127, 0x00000000) /* 0, 127: 0 */
CASE_F_FI(scalbnf, 0x00000000, 128, 0x00000000) /* 0, 128: 0 */
CASE_F_FI(scalbnf, 0x00000000, 253, 0x00000000) /* 0, 253: 0 */
CASE_F_FI(scalbnf, 0x00000000, 254, 0x00000000) /* 0, 254: 0 */
CASE_F_FI(scalbnf, 0x00000000, 276, 0x00000000) /* 0, 276: 0 */
CASE_F_FI(scalbnf, 0x00000000, 277, 0x00000000) /* 0, 277: 0 */
CASE_F_FI(scalbnf, 0x00000000, 278, 0x00000000) /* 0, 278: 0 */
CASE_F_FI(scalbnf, 0x00000000, 300, 0x00000000) /* 0, 300: 0 */
CASE_F_FI(scalbnf, 0x00000000, 1000, 0x00000000) /* 0, 1000: 0 */
CASE_F_FI(scalbnf, 0x00000000, 2147483647, 0x00000000) /* 0, 2147483647: 0 */
CASE_F_FI(scalbnf, 0x80000000, -2147483648, 0x80000000) /* -0, -2147483648: -0 */
CASE_F_FI(scalbnf, 0x80000000, -1000, 0x80000000) /* -0, -1000: -0 */
CASE_F_FI(scalbnf, 0x80000000, -300, 0x80000000) /* -0, -300: -0 */
CASE_F_FI(scalbnf, 0x80000000, -278, 0x80000000) /* -0, -278: -0 */
CASE_F_FI(scalbnf, 0x80000000, -277, 0x80000000) /* -0, -277: -0 */
CASE_F_FI(scalbnf, 0x80000000, -276, 0x80000000) /* -0, -276: -0 */
CASE_F_FI(scalbnf, 0x80000000, -254, 0x80000000) /* -0, -254: -0 */
CASE_F_FI(scalbnf, 0x80000000, -253, 0x80000000) /* -0, -253: -0 */
CASE_F_FI(scalbnf, 0x80000000, -150, 0x80000000) /* -0, -150: -0 */
CASE_F_FI(scalbnf, 0x80000000, -149, 0x80000000) /* -0, -149: -0 */
CASE_F_FI(scalbnf, 0x80000000, -148, 0x80000000) /* -0, -148: -0 */
CASE_F_FI(scalbnf, 0x80000000, -127, 0x80000000) /* -0, -127: -0 */
CASE_F_FI(scalbnf, 0x80000000, -126, 0x80000000) /* -0, -126: -0 */
CASE_F_FI(scalbnf, 0x80000000, -125, 0x80000000) /* -0, -125: -0 */
CASE_F_FI(scalbnf, 0x80000000, -24, 0x80000000) /* -0, -24: -0 */
CASE_F_FI(scalbnf, 0x80000000, -1, 0x80000000) /* -0, -1: -0 */
CASE_F_FI(scalbnf, 0x80000000, 0, 0x80000000) /* -0, 0: -0 */
CASE_F_FI(scalbnf, 0x80000000, 1, 0x80000000) /* -0, 1: -0 */
CASE_F_FI(scalbnf, 0x80000000, 23, 0x80000000) /* -0, 23: -0 */
CASE_F_FI(scalbnf, 0x80000000, 127, 0x80000000) /* -0, 127: -0 */
CASE_F_FI(scalbnf, 0x80000000, 128, 0x80000000) /* -0, 128: -0 */
CASE_F_FI(scalbnf, 0x80000000, 253, 0x80000000) /* -0, 253: -0 */
CASE_F_FI(scalbnf, 0x80000000, 254, 0x80000000) /* -0, 254: -0 */
CASE_F_FI(scalbnf, 0x80000000, 276, 0x80000000) /* -0, 276: -0 */
CASE_F_FI(scalbnf, 0x80000000, 277, 0x80000000) /* -0, 277: -0 */
CASE_F_FI(scalbnf, 0x80000000, 278, 0x80000000) /* -0, 278: -0 */
CASE_F_FI(scalbnf, 0x80000000, 300, 0x80000000) /* -0, 300: -0 */
CASE_F_FI(scalbnf, 0x80000000, 1000, 0x80000000) /* -0, 1000: -0 */
CASE_F_FI(scalbnf, 0x80000000, 2147483647, 0x80000000) /* -0, 2147483647: -0 */
CASE_F_FI(scalbnf, 0x00000001, -2147483648, 0x00000000) /* 1.40129846e-45, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x00000001, -1000, 0x00000000) /* 1.40129846e-45, -1000: 0 */
CASE_F_FI(scalbnf, 0x00000001, -300, 0x00000000) /* 1.40129846e-45, -300: 0 */
CASE_F_FI(scalbnf, 0x00000001, -278, 0x00000000) /* 1.40129846e-45, -278: 0 */
CASE_F_FI(scalbnf, 0x00000001, -277, 0x00000000) /* 1.40129846e-45, -277: 0 */
CASE_F_FI(scalbnf, 0x00000001, -276, 0x00000000) /* 1.40129846e-45, -276: 0 */
CASE_F_FI(scalbnf, 0x00000001, -254, 0x00000000) /* 1.40129846e-45, -254: 0 */
CASE_F_FI(scalbnf, 0x00000001, -253, 0x00000000) /* 1.40129846e-45, -253: 0 */
CASE_F_FI(scalbnf, 0x00000001, -150, 0x00000000) /* 1.40129846e-45, -150: 0 */
CASE_F_FI(scalbnf, 0x00000001, -149, 0x00000000) /* 1.40129846e-45, -149: 0 */
CASE_F_FI(scalbnf, 0x00000001, -148, 0x00000000) /* 1.40129846e-45, -148: 0 */
CASE_F_FI(scalbnf, 0x00000001, -127, 0x00000000) /* 1.40129846e-45, -127: 0 */
CASE_F_FI(scalbnf, 0x00000001, -126, 0x00000000) /* 1.40129846e-45, -126: 0 */
CASE_F_FI(scalbnf, 0x00000001, -125, 0x00000000) /* 1.40129846e-45, -125: 0 */
CASE_F_FI(scalbnf, 0x00000001, -24, 0x00000000) /* 1.40129846e-45, -24: 0 */
CASE_F_FI(scalbnf, 0x00000001, -1, 0x00000000) /* 1.40129846e-45, -1: 0 */
CASE_F_FI(scalbnf, 0x00000001, 0, 0x00000001) /* 1.40129846e-45, 0: 1.40129846e-45 */
CASE_F_FI(scalbnf, 0x00000001, 1, 0x00000002) /* 1.40129846e-45, 1: 2.80259693e-45 */
CASE_F_FI(scalbnf, 0x00000001, 23, 0x00800000) /* 1.40129846e-45, 23: 1.17549435e-38 */
CASE_F_FI(scalbnf, 0x00000001, 127, 0x34800000) /* 1.40129846e-45, 127: 2.38418579e-07 */
CASE_F_FI(scalbnf, 0x00000001, 128, 0x35000000) /* 1.40129846e-45, 128: 4.76837158e-07 */
CASE_F_FI(scalbnf, 0x00000001, 253, 0x73800000) /* 1.40129846e-45, 253: 2.02824096e+31 */
CASE_F_FI(scalbnf, 0x00000001, 254, 0x74000000) /* 1.40129846e-45, 254: 4.05648192e+31 */
CASE_F_FI(scalbnf, 0x00000001, 276, 0x7f000000) /* 1.40129846e-45, 276: 1.70141183e+38 */
CASE_F_FI(scalbnf, 0x00000001, 277, 0x7f800000) /* 1.40129846e-45, 277: inf */
CASE_F_FI(scalbnf, 0x00000001, 278, 0x7f800000) /* 1.40129846e-45, 278: inf */
CASE_F_FI(scalbnf, 0x00000001, 300, 0x7f800000) /* 1.40129846e-45, 300: inf */
CASE_F_FI(scalbnf, 0x00000001, 1000, 0x7f800000) /* 1.40129846e-45, 1000: inf */
CASE_F_FI(scalbnf, 0x00000001, 2147483647, 0x7f800000) /* 1.40129846e-45, 2147483647: inf */
CASE_F_FI(scalbnf, 0x00000003, -2147483648, 0x00000000) /* 4.20389539e-45, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x00000003, -1000, 0x00000000) /* 4.20389539e-45, -1000: 0 */
CASE_F_FI(scalbnf, 0x00000003, -300, 0x00000000) /* 4.20389539e-45, -300: 0 */
CASE_F_FI(scalbnf, 0x00000003, -278, 0x00000000) /* 4.20389539e-45, -278: 0 */
CASE_F_FI(scalbnf, 0x00000003, -277, 0x00000000) /* 4.20389539e-45, -277: 0 */
CASE_F_FI(scalbnf, 0x00000003, -276, 0x00000000) /* 4.20389539e-45, -276: 0 */
CASE_F_FI(scalbnf, 0x00000003, -254, 0x00000000) /* 4.20389539e-45, -254: 0 */
CASE_F_FI(scalbnf, 0x00000003, -253, 0x00000000) /* 4.20389539e-45, -253: 0 */
CASE_F_FI(scalbnf, 0x00000003, -150, 0x00000000) /* 4.20389539e-45, -150: 0 */
CASE_F_FI(scalbnf, 0x00000003, -149, 0x00000000) /* 4.20389539e-45, -149: 0 */
CASE_F_FI(scalbnf, 0x00000003, -148, 0x00000000) /* 4.20389539e-45, -148: 0 */
CASE_F_FI(scalbnf, 0x00000003, -127, 0x00000000) /* 4.20389539e-45, -127: 0 */
CASE_F_FI(scalbnf, 0x00000003, -126, 0x00000000) /* 4.20389539e-45, -126: 0 */
CASE_F_FI(scalbnf, 0x00000003, -125, 0x00000000) /* 4.20389539e-45, -125: 0 */
CASE_F_FI(scalbnf, 0x00000003, -24, 0x00000000) /* 4.20389539e-45, -24: 0 */
CASE_F_FI(scalbnf, 0x00000003, -1, 0x00000002) /* 4.20389539e-45, -1: 2.80259693e-45 */
CASE_F_FI(scalbnf, 0x00000003, 0, 0x00000003) /* 4.20389539e-45, 0: 4.20389539e-45 */
CASE_F_FI(scalbnf, 0x00000003, 1, 0x00000006) /* 4.20389539e-45, 1: 8.40779079e-45 */
CASE_F_FI(scalbnf, 0x00000003, 23, 0x01400000) /* 4.20389539e-45, 23: 3.52648305e-38 */
CASE_F_FI(scalbnf, 0x00000003, 127, 0x35400000) /* 4.20389539e-45, 127: 7.15255737e-07 */
CASE_F_FI(scalbnf, 0x00000003, 128, 0x35c00000) /* 4.20389539e-45, 128: 1.43051147e-06 */
CASE_F_FI(scalbnf, 0x00000003, 253, 0x74400000) /* 4.20389539e-45, 253: 6.08472288e+31 */
CASE_F_FI(scalbnf, 0x00000003, 254, 0x74c00000) /* 4.20389539e-45, 254: 1.21694458e+32 */
CASE_F_FI(scalbnf, 0x00000003, 276, 0x7f800000) /* 4.20389539e-45, 276: inf */
CASE_F_FI(scalbnf, 0x00000003, 277, 0x7f800000) /* 4.20389539e-45, 277: inf */
CASE_F_FI(scalbnf, 0x00000003, 278, 0x7f800000) /* 4.20389539e-45, 278: inf */
CASE_F_FI(scalbnf, 0x00000003, 300, 0x7f800000) /* 4.20389539e-45, 300: inf */
CASE_F_FI(scalbnf, 0x00000003, 1000, 0x7f800000) /* 4.20389539e-45, 1000: inf */
CASE_F_FI(scalbnf, 0x00000003, 2147483647, 0x7f800000) /* 4.20389539e-45, 2147483647: inf */
CASE_F_FI(scalbnf, 0x007fffff, -2147483648, 0x00000000) /* 1.17549421e-38, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -1000, 0x00000000) /* 1.17549421e-38, -1000: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -300, 0x00000000) /* 1.17549421e-38, -300: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -278, 0x00000000) /* 1.17549421e-38, -278: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -277, 0x00000000) /* 1.17549421e-38, -277: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -276, 0x00000000) /* 1.17549421e-38, -276: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -254, 0x00000000) /* 1.17549421e-38, -254: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -253, 0x00000000) /* 1.17549421e-38, -253: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -150, 0x00000000) /* 1.17549421e-38, -150: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -149, 0x00000000) /* 1.17549421e-38, -149: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -148, 0x00000000) /* 1.17549421e-38, -148: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -127, 0x00000000) /* 1.17549421e-38, -127: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -126, 0x00000000) /* 1.17549421e-38, -126: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -125, 0x00000000) /* 1.17549421e-38, -125: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -24, 0x00000000) /* 1.17549421e-38, -24: 0 */
CASE_F_FI(scalbnf, 0x007fffff, -1, 0x00400000) /* 1.17549421e-38, -1: 5.87747175e-39 */
CASE_F_FI(scalbnf, 0x007fffff, 0, 0x007fffff) /* 1.17549421e-38, 0: 1.17549421e-38 */
CASE_F_FI(scalbnf, 0x007fffff, 1, 0x00fffffe) /* 1.17549421e-38, 1: 2.35098842e-38 */
CASE_F_FI(scalbnf, 0x007fffff, 23, 0x0bfffffe) /* 1.17549421e-38, 23: 9.86076014e-32 */
CASE_F_FI(scalbnf, 0x007fffff, 127, 0x3ffffffe) /* 1.17549421e-38, 127: 1.99999976 */
CASE_F_FI(scalbnf, 0x007fffff, 128, 0x407ffffe) /* 1.17549421e-38, 128: 3.99999952 */
CASE_F_FI(scalbnf, 0x007fffff, 253, 0x7efffffe) /* 1.17549421e-38, 253: 1.70141163e+38 */
CASE_F_FI(scalbnf, 0x007fffff, 254, 0x7f7ffffe) /* 1.17549421e-38, 254: 3.40282326e+38 */
CASE_F_FI(scalbnf, 0x007fffff, 276, 0x7f800000) /* 1.17549421e-38, 276: inf */
CASE_F_FI(scalbnf, 0x007fffff, 277, 0x7f800000) /* 1.17549421e-38, 277: inf */
CASE_F_FI(scalbnf, 0x007fffff, 278, 0x7f800000) /* 1.17549421e-38, 278: inf */
CASE_F_FI(scalbnf, 0x007fffff, 300, 0x7f800000) /* 1.17549421e-38, 300: inf */
CASE_F_FI(scalbnf, 0x007fffff, 1000, 0x7f800000) /* 1.17549421e-38, 1000: inf */
CASE_F_FI(scalbnf, 0x007fffff, 2147483647, 0x7f800000) /* 1.17549421e-38, 2147483647: inf */
CASE_F_FI(scalbnf, 0x00800000, -2147483648, 0x00000000) /* 1.17549435e-38, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x00800000, -1000, 0x00000000) /* 1.17549435e-38, -1000: 0 */
CASE_F_FI(scalbnf, 0x00800000, -300, 0x00000000) /* 1.17549435e-38, -300: 0 */
CASE_F_FI(scalbnf, 0x00800000, -278, 0x00000000) /* 1.17549435e-38, -278: 0 */
CASE_F_FI(scalbnf, 0x00800000, -277, 0x00000000) /* 1.17549435e-38, -277: 0 */
CASE_F_FI(scalbnf, 0x00800000, -276, 0x00000000) /* 1.17549435e-38, -276: 0 */
CASE_F_FI(scalbnf, 0x00800000, -254, 0x00000000) /* 1.17549435e-38, -254: 0 */
CASE_F_FI(scalbnf, 0x00800000, -253, 0x00000000) /* 1.17549435e-38, -253: 0 */
CASE_F_FI(scalbnf, 0x00800000, -150, 0x00000000) /* 1.17549435e-38, -150: 0 */
CASE_F_FI(scalbnf, 0x00800000, -149, 0x00000000) /* 1.17549435e-38, -149: 0 */
CASE_F_FI(scalbnf, 0x00800000, -148, 0x00000000) /* 1.17549435e-38, -148: 0 */
CASE_F_FI(scalbnf, 0x00800000, -127, 0x00000000) /* 1.17549435e-38, -127: 0 */
CASE_F_FI(scalbnf, 0x00800000, -126, 0x00000000) /* 1.17549435e-38, -126: 0 */
CASE_F_FI(scalbnf, 0x00800000, -125, 0x00000000) /* 1.17549435e-38, -125: 0 */
CASE_F_FI(scalbnf, 0x00800000, -24, 0x00000000) /* 1.17549435e-38, -24: 0 */
CASE_F_FI(scalbnf, 0x00800000, -1, 0x00400000) /* 1.17549435e-38, -1: 5.87747175e-39 */
CASE_F_FI(scalbnf, 0x00800000, 0, 0x00800000) /* 1.17549435e-38, 0: 1.17549435e-38 */
CASE_F_FI(scalbnf, 0x00800000, 1, 0x01000000) /* 1.17549435e-38, 1: 2.3509887e-38 */
CASE_F_FI(scalbnf, 0x00800000, 23, 0x0c000000) /* 1.17549435e-38, 23: 9.86076132e-32 */
CASE_F_FI(scalbnf, 0x00800000, 127, 0x40000000) /* 1.17549435e-38, 127: 2 */
CASE_F_FI(scalbnf, 0x00800000, 128, 0x40800000) /* 1.17549435e-38, 128: 4 */
CASE_F_FI(scalbnf, 0x00800000, 253, 0x7f000000) /* 1.17549435e-38, 253: 1.70141183e+38 */
CASE_F_FI(scalbnf, 0x00800000, 254, 0x7f800000) /* 1.17549435e-38, 254: inf */
CASE_F_FI(scalbnf, 0x00800000, 276, 0x7f800000) /* 1.17549435e-38, 276: inf */
CASE_F_FI(scalbnf, 0x00800000, 277, 0x7f800000) /* 1.17549435e-38, 277: inf */
CASE_F_FI(scalbnf, 0x00800000, 278, 0x7f800000) /* 1.17549435e-38, 278: inf */
CASE_F_FI(scalbnf, 0x00800000, 300, 0x7f800000) /* 1.17549435e-38, 300: inf */
CASE_F_FI(scalbnf, 0x00800000, 1000, 0x7f800000) /* 1.17549435e-38, 1000: inf */
CASE_F_FI(scalbnf, 0x00800000, 2147483647, 0x7f800000) /* 1.17549435e-38, 2147483647: inf */
CASE_F_FI(scalbnf, 0x00c00000, -2147483648, 0x00000000) /* 1.76324153e-38, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -1000, 0x00000000) /* 1.76324153e-38, -1000: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -300, 0x00000000) /* 1.76324153e-38, -300: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -278, 0x00000000) /* 1.76324153e-38, -278: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -277, 0x00000000) /* 1.76324153e-38, -277: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -276, 0x00000000) /* 1.76324153e-38, -276: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -254, 0x00000000) /* 1.76324153e-38, -254: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -253, 0x00000000) /* 1.76324153e-38, -253: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -150, 0x00000000) /* 1.76324153e-38, -150: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -149, 0x00000000) /* 1.76324153e-38, -149: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -148, 0x00000000) /* 1.76324153e-38, -148: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -127, 0x00000000) /* 1.76324153e-38, -127: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -126, 0x00000000) /* 1.76324153e-38, -126: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -125, 0x00000000) /* 1.76324153e-38, -125: 0 */
CASE_F_FI(scalbnf, 0x00c00000, -24, 0x00000001) /* 1.76324153e-38, -24: 1.40129846e-45 */
CASE_F_FI(scalbnf, 0x00c00000, -1, 0x00600000) /* 1.76324153e-38, -1: 8.81620763e-39 */
CASE_F_FI(scalbnf, 0x00c00000, 0, 0x00c00000) /* 1.76324153e-38, 0: 1.76324153e-38 */
CASE_F_FI(scalbnf, 0x00c00000, 1, 0x01400000) /* 1.76324153e-38, 1: 3.52648305e-38 */
CASE_F_FI(scalbnf, 0x00c00000, 23, 0x0c400000) /* 1.76324153e-38, 23: 1.4791142e-31 */
CASE_F_FI(scalbnf, 0x00c00000, 127, 0x40400000) /* 1.76324153e-38, 127: 3 */
CASE_F_FI(scalbnf, 0x00c00000, 128, 0x40c00000) /* 1.76324153e-38, 128: 6 */
CASE_F_FI(scalbnf, 0x00c00000, 253, 0x7f400000) /* 1.76324153e-38, 253: 2.55211775e+38 */
CASE_F_FI(scalbnf, 0x00c00000, 254, 0x7f800000) /* 1.76324153e-38, 254: inf */
CASE_F_FI(scalbnf, 0x00c00000, 276, 0x7f800000) /* 1.76324153e-38, 276: inf */
CASE_F_FI(scalbnf, 0x00c00000, 277, 0x7f800000) /* 1.76324153e-38, 277: inf */
CASE_F_FI(scalbnf, 0x00c00000, 278, 0x7f800000) /* 1.76324153e-38, 278: inf */
CASE_F_FI(scalbnf, 0x00c00000, 300, 0x7f800000) /* 1.76324153e-38, 300: inf */
CASE_F_FI(scalbnf, 0x00c00000, 1000, 0x7f800000) /* 1.76324153e-38, 1000: inf */
CASE_F_FI(scalbnf, 0x00c00000, 2147483647, 0x7f800000) /* 1.76324153e-38, 2147483647: inf */
CASE_F_FI(scalbnf, 0x3f800000, -2147483648, 0x00000000) /* 1, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -1000, 0x00000000) /* 1, -1000: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -300, 0x00000000) /* 1, -300: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -278, 0x00000000) /* 1, -278: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -277, 0x00000000) /* 1, -277: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -276, 0x00000000) /* 1, -276: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -254, 0x00000000) /* 1, -254: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -253, 0x00000000) /* 1, -253: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -150, 0x00000000) /* 1, -150: 0 */
CASE_F_FI(scalbnf, 0x3f800000, -149, 0x00000001) /* 1, -149: 1.40129846e-45 */
CASE_F_FI(scalbnf, 0x3f800000, -148, 0x00000002) /* 1, -148: 2.80259693e-45 */
CASE_F_FI(scalbnf, 0x3f800000, -127, 0x00400000) /* 1, -127: 5.87747175e-39 */
CASE_F_FI(scalbnf, 0x3f800000, -126, 0x00800000) /* 1, -126: 1.17549435e-38 */
CASE_F_FI(scalbnf, 0x3f800000, -125, 0x01000000) /* 1, -125: 2.3509887e-38 */
CASE_F_FI(scalbnf, 0x3f800000, -24, 0x33800000) /* 1, -24: 5.96046448e-08 */
CASE_F_FI(scalbnf, 0x3f800000, -1, 0x3f000000) /* 1, -1: 0.5 */
CASE_F_FI(scalbnf, 0x3f800000, 0, 0x3f800000) /* 1, 0: 1 */
CASE_F_FI(scalbnf, 0x3f800000, 1, 0x40000000) /* 1, 1: 2 */
CASE_F_FI(scalbnf, 0x3f800000, 23, 0x4b000000) /* 1, 23: 8388608 */
CASE_F_FI(scalbnf, 0x3f800000, 127, 0x7f000000) /* 1, 127: 1.70141183e+38 */
CASE_F_FI(scalbnf, 0x3f800000, 128, 0x7f800000) /* 1, 128: inf */
CASE_F_FI(scalbnf, 0x3f800000, 253, 0x7f800000) /* 1, 253: inf */
CASE_F_FI(scalbnf, 0x3f800000, 254, 0x7f800000) /* 1, 254: inf */
CASE_F_FI(scalbnf, 0x3f800000, 276, 0x7f800000) /* 1, 276: inf */
CASE_F_FI(scalbnf, 0x3f800000, 277, 0x7f800000) /* 1, 277: inf */
CASE_F_FI(scalbnf, 0x3f800000, 278, 0x7f800000) /* 1, 278: inf */
CASE_F_FI(scalbnf, 0x3f800000, 300, 0x7f800000) /* 1, 300: inf */
CASE_F_FI(scalbnf, 0x3f800000, 1000, 0x7f800000) /* 1, 1000: inf */
CASE_F_FI(scalbnf, 0x3f800000, 2147483647, 0x7f800000) /* 1, 2147483647: inf */
CASE_F_FI(scalbnf, 0x3f800001, -2147483648, 0x00000000) /* 1.00000012, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -1000, 0x00000000) /* 1.00000012, -1000: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -300, 0x00000000) /* 1.00000012, -300: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -278, 0x00000000) /* 1.00000012, -278: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -277, 0x00000000) /* 1.00000012, -277: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -276, 0x00000000) /* 1.00000012, -276: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -254, 0x00000000) /* 1.00000012, -254: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -253, 0x00000000) /* 1.00000012, -253: 0 */
CASE_F_FI(scalbnf, 0x3f800001, -150, 0x00000001) /* 1.00000012, -150: 1.40129846e-45 */
CASE_F_FI(scalbnf, 0x3f800001, -149, 0x00000001) /* 1.00000012, -149: 1.40129846e-45 */
CASE_F_FI(scalbnf, 0x3f800001, -148, 0x00000002) /* 1.00000012, -148: 2.80259693e-45 */
CASE_F_FI(scalbnf, 0x3f800001, -127, 0x00400000) /* 1.00000012, -127: 5.87747175e-39 */
CASE_F_FI(scalbnf, 0x3f800001, -126, 0x00800001) /* 1.00000012, -126: 1.17549449e-38 */
CASE_F_FI(scalbnf, 0x3f800001, -125, 0x01000001) /* 1.00000012, -125: 2.35098898e-38 */
CASE_F_FI(scalbnf, 0x3f800001, -24, 0x33800001) /* 1.00000012, -24: 5.96046519e-08 */
CASE_F_FI(scalbnf, 0x3f800001, -1, 0x3f000001) /* 1.00000012, -1: 0.50000006 */
CASE_F_FI(scalbnf, 0x3f800001, 0, 0x3f800001) /* 1.00000012, 0: 1.00000012 */
CASE_F_FI(scalbnf, 0x3f800001, 1, 0x40000001) /* 1.00000012, 1: 2.00000024 */
CASE_F_FI(scalbnf, 0x3f800001, 23, 0x4b000001) /* 1.00000012, 23: 8388609 */
CASE_F_FI(scalbnf, 0x3f800001, 127, 0x7f000001) /* 1.00000012, 127: 1.70141204e+38 */
CASE_F_FI(scalbnf, 0x3f800001, 128, 0x7f800000) /* 1.00000012, 128: inf */
CASE_F_FI(scalbnf, 0x3f800001, 253, 0x7f800000) /* 1.00000012, 253: inf */
CASE_F_FI(scalbnf, 0x3f800001, 254, 0x7f800000) /* 1.00000012, 254: inf */
CASE_F_FI(scalbnf, 0x3f800001, 276, 0x7f800000) /* 1.00000012, 276: inf */
CASE_F_FI(scalbnf, 0x3f800001, 277, 0x7f800000) /* 1.00000012, 277: inf */
CASE_F_FI(scalbnf, 0x3f800001, 278, 0x7f800000) /* 1.00000012, 278: inf */
CASE_F_FI(scalbnf, 0x3f800001, 300, 0x7f800000) /* 1.00000012, 300: inf */
CASE_F_FI(scalbnf, 0x3f800001, 1000, 0x7f800000) /* 1.00000012, 1000: inf */
CASE_F_FI(scalbnf, 0x3f800001, 2147483647, 0x7f800000) /* 1.00000012, 2147483647: inf */
CASE_F_FI(scalbnf, 0xbfc00000, -2147483648, 0x80000000) /* -1.5, -2147483648: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -1000, 0x80000000) /* -1.5, -1000: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -300, 0x80000000) /* -1.5, -300: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -278, 0x80000000) /* -1.5, -278: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -277, 0x80000000) /* -1.5, -277: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -276, 0x80000000) /* -1.5, -276: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -254, 0x80000000) /* -1.5, -254: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -253, 0x80000000) /* -1.5, -253: -0 */
CASE_F_FI(scalbnf, 0xbfc00000, -150, 0x80000001) /* -1.5, -150: -1.40129846e-45 */
CASE_F_FI(scalbnf, 0xbfc00000, -149, 0x80000002) /* -1.5, -149: -2.80259693e-45 */
CASE_F_FI(scalbnf, 0xbfc00000, -148, 0x80000003) /* -1.5, -148: -4.20389539e-45 */
CASE_F_FI(scalbnf, 0xbfc00000, -127, 0x80600000) /* -1.5, -127: -8.81620763e-39 */
CASE_F_FI(scalbnf, 0xbfc00000, -126, 0x80c00000) /* -1.5, -126: -1.76324153e-38 */
CASE_F_FI(scalbnf, 0xbfc00000, -125, 0x81400000) /* -1.5, -125: -3.52648305e-38 */
CASE_F_FI(scalbnf, 0xbfc00000, -24, 0xb3c00000) /* -1.5, -24: -8.94069672e-08 */
CASE_F_FI(scalbnf, 0xbfc00000, -1, 0xbf400000) /* -1.5, -1: -0.75 */
CASE_F_FI(scalbnf, 0xbfc00000, 0, 0xbfc00000) /* -1.5, 0: -1.5 */
CASE_F_FI(scalbnf, 0xbfc00000, 1, 0xc0400000) /* -1.5, 1: -3 */
CASE_F_FI(scalbnf, 0xbfc00000, 23, 0xcb400000) /* -1.5, 23: -12582912 */
CASE_F_FI(scalbnf, 0xbfc00000, 127, 0xff400000) /* -1.5, 127: -2.55211775e+38 */
CASE_F_FI(scalbnf, 0xbfc00000, 128, 0xff800000) /* -1.5, 128: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 253, 0xff800000) /* -1.5, 253: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 254, 0xff800000) /* -1.5, 254: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 276, 0xff800000) /* -1.5, 276: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 277, 0xff800000) /* -1.5, 277: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 278, 0xff800000) /* -1.5, 278: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 300, 0xff800000) /* -1.5, 300: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 1000, 0xff800000) /* -1.5, 1000: -inf */
CASE_F_FI(scalbnf, 0xbfc00000, 2147483647, 0xff800000) /* -1.5, 2147483647: -inf */
CASE_F_FI(scalbnf, 0x7f7fffff, -2147483648, 0x00000000) /* 3.40282347e+38, -2147483648: 0 */
CASE_F_FI(scalbnf, 0x7f7fffff, -1000, 0x00000000) /* 3.40282347e+38, -1000: 0 */
CASE_F_FI(scalbnf, 0x7f7fffff, -300, 0x00000000) /* 3.40282347e+38, -300: 0 */
CASE_F_FI(scalbnf, 0x7f7fffff, -278, 0x00000000) /* 3.40282347e+38, -278: 0 */
CASE_F_FI(scalbnf, 0x7f7fffff, -277, 0x00000001) /* 3.40282347e+38, -277: 1.40129846e-45 */
CASE_F_FI(scalbnf, 0x7f7fffff, -276, 0x00000002) /* 3.40282347e+38, -276: 2.80259693e-45 */
CASE_F_FI(scalbnf, 0x7f7fffff, -254, 0x00800000) /* 3.40282347e+38, -254: 1.17549435e-38 */
CASE_F_FI(scalbnf, 0x7f7fffff, -253, 0x00ffffff) /* 3.40282347e+38, -253: 2.35098856e-38 */
CASE_F_FI(scalbnf, 0x7f7fffff, -150, 0x347fffff) /* 3.40282347e+38, -150: 2.38418565e-07 */
CASE_F_FI(scalbnf, 0x7f7fffff, -149, 0x34ffffff) /* 3.40282347e+38, -149: 4.7683713e-07 */
CASE_F_FI(scalbnf, 0x7f7fffff, -148, 0x357fffff) /* 3.40282347e+38, -148: 9.5367426e-07 */
CASE_F_FI(scalbnf, 0x7f7fffff, -127, 0x3fffffff) /* 3.40282347e+38, -127: 1.99999988 */
CASE_F_FI(scalbnf, 0x7f7fffff, -126, 0x407fffff) /* 3.40282347e+38, -126: 3.99999976 */
CASE_F_FI(scalbnf, 0x7f7fffff, -125, 0x40ffffff) /* 3.40282347e+38, -125: 7.99999952 */
CASE_F_FI(scalbnf, 0x7f7fffff, -24, 0x737fffff) /* 3.40282347e+38, -24: 2.02824084e+31 */
CASE_F_FI(scalbnf, 0x7f7fffff, -1, 0x7effffff) /* 3.40282347e+38, -1: 1.70141173e+38 */
CASE_F_FI(scalbnf, 0x7f7fffff, 0, 0x7f7fffff) /* 3.40282347e+38, 0: 3.40282347e+38 */
CASE_F_FI(scalbnf, 0x7f7fffff, 1, 0x7f800000) /* 3.40282347e+38, 1: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 23, 0x7f800000) /* 3.40282347e+38, 23: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 127, 0x7f800000) /* 3.40282347e+38, 127: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 128, 0x7f800000) /* 3.40282347e+38, 128: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 253, 0x7f800000) /* 3.40282347e+38, 253: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 254, 0x7f800000) /* 3.40282347e+38, 254: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 276, 0x7f800000) /* 3.40282347e+38, 276: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 277, 0x7f800000) /* 3.40282347e+38, 277: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 278, 0x7f800000) /* 3.40282347e+38, 278: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 300, 0x7f800000) /* 3.40282347e+38, 300: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 1000, 0x7f800000) /* 3.40282347e+38, 1000: inf */
CASE_F_FI(scalbnf, 0x7f7fffff, 2147483647, 0x7f800000) /* 3.40282347e+38, 2147483647: inf */
CASE_F_FI(scalbnf, 0x7f800000, -2147483648, 0x7f800000) /* inf, -2147483648: inf */
CASE_F_FI(scalbnf, 0x7f800000, -1000, 0x7f800000) /* inf, -1000: inf */
CASE_F_FI(scalbnf, 0x7f800000, -300, 0x7f800000) /* inf, -300: inf */
CASE_F_FI(scalbnf, 0x7f800000, -278, 0x7f800000) /* inf, -278: inf */
CASE_F_FI(scalbnf, 0x7f800000, -277, 0x7f800000) /* inf, -277: inf */
CASE_F_FI(scalbnf, 0x7f800000, -276, 0x7f800000) /* inf, -276: inf */
CASE_F_FI(scalbnf, 0x7f800000, -254, 0x7f800000) /* inf, -254: inf */
CASE_F_FI(scalbnf, 0x7f800000, -253, 0x7f800000) /* inf, -253: inf */
CASE_F_FI(scalbnf, 0x7f800000, -150, 0x7f800000) /* inf, -150: inf */
CASE_F_FI(scalbnf, 0x7f800000, -149, 0x7f800000) /* inf, -149: inf */
CASE_F_FI(scalbnf, 0x7f800000, -148, 0x7f800000) /* inf, -148: inf */
CASE_F_FI(scalbnf, 0x7f800000, -127, 0x7f800000) /* inf, -127: inf */
CASE_F_FI(scalbnf, 0x7f800000, -126, 0x7f800000) /* inf, -126: inf */
CASE_F_FI(scalbnf, 0x7f800000, -125, 0x7f800000) /* inf, -125: inf */
CASE_F_FI(scalbnf, 0x7f800000, -24, 0x7f800000) /* inf, -24: inf */
CASE_F_FI(scalbnf, 0x7f800000, -1, 0x7f800000) /* inf, -1: inf */
CASE_F_FI(scalbnf, 0x7f800000, 0, 0x7f800000) /* inf, 0: inf */
CASE_F_FI(scalbnf, 0x7f800000, 1, 0x7f800000) /* inf, 1: inf */
CASE_F_FI(scalbnf, 0x7f800000, 23, 0x7f800000) /* inf, 23: inf */
CASE_F_FI(scalbnf, 0x7f800000, 127, 0x7f800000) /* inf, 127: inf */
CASE_F_FI(scalbnf, 0x7f800000, 128, 0x7f800000) /* inf, 128: inf */
CASE_F_FI(scalbnf, 0x7f800000, 253, 0x7f800000) /* inf, 253: inf */
CASE_F_FI(scalbnf, 0x7f800000, 254, 0x7f800000) /* inf, 254: inf */
CASE_F_FI(scalbnf, 0x7f800000, 276, 0x7f800000) /* inf, 276: inf */
CASE_F_FI(scalbnf, 0x7f800000, 277, 0x7f800000) /* inf, 277: inf */
CASE_F_FI(scalbnf, 0x7f800000, 278, 0x7f800000) /* inf, 278: inf */
CASE_F_FI(scalbnf, 0x7f800000, 300, 0x7f800000) /* inf, 300: inf */
CASE_F_FI(scalbnf, 0x7f800000, 1000, 0x7f800000) /* inf, 1000: inf */
CASE_F_FI(scalbnf, 0x7f800000, 2147483647, 0x7f800000) /* inf, 2147483647: inf */
CASE_F_FI(scalbnf, 0xff800000, -2147483648, 0xff800000) /* -inf, -2147483648: -inf */
CASE_F_FI(scalbnf, 0xff800000, -1000, 0xff800000) /* -inf, -1000: -inf */
CASE_F_FI(scalbnf, 0xff800000, -300, 0xff800000) /* -inf, -300: -inf */
CASE_F_FI(scalbnf, 0xff800000, -278, 0xff800000) /* -inf, -278: -inf */
CASE_F_FI(scalbnf, 0xff800000, -277, 0xff800000) /* -inf, -277: -inf */
CASE_F_FI(scalbnf, 0xff800000, -276, 0xff800000) /* -inf, -276: -inf */
CASE_F_FI(scalbnf, 0xff800000, -254, 0xff800000) /* -inf, -254: -inf */
CASE_F_FI(scalbnf, 0xff800000, -253, 0xff800000) /* -inf, -253: -inf */
CASE_F_FI(scalbnf, 0xff800000, -150, 0xff800000) /* -inf, -150: -inf */
CASE_F_FI(scalbnf, 0xff800000, -149, 0xff800000) /* -inf, -149: -inf */
CASE_F_FI(scalbnf, 0xff800000, -148, 0xff800000) /* -inf, -148: -inf */
CASE_F_FI(scalbnf, 0xff800000, -127, 0xff800000) /* -inf, -127: -inf */
CASE_F_FI(scalbnf, 0xff800000, -126, 0xff800000) /* -inf, -126: -inf */
CASE_F_FI(scalbnf, 0xff800000, -125, 0xff800000) /* -inf, -125: -inf */
CASE_F_FI(scalbnf, 0xff800000, -24, 0xff800000) /* -inf, -24: -inf */
CASE_F_FI(scalbnf, 0xff800000, -1, 0xff800000) /* -inf, -1: -inf */
CASE_F_FI(scalbnf, 0xff800000, 0, 0xff800000) /* -inf, 0: -inf */
CASE_F_FI(scalbnf, 0xff800000, 1, 0xff800000) /* -inf, 1: -inf */
CASE_F_FI(scalbnf, 0xff800000, 23, 0xff800000) /* -inf, 23: -inf */
CASE_F_FI(scalbnf, 0xff800000, 127, 0xff800000) /* -inf, 127: -inf */
CASE_F_FI(scalbnf, 0xff800000, 128, 0xff800000) /* -inf, 128: -inf */
CASE_F_FI(scalbnf, 0xff800000, 253, 0xff800000) /* -inf, 253: -inf */
CASE_F_FI(scalbnf, 0xff800000, 254, 0xff800000) /* -inf, 254: -inf */
CASE_F_FI(scalbnf, 0xff800000, 276, 0xff800000) /* -inf, 276: -inf */
CASE_F_FI(scalbnf, 0xff800000, 277, 0xff800000) /* -inf, 277: -inf */
CASE_F_FI(scalbnf, 0xff800000, 278, 0xff800000) /* -inf, 278: -inf */
CASE_F_FI(scalbnf, 0xff800000, 300, 0xff800000) /* -inf, 300: -inf */
CASE_F_FI(scalbnf, 0xff800000, 1000, 0xff800000) /* -inf, 1000: -inf */
CASE_F_FI(scalbnf, 0xff800000, 2147483647, 0xff800000) /* -inf, 2147483647: -inf */
CASE_F_FI(scalbnf, 0x7fc00000, -2147483648, 0x7fc00000) /* nan, -2147483648: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -1000, 0x7fc00000) /* nan, -1000: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -300, 0x7fc00000) /* nan, -300: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -278, 0x7fc00000) /* nan, -278: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -277, 0x7fc00000) /* nan, -277: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -276, 0x7fc00000) /* nan, -276: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -254, 0x7fc00000) /* nan, -254: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -253, 0x7fc00000) /* nan, -253: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -150, 0x7fc00000) /* nan, -150: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -149, 0x7fc00000) /* nan, -149: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -148, 0x7fc00000) /* nan, -148: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -127, 0x7fc00000) /* nan, -127: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -126, 0x7fc00000) /* nan, -126: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -125, 0x7fc00000) /* nan, -125: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -24, 0x7fc00000) /* nan, -24: nan */
CASE_F_FI(scalbnf, 0x7fc00000, -1, 0x7fc00000) /* nan, -1: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 0, 0x7fc00000) /* nan, 0: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 1, 0x7fc00000) /* nan, 1: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 23, 0x7fc00000) /* nan, 23: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 127, 0x7fc00000) /* nan, 127: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 128, 0x7fc00000) /* nan, 128: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 253, 0x7fc00000) /* nan, 253: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 254, 0x7fc00000) /* nan, 254: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 276, 0x7fc00000) /* nan, 276: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 277, 0x7fc00000) /* nan, 277: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 278, 0x7fc00000) /* nan, 278: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 300, 0x7fc00000) /* nan, 300: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 1000, 0x7fc00000) /* nan, 1000: nan */
CASE_F_FI(scalbnf, 0x7fc00000, 2147483647, 0x7fc00000) /* nan, 2147483647: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -2147483648, 0x7fc00000) /* nan, -2147483648: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -1000, 0x7fc00000) /* nan, -1000: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -300, 0x7fc00000) /* nan, -300: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -278, 0x7fc00000) /* nan, -278: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -277, 0x7fc00000) /* nan, -277: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -276, 0x7fc00000) /* nan, -276: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -254, 0x7fc00000) /* nan, -254: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -253, 0x7fc00000) /* nan, -253: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -150, 0x7fc00000) /* nan, -150: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -149, 0x7fc00000) /* nan, -149: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -148, 0x7fc00000) /* nan, -148: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -127, 0x7fc00000) /* nan, -127: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -126, 0x7fc00000) /* nan, -126: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -125, 0x7fc00000) /* nan, -125: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -24, 0x7fc00000) /* nan, -24: nan */
CASE_F_FI(scalbnf, 0x7fa00000, -1, 0x7fc00000) /* nan, -1: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 0, 0x7fc00000) /* nan, 0: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 1, 0x7fc00000) /* nan, 1: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 23, 0x7fc00000) /* nan, 23: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 127, 0x7fc00000) /* nan, 127: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 128, 0x7fc00000) /* nan, 128: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 253, 0x7fc00000) /* nan, 253: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 254, 0x7fc00000) /* nan, 254: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 276, 0x7fc00000) /* nan, 276: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 277, 0x7fc00000) /* nan, 277: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 278, 0x7fc00000) /* nan, 278: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 300, 0x7fc00000) /* nan, 300: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 1000, 0x7fc00000) /* nan, 1000: nan */
CASE_F_FI(scalbnf, 0x7fa00000, 2147483647, 0x7fc00000) /* nan, 2147483647: nan */

/* copysignf: float(float, float), NAN_BITS */
CASE_F_FF(copysignf, 0x00000000, 0x00000000, 0x00000000) /* 0, 0: 0 */
CASE_F_FF(copysignf, 0x00000000, 0x80000000, 0x80000000) /* 0, -0: -0 */
CASE_F_FF(copysignf, 0x00000000, 0x00000001, 0x00000000) /* 0, 1.40129846e-45: 0 */
CASE_F_FF(copysignf, 0x00000000, 0x3f800000, 0x00000000) /* 0, 1: 0 */
CASE_F_FF(copysignf, 0x00000000, 0xbf800000, 0x80000000) /* 0, -1: -0 */
CASE_F_FF(copysignf, 0x00000000, 0x3fc00000, 0x00000000) /* 0, 1.5: 0 */
CASE_F_FF(copysignf, 0x00000000, 0x7f7fffff, 0x00000000) /* 0, 3.40282347e+38: 0 */
CASE_F_FF(copysignf, 0x00000000, 0x7f800000, 0x00000000) /* 0, inf: 0 */
CASE_F_FF(copysignf, 0x00000000, 0xff800000, 0x80000000) /* 0, -inf: -0 */
CASE_F_FF(copysignf, 0x00000000, 0x7fc00000, 0x00000000) /* 0, nan: 0 */
CASE_F_FF(copysignf, 0x00000000, 0xffc00000, 0x80000000) /* 0, -nan: -0 */
CASE_F_FF(copysignf, 0x00000000, 0x7fa00000, 0x00000000) /* 0, nan: 0 */
CASE_F_FF(copysignf, 0x80000000, 0x00000000, 0x00000000) /* -0, 0: 0 */
CASE_F_FF(copysignf, 0x80000000, 0x80000000, 0x80000000) /* -0, -0: -0 */
CASE_F_FF(copysignf, 0x80000000, 0x00000001, 0x00000000) /* -0, 1.40129846e-45: 0 */
CASE_F_FF(copysignf, 0x80000000, 0x3f800000, 0x00000000) /* -0, 1: 0 */
CASE_F_FF(copysignf, 0x80000000, 0xbf800000, 0x80000000) /* -0, -1: -0 */
CASE_F_FF(copysignf, 0x80000000, 0x3fc00000, 0x00000000) /* -0, 1.5: 0 */
CASE_F_FF(copysignf, 0x80000000, 0x7f7fffff, 0x00000000) /* -0, 3.40282347e+38: 0 */
CASE_F_FF(copysignf, 0x80000000, 0x7f800000, 0x00000000) /* -0, inf: 0 */
CASE_F_FF(copysignf, 0x80000000, 0xff800000, 0x80000000) /* -0, -inf: -0 */
CASE_F_FF(copysignf, 0x80000000, 0x7fc00000, 0x00000000) /* -0, nan: 0 */
CASE_F_FF(copysignf, 0x80000000, 0xffc00000, 0x80000000) /* -0, -nan: -0 */
CASE_F_FF(copysignf, 0x80000000, 0x7fa00000, 0x00000000) /* -0, nan: 0 */
CASE_F_FF(copysignf, 0x00000001, 0x00000000, 0x00000001) /* 1.40129846e-45, 0: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x80000000, 0x80000001) /* 1.40129846e-45, -0: -1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x00000001, 0x00000001) /* 1.40129846e-45, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x3f800000, 0x00000001) /* 1.40129846e-45, 1: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0xbf800000, 0x80000001) /* 1.40129846e-45, -1: -1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x3fc00000, 0x00000001) /* 1.40129846e-45, 1.5: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x7f7fffff, 0x00000001) /* 1.40129846e-45, 3.40282347e+38: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x7f800000, 0x00000001) /* 1.40129846e-45, inf: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0xff800000, 0x80000001) /* 1.40129846e-45, -inf: -1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x7fc00000, 0x00000001) /* 1.40129846e-45, nan: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0xffc00000, 0x80000001) /* 1.40129846e-45, -nan: -1.40129846e-45 */
CASE_F_FF(copysignf, 0x00000001, 0x7fa00000, 0x00000001) /* 1.40129846e-45, nan: 1.40129846e-45 */
CASE_F_FF(copysignf, 0x3f800000, 0x00000000, 0x3f800000) /* 1, 0: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0x80000000, 0xbf800000) /* 1, -0: -1 */
CASE_F_FF(copysignf, 0x3f800000, 0x00000001, 0x3f800000) /* 1, 1.40129846e-45: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0x3f800000, 0x3f800000) /* 1, 1: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0xbf800000, 0xbf800000) /* 1, -1: -1 */
CASE_F_FF(copysignf, 0x3f800000, 0x3fc00000, 0x3f800000) /* 1, 1.5: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0x7f7fffff, 0x3f800000) /* 1, 3.40282347e+38: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0x7f800000, 0x3f800000) /* 1, inf: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0xff800000, 0xbf800000) /* 1, -inf: -1 */
CASE_F_FF(copysignf, 0x3f800000, 0x7fc00000, 0x3f800000) /* 1, nan: 1 */
CASE_F_FF(copysignf, 0x3f800000, 0xffc00000, 0xbf800000) /* 1, -nan: -1 */
CASE_F_FF(copysignf, 0x3f800000, 0x7fa00000, 0x3f800000) /* 1, nan: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0x00000000, 0x3f800000) /* -1, 0: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0x80000000, 0xbf800000) /* -1, -0: -1 */
CASE_F_FF(copysignf, 0xbf800000, 0x00000001, 0x3f800000) /* -1, 1.40129846e-45: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0x3f800000, 0x3f800000) /* -1, 1: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0xbf800000, 0xbf800000) /* -1, -1: -1 */
CASE_F_FF(copysignf, 0xbf800000, 0x3fc00000, 0x3f800000) /* -1, 1.5: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0x7f7fffff, 0x3f800000) /* -1, 3.40282347e+38: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0x7f800000, 0x3f800000) /* -1, inf: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0xff800000, 0xbf800000) /* -1, -inf: -1 */
CASE_F_FF(copysignf, 0xbf800000, 0x7fc00000, 0x3f800000) /* -1, nan: 1 */
CASE_F_FF(copysignf, 0xbf800000, 0xffc00000, 0xbf800000) /* -1, -nan: -1 */
CASE_F_FF(copysignf, 0xbf800000, 0x7fa00000, 0x3f800000) /* -1, nan: 1 */
CASE_F_FF(copysignf, 0x3fc00000, 0x00000000, 0x3fc00000) /* 1.5, 0: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x80000000, 0xbfc00000) /* 1.5, -0: -1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x00000001, 0x3fc00000) /* 1.5, 1.40129846e-45: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x3f800000, 0x3fc00000) /* 1.5, 1: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0xbf800000, 0xbfc00000) /* 1.5, -1: -1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x3fc00000, 0x3fc00000) /* 1.5, 1.5: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x7f7fffff, 0x3fc00000) /* 1.5, 3.40282347e+38: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x7f800000, 0x3fc00000) /* 1.5, inf: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0xff800000, 0xbfc00000) /* 1.5, -inf: -1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x7fc00000, 0x3fc00000) /* 1.5, nan: 1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0xffc00000, 0xbfc00000) /* 1.5, -nan: -1.5 */
CASE_F_FF(copysignf, 0x3fc00000, 0x7fa00000, 0x3fc00000) /* 1.5, nan: 1.5 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x00000000, 0x7f7fffff) /* 3.40282347e+38, 0: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x80000000, 0xff7fffff) /* 3.40282347e+38, -0: -3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x00000001, 0x7f7fffff) /* 3.40282347e+38, 1.40129846e-45: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x3f800000, 0x7f7fffff) /* 3.40282347e+38, 1: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0xbf800000, 0xff7fffff) /* 3.40282347e+38, -1: -3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x3fc00000, 0x7f7fffff) /* 3.40282347e+38, 1.5: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x7f800000, 0x7f7fffff) /* 3.40282347e+38, inf: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0xff800000, 0xff7fffff) /* 3.40282347e+38, -inf: -3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x7fc00000, 0x7f7fffff) /* 3.40282347e+38, nan: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0xffc00000, 0xff7fffff) /* 3.40282347e+38, -nan: -3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f7fffff, 0x7fa00000, 0x7f7fffff) /* 3.40282347e+38, nan: 3.40282347e+38 */
CASE_F_FF(copysignf, 0x7f800000, 0x00000000, 0x7f800000) /* inf, 0: inf */
CASE_F_FF(copysignf, 0x7f800000, 0x80000000, 0xff800000) /* inf, -0: -inf */
CASE_F_FF(copysignf, 0x7f800000, 0x00000001, 0x7f800000) /* inf, 1.40129846e-45: inf */
CASE_F_FF(copysignf, 0x7f800000, 0x3f800000, 0x7f800000) /* inf, 1: inf */
CASE_F_FF(copysignf, 0x7f800000, 0xbf800000, 0xff800000) /* inf, -1: -inf */
CASE_F_FF(copysignf, 0x7f800000, 0x3fc00000, 0x7f800000) /* inf, 1.5: inf */
CASE_F_FF(copysignf, 0x7f800000, 0x7f7fffff, 0x7f800000) /* inf, 3.40282347e+38: inf */
CASE_F_FF(copysignf, 0x7f800000, 0x7f800000, 0x7f800000) /* inf, inf: inf */
CASE_F_FF(copysignf, 0x7f800000, 0xff800000, 0xff800000) /* inf, -inf: -inf */
CASE_F_FF(copysignf, 0x7f800000, 0x7fc00000, 0x7f800000) /* inf, nan: inf */
CASE_F_FF(copysignf, 0x7f800000, 0xffc00000, 0xff800000) /* inf, -nan: -inf */
CASE_F_FF(copysignf, 0x7f800000, 0x7fa00000, 0x7f800000) /* inf, nan: inf */
CASE_F_FF(copysignf, 0xff800000, 0x00000000, 0x7f800000) /* -inf, 0: inf */
CASE_F_FF(copysignf, 0xff800000, 0x80000000, 0xff800000) /* -inf, -0: -inf */
CASE_F_FF(copysignf, 0xff800000, 0x00000001, 0x7f800000) /* -inf, 1.40129846e-45: inf */
CASE_F_FF(copysignf, 0xff800000, 0x3f800000, 0x7f800000) /* -inf, 1: inf */
CASE_F_FF(copysignf, 0xff800000, 0xbf800000, 0xff800000) /* -inf, -1: -inf */
CASE_F_FF(copysignf, 0xff800000, 0x3fc00000, 0x7f800000) /* -inf, 1.5: inf */
CASE_F_FF(copysignf, 0xff800000, 0x7f7fffff, 0x7f800000) /* -inf, 3.40282347e+38: inf */
CASE_F_FF(copysignf, 0xff800000, 0x7f800000, 0x7f800000) /* -inf, inf: inf */
CASE_F_FF(copysignf, 0xff800000, 0xff800000, 0xff800000) /* -inf, -inf: -inf */
CASE_F_FF(copysignf, 0xff800000, 0x7fc00000, 0x7f800000) /* -inf, nan: inf */
CASE_F_FF(copysignf, 0xff800000, 0xffc00000, 0xff800000) /* -inf, -nan: -inf */
CASE_F_FF(copysignf, 0xff800000, 0x7fa00000, 0x7f800000) /* -inf, nan: inf */
CASE_F_FF(copysignf, 0x7fc00000, 0x00000000, 0x7fc00000) /* nan, 0: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x80000000, 0xffc00000) /* nan, -0: -nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x00000001, 0x7fc00000) /* nan, 1.40129846e-45: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x3f800000, 0x7fc00000) /* nan, 1: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0xbf800000, 0xffc00000) /* nan, -1: -nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x3fc00000, 0x7fc00000) /* nan, 1.5: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x7f7fffff, 0x7fc00000) /* nan, 3.40282347e+38: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x7f800000, 0x7fc00000) /* nan, inf: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0xff800000, 0xffc00000) /* nan, -inf: -nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x7fc00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(copysignf, 0x7fc00000, 0xffc00000, 0xffc00000) /* nan, -nan: -nan */
CASE_F_FF(copysignf, 0x7fc00000, 0x7fa00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(copysignf, 0xffc00000, 0x00000000, 0x7fc00000) /* -nan, 0: nan */
CASE_F_FF(copysignf, 0xffc00000, 0x80000000, 0xffc00000) /* -nan, -0: -nan */
CASE_F_FF(copysignf, 0xffc00000, 0x00000001, 0x7fc00000) /* -nan, 1.40129846e-45: nan */
CASE_F_FF(copysignf, 0xffc00000, 0x3f800000, 0x7fc00000) /* -nan, 1: nan */
CASE_F_FF(copysignf, 0xffc00000, 0xbf800000, 0xffc00000) /* -nan, -1: -nan */
CASE_F_FF(copysignf, 0xffc00000, 0x3fc00000, 0x7fc00000) /* -nan, 1.5: nan */
CASE_F_FF(copysignf, 0xffc00000, 0x7f7fffff, 0x7fc00000) /* -nan, 3.40282347e+38: nan */
CASE_F_FF(copysignf, 0xffc00000, 0x7f800000, 0x7fc00000) /* -nan, inf: nan */
CASE_F_FF(copysignf, 0xffc00000, 0xff800000, 0xffc00000) /* -nan, -inf: -nan */
CASE_F_FF(copysignf, 0xffc00000, 0x7fc00000, 0x7fc00000) /* -nan, nan: nan */
CASE_F_FF(copysignf, 0xffc00000, 0xffc00000, 0xffc00000) /* -nan, -nan: -nan */
CASE_F_FF(copysignf, 0xffc00000, 0x7fa00000, 0x7fc00000) /* -nan, nan: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x00000000, 0x7fa00000) /* nan, 0: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x80000000, 0xffa00000) /* nan, -0: -nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x00000001, 0x7fa00000) /* nan, 1.40129846e-45: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x3f800000, 0x7fa00000) /* nan, 1: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0xbf800000, 0xffa00000) /* nan, -1: -nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x3fc00000, 0x7fa00000) /* nan, 1.5: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x7f7fffff, 0x7fa00000) /* nan, 3.40282347e+38: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x7f800000, 0x7fa00000) /* nan, inf: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0xff800000, 0xffa00000) /* nan, -inf: -nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x7fc00000, 0x7fa00000) /* nan, nan: nan */
CASE_F_FF(copysignf, 0x7fa00000, 0xffc00000, 0xffa00000) /* nan, -nan: -nan */
CASE_F_FF(copysignf, 0x7fa00000, 0x7fa00000, 0x7fa00000) /* nan, nan: nan */

/* fminf: float(float, float), MIN */
CASE_F_FF(fminf, 0x00000000, 0x00000000, 0x00000000) /* 0, 0: 0 */
CASE_F_FF(fminf, 0x00000000, 0x80000000, 0x80000000) /* 0, -0: -0 */
CASE_F_FF(fminf, 0x00000000, 0x00000001, 0x00000000) /* 0, 1.40129846e-45: 0 */
CASE_F_FF(fminf, 0x00000000, 0x3f800000, 0x00000000) /* 0, 1: 0 */
CASE_F_FF(fminf, 0x00000000, 0xbf800000, 0xbf800000) /* 0, -1: -1 */
CASE_F_FF(fminf, 0x00000000, 0x3fc00000, 0x00000000) /* 0, 1.5: 0 */
CASE_F_FF(fminf, 0x00000000, 0x7f7fffff, 0x00000000) /* 0, 3.40282347e+38: 0 */
CASE_F_FF(fminf, 0x00000000, 0x7f800000, 0x00000000) /* 0, inf: 0 */
CASE_F_FF(fminf, 0x00000000, 0xff800000, 0xff800000) /* 0, -inf: -inf */
CASE_F_FF(fminf, 0x00000000, 0x7fc00000, 0x00000000) /* 0, nan: 0 */
CASE_F_FF(fminf, 0x00000000, 0xffc00000, 0x00000000) /* 0, -nan: 0 */
CASE_F_FF(fminf, 0x00000000, 0x7fa00000, 0x7fc00000) /* 0, nan: nan */
CASE_F_FF(fminf, 0x80000000, 0x00000000, 0x80000000) /* -0, 0: -0 */
CASE_F_FF(fminf, 0x80000000, 0x80000000, 0x80000000) /* -0, -0: -0 */
CASE_F_FF(fminf, 0x80000000, 0x00000001, 0x80000000) /* -0, 1.40129846e-45: -0 */
CASE_F_FF(fminf, 0x80000000, 0x3f800000, 0x80000000) /* -0, 1: -0 */
CASE_F_FF(fminf, 0x80000000, 0xbf800000, 0xbf800000) /* -0, -1: -1 */
CASE_F_FF(fminf, 0x80000000, 0x3fc00000, 0x80000000) /* -0, 1.5: -0 */
CASE_F_FF(fminf, 0x80000000, 0x7f7fffff, 0x80000000) /* -0, 3.40282347e+38: -0 */
CASE_F_FF(fminf, 0x80000000, 0x7f800000, 0x80000000) /* -0, inf: -0 */
CASE_F_FF(fminf, 0x80000000, 0xff800000, 0xff800000) /* -0, -inf: -inf */
CASE_F_FF(fminf, 0x80000000, 0x7fc00000, 0x80000000) /* -0, nan: -0 */
CASE_F_FF(fminf, 0x80000000, 0xffc00000, 0x80000000) /* -0, -nan: -0 */
CASE_F_FF(fminf, 0x80000000, 0x7fa00000, 0x7fc00000) /* -0, nan: nan */
CASE_F_FF(fminf, 0x00000001, 0x00000000, 0x00000000) /* 1.40129846e-45, 0: 0 */
CASE_F_FF(fminf, 0x00000001, 0x80000000, 0x80000000) /* 1.40129846e-45, -0: -0 */
CASE_F_FF(fminf, 0x00000001, 0x00000001, 0x00000001) /* 1.40129846e-45, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0x3f800000, 0x00000001) /* 1.40129846e-45, 1: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0xbf800000, 0xbf800000) /* 1.40129846e-45, -1: -1 */
CASE_F_FF(fminf, 0x00000001, 0x3fc00000, 0x00000001) /* 1.40129846e-45, 1.5: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0x7f7fffff, 0x00000001) /* 1.40129846e-45, 3.40282347e+38: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0x7f800000, 0x00000001) /* 1.40129846e-45, inf: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0xff800000, 0xff800000) /* 1.40129846e-45, -inf: -inf */
CASE_F_FF(fminf, 0x00000001, 0x7fc00000, 0x00000001) /* 1.40129846e-45, nan: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0xffc00000, 0x00000001) /* 1.40129846e-45, -nan: 1.40129846e-45 */
CASE_F_FF(fminf, 0x00000001, 0x7fa00000, 0x7fc00000) /* 1.40129846e-45, nan: nan */
CASE_F_FF(fminf, 0x3f800000, 0x00000000, 0x00000000) /* 1, 0: 0 */
CASE_F_FF(fminf, 0x3f800000, 0x80000000, 0x80000000) /* 1, -0: -0 */
CASE_F_FF(fminf, 0x3f800000, 0x00000001, 0x00000001) /* 1, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0x3f800000, 0x3f800000, 0x3f800000) /* 1, 1: 1 */
CASE_F_FF(fminf, 0x3f800000, 0xbf800000, 0xbf800000) /* 1, -1: -1 */
CASE_F_FF(fminf, 0x3f800000, 0x3fc00000, 0x3f800000) /* 1, 1.5: 1 */
CASE_F_FF(fminf, 0x3f800000, 0x7f7fffff, 0x3f800000) /* 1, 3.40282347e+38: 1 */
CASE_F_FF(fminf, 0x3f800000, 0x7f800000, 0x3f800000) /* 1, inf: 1 */
CASE_F_FF(fminf, 0x3f800000, 0xff800000, 0xff800000) /* 1, -inf: -inf */
CASE_F_FF(fminf, 0x3f800000, 0x7fc00000, 0x3f800000) /* 1, nan: 1 */
CASE_F_FF(fminf, 0x3f800000, 0xffc00000, 0x3f800000) /* 1, -nan: 1 */
CASE_F_FF(fminf, 0x3f800000, 0x7fa00000, 0x7fc00000) /* 1, nan: nan */
CASE_F_FF(fminf, 0xbf800000, 0x00000000, 0xbf800000) /* -1, 0: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x80000000, 0xbf800000) /* -1, -0: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x00000001, 0xbf800000) /* -1, 1.40129846e-45: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x3f800000, 0xbf800000) /* -1, 1: -1 */
CASE_F_FF(fminf, 0xbf800000, 0xbf800000, 0xbf800000) /* -1, -1: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x3fc00000, 0xbf800000) /* -1, 1.5: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x7f7fffff, 0xbf800000) /* -1, 3.40282347e+38: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x7f800000, 0xbf800000) /* -1, inf: -1 */
CASE_F_FF(fminf, 0xbf800000, 0xff800000, 0xff800000) /* -1, -inf: -inf */
CASE_F_FF(fminf, 0xbf800000, 0x7fc00000, 0xbf800000) /* -1, nan: -1 */
CASE_F_FF(fminf, 0xbf800000, 0xffc00000, 0xbf800000) /* -1, -nan: -1 */
CASE_F_FF(fminf, 0xbf800000, 0x7fa00000, 0x7fc00000) /* -1, nan: nan */
CASE_F_FF(fminf, 0x3fc00000, 0x00000000, 0x00000000) /* 1.5, 0: 0 */
CASE_F_FF(fminf, 0x3fc00000, 0x80000000, 0x80000000) /* 1.5, -0: -0 */
CASE_F_FF(fminf, 0x3fc00000, 0x00000001, 0x00000001) /* 1.5, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0x3fc00000, 0x3f800000, 0x3f800000) /* 1.5, 1: 1 */
CASE_F_FF(fminf, 0x3fc00000, 0xbf800000, 0xbf800000) /* 1.5, -1: -1 */
CASE_F_FF(fminf, 0x3fc00000, 0x3fc00000, 0x3fc00000) /* 1.5, 1.5: 1.5 */
CASE_F_FF(fminf, 0x3fc00000, 0x7f7fffff, 0x3fc00000) /* 1.5, 3.40282347e+38: 1.5 */
CASE_F_FF(fminf, 0x3fc00000, 0x7f800000, 0x3fc00000) /* 1.5, inf: 1.5 */
CASE_F_FF(fminf, 0x3fc00000, 0xff800000, 0xff800000) /* 1.5, -inf: -inf */
CASE_F_FF(fminf, 0x3fc00000, 0x7fc00000, 0x3fc00000) /* 1.5, nan: 1.5 */
CASE_F_FF(fminf, 0x3fc00000, 0xffc00000, 0x3fc00000) /* 1.5, -nan: 1.5 */
CASE_F_FF(fminf, 0x3fc00000, 0x7fa00000, 0x7fc00000) /* 1.5, nan: nan */
CASE_F_FF(fminf, 0x7f7fffff, 0x00000000, 0x00000000) /* 3.40282347e+38, 0: 0 */
CASE_F_FF(fminf, 0x7f7fffff, 0x80000000, 0x80000000) /* 3.40282347e+38, -0: -0 */
CASE_F_FF(fminf, 0x7f7fffff, 0x00000001, 0x00000001) /* 3.40282347e+38, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0x7f7fffff, 0x3f800000, 0x3f800000) /* 3.40282347e+38, 1: 1 */
CASE_F_FF(fminf, 0x7f7fffff, 0xbf800000, 0xbf800000) /* 3.40282347e+38, -1: -1 */
CASE_F_FF(fminf, 0x7f7fffff, 0x3fc00000, 0x3fc00000) /* 3.40282347e+38, 1.5: 1.5 */
CASE_F_FF(fminf, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fminf, 0x7f7fffff, 0x7f800000, 0x7f7fffff) /* 3.40282347e+38, inf: 3.40282347e+38 */
CASE_F_FF(fminf, 0x7f7fffff, 0xff800000, 0xff800000) /* 3.40282347e+38, -inf: -inf */
CASE_F_FF(fminf, 0x7f7fffff, 0x7fc00000, 0x7f7fffff) /* 3.40282347e+38, nan: 3.40282347e+38 */
CASE_F_FF(fminf, 0x7f7fffff, 0xffc00000, 0x7f7fffff) /* 3.40282347e+38, -nan: 3.40282347e+38 */
CASE_F_FF(fminf, 0x7f7fffff, 0x7fa00000, 0x7fc00000) /* 3.40282347e+38, nan: nan */
CASE_F_FF(fminf, 0x7f800000, 0x00000000, 0x00000000) /* inf, 0: 0 */
CASE_F_FF(fminf, 0x7f800000, 0x80000000, 0x80000000) /* inf, -0: -0 */
CASE_F_FF(fminf, 0x7f800000, 0x00000001, 0x00000001) /* inf, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0x7f800000, 0x3f800000, 0x3f800000) /* inf, 1: 1 */
CASE_F_FF(fminf, 0x7f800000, 0xbf800000, 0xbf800000) /* inf, -1: -1 */
CASE_F_FF(fminf, 0x7f800000, 0x3fc00000, 0x3fc00000) /* inf, 1.5: 1.5 */
CASE_F_FF(fminf, 0x7f800000, 0x7f7fffff, 0x7f7fffff) /* inf, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fminf, 0x7f800000, 0x7f800000, 0x7f800000) /* inf, inf: inf */
CASE_F_FF(fminf, 0x7f800000, 0xff800000, 0xff800000) /* inf, -inf: -inf */
CASE_F_FF(fminf, 0x7f800000, 0x7fc00000, 0x7f800000) /* inf, nan: inf */
CASE_F_FF(fminf, 0x7f800000, 0xffc00000, 0x7f800000) /* inf, -nan: inf */
CASE_F_FF(fminf, 0x7f800000, 0x7fa00000, 0x7fc00000) /* inf, nan: nan */
CASE_F_FF(fminf, 0xff800000, 0x00000000, 0xff800000) /* -inf, 0: -inf */
CASE_F_FF(fminf, 0xff800000, 0x80000000, 0xff800000) /* -inf, -0: -inf */
CASE_F_FF(fminf, 0xff800000, 0x00000001, 0xff800000) /* -inf, 1.40129846e-45: -inf */
CASE_F_FF(fminf, 0xff800000, 0x3f800000, 0xff800000) /* -inf, 1: -inf */
CASE_F_FF(fminf, 0xff800000, 0xbf800000, 0xff800000) /* -inf, -1: -inf */
CASE_F_FF(fminf, 0xff800000, 0x3fc00000, 0xff800000) /* -inf, 1.5: -inf */
CASE_F_FF(fminf, 0xff800000, 0x7f7fffff, 0xff800000) /* -inf, 3.40282347e+38: -inf */
CASE_F_FF(fminf, 0xff800000, 0x7f800000, 0xff800000) /* -inf, inf: -inf */
CASE_F_FF(fminf, 0xff800000, 0xff800000, 0xff800000) /* -inf, -inf: -inf */
CASE_F_FF(fminf, 0xff800000, 0x7fc00000, 0xff800000) /* -inf, nan: -inf */
CASE_F_FF(fminf, 0xff800000, 0xffc00000, 0xff800000) /* -inf, -nan: -inf */
CASE_F_FF(fminf, 0xff800000, 0x7fa00000, 0x7fc00000) /* -inf, nan: nan */
CASE_F_FF(fminf, 0x7fc00000, 0x00000000, 0x00000000) /* nan, 0: 0 */
CASE_F_FF(fminf, 0x7fc00000, 0x80000000, 0x80000000) /* nan, -0: -0 */
CASE_F_FF(fminf, 0x7fc00000, 0x00000001, 0x00000001) /* nan, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0x7fc00000, 0x3f800000, 0x3f800000) /* nan, 1: 1 */
CASE_F_FF(fminf, 0x7fc00000, 0xbf800000, 0xbf800000) /* nan, -1: -1 */
CASE_F_FF(fminf, 0x7fc00000, 0x3fc00000, 0x3fc00000) /* nan, 1.5: 1.5 */
CASE_F_FF(fminf, 0x7fc00000, 0x7f7fffff, 0x7f7fffff) /* nan, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fminf, 0x7fc00000, 0x7f800000, 0x7f800000) /* nan, inf: inf */
CASE_F_FF(fminf, 0x7fc00000, 0xff800000, 0xff800000) /* nan, -inf: -inf */
CASE_F_FF(fminf, 0x7fc00000, 0x7fc00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(fminf, 0x7fc00000, 0xffc00000, 0x7fc00000) /* nan, -nan: nan */
CASE_F_FF(fminf, 0x7fc00000, 0x7fa00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(fminf, 0xffc00000, 0x00000000, 0x00000000) /* -nan, 0: 0 */
CASE_F_FF(fminf, 0xffc00000, 0x80000000, 0x80000000) /* -nan, -0: -0 */
CASE_F_FF(fminf, 0xffc00000, 0x00000001, 0x00000001) /* -nan, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fminf, 0xffc00000, 0x3f800000, 0x3f800000) /* -nan, 1: 1 */
CASE_F_FF(fminf, 0xffc00000, 0xbf800000, 0xbf800000) /* -nan, -1: -1 */
CASE_F_FF(fminf, 0xffc00000, 0x3fc00000, 0x3fc00000) /* -nan, 1.5: 1.5 */
CASE_F_FF(fminf, 0xffc00000, 0x7f7fffff, 0x7f7fffff) /* -nan, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fminf, 0xffc00000, 0x7f800000, 0x7f800000) /* -nan, inf: inf */
CASE_F_FF(fminf, 0xffc00000, 0xff800000, 0xff800000) /* -nan, -inf: -inf */
CASE_F_FF(fminf, 0xffc00000, 0x7fc00000, 0x7fc00000) /* -nan, nan: nan */
CASE_F_FF(fminf, 0xffc00000, 0xffc00000, 0x7fc00000) /* -nan, -nan: nan */
CASE_F_FF(fminf, 0xffc00000, 0x7fa00000, 0x7fc00000) /* -nan, nan: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x00000000, 0x7fc00000) /* nan, 0: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x80000000, 0x7fc00000) /* nan, -0: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x00000001, 0x7fc00000) /* nan, 1.40129846e-45: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x3f800000, 0x7fc00000) /* nan, 1: nan */
CASE_F_FF(fminf, 0x7fa00000, 0xbf800000, 0x7fc00000) /* nan, -1: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x3fc00000, 0x7fc00000) /* nan, 1.5: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x7f7fffff, 0x7fc00000) /* nan, 3.40282347e+38: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x7f800000, 0x7fc00000) /* nan, inf: nan */
CASE_F_FF(fminf, 0x7fa00000, 0xff800000, 0x7fc00000) /* nan, -inf: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x7fc00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(fminf, 0x7fa00000, 0xffc00000, 0x7fc00000) /* nan, -nan: nan */
CASE_F_FF(fminf, 0x7fa00000, 0x7fa00000, 0x7fc00000) /* nan, nan: nan */

/* fmaxf: float(float, float), MAX */
CASE_F_FF(fmaxf, 0x00000000, 0x00000000, 0x00000000) /* 0, 0: 0 */
CASE_F_FF(fmaxf, 0x00000000, 0x80000000, 0x00000000) /* 0, -0: 0 */
CASE_F_FF(fmaxf, 0x00000000, 0x00000001, 0x00000001) /* 0, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000000, 0x3f800000, 0x3f800000) /* 0, 1: 1 */
CASE_F_FF(fmaxf, 0x00000000, 0xbf800000, 0x00000000) /* 0, -1: 0 */
CASE_F_FF(fmaxf, 0x00000000, 0x3fc00000, 0x3fc00000) /* 0, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0x00000000, 0x7f7fffff, 0x7f7fffff) /* 0, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x00000000, 0x7f800000, 0x7f800000) /* 0, inf: inf */
CASE_F_FF(fmaxf, 0x00000000, 0xff800000, 0x00000000) /* 0, -inf: 0 */
CASE_F_FF(fmaxf, 0x00000000, 0x7fc00000, 0x00000000) /* 0, nan: 0 */
CASE_F_FF(fmaxf, 0x00000000, 0xffc00000, 0x00000000) /* 0, -nan: 0 */
CASE_F_FF(fmaxf, 0x00000000, 0x7fa00000, 0x7fc00000) /* 0, nan: nan */
CASE_F_FF(fmaxf, 0x80000000, 0x00000000, 0x00000000) /* -0, 0: 0 */
CASE_F_FF(fmaxf, 0x80000000, 0x80000000, 0x80000000) /* -0, -0: -0 */
CASE_F_FF(fmaxf, 0x80000000, 0x00000001, 0x00000001) /* -0, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x80000000, 0x3f800000, 0x3f800000) /* -0, 1: 1 */
CASE_F_FF(fmaxf, 0x80000000, 0xbf800000, 0x80000000) /* -0, -1: -0 */
CASE_F_FF(fmaxf, 0x80000000, 0x3fc00000, 0x3fc00000) /* -0, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0x80000000, 0x7f7fffff, 0x7f7fffff) /* -0, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x80000000, 0x7f800000, 0x7f800000) /* -0, inf: inf */
CASE_F_FF(fmaxf, 0x80000000, 0xff800000, 0x80000000) /* -0, -inf: -0 */
CASE_F_FF(fmaxf, 0x80000000, 0x7fc00000, 0x80000000) /* -0, nan: -0 */
CASE_F_FF(fmaxf, 0x80000000, 0xffc00000, 0x80000000) /* -0, -nan: -0 */
CASE_F_FF(fmaxf, 0x80000000, 0x7fa00000, 0x7fc00000) /* -0, nan: nan */
CASE_F_FF(fmaxf, 0x00000001, 0x00000000, 0x00000001) /* 1.40129846e-45, 0: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0x80000000, 0x00000001) /* 1.40129846e-45, -0: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0x00000001, 0x00000001) /* 1.40129846e-45, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0x3f800000, 0x3f800000) /* 1.40129846e-45, 1: 1 */
CASE_F_FF(fmaxf, 0x00000001, 0xbf800000, 0x00000001) /* 1.40129846e-45, -1: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0x3fc00000, 0x3fc00000) /* 1.40129846e-45, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0x00000001, 0x7f7fffff, 0x7f7fffff) /* 1.40129846e-45, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x00000001, 0x7f800000, 0x7f800000) /* 1.40129846e-45, inf: inf */
CASE_F_FF(fmaxf, 0x00000001, 0xff800000, 0x00000001) /* 1.40129846e-45, -inf: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0x7fc00000, 0x00000001) /* 1.40129846e-45, nan: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0xffc00000, 0x00000001) /* 1.40129846e-45, -nan: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x00000001, 0x7fa00000, 0x7fc00000) /* 1.40129846e-45, nan: nan */
CASE_F_FF(fmaxf, 0x3f800000, 0x00000000, 0x3f800000) /* 1, 0: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0x80000000, 0x3f800000) /* 1, -0: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0x00000001, 0x3f800000) /* 1, 1.40129846e-45: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0x3f800000, 0x3f800000) /* 1, 1: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0xbf800000, 0x3f800000) /* 1, -1: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0x3fc00000, 0x3fc00000) /* 1, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0x3f800000, 0x7f7fffff, 0x7f7fffff) /* 1, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x3f800000, 0x7f800000, 0x7f800000) /* 1, inf: inf */
CASE_F_FF(fmaxf, 0x3f800000, 0xff800000, 0x3f800000) /* 1, -inf: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0x7fc00000, 0x3f800000) /* 1, nan: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0xffc00000, 0x3f800000) /* 1, -nan: 1 */
CASE_F_FF(fmaxf, 0x3f800000, 0x7fa00000, 0x7fc00000) /* 1, nan: nan */
CASE_F_FF(fmaxf, 0xbf800000, 0x00000000, 0x00000000) /* -1, 0: 0 */
CASE_F_FF(fmaxf, 0xbf800000, 0x80000000, 0x80000000) /* -1, -0: -0 */
CASE_F_FF(fmaxf, 0xbf800000, 0x00000001, 0x00000001) /* -1, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0xbf800000, 0x3f800000, 0x3f800000) /* -1, 1: 1 */
CASE_F_FF(fmaxf, 0xbf800000, 0xbf800000, 0xbf800000) /* -1, -1: -1 */
CASE_F_FF(fmaxf, 0xbf800000, 0x3fc00000, 0x3fc00000) /* -1, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0xbf800000, 0x7f7fffff, 0x7f7fffff) /* -1, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0xbf800000, 0x7f800000, 0x7f800000) /* -1, inf: inf */
CASE_F_FF(fmaxf, 0xbf800000, 0xff800000, 0xbf800000) /* -1, -inf: -1 */
CASE_F_FF(fmaxf, 0xbf800000, 0x7fc00000, 0xbf800000) /* -1, nan: -1 */
CASE_F_FF(fmaxf, 0xbf800000, 0xffc00000, 0xbf800000) /* -1, -nan: -1 */
CASE_F_FF(fmaxf, 0xbf800000, 0x7fa00000, 0x7fc00000) /* -1, nan: nan */
CASE_F_FF(fmaxf, 0x3fc00000, 0x00000000, 0x3fc00000) /* 1.5, 0: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x80000000, 0x3fc00000) /* 1.5, -0: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x00000001, 0x3fc00000) /* 1.5, 1.40129846e-45: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x3f800000, 0x3fc00000) /* 1.5, 1: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0xbf800000, 0x3fc00000) /* 1.5, -1: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x3fc00000, 0x3fc00000) /* 1.5, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x7f7fffff, 0x7f7fffff) /* 1.5, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x7f800000, 0x7f800000) /* 1.5, inf: inf */
CASE_F_FF(fmaxf, 0x3fc00000, 0xff800000, 0x3fc00000) /* 1.5, -inf: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x7fc00000, 0x3fc00000) /* 1.5, nan: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0xffc00000, 0x3fc00000) /* 1.5, -nan: 1.5 */
CASE_F_FF(fmaxf, 0x3fc00000, 0x7fa00000, 0x7fc00000) /* 1.5, nan: nan */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x00000000, 0x7f7fffff) /* 3.40282347e+38, 0: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x80000000, 0x7f7fffff) /* 3.40282347e+38, -0: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x00000001, 0x7f7fffff) /* 3.40282347e+38, 1.40129846e-45: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x3f800000, 0x7f7fffff) /* 3.40282347e+38, 1: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0xbf800000, 0x7f7fffff) /* 3.40282347e+38, -1: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x3fc00000, 0x7f7fffff) /* 3.40282347e+38, 1.5: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff) /* 3.40282347e+38, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x7f800000, 0x7f800000) /* 3.40282347e+38, inf: inf */
CASE_F_FF(fmaxf, 0x7f7fffff, 0xff800000, 0x7f7fffff) /* 3.40282347e+38, -inf: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x7fc00000, 0x7f7fffff) /* 3.40282347e+38, nan: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0xffc00000, 0x7f7fffff) /* 3.40282347e+38, -nan: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7f7fffff, 0x7fa00000, 0x7fc00000) /* 3.40282347e+38, nan: nan */
CASE_F_FF(fmaxf, 0x7f800000, 0x00000000, 0x7f800000) /* inf, 0: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x80000000, 0x7f800000) /* inf, -0: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x00000001, 0x7f800000) /* inf, 1.40129846e-45: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x3f800000, 0x7f800000) /* inf, 1: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0xbf800000, 0x7f800000) /* inf, -1: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x3fc00000, 0x7f800000) /* inf, 1.5: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x7f7fffff, 0x7f800000) /* inf, 3.40282347e+38: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x7f800000, 0x7f800000) /* inf, inf: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0xff800000, 0x7f800000) /* inf, -inf: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x7fc00000, 0x7f800000) /* inf, nan: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0xffc00000, 0x7f800000) /* inf, -nan: inf */
CASE_F_FF(fmaxf, 0x7f800000, 0x7fa00000, 0x7fc00000) /* inf, nan: nan */
CASE_F_FF(fmaxf, 0xff800000, 0x00000000, 0x00000000) /* -inf, 0: 0 */
CASE_F_FF(fmaxf, 0xff800000, 0x80000000, 0x80000000) /* -inf, -0: -0 */
CASE_F_FF(fmaxf, 0xff800000, 0x00000001, 0x00000001) /* -inf, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0xff800000, 0x3f800000, 0x3f800000) /* -inf, 1: 1 */
CASE_F_FF(fmaxf, 0xff800000, 0xbf800000, 0xbf800000) /* -inf, -1: -1 */
CASE_F_FF(fmaxf, 0xff800000, 0x3fc00000, 0x3fc00000) /* -inf, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0xff800000, 0x7f7fffff, 0x7f7fffff) /* -inf, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0xff800000, 0x7f800000, 0x7f800000) /* -inf, inf: inf */
CASE_F_FF(fmaxf, 0xff800000, 0xff800000, 0xff800000) /* -inf, -inf: -inf */
CASE_F_FF(fmaxf, 0xff800000, 0x7fc00000, 0xff800000) /* -inf, nan: -inf */
CASE_F_FF(fmaxf, 0xff800000, 0xffc00000, 0xff800000) /* -inf, -nan: -inf */
CASE_F_FF(fmaxf, 0xff800000, 0x7fa00000, 0x7fc00000) /* -inf, nan: nan */
CASE_F_FF(fmaxf, 0x7fc00000, 0x00000000, 0x00000000) /* nan, 0: 0 */
CASE_F_FF(fmaxf, 0x7fc00000, 0x80000000, 0x80000000) /* nan, -0: -0 */
CASE_F_FF(fmaxf, 0x7fc00000, 0x00000001, 0x00000001) /* nan, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0x7fc00000, 0x3f800000, 0x3f800000) /* nan, 1: 1 */
CASE_F_FF(fmaxf, 0x7fc00000, 0xbf800000, 0xbf800000) /* nan, -1: -1 */
CASE_F_FF(fmaxf, 0x7fc00000, 0x3fc00000, 0x3fc00000) /* nan, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0x7fc00000, 0x7f7fffff, 0x7f7fffff) /* nan, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0x7fc00000, 0x7f800000, 0x7f800000) /* nan, inf: inf */
CASE_F_FF(fmaxf, 0x7fc00000, 0xff800000, 0xff800000) /* nan, -inf: -inf */
CASE_F_FF(fmaxf, 0x7fc00000, 0x7fc00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(fmaxf, 0x7fc00000, 0xffc00000, 0x7fc00000) /* nan, -nan: nan */
CASE_F_FF(fmaxf, 0x7fc00000, 0x7fa00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(fmaxf, 0xffc00000, 0x00000000, 0x00000000) /* -nan, 0: 0 */
CASE_F_FF(fmaxf, 0xffc00000, 0x80000000, 0x80000000) /* -nan, -0: -0 */
CASE_F_FF(fmaxf, 0xffc00000, 0x00000001, 0x00000001) /* -nan, 1.40129846e-45: 1.40129846e-45 */
CASE_F_FF(fmaxf, 0xffc00000, 0x3f800000, 0x3f800000) /* -nan, 1: 1 */
CASE_F_FF(fmaxf, 0xffc00000, 0xbf800000, 0xbf800000) /* -nan, -1: -1 */
CASE_F_FF(fmaxf, 0xffc00000, 0x3fc00000, 0x3fc00000) /* -nan, 1.5: 1.5 */
CASE_F_FF(fmaxf, 0xffc00000, 0x7f7fffff, 0x7f7fffff) /* -nan, 3.40282347e+38: 3.40282347e+38 */
CASE_F_FF(fmaxf, 0xffc00000, 0x7f800000, 0x7f800000) /* -nan, inf: inf */
CASE_F_FF(fmaxf, 0xffc00000, 0xff800000, 0xff800000) /* -nan, -inf: -inf */
CASE_F_FF(fmaxf, 0xffc00000, 0x7fc00000, 0x7fc00000) /* -nan, nan: nan */
CASE_F_FF(fmaxf, 0xffc00000, 0xffc00000, 0x7fc00000) /* -nan, -nan: nan */
CASE_F_FF(fmaxf, 0xffc00000, 0x7fa00000, 0x7fc00000) /* -nan, nan: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x00000000, 0x7fc00000) /* nan, 0: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x80000000, 0x7fc00000) /* nan, -0: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x00000001, 0x7fc00000) /* nan, 1.40129846e-45: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x3f800000, 0x7fc00000) /* nan, 1: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0xbf800000, 0x7fc00000) /* nan, -1: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x3fc00000, 0x7fc00000) /* nan, 1.5: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x7f7fffff, 0x7fc00000) /* nan, 3.40282347e+38: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x7f800000, 0x7fc00000) /* nan, inf: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0xff800000, 0x7fc00000) /* nan, -inf: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x7fc00000, 0x7fc00000) /* nan, nan: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0xffc00000, 0x7fc00000) /* nan, -nan: nan */
CASE_F_FF(fmaxf, 0x7fa00000, 0x7fa00000, 0x7fc00000) /* nan, nan: nan */

/* fmaf: float(float, float, float), NAN_QUIET */
CASE_F_FFF(fmaf, 0x7f800000, 0x00000000, 0x3f800000, 0x7fc00000) /* inf, 0, 1: nan */
CASE_F_FFF(fmaf, 0x00000000, 0x7f800000, 0x3f800000, 0x7fc00000) /* 0, inf, 1: nan */
CASE_F_FFF(fmaf, 0x7f800000, 0x3f800000, 0xff800000, 0x7fc00000) /* inf, 1, -inf: nan */
CASE_F_FFF(fmaf, 0x7f800000, 0x3f800000, 0x7f800000, 0x7f800000) /* inf, 1, inf: inf */
CASE_F_FFF(fmaf, 0xff800000, 0x3f800000, 0x7f800000, 0x7fc00000) /* -inf, 1, inf: nan */
CASE_F_FFF(fmaf, 0x7f800000, 0xbf800000, 0x3f800000, 0xff800000) /* inf, -1, 1: -inf */
CASE_F_FFF(fmaf, 0x7fc00000, 0x3f800000, 0x3f800000, 0x7fc00000) /* nan, 1, 1: nan */
CASE_F_FFF(fmaf, 0x3f800000, 0x7fc00000, 0x3f800000, 0x7fc00000) /* 1, nan, 1: nan */
CASE_F_FFF(fmaf, 0x3f800000, 0x3f800000, 0x7fc00000, 0x7fc00000) /* 1, 1, nan: nan */
CASE_F_FFF(fmaf, 0x7fa00000, 0x3f800000, 0x3f800000, 0x7fc00000) /* nan, 1, 1: nan */
CASE_F_FFF(fmaf, 0x7f800000, 0x00000000, 0x7fc00000, 0x7fc00000) /* inf, 0, nan: nan */
CASE_F_FFF(fmaf, 0x3f800000, 0x3f800000, 0x7fa00000, 0x7fc00000) /* 1, 1, nan: nan */
CASE_F_FFF(fmaf, 0x00000000, 0x3f800000, 0x00000000, 0x00000000) /* 0, 1, 0: 0 */
CASE_F_FFF(fmaf, 0x80000000, 0x3f800000, 0x00000000, 0x00000000) /* -0, 1, 0: 0 */
CASE_F_FFF(fmaf, 0x80000000, 0x3f800000, 0x80000000, 0x80000000) /* -0, 1, -0: -0 */
CASE_F_FFF(fmaf, 0x00000000, 0xbf800000, 0x80000000, 0x80000000) /* 0, -1, -0: -0 */
CASE_F_FFF(fmaf, 0x3f800000, 0x3f800000, 0xbf800000, 0x00000000) /* 1, 1, -1: 0 */
CASE_F_FFF(fmaf, 0xbf800000, 0x3f800000, 0x3f800000, 0x00000000) /* -1, 1, 1: 0 */
CASE_F_FFF(fmaf, 0x0d800000, 0x0d800000, 0x00000000, 0x00000000) /* 7.88860905e-31, 7.88860905e-31, 0: 0 */
CASE_F_FFF(fmaf, 0x8d800000, 0x0d800000, 0x00000000, 0x80000000) /* -7.88860905e-31, 7.88860905e-31, 0: -0 */
CASE_F_FFF(fmaf, 0x0d800000, 0x8d800000, 0x80000000, 0x80000000) /* 7.88860905e-31, -7.88860905e-31, -0: -0 */
CASE_F_FFF(fmaf, 0x1a000000, 0x1a000000, 0x00000000, 0x00000000) /* 2.64697796e-23, 2.64697796e-23, 0: 0 */
CASE_F_FFF(fmaf, 0x1a000000, 0x1a400000, 0x00000000, 0x00000001) /* 2.64697796e-23, 3.97046694e-23, 0: 1.40129846e-45 */
CASE_F_FFF(fmaf, 0x1a000000, 0x1a000000, 0x00000001, 0x00000002) /* 2.64697796e-23, 2.64697796e-23, 1.40129846e-45: 2.80259693e-45 */
CASE_F_FFF(fmaf, 0x1a000000, 0x1a000000, 0x80000001, 0x80000000) /* 2.64697796e-23, 2.64697796e-23, -1.40129846e-45: -0 */
CASE_F_FFF(fmaf, 0x1a400000, 0x1a400000, 0x80000001, 0x00000000) /* 3.97046694e-23, 3.97046694e-23, -1.40129846e-45: 0 */
CASE_F_FFF(fmaf, 0x7f7fffff, 0x40000000, 0xff800000, 0xff800000) /* 3.40282347e+38, 2, -inf: -inf */
CASE_F_FFF(fmaf, 0x7f7fffff, 0x7f7fffff, 0x00000000, 0x7f800000) /* 3.40282347e+38, 3.40282347e+38, 0: inf */
CASE_F_FFF(fmaf, 0x7f7fffff, 0x40000000, 0xff7fffff, 0x7f7fffff) /* 3.40282347e+38, 2, -3.40282347e+38: 3.40282347e+38 */
CASE_F_FFF(fmaf, 0x7f7fffff, 0xc0000000, 0x7f7fffff, 0xff7fffff) /* 3.40282347e+38, -2, 3.40282347e+38: -3.40282347e+38 */
CASE_F_FFF(fmaf, 0x7f7fffff, 0x3f800001, 0x00000000, 0x7f800000) /* 3.40282347e+38, 1.00000012, 0: inf */
CASE_F_FFF(fmaf, 0x5f800000, 0x5f800000, 0xff000000, 0x7f000000) /* 1.84467441e+19, 1.84467441e+19, -1.70141183e+38: 1.70141183e+38 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000001, 0x00000000, 0x4bc00002) /* 3, 8388609, 0: 25165828 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000001, 0x00000001, 0x4bc00002) /* 3, 8388609, 1.40129846e-45: 25165828 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000001, 0x80000001, 0x4bc00001) /* 3, 8388609, -1.40129846e-45: 25165826 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000001, 0x0d800000, 0x4bc00002) /* 3, 8388609, 7.88860905e-31: 25165828 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000001, 0x8d800000, 0x4bc00001) /* 3, 8388609, -7.88860905e-31: 25165826 */
CASE_F_FFF(fmaf, 0xc0400000, 0x4b000001, 0x00000001, 0xcbc00001) /* -3, 8388609, 1.40129846e-45: -25165826 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000003, 0x00000001, 0x4bc00005) /* 3, 8388611, 1.40129846e-45: 25165834 */
CASE_F_FFF(fmaf, 0x40400000, 0x4b000003, 0x80000001, 0x4bc00004) /* 3, 8388611, -1.40129846e-45: 25165832 */
CASE_F_FFF(fmaf, 0x40a00000, 0x4acccccd, 0x00000000, 0x4c000000) /* 5, 6710886.5, 0: 33554432 */
CASE_F_FFF(fmaf, 0x40a00000, 0x4acccccd, 0x00000001, 0x4c000000) /* 5, 6710886.5, 1.40129846e-45: 33554432 */
CASE_F_FFF(fmaf, 0x3f800001, 0x3f7fffff, 0xbf800000, 0x337ffffe) /* 1.00000012, 0.99999994, -1: 5.96046377e-08 */
CASE_F_FFF(fmaf, 0x3f800001, 0x3f800001, 0xbf800002, 0x28800000) /* 1.00000012, 1.00000012, -1.00000024: 1.42108547e-14 */
CASE_F_FFF(fmaf, 0x3f8ccccd, 0x3f8ccccd, 0xbf9ae148, 0x3275c290) /* 1.10000002, 1.10000002, -1.21000004: 1.43051153e-08 */
CASE_F_FFF(fmaf, 0x3dcccccd, 0x41200000, 0xbf800000, 0x32800000) /* 0.100000001, 10, -1: 1.49011612e-08 */
CASE_F_FFF(fmaf, 0x3eaaaaab, 0x40400000, 0xbf800000, 0x33000000) /* 0.333333343, 3, -1: 2.98023224e-08 */
CASE_F_FFF(fmaf, 0x00000003, 0x3f000000, 0x00000000, 0x00000002) /* 4.20389539e-45, 0.5, 0: 2.80259693e-45 */
CASE_F_FFF(fmaf, 0x00000001, 0x3f000000, 0x00000000, 0x00000000) /* 1.40129846e-45, 0.5, 0: 0 */
CASE_F_FFF(fmaf, 0x00000001, 0x3f400000, 0x80000000, 0x00000001) /* 1.40129846e-45, 0.75, -0: 1.40129846e-45 */
CASE_F_FFF(fmaf, 0x00000003, 0x3f000000, 0x00000001, 0x00000002) /* 4.20389539e-45, 0.5, 1.40129846e-45: 2.80259693e-45 */
CASE_F_FFF(fmaf, 0x00800000, 0x3f7fffff, 0x00000000, 0x00800000) /* 1.17549435e-38, 0.99999994, 0: 1.17549435e-38 */
CASE_F_FFF(fmaf, 0x00800001, 0x3f000000, 0x00000000, 0x00400000) /* 1.17549449e-38, 0.5, 0: 5.87747175e-39 */
CASE_F_FFF(fmaf, 0x3f800000, 0x3f800000, 0x33800000, 0x3f800000) /* 1, 1, 5.96046448e-08: 1 */
CASE_F_FFF(fmaf, 0x3f800000, 0x3f800000, 0x33800001, 0x3f800001) /* 1, 1, 5.96046519e-08: 1.00000012 */
CASE_F_FFF(fmaf, 0x3f800000, 0x3f800000, 0xb3800000, 0x3f7fffff) /* 1, 1, -5.96046448e-08: 0.99999994 */
CASE_F_FFF(fmaf, 0x3f800001, 0x3f800000, 0xb3800000, 0x3f800000) /* 1.00000012, 1, -5.96046448e-08: 1 */
CASE_F_FFF(fmaf, 0x3f800001, 0x3f800000, 0x33800000, 0x3f800002) /* 1.00000012, 1, 5.96046448e-08: 1.00000024 */

/* sqrt: double(double), NAN_QUIET */
CASE_D_D(sqrt, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(sqrt, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(sqrt, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(sqrt, 0xfff0000000000000, 0x7ff8000000000000) /* -inf: nan */
CASE_D_D(sqrt, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(sqrt, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(sqrt, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(sqrt, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(sqrt, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(sqrt, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(sqrt, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(sqrt, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(sqrt, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(sqrt, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(sqrt, 0x0000000000000001, 0x1e60000000000000) /* 4.9406564584124654e-324: 2.2227587494850775e-162 */
CASE_D_D(sqrt, 0x8000000000000001, 0x7ff8000000000000) /* -4.9406564584124654e-324: nan */
CASE_D_D(sqrt, 0x0000000000000002, 0x1e66a09e667f3bcd) /* 9.8813129168249309e-324: 3.1434555694052576e-162 */
CASE_D_D(sqrt, 0x8000000000000002, 0x7ff8000000000000) /* -9.8813129168249309e-324: nan */
CASE_D_D(sqrt, 0x0000000000000003, 0x1e6bb67ae8584caa) /* 1.4821969375237396e-323: 3.849931087076416e-162 */
CASE_D_D(sqrt, 0x8000000000000003, 0x7ff8000000000000) /* -1.4821969375237396e-323: nan */
CASE_D_D(sqrt, 0x0007ffffffffffff, 0x1ff6a09e667f3bcb) /* 1.1125369292536002e-308: 1.0547686614862996e-154 */
CASE_D_D(sqrt, 0x8007ffffffffffff, 0x7ff8000000000000) /* -1.1125369292536002e-308: nan */
CASE_D_D(sqrt, 0x0008000000000000, 0x1ff6a09e667f3bcd) /* 1.1125369292536007e-308: 1.0547686614863e-154 */
CASE_D_D(sqrt, 0x8008000000000000, 0x7ff8000000000000) /* -1.1125369292536007e-308: nan */
CASE_D_D(sqrt, 0x000fffffffffffff, 0x1fffffffffffffff) /* 2.2250738585072009e-308: 1.4916681462400412e-154 */
CASE_D_D(sqrt, 0x800fffffffffffff, 0x7ff8000000000000) /* -2.2250738585072009e-308: nan */
CASE_D_D(sqrt, 0x0010000000000000, 0x2000000000000000) /* 2.2250738585072014e-308: 1.4916681462400413e-154 */
CASE_D_D(sqrt, 0x8010000000000000, 0x7ff8000000000000) /* -2.2250738585072014e-308: nan */
CASE_D_D(sqrt, 0x0010000000000001, 0x2000000000000000) /* 2.2250738585072019e-308: 1.4916681462400413e-154 */
CASE_D_D(sqrt, 0x8010000000000001, 0x7ff8000000000000) /* -2.2250738585072019e-308: nan */
CASE_D_D(sqrt, 0x3fd0000000000000, 0x3fe0000000000000) /* 0.25: 0.5 */
CASE_D_D(sqrt, 0xbfd0000000000000, 0x7ff8000000000000) /* -0.25: nan */
CASE_D_D(sqrt, 0x3fdfffffffffffff, 0x3fe6a09e667f3bcc) /* 0.49999999999999994: 0.70710678118654746 */
CASE_D_D(sqrt, 0xbfdfffffffffffff, 0x7ff8000000000000) /* -0.49999999999999994: nan */
CASE_D_D(sqrt, 0x3fe0000000000000, 0x3fe6a09e667f3bcd) /* 0.5: 0.70710678118654757 */
CASE_D_D(sqrt, 0xbfe0000000000000, 0x7ff8000000000000) /* -0.5: nan */
CASE_D_D(sqrt, 0x3fe0000000000001, 0x3fe6a09e667f3bcd) /* 0.50000000000000011: 0.70710678118654757 */
CASE_D_D(sqrt, 0xbfe0000000000001, 0x7ff8000000000000) /* -0.50000000000000011: nan */
CASE_D_D(sqrt, 0x3fe7ffffffffffff, 0x3febb67ae8584caa) /* 0.74999999999999989: 0.8660254037844386 */
CASE_D_D(sqrt, 0xbfe7ffffffffffff, 0x7ff8000000000000) /* -0.74999999999999989: nan */
CASE_D_D(sqrt, 0x3fe8000000000000, 0x3febb67ae8584caa) /* 0.75: 0.8660254037844386 */
CASE_D_D(sqrt, 0xbfe8000000000000, 0x7ff8000000000000) /* -0.75: nan */
CASE_D_D(sqrt, 0x3fefffffffffffff, 0x3fefffffffffffff) /* 0.99999999999999989: 0.99999999999999989 */
CASE_D_D(sqrt, 0xbfefffffffffffff, 0x7ff8000000000000) /* -0.99999999999999989: nan */
CASE_D_D(sqrt, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(sqrt, 0xbff0000000000000, 0x7ff8000000000000) /* -1: nan */
CASE_D_D(sqrt, 0x3ff0000000000001, 0x3ff0000000000000) /* 1.0000000000000002: 1 */
CASE_D_D(sqrt, 0xbff0000000000001, 0x7ff8000000000000) /* -1.0000000000000002: nan */
CASE_D_D(sqrt, 0x3ff7ffffffffffff, 0x3ff3988e1409212e) /* 1.4999999999999998: 1.2247448713915889 */
CASE_D_D(sqrt, 0xbff7ffffffffffff, 0x7ff8000000000000) /* -1.4999999999999998: nan */
CASE_D_D(sqrt, 0x3ff8000000000000, 0x3ff3988e1409212e) /* 1.5: 1.2247448713915889 */
CASE_D_D(sqrt, 0xbff8000000000000, 0x7ff8000000000000) /* -1.5: nan */
CASE_D_D(sqrt, 0x3ff8000000000001, 0x3ff3988e1409212f) /* 1.5000000000000002: 1.2247448713915892 */
CASE_D_D(sqrt, 0xbff8000000000001, 0x7ff8000000000000) /* -1.5000000000000002: nan */
CASE_D_D(sqrt, 0x4000000000000000, 0x3ff6a09e667f3bcd) /* 2: 1.4142135623730951 */
CASE_D_D(sqrt, 0xc000000000000000, 0x7ff8000000000000) /* -2: nan */
CASE_D_D(sqrt, 0x4003ffffffffffff, 0x3ff94c583ada5b52) /* 2.4999999999999996: 1.5811388300841895 */
CASE_D_D(sqrt, 0xc003ffffffffffff, 0x7ff8000000000000) /* -2.4999999999999996: nan */
CASE_D_D(sqrt, 0x4004000000000000, 0x3ff94c583ada5b53) /* 2.5: 1.5811388300841898 */
CASE_D_D(sqrt, 0xc004000000000000, 0x7ff8000000000000) /* -2.5: nan */
CASE_D_D(sqrt, 0x4004000000000001, 0x3ff94c583ada5b53) /* 2.5000000000000004: 1.5811388300841898 */
CASE_D_D(sqrt, 0xc004000000000001, 0x7ff8000000000000) /* -2.5000000000000004: nan */
CASE_D_D(sqrt, 0x400c000000000000, 0x3ffdeeea11683f49) /* 3.5: 1.8708286933869707 */
CASE_D_D(sqrt, 0xc00c000000000000, 0x7ff8000000000000) /* -3.5: nan */
CASE_D_D(sqrt, 0x4012000000000000, 0x4000f876ccdf6cd9) /* 4.5: 2.1213203435596424 */
CASE_D_D(sqrt, 0xc012000000000000, 0x7ff8000000000000) /* -4.5: nan */
CASE_D_D(sqrt, 0x4025000000000000, 0x4009ec474a261264) /* 10.5: 3.2403703492039302 */
CASE_D_D(sqrt, 0xc025000000000000, 0x7ff8000000000000) /* -10.5: nan */
CASE_D_D(sqrt, 0x4027000000000000, 0x400b211b1c70d023) /* 11.5: 3.3911649915626341 */
CASE_D_D(sqrt, 0xc027000000000000, 0x7ff8000000000000) /* -11.5: nan */
CASE_D_D(sqrt, 0x431fffffffffffff, 0x4186a09e667f3bcc) /* 2251799813685247.8: 47453132.812125772 */
CASE_D_D(sqrt, 0xc31fffffffffffff, 0x7ff8000000000000) /* -2251799813685247.8: nan */
CASE_D_D(sqrt, 0x4320000000000000, 0x4186a09e667f3bcd) /* 2251799813685248: 47453132.81212578 */
CASE_D_D(sqrt, 0xc320000000000000, 0x7ff8000000000000) /* -2251799813685248: nan */
CASE_D_D(sqrt, 0x4320000000000001, 0x4186a09e667f3bcd) /* 2251799813685248.5: 47453132.81212578 */
CASE_D_D(sqrt, 0xc320000000000001, 0x7ff8000000000000) /* -2251799813685248.5: nan */
CASE_D_D(sqrt, 0x4320000000000003, 0x4186a09e667f3bcf) /* 2251799813685249.5: 47453132.812125795 */
CASE_D_D(sqrt, 0xc320000000000003, 0x7ff8000000000000) /* -2251799813685249.5: nan */
CASE_D_D(sqrt, 0x432ffffffffffffd, 0x418ffffffffffffe) /* 4503599627370494.5: 67108863.999999985 */
CASE_D_D(sqrt, 0xc32ffffffffffffd, 0x7ff8000000000000) /* -4503599627370494.5: nan */
CASE_D_D(sqrt, 0x432ffffffffffffe, 0x418fffffffffffff) /* 4503599627370495: 67108863.999999993 */
CASE_D_D(sqrt, 0xc32ffffffffffffe, 0x7ff8000000000000) /* -4503599627370495: nan */
CASE_D_D(sqrt, 0x432fffffffffffff, 0x418fffffffffffff) /* 4503599627370495.5: 67108863.999999993 */
CASE_D_D(sqrt, 0xc32fffffffffffff, 0x7ff8000000000000) /* -4503599627370495.5: nan */
CASE_D_D(sqrt, 0x4330000000000000, 0x4190000000000000) /* 4503599627370496: 67108864 */
CASE_D_D(sqrt, 0xc330000000000000, 0x7ff8000000000000) /* -4503599627370496: nan */
CASE_D_D(sqrt, 0x4330000000000001, 0x4190000000000000) /* 4503599627370497: 67108864 */
CASE_D_D(sqrt, 0xc330000000000001, 0x7ff8000000000000) /* -4503599627370497: nan */
CASE_D_D(sqrt, 0x433fffffffffffff, 0x4196a09e667f3bcc) /* 9007199254740991: 94906265.624251544 */
CASE_D_D(sqrt, 0xc33fffffffffffff, 0x7ff8000000000000) /* -9007199254740991: nan */
CASE_D_D(sqrt, 0x4340000000000000, 0x4196a09e667f3bcd) /* 9007199254740992: 94906265.624251559 */
CASE_D_D(sqrt, 0xc340000000000000, 0x7ff8000000000000) /* -9007199254740992: nan */
CASE_D_D(sqrt, 0x4340000000000001, 0x4196a09e667f3bcd) /* 9007199254740994: 94906265.624251559 */
CASE_D_D(sqrt, 0xc340000000000001, 0x7ff8000000000000) /* -9007199254740994: nan */
CASE_D_D(sqrt, 0x43dfffffffffffff, 0x41e6a09e667f3bcc) /* 9.2233720368547748e+18: 3037000499.9760494 */
CASE_D_D(sqrt, 0xc3dfffffffffffff, 0x7ff8000000000000) /* -9.2233720368547748e+18: nan */
CASE_D_D(sqrt, 0x43e0000000000000, 0x41e6a09e667f3bcd) /* 9.2233720368547758e+18: 3037000499.9760499 */
CASE_D_D(sqrt, 0xc3e0000000000000, 0x7ff8000000000000) /* -9.2233720368547758e+18: nan */
CASE_D_D(sqrt, 0x43e0000000000001, 0x41e6a09e667f3bcd) /* 9.2233720368547779e+18: 3037000499.9760499 */
CASE_D_D(sqrt, 0xc3e0000000000001, 0x7ff8000000000000) /* -9.2233720368547779e+18: nan */
CASE_D_D(sqrt, 0x7fe0000000000000, 0x5fe6a09e667f3bcd) /* 8.9884656743115795e+307: 9.4807519081091774e+153 */
CASE_D_D(sqrt, 0xffe0000000000000, 0x7ff8000000000000) /* -8.9884656743115795e+307: nan */
CASE_D_D(sqrt, 0x7fefffffffffffff, 0x5fefffffffffffff) /* 1.7976931348623157e+308: 1.3407807929942596e+154 */
CASE_D_D(sqrt, 0xffefffffffffffff, 0x7ff8000000000000) /* -1.7976931348623157e+308: nan */
CASE_D_D(sqrt, 0x400fffffffffffff, 0x3fffffffffffffff) /* 3.9999999999999996: 1.9999999999999998 */
CASE_D_D(sqrt, 0xc00fffffffffffff, 0x7ff8000000000000) /* -3.9999999999999996: nan */
CASE_D_D(sqrt, 0x4010000000000000, 0x4000000000000000) /* 4: 2 */
CASE_D_D(sqrt, 0xc010000000000000, 0x7ff8000000000000) /* -4: nan */
CASE_D_D(sqrt, 0x4010000000000001, 0x4000000000000000) /* 4.0000000000000009: 2 */
CASE_D_D(sqrt, 0xc010000000000001, 0x7ff8000000000000) /* -4.0000000000000009: nan */
CASE_D_D(sqrt, 0x4022000000000000, 0x4008000000000000) /* 9: 3 */
CASE_D_D(sqrt, 0xc022000000000000, 0x7ff8000000000000) /* -9: nan */

/* floor: double(double), NAN_QUIET */
CASE_D_D(floor, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(floor, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(floor, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(floor, 0xfff0000000000000, 0xfff0000000000000) /* -inf: -inf */
CASE_D_D(floor, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(floor, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(floor, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(floor, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(floor, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(floor, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(floor, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(floor, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(floor, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(floor, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(floor, 0x0000000000000001, 0x0000000000000000) /* 4.9406564584124654e-324: 0 */
CASE_D_D(floor, 0x8000000000000001, 0xbff0000000000000) /* -4.9406564584124654e-324: -1 */
CASE_D_D(floor, 0x0000000000000002, 0x0000000000000000) /* 9.8813129168249309e-324: 0 */
CASE_D_D(floor, 0x8000000000000002, 0xbff0000000000000) /* -9.8813129168249309e-324: -1 */
CASE_D_D(floor, 0x0000000000000003, 0x0000000000000000) /* 1.4821969375237396e-323: 0 */
CASE_D_D(floor, 0x8000000000000003, 0xbff0000000000000) /* -1.4821969375237396e-323: -1 */
CASE_D_D(floor, 0x0007ffffffffffff, 0x0000000000000000) /* 1.1125369292536002e-308: 0 */
CASE_D_D(floor, 0x8007ffffffffffff, 0xbff0000000000000) /* -1.1125369292536002e-308: -1 */
CASE_D_D(floor, 0x0008000000000000, 0x0000000000000000) /* 1.1125369292536007e-308: 0 */
CASE_D_D(floor, 0x8008000000000000, 0xbff0000000000000) /* -1.1125369292536007e-308: -1 */
CASE_D_D(floor, 0x000fffffffffffff, 0x0000000000000000) /* 2.2250738585072009e-308: 0 */
CASE_D_D(floor, 0x800fffffffffffff, 0xbff0000000000000) /* -2.2250738585072009e-308: -1 */
CASE_D_D(floor, 0x0010000000000000, 0x0000000000000000) /* 2.2250738585072014e-308: 0 */
CASE_D_D(floor, 0x8010000000000000, 0xbff0000000000000) /* -2.2250738585072014e-308: -1 */
CASE_D_D(floor, 0x0010000000000001, 0x0000000000000000) /* 2.2250738585072019e-308: 0 */
CASE_D_D(floor, 0x8010000000000001, 0xbff0000000000000) /* -2.2250738585072019e-308: -1 */
CASE_D_D(floor, 0x3fd0000000000000, 0x0000000000000000) /* 0.25: 0 */
CASE_D_D(floor, 0xbfd0000000000000, 0xbff0000000000000) /* -0.25: -1 */
CASE_D_D(floor, 0x3fdfffffffffffff, 0x0000000000000000) /* 0.49999999999999994: 0 */
CASE_D_D(floor, 0xbfdfffffffffffff, 0xbff0000000000000) /* -0.49999999999999994: -1 */
CASE_D_D(floor, 0x3fe0000000000000, 0x0000000000000000) /* 0.5: 0 */
CASE_D_D(floor, 0xbfe0000000000000, 0xbff0000000000000) /* -0.5: -1 */
CASE_D_D(floor, 0x3fe0000000000001, 0x0000000000000000) /* 0.50000000000000011: 0 */
CASE_D_D(floor, 0xbfe0000000000001, 0xbff0000000000000) /* -0.50000000000000011: -1 */
CASE_D_D(floor, 0x3fe7ffffffffffff, 0x0000000000000000) /* 0.74999999999999989: 0 */
CASE_D_D(floor, 0xbfe7ffffffffffff, 0xbff0000000000000) /* -0.74999999999999989: -1 */
CASE_D_D(floor, 0x3fe8000000000000, 0x0000000000000000) /* 0.75: 0 */
CASE_D_D(floor, 0xbfe8000000000000, 0xbff0000000000000) /* -0.75: -1 */
CASE_D_D(floor, 0x3fefffffffffffff, 0x0000000000000000) /* 0.99999999999999989: 0 */
CASE_D_D(floor, 0xbfefffffffffffff, 0xbff0000000000000) /* -0.99999999999999989: -1 */
CASE_D_D(floor, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(floor, 0xbff0000000000000, 0xbff0000000000000) /* -1: -1 */
CASE_D_D(floor, 0x3ff0000000000001, 0x3ff0000000000000) /* 1.0000000000000002: 1 */
CASE_D_D(floor, 0xbff0000000000001, 0xc000000000000000) /* -1.0000000000000002: -2 */
CASE_D_D(floor, 0x3ff7ffffffffffff, 0x3ff0000000000000) /* 1.4999999999999998: 1 */
CASE_D_D(floor, 0xbff7ffffffffffff, 0xc000000000000000) /* -1.4999999999999998: -2 */
CASE_D_D(floor, 0x3ff8000000000000, 0x3ff0000000000000) /* 1.5: 1 */
CASE_D_D(floor, 0xbff8000000000000, 0xc000000000000000) /* -1.5: -2 */
CASE_D_D(floor, 0x3ff8000000000001, 0x3ff0000000000000) /* 1.5000000000000002: 1 */
CASE_D_D(floor, 0xbff8000000000001, 0xc000000000000000) /* -1.5000000000000002: -2 */
CASE_D_D(floor, 0x4000000000000000, 0x4000000000000000) /* 2: 2 */
CASE_D_D(floor, 0xc000000000000000, 0xc000000000000000) /* -2: -2 */
CASE_D_D(floor, 0x4003ffffffffffff, 0x4000000000000000) /* 2.4999999999999996: 2 */
CASE_D_D(floor, 0xc003ffffffffffff, 0xc008000000000000) /* -2.4999999999999996: -3 */
CASE_D_D(floor, 0x4004000000000000, 0x4000000000000000) /* 2.5: 2 */
CASE_D_D(floor, 0xc004000000000000, 0xc008000000000000) /* -2.5: -3 */
CASE_D_D(floor, 0x4004000000000001, 0x4000000000000000) /* 2.5000000000000004: 2 */
CASE_D_D(floor, 0xc004000000000001, 0xc008000000000000) /* -2.5000000000000004: -3 */
CASE_D_D(floor, 0x400c000000000000, 0x4008000000000000) /* 3.5: 3 */
CASE_D_D(floor, 0xc00c000000000000, 0xc010000000000000) /* -3.5: -4 */
CASE_D_D(floor, 0x4012000000000000, 0x4010000000000000) /* 4.5: 4 */
CASE_D_D(floor, 0xc012000000000000, 0xc014000000000000) /* -4.5: -5 */
CASE_D_D(floor, 0x4025000000000000, 0x4024000000000000) /* 10.5: 10 */
CASE_D_D(floor, 0xc025000000000000, 0xc026000000000000) /* -10.5: -11 */
CASE_D_D(floor, 0x4027000000000000, 0x4026000000000000) /* 11.5: 11 */
CASE_D_D(floor, 0xc027000000000000, 0xc028000000000000) /* -11.5: -12 */
CASE_D_D(floor, 0x431fffffffffffff, 0x431ffffffffffffc) /* 2251799813685247.8: 2251799813685247 */
CASE_D_D(floor, 0xc31fffffffffffff, 0xc320000000000000) /* -2251799813685247.8: -2251799813685248 */
CASE_D_D(floor, 0x4320000000000000, 0x4320000000000000) /* 2251799813685248: 2251799813685248 */
CASE_D_D(floor, 0xc320000000000000, 0xc320000000000000) /* -2251799813685248: -2251799813685248 */
CASE_D_D(floor, 0x4320000000000001, 0x4320000000000000) /* 2251799813685248.5: 2251799813685248 */
CASE_D_D(floor, 0xc320000000000001, 0xc320000000000002) /* -2251799813685248.5: -2251799813685249 */
CASE_D_D(floor, 0x4320000000000003, 0x4320000000000002) /* 2251799813685249.5: 2251799813685249 */
CASE_D_D(floor, 0xc320000000000003, 0xc320000000000004) /* -2251799813685249.5: -2251799813685250 */
CASE_D_D(floor, 0x432ffffffffffffd, 0x432ffffffffffffc) /* 4503599627370494.5: 4503599627370494 */
CASE_D_D(floor, 0xc32ffffffffffffd, 0xc32ffffffffffffe) /* -4503599627370494.5: -4503599627370495 */
CASE_D_D(floor, 0x432ffffffffffffe, 0x432ffffffffffffe) /* 4503599627370495: 4503599627370495 */
CASE_D_D(floor, 0xc32ffffffffffffe, 0xc32ffffffffffffe) /* -4503599627370495: -4503599627370495 */
CASE_D_D(floor, 0x432fffffffffffff, 0x432ffffffffffffe) /* 4503599627370495.5: 4503599627370495 */
CASE_D_D(floor, 0xc32fffffffffffff, 0xc330000000000000) /* -4503599627370495.5: -4503599627370496 */
CASE_D_D(floor, 0x4330000000000000, 0x4330000000000000) /* 4503599627370496: 4503599627370496 */
CASE_D_D(floor, 0xc330000000000000, 0xc330000000000000) /* -4503599627370496: -4503599627370496 */
CASE_D_D(floor, 0x4330000000000001, 0x4330000000000001) /* 4503599627370497: 4503599627370497 */
CASE_D_D(floor, 0xc330000000000001, 0xc330000000000001) /* -4503599627370497: -4503599627370497 */
CASE_D_D(floor, 0x433fffffffffffff, 0x433fffffffffffff) /* 9007199254740991: 9007199254740991 */
CASE_D_D(floor, 0xc33fffffffffffff, 0xc33fffffffffffff) /* -9007199254740991: -9007199254740991 */
CASE_D_D(floor, 0x4340000000000000, 0x4340000000000000) /* 9007199254740992: 9007199254740992 */
CASE_D_D(floor, 0xc340000000000000, 0xc340000000000000) /* -9007199254740992: -9007199254740992 */
CASE_D_D(floor, 0x4340000000000001, 0x4340000000000001) /* 9007199254740994: 9007199254740994 */
CASE_D_D(floor, 0xc340000000000001, 0xc340000000000001) /* -9007199254740994: -9007199254740994 */
CASE_D_D(floor, 0x43dfffffffffffff, 0x43dfffffffffffff) /* 9.2233720368547748e+18: 9.2233720368547748e+18 */
CASE_D_D(floor, 0xc3dfffffffffffff, 0xc3dfffffffffffff) /* -9.2233720368547748e+18: -9.2233720368547748e+18 */
CASE_D_D(floor, 0x43e0000000000000, 0x43e0000000000000) /* 9.2233720368547758e+18: 9.2233720368547758e+18 */
CASE_D_D(floor, 0xc3e0000000000000, 0xc3e0000000000000) /* -9.2233720368547758e+18: -9.2233720368547758e+18 */
CASE_D_D(floor, 0x43e0000000000001, 0x43e0000000000001) /* 9.2233720368547779e+18: 9.2233720368547779e+18 */
CASE_D_D(floor, 0xc3e0000000000001, 0xc3e0000000000001) /* -9.2233720368547779e+18: -9.2233720368547779e+18 */
CASE_D_D(floor, 0x7fe0000000000000, 0x7fe0000000000000) /* 8.9884656743115795e+307: 8.9884656743115795e+307 */
CASE_D_D(floor, 0xffe0000000000000, 0xffe0000000000000) /* -8.9884656743115795e+307: -8.9884656743115795e+307 */
CASE_D_D(floor, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_D(floor, 0xffefffffffffffff, 0xffefffffffffffff) /* -1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_D(floor, 0x400fffffffffffff, 0x4008000000000000) /* 3.9999999999999996: 3 */
CASE_D_D(floor, 0xc00fffffffffffff, 0xc010000000000000) /* -3.9999999999999996: -4 */
CASE_D_D(floor, 0x4010000000000000, 0x4010000000000000) /* 4: 4 */
CASE_D_D(floor, 0xc010000000000000, 0xc010000000000000) /* -4: -4 */
CASE_D_D(floor, 0x4010000000000001, 0x4010000000000000) /* 4.0000000000000009: 4 */
CASE_D_D(floor, 0xc010000000000001, 0xc014000000000000) /* -4.0000000000000009: -5 */
CASE_D_D(floor, 0x4022000000000000, 0x4022000000000000) /* 9: 9 */
CASE_D_D(floor, 0xc022000000000000, 0xc022000000000000) /* -9: -9 */

/* ceil: double(double), NAN_QUIET */
CASE_D_D(ceil, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(ceil, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(ceil, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(ceil, 0xfff0000000000000, 0xfff0000000000000) /* -inf: -inf */
CASE_D_D(ceil, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(ceil, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(ceil, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(ceil, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(ceil, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(ceil, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(ceil, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(ceil, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(ceil, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(ceil, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(ceil, 0x0000000000000001, 0x3ff0000000000000) /* 4.9406564584124654e-324: 1 */
CASE_D_D(ceil, 0x8000000000000001, 0x8000000000000000) /* -4.9406564584124654e-324: -0 */
CASE_D_D(ceil, 0x0000000000000002, 0x3ff0000000000000) /* 9.8813129168249309e-324: 1 */
CASE_D_D(ceil, 0x8000000000000002, 0x8000000000000000) /* -9.8813129168249309e-324: -0 */
CASE_D_D(ceil, 0x0000000000000003, 0x3ff0000000000000) /* 1.4821969375237396e-323: 1 */
CASE_D_D(ceil, 0x8000000000000003, 0x8000000000000000) /* -1.4821969375237396e-323: -0 */
CASE_D_D(ceil, 0x0007ffffffffffff, 0x3ff0000000000000) /* 1.1125369292536002e-308: 1 */
CASE_D_D(ceil, 0x8007ffffffffffff, 0x8000000000000000) /* -1.1125369292536002e-308: -0 */
CASE_D_D(ceil, 0x0008000000000000, 0x3ff0000000000000) /* 1.1125369292536007e-308: 1 */
CASE_D_D(ceil, 0x8008000000000000, 0x8000000000000000) /* -1.1125369292536007e-308: -0 */
CASE_D_D(ceil, 0x000fffffffffffff, 0x3ff0000000000000) /* 2.2250738585072009e-308: 1 */
CASE_D_D(ceil, 0x800fffffffffffff, 0x8000000000000000) /* -2.2250738585072009e-308: -0 */
CASE_D_D(ceil, 0x0010000000000000, 0x3ff0000000000000) /* 2.2250738585072014e-308: 1 */
CASE_D_D(ceil, 0x8010000000000000, 0x8000000000000000) /* -2.2250738585072014e-308: -0 */
CASE_D_D(ceil, 0x0010000000000001, 0x3ff0000000000000) /* 2.2250738585072019e-308: 1 */
CASE_D_D(ceil, 0x8010000000000001, 0x8000000000000000) /* -2.2250738585072019e-308: -0 */
CASE_D_D(ceil, 0x3fd0000000000000, 0x3ff0000000000000) /* 0.25: 1 */
CASE_D_D(ceil, 0xbfd0000000000000, 0x8000000000000000) /* -0.25: -0 */
CASE_D_D(ceil, 0x3fdfffffffffffff, 0x3ff0000000000000) /* 0.49999999999999994: 1 */
CASE_D_D(ceil, 0xbfdfffffffffffff, 0x8000000000000000) /* -0.49999999999999994: -0 */
CASE_D_D(ceil, 0x3fe0000000000000, 0x3ff0000000000000) /* 0.5: 1 */
CASE_D_D(ceil, 0xbfe0000000000000, 0x8000000000000000) /* -0.5: -0 */
CASE_D_D(ceil, 0x3fe0000000000001, 0x3ff0000000000000) /* 0.50000000000000011: 1 */
CASE_D_D(ceil, 0xbfe0000000000001, 0x8000000000000000) /* -0.50000000000000011: -0 */
CASE_D_D(ceil, 0x3fe7ffffffffffff, 0x3ff0000000000000) /* 0.74999999999999989: 1 */
CASE_D_D(ceil, 0xbfe7ffffffffffff, 0x8000000000000000) /* -0.74999999999999989: -0 */
CASE_D_D(ceil, 0x3fe8000000000000, 0x3ff0000000000000) /* 0.75: 1 */
CASE_D_D(ceil, 0xbfe8000000000000, 0x8000000000000000) /* -0.75: -0 */
CASE_D_D(ceil, 0x3fefffffffffffff, 0x3ff0000000000000) /* 0.99999999999999989: 1 */
CASE_D_D(ceil, 0xbfefffffffffffff, 0x8000000000000000) /* -0.99999999999999989: -0 */
CASE_D_D(ceil, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(ceil, 0xbff0000000000000, 0xbff0000000000000) /* -1: -1 */
CASE_D_D(ceil, 0x3ff0000000000001, 0x4000000000000000) /* 1.0000000000000002: 2 */
CASE_D_D(ceil, 0xbff0000000000001, 0xbff0000000000000) /* -1.0000000000000002: -1 */
CASE_D_D(ceil, 0x3ff7ffffffffffff, 0x4000000000000000) /* 1.4999999999999998: 2 */
CASE_D_D(ceil, 0xbff7ffffffffffff, 0xbff0000000000000) /* -1.4999999999999998: -1 */
CASE_D_D(ceil, 0x3ff8000000000000, 0x4000000000000000) /* 1.5: 2 */
CASE_D_D(ceil, 0xbff8000000000000, 0xbff0000000000000) /* -1.5: -1 */
CASE_D_D(ceil, 0x3ff8000000000001, 0x4000000000000000) /* 1.5000000000000002: 2 */
CASE_D_D(ceil, 0xbff8000000000001, 0xbff0000000000000) /* -1.5000000000000002: -1 */
CASE_D_D(ceil, 0x4000000000000000, 0x4000000000000000) /* 2: 2 */
CASE_D_D(ceil, 0xc000000000000000, 0xc000000000000000) /* -2: -2 */
CASE_D_D(ceil, 0x4003ffffffffffff, 0x4008000000000000) /* 2.4999999999999996: 3 */
CASE_D_D(ceil, 0xc003ffffffffffff, 0xc000000000000000) /* -2.4999999999999996: -2 */
CASE_D_D(ceil, 0x4004000000000000, 0x4008000000000000) /* 2.5: 3 */
CASE_D_D(ceil, 0xc004000000000000, 0xc000000000000000) /* -2.5: -2 */
CASE_D_D(ceil, 0x4004000000000001, 0x4008000000000000) /* 2.5000000000000004: 3 */
CASE_D_D(ceil, 0xc004000000000001, 0xc000000000000000) /* -2.5000000000000004: -2 */
CASE_D_D(ceil, 0x400c000000000000, 0x4010000000000000) /* 3.5: 4 */
CASE_D_D(ceil, 0xc00c000000000000, 0xc008000000000000) /* -3.5: -3 */
CASE_D_D(ceil, 0x4012000000000000, 0x4014000000000000) /* 4.5: 5 */
CASE_D_D(ceil, 0xc012000000000000, 0xc010000000000000) /* -4.5: -4 */
CASE_D_D(ceil, 0x4025000000000000, 0x4026000000000000) /* 10.5: 11 */
CASE_D_D(ceil, 0xc025000000000000, 0xc024000000000000) /* -10.5: -10 */
CASE_D_D(ceil, 0x4027000000000000, 0x4028000000000000) /* 11.5: 12 */
CASE_D_D(ceil, 0xc027000000000000, 0xc026000000000000) /* -11.5: -11 */
CASE_D_D(ceil, 0x431fffffffffffff, 0x4320000000000000) /* 2251799813685247.8: 2251799813685248 */
CASE_D_D(ceil, 0xc31fffffffffffff, 0xc31ffffffffffffc) /* -2251799813685247.8: -2251799813685247 */
CASE_D_D(ceil, 0x4320000000000000, 0x4320000000000000) /* 2251799813685248: 2251799813685248 */
CASE_D_D(ceil, 0xc320000000000000, 0xc320000000000000) /* -2251799813685248: -2251799813685248 */
CASE_D_D(ceil, 0x4320000000000001, 0x4320000000000002) /* 2251799813685248.5: 2251799813685249 */
CASE_D_D(ceil, 0xc320000000000001, 0xc320000000000000) /* -2251799813685248.5: -2251799813685248 */
CASE_D_D(ceil, 0x4320000000000003, 0x4320000000000004) /* 2251799813685249.5: 2251799813685250 */
CASE_D_D(ceil, 0xc320000000000003, 0xc320000000000002) /* -2251799813685249.5: -2251799813685249 */
CASE_D_D(ceil, 0x432ffffffffffffd, 0x432ffffffffffffe) /* 4503599627370494.5: 4503599627370495 */
CASE_D_D(ceil, 0xc32ffffffffffffd, 0xc32ffffffffffffc) /* -4503599627370494.5: -4503599627370494 */
CASE_D_D(ceil, 0x432ffffffffffffe, 0x432ffffffffffffe) /* 4503599627370495: 4503599627370495 */
CASE_D_D(ceil, 0xc32ffffffffffffe, 0xc32ffffffffffffe) /* -4503599627370495: -4503599627370495 */
CASE_D_D(ceil, 0x432fffffffffffff, 0x4330000000000000) /* 4503599627370495.5: 4503599627370496 */
CASE_D_D(ceil, 0xc32fffffffffffff, 0xc32ffffffffffffe) /* -4503599627370495.5: -4503599627370495 */
CASE_D_D(ceil, 0x4330000000000000, 0x4330000000000000) /* 4503599627370496: 4503599627370496 */
CASE_D_D(ceil, 0xc330000000000000, 0xc330000000000000) /* -4503599627370496: -4503599627370496 */
CASE_D_D(ceil, 0x4330000000000001, 0x4330000000000001) /* 4503599627370497: 4503599627370497 */
CASE_D_D(ceil, 0xc330000000000001, 0xc330000000000001) /* -4503599627370497: -4503599627370497 */
CASE_D_D(ceil, 0x433fffffffffffff, 0x433fffffffffffff) /* 9007199254740991: 9007199254740991 */
CASE_D_D(ceil, 0xc33fffffffffffff, 0xc33fffffffffffff) /* -9007199254740991: -9007199254740991 */
CASE_D_D(ceil, 0x4340000000000000, 0x4340000000000000) /* 9007199254740992: 9007199254740992 */
CASE_D_D(ceil, 0xc340000000000000, 0xc340000000000000) /* -9007199254740992: -9007199254740992 */
CASE_D_D(ceil, 0x4340000000000001, 0x4340000000000001) /* 9007199254740994: 9007199254740994 */
CASE_D_D(ceil, 0xc340000000000001, 0xc340000000000001) /* -9007199254740994: -9007199254740994 */
CASE_D_D(ceil, 0x43dfffffffffffff, 0x43dfffffffffffff) /* 9.2233720368547748e+18: 9.2233720368547748e+18 */
CASE_D_D(ceil, 0xc3dfffffffffffff, 0xc3dfffffffffffff) /* -9.2233720368547748e+18: -9.2233720368547748e+18 */
CASE_D_D(ceil, 0x43e0000000000000, 0x43e0000000000000) /* 9.2233720368547758e+18: 9.2233720368547758e+18 */
CASE_D_D(ceil, 0xc3e0000000000000, 0xc3e0000000000000) /* -9.2233720368547758e+18: -9.2233720368547758e+18 */
CASE_D_D(ceil, 0x43e0000000000001, 0x43e0000000000001) /* 9.2233720368547779e+18: 9.2233720368547779e+18 */
CASE_D_D(ceil, 0xc3e0000000000001, 0xc3e0000000000001) /* -9.2233720368547779e+18: -9.2233720368547779e+18 */
CASE_D_D(ceil, 0x7fe0000000000000, 0x7fe0000000000000) /* 8.9884656743115795e+307: 8.9884656743115795e+307 */
CASE_D_D(ceil, 0xffe0000000000000, 0xffe0000000000000) /* -8.9884656743115795e+307: -8.9884656743115795e+307 */
CASE_D_D(ceil, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_D(ceil, 0xffefffffffffffff, 0xffefffffffffffff) /* -1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_D(ceil, 0x400fffffffffffff, 0x4010000000000000) /* 3.9999999999999996: 4 */
CASE_D_D(ceil, 0xc00fffffffffffff, 0xc008000000000000) /* -3.9999999999999996: -3 */
CASE_D_D(ceil, 0x4010000000000000, 0x4010000000000000) /* 4: 4 */
CASE_D_D(ceil, 0xc010000000000000, 0xc010000000000000) /* -4: -4 */
CASE_D_D(ceil, 0x4010000000000001, 0x4014000000000000) /* 4.0000000000000009: 5 */
CASE_D_D(ceil, 0xc010000000000001, 0xc010000000000000) /* -4.0000000000000009: -4 */
CASE_D_D(ceil, 0x4022000000000000, 0x4022000000000000) /* 9: 9 */
CASE_D_D(ceil, 0xc022000000000000, 0xc022000000000000) /* -9: -9 */

/* trunc: double(double), NAN_QUIET */
CASE_D_D(trunc, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(trunc, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(trunc, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(trunc, 0xfff0000000000000, 0xfff0000000000000) /* -inf: -inf */
CASE_D_D(trunc, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(trunc, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(trunc, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(trunc, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(trunc, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(trunc, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(trunc, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(trunc, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(trunc, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(trunc, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(trunc, 0x0000000000000001, 0x0000000000000000) /* 4.9406564584124654e-324: 0 */
CASE_D_D(trunc, 0x8000000000000001, 0x8000000000000000) /* -4.9406564584124654e-324: -0 */
CASE_D_D(trunc, 0x0000000000000002, 0x0000000000000000) /* 9.8813129168249309e-324: 0 */
CASE_D_D(trunc, 0x8000000000000002, 0x8000000000000000) /* -9.8813129168249309e-324: -0 */
CASE_D_D(trunc, 0x0000000000000003, 0x0000000000000000) /* 1.4821969375237396e-323: 0 */
CASE_D_D(trunc, 0x8000000000000003, 0x8000000000000000) /* -1.4821969375237396e-323: -0 */
CASE_D_D(trunc, 0x0007ffffffffffff, 0x0000000000000000) /* 1.1125369292536002e-308: 0 */
CASE_D_D(trunc, 0x8007ffffffffffff, 0x8000000000000000) /* -1.1125369292536002e-308: -0 */
CASE_D_D(trunc, 0x0008000000000000, 0x0000000000000000) /* 1.1125369292536007e-308: 0 */
CASE_D_D(trunc, 0x8008000000000000, 0x8000000000000000) /* -1.1125369292536007e-308: -0 */
CASE_D_D(trunc, 0x000fffffffffffff, 0x0000000000000000) /* 2.2250738585072009e-308: 0 */
CASE_D_D(trunc, 0x800fffffffffffff, 0x8000000000000000) /* -2.2250738585072009e-308: -0 */
CASE_D_D(trunc, 0x0010000000000000, 0x0000000000000000) /* 2.2250738585072014e-308: 0 */
CASE_D_D(trunc, 0x8010000000000000, 0x8000000000000000) /* -2.2250738585072014e-308: -0 */
CASE_D_D(trunc, 0x0010000000000001, 0x0000000000000000) /* 2.2250738585072019e-308: 0 */
CASE_D_D(trunc, 0x8010000000000001, 0x8000000000000000) /* -2.2250738585072019e-308: -0 */
CASE_D_D(trunc, 0x3fd0000000000000, 0x0000000000000000) /* 0.25: 0 */
CASE_D_D(trunc, 0xbfd0000000000000, 0x8000000000000000) /* -0.25: -0 */
CASE_D_D(trunc, 0x3fdfffffffffffff, 0x0000000000000000) /* 0.49999999999999994: 0 */
CASE_D_D(trunc, 0xbfdfffffffffffff, 0x8000000000000000) /* -0.49999999999999994: -0 */
CASE_D_D(trunc, 0x3fe0000000000000, 0x0000000000000000) /* 0.5: 0 */
CASE_D_D(trunc, 0xbfe0000000000000, 0x8000000000000000) /* -0.5: -0 */
CASE_D_D(trunc, 0x3fe0000000000001, 0x0000000000000000) /* 0.50000000000000011: 0 */
CASE_D_D(trunc, 0xbfe0000000000001, 0x8000000000000000) /* -0.50000000000000011: -0 */
CASE_D_D(trunc, 0x3fe7ffffffffffff, 0x0000000000000000) /* 0.74999999999999989: 0 */
CASE_D_D(trunc, 0xbfe7ffffffffffff, 0x8000000000000000) /* -0.74999999999999989: -0 */
CASE_D_D(trunc, 0x3fe8000000000000, 0x0000000000000000) /* 0.75: 0 */
CASE_D_D(trunc, 0xbfe8000000000000, 0x8000000000000000) /* -0.75: -0 */
CASE_D_D(trunc, 0x3fefffffffffffff, 0x0000000000000000) /* 0.99999999999999989: 0 */
CASE_D_D(trunc, 0xbfefffffffffffff, 0x8000000000000000) /* -0.99999999999999989: -0 */
CASE_D_D(trunc, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(trunc, 0xbff0000000000000, 0xbff0000000000000) /* -1: -1 */
CASE_D_D(trunc, 0x3ff0000000000001, 0x3ff0000000000000) /* 1.0000000000000002: 1 */
CASE_D_D(trunc, 0xbff0000000000001, 0xbff0000000000000) /* -1.0000000000000002: -1 */
CASE_D_D(trunc, 0x3ff7ffffffffffff, 0x3ff0000000000000) /* 1.4999999999999998: 1 */
CASE_D_D(trunc, 0xbff7ffffffffffff, 0xbff0000000000000) /* -1.4999999999999998: -1 */
CASE_D_D(trunc, 0x3ff8000000000000, 0x3ff0000000000000) /* 1.5: 1 */
CASE_D_D(trunc, 0xbff8000000000000, 0xbff0000000000000) /* -1.5: -1 */
CASE_D_D(trunc, 0x3ff8000000000001, 0x3ff0000000000000) /* 1.5000000000000002: 1 */
CASE_D_D(trunc, 0xbff8000000000001, 0xbff0000000000000) /* -1.5000000000000002: -1 */
CASE_D_D(trunc, 0x4000000000000000, 0x4000000000000000) /* 2: 2 */
CASE_D_D(trunc, 0xc000000000000000, 0xc000000000000000) /* -2: -2 */
CASE_D_D(trunc, 0x4003ffffffffffff, 0x4000000000000000) /* 2.4999999999999996: 2 */
CASE_D_D(trunc, 0xc003ffffffffffff, 0xc000000000000000) /* -2.4999999999999996: -2 */
CASE_D_D(trunc, 0x4004000000000000, 0x4000000000000000) /* 2.5: 2 */
CASE_D_D(trunc, 0xc004000000000000, 0xc000000000000000) /* -2.5: -2 */
CASE_D_D(trunc, 0x4004000000000001, 0x4000000000000000) /* 2.5000000000000004: 2 */
CASE_D_D(trunc, 0xc004000000000001, 0xc000000000000000) /* -2.5000000000000004: -2 */
CASE_D_D(trunc, 0x400c000000000000, 0x4008000000000000) /* 3.5: 3 */
CASE_D_D(trunc, 0xc00c000000000000, 0xc008000000000000) /* -3.5: -3 */
CASE_D_D(trunc, 0x4012000000000000, 0x4010000000000000) /* 4.5: 4 */
CASE_D_D(trunc, 0xc012000000000000, 0xc010000000000000) /* -4.5: -4 */
CASE_D_D(trunc, 0x4025000000000000, 0x4024000000000000) /* 10.5: 10 */
CASE_D_D(trunc, 0xc025000000000000, 0xc024000000000000) /* -10.5: -10 */
CASE_D_D(trunc, 0x4027000000000000, 0x4026000000000000) /* 11.5: 11 */
CASE_D_D(trunc, 0xc027000000000000, 0xc026000000000000) /* -11.5: -11 */
CASE_D_D(trunc, 0x431fffffffffffff, 0x431ffffffffffffc) /* 2251799813685247.8: 2251799813685247 */
CASE_D_D(trunc, 0xc31fffffffffffff, 0xc31ffffffffffffc) /* -2251799813685247.8: -2251799813685247 */
CASE_D_D(trunc, 0x4320000000000000, 0x4320000000000000) /* 2251799813685248: 2251799813685248 */
CASE_D_D(trunc, 0xc320000000000000, 0xc320000000000000) /* -2251799813685248: -2251799813685248 */
CASE_D_D(trunc, 0x4320000000000001, 0x4320000000000000) /* 2251799813685248.5: 2251799813685248 */
CASE_D_D(trunc, 0xc320000000000001, 0xc320000000000000) /* -2251799813685248.5: -2251799813685248 */
CASE_D_D(trunc, 0x4320000000000003, 0x4320000000000002) /* 2251799813685249.5: 2251799813685249 */
CASE_D_D(trunc, 0xc320000000000003, 0xc320000000000002) /* -2251799813685249.5: -2251799813685249 */
CASE_D_D(trunc, 0x432ffffffffffffd, 0x432ffffffffffffc) /* 4503599627370494.5: 4503599627370494 */
CASE_D_D(trunc, 0xc32ffffffffffffd, 0xc32ffffffffffffc) /* -4503599627370494.5: -4503599627370494 */
CASE_D_D(trunc, 0x432ffffffffffffe, 0x432ffffffffffffe) /* 4503599627370495: 4503599627370495 */
CASE_D_D(trunc, 0xc32ffffffffffffe, 0xc32ffffffffffffe) /* -4503599627370495: -4503599627370495 */
CASE_D_D(trunc, 0x432fffffffffffff, 0x432ffffffffffffe) /* 4503599627370495.5: 4503599627370495 */
CASE_D_D(trunc, 0xc32fffffffffffff, 0xc32ffffffffffffe) /* -4503599627370495.5: -4503599627370495 */
CASE_D_D(trunc, 0x4330000000000000, 0x4330000000000000) /* 4503599627370496: 4503599627370496 */
CASE_D_D(trunc, 0xc330000000000000, 0xc330000000000000) /* -4503599627370496: -4503599627370496 */
CASE_D_D(trunc, 0x4330000000000001, 0x4330000000000001) /* 4503599627370497: 4503599627370497 */
CASE_D_D(trunc, 0xc330000000000001, 0xc330000000000001) /* -4503599627370497: -4503599627370497 */
CASE_D_D(trunc, 0x433fffffffffffff, 0x433fffffffffffff) /* 9007199254740991: 9007199254740991 */
CASE_D_D(trunc, 0xc33fffffffffffff, 0xc33fffffffffffff) /* -9007199254740991: -9007199254740991 */
CASE_D_D(trunc, 0x4340000000000000, 0x4340000000000000) /* 9007199254740992: 9007199254740992 */
CASE_D_D(trunc, 0xc340000000000000, 0xc340000000000000) /* -9007199254740992: -9007199254740992 */
CASE_D_D(trunc, 0x4340000000000001, 0x4340000000000001) /* 9007199254740994: 9007199254740994 */
CASE_D_D(trunc, 0xc340000000000001, 0xc340000000000001) /* -9007199254740994: -9007199254740994 */
CASE_D_D(trunc, 0x43dfffffffffffff, 0x43dfffffffffffff) /* 9.2233720368547748e+18: 9.2233720368547748e+18 */
CASE_D_D(trunc, 0xc3dfffffffffffff, 0xc3dfffffffffffff) /* -9.2233720368547748e+18: -9.2233720368547748e+18 */
CASE_D_D(trunc, 0x43e0000000000000, 0x43e0000000000000) /* 9.2233720368547758e+18: 9.2233720368547758e+18 */
CASE_D_D(trunc, 0xc3e0000000000000, 0xc3e0000000000000) /* -9.2233720368547758e+18: -9.2233720368547758e+18 */
CASE_D_D(trunc, 0x43e0000000000001, 0x43e0000000000001) /* 9.2233720368547779e+18: 9.2233720368547779e+18 */
CASE_D_D(trunc, 0xc3e0000000000001, 0xc3e0000000000001) /* -9.2233720368547779e+18: -9.2233720368547779e+18 */
CASE_D_D(trunc, 0x7fe0000000000000, 0x7fe0000000000000) /* 8.9884656743115795e+307: 8.9884656743115795e+307 */
CASE_D_D(trunc, 0xffe0000000000000, 0xffe0000000000000) /* -8.9884656743115795e+307: -8.9884656743115795e+307 */
CASE_D_D(trunc, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_D(trunc, 0xffefffffffffffff, 0xffefffffffffffff) /* -1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_D(trunc, 0x400fffffffffffff, 0x4008000000000000) /* 3.9999999999999996: 3 */
CASE_D_D(trunc, 0xc00fffffffffffff, 0xc008000000000000) /* -3.9999999999999996: -3 */
CASE_D_D(trunc, 0x4010000000000000, 0x4010000000000000) /* 4: 4 */
CASE_D_D(trunc, 0xc010000000000000, 0xc010000000000000) /* -4: -4 */
CASE_D_D(trunc, 0x4010000000000001, 0x4010000000000000) /* 4.0000000000000009: 4 */
CASE_D_D(trunc, 0xc010000000000001, 0xc010000000000000) /* -4.0000000000000009: -4 */
CASE_D_D(trunc, 0x4022000000000000, 0x4022000000000000) /* 9: 9 */
CASE_D_D(trunc, 0xc022000000000000, 0xc022000000000000) /* -9: -9 */

/* round: double(double), NAN_QUIET */
CASE_D_D(round, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(round, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(round, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(round, 0xfff0000000000000, 0xfff0000000000000) /* -inf: -inf */
CASE_D_D(round, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(round, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(round, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(round, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(round, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(round, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(round, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(round, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(round, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(round, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(round, 0x0000000000000001, 0x0000000000000000) /* 4.9406564584124654e-324: 0 */
CASE_D_D(round, 0x8000000000000001, 0x8000000000000000) /* -4.9406564584124654e-324: -0 */
CASE_D_D(round, 0x0000000000000002, 0x0000000000000000) /* 9.8813129168249309e-324: 0 */
CASE_D_D(round, 0x8000000000000002, 0x8000000000000000) /* -9.8813129168249309e-324: -0 */
CASE_D_D(round, 0x0000000000000003, 0x0000000000000000) /* 1.4821969375237396e-323: 0 */
CASE_D_D(round, 0x8000000000000003, 0x8000000000000000) /* -1.4821969375237396e-323: -0 */
CASE_D_D(round, 0x0007ffffffffffff, 0x0000000000000000) /* 1.1125369292536002e-308: 0 */
CASE_D_D(round, 0x8007ffffffffffff, 0x8000000000000000) /* -1.1125369292536002e-308: -0 */
CASE_D_D(round, 0x0008000000000000, 0x0000000000000000) /* 1.1125369292536007e-308: 0 */
CASE_D_D(round, 0x8008000000000000, 0x8000000000000000) /* -1.1125369292536007e-308: -0 */
CASE_D_D(round, 0x000fffffffffffff, 0x0000000000000000) /* 2.2250738585072009e-308: 0 */
CASE_D_D(round, 0x800fffffffffffff, 0x8000000000000000) /* -2.2250738585072009e-308: -0 */
CASE_D_D(round, 0x0010000000000000, 0x0000000000000000) /* 2.2250738585072014e-308: 0 */
CASE_D_D(round, 0x8010000000000000, 0x8000000000000000) /* -2.2250738585072014e-308: -0 */
CASE_D_D(round, 0x0010000000000001, 0x0000000000000000) /* 2.2250738585072019e-308: 0 */
CASE_D_D(round, 0x8010000000000001, 0x8000000000000000) /* -2.2250738585072019e-308: -0 */
CASE_D_D(round, 0x3fd0000000000000, 0x0000000000000000) /* 0.25: 0 */
CASE_D_D(round, 0xbfd0000000000000, 0x8000000000000000) /* -0.25: -0 */
CASE_D_D(round, 0x3fdfffffffffffff, 0x0000000000000000) /* 0.49999999999999994: 0 */
CASE_D_D(round, 0xbfdfffffffffffff, 0x8000000000000000) /* -0.49999999999999994: -0 */
CASE_D_D(round, 0x3fe0000000000000, 0x3ff0000000000000) /* 0.5: 1 */
CASE_D_D(round, 0xbfe0000000000000, 0xbff0000000000000) /* -0.5: -1 */
CASE_D_D(round, 0x3fe0000000000001, 0x3ff0000000000000) /* 0.50000000000000011: 1 */
CASE_D_D(round, 0xbfe0000000000001, 0xbff0000000000000) /* -0.50000000000000011: -1 */
CASE_D_D(round, 0x3fe7ffffffffffff, 0x3ff0000000000000) /* 0.74999999999999989: 1 */
CASE_D_D(round, 0xbfe7ffffffffffff, 0xbff0000000000000) /* -0.74999999999999989: -1 */
CASE_D_D(round, 0x3fe8000000000000, 0x3ff0000000000000) /* 0.75: 1 */
CASE_D_D(round, 0xbfe8000000000000, 0xbff0000000000000) /* -0.75: -1 */
CASE_D_D(round, 0x3fefffffffffffff, 0x3ff0000000000000) /* 0.99999999999999989: 1 */
CASE_D_D(round, 0xbfefffffffffffff, 0xbff0000000000000) /* -0.99999999999999989: -1 */
CASE_D_D(round, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(round, 0xbff0000000000000, 0xbff0000000000000) /* -1: -1 */
CASE_D_D(round, 0x3ff0000000000001, 0x3ff0000000000000) /* 1.0000000000000002: 1 */
CASE_D_D(round, 0xbff0000000000001, 0xbff0000000000000) /* -1.0000000000000002: -1 */
CASE_D_D(round, 0x3ff7ffffffffffff, 0x3ff0000000000000) /* 1.4999999999999998: 1 */
CASE_D_D(round, 0xbff7ffffffffffff, 0xbff0000000000000) /* -1.4999999999999998: -1 */
CASE_D_D(round, 0x3ff8000000000000, 0x4000000000000000) /* 1.5: 2 */
CASE_D_D(round, 0xbff8000000000000, 0xc000000000000000) /* -1.5: -2 */
CASE_D_D(round, 0x3ff8000000000001, 0x4000000000000000) /* 1.5000000000000002: 2 */
CASE_D_D(round, 0xbff8000000000001, 0xc000000000000000) /* -1.5000000000000002: -2 */
CASE_D_D(round, 0x4000000000000000, 0x4000000000000000) /* 2: 2 */
CASE_D_D(round, 0xc000000000000000, 0xc000000000000000) /* -2: -2 */
CASE_D_D(round, 0x4003ffffffffffff, 0x4000000000000000) /* 2.4999999999999996: 2 */
CASE_D_D(round, 0xc003ffffffffffff, 0xc000000000000000) /* -2.4999999999999996: -2 */
CASE_D_D(round, 0x4004000000000000, 0x4008000000000000) /* 2.5: 3 */
CASE_D_D(round, 0xc004000000000000, 0xc008000000000000) /* -2.5: -3 */
CASE_D_D(round, 0x4004000000000001, 0x4008000000000000) /* 2.5000000000000004: 3 */
CASE_D_D(round, 0xc004000000000001, 0xc008000000000000) /* -2.5000000000000004: -3 */
CASE_D_D(round, 0x400c000000000000, 0x4010000000000000) /* 3.5: 4 */
CASE_D_D(round, 0xc00c000000000000, 0xc010000000000000) /* -3.5: -4 */
CASE_D_D(round, 0x4012000000000000, 0x4014000000000000) /* 4.5: 5 */
CASE_D_D(round, 0xc012000000000000, 0xc014000000000000) /* -4.5: -5 */
CASE_D_D(round, 0x4025000000000000, 0x4026000000000000) /* 10.5: 11 */
CASE_D_D(round, 0xc025000000000000, 0xc026000000000000) /* -10.5: -11 */
CASE_D_D(round, 0x4027000000000000, 0x4028000000000000) /* 11.5: 12 */
CASE_D_D(round, 0xc027000000000000, 0xc028000000000000) /* -11.5: -12 */
CASE_D_D(round, 0x431fffffffffffff, 0x4320000000000000) /* 2251799813685247.8: 2251799813685248 */
CASE_D_D(round, 0xc31fffffffffffff, 0xc320000000000000) /* -2251799813685247.8: -2251799813685248 */
CASE_D_D(round, 0x4320000000000000, 0x4320000000000000) /* 2251799813685248: 2251799813685248 */
CASE_D_D(round, 0xc320000000000000, 0xc320000000000000) /* -2251799813685248: -2251799813685248 */
CASE_D_D(round, 0x4320000000000001, 0x4320000000000002) /* 2251799813685248.5: 2251799813685249 */
CASE_D_D(round, 0xc320000000000001, 0xc320000000000002) /* -2251799813685248.5: -2251799813685249 */
CASE_D_D(round, 0x4320000000000003, 0x4320000000000004) /* 2251799813685249.5: 2251799813685250 */
CASE_D_D(round, 0xc320000000000003, 0xc320000000000004) /* -2251799813685249.5: -2251799813685250 */
CASE_D_D(round, 0x432ffffffffffffd, 0x432ffffffffffffe) /* 4503599627370494.5: 4503599627370495 */
CASE_D_D(round, 0xc32ffffffffffffd, 0xc32ffffffffffffe) /* -4503599627370494.5: -4503599627370495 */
CASE_D_D(round, 0x432ffffffffffffe, 0x432ffffffffffffe) /* 4503599627370495: 4503599627370495 */
CASE_D_D(round, 0xc32ffffffffffffe, 0xc32ffffffffffffe) /* -4503599627370495: -4503599627370495 */
CASE_D_D(round, 0x432fffffffffffff, 0x4330000000000000) /* 4503599627370495.5: 4503599627370496 */
CASE_D_D(round, 0xc32fffffffffffff, 0xc330000000000000) /* -4503599627370495.5: -4503599627370496 */
CASE_D_D(round, 0x4330000000000000, 0x4330000000000000) /* 4503599627370496: 4503599627370496 */
CASE_D_D(round, 0xc330000000000000, 0xc330000000000000) /* -4503599627370496: -4503599627370496 */
CASE_D_D(round, 0x4330000000000001, 0x4330000000000001) /* 4503599627370497: 4503599627370497 */
CASE_D_D(round, 0xc330000000000001, 0xc330000000000001) /* -4503599627370497: -4503599627370497 */
CASE_D_D(round, 0x433fffffffffffff, 0x433fffffffffffff) /* 9007199254740991: 9007199254740991 */
CASE_D_D(round, 0xc33fffffffffffff, 0xc33fffffffffffff) /* -9007199254740991: -9007199254740991 */
CASE_D_D(round, 0x4340000000000000, 0x4340000000000000) /* 9007199254740992: 9007199254740992 */
CASE_D_D(round, 0xc340000000000000, 0xc340000000000000) /* -9007199254740992: -9007199254740992 */
CASE_D_D(round, 0x4340000000000001, 0x4340000000000001) /* 9007199254740994: 9007199254740994 */
CASE_D_D(round, 0xc340000000000001, 0xc340000000000001) /* -9007199254740994: -9007199254740994 */
CASE_D_D(round, 0x43dfffffffffffff, 0x43dfffffffffffff) /* 9.2233720368547748e+18: 9.2233720368547748e+18 */
CASE_D_D(round, 0xc3dfffffffffffff, 0xc3dfffffffffffff) /* -9.2233720368547748e+18: -9.2233720368547748e+18 */
CASE_D_D(round, 0x43e0000000000000, 0x43e0000000000000) /* 9.2233720368547758e+18: 9.2233720368547758e+18 */
CASE_D_D(round, 0xc3e0000000000000, 0xc3e0000000000000) /* -9.2233720368547758e+18: -9.2233720368547758e+18 */
CASE_D_D(round, 0x43e0000000000001, 0x43e0000000000001) /* 9.2233720368547779e+18: 9.2233720368547779e+18 */
CASE_D_D(round, 0xc3e0000000000001, 0xc3e0000000000001) /* -9.2233720368547779e+18: -9.2233720368547779e+18 */
CASE_D_D(round, 0x7fe0000000000000, 0x7fe0000000000000) /* 8.9884656743115795e+307: 8.9884656743115795e+307 */
CASE_D_D(round, 0xffe0000000000000, 0xffe0000000000000) /* -8.9884656743115795e+307: -8.9884656743115795e+307 */
CASE_D_D(round, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_D(round, 0xffefffffffffffff, 0xffefffffffffffff) /* -1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_D(round, 0x400fffffffffffff, 0x4010000000000000) /* 3.9999999999999996: 4 */
CASE_D_D(round, 0xc00fffffffffffff, 0xc010000000000000) /* -3.9999999999999996: -4 */
CASE_D_D(round, 0x4010000000000000, 0x4010000000000000) /* 4: 4 */
CASE_D_D(round, 0xc010000000000000, 0xc010000000000000) /* -4: -4 */
CASE_D_D(round, 0x4010000000000001, 0x4010000000000000) /* 4.0000000000000009: 4 */
CASE_D_D(round, 0xc010000000000001, 0xc010000000000000) /* -4.0000000000000009: -4 */
CASE_D_D(round, 0x4022000000000000, 0x4022000000000000) /* 9: 9 */
CASE_D_D(round, 0xc022000000000000, 0xc022000000000000) /* -9: -9 */

/* rint: double(double), NAN_QUIET */
CASE_D_D(rint, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(rint, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(rint, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(rint, 0xfff0000000000000, 0xfff0000000000000) /* -inf: -inf */
CASE_D_D(rint, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(rint, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(rint, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(rint, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(rint, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(rint, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(rint, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(rint, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(rint, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(rint, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(rint, 0x0000000000000001, 0x0000000000000000) /* 4.9406564584124654e-324: 0 */
CASE_D_D(rint, 0x8000000000000001, 0x8000000000000000) /* -4.9406564584124654e-324: -0 */
CASE_D_D(rint, 0x0000000000000002, 0x0000000000000000) /* 9.8813129168249309e-324: 0 */
CASE_D_D(rint, 0x8000000000000002, 0x8000000000000000) /* -9.8813129168249309e-324: -0 */
CASE_D_D(rint, 0x0000000000000003, 0x0000000000000000) /* 1.4821969375237396e-323: 0 */
CASE_D_D(rint, 0x8000000000000003, 0x8000000000000000) /* -1.4821969375237396e-323: -0 */
CASE_D_D(rint, 0x0007ffffffffffff, 0x0000000000000000) /* 1.1125369292536002e-308: 0 */
CASE_D_D(rint, 0x8007ffffffffffff, 0x8000000000000000) /* -1.1125369292536002e-308: -0 */
CASE_D_D(rint, 0x0008000000000000, 0x0000000000000000) /* 1.1125369292536007e-308: 0 */
CASE_D_D(rint, 0x8008000000000000, 0x8000000000000000) /* -1.1125369292536007e-308: -0 */
CASE_D_D(rint, 0x000fffffffffffff, 0x0000000000000000) /* 2.2250738585072009e-308: 0 */
CASE_D_D(rint, 0x800fffffffffffff, 0x8000000000000000) /* -2.2250738585072009e-308: -0 */
CASE_D_D(rint, 0x0010000000000000, 0x0000000000000000) /* 2.2250738585072014e-308: 0 */
CASE_D_D(rint, 0x8010000000000000, 0x8000000000000000) /* -2.2250738585072014e-308: -0 */
CASE_D_D(rint, 0x0010000000000001, 0x0000000000000000) /* 2.2250738585072019e-308: 0 */
CASE_D_D(rint, 0x8010000000000001, 0x8000000000000000) /* -2.2250738585072019e-308: -0 */
CASE_D_D(rint, 0x3fd0000000000000, 0x0000000000000000) /* 0.25: 0 */
CASE_D_D(rint, 0xbfd0000000000000, 0x8000000000000000) /* -0.25: -0 */
CASE_D_D(rint, 0x3fdfffffffffffff, 0x0000000000000000) /* 0.49999999999999994: 0 */
CASE_D_D(rint, 0xbfdfffffffffffff, 0x8000000000000000) /* -0.49999999999999994: -0 */
CASE_D_D(rint, 0x3fe0000000000000, 0x0000000000000000) /* 0.5: 0 */
CASE_D_D(rint, 0xbfe0000000000000, 0x8000000000000000) /* -0.5: -0 */
CASE_D_D(rint, 0x3fe0000000000001, 0x3ff0000000000000) /* 0.50000000000000011: 1 */
CASE_D_D(rint, 0xbfe0000000000001, 0xbff0000000000000) /* -0.50000000000000011: -1 */
CASE_D_D(rint, 0x3fe7ffffffffffff, 0x3ff0000000000000) /* 0.74999999999999989: 1 */
CASE_D_D(rint, 0xbfe7ffffffffffff, 0xbff0000000000000) /* -0.74999999999999989: -1 */
CASE_D_D(rint, 0x3fe8000000000000, 0x3ff0000000000000) /* 0.75: 1 */
CASE_D_D(rint, 0xbfe8000000000000, 0xbff0000000000000) /* -0.75: -1 */
CASE_D_D(rint, 0x3fefffffffffffff, 0x3ff0000000000000) /* 0.99999999999999989: 1 */
CASE_D_D(rint, 0xbfefffffffffffff, 0xbff0000000000000) /* -0.99999999999999989: -1 */
CASE_D_D(rint, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(rint, 0xbff0000000000000, 0xbff0000000000000) /* -1: -1 */
CASE_D_D(rint, 0x3ff0000000000001, 0x3ff0000000000000) /* 1.0000000000000002: 1 */
CASE_D_D(rint, 0xbff0000000000001, 0xbff0000000000000) /* -1.0000000000000002: -1 */
CASE_D_D(rint, 0x3ff7ffffffffffff, 0x3ff0000000000000) /* 1.4999999999999998: 1 */
CASE_D_D(rint, 0xbff7ffffffffffff, 0xbff0000000000000) /* -1.4999999999999998: -1 */
CASE_D_D(rint, 0x3ff8000000000000, 0x4000000000000000) /* 1.5: 2 */
CASE_D_D(rint, 0xbff8000000000000, 0xc000000000000000) /* -1.5: -2 */
CASE_D_D(rint, 0x3ff8000000000001, 0x4000000000000000) /* 1.5000000000000002: 2 */
CASE_D_D(rint, 0xbff8000000000001, 0xc000000000000000) /* -1.5000000000000002: -2 */
CASE_D_D(rint, 0x4000000000000000, 0x4000000000000000) /* 2: 2 */
CASE_D_D(rint, 0xc000000000000000, 0xc000000000000000) /* -2: -2 */
CASE_D_D(rint, 0x4003ffffffffffff, 0x4000000000000000) /* 2.4999999999999996: 2 */
CASE_D_D(rint, 0xc003ffffffffffff, 0xc000000000000000) /* -2.4999999999999996: -2 */
CASE_D_D(rint, 0x4004000000000000, 0x4000000000000000) /* 2.5: 2 */
CASE_D_D(rint, 0xc004000000000000, 0xc000000000000000) /* -2.5: -2 */
CASE_D_D(rint, 0x4004000000000001, 0x4008000000000000) /* 2.5000000000000004: 3 */
CASE_D_D(rint, 0xc004000000000001, 0xc008000000000000) /* -2.5000000000000004: -3 */
CASE_D_D(rint, 0x400c000000000000, 0x4010000000000000) /* 3.5: 4 */
CASE_D_D(rint, 0xc00c000000000000, 0xc010000000000000) /* -3.5: -4 */
CASE_D_D(rint, 0x4012000000000000, 0x4010000000000000) /* 4.5: 4 */
CASE_D_D(rint, 0xc012000000000000, 0xc010000000000000) /* -4.5: -4 */
CASE_D_D(rint, 0x4025000000000000, 0x4024000000000000) /* 10.5: 10 */
CASE_D_D(rint, 0xc025000000000000, 0xc024000000000000) /* -10.5: -10 */
CASE_D_D(rint, 0x4027000000000000, 0x4028000000000000) /* 11.5: 12 */
CASE_D_D(rint, 0xc027000000000000, 0xc028000000000000) /* -11.5: -12 */
CASE_D_D(rint, 0x431fffffffffffff, 0x4320000000000000) /* 2251799813685247.8: 2251799813685248 */
CASE_D_D(rint, 0xc31fffffffffffff, 0xc320000000000000) /* -2251799813685247.8: -2251799813685248 */
CASE_D_D(rint, 0x4320000000000000, 0x4320000000000000) /* 2251799813685248: 2251799813685248 */
CASE_D_D(rint, 0xc320000000000000, 0xc320000000000000) /* -2251799813685248: -2251799813685248 */
CASE_D_D(rint, 0x4320000000000001, 0x4320000000000000) /* 2251799813685248.5: 2251799813685248 */
CASE_D_D(rint, 0xc320000000000001, 0xc320000000000000) /* -2251799813685248.5: -2251799813685248 */
CASE_D_D(rint, 0x4320000000000003, 0x4320000000000004) /* 2251799813685249.5: 2251799813685250 */
CASE_D_D(rint, 0xc320000000000003, 0xc320000000000004) /* -2251799813685249.5: -2251799813685250 */
CASE_D_D(rint, 0x432ffffffffffffd, 0x432ffffffffffffc) /* 4503599627370494.5: 4503599627370494 */
CASE_D_D(rint, 0xc32ffffffffffffd, 0xc32ffffffffffffc) /* -4503599627370494.5: -4503599627370494 */
CASE_D_D(rint, 0x432ffffffffffffe, 0x432ffffffffffffe) /* 4503599627370495: 4503599627370495 */
CASE_D_D(rint, 0xc32ffffffffffffe, 0xc32ffffffffffffe) /* -4503599627370495: -4503599627370495 */
CASE_D_D(rint, 0x432fffffffffffff, 0x4330000000000000) /* 4503599627370495.5: 4503599627370496 */
CASE_D_D(rint, 0xc32fffffffffffff, 0xc330000000000000) /* -4503599627370495.5: -4503599627370496 */
CASE_D_D(rint, 0x4330000000000000, 0x4330000000000000) /* 4503599627370496: 4503599627370496 */
CASE_D_D(rint, 0xc330000000000000, 0xc330000000000000) /* -4503599627370496: -4503599627370496 */
CASE_D_D(rint, 0x4330000000000001, 0x4330000000000001) /* 4503599627370497: 4503599627370497 */
CASE_D_D(rint, 0xc330000000000001, 0xc330000000000001) /* -4503599627370497: -4503599627370497 */
CASE_D_D(rint, 0x433fffffffffffff, 0x433fffffffffffff) /* 9007199254740991: 9007199254740991 */
CASE_D_D(rint, 0xc33fffffffffffff, 0xc33fffffffffffff) /* -9007199254740991: -9007199254740991 */
CASE_D_D(rint, 0x4340000000000000, 0x4340000000000000) /* 9007199254740992: 9007199254740992 */
CASE_D_D(rint, 0xc340000000000000, 0xc340000000000000) /* -9007199254740992: -9007199254740992 */
CASE_D_D(rint, 0x4340000000000001, 0x4340000000000001) /* 9007199254740994: 9007199254740994 */
CASE_D_D(rint, 0xc340000000000001, 0xc340000000000001) /* -9007199254740994: -9007199254740994 */
CASE_D_D(rint, 0x43dfffffffffffff, 0x43dfffffffffffff) /* 9.2233720368547748e+18: 9.2233720368547748e+18 */
CASE_D_D(rint, 0xc3dfffffffffffff, 0xc3dfffffffffffff) /* -9.2233720368547748e+18: -9.2233720368547748e+18 */
CASE_D_D(rint, 0x43e0000000000000, 0x43e0000000000000) /* 9.2233720368547758e+18: 9.2233720368547758e+18 */
CASE_D_D(rint, 0xc3e0000000000000, 0xc3e0000000000000) /* -9.2233720368547758e+18: -9.2233720368547758e+18 */
CASE_D_D(rint, 0x43e0000000000001, 0x43e0000000000001) /* 9.2233720368547779e+18: 9.2233720368547779e+18 */
CASE_D_D(rint, 0xc3e0000000000001, 0xc3e0000000000001) /* -9.2233720368547779e+18: -9.2233720368547779e+18 */
CASE_D_D(rint, 0x7fe0000000000000, 0x7fe0000000000000) /* 8.9884656743115795e+307: 8.9884656743115795e+307 */
CASE_D_D(rint, 0xffe0000000000000, 0xffe0000000000000) /* -8.9884656743115795e+307: -8.9884656743115795e+307 */
CASE_D_D(rint, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_D(rint, 0xffefffffffffffff, 0xffefffffffffffff) /* -1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_D(rint, 0x400fffffffffffff, 0x4010000000000000) /* 3.9999999999999996: 4 */
CASE_D_D(rint, 0xc00fffffffffffff, 0xc010000000000000) /* -3.9999999999999996: -4 */
CASE_D_D(rint, 0x4010000000000000, 0x4010000000000000) /* 4: 4 */
CASE_D_D(rint, 0xc010000000000000, 0xc010000000000000) /* -4: -4 */
CASE_D_D(rint, 0x4010000000000001, 0x4010000000000000) /* 4.0000000000000009: 4 */
CASE_D_D(rint, 0xc010000000000001, 0xc010000000000000) /* -4.0000000000000009: -4 */
CASE_D_D(rint, 0x4022000000000000, 0x4022000000000000) /* 9: 9 */
CASE_D_D(rint, 0xc022000000000000, 0xc022000000000000) /* -9: -9 */

/* nearbyint: double(double), NAN_QUIET */
CASE_D_D(nearbyint, 0x0000000000000000, 0x0000000000000000) /* 0: 0 */
CASE_D_D(nearbyint, 0x8000000000000000, 0x8000000000000000) /* -0: -0 */
CASE_D_D(nearbyint, 0x7ff0000000000000, 0x7ff0000000000000) /* inf: inf */
CASE_D_D(nearbyint, 0xfff0000000000000, 0xfff0000000000000) /* -inf: -inf */
CASE_D_D(nearbyint, 0x7ff8000000000000, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(nearbyint, 0xfff8000000000000, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(nearbyint, 0x7ff8000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(nearbyint, 0xfff8000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(nearbyint, 0x7fffffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(nearbyint, 0xffffffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(nearbyint, 0x7ff0000000000001, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(nearbyint, 0xfff0000000000001, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(nearbyint, 0x7ff7ffffffffffff, 0x7ff8000000000000) /* nan: nan */
CASE_D_D(nearbyint, 0xfff7ffffffffffff, 0x7ff8000000000000) /* -nan: nan */
CASE_D_D(nearbyint, 0x0000000000000001, 0x0000000000000000) /* 4.9406564584124654e-324: 0 */
CASE_D_D(nearbyint, 0x8000000000000001, 0x8000000000000000) /* -4.9406564584124654e-324: -0 */
CASE_D_D(nearbyint, 0x0000000000000002, 0x0000000000000000) /* 9.8813129168249309e-324: 0 */
CASE_D_D(nearbyint, 0x8000000000000002, 0x8000000000000000) /* -9.8813129168249309e-324: -0 */
CASE_D_D(nearbyint, 0x0000000000000003, 0x0000000000000000) /* 1.4821969375237396e-323: 0 */
CASE_D_D(nearbyint, 0x8000000000000003, 0x8000000000000000) /* -1.4821969375237396e-323: -0 */
CASE_D_D(nearbyint, 0x0007ffffffffffff, 0x0000000000000000) /* 1.1125369292536002e-308: 0 */
CASE_D_D(nearbyint, 0x8007ffffffffffff, 0x8000000000000000) /* -1.1125369292536002e-308: -0 */
CASE_D_D(nearbyint, 0x0008000000000000, 0x0000000000000000) /* 1.1125369292536007e-308: 0 */
CASE_D_D(nearbyint, 0x8008000000000000, 0x8000000000000000) /* -1.1125369292536007e-308: -0 */
CASE_D_D(nearbyint, 0x000fffffffffffff, 0x0000000000000000) /* 2.2250738585072009e-308: 0 */
CASE_D_D(nearbyint, 0x800fffffffffffff, 0x8000000000000000) /* -2.2250738585072009e-308: -0 */
CASE_D_D(nearbyint, 0x0010000000000000, 0x0000000000000000) /* 2.2250738585072014e-308: 0 */
CASE_D_D(nearbyint, 0x8010000000000000, 0x8000000000000000) /* -2.2250738585072014e-308: -0 */
CASE_D_D(nearbyint, 0x0010000000000001, 0x0000000000000000) /* 2.2250738585072019e-308: 0 */
CASE_D_D(nearbyint, 0x8010000000000001, 0x8000000000000000) /* -2.2250738585072019e-308: -0 */
CASE_D_D(nearbyint, 0x3fd0000000000000, 0x0000000000000000) /* 0.25: 0 */
CASE_D_D(nearbyint, 0xbfd0000000000000, 0x8000000000000000) /* -0.25: -0 */
CASE_D_D(nearbyint, 0x3fdfffffffffffff, 0x0000000000000000) /* 0.49999999999999994: 0 */
CASE_D_D(nearbyint, 0xbfdfffffffffffff, 0x8000000000000000) /* -0.49999999999999994: -0 */
CASE_D_D(nearbyint, 0x3fe0000000000000, 0x0000000000000000) /* 0.5: 0 */
CASE_D_D(nearbyint, 0xbfe0000000000000, 0x8000000000000000) /* -0.5: -0 */
CASE_D_D(nearbyint, 0x3fe0000000000001, 0x3ff0000000000000) /* 0.50000000000000011: 1 */
CASE_D_D(nearbyint, 0xbfe0000000000001, 0xbff0000000000000) /* -0.50000000000000011: -1 */
CASE_D_D(nearbyint, 0x3fe7ffffffffffff, 0x3ff0000000000000) /* 0.74999999999999989: 1 */
CASE_D_D(nearbyint, 0xbfe7ffffffffffff, 0xbff0000000000000) /* -0.74999999999999989: -1 */
CASE_D_D(nearbyint, 0x3fe8000000000000, 0x3ff0000000000000) /* 0.75: 1 */
CASE_D_D(nearbyint, 0xbfe8000000000000, 0xbff0000000000000) /* -0.75: -1 */
CASE_D_D(nearbyint, 0x3fefffffffffffff, 0x3ff0000000000000) /* 0.99999999999999989: 1 */
CASE_D_D(nearbyint, 0xbfefffffffffffff, 0xbff0000000000000) /* -0.99999999999999989: -1 */
CASE_D_D(nearbyint, 0x3ff0000000000000, 0x3ff0000000000000) /* 1: 1 */
CASE_D_D(nearbyint, 0xbff0000000000000, 0xbff0000000000000) /* -1: -1 */
CASE_D_D(nearbyint, 0x3ff0000000000001, 0x3ff0000000000000) /* 1.0000000000000002: 1 */
CASE_D_D(nearbyint, 0xbff0000000000001, 0xbff0000000000000) /* -1.0000000000000002: -1 */
CASE_D_D(nearbyint, 0x3ff7ffffffffffff, 0x3ff0000000000000) /* 1.4999999999999998: 1 */
CASE_D_D(nearbyint, 0xbff7ffffffffffff, 0xbff0000000000000) /* -1.4999999999999998: -1 */
CASE_D_D(nearbyint, 0x3ff8000000000000, 0x4000000000000000) /* 1.5: 2 */
CASE_D_D(nearbyint, 0xbff8000000000000, 0xc000000000000000) /* -1.5: -2 */
CASE_D_D(nearbyint, 0x3ff8000000000001, 0x4000000000000000) /* 1.5000000000000002: 2 */
CASE_D_D(nearbyint, 0xbff8000000000001, 0xc000000000000000) /* -1.5000000000000002: -2 */
CASE_D_D(nearbyint, 0x4000000000000000, 0x4000000000000000) /* 2: 2 */
CASE_D_D(nearbyint, 0xc000000000000000, 0xc000000000000000) /* -2: -2 */
CASE_D_D(nearbyint, 0x4003ffffffffffff, 0x4000000000000000) /* 2.4999999999999996: 2 */
CASE_D_D(nearbyint, 0xc003ffffffffffff, 0xc000000000000000) /* -2.4999999999999996: -2 */
CASE_D_D(nearbyint, 0x4004000000000000, 0x4000000000000000) /* 2.5: 2 */
CASE_D_D(nearbyint, 0xc004000000000000, 0xc000000000000000) /* -2.5: -2 */
CASE_D_D(nearbyint, 0x4004000000000001, 0x4008000000000000) /* 2.5000000000000004: 3 */
CASE_D_D(nearbyint, 0xc004000000000001, 0xc008000000000000) /* -2.5000000000000004: -3 */
CASE_D_D(nearbyint, 0x400c000000000000, 0x4010000000000000) /* 3.5: 4 */
CASE_D_D(nearbyint, 0xc00c000000000000, 0xc010000000000000) /* -3.5: -4 */
CASE_D_D(nearbyint, 0x4012000000000000, 0x4010000000000000) /* 4.5: 4 */
CASE_D_D(nearbyint, 0xc012000000000000, 0xc010000000000000) /* -4.5: -4 */
CASE_D_D(nearbyint, 0x4025000000000000, 0x4024000000000000) /* 10.5: 10 */
CASE_D_D(nearbyint, 0xc025000000000000, 0xc024000000000000) /* -10.5: -10 */
CASE_D_D(nearbyint, 0x4027000000000000, 0x4028000000000000) /* 11.5: 12 */
CASE_D_D(nearbyint, 0xc027000000000000, 0xc028000000000000) /* -11.5: -12 */
CASE_D_D(nearbyint, 0x431fffffffffffff, 0x4320000000000000) /* 2251799813685247.8: 2251799813685248 */
CASE_D_D(nearbyint, 0xc31fffffffffffff, 0xc320000000000000) /* -2251799813685247.8: -2251799813685248 */
CASE_D_D(nearbyint, 0x4320000000000000, 0x4320000000000000) /* 2251799813685248: 2251799813685248 */
CASE_D_D(nearbyint, 0xc320000000000000, 0xc320000000000000) /* -2251799813685248: -2251799813685248 */
CASE_D_D(nearbyint, 0x4320000000000001, 0x4320000000000000) /* 2251799813685248.5: 2251799813685248 */
CASE_D_D(nearbyint, 0xc320000000000001, 0xc320000000000000) /* -2251799813685248.5: -2251799813685248 */
CASE_D_D(nearbyint, 0x4320000000000003, 0x4320000000000004) /* 2251799813685249.5: 2251799813685250 */
CASE_D_D(nearbyint, 0xc320000000000003, 0xc320000000000004) /* -2251799813685249.5: -2251799813685250 */
CASE_D_D(nearbyint, 0x432ffffffffffffd, 0x432ffffffffffffc) /* 4503599627370494.5: 4503599627370494 */
CASE_D_D(nearbyint, 0xc32ffffffffffffd, 0xc32ffffffffffffc) /* -4503599627370494.5: -4503599627370494 */
CASE_D_D(nearbyint, 0x432ffffffffffffe, 0x432ffffffffffffe) /* 4503599627370495: 4503599627370495 */
CASE_D_D(nearbyint, 0xc32ffffffffffffe, 0xc32ffffffffffffe) /* -4503599627370495: -4503599627370495 */
CASE_D_D(nearbyint, 0x432fffffffffffff, 0x4330000000000000) /* 4503599627370495.5: 4503599627370496 */
CASE_D_D(nearbyint, 0xc32fffffffffffff, 0xc330000000000000) /* -4503599627370495.5: -4503599627370496 */
CASE_D_D(nearbyint, 0x4330000000000000, 0x4330000000000000) /* 4503599627370496: 4503599627370496 */
CASE_D_D(nearbyint, 0xc330000000000000, 0xc330000000000000) /* -4503599627370496: -4503599627370496 */
CASE_D_D(nearbyint, 0x4330000000000001, 0x4330000000000001) /* 4503599627370497: 4503599627370497 */
CASE_D_D(nearbyint, 0xc330000000000001, 0xc330000000000001) /* -4503599627370497: -4503599627370497 */
CASE_D_D(nearbyint, 0x433fffffffffffff, 0x433fffffffffffff) /* 9007199254740991: 9007199254740991 */
CASE_D_D(nearbyint, 0xc33fffffffffffff, 0xc33fffffffffffff) /* -9007199254740991: -9007199254740991 */
CASE_D_D(nearbyint, 0x4340000000000000, 0x4340000000000000) /* 9007199254740992: 9007199254740992 */
CASE_D_D(nearbyint, 0xc340000000000000, 0xc340000000000000) /* -9007199254740992: -9007199254740992 */
CASE_D_D(nearbyint, 0x4340000000000001, 0x4340000000000001) /* 9007199254740994: 9007199254740994 */
CASE_D_D(nearbyint, 0xc340000000000001, 0xc340000000000001) /* -9007199254740994: -9007199254740994 */
CASE_D_D(nearbyint, 0x43dfffffffffffff, 0x43dfffffffffffff) /* 9.2233720368547748e+18: 9.2233720368547748e+18 */
CASE_D_D(nearbyint, 0xc3dfffffffffffff, 0xc3dfffffffffffff) /* -9.2233720368547748e+18: -9.2233720368547748e+18 */
CASE_D_D(nearbyint, 0x43e0000000000000, 0x43e0000000000000) /* 9.2233720368547758e+18: 9.2233720368547758e+18 */
CASE_D_D(nearbyint, 0xc3e0000000000000, 0xc3e0000000000000) /* -9.2233720368547758e+18: -9.2233720368547758e+18 */
CASE_D_D(nearbyint, 0x43e0000000000001, 0x43e0000000000001) /* 9.2233720368547779e+18: 9.2233720368547779e+18 */
CASE_D_D(nearbyint, 0xc3e0000000000001, 0xc3e0000000000001) /* -9.2233720368547779e+18: -9.2233720368547779e+18 */
CASE_D_D(nearbyint, 0x7fe0000000000000, 0x7fe0000000000000) /* 8.9884656743115795e+307: 8.9884656743115795e+307 */
CASE_D_D(nearbyint, 0xffe0000000000000, 0xffe0000000000000) /* -8.9884656743115795e+307: -8.9884656743115795e+307 */
CASE_D_D(nearbyint, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_D(nearbyint, 0xffefffffffffffff, 0xffefffffffffffff) /* -1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_D(nearbyint, 0x400fffffffffffff, 0x4010000000000000) /* 3.9999999999999996: 4 */
CASE_D_D(nearbyint, 0xc00fffffffffffff, 0xc010000000000000) /* -3.9999999999999996: -4 */
CASE_D_D(nearbyint, 0x4010000000000000, 0x4010000000000000) /* 4: 4 */
CASE_D_D(nearbyint, 0xc010000000000000, 0xc010000000000000) /* -4: -4 */
CASE_D_D(nearbyint, 0x4010000000000001, 0x4010000000000000) /* 4.0000000000000009: 4 */
CASE_D_D(nearbyint, 0xc010000000000001, 0xc010000000000000) /* -4.0000000000000009: -4 */
CASE_D_D(nearbyint, 0x4022000000000000, 0x4022000000000000) /* 9: 9 */
CASE_D_D(nearbyint, 0xc022000000000000, 0xc022000000000000) /* -9: -9 */

/* fmin: double(double, double), MIN */
CASE_D_DD(fmin, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000) /* 0, 0: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0x8000000000000000, 0x8000000000000000) /* 0, -0: -0 */
CASE_D_DD(fmin, 0x0000000000000000, 0x0000000000000001, 0x0000000000000000) /* 0, 4.9406564584124654e-324: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0x3ff0000000000000, 0x0000000000000000) /* 0, 1: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0xbff0000000000000, 0xbff0000000000000) /* 0, -1: -1 */
CASE_D_DD(fmin, 0x0000000000000000, 0x3ff8000000000000, 0x0000000000000000) /* 0, 1.5: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0x7fefffffffffffff, 0x0000000000000000) /* 0, 1.7976931348623157e+308: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0x7ff0000000000000, 0x0000000000000000) /* 0, inf: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0xfff0000000000000, 0xfff0000000000000) /* 0, -inf: -inf */
CASE_D_DD(fmin, 0x0000000000000000, 0x7ff8000000000000, 0x0000000000000000) /* 0, nan: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0xfff8000000000000, 0x0000000000000000) /* 0, -nan: 0 */
CASE_D_DD(fmin, 0x0000000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* 0, nan: nan */
CASE_D_DD(fmin, 0x8000000000000000, 0x0000000000000000, 0x8000000000000000) /* -0, 0: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0x8000000000000000, 0x8000000000000000) /* -0, -0: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0x0000000000000001, 0x8000000000000000) /* -0, 4.9406564584124654e-324: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0x3ff0000000000000, 0x8000000000000000) /* -0, 1: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0xbff0000000000000, 0xbff0000000000000) /* -0, -1: -1 */
CASE_D_DD(fmin, 0x8000000000000000, 0x3ff8000000000000, 0x8000000000000000) /* -0, 1.5: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0x7fefffffffffffff, 0x8000000000000000) /* -0, 1.7976931348623157e+308: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0x7ff0000000000000, 0x8000000000000000) /* -0, inf: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0xfff0000000000000, 0xfff0000000000000) /* -0, -inf: -inf */
CASE_D_DD(fmin, 0x8000000000000000, 0x7ff8000000000000, 0x8000000000000000) /* -0, nan: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0xfff8000000000000, 0x8000000000000000) /* -0, -nan: -0 */
CASE_D_DD(fmin, 0x8000000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -0, nan: nan */
CASE_D_DD(fmin, 0x0000000000000001, 0x0000000000000000, 0x0000000000000000) /* 4.9406564584124654e-324, 0: 0 */
CASE_D_DD(fmin, 0x0000000000000001, 0x8000000000000000, 0x8000000000000000) /* 4.9406564584124654e-324, -0: -0 */
CASE_D_DD(fmin, 0x0000000000000001, 0x0000000000000001, 0x0000000000000001) /* 4.9406564584124654e-324, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0x3ff0000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, 1: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0xbff0000000000000, 0xbff0000000000000) /* 4.9406564584124654e-324, -1: -1 */
CASE_D_DD(fmin, 0x0000000000000001, 0x3ff8000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, 1.5: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0x7fefffffffffffff, 0x0000000000000001) /* 4.9406564584124654e-324, 1.7976931348623157e+308: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0x7ff0000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, inf: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0xfff0000000000000, 0xfff0000000000000) /* 4.9406564584124654e-324, -inf: -inf */
CASE_D_DD(fmin, 0x0000000000000001, 0x7ff8000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, nan: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0xfff8000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, -nan: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x0000000000000001, 0x7ff4000000000000, 0x7ff8000000000000) /* 4.9406564584124654e-324, nan: nan */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x0000000000000000, 0x0000000000000000) /* 1, 0: 0 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x8000000000000000, 0x8000000000000000) /* 1, -0: -0 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x0000000000000001, 0x0000000000000001) /* 1, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* 1, 1: 1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0xbff0000000000000, 0xbff0000000000000) /* 1, -1: -1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x3ff8000000000000, 0x3ff0000000000000) /* 1, 1.5: 1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x7fefffffffffffff, 0x3ff0000000000000) /* 1, 1.7976931348623157e+308: 1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x7ff0000000000000, 0x3ff0000000000000) /* 1, inf: 1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0xfff0000000000000, 0xfff0000000000000) /* 1, -inf: -inf */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x7ff8000000000000, 0x3ff0000000000000) /* 1, nan: 1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0xfff8000000000000, 0x3ff0000000000000) /* 1, -nan: 1 */
CASE_D_DD(fmin, 0x3ff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* 1, nan: nan */
CASE_D_DD(fmin, 0xbff0000000000000, 0x0000000000000000, 0xbff0000000000000) /* -1, 0: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x8000000000000000, 0xbff0000000000000) /* -1, -0: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x0000000000000001, 0xbff0000000000000) /* -1, 4.9406564584124654e-324: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x3ff0000000000000, 0xbff0000000000000) /* -1, 1: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0xbff0000000000000, 0xbff0000000000000) /* -1, -1: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x3ff8000000000000, 0xbff0000000000000) /* -1, 1.5: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x7fefffffffffffff, 0xbff0000000000000) /* -1, 1.7976931348623157e+308: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x7ff0000000000000, 0xbff0000000000000) /* -1, inf: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0xfff0000000000000, 0xfff0000000000000) /* -1, -inf: -inf */
CASE_D_DD(fmin, 0xbff0000000000000, 0x7ff8000000000000, 0xbff0000000000000) /* -1, nan: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0xfff8000000000000, 0xbff0000000000000) /* -1, -nan: -1 */
CASE_D_DD(fmin, 0xbff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -1, nan: nan */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x0000000000000000, 0x0000000000000000) /* 1.5, 0: 0 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x8000000000000000, 0x8000000000000000) /* 1.5, -0: -0 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x0000000000000001, 0x0000000000000001) /* 1.5, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* 1.5, 1: 1 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0xbff0000000000000, 0xbff0000000000000) /* 1.5, -1: -1 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* 1.5, 1.5: 1.5 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x7fefffffffffffff, 0x3ff8000000000000) /* 1.5, 1.7976931348623157e+308: 1.5 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x7ff0000000000000, 0x3ff8000000000000) /* 1.5, inf: 1.5 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0xfff0000000000000, 0xfff0000000000000) /* 1.5, -inf: -inf */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x7ff8000000000000, 0x3ff8000000000000) /* 1.5, nan: 1.5 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0xfff8000000000000, 0x3ff8000000000000) /* 1.5, -nan: 1.5 */
CASE_D_DD(fmin, 0x3ff8000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* 1.5, nan: nan */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x0000000000000000, 0x0000000000000000) /* 1.7976931348623157e+308, 0: 0 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x8000000000000000, 0x8000000000000000) /* 1.7976931348623157e+308, -0: -0 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x0000000000000001, 0x0000000000000001) /* 1.7976931348623157e+308, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x3ff0000000000000, 0x3ff0000000000000) /* 1.7976931348623157e+308, 1: 1 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0xbff0000000000000, 0xbff0000000000000) /* 1.7976931348623157e+308, -1: -1 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x3ff8000000000000, 0x3ff8000000000000) /* 1.7976931348623157e+308, 1.5: 1.5 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x7ff0000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, inf: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0xfff0000000000000, 0xfff0000000000000) /* 1.7976931348623157e+308, -inf: -inf */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x7ff8000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, nan: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0xfff8000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, -nan: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0x7fefffffffffffff, 0x7ff4000000000000, 0x7ff8000000000000) /* 1.7976931348623157e+308, nan: nan */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x0000000000000000, 0x0000000000000000) /* inf, 0: 0 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x8000000000000000, 0x8000000000000000) /* inf, -0: -0 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x0000000000000001, 0x0000000000000001) /* inf, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* inf, 1: 1 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0xbff0000000000000, 0xbff0000000000000) /* inf, -1: -1 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* inf, 1.5: 1.5 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* inf, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* inf, inf: inf */
CASE_D_DD(fmin, 0x7ff0000000000000, 0xfff0000000000000, 0xfff0000000000000) /* inf, -inf: -inf */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x7ff8000000000000, 0x7ff0000000000000) /* inf, nan: inf */
CASE_D_DD(fmin, 0x7ff0000000000000, 0xfff8000000000000, 0x7ff0000000000000) /* inf, -nan: inf */
CASE_D_DD(fmin, 0x7ff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* inf, nan: nan */
CASE_D_DD(fmin, 0xfff0000000000000, 0x0000000000000000, 0xfff0000000000000) /* -inf, 0: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x8000000000000000, 0xfff0000000000000) /* -inf, -0: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x0000000000000001, 0xfff0000000000000) /* -inf, 4.9406564584124654e-324: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x3ff0000000000000, 0xfff0000000000000) /* -inf, 1: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0xbff0000000000000, 0xfff0000000000000) /* -inf, -1: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x3ff8000000000000, 0xfff0000000000000) /* -inf, 1.5: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x7fefffffffffffff, 0xfff0000000000000) /* -inf, 1.7976931348623157e+308: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x7ff0000000000000, 0xfff0000000000000) /* -inf, inf: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0xfff0000000000000, 0xfff0000000000000) /* -inf, -inf: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x7ff8000000000000, 0xfff0000000000000) /* -inf, nan: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0xfff8000000000000, 0xfff0000000000000) /* -inf, -nan: -inf */
CASE_D_DD(fmin, 0xfff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -inf, nan: nan */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x0000000000000000, 0x0000000000000000) /* nan, 0: 0 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x8000000000000000, 0x8000000000000000) /* nan, -0: -0 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x0000000000000001, 0x0000000000000001) /* nan, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* nan, 1: 1 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0xbff0000000000000, 0xbff0000000000000) /* nan, -1: -1 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* nan, 1.5: 1.5 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* nan, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* nan, inf: inf */
CASE_D_DD(fmin, 0x7ff8000000000000, 0xfff0000000000000, 0xfff0000000000000) /* nan, -inf: -inf */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* nan, nan: nan */
CASE_D_DD(fmin, 0x7ff8000000000000, 0xfff8000000000000, 0x7ff8000000000000) /* nan, -nan: nan */
CASE_D_DD(fmin, 0x7ff8000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* nan, nan: nan */
CASE_D_DD(fmin, 0xfff8000000000000, 0x0000000000000000, 0x0000000000000000) /* -nan, 0: 0 */
CASE_D_DD(fmin, 0xfff8000000000000, 0x8000000000000000, 0x8000000000000000) /* -nan, -0: -0 */
CASE_D_DD(fmin, 0xfff8000000000000, 0x0000000000000001, 0x0000000000000001) /* -nan, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmin, 0xfff8000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* -nan, 1: 1 */
CASE_D_DD(fmin, 0xfff8000000000000, 0xbff0000000000000, 0xbff0000000000000) /* -nan, -1: -1 */
CASE_D_DD(fmin, 0xfff8000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* -nan, 1.5: 1.5 */
CASE_D_DD(fmin, 0xfff8000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* -nan, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmin, 0xfff8000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* -nan, inf: inf */
CASE_D_DD(fmin, 0xfff8000000000000, 0xfff0000000000000, 0xfff0000000000000) /* -nan, -inf: -inf */
CASE_D_DD(fmin, 0xfff8000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* -nan, nan: nan */
CASE_D_DD(fmin, 0xfff8000000000000, 0xfff8000000000000, 0x7ff8000000000000) /* -nan, -nan: nan */
CASE_D_DD(fmin, 0xfff8000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -nan, nan: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x0000000000000000, 0x7ff8000000000000) /* nan, 0: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x8000000000000000, 0x7ff8000000000000) /* nan, -0: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x0000000000000001, 0x7ff8000000000000) /* nan, 4.9406564584124654e-324: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x3ff0000000000000, 0x7ff8000000000000) /* nan, 1: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0xbff0000000000000, 0x7ff8000000000000) /* nan, -1: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x3ff8000000000000, 0x7ff8000000000000) /* nan, 1.5: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x7fefffffffffffff, 0x7ff8000000000000) /* nan, 1.7976931348623157e+308: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x7ff0000000000000, 0x7ff8000000000000) /* nan, inf: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0xfff0000000000000, 0x7ff8000000000000) /* nan, -inf: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* nan, nan: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0xfff8000000000000, 0x7ff8000000000000) /* nan, -nan: nan */
CASE_D_DD(fmin, 0x7ff4000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* nan, nan: nan */

/* fmax: double(double, double), MAX */
CASE_D_DD(fmax, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000) /* 0, 0: 0 */
CASE_D_DD(fmax, 0x0000000000000000, 0x8000000000000000, 0x0000000000000000) /* 0, -0: 0 */
CASE_D_DD(fmax, 0x0000000000000000, 0x0000000000000001, 0x0000000000000001) /* 0, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* 0, 1: 1 */
CASE_D_DD(fmax, 0x0000000000000000, 0xbff0000000000000, 0x0000000000000000) /* 0, -1: 0 */
CASE_D_DD(fmax, 0x0000000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* 0, 1.5: 1.5 */
CASE_D_DD(fmax, 0x0000000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 0, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x0000000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* 0, inf: inf */
CASE_D_DD(fmax, 0x0000000000000000, 0xfff0000000000000, 0x0000000000000000) /* 0, -inf: 0 */
CASE_D_DD(fmax, 0x0000000000000000, 0x7ff8000000000000, 0x0000000000000000) /* 0, nan: 0 */
CASE_D_DD(fmax, 0x0000000000000000, 0xfff8000000000000, 0x0000000000000000) /* 0, -nan: 0 */
CASE_D_DD(fmax, 0x0000000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* 0, nan: nan */
CASE_D_DD(fmax, 0x8000000000000000, 0x0000000000000000, 0x0000000000000000) /* -0, 0: 0 */
CASE_D_DD(fmax, 0x8000000000000000, 0x8000000000000000, 0x8000000000000000) /* -0, -0: -0 */
CASE_D_DD(fmax, 0x8000000000000000, 0x0000000000000001, 0x0000000000000001) /* -0, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x8000000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* -0, 1: 1 */
CASE_D_DD(fmax, 0x8000000000000000, 0xbff0000000000000, 0x8000000000000000) /* -0, -1: -0 */
CASE_D_DD(fmax, 0x8000000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* -0, 1.5: 1.5 */
CASE_D_DD(fmax, 0x8000000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* -0, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x8000000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* -0, inf: inf */
CASE_D_DD(fmax, 0x8000000000000000, 0xfff0000000000000, 0x8000000000000000) /* -0, -inf: -0 */
CASE_D_DD(fmax, 0x8000000000000000, 0x7ff8000000000000, 0x8000000000000000) /* -0, nan: -0 */
CASE_D_DD(fmax, 0x8000000000000000, 0xfff8000000000000, 0x8000000000000000) /* -0, -nan: -0 */
CASE_D_DD(fmax, 0x8000000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -0, nan: nan */
CASE_D_DD(fmax, 0x0000000000000001, 0x0000000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, 0: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0x8000000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, -0: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0x0000000000000001, 0x0000000000000001) /* 4.9406564584124654e-324, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0x3ff0000000000000, 0x3ff0000000000000) /* 4.9406564584124654e-324, 1: 1 */
CASE_D_DD(fmax, 0x0000000000000001, 0xbff0000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, -1: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0x3ff8000000000000, 0x3ff8000000000000) /* 4.9406564584124654e-324, 1.5: 1.5 */
CASE_D_DD(fmax, 0x0000000000000001, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 4.9406564584124654e-324, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x0000000000000001, 0x7ff0000000000000, 0x7ff0000000000000) /* 4.9406564584124654e-324, inf: inf */
CASE_D_DD(fmax, 0x0000000000000001, 0xfff0000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, -inf: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0x7ff8000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, nan: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0xfff8000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, -nan: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x0000000000000001, 0x7ff4000000000000, 0x7ff8000000000000) /* 4.9406564584124654e-324, nan: nan */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x0000000000000000, 0x3ff0000000000000) /* 1, 0: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x8000000000000000, 0x3ff0000000000000) /* 1, -0: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x0000000000000001, 0x3ff0000000000000) /* 1, 4.9406564584124654e-324: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* 1, 1: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0xbff0000000000000, 0x3ff0000000000000) /* 1, -1: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* 1, 1.5: 1.5 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* 1, inf: inf */
CASE_D_DD(fmax, 0x3ff0000000000000, 0xfff0000000000000, 0x3ff0000000000000) /* 1, -inf: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x7ff8000000000000, 0x3ff0000000000000) /* 1, nan: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0xfff8000000000000, 0x3ff0000000000000) /* 1, -nan: 1 */
CASE_D_DD(fmax, 0x3ff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* 1, nan: nan */
CASE_D_DD(fmax, 0xbff0000000000000, 0x0000000000000000, 0x0000000000000000) /* -1, 0: 0 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x8000000000000000, 0x8000000000000000) /* -1, -0: -0 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x0000000000000001, 0x0000000000000001) /* -1, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* -1, 1: 1 */
CASE_D_DD(fmax, 0xbff0000000000000, 0xbff0000000000000, 0xbff0000000000000) /* -1, -1: -1 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* -1, 1.5: 1.5 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* -1, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* -1, inf: inf */
CASE_D_DD(fmax, 0xbff0000000000000, 0xfff0000000000000, 0xbff0000000000000) /* -1, -inf: -1 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x7ff8000000000000, 0xbff0000000000000) /* -1, nan: -1 */
CASE_D_DD(fmax, 0xbff0000000000000, 0xfff8000000000000, 0xbff0000000000000) /* -1, -nan: -1 */
CASE_D_DD(fmax, 0xbff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -1, nan: nan */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x0000000000000000, 0x3ff8000000000000) /* 1.5, 0: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x8000000000000000, 0x3ff8000000000000) /* 1.5, -0: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x0000000000000001, 0x3ff8000000000000) /* 1.5, 4.9406564584124654e-324: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x3ff0000000000000, 0x3ff8000000000000) /* 1.5, 1: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0xbff0000000000000, 0x3ff8000000000000) /* 1.5, -1: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* 1.5, 1.5: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.5, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* 1.5, inf: inf */
CASE_D_DD(fmax, 0x3ff8000000000000, 0xfff0000000000000, 0x3ff8000000000000) /* 1.5, -inf: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x7ff8000000000000, 0x3ff8000000000000) /* 1.5, nan: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0xfff8000000000000, 0x3ff8000000000000) /* 1.5, -nan: 1.5 */
CASE_D_DD(fmax, 0x3ff8000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* 1.5, nan: nan */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x0000000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 0: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x8000000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, -0: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x0000000000000001, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 4.9406564584124654e-324: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x3ff0000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 1: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0xbff0000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, -1: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x3ff8000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 1.5: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x7fefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x7ff0000000000000, 0x7ff0000000000000) /* 1.7976931348623157e+308, inf: inf */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0xfff0000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, -inf: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x7ff8000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, nan: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0xfff8000000000000, 0x7fefffffffffffff) /* 1.7976931348623157e+308, -nan: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7fefffffffffffff, 0x7ff4000000000000, 0x7ff8000000000000) /* 1.7976931348623157e+308, nan: nan */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x0000000000000000, 0x7ff0000000000000) /* inf, 0: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x8000000000000000, 0x7ff0000000000000) /* inf, -0: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x0000000000000001, 0x7ff0000000000000) /* inf, 4.9406564584124654e-324: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x3ff0000000000000, 0x7ff0000000000000) /* inf, 1: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0xbff0000000000000, 0x7ff0000000000000) /* inf, -1: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x3ff8000000000000, 0x7ff0000000000000) /* inf, 1.5: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x7fefffffffffffff, 0x7ff0000000000000) /* inf, 1.7976931348623157e+308: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* inf, inf: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0xfff0000000000000, 0x7ff0000000000000) /* inf, -inf: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x7ff8000000000000, 0x7ff0000000000000) /* inf, nan: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0xfff8000000000000, 0x7ff0000000000000) /* inf, -nan: inf */
CASE_D_DD(fmax, 0x7ff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* inf, nan: nan */
CASE_D_DD(fmax, 0xfff0000000000000, 0x0000000000000000, 0x0000000000000000) /* -inf, 0: 0 */
CASE_D_DD(fmax, 0xfff0000000000000, 0x8000000000000000, 0x8000000000000000) /* -inf, -0: -0 */
CASE_D_DD(fmax, 0xfff0000000000000, 0x0000000000000001, 0x0000000000000001) /* -inf, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0xfff0000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* -inf, 1: 1 */
CASE_D_DD(fmax, 0xfff0000000000000, 0xbff0000000000000, 0xbff0000000000000) /* -inf, -1: -1 */
CASE_D_DD(fmax, 0xfff0000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* -inf, 1.5: 1.5 */
CASE_D_DD(fmax, 0xfff0000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* -inf, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0xfff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* -inf, inf: inf */
CASE_D_DD(fmax, 0xfff0000000000000, 0xfff0000000000000, 0xfff0000000000000) /* -inf, -inf: -inf */
CASE_D_DD(fmax, 0xfff0000000000000, 0x7ff8000000000000, 0xfff0000000000000) /* -inf, nan: -inf */
CASE_D_DD(fmax, 0xfff0000000000000, 0xfff8000000000000, 0xfff0000000000000) /* -inf, -nan: -inf */
CASE_D_DD(fmax, 0xfff0000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -inf, nan: nan */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x0000000000000000, 0x0000000000000000) /* nan, 0: 0 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x8000000000000000, 0x8000000000000000) /* nan, -0: -0 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x0000000000000001, 0x0000000000000001) /* nan, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* nan, 1: 1 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0xbff0000000000000, 0xbff0000000000000) /* nan, -1: -1 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* nan, 1.5: 1.5 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* nan, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* nan, inf: inf */
CASE_D_DD(fmax, 0x7ff8000000000000, 0xfff0000000000000, 0xfff0000000000000) /* nan, -inf: -inf */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* nan, nan: nan */
CASE_D_DD(fmax, 0x7ff8000000000000, 0xfff8000000000000, 0x7ff8000000000000) /* nan, -nan: nan */
CASE_D_DD(fmax, 0x7ff8000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* nan, nan: nan */
CASE_D_DD(fmax, 0xfff8000000000000, 0x0000000000000000, 0x0000000000000000) /* -nan, 0: 0 */
CASE_D_DD(fmax, 0xfff8000000000000, 0x8000000000000000, 0x8000000000000000) /* -nan, -0: -0 */
CASE_D_DD(fmax, 0xfff8000000000000, 0x0000000000000001, 0x0000000000000001) /* -nan, 4.9406564584124654e-324: 4.9406564584124654e-324 */
CASE_D_DD(fmax, 0xfff8000000000000, 0x3ff0000000000000, 0x3ff0000000000000) /* -nan, 1: 1 */
CASE_D_DD(fmax, 0xfff8000000000000, 0xbff0000000000000, 0xbff0000000000000) /* -nan, -1: -1 */
CASE_D_DD(fmax, 0xfff8000000000000, 0x3ff8000000000000, 0x3ff8000000000000) /* -nan, 1.5: 1.5 */
CASE_D_DD(fmax, 0xfff8000000000000, 0x7fefffffffffffff, 0x7fefffffffffffff) /* -nan, 1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DD(fmax, 0xfff8000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* -nan, inf: inf */
CASE_D_DD(fmax, 0xfff8000000000000, 0xfff0000000000000, 0xfff0000000000000) /* -nan, -inf: -inf */
CASE_D_DD(fmax, 0xfff8000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* -nan, nan: nan */
CASE_D_DD(fmax, 0xfff8000000000000, 0xfff8000000000000, 0x7ff8000000000000) /* -nan, -nan: nan */
CASE_D_DD(fmax, 0xfff8000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* -nan, nan: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x0000000000000000, 0x7ff8000000000000) /* nan, 0: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x8000000000000000, 0x7ff8000000000000) /* nan, -0: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x0000000000000001, 0x7ff8000000000000) /* nan, 4.9406564584124654e-324: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x3ff0000000000000, 0x7ff8000000000000) /* nan, 1: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0xbff0000000000000, 0x7ff8000000000000) /* nan, -1: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x3ff8000000000000, 0x7ff8000000000000) /* nan, 1.5: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x7fefffffffffffff, 0x7ff8000000000000) /* nan, 1.7976931348623157e+308: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x7ff0000000000000, 0x7ff8000000000000) /* nan, inf: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0xfff0000000000000, 0x7ff8000000000000) /* nan, -inf: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* nan, nan: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0xfff8000000000000, 0x7ff8000000000000) /* nan, -nan: nan */
CASE_D_DD(fmax, 0x7ff4000000000000, 0x7ff4000000000000, 0x7ff8000000000000) /* nan, nan: nan */

/* fma: double(double, double, double), NAN_QUIET */
CASE_D_DDD(fma, 0x7ff0000000000000, 0x0000000000000000, 0x3ff0000000000000, 0x7ff8000000000000) /* inf, 0, 1: nan */
CASE_D_DDD(fma, 0x7ff0000000000000, 0x3ff0000000000000, 0xfff0000000000000, 0x7ff8000000000000) /* inf, 1, -inf: nan */
CASE_D_DDD(fma, 0x7ff0000000000000, 0x3ff0000000000000, 0x7ff0000000000000, 0x7ff0000000000000) /* inf, 1, inf: inf */
CASE_D_DDD(fma, 0xfff0000000000000, 0x3ff0000000000000, 0x7ff0000000000000, 0x7ff8000000000000) /* -inf, 1, inf: nan */
CASE_D_DDD(fma, 0x7ff8000000000000, 0x3ff0000000000000, 0x3ff0000000000000, 0x7ff8000000000000) /* nan, 1, 1: nan */
CASE_D_DDD(fma, 0x3ff0000000000000, 0x3ff0000000000000, 0x7ff8000000000000, 0x7ff8000000000000) /* 1, 1, nan: nan */
CASE_D_DDD(fma, 0x7ff4000000000000, 0x3ff0000000000000, 0x3ff0000000000000, 0x7ff8000000000000) /* nan, 1, 1: nan */
CASE_D_DDD(fma, 0x0000000000000000, 0x3ff0000000000000, 0x0000000000000000, 0x0000000000000000) /* 0, 1, 0: 0 */
CASE_D_DDD(fma, 0x8000000000000000, 0x3ff0000000000000, 0x0000000000000000, 0x0000000000000000) /* -0, 1, 0: 0 */
CASE_D_DDD(fma, 0x8000000000000000, 0x3ff0000000000000, 0x8000000000000000, 0x8000000000000000) /* -0, 1, -0: -0 */
CASE_D_DDD(fma, 0x3ff0000000000000, 0x3ff0000000000000, 0xbff0000000000000, 0x0000000000000000) /* 1, 1, -1: 0 */
CASE_D_DDD(fma, 0x2000000000000000, 0x2000000000000000, 0x0000000000000000, 0x0010000000000000) /* 1.4916681462400413e-154, 1.4916681462400413e-154, 0: 2.2250738585072014e-308 */
CASE_D_DDD(fma, 0xa000000000000000, 0x2000000000000000, 0x0000000000000000, 0x8010000000000000) /* -1.4916681462400413e-154, 1.4916681462400413e-154, 0: -2.2250738585072014e-308 */
CASE_D_DDD(fma, 0x2000000000000000, 0xa000000000000000, 0x8000000000000000, 0x8010000000000000) /* 1.4916681462400413e-154, -1.4916681462400413e-154, -0: -2.2250738585072014e-308 */
CASE_D_DDD(fma, 0x1fe0000000000000, 0x1fe0000000000000, 0x0000000000000000, 0x0001000000000000) /* 3.7291703656001034e-155, 3.7291703656001034e-155, 0: 1.3906711615670009e-309 */
CASE_D_DDD(fma, 0x1fe0000000000000, 0x1fe8000000000000, 0x0000000000000000, 0x0001800000000000) /* 3.7291703656001034e-155, 5.5937555484001551e-155, 0: 2.0860067423505013e-309 */
CASE_D_DDD(fma, 0x1fe0000000000000, 0x1fe0000000000000, 0x0000000000000001, 0x0001000000000001) /* 3.7291703656001034e-155, 3.7291703656001034e-155, 4.9406564584124654e-324: 1.3906711615670058e-309 */
CASE_D_DDD(fma, 0x1fe0000000000000, 0x1fe0000000000000, 0x8000000000000001, 0x0000ffffffffffff) /* 3.7291703656001034e-155, 3.7291703656001034e-155, -4.9406564584124654e-324: 1.3906711615669959e-309 */
CASE_D_DDD(fma, 0x7fefffffffffffff, 0x4000000000000000, 0xfff0000000000000, 0xfff0000000000000) /* 1.7976931348623157e+308, 2, -inf: -inf */
CASE_D_DDD(fma, 0x7fefffffffffffff, 0x7fefffffffffffff, 0x0000000000000000, 0x7ff0000000000000) /* 1.7976931348623157e+308, 1.7976931348623157e+308, 0: inf */
CASE_D_DDD(fma, 0x7fefffffffffffff, 0x4000000000000000, 0xffefffffffffffff, 0x7fefffffffffffff) /* 1.7976931348623157e+308, 2, -1.7976931348623157e+308: 1.7976931348623157e+308 */
CASE_D_DDD(fma, 0x7fefffffffffffff, 0xc000000000000000, 0x7fefffffffffffff, 0xffefffffffffffff) /* 1.7976931348623157e+308, -2, 1.7976931348623157e+308: -1.7976931348623157e+308 */
CASE_D_DDD(fma, 0x7fefffffffffffff, 0x3ff0000000000001, 0x0000000000000000, 0x7ff0000000000000) /* 1.7976931348623157e+308, 1.0000000000000002, 0: inf */
CASE_D_DDD(fma, 0x4008000000000000, 0x4330000000000001, 0x0000000000000000, 0x4348000000000002) /* 3, 4503599627370497, 0: 13510798882111492 */
CASE_D_DDD(fma, 0x4008000000000000, 0x4330000000000001, 0x0000000000000001, 0x4348000000000002) /* 3, 4503599627370497, 4.9406564584124654e-324: 13510798882111492 */
CASE_D_DDD(fma, 0x4008000000000000, 0x4330000000000001, 0x8000000000000001, 0x4348000000000001) /* 3, 4503599627370497, -4.9406564584124654e-324: 13510798882111490 */
CASE_D_DDD(fma, 0xc008000000000000, 0x4330000000000001, 0x0000000000000001, 0xc348000000000001) /* -3, 4503599627370497, 4.9406564584124654e-324: -13510798882111490 */
CASE_D_DDD(fma, 0x4008000000000000, 0x4330000000000003, 0x0000000000000001, 0x4348000000000005) /* 3, 4503599627370499, 4.9406564584124654e-324: 13510798882111498 */
CASE_D_DDD(fma, 0x4008000000000000, 0x4330000000000003, 0x8000000000000001, 0x4348000000000004) /* 3, 4503599627370499, -4.9406564584124654e-324: 13510798882111496 */
CASE_D_DDD(fma, 0x3ff0000000000001, 0x3fefffffffffffff, 0xbff0000000000000, 0x3c9ffffffffffffe) /* 1.0000000000000002, 0.99999999999999989, -1: 1.1102230246251563e-16 */
CASE_D_DDD(fma, 0x3ff0000000000001, 0x3ff0000000000001, 0xbff0000000000002, 0x3970000000000000) /* 1.0000000000000002, 1.0000000000000002, -1.0000000000000004: 4.9303806576313238e-32 */
CASE_D_DDD(fma, 0x3ff199999999999a, 0x3ff199999999999a, 0xbff35c28f5c28f5c, 0x3cb0a3d70a3d70a4) /* 1.1000000000000001, 1.1000000000000001, -1.21: 2.3092638912203257e-16 */
CASE_D_DDD(fma, 0x3fb999999999999a, 0x4024000000000000, 0xbff0000000000000, 0x3c90000000000000) /* 0.10000000000000001, 10, -1: 5.5511151231257827e-17 */
CASE_D_DDD(fma, 0x0000000000000003, 0x3fe0000000000000, 0x0000000000000000, 0x0000000000000002) /* 1.4821969375237396e-323, 0.5, 0: 9.8813129168249309e-324 */
CASE_D_DDD(fma, 0x0000000000000001, 0x3fe0000000000000, 0x0000000000000000, 0x0000000000000000) /* 4.9406564584124654e-324, 0.5, 0: 0 */
CASE_D_DDD(fma, 0x0000000000000001, 0x3fe8000000000000, 0x8000000000000000, 0x0000000000000001) /* 4.9406564584124654e-324, 0.75, -0: 4.9406564584124654e-324 */
CASE_D_DDD(fma, 0x0000000000000003, 0x3fe0000000000000, 0x0000000000000001, 0x0000000000000002) /* 1.4821969375237396e-323, 0.5, 4.9406564584124654e-324: 9.8813129168249309e-324 */
CASE_D_DDD(fma, 0x0010000000000000, 0x3fefffffffffffff, 0x0000000000000000, 0x0010000000000000) /* 2.2250738585072014e-308, 0.99999999999999989, 0: 2.2250738585072014e-308 */
CASE_D_DDD(fma, 0x3ff0000000000000, 0x3ff0000000000000, 0x3ca0000000000000, 0x3ff0000000000000) /* 1, 1, 1.1102230246251565e-16: 1 */
CASE_D_DDD(fma, 0x3ff0000000000000, 0x3ff0000000000000, 0x3ca0000000000001, 0x3ff0000000000001) /* 1, 1, 1.1102230246251568e-16: 1.0000000000000002 */
CASE_D_DDD(fma, 0x3ff0000000000001, 0x3ff0000000000000, 0xbca0000000000000, 0x3ff0000000000000) /* 1.0000000000000002, 1, -1.1102230246251565e-16: 1 */
CASE_D_DDD(fma, 0x3ff0000000000001, 0x3ff0000000000000, 0x3ca0000000000000, 0x3ff0000000000002) /* 1.0000000000000002, 1, 1.1102230246251565e-16: 1.0000000000000004 */

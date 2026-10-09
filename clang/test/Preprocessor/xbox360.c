// RUN: %clang_cc1 -E -dM -ffreestanding -triple=ppc64-ibm-xbox360 %s | FileCheck %s

// CHECK: #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
// CHECK: #define __LDBL_MANT_DIG__ 53
// CHECK: #define __SIZEOF_LONG_DOUBLE__ 8
// CHECK: #define __SIZEOF_LONG__ 4
// CHECK: #define __SIZEOF_POINTER__ 4
// CHECK: #define __SIZEOF_WCHAR_T__ 2
// CHECK: #define __WCHAR_WIDTH__ 16

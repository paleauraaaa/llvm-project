// RUN: %clang_cc1 -triple powerpc64-unknown-unknown -fms-extensions -fsyntax-only -verify %s

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;

typedef struct __declspec(intrin_type) __vector4 { // expected-error {{intrinsic type '__vector4' has size 8 and alignment 4, but the compiler expects size 16 and alignment 16}}
  float v[2];
} __vector4_bad;
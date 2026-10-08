// RUN: %clang_cc1 -triple powerpc64-unknown-unknown -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64-unknown-unknown -fms-extensions -fsyntax-only -Wno-unknown-intrin-type -verify=nowarn %s

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;

struct __declspec(intrin_type) not_intrinsic { // expected-warning {{'not_intrinsic' is not a recognized intrinsic type; attribute 'intrin_type' is ignored}}
  float v[4];
};

__declspec(intrin_type) int not_a_record; // expected-warning {{'intrin_type' attribute only applies to structs, unions, and classes}} nowarn-warning@-0 {{only applies}}

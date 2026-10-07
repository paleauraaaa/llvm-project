// RUN: %clang_cc1 -triple powerpc64-unknown-unknown -target-cpu xenon -fms-extensions -fsyntax-only -verify %s

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;

struct __declspec(passinreg) F { float x; };
struct __declspec(passinreg) D { double x; };
struct __declspec(passinreg) V { __vector4 v; };

struct __declspec(passinreg) Two { float x, y; }; // expected-warning {{'passinreg' type 'Two' must consist of a single float, double, or __vector4 member; attribute is ignored}}
struct __declspec(passinreg) I { int x; }; // expected-warning {{'passinreg' type 'I' must}}
struct __declspec(passinreg) Empty {}; // expected-warning {{'passinreg' type 'Empty' must}}
struct __declspec(passinreg) B { int x : 3; }; // expected-warning {{'passinreg' type 'B' must}}
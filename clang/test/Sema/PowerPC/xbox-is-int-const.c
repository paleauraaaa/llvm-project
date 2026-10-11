// RUN: split-file %s %t
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/missing.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -verify %t/missing.cpp
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/local.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -verify %t/types.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -verify %t/constant.cpp
// RUN: %clang_cc1 -triple powerpc64-unknown-linux -std=c89 -verify %t/other.c

//--- missing.c
void missing(void) {
  __IsIntConst(1); // expected-error 2 {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}

//--- missing.cpp
void missing() {
  __IsIntConst(1); // expected-error {{use of undeclared identifier}}
}

//--- local.c
void local(void) {
  int __IsIntConst(int);
  (void)__IsIntConst(1);
}
void outside(void) {
  __IsIntConst(1); // expected-error 2 {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}

//--- types.c
float __IsIntConst(int); // expected-error {{invalid declaration of Xbox vector intrinsic}}

//--- constant.cpp
// expected-no-diagnostics
extern "C" int __IsIntConst(int);
enum { Value = 7 };
const int variable = 7;
static const int static_variable = 9;
constexpr int constexpr_variable = 11;
constexpr int constexpr_function() { return 13; }
static_assert(__IsIntConst(1), "");
static_assert(__IsIntConst(Value + 3), "");
static_assert(!__IsIntConst(variable), "");
static_assert(!__IsIntConst(static_variable), "");
static_assert(!__IsIntConst(constexpr_variable), "");
static_assert(!__IsIntConst(constexpr_function()), "");
template <class T> int classify(T value) { return __IsIntConst(value); }
template int classify<int>(int);

//--- other.c
// expected-no-diagnostics
int __IsIntConst(int value) { return value; }

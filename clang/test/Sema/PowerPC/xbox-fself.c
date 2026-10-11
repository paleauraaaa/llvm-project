// RUN: split-file %s %t
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/missing.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -verify %t/missing.cpp
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/local.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -verify %t/types.c
// RUN: %clang_cc1 -triple powerpc64-unknown-linux -verify %t/other.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -verify %t/arguments.c
//--- missing.c
float f(void) {
  return __fself(0, 1, 2); // expected-error {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}
//--- missing.cpp
float f() {
  return __fself(0, 1, 2); // expected-error {{use of undeclared identifier}}
}
//--- local.c
float local(void) {
  float __fself(float, float, float);
  return __fself(0, 1, 2);
}
float outside(void) {
  return __fself(0, 1, 2); // expected-error {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}
//--- types.c
double __fself(float, float, float); // expected-error {{invalid declaration of Xbox intrinsic}}
//--- other.c
// expected-no-diagnostics
float __fself(float compare, float ge, float lt) {
  return compare >= 0 ? ge : lt;
}
//--- arguments.c
float __fself(float, float, float);
float too_few(void) {
  return __fself(1, 2); // expected-error {{too few arguments}}
}
float too_many(void) {
  return __fself(1, 2, 3, 4); // expected-error {{too many arguments}}
}

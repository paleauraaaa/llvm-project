// RUN: split-file %s %t
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/missing.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -verify %t/missing.cpp
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/scope.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -verify %t/signatures.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -verify %t/operands.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -verify %t/linkage.cpp
// RUN: %clang_cc1 -triple powerpc64-unknown-linux -verify %t/other.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c89 -verify %t/timebase.c

//--- missing.c
unsigned int absent(void) {
  return _byteswap_ulong(1); // expected-error {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}
void absent_opcode(void) {
  __emit(0); // expected-error 2 {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}
//--- missing.cpp
unsigned long long absent() {
  return __mftb(); // expected-error {{use of undeclared identifier}}
}
unsigned long swapped() {
  return _byteswap_ulong(1); // expected-error {{use of undeclared identifier}}
}
//--- scope.c
unsigned int local(long value) {
  unsigned int _CountLeadingZeros(long);
  return _CountLeadingZeros(value);
}
unsigned int outside(long value) {
  return _CountLeadingZeros(value); // expected-error {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}
//--- signatures.c
float __fres(float); // expected-error {{invalid declaration of Xbox}}
unsigned int __mftb(void); // expected-error {{invalid declaration of Xbox}}
unsigned int _byteswap_ulong(unsigned int); // expected-error {{invalid declaration of Xbox}}
unsigned int __loadwordupdate(int, void *); // expected-error {{invalid declaration of Xbox}}
void __dcbst(int, void *); // expected-error {{invalid declaration of Xbox}}
void __tw(unsigned int, long, long); // expected-error {{invalid declaration of Xbox}}
//--- operands.c
void __emit(unsigned int);
void __twi(unsigned int, int, int);
void __tdi(unsigned int, long long, long long);
unsigned long __loadwordupdate(int, void *);
void __storewordupdate(unsigned long, int, void *);
void check(int dynamic, void *pointer) {
  __emit(dynamic); // expected-error {{must be a constant integer}}
  __twi(dynamic, 1, 2); // expected-error {{must be a constant integer}}
  __twi(32, 1, 2); // expected-error {{outside the valid range}}
  __twi(4, 1, dynamic); // expected-error {{must be a constant integer}}
  __twi(4, 1, -32769); // expected-error {{outside the valid range}}
  __tdi(4, 1, 32768); // expected-error {{outside the valid range}}
  __loadwordupdate(4, (void *)0); // expected-error {{modifiable pointer lvalue}}
  __storewordupdate(1, 4, pointer + 1); // expected-error {{modifiable pointer lvalue}}
  void *const immutable = pointer;
  __loadwordupdate(4, immutable); // expected-error {{modifiable pointer lvalue}}
  __loadwordupdate(4, pointer);
  __storewordupdate(1, 4, pointer);
}
void load_memory_reference(void **pointer) {
  __loadwordupdate(4, *pointer); // expected-error {{must be a pointer variable}}
}
//--- linkage.cpp
// expected-no-diagnostics
double __frnd(double value) { return value; }
//--- other.c
// expected-no-diagnostics
#undef __fres
float __fres(float value) { return value; }
unsigned int __mftb(void) { return 1; }
void __emit(unsigned int word) {}
//--- timebase.c
unsigned long long __mftb();
unsigned long long read_time(void) { return __mftb(); }
unsigned long long bad_time(void) {
  return __mftb(1); // expected-error {{too many arguments}} expected-warning {{passing arguments to '__mftb' without a prototype}}
}

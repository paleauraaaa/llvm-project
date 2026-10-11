// RUN: split-file %s %t
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -std=c89 -verify %t/missing.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -x c++ -verify %t/missing.cpp
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -std=c89 -verify %t/local.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -verify %t/types.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -verify=expected,c %t/calls.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -x c++ -verify=expected,cxx %t/calls.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -target-feature -vmx128 -emit-llvm -o %t/feature.ll -verify %t/feature.c
// RUN: %clang_cc1 -triple powerpc-unknown-linux -target-cpu xenon -fms-extensions -verify %t/not-xbox.c

//--- missing.c
void f(void) {
  __vaddfp(0, 0); // expected-error 2 {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
  __VMXGetReg(0); // expected-error 2 {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}

//--- missing.cpp
void f() {
  __vaddfp(0, 0); // expected-error {{use of undeclared identifier '__vaddfp'}}
  __VMXGetReg(0); // expected-error {{use of undeclared identifier '__VMXGetReg'}}
}

//--- types.c
__vector4 __vaddfp(__vector4 a, __vector4 b); // expected-error 3 {{unknown type name '__vector4'}}

struct __vector4;
struct __vector4 __vsubfp(struct __vector4, struct __vector4); // expected-error {{invalid declaration of Xbox vector intrinsic}}

struct other { float v[4]; };
struct other __vaddsws(struct other, struct other); // expected-error {{invalid declaration of Xbox vector intrinsic}}

//--- local.c
typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;
__vector4 local(__vector4 a) {
  __vector4 __vaddfp(__vector4, __vector4);
  return __vaddfp(a, a);
}
void outside(void) {
  __vaddfp(0, 0); // expected-error 2 {{requires an explicit declaration}} expected-error {{use of undeclared identifier}}
}

//--- calls.c
struct __vector4;
typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;

enum PackType { Pack = 0 };
enum PackMask { Mask = 0 };

#ifdef __cplusplus
extern "C" {
#endif
__vector4 __vaddfp(__vector4, __vector4); // cxx-note 2 {{candidate function not viable}}
__vector4 __vcfsx(__vector4, unsigned short);
__vector4 __vspltisw(int);
__vector4 __vspltw(__vector4, unsigned int);
__vector4 __vpermwi(__vector4, unsigned int);
__vector4 __vpkd3d(__vector4, __vector4, enum PackType, enum PackMask, unsigned int);
__vector4 __VMXGetReg(int);
void __VMXSetReg(int, __vector4);

int __vsubfp(int, int); // expected-error {{invalid declaration of Xbox vector intrinsic}}
__vector4 __vslw(__vector4); // expected-error {{invalid declaration of Xbox vector intrinsic}}
__vector4 __vsrw(__vector4, __vector4, ...); // expected-error {{invalid declaration of Xbox vector intrinsic}}
__vector4 __vcfux(__vector4, unsigned int); // expected-error {{invalid declaration of Xbox vector intrinsic}}
__vector4 __vcmpeqfpR(__vector4, __vector4, int *); // expected-error {{invalid declaration of Xbox vector intrinsic}}
__vector4 __lvx_volatile(const void *, int); // expected-error {{invalid declaration of Xbox vector intrinsic}}

__vector4 __vmulfp(__vector4 a, __vector4 b) { // expected-error {{definition of builtin function}}
  return a;
}
#ifdef __cplusplus
}
#endif

void f(__vector4 a, __vector4 b, int n) {
  __vaddfp(a, b);
  __vaddfp(a); // c-error {{too few arguments}} cxx-error {{no matching function}}
  __vaddfp(a, b, a); // c-error {{too many arguments}} cxx-error {{no matching function}}
  __vcfsx(a, 31);
  __vcfsx(a, 32); // expected-error {{outside the valid range}}
  __vcfsx(a, n); // expected-error {{must be a constant integer}}
  __vspltisw(-16);
  __vspltisw(15);
  __vspltisw(-17); // expected-error {{outside the valid range}}
  __vspltw(a, 4); // expected-error {{outside the valid range}}
  __vpermwi(a, 256); // expected-error {{outside the valid range}}
  __vpkd3d(a, b, Pack, Mask, 3);
  __vpkd3d(a, b, Pack, Mask, 4); // expected-error {{outside the valid range}}
  __VMXGetReg(127);
  __VMXGetReg(128); // expected-error {{outside the valid range}}
  __VMXSetReg(127, a);
  __VMXSetReg(n, a); // expected-error {{must be a constant integer}}
  (void)&__vaddfp; // expected-error {{builtin functions must be directly called}}
}

//--- feature.c
typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;
__vector4 __vaddfp(__vector4, __vector4);
__vector4 f(__vector4 a) {
  return __vaddfp(a, a); // expected-error {{needs target feature vmx128}}
}

//--- not-xbox.c
// expected-no-diagnostics
int __vaddfp(int, int);
int f(int x) { return __vaddfp(x, x); }

// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -I %S/../../../include -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -I %S/../../../include -verify %s
// expected-error@* 70 {{use of undeclared identifier}}

#define XBOX_PPC_BUILTIN(NAME, SIG, KIND, INTRINSIC, ASM, ICE) \
  void test_##NAME() { (void)NAME; }
#include "clang/Basic/BuiltinsPPCXboxScalar.def"
#undef XBOX_PPC_BUILTIN

#ifdef __sync
#error SDK macros must not be predefined on Xbox
#endif
#ifdef __trap
#error SDK macros must not be predefined on Xbox
#endif
#ifdef __mulhd
#error SDK macros must not be predefined on Xbox
#endif
#ifdef MemoryBarrier
#error SDK macros must not be predefined on Xbox
#endif
#ifdef YieldProcessor
#error SDK macros must not be predefined on Xbox
#endif

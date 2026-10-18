// RUN: %clang_cc1 -triple i686-unknown-windows-msvc -fsyntax-only -fdeclspec -verify %s
// RUN: %clang_cc1 -triple thumbv7-unknown-windows-msvc -fsyntax-only -fdeclspec -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-windows-msvc -fsyntax-only -fdeclspec -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fsyntax-only -fdeclspec -DXBOX -verify %s
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fsyntax-only -fdeclspec -DPPC -verify %s
#if defined(_M_IX86) || defined(_M_ARM) || defined(XBOX)
// CHECK: expected-no-diagnostics
#endif

void __declspec(naked) f(void) {}
#if defined(PPC)
// expected-error@-2{{'naked' attribute is not supported on 'powerpc64'}}
#elif !defined(_M_IX86) && !defined(_M_ARM) && !defined(XBOX)
// expected-error@-4{{'naked' attribute is not supported on 'x86_64'}}
#endif

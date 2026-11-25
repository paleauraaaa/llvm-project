// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -fsyntax-only -verify=xbox %s
// RUN: %clang_cc1 -triple powerpc64-microsoft-xbox360 -fms-extensions -fsyntax-only -verify=xbox %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -x c++ -fsyntax-only -verify=xbox %s
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fms-extensions -fsyntax-only -verify=other %s

int __cdecl keyword(int x); // other-warning {{'__cdecl' calling convention is not supported for this target}}
int __attribute__((cdecl)) attribute(int x); // other-warning {{'cdecl' calling convention is not supported for this target}}
typedef int (__cdecl *callback)(int); // other-warning {{'__cdecl' calling convention is not supported for this target}}
int __cdecl variadic(const char *format, ...); // other-warning {{'__cdecl' calling convention is not supported for this target}}
int __stdcall unsupported(int x); // xbox-warning {{'__stdcall' calling convention is not supported for this target}} other-warning {{'__stdcall' calling convention is not supported for this target}}

void compatible(int (*ordinary)(int), callback explicit_cdecl) {
  ordinary = explicit_cdecl;
  explicit_cdecl = ordinary;
}

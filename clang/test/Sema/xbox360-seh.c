// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -fasync-exceptions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-extensions -fsyntax-only -verify=desktop %s
// desktop-no-diagnostics

void *_alloca(__SIZE_TYPE__);
void *alloca(__SIZE_TYPE__);
#pragma intrinsic(_alloca)
void escape(void *);
void work(void);

void finally_alloca(void) {
  __try { work(); } __finally {
    escape(_alloca(32)); // expected-error {{cannot be called from a filter expression or '__finally' block on Xbox 360}}
    escape(alloca(32)); // expected-error {{cannot be called from a filter expression}}
    escape(__builtin_alloca(32)); // expected-error {{cannot be called from a filter expression}}
  }
}

void filter_alloca(void) {
  __try { work(); }
  __except ((escape(_alloca(32)), 1)) {} // expected-error {{cannot be called from a filter expression}}
}

void except_in_finally(void) {
  __try { work(); } __finally {
    __try { work(); }
    __except (1) {} // expected-error {{cannot use '__except' in a '__finally' block on Xbox 360}}
  }
}

void allowed(void) {
  escape(_alloca(32));
  __try { escape(_alloca(32)); }
  __except (1) { escape(_alloca(32)); }
  __try { work(); } __finally {
    __try { work(); } __finally { work(); }
  }
}

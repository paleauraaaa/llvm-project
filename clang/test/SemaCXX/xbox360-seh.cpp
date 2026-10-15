// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -std=c++11 -fexceptions -fcxx-exceptions -fsyntax-only -verify=common,eh %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -std=c++11 -fexceptions -fcxx-exceptions -fasync-exceptions -fsyntax-only -verify=common,eh %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -std=c++11 -fsyntax-only -verify=common %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-extensions -std=c++11 -fexceptions -fcxx-exceptions -fsyntax-only -verify=desktop %s
// desktop-no-diagnostics

extern "C" void *_alloca(__SIZE_TYPE__);
#pragma intrinsic(_alloca)
void escape(void *);
void work();
struct Guard {
  Guard();
  ~Guard();
};
struct Trivial {
  Trivial();
};
void use(const Guard &);
void default_argument(const Guard & = Guard());

void before() {
  Guard g;
  __try { work(); } __except (1) {} // eh-error {{cannot use SEH '__try' in a function that requires C++ object unwinding on Xbox 360}}
}

void after() {
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
  Guard g;
}

void inside_try() {
  __try { Guard g; } __except (1) {} // eh-error {{requires C++ object unwinding}}
}

void inside_except() {
  __try { work(); } __except (1) { Guard g; } // eh-error {{requires C++ object unwinding}}
}

void inside_finally() {
  __try { work(); } __finally { Guard g; } // eh-error {{requires C++ object unwinding}}
}

void temporary() {
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
  use(Guard());
}

void default_temporary() {
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
  default_argument();
}

void array() {
  Guard g[2];
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
}

void static_local() {
  static Guard g;
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
}

void parameter(Guard g) {
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
}

struct Owner {
  Guard g;
  Owner() {
    __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
  }
  ~Owner() {
    __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
  }
};

struct Derived : Guard {
  Derived() {
    __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
  }
};

void allowed_objects() {
  extern Guard g;
  Guard *p = &g;
  Trivial t;
  (void)p;
  (void)sizeof(Guard);
  __try { work(); } __except (1) {}
}

void nested_functions() {
  auto lambda = [] { Guard g; };
  struct Local {
    void f() { Guard g; }
  };
  __try { lambda(); } __except (1) {}
}

void finally_alloca() {
  __try { work(); } __finally {
    escape(_alloca(32)); // common-error {{cannot be called from a filter expression or '__finally' block on Xbox 360}}
    (void)sizeof(_alloca(32)); // common-error {{cannot be called from a filter expression}}
    escape(__builtin_alloca(32)); // common-error {{cannot be called from a filter expression}}
    escape(__builtin_alloca_with_align(32, 128)); // common-error {{cannot be called from a filter expression}}
    escape(__builtin_alloca_uninitialized(32)); // common-error {{cannot be called from a filter expression}}
    escape(__builtin_alloca_with_align_uninitialized(32, 128)); // common-error {{cannot be called from a filter expression}}
  }
}

void filter_alloca() {
  __try { work(); }
  __except ((escape(_alloca(32)), 1)) {} // common-error {{cannot be called from a filter expression}}
}

void except_in_finally() {
  __try { work(); } __finally {
    __try { work(); }
    __except (1) {} // common-error {{cannot use '__except' in a '__finally' block on Xbox 360}}
  }
}

void allowed_alloca_and_nesting() {
  escape(_alloca(32));
  __try { escape(_alloca(32)); }
  __except (1) { escape(_alloca(32)); }
  __try { work(); } __finally {
    __try { work(); } __finally { work(); }
    auto lambda = [] {
      escape(_alloca(32));
      __try { work(); } __except (1) {}
    };
    lambda();
  }
}

template <class T> void dependent_object() {
  T object;
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
}
template void dependent_object<Trivial>();
template void dependent_object<Guard>(); // eh-note {{in instantiation of function template specialization}}

template <class T> void nondependent_object() {
  Guard object;
  __try { work(); } __except (1) {} // eh-error {{requires C++ object unwinding}}
}

template <class T> void dependent_alloca(T n) {
  __try { work(); } __finally {
    escape(_alloca(n)); // common-error {{cannot be called from a filter expression}}
  }
}
template void dependent_alloca<unsigned>(unsigned); // common-note {{in instantiation of function template specialization}}

namespace ADL {
struct Size {};
void *_alloca(Size);
}
template void dependent_alloca<ADL::Size>(ADL::Size);

template <class T> void template_finally() {
  __try { work(); } __finally {
    __try { work(); }
    __except (1) {} // common-error {{cannot use '__except' in a '__finally' block}}
  }
}

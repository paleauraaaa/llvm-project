// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c++20 -fexperimental-new-constant-interpreter -fsyntax-only -verify %s

#pragma bitfield_order(msb_to_lsb)
template<class T> struct Show {
#pragma bitfield_order(show) // expected-warning 2 {{== msb_to_lsb}} expected-warning {{== lsb_to_msb}}
  unsigned a:3;
};
#pragma bitfield_order(lsb_to_msb)
Show<int> first; // expected-note {{in instantiation of template class 'Show<int>' requested here}}
#pragma bitfield_order(msb_to_lsb)
Show<int> reused;
Show<char> second; // expected-note {{in instantiation of template class 'Show<char>' requested here}}

// The empty definition-time stack and the nonempty instantiation-time stack
// produce different failed-pop diagnostics.
template<class T> struct Missing {
#pragma bitfield_order(pop, absent) // expected-warning {{failed: stack empty}} expected-warning {{failed: no matching push for identifier}}
};
#pragma bitfield_order(lsb_to_msb)
#pragma bitfield_order(push, survivor, msb_to_lsb)
Missing<int> missing; // expected-note {{in instantiation of template class 'Missing<int>' requested here}}
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(pop, survivor)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}

template<class T> struct Reset {
#pragma bitfield_order()
};
#pragma bitfield_order(lsb_to_msb)
Reset<int> reset;
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}

#pragma bitfield_order(push, override_label)
template<class T> struct PopOverride {
#pragma bitfield_order(pop, override_label, lsb_to_msb)
};
#pragma bitfield_order(push, override_label, msb_to_lsb)
PopOverride<int> override_order;
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}

// An unused function pattern must neither show nor push yet.
template<class T> void unused() {
#pragma bitfield_order(show)
#pragma bitfield_order(push, unused_label)
}
#pragma bitfield_order(pop, unused_label) // expected-warning {{failed: stack empty}}

template<class T> unsigned deferred() {
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}} expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(lsb_to_msb)
  return 0;
}
#pragma bitfield_order(msb_to_lsb)
unsigned (*function_first)() = &deferred<int>; // expected-note {{in instantiation of function template specialization 'deferred<int>' requested here}}
unsigned (*function_reused)() = &deferred<int>;
unsigned (*function_second)() = &deferred<char>; // expected-note {{in instantiation of function template specialization 'deferred<char>' requested here}}
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}

#if __cplusplus >= 202002L
template<class T> constexpr unsigned constant() {
#pragma bitfield_order(push, constant_label, msb_to_lsb)
#pragma clang fp contract(off)
  unsigned value = 1;
#pragma bitfield_order(pop, constant_label)
  return value;
}
static_assert(constant<int>() == 1);
#endif

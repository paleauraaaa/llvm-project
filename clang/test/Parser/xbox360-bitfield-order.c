// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -fsyntax-only -verify %s

#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(lsb_to_msb)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(show, msb_to_lsb) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(show, ignored) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order()
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(lsb_to_msb, show)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order()

#define LOW lsb_to_msb
#pragma bitfield_order(push, outer, LOW)
#pragma bitfield_order(push, inner, msb_to_lsb)
#pragma bitfield_order(pop, outer)
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(pop) // expected-warning {{failed: stack empty}}

#pragma bitfield_order(lsb_to_msb)
#pragma bitfield_order(push, saved, msb_to_lsb)
#pragma bitfield_order(pop, missing) // expected-warning {{failed: no matching push for identifier}}
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(pop, saved)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(push, saved, msb_to_lsb)
#pragma bitfield_order(push, child, lsb_to_msb)
#pragma bitfield_order(pop, missing) // expected-warning {{failed: no matching push for identifier}}
#pragma bitfield_order(pop, child)
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(pop, saved)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(pop) // expected-warning {{failed: stack empty}}
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(pop, missing, lsb_to_msb) // expected-warning {{failed: stack empty}}
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}

#pragma bitfield_order(push)
#pragma bitfield_order()
#pragma bitfield_order(pop)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order(push, label)
#pragma bitfield_order()
#pragma bitfield_order(pop, label, msb_to_lsb)
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma bitfield_order(push, lsb_to_msb)
#pragma bitfield_order(pop, msb_to_lsb)
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}

struct Record {
  unsigned a : 3;
#pragma bitfield_order(lsb_to_msb)
  unsigned b : 5;
};
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}

void function(void) {
#pragma bitfield_order(msb_to_lsb)
  if (1) {
#pragma bitfield_order(lsb_to_msb)
  }
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
}
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}

#ifdef __cplusplus
namespace N {
#pragma bitfield_order(msb_to_lsb)
struct C {
  void method() {
#pragma bitfield_order(lsb_to_msb)
  }
};
}
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#endif

#pragma reverse_bitfield(off)
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}
#pragma reverse_bitfield(on)
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma reverse_bitfield(off)
#define SET_LOW _Pragma("bitfield_order(lsb_to_msb)")
SET_LOW
#pragma bitfield_order(show) // expected-warning {{== lsb_to_msb}}
#pragma bitfield_order()

#pragma bitfield_order // expected-warning {{malformed '#pragma bitfield_order'}}
#pragma bitfield_order(unknown) // expected-warning {{malformed '#pragma bitfield_order'}}
#pragma bitfield_order(push,) // expected-warning {{malformed '#pragma bitfield_order'}}
#pragma bitfield_order(push, label,) // expected-warning {{malformed '#pragma bitfield_order'}}
#pragma bitfield_order(lsb_to_msb // expected-warning {{malformed '#pragma bitfield_order'}}
#pragma bitfield_order(lsb_to_msb) extra // expected-warning {{extra tokens at end of '#pragma bitfield_order'}}
#pragma reverse_bitfield() // expected-warning {{malformed '#pragma reverse_bitfield'}}
#pragma reverse_bitfield(invalid) // expected-warning {{malformed '#pragma reverse_bitfield'}}
#pragma bitfield_order(show) // expected-warning {{== msb_to_lsb}}

#pragma bitfield_order(push, no_eof_warning)

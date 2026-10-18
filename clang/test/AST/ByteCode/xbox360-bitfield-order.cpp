// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -std=c++20 -fexperimental-new-constant-interpreter -fsyntax-only -verify %s
// expected-no-diagnostics

#pragma bitfield_order(lsb_to_msb)
struct Full { unsigned a:3; unsigned b:29; };
constexpr Full full = {5, 17};
static_assert(__builtin_bit_cast(unsigned, full) == 141);
constexpr Full inverse = __builtin_bit_cast(Full, 141u);
static_assert(inverse.a == 5 && inverse.b == 17);

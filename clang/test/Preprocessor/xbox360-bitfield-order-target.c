// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fsyntax-only -Wunknown-pragmas -verify %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fsyntax-only -Wunknown-pragmas -verify %s

#pragma bitfield_order(lsb_to_msb) // expected-warning {{unknown pragma ignored}}
#pragma reverse_bitfield(on) // expected-warning {{unknown pragma ignored}}

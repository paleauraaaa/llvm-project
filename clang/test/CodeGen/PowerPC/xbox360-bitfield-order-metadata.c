// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s

// Capturing the order does not yet change layout or bitfield accesses.
#pragma bitfield_order(lsb_to_msb)
struct Bits { unsigned a : 3; unsigned b : 5; };
struct Bits bits = {5, 17};

// CHECK: @bits = {{.*}}global { i8, [3 x i8] } { i8 -79, [3 x i8] zeroinitializer }

// CHECK-LABEL: define {{.*}} @read_b(
// CHECK: lshr i32 {{.*}}, 24
// CHECK: and i32 {{.*}}, 31
unsigned read_b(struct Bits *p) { return p->b; }

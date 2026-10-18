// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -O2 -disable-llvm-passes -emit-llvm -o - %s | FileCheck %s --check-prefix=TBAA

#pragma bitfield_order(lsb_to_msb)
struct Bits { unsigned a : 3; unsigned b : 5; };
struct Bits bits = {5, 17};

// CHECK: @bits = {{.*}}global {{.*}}i8 -115
// CHECK: @overlap = {{.*}}i8 -69, i8 -3, i8 -32

// CHECK-LABEL: define {{.*}} @read_b(
// CHECK: lshr i32 {{.*}}, 3
// CHECK: and i32 {{.*}}, 31
unsigned read_b(struct Bits *p) { return p->b; }

// CHECK-LABEL: define {{.*}} @write_b(
// CHECK: and i32 {{.*}}, 31
// CHECK: shl i32 {{.*}}, 3
// CHECK: and i32 {{.*}}, -249
void write_b(volatile struct Bits *p, unsigned value) { p->b = value; }

// TBAA-LABEL: define {{.*}} @copy(
// TBAA: call void @llvm.memcpy{{.*}}!tbaa.struct ![[FIELDS:[0-9]+]]
void copy(struct Bits *to, const struct Bits *from) { *to = *from; }
// TBAA: ![[FIELDS]] = !{i64 0, i64 4, !{{[0-9]+}}}

#pragma bitfield_order(msb_to_lsb)
struct Overlap {
  unsigned a:20;
#pragma bitfield_order(lsb_to_msb)
  unsigned b:12;
};
struct Overlap overlap = {0xabcde, 0x123};

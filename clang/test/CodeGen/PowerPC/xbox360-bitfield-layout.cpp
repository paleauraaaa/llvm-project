// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -emit-llvm -debug-info-kind=standalone -dwarf-version=4 -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -O2 -emit-obj -o %t.obj %s
// RUN: llvm-objdump -s %t.obj | FileCheck %s --check-prefix=BYTES
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fdump-record-layouts -fsyntax-only %s | FileCheck %s --check-prefix=LAYOUT
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -gcodeview -debug-info-kind=standalone -emit-obj -o %t.cv.obj %s
// RUN: llvm-readobj --codeview %t.cv.obj | FileCheck %s --check-prefix=CV

// Keep objects in one section, in declaration order, to check native bytes.
#pragma bitfield_order(lsb_to_msb)
struct Low { unsigned a:3; unsigned b:5; };
extern "C" {
// CHECK-DAG: @low = {{.*}}i8 -115
Low low = {5, 17};
struct Wide { unsigned long long a:3; unsigned long long b:5; };
Wide wide = {5, 17};
struct Mixed { unsigned char a:3; unsigned b:5; unsigned short c:4; };
Mixed mixed = {5, 17, 9};
struct Cross { unsigned a:30; unsigned b:5; unsigned c:3; };
Cross cross = {1, 17, 5};
struct Zero { unsigned a:3; unsigned :0; unsigned b:5; };
Zero zero = {5, 17};
union U { unsigned a:3; unsigned b:5; };
U u = {5};
#pragma pack(push, 1)
struct Packed { unsigned char tag; unsigned a:3; unsigned b:5; unsigned char end; };
Packed packed = {0x12, 5, 17, 0x34};
#pragma pack(pop)

#pragma bitfield_order(msb_to_lsb)
struct Switch {
  unsigned a:3;
#pragma bitfield_order(lsb_to_msb)
  unsigned b:5;
};
Switch switched = {5, 17};

#pragma bitfield_order(lsb_to_msb)
struct Reverse {
  unsigned a:3;
#pragma bitfield_order(msb_to_lsb)
  unsigned b:5;
};
Reverse reverse = {5, 17};

// XDK merges overlapping static initializer bits with OR, not assignment.
#pragma bitfield_order(msb_to_lsb)
struct Overlap {
  unsigned a:20;
#pragma bitfield_order(lsb_to_msb)
  unsigned b:12;
};
Overlap overlap = {0xabcde, 0x123};
Overlap zero_overlap = {0xabcde, 0};
}

static_assert(sizeof(Low) == 4, "");
static_assert(sizeof(Wide) == 8, "");
static_assert(sizeof(Mixed) == 12, "");
static_assert(sizeof(Cross) == 8, "");
static_assert(sizeof(Zero) == 8, "");
static_assert(sizeof(U) == 4, "");
static_assert(sizeof(Packed) == 6, "");

// BYTES: 0000 0000008d 00000000 00000000 0000008d
// BYTES: 0010 05000000 00000011 00090000 00000001
// BYTES: 0020 000000b1 00000005 00000011 00000005
// BYTES: 0030 12000000 8d340000 a0000088 11000005
// BYTES: 0040 bbfde000 abcde000

// LAYOUT: 3:5-7 |   unsigned int a
// LAYOUT: 3:0-4 |   unsigned int b
// LAYOUT: 0:0-2 |   unsigned int a
// LAYOUT: 3:0-4 |   unsigned int b

#pragma bitfield_order(lsb_to_msb)
struct Signed { signed a:3; signed b:5; };
// CHECK-LABEL: define {{.*}} @read_signed(
// CHECK: shl i32 {{.*}}, 24
// CHECK: ashr i32 {{.*}}, 27
extern "C" int read_signed(volatile Signed *p) { return p->b; }

// CHECK-DAG: !DIDerivedType(tag: DW_TAG_member, name: "a", {{.*}}size: 3, offset: 29, flags: DIFlagBitField
// CHECK-DAG: !DIDerivedType(tag: DW_TAG_member, name: "b", {{.*}}size: 5, offset: 24, flags: DIFlagBitField

// CV: BitSize: 3
// CV-NEXT: BitOffset: 0
// CV: BitSize: 5
// CV-NEXT: BitOffset: 3

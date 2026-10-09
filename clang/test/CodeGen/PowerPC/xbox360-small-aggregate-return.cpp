// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O2 -mllvm -verify-machineinstrs -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s

template <unsigned N> struct Bytes { unsigned char data[N]; };

Bytes<1> result1() { Bytes<1> value = {{0x12}}; return value; }
Bytes<2> result2() { Bytes<2> value = {{0x12, 0x34}}; return value; }
Bytes<3> result3() { Bytes<3> value = {{0x12, 0x34, 0x56}}; return value; }
Bytes<4> result4() { Bytes<4> value = {{0x12, 0x34, 0x56, 0x78}}; return value; }
Bytes<5> result5() { Bytes<5> value = {{0x12}}; return value; }
Bytes<6> result6() { Bytes<6> value = {{0x12}}; return value; }
Bytes<7> result7() { Bytes<7> value = {{0x12}}; return value; }
Bytes<8> result8() {
  Bytes<8> value = {{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0}};
  return value;
}
Bytes<9> result9() { Bytes<9> value = {{0x12}}; return value; }

// CHECK-LABEL: define dso_local i8 @"?result1@@
// CHECK-SAME: ()
// CHECK: ret i8
// OPT-LABEL: define dso_local {{.*}}i8 @"?result1@@
// OPT: ret i8 18
// ASM-LABEL: "?result1@@YA?AU?$Bytes@$00@@XZ":
// ASM: li 3, 18
// ASM-NEXT: blr
// CHECK-LABEL: define dso_local i16 @"?result2@@
// CHECK-SAME: ()
// CHECK: ret i16
// OPT-LABEL: define dso_local {{.*}}i16 @"?result2@@
// OPT: ret i16 4660
// ASM-LABEL: "?result2@@YA?AU?$Bytes@$01@@XZ":
// ASM: li 3, 4660
// ASM-NEXT: blr
// CHECK-LABEL: define dso_local void @"?result3@@
// CHECK-SAME: (ptr {{.*}}sret(
// CHECK-LABEL: define dso_local i32 @"?result4@@
// CHECK-SAME: ()
// CHECK: ret i32
// OPT-LABEL: define dso_local {{.*}}i32 @"?result4@@
// OPT: ret i32 305419896
// ASM-LABEL: "?result4@@YA?AU?$Bytes@$03@@XZ":
// ASM: lis 3, 4660
// ASM-NEXT: ori 3, 3, 22136
// ASM-NEXT: blr
// CHECK-LABEL: define dso_local void @"?result5@@
// CHECK-SAME: (ptr {{.*}}sret(
// CHECK-LABEL: define dso_local void @"?result6@@
// CHECK-SAME: (ptr {{.*}}sret(
// CHECK-LABEL: define dso_local void @"?result7@@
// CHECK-SAME: (ptr {{.*}}sret(
// CHECK-LABEL: define dso_local i64 @"?result8@@
// CHECK-SAME: ()
// CHECK: ret i64
// OPT-LABEL: define dso_local {{.*}}i64 @"?result8@@
// OPT: ret i64 1311768467463790320
// ASM-LABEL: "?result8@@YA?AU?$Bytes@$07@@XZ":
// ASM: lis 3, 582
// ASM-NEXT: ori 3, 3, 35535
// ASM-NEXT: rldic 3, 3, 35, 3
// ASM-NEXT: oris 3, 3, 39612
// ASM-NEXT: ori 3, 3, 57072
// ASM-NEXT: blr
// CHECK-LABEL: define dso_local void @"?result9@@
// CHECK-SAME: (ptr {{.*}}sret(

struct FloatRecord { float value; };
struct DoubleRecord { double value; };
union WordUnion { unsigned value; unsigned char bytes[4]; };

FloatRecord float_record() { FloatRecord value = {1.0f}; return value; }
DoubleRecord double_record() { DoubleRecord value = {1.0}; return value; }
WordUnion word_union() { WordUnion value; value.value = 0x12345678; return value; }

// Ordinary floating-point records and unions return their integer bit image.
// CHECK-LABEL: define dso_local i32 @"?float_record@@
// OPT-LABEL: define dso_local {{.*}}i32 @"?float_record@@
// OPT: ret i32 1065353216
// CHECK-LABEL: define dso_local i64 @"?double_record@@
// OPT-LABEL: define dso_local {{.*}}i64 @"?double_record@@
// OPT: ret i64 4607182418800017408
// CHECK-LABEL: define dso_local i32 @"?word_union@@
// OPT-LABEL: define dso_local {{.*}}i32 @"?word_union@@
// OPT: ret i32 305419896

struct Methods {
  static Bytes<4> static_result();
  Bytes<4> instance_result();
};
Bytes<4> Methods::static_result() { return result4(); }
Bytes<4> Methods::instance_result() { return result4(); }
// CHECK-LABEL: define dso_local i32 @"?static_result@Methods@@
// CHECK: call i32 @"?result4@@
// CHECK-LABEL: define dso_local void @"?instance_result@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{.*}} %this)

struct Nontrivial {
  unsigned value;
  ~Nontrivial() {}
};
Nontrivial nontrivial() { Nontrivial value = {1}; return value; }
// CHECK-LABEL: define dso_local void @"?nontrivial@@
// CHECK-SAME: (ptr {{.*}}sret(%struct.Nontrivial)

unsigned consume_results() {
  return result1().data[0] + result2().data[1] + result4().data[3] +
         result8().data[7];
}
// CHECK-LABEL: define dso_local {{.*}}i32 @"?consume_results@@
// CHECK: call i8 @"?result1@@
// CHECK: call i16 @"?result2@@
// CHECK: call i32 @"?result4@@
// CHECK: call i64 @"?result8@@

typedef Bytes<4> (*ReturnPointer)();
unsigned call_indirect(ReturnPointer function) {
  return function().data[3];
}
// CHECK-LABEL: define dso_local {{.*}}i32 @"?call_indirect@@
// CHECK: call i32 %{{[^ (]+}}()

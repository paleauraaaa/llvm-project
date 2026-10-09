// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s

template <unsigned N> struct Bytes { unsigned char data[N]; };

struct Methods {
  unsigned seed;
  Bytes<1> result1();
  Bytes<2> result2();
  Bytes<3> result3();
  Bytes<4> result4();
  Bytes<8> result8();
  Bytes<9> result9();
  virtual Bytes<16> result16();
};

Bytes<1> Methods::result1() { return {{(unsigned char)seed}}; }
Bytes<2> Methods::result2() { return {{(unsigned char)seed}}; }
Bytes<3> Methods::result3() { return {{(unsigned char)seed}}; }
Bytes<4> Methods::result4() { return {{(unsigned char)seed}}; }
Bytes<8> Methods::result8() { return {{(unsigned char)seed}}; }
Bytes<9> Methods::result9() { return {{(unsigned char)seed}}; }
Bytes<16> Methods::result16() { return {{(unsigned char)seed}}; }

// CHECK-LABEL: define dso_local void @"?result1@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)
// CHECK-LABEL: define dso_local void @"?result2@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)
// CHECK-LABEL: define dso_local void @"?result3@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)
// CHECK-LABEL: define dso_local void @"?result4@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)
// CHECK-LABEL: define dso_local void @"?result8@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)
// CHECK-LABEL: define dso_local void @"?result9@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)
// CHECK-LABEL: define dso_local void @"?result16@Methods@@
// CHECK-SAME: (ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}} %this)

Bytes<9> call_direct(Methods *object) { return object->result9(); }
// CHECK-LABEL: define dso_local void @"?call_direct@@
// CHECK: call void @"?result9@Methods@@{{[^"]*}}"(ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}})

Bytes<16> call_virtual(Methods *object) { return object->result16(); }
// CHECK-LABEL: define dso_local void @"?call_virtual@@
// CHECK: call void %{{[^ (]+}}(ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}})

typedef Bytes<9> (Methods::*MemberPointer)();
Bytes<9> call_member_pointer(Methods *object, MemberPointer member) {
  return (object->*member)();
}
// CHECK-LABEL: define dso_local void @"?call_member_pointer@@
// CHECK: call void %{{[^ (]+}}(ptr {{[^,]*}}sret({{[^)]*}}) align 1 %agg.result, ptr {{[^,]*}})

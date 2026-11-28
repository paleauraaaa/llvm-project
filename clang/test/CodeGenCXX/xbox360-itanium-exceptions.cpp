// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fc++-abi=itanium -mxbox360-libunwind -fexceptions -fcxx-exceptions -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fc++-abi=itanium -mxbox360-libunwind -fexceptions -fcxx-exceptions -O0 -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fc++-abi=itanium -mxbox360-libunwind -fexceptions -fcxx-exceptions -O2 -emit-obj -o %t.O2 %s
// RUN: llvm-readobj --relocations %t.O0 | FileCheck %s --check-prefix=RELOC
// RUN: llvm-readobj --relocations %t.O2 | FileCheck %s --check-prefix=RELOC
// RUN: not %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fc++-abi=itanium -fexceptions -fcxx-exceptions -emit-llvm -o - %s 2>&1 | FileCheck %s --check-prefix=NO-UNWIND
// NO-UNWIND: error: option '-fc++-abi=itanium' cannot be specified without '-mxbox360-libunwind'
// RELOC: IMAGE_REL_PPC_ADDR32 __gxx_personality_v0
// RELOC: IMAGE_REL_PPC_ADDR32 .gcc_except_table
// RELOC: .gcc_except_table {
// RELOC: IMAGE_REL_PPC_ADDR32 _ZTIi

struct A {
  virtual ~A();
};
A::~A() {}

extern void risky();
void test() {
  try {
    risky();
  } catch (int value) {
    throw value;
  }
}

// CHECK-DAG: @_ZTV1A =
// CHECK-DAG: @_ZTIi = external
// CHECK: define{{.*}} void @_Z4testv()
// CHECK-SAME: personality ptr @__gxx_personality_v0
// CHECK: landingpad
// CHECK: catch ptr @_ZTIi
// CHECK: call ptr @__cxa_begin_catch
// CHECK: invoke void @__cxa_throw

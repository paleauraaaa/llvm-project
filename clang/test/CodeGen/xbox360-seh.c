// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.O2 %s

unsigned long _exception_code(void);
int _abnormal_termination(void);
#pragma intrinsic(_exception_code, _abnormal_termination)
void observe(int);

// CHECK-LABEL: define {{.*}}i32 @memory(
// CHECK-SAME: personality ptr @__C_specific_handler
// CHECK: invoke void @llvm.seh.try.begin()
// CHECK: load volatile i32
// CHECK: store volatile i32
// CHECK: invoke void @llvm.seh.try.end()
// CHECK: call i32 @llvm.eh.exceptioncode(
int memory(int *p) {
  __try { ++*p; }
  __except (1) { return (int)_exception_code(); }
  return 0;
}

// CHECK-LABEL: define {{.*}}void @finalizer(
// CHECK: call ppc_xbox360_sehcc void
// CHECK: cleanuppad within none
// CHECK: call ppc_xbox360_sehcc void
void finalizer(int *p) {
  __try { ++*p; }
  __finally { observe(*p + _abnormal_termination()); }
}

// CHECK-LABEL: define {{.*}}i32 @early_return(
// CHECK: invoke void @llvm.seh.try.end()
// CHECK: invoke void @__jump_unwind(ptr {{%.*}}, ptr blockaddress(@early_return, %seh.localunwind.cont))
// CHECK-NEXT: to label %seh.localunwind.cont unwind label %ehcleanup
// CHECK: seh.localunwind.cont:
int early_return(int *p) {
  __try { return *p; }
  __finally { observe(_abnormal_termination()); }
}

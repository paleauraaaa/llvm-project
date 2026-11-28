// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --implicit-check-not=__jump_unwind
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.O2 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=KERNEL
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mno-xbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=KERNEL
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -mno-xbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=KERNEL
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mno-xbox360-libunwind -mxbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --implicit-check-not=__jump_unwind

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
// CHECK: [[DEST:%.*]] = load i32, ptr %cleanup.dest.slot
// CHECK: [[ABNORMAL:%.*]] = icmp ne i32 [[DEST]], 0
// CHECK: [[FLAG:%.*]] = zext i1 [[ABNORMAL]] to i8
// CHECK: call ppc_xbox360_sehcc void {{.*}}(i8 noundef [[FLAG]],
// KERNEL-LABEL: define {{.*}}i32 @early_return(
// KERNEL: invoke void @llvm.seh.try.end()
// KERNEL: invoke void @__jump_unwind(ptr {{%.*}}, ptr blockaddress(@early_return, %seh.localunwind.cont))
// KERNEL-NEXT: to label %seh.localunwind.cont unwind label %ehcleanup
// KERNEL: call ppc_xbox360_sehcc void {{.*}}(i8 noundef 0,
// KERNEL: seh.localunwind.cont:
int early_return(int *p) {
  __try { return *p; }
  __finally { observe(_abnormal_termination()); }
}

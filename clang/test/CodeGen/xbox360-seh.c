// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --implicit-check-not=__jump_unwind
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.O2 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -O0 -S -o - %s | FileCheck %s --check-prefix=META
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -O0 -S -o - %s | FileCheck %s --check-prefix=NOMETA --implicit-check-not=.xseh
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=KERNEL --implicit-check-not=xbox360-libunwind --implicit-check-not=llvm.eh.xbox.exceptionobject --implicit-check-not=_Unwind_DeleteException
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mno-xbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=KERNEL
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mxbox360-libunwind -mno-xbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=KERNEL
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -mno-xbox360-libunwind -mxbox360-libunwind -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --implicit-check-not=__jump_unwind

unsigned long _exception_code(void);
int _abnormal_termination(void);
#pragma intrinsic(_exception_code, _abnormal_termination)
void observe(int);

// CHECK-LABEL: define {{.*}}i32 @memory(
// CHECK-SAME: #[[LIBUNWIND:[0-9]+]] personality ptr @__C_specific_handler
// CHECK: invoke void @llvm.seh.try.begin()
// CHECK: load volatile i32
// CHECK: store volatile i32
// CHECK: invoke void @llvm.seh.try.end()
// CHECK: call i32 @llvm.eh.exceptioncode(
// CHECK: call ptr @llvm.eh.xbox.exceptionobject.p0(token
// CHECK: call void @_Unwind_DeleteException(
// META-LABEL: memory:
// META: .section .xseh
// META: .long 1481852161
// META-NEXT: .long Lxbox_begin
// META-NEXT: .long memory
// META-NEXT: .long {{[0-9]+}}
// META-NEXT: .long 12
int memory(int *p) {
  __try { ++*p; }
  __except (1) { return (int)_exception_code(); }
  return 0;
}

// CHECK-LABEL: define {{.*}}void @handler_exits(
// CHECK: call ptr @llvm.eh.xbox.exceptionobject.p0(token
// CHECK: invoke void @observe(
// CHECK: call void @_Unwind_DeleteException(
// CHECK: cleanuppad within none
// CHECK: call void @_Unwind_DeleteException(
void handler_exits(int n) {
  __try { observe(n); }
  __except (1) {
    if (n == 1)
      goto done;
    if (n == 2)
      return;
    observe(n);
  }
done:
  observe(0);
}

// CHECK-LABEL: define {{.*}}void @finalizer(
// CHECK: call ppc_xbox360_sehcc void
// CHECK: cleanuppad within none
// CHECK: call ppc_xbox360_sehcc void
// CHECK: define internal ppc_xbox360_sehcc void {{.*}} #[[HELPER:[0-9]+]]
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

// CHECK-LABEL: define {{.*}}void @nested_helper(
// CHECK: define internal ppc_xbox360_sehcc void {{.*}}nested_helper{{.*}} #[[HELPER]]
// CHECK-SAME: personality ptr @__C_specific_handler
// CHECK: cleanuppad within none
// NOMETA: nested_helper:
// META-LABEL: "?fin$0@0@nested_helper@@":
// META: .section .xseh
// META: .long 1481852161
// META-NEXT: .long Lxbox_begin
// META-NEXT: .long "?fin$0@0@nested_helper@@"
// META-NEXT: .long [[FRAME:[0-9]+]]
// META-NEXT: .long 4
// META: .section .xseh
// META: .long 1481852161
// META-NEXT: .long Lxbox_begin
// META-NEXT: .long "?fin$0@0@nested_helper@@"
// META-NEXT: .long [[FRAME]]
// META-NEXT: .long 1
void nested_helper(int *p) {
  __try { observe(*p); }
  __finally {
    __try { observe(*p); }
    __finally { observe(*p); }
  }
}

// CHECK: attributes #[[LIBUNWIND]] = { {{.*}}"xbox360-libunwind"{{.*}} }
// CHECK: attributes #[[HELPER]] = { {{.*}}"xbox360-libunwind"{{.*}} }

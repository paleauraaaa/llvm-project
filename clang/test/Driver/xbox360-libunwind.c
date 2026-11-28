// RUN: %clang --target=powerpc64-ibm-xbox360 -### -c %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang --target=powerpc64-ibm-xbox360 -mxbox360-libunwind -### -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang --target=powerpc64-ibm-xbox360 -mxbox360-libunwind -mno-xbox360-libunwind -### -c %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang --target=powerpc64-ibm-xbox360 -mno-xbox360-libunwind -mxbox360-libunwind -### -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang --target=powerpc64-ibm-xbox360 -fms-extensions -mxbox360-libunwind -S -emit-llvm -o - %s | FileCheck %s --check-prefix=DIRECT --implicit-check-not=__jump_unwind
// RUN: %clang --target=powerpc64-ibm-xbox360 -fms-extensions -mxbox360-libunwind -mno-xbox360-libunwind -S -emit-llvm -o - %s | FileCheck %s --check-prefix=KERNEL

// ON: "-cc1"
// ON-SAME: "-mxbox360-libunwind"
// OFF: "-cc1"
// OFF-NOT: "-mxbox360-libunwind"

// DIRECT-LABEL: define{{.*}} @early_return(
// DIRECT: call ppc_xbox360_sehcc void
// KERNEL-LABEL: define{{.*}} @early_return(
// KERNEL: invoke void @__jump_unwind
void observe(int);
int _abnormal_termination(void);
#pragma intrinsic(_abnormal_termination)
int early_return(int n) {
  __try { return n; }
  __finally { observe(_abnormal_termination()); }
}

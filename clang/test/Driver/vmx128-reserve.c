// REQUIRES: powerpc-registered-target
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -mvmx128-reserve -### -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -mvmx128-reserve -mno-vmx128-reserve -### -c %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -mno-vmx128-reserve -mvmx128-reserve -### -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /c -### -- %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve- /c -### -- %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /QVMXReserve- /c -### -- %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve- /QVMXReserve /c -### -- %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /clang:-mno-vmx128-reserve /c -### -- %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -mvmx128-reserve -E -dM %s | FileCheck %s --check-prefix=MACRO
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /E /clang:-dM /TC -- %s | FileCheck %s --check-prefix=MACRO
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -E -dM %s | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /E /clang:-dM /TC -- %s | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /QVMXReserve- /E /clang:-dM /TC -- %s | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -mvmx128-reserve -mno-vmx128-reserve -E -dM %s | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -fms-extensions -mvmx128-reserve -fsyntax-only -Xclang -verify=common,reserve %S/../Sema/PowerPC/vmx128-reserve.c
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /Zs /clang:-Xclang /clang:-verify=common,reserve /TC -- %S/../Sema/PowerPC/vmx128-reserve.c
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /QVMXReserve- /Zs /clang:-Xclang /clang:-verify=common /TC -- %S/../Sema/PowerPC/vmx128-reserve.c
// RUN: %clang --target=powerpc64-ibm-xbox360 -mcpu=xenon -mvmx128-reserve -S -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /QVMXReserve /clang:-S /clang:-emit-llvm /clang:-o /clang:- /TC -- %s | FileCheck %s --check-prefix=IR
// ON-NOT: warning:
// ON: "-target-feature" "+vmx128-reserve"
// ON-NOT: "-target-feature" "-vmx128-reserve"
// OFF-NOT: warning:
// OFF-NOT: "-target-feature" "+vmx128-reserve"
// OFF: "-target-feature" "-vmx128-reserve"
// MACRO: #define __VMX128_RESERVE 1
// DEFAULT-NOT: __VMX128_RESERVE
// DEFAULT: #define __VMX128__ 1
// DEFAULT-NOT: __VMX128_RESERVE
// IR: "target-features"="{{[^"]*}}+vmx128-reserve

void f(void) {}

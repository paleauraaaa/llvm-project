// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -debug-info-kind=standalone -gcodeview -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -debug-info-kind=standalone -gcodeview -gcodeview-xbox360-modern -emit-llvm %s -o - | FileCheck %s --check-prefix=MODERN
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -debug-info-kind=standalone -gcodeview -gcodeview-xbox360-modern -gno-codeview-xbox360-modern -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -debug-info-kind=standalone -gcodeview -gcodeview-xbox360-modern -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -gcodeview-xbox360-modern -emit-llvm %s -o - | FileCheck %s --check-prefix=NONE

// DEFAULT-NOT: !"CodeViewXbox360Modern"
// DEFAULT: !"CodeView", i32 1
// DEFAULT-NOT: !"CodeViewXbox360Modern"
// MODERN: !{i32 7, !"CodeViewXbox360Modern", i32 1}
// NONE-NOT: !"CodeView"
// NONE-NOT: !"CodeViewXbox360Modern"

int function(int value) { return value + 1; }

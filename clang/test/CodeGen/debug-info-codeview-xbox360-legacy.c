// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -debug-info-kind=standalone -gcodeview -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -debug-info-kind=standalone -gcodeview -gcodeview-xbox360-legacy -emit-llvm %s -o - | FileCheck %s --check-prefix=LEGACY
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -debug-info-kind=standalone -gcodeview -gcodeview-xbox360-legacy -gno-codeview-xbox360-legacy -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -debug-info-kind=standalone -gcodeview -gcodeview-xbox360-legacy -emit-llvm %s -o - | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -gcodeview-xbox360-legacy -emit-llvm %s -o - | FileCheck %s --check-prefix=NONE

// DEFAULT-NOT: !"CodeViewXbox360Legacy"
// DEFAULT: !"CodeView", i32 1
// DEFAULT-NOT: !"CodeViewXbox360Legacy"
// LEGACY: !{i32 7, !"CodeViewXbox360Legacy", i32 1}
// NONE-NOT: !"CodeView"
// NONE-NOT: !"CodeViewXbox360Legacy"

int function(int value) { return value + 1; }

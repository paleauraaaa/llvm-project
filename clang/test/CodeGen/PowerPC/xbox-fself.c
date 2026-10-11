// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -x c++ -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fno-builtin -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O0 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM-O0
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O2 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
#ifdef __cplusplus
extern "C" {
#endif
float __fself(float, float, float);
float operand(int);
// CHECK-LABEL: define{{.*}} @select_float(
// CHECK: call float @llvm.ppc.fsels(float
// ASM-LABEL: select_float:
// ASM: fsel 1, 1, 2, 3
// ASM-O0-LABEL: select_float:
// ASM-O0: fsel 1, 0, 1, 2
float select_float(float compare, float ge, float lt) {
  return __fself(compare, ge, lt);
}
// CHECK-LABEL: define{{.*}} @evaluate_once(
// CHECK: call{{.*}} float @operand(i32{{.*}} 0)
// CHECK-NOT: @operand
// CHECK: call{{.*}} float @operand(i32{{.*}} 1)
// CHECK-NOT: @operand
// CHECK: call{{.*}} float @operand(i32{{.*}} 2)
// CHECK-NOT: @operand
// CHECK: call float @llvm.ppc.fsels(float
float evaluate_once(void) {
  return __fself(operand(0), operand(1), operand(2));
}
// CHECK-LABEL: define{{.*}} @positive_zero(
// CHECK: call float @llvm.ppc.fsels(float 0.000000e+00, float 1.000000e+00, float 2.000000e+00)
float positive_zero(void) { return __fself(0.0f, 1.0f, 2.0f); }
// CHECK-LABEL: define{{.*}} @negative_zero(
// CHECK: call float @llvm.ppc.fsels(float -0.000000e+00, float 1.000000e+00, float 2.000000e+00)
float negative_zero(void) { return __fself(-0.0f, 1.0f, 2.0f); }
// CHECK-LABEL: define{{.*}} @nan_comparand(
// CHECK: call float @llvm.ppc.fsels(float 0x7FF8000000000000, float 1.000000e+00, float 2.000000e+00)
float nan_comparand(void) { return __fself(__builtin_nanf(""), 1.0f, 2.0f); }
#ifdef __cplusplus
}
#endif

// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -x c++ -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -x c++ -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fno-builtin -O2 -emit-llvm -o - %s | FileCheck %s

#ifdef __cplusplus
extern "C"
#endif
int __IsIntConst(int);
#define NUMBER 17
enum { Enumerator = 19 };
const int constant = 23;
static const int static_constant = 29;
int variable;
int function(void);

// CHECK-LABEL: define{{.*}} @{{.*}}literal
// CHECK: ret i32 1
int literal(void) { return __IsIntConst(42); }
// CHECK-LABEL: define{{.*}} @{{.*}}macro
// CHECK: ret i32 1
int macro(void) { return __IsIntConst(NUMBER); }
// CHECK-LABEL: define{{.*}} @{{.*}}enumeration
// CHECK: ret i32 1
int enumeration(void) { return __IsIntConst(Enumerator); }
// CHECK-LABEL: define{{.*}} @{{.*}}expression
// CHECK: ret i32 1
int expression(void) { return __IsIntConst((NUMBER + Enumerator) * 2); }
// CHECK-LABEL: define{{.*}} @{{.*}}const_variable
// CHECK: ret i32 0
int const_variable(void) { return __IsIntConst(constant); }
// CHECK-LABEL: define{{.*}} @{{.*}}static_variable
// CHECK: ret i32 0
int static_variable(void) { return __IsIntConst(static_constant); }
// CHECK-LABEL: define{{.*}} @{{.*}}optimized_variable
// CHECK: ret i32 0
int optimized_variable(void) { int value = 7; return __IsIntConst(value); }
// CHECK-LABEL: define{{.*}} @{{.*}}parameter
// CHECK: ret i32 0
int parameter(int value) { return __IsIntConst(value); }
// CHECK-LABEL: define{{.*}} @{{.*}}address
// CHECK: ret i32 0
int address(void) { return __IsIntConst((int)&variable); }
// CHECK-LABEL: define{{.*}} @{{.*}}function_address
// CHECK: ret i32 0
int function_address(void) { return __IsIntConst((int)&function); }
// CHECK-LABEL: define{{.*}} @{{.*}}side_effect
// CHECK-NOT: load
// CHECK-NOT: store
// CHECK: ret i32 0
int side_effect(void) { return __IsIntConst(variable++); }
// CHECK-LABEL: define{{.*}} @{{.*}}call
// CHECK-NOT: call
// CHECK: ret i32 0
int call(void) { return __IsIntConst(function()); }
// CHECK-LABEL: define{{.*}} @{{.*}}size
// CHECK: ret i32 1
int size(void) { return __IsIntConst(sizeof(variable)); }

// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s

typedef __attribute__((vector_size(16))) int vector4i;

struct VectorMethods {
  vector4i instance_vector();
  static vector4i static_vector();
};

vector4i VectorMethods::instance_vector() { return vector4i{1, 2, 3, 4}; }

vector4i VectorMethods::static_vector() { return vector4i{5, 6, 7, 8}; }

// CHECK-LABEL: define dso_local void @"?instance_vector@VectorMethods@@QAA
// CHECK-SAME: (ptr noundef nonnull align 1 dereferenceable(1) %this, ptr dead_on_unwind noalias writable sret(<4 x i32>) align 16 %agg.result)
// CHECK: ret void

// CHECK-LABEL: define dso_local noundef <4 x i32> @"?static_vector@VectorMethods@@SA
// CHECK-SAME: ()
// CHECK: ret <4 x i32>
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -Wno-deprecated-non-prototype -emit-llvm -o - %s | FileCheck %s

void old_style();

void caller(float single, double extended) {
  old_style(single, extended);
}

// CHECK-LABEL: define dso_local void @caller
// CHECK: fpext float {{.*}} to double
// CHECK: call void @old_style(double {{.*}}, double {{.*}})
// CHECK: declare dso_local void @old_style(...)
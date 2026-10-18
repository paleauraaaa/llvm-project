// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -std=c11 -mrelocation-model static -Wno-deprecated-non-prototype -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -std=c11 -target-cpu xenon -mrelocation-model static -Wno-deprecated-non-prototype -O0 -mllvm -fast-isel=false -mllvm -verify-machineinstrs -mllvm -stop-after=finalize-isel -S -o - %s | FileCheck %s --check-prefix=MIR
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -std=c11 -target-cpu xenon -mrelocation-model static -Wno-deprecated-non-prototype -O0 -mllvm -fast-isel=true -mllvm -verify-machineinstrs -mllvm -stop-after=finalize-isel -S -o - %s | FileCheck %s --check-prefix=MIR
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -std=c11 -target-cpu xenon -mrelocation-model static -Wno-deprecated-non-prototype -O2 -mllvm -fast-isel=false -mllvm -verify-machineinstrs -mllvm -stop-after=finalize-isel -S -o - %s | FileCheck %s --check-prefix=MIR
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -std=c11 -target-cpu xenon -mrelocation-model static -Wno-deprecated-non-prototype -O2 -mllvm -fast-isel=true -mllvm -verify-machineinstrs -mllvm -stop-after=finalize-isel -S -o - %s | FileCheck %s --check-prefix=MIR

void old_style();
void prototype(double, double);

// CHECK-LABEL: define dso_local void @caller
// CHECK: fpext float {{.*}} to double
// CHECK: call void (double, double, ...) @old_style(double {{.*}}, double {{.*}})
// CHECK: declare dso_local void @old_style(...)
// MIR-LABEL: name: caller
// MIR: BL @old_style, {{.*}}implicit $f1, implicit $x3, implicit $f2, implicit $x4, implicit-def $r1
void caller(float single, double extended) {
  old_style(single, extended);
}

// CHECK-LABEL: define dso_local void @indirect
// CHECK: fpext float {{.*}} to double
// CHECK: call void (double, double, ...) %{{.*}}(double {{.*}}, double {{.*}})
// MIR-LABEL: name: indirect
// MIR: BCTRL8 {{.*}}implicit $f1, implicit $x3, implicit $f2, implicit $x4, implicit-def $r1
void indirect(void (*fn)(), float single, double extended) {
  fn(single, extended);
}

// CHECK-LABEL: define dso_local void @boundary
// CHECK: call void (double, double, double, double, double, double, double, double, double, ...) @old_style(
// MIR-LABEL: name: boundary
// MIR: ST{{F?}}D {{.*}}, 80, $x1
// MIR: BL @old_style, {{.*}}implicit $f1, implicit $x3, implicit $f2, implicit $x4, implicit $f3, implicit $x5, implicit $f4, implicit $x6, implicit $f5, implicit $x7, implicit $f6, implicit $x8, implicit $f7, implicit $x9, implicit $f8, implicit $x10, implicit-def $r1
void boundary(double a, double b, double c, double d, double e, double f,
              double g, double h, double i) {
  old_style(a, b, c, d, e, f, g, h, i);
}

// CHECK-LABEL: define dso_local void @prototyped
// CHECK: call void @prototype(double {{.*}}, double {{.*}})
// MIR-LABEL: name: prototyped
// MIR: BL{{(8_NOP)?}} @prototype, {{.*}}implicit $f1, implicit $f2{{(, implicit-def \$r1)?$}}
void prototyped(double a, double b) {
  prototype(a, b);
}
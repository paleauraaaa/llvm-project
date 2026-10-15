// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fcxx-exceptions -fexceptions -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,XBOX --implicit-check-not=__CxxFrameHandler3 --implicit-check-not=landingpad
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fcxx-exceptions -fexceptions -O2 -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,XBOX --implicit-check-not=__CxxFrameHandler3 --implicit-check-not=landingpad
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fcxx-exceptions -fexceptions -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,WINDOWS --implicit-check-not=landingpad

void may_throw();
struct Guard {
  ~Guard();
};

// CHECK-LABEL: define {{.*}} @{{.*}}catch_int
// XBOX-SAME: personality ptr @__CxxFrameHandler {
// WINDOWS-SAME: personality ptr @__CxxFrameHandler3 {
// CHECK: invoke void
// CHECK: catchswitch within none
// CHECK: catchpad within
// CHECK: catchret from
int catch_int() {
  try {
    may_throw();
  } catch (int value) {
    return value;
  }
  return 0;
}

// CHECK-LABEL: define {{.*}} @{{.*}}cleanup
// XBOX-SAME: personality ptr @__CxxFrameHandler {
// WINDOWS-SAME: personality ptr @__CxxFrameHandler3 {
// CHECK: invoke void
// CHECK: cleanuppad within none
// CHECK: "funclet"(token
// CHECK: cleanupret from
void cleanup() {
  Guard guard;
  may_throw();
}

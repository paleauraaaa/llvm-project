// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fcxx-exceptions -fexceptions -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,XBOX
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fcxx-exceptions -fexceptions -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,WINDOWS

struct Object { int value; };
void may_throw();

// CHECK-LABEL: define {{.*}} @const_reference(
// XBOX: catchpad within %{{.*}} [ptr @"??_R0?AUObject@@@8", i32 9,
// WINDOWS: catchpad within %{{.*}} [ptr @"??_R0?AUObject@@@8", i32 8,
extern "C" int const_reference() {
  try { may_throw(); }
  catch (const Object &v) { return v.value; }
  return 0;
}

// CHECK-LABEL: define {{.*}} @volatile_reference(
// XBOX: catchpad within %{{.*}} [ptr @"??_R0?AUObject@@@8", i32 10,
// WINDOWS: catchpad within %{{.*}} [ptr @"??_R0?AUObject@@@8", i32 8,
extern "C" int volatile_reference() {
  try { may_throw(); }
  catch (volatile Object &v) { return v.value; }
  return 0;
}

// CHECK-LABEL: define {{.*}} @cv_reference(
// XBOX: catchpad within %{{.*}} [ptr @"??_R0?AUObject@@@8", i32 11,
// WINDOWS: catchpad within %{{.*}} [ptr @"??_R0?AUObject@@@8", i32 8,
extern "C" int cv_reference() {
  try { may_throw(); }
  catch (const volatile Object &v) { return v.value; }
  return 0;
}

// CHECK-LABEL: define {{.*}} @cv_pointer(
// CHECK: catchpad within %{{.*}} [ptr @"??_R0P{{A|EA}}UObject@@@8", i32 3,
extern "C" int cv_pointer() {
  try { may_throw(); }
  catch (const volatile Object *v) { return v->value; }
  return 0;
}

// CHECK-LABEL: define {{.*}} @const_pointer_reference(
// XBOX: catchpad within %{{.*}} [ptr @"??_R0PAUObject@@@8", i32 8,
// WINDOWS: catchpad within %{{.*}} [ptr @"??_R0PEAUObject@@@8", i32 9,
extern "C" int const_pointer_reference() {
  try { may_throw(); }
  catch (const Object *&v) { return v->value; }
  return 0;
}

// CHECK-LABEL: define {{.*}} @reference_to_const_pointer(
// XBOX: catchpad within %{{.*}} [ptr @"??_R0PAUObject@@@8", i32 9,
// WINDOWS: catchpad within %{{.*}} [ptr @"??_R0PEAUObject@@@8", i32 8,
extern "C" int reference_to_const_pointer() {
  try { may_throw(); }
  catch (Object *const &v) { return v->value; }
  return 0;
}

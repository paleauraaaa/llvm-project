// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.O2 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.exceptions %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s

struct Guard {
  Guard();
  ~Guard();
};
void work();
void use(const Guard &);
void use_default(const Guard & = Guard());

// CHECK-LABEL: define {{.*}} @in_try(
// CHECK: {{call|invoke}} {{.*}} @"??0Guard@@
// CHECK-NOT: @llvm.seh.try.end
// CHECK: {{call|invoke}} void @"??1Guard@@
// CHECK: invoke void @llvm.seh.try.end()
// CHECK-NOT: cleanuppad
extern "C" void in_try() {
  __try { Guard g; work(); } __except (1) {}
}

// CHECK-LABEL: define {{.*}} @in_except(
// CHECK: {{call|invoke}} {{.*}} @"??0Guard@@
// CHECK: call void @"??1Guard@@
// CHECK-NOT: cleanuppad
extern "C" void in_except() {
  __try { work(); } __except (1) { Guard g; work(); }
}

// CHECK-LABEL: define {{.*}} @after(
// CHECK: {{call|invoke}} {{.*}} @"??0Guard@@
// CHECK: call void @"??1Guard@@
// CHECK-NOT: cleanuppad
extern "C" void after() {
  __try { work(); } __except (1) {}
  Guard g;
}

// CHECK-LABEL: define {{.*}} @temporary(
// CHECK: {{call|invoke}} {{.*}} @"??0Guard@@
// CHECK: call void @"??1Guard@@
// CHECK-NOT: cleanuppad
extern "C" void temporary() {
  __try { work(); } __except (1) {}
  use(Guard());
}

// CHECK-LABEL: define {{.*}} @default_temporary(
// CHECK: {{call|invoke}} {{.*}} @"??0Guard@@
// CHECK: call void @"??1Guard@@
// CHECK-NOT: cleanuppad
extern "C" void default_temporary() {
  __try { work(); } __except (1) {}
  use_default();
}

// CHECK-LABEL: define {{.*}} @extended_temporary(
// CHECK-NOT: cleanuppad
// CHECK: ret void
extern "C" void extended_temporary() {
  __try { work(); } __except (1) {}
  const Guard &g = Guard();
  use(g);
}

// CHECK-LABEL: define {{.*}} @array(
// CHECK-NOT: cleanuppad
// CHECK: ret void
extern "C" void array() {
  __try { Guard g[2]; work(); } __except (1) {}
}

// CHECK-LABEL: define {{.*}} @in_finally(
extern "C" void in_finally() {
  __try { work(); } __finally { Guard g; work(); }
}

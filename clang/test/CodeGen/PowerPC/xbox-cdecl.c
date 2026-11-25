// REQUIRES: powerpc-registered-target
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -Werror -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -Werror -emit-obj -o %t.o %s

int __cdecl target(int value) { return value + 1; }
int __cdecl formatted(const char *, ...);
typedef int (__cdecl *callback)(int);

int invoke(callback fn, const char *format) {
  return target(1) + fn(2) + formatted(format, 3);
}

// CHECK-LABEL: define dso_local signext i32 @target(i32
// CHECK-LABEL: define dso_local signext i32 @invoke(ptr
// CHECK: call signext i32 @target(i32{{.*}} 1)
// CHECK: call signext i32 %{{.*}}(i32{{.*}} 2)
// CHECK: call signext i32 (ptr, ...) @formatted(ptr{{.*}}, i32{{.*}} 3)

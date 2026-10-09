// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.obj %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O2 -mllvm -verify-machineinstrs -S -o - %s | FileCheck %s --check-prefix=ASM

struct B3 { unsigned char bytes[3]; };
struct B4 { unsigned char bytes[4]; };
struct B9 { unsigned char bytes[9]; };
struct B17 { unsigned char bytes[17]; };
struct __attribute__((aligned(32))) A32 { unsigned char bytes[9]; };

// Odd-sized small aggregates are left-justified, not moved forward by five.
// CHECK-LABEL: define {{.*}} @read3(
// CHECK: [[CUR3:%.*]] = load ptr, ptr {{.*}}
// CHECK: getelementptr inbounds i8, ptr [[CUR3]], i32 8
// CHECK-NOT: getelementptr inbounds i8, ptr [[CUR3]], i32 5
// CHECK: call void @llvm.memcpy{{.*}}ptr align 8 [[CUR3]], i32 3
unsigned read3(unsigned marker, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, marker);
  struct B3 value = __builtin_va_arg(ap, struct B3);
  __builtin_va_end(ap);
  return value.bytes[0];
}

// Four-byte aggregates are right-justified within their eight-byte slot.
// CHECK-LABEL: define {{.*}} @read4(
// CHECK: [[CUR4:%.*]] = load ptr, ptr {{.*}}
// CHECK: getelementptr inbounds i8, ptr [[CUR4]], i32 8
// CHECK: [[ADDR4:%.*]] = getelementptr inbounds i8, ptr [[CUR4]], i32 4
// CHECK: call void @llvm.memcpy{{.*}}ptr align 4 [[ADDR4]], i32 4
unsigned read4(unsigned marker, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, marker);
  struct B4 value = __builtin_va_arg(ap, struct B4);
  __builtin_va_end(ap);
  return value.bytes[0];
}

// Nine-byte ellipsis aggregates advance by 16 without aligning to 16.
// CHECK-LABEL: define {{.*}} @read9(
// CHECK: [[CUR9:%.*]] = load ptr, ptr {{.*}}
// CHECK: getelementptr inbounds i8, ptr [[CUR9]], i32 16
// CHECK: call void @llvm.memcpy{{.*}}ptr align 8 [[CUR9]], i32 9
unsigned read9(unsigned marker, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, marker);
  struct B9 value = __builtin_va_arg(ap, struct B9);
  __builtin_va_end(ap);
  return value.bytes[8];
}

// Seventeen-byte aggregates advance by 24, not 32.
// CHECK-LABEL: define {{.*}} @read17(
// CHECK: [[CUR17:%.*]] = load ptr, ptr {{.*}}
// CHECK: getelementptr inbounds i8, ptr [[CUR17]], i32 24
// CHECK: call void @llvm.memcpy{{.*}}ptr align 8 [[CUR17]], i32 17
unsigned read17(unsigned marker, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, marker);
  struct B17 value = __builtin_va_arg(ap, struct B17);
  __builtin_va_end(ap);
  return value.bytes[16];
}

// Natural over-alignment must not realign the ellipsis cursor.
// CHECK-LABEL: define {{.*}} @read_aligned(
// CHECK: [[CURA:%.*]] = load ptr, ptr {{.*}}
// CHECK: getelementptr inbounds i8, ptr [[CURA]], i32 32
// CHECK: call void @llvm.memcpy{{.*}}ptr align 8 [[CURA]], i32 32
unsigned read_aligned(unsigned marker, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, marker);
  struct A32 value = __builtin_va_arg(ap, struct A32);
  __builtin_va_end(ap);
  return value.bytes[8];
}

// The first ellipsis value follows the padded named aggregate at +48 and
// arrives in R6, not R7. Check this through the frontend as well as LLVM IR.
// CHECK-LABEL: define {{.*}} @read_named_tail(
// CHECK: call void @llvm.va_start
// ASM-LABEL: read_named_tail:
// ASM-DAG: std 6, 48(1)
// ASM-DAG: std 10, 80(1)
// ASM: blr
unsigned long long read_named_tail(unsigned long long lead, struct B9 value,
                                  ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, value);
  unsigned long long tail = __builtin_va_arg(ap, unsigned long long);
  __builtin_va_end(ap);
  return tail;
}

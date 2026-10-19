// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.30 -ffreestanding -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.30 -ffreestanding -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple thumbv7-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.30 -ffreestanding -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple riscv64-unknown-linux-gnu -fms-extensions -DEXPLICIT_DECLS -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple riscv64-unknown-linux-gnu -fms-extensions -ffreestanding -fno-builtin -DEXPLICIT_DECLS -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.30 -ffreestanding -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.30 -ffreestanding -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT
// RUN: %clang_cc1 -triple aarch64-pc-windows-msvc -fms-extensions -fms-compatibility -fms-compatibility-version=19.30 -ffreestanding -O2 -S -o - %s | FileCheck %s --check-prefix=ASM

#ifdef EXPLICIT_DECLS
void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);
#else
#include <intrin0.h>
#endif

// CHECK-LABEL: define {{.*}}void @read_barrier(
// CHECK: fence syncscope("singlethread") seq_cst
// CHECK-NEXT: ret void
void read_barrier(void) { _ReadBarrier(); }

// CHECK-LABEL: define {{.*}}void @write_barrier(
// CHECK: fence syncscope("singlethread") seq_cst
// CHECK-NEXT: ret void
void write_barrier(void) { _WriteBarrier(); }

// CHECK-LABEL: define {{.*}}void @read_write_barrier(
// CHECK: fence syncscope("singlethread") seq_cst
// CHECK-NEXT: ret void
void read_write_barrier(void) { _ReadWriteBarrier(); }

// OPT-LABEL: define {{.*}}i32 @preserve_reads(
// OPT: load i32, ptr
// OPT: fence syncscope("singlethread") seq_cst
// OPT: load i32, ptr
// OPT: ret i32
int preserve_reads(int *p) {
  int before = *p;
  _ReadBarrier();
  return before + *p;
}

// OPT-LABEL: define {{.*}}void @preserve_writes(
// OPT: store i32 1, ptr
// OPT: fence syncscope("singlethread") seq_cst
// OPT: store i32 2, ptr
// OPT: ret void
void preserve_writes(int *p) {
  *p = 1;
  _WriteBarrier();
  *p = 2;
}

// ASM-LABEL: read_barrier:
// ASM-NOT: bl
// ASM-NOT: dmb
// ASM-NOT: dsb
// ASM: ret
// ASM-LABEL: write_barrier:
// ASM-NOT: bl
// ASM-NOT: dmb
// ASM-NOT: dsb
// ASM: ret
// ASM-LABEL: read_write_barrier:
// ASM-NOT: bl
// ASM-NOT: dmb
// ASM-NOT: dsb
// ASM: ret

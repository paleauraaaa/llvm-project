// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fdeclspec -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fdeclspec -O0 -mllvm -verify-machineinstrs -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fdeclspec -O2 -mllvm -verify-machineinstrs -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fdeclspec -funwind-tables=2 -mllvm -verify-machineinstrs -emit-obj -o %t.obj %s
// RUN: llvm-readobj --sections %t.obj | FileCheck %s --check-prefix=SECTIONS --implicit-check-not=.pdata --implicit-check-not=.xdata

// IR-LABEL: define {{.*}}void @declared(
// IR-SAME: i32 {{.*}}) #[[NAKED:[0-9]+]]
// IR-NOT: alloca
// IR-NOT: store
// IR: call void asm sideeffect "blr"
// IR-NEXT: unreachable
// ASM-LABEL: declared:
// ASM-NOT: mflr
// ASM-NOT: stw
// ASM-NOT: std
// ASM-NOT: stdu
// ASM: #APP
// ASM-NEXT: blr
// ASM-NEXT: #NO_APP
// ASM-NOT: blr
__declspec(naked) void declared(int);
void declared(int value) {
  __asm__("blr");
}

// IR-LABEL: define {{.*}}void @empty(
// IR-SAME: i32 {{.*}}) #[[NAKED]]
// IR-NOT: alloca
// IR-NOT: store
// IR: unreachable
// ASM-LABEL: empty:
// ASM-NOT: mflr
// ASM-NOT: stw
// ASM-NOT: std
// ASM-NOT: blr
__declspec(naked) void empty(int value) {}

// IR: attributes #[[NAKED]] = { naked noinline
// SECTIONS: Name: .text

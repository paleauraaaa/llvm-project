; RUN: llc -mtriple=ppc64-ibm-xbox360 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=ppc64-ibm-xbox360 -relocation-model=static -verify-machineinstrs %s -o - | FileCheck %s
; RUN: not llc -mtriple=ppc64-ibm-xbox360 -relocation-model=pic %s -o - 2>&1 | FileCheck %s --check-prefix=ERROR
; RUN: llc -mtriple=ppc64-ibm-xbox360 %s -filetype=obj -o %t.obj
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=COFF

; Xbox images are fixed-address. These are object-file fixups resolved by the
; linker, not image-base relocations applied by the loader.
; ERROR: Xbox 360 only supports the static relocation model
; COFF: IMAGE_REL_PPC_REFHI g
; COFF-NEXT: {{.*}}IMAGE_REL_PPC_PAIR
; COFF-NEXT: {{.*}}IMAGE_REL_PPC_REFLO g

@g = external global i32

; CHECK-LABEL: address:
; CHECK-NOT: mflr
; CHECK: lis {{[0-9]+}}, g@ha
; CHECK: la {{[0-9]+}}, g@l({{[0-9]+}})
; CHECK: blr
define ptr @address() {
  ret ptr @g
}

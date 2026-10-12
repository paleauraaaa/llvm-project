; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t
; RUN: llvm-readobj --sections --section-data %t | FileCheck %s --check-prefix=OBJ

define void @saved() nounwind {
  call void asm sideeffect "", "~{r14},~{f14},~{v64}"()
  ret void
}

; ASM-LABEL: saved:
; ASM: Lxbox_begin{{[0-9]+}}:
; ASM: li [[INDEX:[0-9]+]], {{-?[0-9]+}}
; ASM-DAG: std 14, {{-?[0-9]+}}(1)
; ASM-DAG: stfd 14, {{-?[0-9]+}}(1)
; ASM: stvx128 64, 1, [[INDEX]]
; ASM-NEXT: Lxbox_prologue_end{{[0-9]+}}:
; ASM: blr
; ASM-NEXT: Lxbox_end{{[0-9]+}}:

; OBJ: Name: .pdata
; OBJ-NOT: Name:
; OBJ: RawDataSize: 8
; OBJ: SectionData (
; OBJ-NEXT: 0000: 00000000 40000804

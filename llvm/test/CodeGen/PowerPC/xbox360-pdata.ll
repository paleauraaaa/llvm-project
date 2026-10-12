; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj < %s -o %t
; RUN: llvm-readobj --sections --relocations --section-data %t | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon < %s -o %t.s
; RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t.s -o %t.roundtrip
; RUN: llvm-readobj --sections --relocations --section-data %t.roundtrip | FileCheck %s --check-prefix=OBJ

; Records do not depend on uwtable or exceptions being enabled.
define void @leaf() nounwind {
  ret void
}

; ASM-LABEL: leaf:
; ASM: [[LEAF_BEGIN:Lxbox_begin[0-9]+]]:
; ASM-NEXT: [[LEAF_PROLOGUE:Lxbox_prologue_end[0-9]+]]:
; ASM: blr
; ASM-NEXT: [[LEAF_END:Lxbox_end[0-9]+]]:
; ASM: .section .pdata,"dr"
; ASM: .long [[LEAF_BEGIN]]
; ASM-NEXT: .long (0x40000000 | (([[LEAF_END]] - [[LEAF_BEGIN]]) / 4 << 8) | (([[LEAF_PROLOGUE]] - [[LEAF_BEGIN]]) / 4))

define void @caller() nounwind {
  call void @external()
  ret void
}

; ASM-LABEL: caller:
; ASM: [[CALL_BEGIN:Lxbox_begin[0-9]+]]:
; ASM: mflr 12
; ASM-NEXT: stw 12, -8(1)
; ASM-NEXT: stwu 1, -96(1)
; ASM-NEXT: [[CALL_PROLOGUE:Lxbox_prologue_end[0-9]+]]:
; ASM-NEXT: bl external
; ASM: [[CALL_END:Lxbox_end[0-9]+]]:
; ASM: .section .pdata,"dr"
; ASM: .long [[CALL_BEGIN]]
; ASM-NEXT: .long (0x40000000 | (([[CALL_END]] - [[CALL_BEGIN]]) / 4 << 8) | (([[CALL_PROLOGUE]] - [[CALL_BEGIN]]) / 4))

$associated = comdat any
define linkonce_odr void @associated() nounwind comdat {
  call void @external()
  ret void
}

; ASM-LABEL: associated:
; ASM: .section .pdata,"dr",associative,associated

; Assembly-only naked functions are responsible for their own unwind metadata.
define void @naked() naked nounwind {
  call void asm sideeffect "blr", ""()
  unreachable
}
; ASM-LABEL: naked:
; ASM-NOT: xbox_begin
; ASM-NOT: .pdata

declare void @external()

; leaf: one instruction, no prologue.
; caller: eight instructions, three-instruction prologue.
; OBJ: Name: .pdata
; OBJ-NOT: Name:
; OBJ: RawDataSize: 16
; OBJ: Characteristics [ (0x40300040)
; OBJ: SectionData (
; OBJ-NEXT: 0000: 00000000 40000100 00000010 40000803
; OBJ: Name: .pdata
; OBJ-NOT: Name:
; OBJ: RawDataSize: 8
; OBJ: Characteristics [ (0x40301040)
; OBJ: SectionData (
; OBJ-NEXT: 0000: 00000000 40000803
; OBJ: IMAGE_REL_PPC_ADDR32 .text

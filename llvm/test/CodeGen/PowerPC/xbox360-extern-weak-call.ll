; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=false -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=true -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -filetype=obj < %s -o %t.obj
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=RELOC

declare extern_weak i32 @weak_hook(i32)
declare i32 @strong_hook(i32)

define i32 @guarded(i32 %value) {
  %present = icmp ne ptr @weak_hook, null
  br i1 %present, label %call, label %absent

call:
  %result = call i32 @weak_hook(i32 %value)
  ret i32 %result

absent:
  ret i32 -1
}

; CHECK-LABEL: guarded:
; CHECK: weak_hook@ha
; CHECK: weak_hook@l
; CHECK: mtctr
; CHECK: bctrl

define i32 @direct(i32 %value) {
  %result = call i32 @weak_hook(i32 %value)
  ret i32 %result
}

; CHECK-LABEL: direct:
; CHECK: weak_hook@ha
; CHECK: weak_hook@l
; CHECK: mtctr
; CHECK: bctrl

define i32 @tail(i32 %value) {
  %result = musttail call i32 @weak_hook(i32 %value)
  ret i32 %result
}

; CHECK-LABEL: tail:
; CHECK: weak_hook@ha
; CHECK: weak_hook@l
; CHECK: mtctr
; CHECK: bctr

define i32 @strong(i32 %value) {
  %result = call i32 @strong_hook(i32 %value)
  ret i32 %result
}

; CHECK-LABEL: strong:
; CHECK: bl strong_hook

; RELOC-NOT: IMAGE_REL_PPC_REL24 weak_hook
; RELOC: IMAGE_REL_PPC_REFHI weak_hook
; RELOC-NOT: IMAGE_REL_PPC_REL24 weak_hook
; RELOC: IMAGE_REL_PPC_REL24 strong_hook
; RELOC-NOT: IMAGE_REL_PPC_REL24 weak_hook

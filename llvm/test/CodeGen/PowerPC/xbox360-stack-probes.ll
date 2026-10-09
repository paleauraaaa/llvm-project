; RUN: llc -mtriple=ppc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=ppc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -filetype=obj -o %t.obj
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=COFF
; RUN: llc -mtriple=ppc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

; COFF: IMAGE_REL_PPC_REL24 _RtlCheckStack12
declare void @escape(ptr)

; The final stack update probes frames of exactly one page.
; CHECK-LABEL: one_page:
; CHECK-NOT: _RtlCheckStack12
; CHECK-NOT: ld 12, -4096(1)
; CHECK: stwu 1, -4096(1)
; CHECK: blr
define void @one_page() {
  %p = alloca [4080 x i8], align 16
  %end = getelementptr i8, ptr %p, i32 4079
  store volatile i8 1, ptr %end
  ret void
}

; One byte beyond a page needs an intermediate probe.
; CHECK-LABEL: over_page:
; CHECK: ld 12, -4096(1)
; CHECK-NEXT: stwu 1, -4112(1)
; CHECK-NOT: _RtlCheckStack12
; CHECK: blr
define void @over_page() {
  %p = alloca [4081 x i8], align 16
  %end = getelementptr i8, ptr %p, i32 4080
  store volatile i8 1, ptr %end
  ret void
}

; Moderate frames use bounded inline page reads, not a runtime call.
; CHECK-LABEL: inline_pages:
; CHECK: ld 12, -4096(1)
; CHECK-NEXT: ld 12, -8192(1)
; CHECK-NEXT: stwu 1, -8208(1)
; CHECK-NOT: _RtlCheckStack12
; CHECK: blr
define void @inline_pages() {
  %p = alloca [8192 x i8], align 16
  %end = getelementptr i8, ptr %p, i32 8191
  store volatile i8 1, ptr %end
  ret void
}

; Even a leaf must save LR and reserve the call area for the helper.
; CHECK-LABEL: helper_leaf:
; CHECK: mflr 12
; CHECK-NEXT: stw 12, -8(1)
; CHECK: li 12, -{{[0-9]+}}
; CHECK-NEXT: bl _RtlCheckStack12
; CHECK-NEXT: stwu 1, -24096(1)
; CHECK: lwz 12, -8(1)
; CHECK: mtlr 12
; CHECK: blr
define i32 @helper_leaf(i32 %value) {
  %p = alloca [24000 x i8], align 16
  %end = getelementptr i8, ptr %p, i32 23999
  store volatile i8 1, ptr %end
  ret i32 %value
}

; CHECK-LABEL: large_nonleaf:
; CHECK: stw 12, -8(1)
; CHECK: lis 12, -1
; CHECK: ori 12, 12,
; CHECK-NEXT: bl _RtlCheckStack12
; CHECK-NEXT: stwux 1, 1, 12
; CHECK: bl escape
; CHECK: lwz 1, 0(1)
; CHECK: lwz 12, -8(1)
; CHECK: blr
define void @large_nonleaf() {
  %p = alloca [40000 x i8], align 16
  call void @escape(ptr %p)
  ret void
}

; Probe the alignment-adjusted displacement, then reload the 32-bit backchain.
; CHECK-LABEL: aligned_alloca:
; CHECK: stw 12, -8(1)
; CHECK: mr 31, 1
; CHECK: and [[SIZE:[0-9]+]],
; CHECK: mr 12, [[SIZE]]
; CHECK-NEXT: bl _RtlCheckStack12
; CHECK-NEXT: lwz [[CHAIN:[0-9]+]], 0(1)
; CHECK-NEXT: stwux [[CHAIN]], 1, 12
; CHECK: bl escape
; CHECK: blr
; MIR-LABEL: name: aligned_alloca
; MIR: XBOX_PROBED_ALLOCA
; MIR-SAME: implicit-def {{(dead )?}}$x0
; MIR-SAME: implicit-def {{(dead )?}}$x11
; MIR-SAME: implicit-def {{(dead )?}}$x12
; MIR-SAME: implicit-def {{(dead )?}}$lr
; MIR-SAME: implicit-def {{(dead )?}}$ctr
; MIR-SAME: implicit-def {{(dead )?}}$cr0
define void @aligned_alloca(i32 %n) {
  %fixed = alloca [32 x i8], align 32
  %p = alloca i8, i32 %n, align 32
  call void @escape(ptr %fixed)
  call void @escape(ptr %p)
  ret void
}

; Arguments and floating/vector values remain live across the special call.
; CHECK-LABEL: live_arguments:
; CHECK: bl _RtlCheckStack12
; CHECK: bl escape
; CHECK: blr
define <4 x float> @live_arguments(i32 %n, i64 %a, i64 %b, double %f,
                                   <4 x float> %v) {
  %p = alloca i8, i32 %n, align 16
  store volatile i64 %a, ptr %p, align 8
  %q = getelementptr i8, ptr %p, i32 8
  store volatile i64 %b, ptr %q, align 8
  %r = getelementptr i8, ptr %p, i32 16
  store volatile double %f, ptr %r, align 8
  call void @escape(ptr %p)
  ret <4 x float> %v
}

; Alignment slack must be probed before the indexed fixed-frame allocation.
; CHECK-LABEL: aligned_fixed:
; CHECK: stw 12, -8(1)
; CHECK: clrldi 12, 1,
; CHECK: subfic 12, 12,
; CHECK: bl _RtlCheckStack12
; CHECK-NEXT: stwux 1, 1, 12
; CHECK: bl escape
; CHECK: blr
define void @aligned_fixed() {
  %p = alloca [4096 x i8], align 256
  call void @escape(ptr %p)
  ret void
}

; No user call is needed to force LR preservation around a dynamic probe.
; CHECK-LABEL: dynamic_leaf:
; CHECK: mflr 12
; CHECK: stw 12, -8(1)
; CHECK: bl _RtlCheckStack12
; CHECK-NEXT: lwz [[LEAF_CHAIN:[0-9]+]], 0(1)
; CHECK-NEXT: stwux [[LEAF_CHAIN]], 1, 12
; CHECK: addi 3, 1, 80
; CHECK: lwz 12, -8(1)
; CHECK: mtlr 12
; CHECK: blr
define ptr @dynamic_leaf(i32 %n) {
  %p = alloca i8, i32 %n, align 16
  store volatile i8 7, ptr %p
  ret ptr %p
}

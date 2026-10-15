; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O1 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs %s -o - | FileCheck %s

; Moving hardware-loop preparation before WinEHPrepare must retain CTR loops.
; CHECK-LABEL: count_loop:
; CHECK: mtctr
; CHECK: bdnz
define void @count_loop(ptr %p, i32 %count) {
entry:
  %empty = icmp eq i32 %count, 0
  br i1 %empty, label %return, label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %next, %loop ]
  %element = getelementptr i32, ptr %p, i32 %i
  %value = load volatile i32, ptr %element, align 4
  %updated = add i32 %value, 1
  store volatile i32 %updated, ptr %element, align 4
  %next = add nuw i32 %i, 1
  %done = icmp eq i32 %next, %count
  br i1 %done, label %return, label %loop
return:
  ret void
}

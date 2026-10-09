; RUN: llc -mtriple=ppc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=ppc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -filetype=obj -o %t.obj

declare void @consume(ptr, ptr, ptr, i64)

; The frame pointer must be established after allocating the fixed frame.
; Multiple allocas must retain the incoming SP in each 32-bit backchain.
; CHECK-LABEL: multiple_allocas:
; CHECK: std 31, -16(1)
; CHECK-NOT: mr 31, 1
; CHECK: stwu 1, -[[#FRAME:]](1)
; CHECK: mr 31, 1
; CHECK: ld 6, [[#FRAME+80]](31)
; CHECK: addi [[CHAIN1:[0-9]+]], 31, [[#FRAME]]
; CHECK: stwux [[CHAIN1]], 1,
; CHECK: addi {{[0-9]+}}, 1, 80
; CHECK: addi [[CHAIN2:[0-9]+]], 31, [[#FRAME]]
; CHECK: stwux [[CHAIN2]], 1,
; CHECK: addi {{[0-9]+}}, 1, 80
; CHECK: stb {{[0-9]+}}, [[#LOCAL:]](31)
; CHECK: bl consume
; CHECK: lbz 3, [[#LOCAL]](31)
; CHECK: lwz 1, 0(1)
; CHECK: lwz 12, -8(1)
; CHECK: ld 31, -16(1)
; CHECK: blr
define i8 @multiple_allocas(i32 %n, i64 %a, i64 %b, i64 %c, i64 %d,
                           i64 %e, i64 %f, i64 %g, i64 %stack_arg) {
  %fixed = alloca [64 x i8], align 8
  %first = alloca i8, i32 %n, align 8
  %second_size = add i32 %n, 17
  %second = alloca i8, i32 %second_size, align 8
  store volatile i8 7, ptr %fixed, align 8
  store volatile i8 1, ptr %first, align 8
  store volatile i8 2, ptr %second, align 8
  call void asm sideeffect "", "~{r27},~{r28},~{r29},~{r30}"()
  call void @consume(ptr %fixed, ptr %first, ptr %second, i64 %stack_arg)
  %value = load volatile i8, ptr %fixed, align 8
  ret i8 %value
}

; Large fixed frames use the backchain rather than a 16-bit displacement.
; CHECK-LABEL: large_dynamic:
; CHECK: std 31, -16(1)
; CHECK-NOT: mr 31, 1
; CHECK: stwux 1, 1,
; CHECK: mr 31, 1
; CHECK: lwz [[CHAIN:[0-9]+]], 0(1)
; CHECK: stwux [[CHAIN]], 1,
; CHECK: bl consume
; CHECK: lwz 1, 0(1)
; CHECK: ld 31, -16(1)
; CHECK: blr
define void @large_dynamic(i32 %n) {
  %fixed = alloca [40000 x i8], align 16
  %p = alloca i8, i32 %n, align 16
  call void asm sideeffect "", "~{r27},~{r28},~{r29},~{r30}"()
  call void @consume(ptr %fixed, ptr %p, ptr %p, i64 0)
  ret void
}

; Realigned frames also establish FP after the indexed stack update.
; CHECK-LABEL: aligned_dynamic:
; CHECK: std 31, -16(1)
; CHECK-NOT: mr 31, 1
; CHECK: stwux 1, 1,
; CHECK: mr 31, 1
; CHECK: lwz [[ALIGNED_CHAIN:[0-9]+]], 0(1)
; CHECK: stwux [[ALIGNED_CHAIN]], 1,
; CHECK: bl consume
; CHECK: mr 1, 30
; CHECK: ld 31, -16(1)
; CHECK: blr
define void @aligned_dynamic(i32 %n) {
  %fixed = alloca [64 x i8], align 32
  %p = alloca i8, i32 %n, align 16
  call void asm sideeffect "", "~{r27},~{r28},~{r29}"()
  call void @consume(ptr %fixed, ptr %p, ptr %p, i64 0)
  ret void
}

; This test verifies that prologue/epilogue sequences for the Xbox 360 (Xenon)
; calling convention follow the spec: use stwu/stwux to allocate the frame (only R1 updates),
; save nonvolatile state with stw/std/stfd/stvx or via millicode calls, and restore LR before
; returning.
;
; RUN: llc -mtriple=ppc64-ibm-xbox360 %s -o - | FileCheck %s

target triple = "ppc64-ibm-xbox360"

%S = type { i8, i64 }

define void @test(i32 %a32, i64 %a64, float %af, double %ad, <4 x float> %v) {
entry:
  %b32 = alloca i32, align 4
  %b64 = alloca i64, align 8
  %bf = alloca float, align 4
  %bd = alloca double, align 8
  %av = alloca <4 x float>, align 16
  store i32 %a32, ptr %b32
  store i64 %a64, ptr %b64
  store float %af, ptr %bf
  store double %ad, ptr %bd
  store <4 x float> %v, ptr %av
  ; Force non-leaf to ensure outgoing area is created
  call void @callee()
  ret void
}

; CHECK-LABEL: test:
; CHECK: mflr 12
; CHECK-NEXT: stwu 1, -144(1)
; CHECK-NEXT: addi 5, 1, 96
; CHECK-NEXT: std 12, 136(1)
; CHECK: stvx 1, 0, 3
; CHECK: bl callee
; CHECK-NEXT: addi 1, 1, 144
; CHECK-NEXT: ld 12, -8(1)
; CHECK-NEXT: mtlr 12
; CHECK-NEXT: blr

define i64 @test_leaf(i64 %a, i64 %b) {
entry:
  %x = add i64 %a, %b
  ret i64 %x
}

; CHECK-LABEL: test_leaf:
; CHECK: add 3, 3, 4
; CHECK-NOT: stwu 1,
; CHECK-NEXT: blr

define void @test_struct(%S %s) {
entry:
  %sink8 = alloca i8, align 1
  %sink64 = alloca i64, align 8

  ; Force lowering of both fields from the by-value struct argument.
  ; i64 field alignment requires 8-byte placement despite leading i8.
  %f0 = extractvalue %S %s, 0
  %f1 = extractvalue %S %s, 1
  store volatile i8 %f0, ptr %sink8, align 1
  store volatile i64 %f1, ptr %sink64, align 8
  ; Keep non-leaf behavior to preserve explicit prologue/epilogue.
  call void @callee()
  ret void
}

; CHECK-LABEL: test_struct:
; CHECK: mflr 12
; CHECK-NEXT: stwu 1, -112(1)
; CHECK-NEXT: std 12, 104(1)
; CHECK-NEXT: stb 3, 111(1)
; CHECK-NEXT: std 4, 96(1)
; CHECK-NEXT: bl callee
; CHECK-NEXT: addi 1, 1, 112
; CHECK-NEXT: ld 12, -8(1)
; CHECK-NEXT: mtlr 12
; CHECK-NEXT: blr

define void @test_dyn_alloca_call(i64 %n) {
entry:
  %p = alloca i8, i64 %n, align 16
  store volatile i8 7, ptr %p, align 1
  call void @callee()
  ret void
}

; CHECK-LABEL: test_dyn_alloca_call:
; CHECK: stwux
; CHECK-NOT: stdux
; CHECK: bl callee

define void @test_probed_dyn_alloca_call(i64 %n) #0 {
entry:
  %p = alloca i8, i64 %n, align 16
  store volatile i8 9, ptr %p, align 1
  call void @callee()
  ret void
}

; CHECK-LABEL: test_probed_dyn_alloca_call:
; CHECK: stwux
; CHECK: stwu
; CHECK-NOT: stdux
; CHECK-NOT: stdu
; CHECK: bl callee

define void @test_nonleaf_min_frame() {
entry:
  call void @callee()
  ret void
}

; CHECK-LABEL: test_nonleaf_min_frame:
; CHECK: stwu 1, -96(1)
; CHECK: std 12, 88(1)
; CHECK: bl callee

define void @test_leaf_redzone_limit() {
entry:
  %space = alloca [1344 x i8], align 16
  %last = getelementptr inbounds [1344 x i8], ptr %space, i64 0, i64 1343
  store volatile i8 1, ptr %last, align 1
  ret void
}

; CHECK-LABEL: test_leaf_redzone_limit:
; CHECK-NOT: stwu 1,
; CHECK-NOT: stwux
; CHECK: stb 3, -1(1)
; CHECK: blr

define void @test_leaf_redzone_overflow() {
entry:
  %space = alloca [1345 x i8], align 16
  %last = getelementptr inbounds [1345 x i8], ptr %space, i64 0, i64 1344
  store volatile i8 2, ptr %last, align 1
  ret void
}

; CHECK-LABEL: test_leaf_redzone_overflow:
; CHECK: stwu 1, -1376(1)
; CHECK: blr

define void @test_large_fixed_frame() {
entry:
  %space = alloca [65536 x i8], align 16
  %last = getelementptr inbounds [65536 x i8], ptr %space, i64 0, i64 65535
  store volatile i8 3, ptr %last, align 1
  ret void
}

; CHECK-LABEL: test_large_fixed_frame:
; CHECK: stwux 1,
; CHECK-NOT: stdux
; CHECK-NOT: stdu
; CHECK: blr

define void @test_probed_large_fixed_frame() #0 {
entry:
  %space = alloca [65536 x i8], align 16
  %last = getelementptr inbounds [65536 x i8], ptr %space, i64 0, i64 65535
  store volatile i8 4, ptr %last, align 1
  ret void
}

; CHECK-LABEL: test_probed_large_fixed_frame:
; CHECK: stwu
; CHECK-NOT: stdu
; CHECK: blr

define void @test_dyn_alloca_stack_args(i64 %n) {
entry:
  %space = alloca i8, i64 %n, align 16
  store volatile i8 3, ptr %space, align 1
  call void @callee_ten_args(i64 1, i64 2, i64 3, i64 4, i64 5,
                             i64 6, i64 7, i64 8, i64 9, i64 10)
  ret void
}

; CHECK-LABEL: test_dyn_alloca_stack_args:
; The dynamic allocation stores the old SP as the new backchain and returns
; newSP + 16 bytes of linkage + 96 bytes reserved for this call's parameters.
; CHECK: addi 4, 31, 128
; CHECK: stwux 4, 1, 5
; CHECK: addi 3, 1, 112
; CHECK-NOT: stdux
; CHECK-DAG: std {{[0-9]+}}, 80(1)
; CHECK-DAG: std {{[0-9]+}}, 88(1)
; CHECK: bl callee_ten_args

define void @test_nonvolatile_clobbers() {
entry:
  call void asm sideeffect "", "~{r14},~{f14},~{v20},~{v31}"()
  ret void
}

; CHECK-LABEL: test_nonvolatile_clobbers:
; CHECK-DAG: std 14, {{-?[0-9]+}}(1)
; CHECK-DAG: stfd 14, {{-?[0-9]+}}(1)
; CHECK-DAG: ld 14, {{-?[0-9]+}}(1)
; CHECK-DAG: lfd 14, {{-?[0-9]+}}(1)
; CHECK-DAG: li 3, -480
; CHECK-DAG: stvx 20, 1, 3
; CHECK-DAG: lvx 20, 1, 3
; CHECK-DAG: li 3, -304
; CHECK-DAG: stvx 31, 1, 3
; CHECK-DAG: lvx 31, 1, 3
; CHECK: blr

define void @test_nonvolatile_cr_clobbers() {
entry:
  call void asm sideeffect "", "~{cr2},~{cr3},~{cr4},~{cr5}"()
  ret void
}

; CHECK-LABEL: test_nonvolatile_cr_clobbers:
; CHECK: mfcr [[CRSCRATCH:[0-9]+]]
; CHECK-NEXT: stw [[CRSCRATCH]], 8(1)
; CHECK: #APP
; CHECK: #NO_APP
; CHECK: lwz [[CRSCRATCH]], 8(1)
; CHECK-DAG: mtocrf 32, [[CRSCRATCH]]
; CHECK-DAG: mtocrf 16, [[CRSCRATCH]]
; CHECK-DAG: mtocrf 8, [[CRSCRATCH]]
; CHECK-DAG: mtocrf 4, [[CRSCRATCH]]
; CHECK: blr

declare void @callee()
declare void @callee_ten_args(i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)

attributes #0 = { "probe-stack"="inline-asm" }

; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -tailcallopt -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -filetype=obj -verify-machineinstrs < %s -o %t.obj

declare void @target(ptr sret([16 x i8]), i32, i32, i32, i32, i32, i32, i32, i32, i32, <4 x float>, <4 x float>)

; Swapping stack arguments requires all incoming loads to precede stores.
define void @stack_sret(ptr sret([16 x i8]) %result,
                       i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f,
                       i32 %g, i32 %h, i32 %i, <4 x float> %v, <4 x float> %w) {
  musttail call void @target(ptr sret([16 x i8]) %result,
                            i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f,
                            i32 %i, i32 %g, i32 %h, <4 x float> %w, <4 x float> %v)
  ret void
}
; CHECK-LABEL: stack_sret:
; CHECK-DAG: lwz {{[0-9]+}}, 84(1)
; CHECK-DAG: lwz {{[0-9]+}}, 92(1)
; CHECK-DAG: std {{[0-9]+}}, 80(1)
; CHECK-DAG: std {{[0-9]+}}, 88(1)
; CHECK-NOT: bl target
; CHECK: b target

define i64 @indirect(ptr %target, i64 %a, i64 %b, i64 %c, i64 %d,
                     i64 %e, i64 %f, i64 %g, i64 %h, i64 %i) {
  %buffer = alloca [2048 x i8], align 32
  %byte = getelementptr [2048 x i8], ptr %buffer, i32 0, i32 1023
  store volatile i8 7, ptr %byte
  %sum = add i64 %h, %i
  %r = musttail call i64 %target(ptr %target, i64 %a, i64 %b, i64 %c,
                                i64 %d, i64 %e, i64 %f, i64 %g,
                                i64 %i, i64 %sum)
  ret i64 %r
}
; CHECK-LABEL: indirect:
; CHECK-NOT: bctrl
; CHECK: mtctr
; CHECK-NOT: bctrl
; CHECK: bctr

declare void @escape(ptr)
declare i64 @dynamic_target(i32, i64, i64, i64, i64, i64, i64, i64, i64, i64)
define i64 @dynamic_stack(i32 %size, i64 %a, i64 %b, i64 %c, i64 %d,
                          i64 %e, i64 %f, i64 %g, i64 %h, i64 %i) {
  %buffer = alloca i8, i32 %size, align 32
  call void @escape(ptr %buffer)
  %sum = add i64 %h, %i
  %r = musttail call i64 @dynamic_target(i32 %size, i64 %a, i64 %b, i64 %c,
                                        i64 %d, i64 %e, i64 %f, i64 %g,
                                        i64 %i, i64 %sum)
  ret i64 %r
}
; CHECK-LABEL: dynamic_stack:
; CHECK: bl escape
; CHECK: std
; CHECK-NOT: bl dynamic_target
; CHECK: b dynamic_target

; The fourteenth single-precision argument occupies the low word of its slot.
declare float @float_target(float, float, float, float, float, float, float,
                            float, float, float, float, float, float, float)
define float @float_stack(float %a, float %b, float %c, float %d,
                          float %e, float %f, float %g, float %h,
                          float %i, float %j, float %k, float %l,
                          float %m, float %n) {
  %sum = fadd float %n, %a
  %r = musttail call float @float_target(
      float %a, float %b, float %c, float %d, float %e, float %f, float %g,
      float %h, float %i, float %j, float %k, float %l, float %m, float %sum)
  ret float %r
}
; CHECK-LABEL: float_stack:
; CHECK: lfs {{[0-9]+}}, 124(1)
; CHECK: stfs {{[0-9]+}}, 124(1)
; CHECK-NOT: bl float_target
; CHECK: b float_target

declare <4 x i32> @vector_target(<4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
    <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
    <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
define <4 x i32> @vector_stack(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c,
    <4 x i32> %d, <4 x i32> %e, <4 x i32> %f, <4 x i32> %g, <4 x i32> %h,
    <4 x i32> %i, <4 x i32> %j, <4 x i32> %k, <4 x i32> %l,
    <4 x i32> %m, <4 x i32> %n) {
  %sum = add <4 x i32> %a, %n
  %r = musttail call <4 x i32> @vector_target(
      <4 x i32> %a, <4 x i32> %b, <4 x i32> %c, <4 x i32> %d,
      <4 x i32> %e, <4 x i32> %f, <4 x i32> %g, <4 x i32> %h,
      <4 x i32> %i, <4 x i32> %j, <4 x i32> %k, <4 x i32> %l,
      <4 x i32> %m, <4 x i32> %sum)
  ret <4 x i32> %r
}
; CHECK-LABEL: vector_stack:
; CHECK: lvx
; CHECK: stvx
; CHECK-NOT: bl vector_target
; CHECK: b vector_target

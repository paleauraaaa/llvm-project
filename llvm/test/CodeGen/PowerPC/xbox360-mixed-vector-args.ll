; RUN: llc -mtriple=ppc64-ibm-xbox360 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=ppc64-ibm-xbox360 -verify-machineinstrs -filetype=obj %s -o %t.obj

; MSVC's prototyped vectors occupy aligned parameter-image slots without
; consuming GPR slots, including the padding before a vector. Floating-point
; scalars still skip one GPR slot. Variadic vectors retain their GPR image.

declare void @mixed(i32, float, i64, <4 x float>, i32, double, ptr, i64, <4 x float>, i64)

; CHECK-LABEL: name: call_mixed
; CHECK: BL @mixed{{.*}}implicit $x3, implicit $f1, implicit $x5, implicit $v1, implicit $x6, implicit $f2, implicit $x8, implicit $x9, implicit $v2, implicit $x10
define void @call_mixed(ptr %p, <4 x float> %v) {
  call void @mixed(i32 1, float 2.0, i64 3, <4 x float> %v,
                  i32 5, double 6.0, ptr %p, i64 8, <4 x float> %v, i64 10)
  ret void
}

; CHECK-LABEL: name: receive_mixed
; CHECK: liveins: $x3, $f1, $x5, $v1, $x6, $f2, $x8, $x9, $v2, $x10
define void @receive_mixed(i32 %a, float %b, i64 %c, <4 x float> %d,
                          i32 %e, double %f, ptr %g, i64 %h,
                          <4 x float> %i, i64 %j) {
  call void @mixed(i32 %a, float %b, i64 %c, <4 x float> %d,
                  i32 %e, double %f, ptr %g, i64 %h, <4 x float> %i, i64 %j)
  ret void
}

declare void @overflow(i64, <4 x i32>, i64, i64, i64, i64, i64, i64, i64, i64)

; The ninth integer remains on the stack at its full parameter-image offset.
; CHECK-LABEL: name: call_overflow
; CHECK: STD {{.*}}, 104, {{.*}}
; CHECK: BL @overflow{{.*}}implicit $x3, implicit $v1, implicit $x4, implicit $x5, implicit $x6, implicit $x7, implicit $x8, implicit $x9, implicit $x10
define void @call_overflow(<4 x i32> %v) {
  call void @overflow(i64 1, <4 x i32> %v, i64 2, i64 3, i64 4,
                     i64 5, i64 6, i64 7, i64 8, i64 9)
  ret void
}

; CHECK-LABEL: name: receive_overflow
; CHECK: offset: 104, size: 8
define i64 @receive_overflow(i64 %a, <4 x i32> %v, i64 %b, i64 %c,
                            i64 %d, i64 %e, i64 %f, i64 %g, i64 %h, i64 %i) {
  ret i64 %i
}

%Pair = type { i64, i64 }
declare void @vector_byval(<4 x i32>, ptr byval(%Pair) align 8, i64)

; By-value aggregates following a vector still consume their GPR chunks.
; CHECK-LABEL: name: receive_byval
; CHECK: liveins: $v1, $x3, $x4, $x5
define void @receive_byval(<4 x i32> %v, ptr byval(%Pair) align 8 %p, i64 %n) {
  call void @vector_byval(<4 x i32> %v, ptr byval(%Pair) align 8 %p, i64 %n)
  ret void
}

declare void @vararg(i64, ...)

; CHECK-LABEL: name: call_vararg
; CHECK: BL @vararg{{.*}}implicit $x3, implicit $v1, implicit $x5, implicit $x6, implicit $x7
define void @call_vararg(<4 x i32> %v) {
  call void (i64, ...) @vararg(i64 1, <4 x i32> %v, i64 2)
  ret void
}

; CHECK-LABEL: name: receive_vararg
; CHECK: liveins: $x7
define i64 @receive_vararg(i64 %a, <4 x i32> %v, i64 %b, ...) {
  ret i64 %b
}

declare void @vectors_then_integer(
    <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
    <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
    <4 x i32>, <4 x i32>, i64)

; Even a vector overflowing the VR bank must not consume a GPR slot.
; CHECK-LABEL: name: call_vectors_then_integer
; CHECK: BL @vectors_then_integer{{.*}}implicit $v13, implicit $x3
define void @call_vectors_then_integer(<4 x i32> %v) {
  call void @vectors_then_integer(
      <4 x i32> %v, <4 x i32> %v, <4 x i32> %v, <4 x i32> %v,
      <4 x i32> %v, <4 x i32> %v, <4 x i32> %v, <4 x i32> %v,
      <4 x i32> %v, <4 x i32> %v, <4 x i32> %v, <4 x i32> %v,
      <4 x i32> %v, <4 x i32> %v, i64 42)
  ret void
}

; CHECK-LABEL: name: receive_vectors_then_integer
; CHECK: liveins: $x3
define i64 @receive_vectors_then_integer(
    <4 x i32> %a, <4 x i32> %b, <4 x i32> %c, <4 x i32> %d,
    <4 x i32> %e, <4 x i32> %f, <4 x i32> %g, <4 x i32> %h,
    <4 x i32> %i, <4 x i32> %j, <4 x i32> %k, <4 x i32> %l,
    <4 x i32> %m, <4 x i32> %n, i64 %value) {
  ret i64 %value
}

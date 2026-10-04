; RUN: llc -verify-machineinstrs -mtriple=ppc64-ibm-xbox360 -stop-after=machine-cp %s -o - | FileCheck %s
;
; Comprehensive Xbox 360 call ABI coverage:
; - Integer, floating-point, pointer, struct(byval), vector, and varargs calls.
; - Register usage for argument registers.
; - Stack overflow behavior once register banks are exhausted.
;
; CHECK-LABEL: name: call_gap_i64_f64_i64
; CHECK-NOT: BL {{.*}}mix_gap{{.*}}implicit $x4
; CHECK: BL {{.*}}mix_gap{{.*}}implicit $x3, implicit $f1, implicit killed $x5

target triple = "ppc64-ibm-xbox360"

declare void @mix_gap(i64, double, i64)

define void @call_gap_i64_f64_i64() {
entry:
  call void @mix_gap(i64 1, double 2.000000e+00, i64 3)
  ret void
}

declare void @takes_float(float)

; A prototyped float uses its FPR, without consuming the parallel GPR.
; CHECK-LABEL: name: call_one_float
; CHECK: BL {{.*}}takes_float{{.*}}implicit $f1
; CHECK-NOT: BL {{.*}}takes_float{{.*}}implicit $x3
define void @call_one_float() {
entry:
  call void @takes_float(float 1.000000e+00)
  ret void
}

declare void @takes_14_floats(float, float, float, float, float, float, float,
                              float, float, float, float, float, float, float)

; The fourteenth prototyped float overflows the FPR bank into the first stack
; argument slot, with the single-precision value right-justified in its word.
; CHECK-LABEL: name: call_14_floats
; CHECK: STW8 {{.*}}, 124, $x1
; CHECK: BL {{.*}}takes_14_floats{{.*}}implicit $f1, implicit $f2, implicit $f3, implicit $f4, implicit $f5, implicit $f6, implicit $f7, implicit $f8, implicit $f9, implicit $f10, implicit $f11, implicit $f12, implicit $f13
define void @call_14_floats() {
entry:
  call void @takes_14_floats(
      float 1.000000e+00, float 2.000000e+00, float 3.000000e+00,
      float 4.000000e+00, float 5.000000e+00, float 6.000000e+00,
      float 7.000000e+00, float 8.000000e+00, float 9.000000e+00,
      float 1.000000e+01, float 1.100000e+01, float 1.200000e+01,
      float 1.300000e+01, float 1.400000e+01)
  ret void
}

; CHECK-LABEL: name: sum_ints10
; CHECK: liveins: $x3, $x4, $x5, $x6, $x7, $x8, $x9, $x10
; CHECK: %fixed-stack.
; CHECK: %fixed-stack.
; CHECK: BLR8 implicit $lr8, implicit $rm, implicit $x3
define i64 @sum_ints10(
    i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4,
    i64 %a5, i64 %a6, i64 %a7, i64 %a8, i64 %a9) {
entry:
  %s01 = add i64 %a0, %a1
  %s23 = add i64 %a2, %a3
  %s45 = add i64 %a4, %a5
  %s67 = add i64 %a6, %a7
  %s89 = add i64 %a8, %a9
  %s0123 = add i64 %s01, %s23
  %s4567 = add i64 %s45, %s67
  %s0to7 = add i64 %s0123, %s4567
  %res = add i64 %s0to7, %s89
  ret i64 %res
}

; CHECK-LABEL: name: call_ints10
; CHECK: BL {{.*}}sum_ints10{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit $x7, implicit $x8, implicit $x9, implicit $x10, {{.*}}implicit-def $x3
define i64 @call_ints10() {
entry:
  %r = call i64 @sum_ints10(
      i64 1, i64 2, i64 3, i64 4, i64 5,
      i64 6, i64 7, i64 8, i64 9, i64 10)
  ret i64 %r
}

; CHECK-LABEL: name: sum_doubles15
; CHECK: liveins: $f1, $f2, $f3, $f4, $f5, $f6, $f7, $f8, $f9, $f10, $f11, $f12, $f13
; CHECK: %fixed-stack.
; CHECK: %fixed-stack.
; CHECK: BLR8 implicit $lr8, implicit $rm, implicit $f1
define double @sum_doubles15(
    double %a0, double %a1, double %a2, double %a3, double %a4,
    double %a5, double %a6, double %a7, double %a8, double %a9,
    double %a10, double %a11, double %a12, double %a13, double %a14) {
entry:
  %s01 = fadd double %a0, %a1
  %s23 = fadd double %a2, %a3
  %s45 = fadd double %a4, %a5
  %s67 = fadd double %a6, %a7
  %s89 = fadd double %a8, %a9
  %s1011 = fadd double %a10, %a11
  %s1213 = fadd double %a12, %a13
  %t0 = fadd double %s01, %s23
  %t1 = fadd double %s45, %s67
  %t2 = fadd double %s89, %s1011
  %t3 = fadd double %s1213, %a14
  %u0 = fadd double %t0, %t1
  %u1 = fadd double %t2, %t3
  %res = fadd double %u0, %u1
  ret double %res
}

; CHECK-LABEL: name: call_doubles15
; CHECK: BL {{.*}}sum_doubles15{{.*}}implicit $f1, implicit $f2, implicit $f3, implicit $f4, implicit $f5, implicit $f6, implicit $f7, implicit $f8, implicit $f9, implicit $f10, implicit $f11, implicit $f12, implicit $f13, {{.*}}implicit-def $f1
define double @call_doubles15() {
entry:
  %r = call double @sum_doubles15(
      double 1.000000e+00, double 2.000000e+00, double 3.000000e+00,
      double 4.000000e+00, double 5.000000e+00, double 6.000000e+00,
      double 7.000000e+00, double 8.000000e+00, double 9.000000e+00,
      double 1.000000e+01, double 1.100000e+01, double 1.200000e+01,
      double 1.300000e+01, double 1.400000e+01, double 1.500000e+01)
  ret double %r
}

; CHECK-LABEL: name: sum_vec15
; CHECK: liveins: $v1, $v2, $v3, $v4, $v5, $v6, $v7, $v8, $v9, $v10, $v11, $v12, $v13
; CHECK: %fixed-stack.
; CHECK: %fixed-stack.
; CHECK: BLR8 implicit $lr8, implicit $rm, implicit $v1
define <4 x i32> @sum_vec15(
    <4 x i32> %v0, <4 x i32> %v1, <4 x i32> %v2, <4 x i32> %v3, <4 x i32> %v4,
    <4 x i32> %v5, <4 x i32> %v6, <4 x i32> %v7, <4 x i32> %v8, <4 x i32> %v9,
    <4 x i32> %v10, <4 x i32> %v11, <4 x i32> %v12, <4 x i32> %v13, <4 x i32> %v14) {
entry:
  %s01 = add <4 x i32> %v0, %v1
  %s23 = add <4 x i32> %v2, %v3
  %s45 = add <4 x i32> %v4, %v5
  %s67 = add <4 x i32> %v6, %v7
  %s89 = add <4 x i32> %v8, %v9
  %s1011 = add <4 x i32> %v10, %v11
  %s1213 = add <4 x i32> %v12, %v13
  %t0 = add <4 x i32> %s01, %s23
  %t1 = add <4 x i32> %s45, %s67
  %t2 = add <4 x i32> %s89, %s1011
  %t3 = add <4 x i32> %s1213, %v14
  %u0 = add <4 x i32> %t0, %t1
  %u1 = add <4 x i32> %t2, %t3
  %res = add <4 x i32> %u0, %u1
  ret <4 x i32> %res
}

; CHECK-LABEL: name: call_vec15
; CHECK: STVX {{.*}}, $zero8, {{(killed )?}}renamable $x{{[0-9]+}}
; CHECK: STVX {{.*}}, $zero8, {{(killed )?}}renamable $x{{[0-9]+}}
; CHECK: BL {{.*}}sum_vec15{{.*}}implicit $v1, implicit $v2, implicit $v3, implicit $v4, implicit $v5, implicit $v6, implicit $v7, implicit $v8, implicit $v9, implicit $v10, implicit $v11, implicit $v12, implicit $v13, {{.*}}implicit-def $v1
define <4 x i32> @call_vec15() {
entry:
  %r = call <4 x i32> @sum_vec15(
      <4 x i32> <i32 1, i32 2, i32 3, i32 4>,
      <4 x i32> <i32 5, i32 6, i32 7, i32 8>,
      <4 x i32> <i32 9, i32 10, i32 11, i32 12>,
      <4 x i32> <i32 13, i32 14, i32 15, i32 16>,
      <4 x i32> <i32 17, i32 18, i32 19, i32 20>,
      <4 x i32> <i32 21, i32 22, i32 23, i32 24>,
      <4 x i32> <i32 25, i32 26, i32 27, i32 28>,
      <4 x i32> <i32 29, i32 30, i32 31, i32 32>,
      <4 x i32> <i32 33, i32 34, i32 35, i32 36>,
      <4 x i32> <i32 37, i32 38, i32 39, i32 40>,
      <4 x i32> <i32 41, i32 42, i32 43, i32 44>,
      <4 x i32> <i32 45, i32 46, i32 47, i32 48>,
      <4 x i32> <i32 49, i32 50, i32 51, i32 52>,
      <4 x i32> <i32 53, i32 54, i32 55, i32 56>,
      <4 x i32> <i32 57, i32 58, i32 59, i32 60>)
  ret <4 x i32> %r
}

%S24 = type { i64, i64, i64 }
@G_S24 = global %S24 { i64 11, i64 22, i64 33 }, align 8
%S3 = type <{ i8, i8, i8 }>
%S4 = type <{ i8, i8, i8, i8 }>
%S7 = type <{ i8, i8, i8, i8, i8, i8, i8 }>
%S8 = type { i64 }
%S16 = type { i64, i64 }
@G_S3 = global %S3 <{ i8 1, i8 2, i8 3 }>, align 1
@G_S4 = global %S4 <{ i8 4, i8 5, i8 6, i8 7 }>, align 1
@G_S7 = global %S7 <{ i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7 }>, align 1
@G_S8 = global %S8 { i64 8 }, align 8
@G_S16 = global %S16 { i64 16, i64 17 }, align 8

declare void @take_s3(ptr byval(%S3) align 1)
declare void @take_s4(ptr byval(%S4) align 1)
declare void @take_s7(ptr byval(%S7) align 1)
declare void @take_s8(ptr byval(%S8) align 8)
declare void @take_split_s16(i64, i64, i64, i64, i64, i64, i64,
                             ptr byval(%S16) align 8, i64)

; CHECK-LABEL: name: call_packed_s3
; CHECK: STB {{.*}}, 18, $x1
; CHECK: STH {{.*}}, 16, $x1
; CHECK: BL @take_s3{{.*}}implicit $x3
define void @call_packed_s3() {
entry:
  call void @take_s3(ptr byval(%S3) align 1 @G_S3)
  ret void
}

; CHECK-LABEL: name: call_packed_s4
; CHECK: BL @take_s4{{.*}}implicit $x3
define void @call_packed_s4() {
entry:
  call void @take_s4(ptr byval(%S4) align 1 @G_S4)
  ret void
}

; CHECK-LABEL: name: call_packed_s7
; CHECK-DAG: STW {{.*}}, 16, $x1
; CHECK-DAG: STW {{.*}}, 19, $x1
; CHECK: BL @take_s7{{.*}}implicit $x3
define void @call_packed_s7() {
entry:
  call void @take_s7(ptr byval(%S7) align 1 @G_S7)
  ret void
}

; CHECK-LABEL: name: call_s8
; CHECK: BL @take_s8{{.*}}implicit $x3
define void @call_s8() {
entry:
  call void @take_s8(ptr byval(%S8) align 8 @G_S8)
  ret void
}

; CHECK-LABEL: name: call_split_s16
; CHECK: STD {{.*}}, 80, $x1
; CHECK: $x10 = LD 0,
; CHECK: BL @take_split_s16{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit {{(killed )?}}$x7, implicit {{(killed )?}}$x8, implicit {{(killed )?}}$x9, implicit $x10
define void @call_split_s16() {
entry:
  call void @take_split_s16(
      i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7,
      ptr byval(%S16) align 8 @G_S16, i64 18)
  ret void
}

; CHECK-LABEL: name: make_sret
; CHECK: body:
; CHECK: liveins: $x3, $x4
; CHECK: BLR8 implicit $lr8, implicit $rm
define void @make_sret(ptr sret(%S24) align 8 %result, i64 %value) {
entry:
  %first = getelementptr inbounds %S24, ptr %result, i32 0, i32 0
  %second = getelementptr inbounds %S24, ptr %result, i32 0, i32 1
  %third = getelementptr inbounds %S24, ptr %result, i32 0, i32 2
  store i64 %value, ptr %first, align 8
  store i64 %value, ptr %second, align 8
  store i64 %value, ptr %third, align 8
  ret void
}

; CHECK-LABEL: name: call_sret
; CHECK: BL {{.*}}make_sret{{.*}}implicit $x3, implicit killed $x4
define i64 @call_sret() {
entry:
  %result = alloca %S24, align 8
  call void @make_sret(ptr sret(%S24) align 8 %result, i64 19)
  %value = load i64, ptr %result, align 8
  ret i64 %value
}

declare void @take_byval(ptr byval(%S24) align 8, i64, i64, i64, i64, i64, i64, i64, i64)

; CHECK-LABEL: name: call_byval_mix
; CHECK: BL {{.*}}take_byval{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit killed $x6, implicit killed $x7, implicit killed $x8, implicit killed $x9, implicit killed $x10
define void @call_byval_mix() {
entry:
  call void @take_byval(
      ptr byval(%S24) align 8 @G_S24,
      i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8)
  ret void
}

@G_PTR = global i64 0, align 8

declare void @var_mix(i64, ...)

; CHECK-LABEL: name: call_var_mix
; CHECK: BL {{.*}}var_mix{{.*}}implicit $x3, implicit $f1, implicit $x4, implicit $x5, implicit $v1, implicit $x7, implicit $x8, implicit $x9, implicit $x10, implicit $f2
define void @call_var_mix() {
entry:
  call void (i64, ...) @var_mix(
      i64 1,
      double 2.000000e+00,
      ptr @G_PTR,
      <4 x i32> <i32 10, i32 20, i32 30, i32 40>,
      i64 5, i64 6, i64 7, i64 8, i64 9,
      double 1.000000e+01)
  ret void
}

declare void @old_style(...)

; Calls through an old-style C declaration use default argument promotions
; and the variadic ABI's FPR/GPR duplication.
; CHECK-LABEL: name: call_old_style
; CHECK: BL {{.*}}old_style{{.*}}implicit $f1, implicit $x3, implicit $f2, implicit $x4
define void @call_old_style() {
entry:
  call void (...) @old_style(double 2.000000e+00, double 3.000000e+00)
  ret void
}

declare void @old_style_many(...)

; After GPR3-GPR10 are filled, old-style doubles remain in FPRs and get
; backing copies in the argument area.
; CHECK-LABEL: name: call_old_style_many
; CHECK-DAG: STD {{.*}}, 80, $x1
; CHECK-DAG: STD {{.*}}, 88, $x1
; CHECK: BL {{.*}}old_style_many{{.*}}implicit $f1, implicit $x3, implicit $f2, implicit $x4, implicit $f3, implicit $x5, implicit $f4, implicit $x6, implicit $f5, implicit $x7, implicit $f6, implicit $x8, implicit $f7, implicit $x9, implicit $f8, implicit $x10, implicit $f9, implicit $f10
define void @call_old_style_many() {
entry:
  call void (...) @old_style_many(
      double 1.000000e+00, double 2.000000e+00, double 3.000000e+00,
      double 4.000000e+00, double 5.000000e+00, double 6.000000e+00,
      double 7.000000e+00, double 8.000000e+00, double 9.000000e+00,
      double 1.000000e+01)
  ret void
}

declare void @take_14_vectors(<4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
                              <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
                              <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>,
                              <4 x i32>, <4 x i32>)

; The 14th vector goes to an aligned stack slot after 13 vector registers.
; CHECK-LABEL: name: call_14_vectors
; CHECK: renamable $r5 = ADDI $r1, 224,
; CHECK: ADJCALLSTACKDOWN 240
; CHECK: STVX {{.*}}(store (s128))
; CHECK: BL {{.*}}take_14_vectors{{.*}}implicit killed $v1, implicit $v2, implicit $v3, implicit $v4, implicit $v5, implicit killed $v6, implicit killed $v7, implicit killed $v8, implicit killed $v9, implicit killed $v10, implicit killed $v11, implicit killed $v12, implicit killed $v13
define void @call_14_vectors() {
entry:
  call void @take_14_vectors(
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer,
      <4 x i32> <i32 1, i32 2, i32 3, i32 4>)
  ret void
}

declare void @take_scalar_and_14_vectors(i64, <4 x i32>, <4 x i32>,
                                         <4 x i32>, <4 x i32>, <4 x i32>,
                                         <4 x i32>, <4 x i32>, <4 x i32>,
                                         <4 x i32>, <4 x i32>, <4 x i32>,
                                         <4 x i32>, <4 x i32>, <4 x i32>)

; The scalar advances the cursor from 16 to 24; the first vector is aligned
; up to 32, leaving the 14th vector at the aligned offset 240.
; CHECK-LABEL: name: call_mixed_14_vectors
; CHECK: renamable $r6 = ADDI $r1, 240,
; CHECK: ADJCALLSTACKDOWN 256
; CHECK: STVX {{.*}}(store (s128))
; CHECK: BL {{.*}}take_scalar_and_14_vectors
define void @call_mixed_14_vectors(i64 %scalar) {
entry:
  call void @take_scalar_and_14_vectors(i64 %scalar,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer, <4 x i32> zeroinitializer,
      <4 x i32> zeroinitializer,
      <4 x i32> <i32 1, i32 2, i32 3, i32 4>)
  ret void
}

; Force register pressure while keeping R2 and R13 unavailable to allocation.
; CHECK-LABEL: name: reserved_gprs
; CHECK-NOT: {{$x2([^0-9]|$)}}
; CHECK-NOT: {{$x13([^0-9]|$)}}
; CHECK: INLINEASM
; CHECK-NOT: {{$x2([^0-9]|$)}}
; CHECK-NOT: {{$x13([^0-9]|$)}}
; CHECK: BLR8
define void @reserved_gprs(i64 %a0, i64 %a1, i64 %a2, i64 %a3,
                           i64 %a4, i64 %a5, i64 %a6, i64 %a7,
                           i64 %a8, i64 %a9, i64 %a10, i64 %a11) {
entry:
  call void asm sideeffect "", "r,r,r,r,r,r,r,r,r,r,r,r"(
      i64 %a0, i64 %a1, i64 %a2, i64 %a3, i64 %a4, i64 %a5,
      i64 %a6, i64 %a7, i64 %a8, i64 %a9, i64 %a10, i64 %a11)
  ret void
}

declare void @llvm.va_start(ptr)

; The variadic callee reads arguments from the contiguous parameter image.
; CHECK-LABEL: name: vararg_callee
; CHECK: liveins: $x3, $x4, $x5, $x6, $x7, $x8, $x9, $x10
; CHECK-DAG: STD {{.*}}$x4, 0, %fixed-stack.0
; CHECK-DAG: STD {{.*}}$x5, 8, %fixed-stack.0
; CHECK-DAG: STD {{.*}}$x6, 16, %fixed-stack.0
; CHECK-DAG: STD {{.*}}$x7, 24, %fixed-stack.0
; CHECK-DAG: STD {{.*}}$x8, 32, %fixed-stack.0
; CHECK-DAG: STD {{.*}}$x9, 40, %fixed-stack.0
; CHECK-DAG: STD {{.*}}$x10, 48, %fixed-stack.0
; CHECK: LVX
; CHECK: STVX
; CHECK: LWZ8
define i64 @vararg_callee(i64 %fixed, ...) {
entry:
  %ap = alloca ptr, align 8
  call void @llvm.va_start(ptr %ap)
  %integer = va_arg ptr %ap, i64
  %floating = va_arg ptr %ap, double
  %pointer = va_arg ptr %ap, ptr
  %vector = va_arg ptr %ap, <4 x i32>
  %pointer.int = ptrtoint ptr %pointer to i64
  %floating.bits = bitcast double %floating to i64
  %vector.first = extractelement <4 x i32> %vector, i32 0
  %vector.wide = zext i32 %vector.first to i64
  %sum0 = add i64 %fixed, %integer
  %sum1 = add i64 %sum0, %pointer.int
  %sum2 = add i64 %sum1, %floating.bits
  %sum = add i64 %sum2, %vector.wide
  ret i64 %sum
}

; CHECK-LABEL: name: call_vararg_callee
; CHECK: BL {{.*}}vararg_callee
define i64 @call_vararg_callee() {
entry:
  %result = call i64 (i64, ...) @vararg_callee(
      i64 0, i64 1, double 2.000000e+00, ptr @G_PTR,
      <4 x i32> <i32 3, i32 4, i32 5, i32 6>,
      i64 7, i64 8, i64 9, i64 10, i64 11)
  ret i64 %result
}

; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=true -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=FAST
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=false -verify-machineinstrs %s -filetype=obj -o %t.dag.obj
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs %s -filetype=obj -o %t.opt.obj
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=false -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=DAG

target datalayout = "E-m:w-S128-p:32:32-Fi32-i64:64-i128:128-n32:64-v128:128"

@global_data = global [4 x float] zeroinitializer, align 16

declare void @escape(ptr)

; FAST-LABEL: name: stack_argument
; FAST: %[[STACK:[0-9]+]]:gprc{{[^ ]*}} = ADDI %stack.0
; FAST: {{%[0-9]+}}:g8rc{{[^ ]*}} = RLDICL_32_64 %[[STACK]], 0, 32
define void @stack_argument() {
  %p = alloca [32 x i8], align 16
  call void @escape(ptr %p)
  ret void
}

; FAST-LABEL: name: shared_stack_address
; FAST: %[[BASE:[0-9]+]]:gprc{{[^ ]*}} = ADDI %stack.0
; FAST: %[[LANE:[0-9]+]]:gprc{{[^ ]*}} = ADD4 %[[BASE]],
; FAST: RLDICL_32_64 %[[LANE]], 0, 32
; FAST: LFS
define i32 @shared_stack_address(<4 x float> %v, i32 %index, ptr %out) {
  %p = alloca <4 x float>, align 16
  store <4 x float> %v, ptr %p, align 16
  %lane = getelementptr float, ptr %p, i32 %index
  %value = load float, ptr %lane, align 4
  store float %value, ptr %out, align 4
  %address = ptrtoint ptr %lane to i32
  ret i32 %address
}

; FAST-LABEL: name: shared_global_address
; FAST: %[[HI:[0-9]+]]:gprc{{[^ ]*}} = ADDIS
; FAST: %[[GLOBAL:[0-9]+]]:gprc{{[^ ]*}} = ADDI killed %[[HI]],
; FAST: %[[GLOBAL_LANE:[0-9]+]]:gprc{{[^ ]*}} = ADD4 %[[GLOBAL]],
; FAST: RLDICL_32_64 %[[GLOBAL_LANE]], 0, 32
define i32 @shared_global_address(i32 %index, ptr %out) {
  %lane = getelementptr [4 x float], ptr @global_data, i32 0, i32 %index
  %value = load float, ptr %lane, align 4
  store float %value, ptr %out, align 4
  %address = ptrtoint ptr %lane to i32
  ret i32 %address
}

; FAST-LABEL: name: single_constant
; FAST: %[[BITS:[0-9]+]]:gprc = LIS 16256
; FAST: STW %[[BITS]],
; FAST: LFS
; FAST-NOT: FCFID
define float @single_constant(float %a) {
  %r = fadd float %a, 1.0
  ret float %r
}

; FAST-LABEL: name: double_constant
; FAST: STD %{{[0-9]+}},
; FAST: LFD
; FAST-NOT: FCFID
define double @double_constant(double %a) {
  %r = fadd double %a, 1.0
  ret double %r
}

define float @unsigned_conversion(i32 %a) {
  %r = uitofp i32 %a to float
  ret float %r
}

; FAST-LABEL: name: signed_conversion
; FAST: %[[SIGNED:[0-9]+]]:g8rc = EXTSW_32_64
; FAST: STD %[[SIGNED]],
; FAST: LFD
; FAST: FCFID
define double @signed_conversion(i32 %a) {
  %r = sitofp i32 %a to double
  ret double %r
}

define i64 @wide_load(ptr %p, i32 %offset) {
  %address = getelementptr i64, ptr %p, i32 %offset
  %value = load i64, ptr %address, align 8
  ret i64 %value
}

; FAST-LABEL: name: indexed_vector
; FAST: %[[INDEXED:[0-9]+]]:gprc{{[^ ]*}} = ADD4
; FAST: RLDICL_32_64 %[[INDEXED]], 0, 32
; FAST: LVX $zero8,
define <4 x i32> @indexed_vector(ptr %p, i32 %offset) {
  %address = getelementptr i8, ptr %p, i32 %offset
  %value = load <4 x i32>, ptr %address, align 16
  ret <4 x i32> %value
}

; FAST-LABEL: name: negative_offset
; FAST: %[[NEGHIGH:[0-9]+]]:gprc = LIS 65535
; FAST: %[[NEG:[0-9]+]]:gprc = ORI %[[NEGHIGH]], 65535
; FAST: %[[ADJUSTED:[0-9]+]]:gprc{{[^ ]*}} = ADD4 {{.*}}, %[[NEG]]
; FAST: RLDICL_32_64 %[[ADJUSTED]], 0, 32
; FAST: LBZ8 0,
; DAG-LABEL: name: negative_offset
; DAG: %[[ADJUSTED:[0-9]+]]:gprc{{[^ ]*}} = ADDI {{.*}}, -1
; DAG: RLDICL_32_64 killed %[[ADJUSTED]], 0, 32
; DAG: LBZ8 0,
define i8 @negative_offset(ptr %p) {
  %previous = getelementptr i8, ptr %p, i32 -1
  %byte = load i8, ptr %previous, align 1
  ret i8 %byte
}

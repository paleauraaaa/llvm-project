; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -filetype=obj -o %t.obj
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR

target datalayout = "E-m:w-S128-p:32:32-Fi32-i64:64-i128:128-n32:64-v128:128"

; Wide integer lanes must be scalarized, not reinterpreted as independent
; 32-bit additions: each lane must propagate carries across its low word.
; CHECK-LABEL: add_lanes:
; CHECK: add
; CHECK: add
; MIR-LABEL: name: add_lanes
; MIR-COUNT-2: :g8rc = ADD8
define void @add_lanes(ptr %out, ptr %a, ptr %b) {
  %va = load <2 x i64>, ptr %a, align 16
  %vb = load <2 x i64>, ptr %b, align 16
  %sum = add <2 x i64> %va, %vb
  store <2 x i64> %sum, ptr %out, align 16
  ret void
}

; CHECK-LABEL: sub_lanes:
; CHECK: sub
; CHECK: sub
; MIR-LABEL: name: sub_lanes
; MIR-COUNT-2: :g8rc = SUBF8
define void @sub_lanes(ptr %out, ptr %a, ptr %b) {
  %va = load <2 x i64>, ptr %a, align 16
  %vb = load <2 x i64>, ptr %b, align 16
  %diff = sub <2 x i64> %va, %vb
  store <2 x i64> %diff, ptr %out, align 16
  ret void
}

; CHECK-LABEL: add_quadword:
; CHECK: addc
; CHECK: adde
; MIR-LABEL: name: add_quadword
; MIR: ADDC8
; MIR: ADDE8
define void @add_quadword(ptr %out, ptr %a, ptr %b) {
  %va = load <1 x i128>, ptr %a, align 16
  %vb = load <1 x i128>, ptr %b, align 16
  %sum = add <1 x i128> %va, %vb
  store <1 x i128> %sum, ptr %out, align 16
  ret void
}

; CHECK-LABEL: sub_quadword:
; CHECK: subc
; CHECK: subfe
; MIR-LABEL: name: sub_quadword
; MIR: SUBFC8
; MIR: SUBFE8
define void @sub_quadword(ptr %out, ptr %a, ptr %b) {
  %va = load <1 x i128>, ptr %a, align 16
  %vb = load <1 x i128>, ptr %b, align 16
  %diff = sub <1 x i128> %va, %vb
  store <1 x i128> %diff, ptr %out, align 16
  ret void
}

; CHECK-LABEL: convert_reduce:
; CHECK: fctidz
; CHECK: add
; MIR-LABEL: name: convert_reduce
; MIR: ADD8
define i64 @convert_reduce(<4 x float> %a) {
  %wide = fptoui <4 x float> %a to <4 x i64>
  %sum = call i64 @llvm.vector.reduce.add.v4i64(<4 x i64> %wide)
  ret i64 %sum
}

declare i64 @llvm.vector.reduce.add.v4i64(<4 x i64>)

; CHECK-LABEL: wide_bitwise:
; CHECK: lvx
; CHECK: lvx
; CHECK: vxor
; CHECK: stvx
define void @wide_bitwise(ptr %out, ptr %a, ptr %b) {
  %va = load volatile <2 x i64>, ptr %a, align 16
  %vb = load volatile <2 x i64>, ptr %b, align 16
  %bits = xor <2 x i64> %va, %vb
  store volatile <2 x i64> %bits, ptr %out, align 16
  ret void
}

define void @wide_compare_select(ptr %out, ptr %a, ptr %b) {
  %va = load <2 x i64>, ptr %a, align 16
  %vb = load <2 x i64>, ptr %b, align 16
  %cmp = icmp sgt <2 x i64> %va, %vb
  %max = select <2 x i1> %cmp, <2 x i64> %va, <2 x i64> %vb
  store <2 x i64> %max, ptr %out, align 16
  ret void
}

define void @wide_shift(ptr %out, ptr %a, ptr %b) {
  %va = load <2 x i64>, ptr %a, align 16
  %vb = load <2 x i64>, ptr %b, align 16
  %shift = shl <2 x i64> %va, %vb
  store <2 x i64> %shift, ptr %out, align 16
  ret void
}

; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -verify-machineinstrs < %s | FileCheck %s

declare float @llvm.ppc.fsels(float, float, float)
declare float @operand(i32)

; CHECK-LABEL: select_float:
; CHECK: fsel 1, 1, 2, 3
; CHECK: blr
define float @select_float(float %comparand, float %ge, float %lt) {
  %result = call float @llvm.ppc.fsels(float %comparand, float %ge, float %lt)
  ret float %result
}

; CHECK-LABEL: select_call_results:
; CHECK: bl operand
; CHECK: bl operand
; CHECK: bl operand
; CHECK: fsel
; CHECK: blr
define float @select_call_results() {
  %comparand = call float @operand(i32 0)
  %ge = call float @operand(i32 1)
  %lt = call float @operand(i32 2)
  %result = call float @llvm.ppc.fsels(float %comparand, float %ge, float %lt)
  ret float %result
}

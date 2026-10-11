; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=g5 -mattr=+vmx128 -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=g5 -mattr=+vmx128 -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc-unknown-linux-gnu -mcpu=xenon -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc-unknown-linux-gnu -mcpu=xenon -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=XBOX
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=XBOX

declare <4 x i32> @llvm.ppc.altivec.get.vr128(i32 immarg)
declare void @llvm.ppc.altivec.set.vr128(<4 x i32>, i32 immarg)

define <4 x i32> @get_reg() {
; CHECK-LABEL: get_reg:
; CHECK:       vor128 2, 64, 64
; XBOX-LABEL: get_reg:
; XBOX:       vor128 1, 64, 64
  %v = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 64)
  ret <4 x i32> %v
}

define void @set_reg(<4 x i32> %v) {
; CHECK-LABEL: set_reg:
; CHECK:       vor128 65, 2, 2
; XBOX-LABEL: set_reg:
; XBOX:       vor128 65, 1, 1
  call void @llvm.ppc.altivec.set.vr128(<4 x i32> %v, i32 65)
  ret void
}

define <4 x i32> @get_reg_in_block(i1 %cond, <4 x i32> %fallback) {
; CHECK-LABEL: get_reg_in_block:
; CHECK: vor128 {{[0-9]+}}, 127, 127
; XBOX-LABEL: get_reg_in_block:
; XBOX: vor128 {{[0-9]+}}, 127, 127
entry:
  br i1 %cond, label %read, label %exit
read:
  %v = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 127)
  br label %exit
exit:
  %result = phi <4 x i32> [ %fallback, %entry ], [ %v, %read ]
  ret <4 x i32> %result
}

define <4 x i32> @get_reg_low() {
; CHECK-LABEL: get_reg_low:
; CHECK: vor128 {{[0-9]+}}, 0, 0
; XBOX-LABEL: get_reg_low:
; XBOX: vor128 {{[0-9]+}}, 0, 0
  %v = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 0)
  ret <4 x i32> %v
}

define <4 x i32> @set_then_get_low(<4 x i32> %v) {
; CHECK-LABEL: set_then_get_low:
; CHECK: vor128 0,
; CHECK: vor128 {{[0-9]+}}, 0, 0
; XBOX-LABEL: set_then_get_low:
; XBOX: vor128 0,
; XBOX: vor128 {{[0-9]+}}, 0, 0
  call void @llvm.ppc.altivec.set.vr128(<4 x i32> %v, i32 0)
  %result = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 0)
  ret <4 x i32> %result
}

define <4 x i32> @read_write_read(<4 x i32> %v) {
; CHECK-LABEL: read_write_read:
; CHECK: vor128 {{[0-9]+}}, 65, 65
; CHECK: vor128 65,
; CHECK: vor128 {{[0-9]+}}, 65, 65
; XBOX-LABEL: read_write_read:
; XBOX: vor128 {{[0-9]+}}, 65, 65
; XBOX: vor128 65,
; XBOX: vor128 {{[0-9]+}}, 65, 65
  %before = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 65)
  call void @llvm.ppc.altivec.set.vr128(<4 x i32> %v, i32 65)
  %after = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 65)
  %result = xor <4 x i32> %before, %after
  ret <4 x i32> %result
}

define void @unused_read() {
; CHECK-LABEL: unused_read:
; CHECK: vor128 {{[0-9]+}}, 31, 31
; XBOX-LABEL: unused_read:
; XBOX: vor128 {{[0-9]+}}, 31, 31
  %v = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 31)
  ret void
}

define <4 x i32> @set_then_get(<4 x i32> %v) {
; CHECK-LABEL: set_then_get:
; CHECK: vor128 65,
; CHECK: vor128 {{[0-9]+}}, 65, 65
; XBOX-LABEL: set_then_get:
; XBOX: vor128 65,
; XBOX: vor128 {{[0-9]+}}, 65, 65
  call void @llvm.ppc.altivec.set.vr128(<4 x i32> %v, i32 65)
  %result = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 65)
  ret <4 x i32> %result
}
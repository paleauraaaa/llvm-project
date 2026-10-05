; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=g5 -mattr=+vmx128 < %s | \
; RUN:   FileCheck %s

declare <4 x i32> @llvm.ppc.altivec.get.vr128(i32 immarg)
declare void @llvm.ppc.altivec.set.vr128(<4 x i32>, i32 immarg)

define <4 x i32> @get_reg() {
; CHECK-LABEL: get_reg:
; CHECK:       vor128 2, 64, 64
  %v = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 64)
  ret <4 x i32> %v
}

define void @set_reg(<4 x i32> %v) {
; CHECK-LABEL: set_reg:
; CHECK:       vor128 65, 2, 2
  call void @llvm.ppc.altivec.set.vr128(<4 x i32> %v, i32 65)
  ret void
}
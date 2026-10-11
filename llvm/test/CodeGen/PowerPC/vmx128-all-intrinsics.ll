; Exercise all 86 VMX128 intrinsic signatures, including immediate operands.
; RUN: opt -passes=verify -disable-output %s
; Only the six currently selectable, machine-verifier-clean intrinsics are
; extracted for code generation. The remaining wrappers are reproducers for
; missing selection rules.
; RUN: llvm-extract --func=probe_get_vr128 --func=probe_set_vr128 --func=probe_lvlx128_volatile --func=probe_lvrx128_volatile --func=probe_stvlx128_volatile --func=probe_stvrx128_volatile -S %s -o %t.ll
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -verify-machineinstrs %t.ll -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs %t.ll -o - | FileCheck %s
; RUN: llc -mtriple=powerpc-unknown-linux-gnu -mcpu=xenon -relocation-model=static -O0 -verify-machineinstrs %t.ll -o - | FileCheck %s
; RUN: llc -mtriple=powerpc-unknown-linux-gnu -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs %t.ll -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=xenon -relocation-model=static -O0 -verify-machineinstrs %t.ll -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs %t.ll -o - | FileCheck %s

; CHECK-LABEL: probe_get_vr128:
; CHECK: vor128
declare <4 x i32> @llvm.ppc.altivec.get.vr128(i32)
define <4 x i32> @probe_get_vr128() {
  %result = call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 64)
  ret <4 x i32> %result
}

; CHECK-LABEL: probe_set_vr128:
; CHECK: vor128
declare void @llvm.ppc.altivec.set.vr128(<4 x i32>, i32)
define void @probe_set_vr128(<4 x i32> %a0) {
  call void @llvm.ppc.altivec.set.vr128(<4 x i32> %a0, i32 65)
  ret void
}

declare <4 x i32> @llvm.ppc.altivec.lvewx128(ptr, i32)
define <4 x i32> @probe_lvewx128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvewx128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.lvlx128(ptr, i32)
define <4 x i32> @probe_lvlx128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvlx128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

; CHECK-LABEL: probe_lvlx128_volatile:
; CHECK: lvlx128
declare <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr, i32)
define <4 x i32> @probe_lvlx128_volatile(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.lvlxl128(ptr, i32)
define <4 x i32> @probe_lvlxl128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvlxl128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.lvrx128(ptr, i32)
define <4 x i32> @probe_lvrx128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvrx128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

; CHECK-LABEL: probe_lvrx128_volatile:
; CHECK: lvrx128
declare <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr, i32)
define <4 x i32> @probe_lvrx128_volatile(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.lvrxl128(ptr, i32)
define <4 x i32> @probe_lvrxl128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvrxl128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare <16 x i8> @llvm.ppc.altivec.lvsl128(ptr, i32)
define <16 x i8> @probe_lvsl128(ptr %a0, i32 %a1) {
  %result = call <16 x i8> @llvm.ppc.altivec.lvsl128(ptr %a0, i32 %a1)
  ret <16 x i8> %result
}

declare <16 x i8> @llvm.ppc.altivec.lvsr128(ptr, i32)
define <16 x i8> @probe_lvsr128(ptr %a0, i32 %a1) {
  %result = call <16 x i8> @llvm.ppc.altivec.lvsr128(ptr %a0, i32 %a1)
  ret <16 x i8> %result
}

declare <4 x i32> @llvm.ppc.altivec.lvx128(ptr, i32)
define <4 x i32> @probe_lvx128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvx128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.lvxl128(ptr, i32)
define <4 x i32> @probe_lvxl128(ptr %a0, i32 %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.lvxl128(ptr %a0, i32 %a1)
  ret <4 x i32> %result
}

declare void @llvm.ppc.altivec.stvewx128(<4 x i32>, ptr, i32)
define void @probe_stvewx128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvewx128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare void @llvm.ppc.altivec.stvlx128(<4 x i32>, ptr, i32)
define void @probe_stvlx128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvlx128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

; CHECK-LABEL: probe_stvlx128_volatile:
; CHECK: stvlx128
declare void @llvm.ppc.altivec.stvlx128.volatile(<4 x i32>, ptr, i32)
define void @probe_stvlx128_volatile(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvlx128.volatile(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare void @llvm.ppc.altivec.stvlxl128(<4 x i32>, ptr, i32)
define void @probe_stvlxl128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvlxl128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare void @llvm.ppc.altivec.stvrx128(<4 x i32>, ptr, i32)
define void @probe_stvrx128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvrx128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

; CHECK-LABEL: probe_stvrx128_volatile:
; CHECK: stvrx128
declare void @llvm.ppc.altivec.stvrx128.volatile(<4 x i32>, ptr, i32)
define void @probe_stvrx128_volatile(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvrx128.volatile(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare void @llvm.ppc.altivec.stvrxl128(<4 x i32>, ptr, i32)
define void @probe_stvrxl128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvrxl128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare void @llvm.ppc.altivec.stvx128(<4 x i32>, ptr, i32)
define void @probe_stvx128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvx128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare void @llvm.ppc.altivec.stvxl128(<4 x i32>, ptr, i32)
define void @probe_stvxl128(<4 x i32> %a0, ptr %a1, i32 %a2) {
  call void @llvm.ppc.altivec.stvxl128(<4 x i32> %a0, ptr %a1, i32 %a2)
  ret void
}

declare <4 x float> @llvm.ppc.altivec.vaddfp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vaddfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vaddfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x i32> @llvm.ppc.altivec.vand128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vand128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vand128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vandc128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vandc128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vandc128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x float> @llvm.ppc.altivec.vmaxfp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vmaxfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vmaxfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vminfp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vminfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vminfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x i32> @llvm.ppc.altivec.vnor128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vnor128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vnor128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vor128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vor128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vor128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vxor128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vxor128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vxor128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x float> @llvm.ppc.altivec.vsubfp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vsubfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vsubfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpbfp128(<4 x float>, <4 x float>)
define <4 x i32> @probe_vcmpbfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpbfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpbfp128.rec(<4 x float>, <4 x float>, ptr)
define <4 x i32> @probe_vcmpbfp128_rec(<4 x float> %a0, <4 x float> %a1, ptr %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpbfp128.rec(<4 x float> %a0, <4 x float> %a1, ptr %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpeqfp128(<4 x float>, <4 x float>)
define <4 x i32> @probe_vcmpeqfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float>, <4 x float>, ptr)
define <4 x i32> @probe_vcmpeqfp128_rec(<4 x float> %a0, <4 x float> %a1, ptr %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float> %a0, <4 x float> %a1, ptr %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpgefp128(<4 x float>, <4 x float>)
define <4 x i32> @probe_vcmpgefp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpgefp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpgefp128.rec(<4 x float>, <4 x float>, ptr)
define <4 x i32> @probe_vcmpgefp128_rec(<4 x float> %a0, <4 x float> %a1, ptr %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpgefp128.rec(<4 x float> %a0, <4 x float> %a1, ptr %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpgtfp128(<4 x float>, <4 x float>)
define <4 x i32> @probe_vcmpgtfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpgtfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpgtfp128.rec(<4 x float>, <4 x float>, ptr)
define <4 x i32> @probe_vcmpgtfp128_rec(<4 x float> %a0, <4 x float> %a1, ptr %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpgtfp128.rec(<4 x float> %a0, <4 x float> %a1, ptr %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpequw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vcmpequw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpequw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcmpequw128.rec(<4 x i32>, <4 x i32>, ptr)
define <4 x i32> @probe_vcmpequw128_rec(<4 x i32> %a0, <4 x i32> %a1, ptr %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcmpequw128.rec(<4 x i32> %a0, <4 x i32> %a1, ptr %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcfpsxws128(<4 x float>, i32)
define <4 x i32> @probe_vcfpsxws128(<4 x float> %a0) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcfpsxws128(<4 x float> %a0, i32 1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vcfpuxws128(<4 x float>, i32)
define <4 x i32> @probe_vcfpuxws128(<4 x float> %a0) {
  %result = call <4 x i32> @llvm.ppc.altivec.vcfpuxws128(<4 x float> %a0, i32 1)
  ret <4 x i32> %result
}

declare <4 x float> @llvm.ppc.altivec.vcsxwfp128(<4 x i32>, i32)
define <4 x float> @probe_vcsxwfp128(<4 x i32> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vcsxwfp128(<4 x i32> %a0, i32 1)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vcuxwfp128(<4 x i32>, i32)
define <4 x float> @probe_vcuxwfp128(<4 x i32> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vcuxwfp128(<4 x i32> %a0, i32 1)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vexptefp128(<4 x float>)
define <4 x float> @probe_vexptefp128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vexptefp128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vlogefp128(<4 x float>)
define <4 x float> @probe_vlogefp128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vlogefp128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vrefp128(<4 x float>)
define <4 x float> @probe_vrefp128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vrefp128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vrsqrtefp128(<4 x float>)
define <4 x float> @probe_vrsqrtefp128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vrsqrtefp128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vrfim128(<4 x float>)
define <4 x float> @probe_vrfim128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vrfim128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vrfin128(<4 x float>)
define <4 x float> @probe_vrfin128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vrfin128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vrfip128(<4 x float>)
define <4 x float> @probe_vrfip128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vrfip128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vrfiz128(<4 x float>)
define <4 x float> @probe_vrfiz128(<4 x float> %a0) {
  %result = call <4 x float> @llvm.ppc.altivec.vrfiz128(<4 x float> %a0)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vmaddcfp128(<4 x float>, <4 x float>, <4 x float>)
define <4 x float> @probe_vmaddcfp128(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) {
  %result = call <4 x float> @llvm.ppc.altivec.vmaddcfp128(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vmaddfp128(<4 x float>, <4 x float>, <4 x float>)
define <4 x float> @probe_vmaddfp128(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) {
  %result = call <4 x float> @llvm.ppc.altivec.vmaddfp128(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vmsum3fp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vmsum3fp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vmsum3fp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vmsum4fp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vmsum4fp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vmsum4fp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vmulfp128(<4 x float>, <4 x float>)
define <4 x float> @probe_vmulfp128(<4 x float> %a0, <4 x float> %a1) {
  %result = call <4 x float> @llvm.ppc.altivec.vmulfp128(<4 x float> %a0, <4 x float> %a1)
  ret <4 x float> %result
}

declare <4 x float> @llvm.ppc.altivec.vnmsubfp128(<4 x float>, <4 x float>, <4 x float>)
define <4 x float> @probe_vnmsubfp128(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) {
  %result = call <4 x float> @llvm.ppc.altivec.vnmsubfp128(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %result
}

declare <4 x i32> @llvm.ppc.altivec.vmrghw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vmrghw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vmrghw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vmrglw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vmrglw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vmrglw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vperm128(<4 x i32>, <4 x i32>, <16 x i8>)
define <4 x i32> @probe_vperm128(<4 x i32> %a0, <4 x i32> %a1, <16 x i8> %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vperm128(<4 x i32> %a0, <4 x i32> %a1, <16 x i8> %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vpermwi128(<4 x i32>, i32)
define <4 x i32> @probe_vpermwi128(<4 x i32> %a0) {
  %result = call <4 x i32> @llvm.ppc.altivec.vpermwi128(<4 x i32> %a0, i32 1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vpkd3d128(<4 x i32>, <4 x i32>, i32, i32, i32)
define <4 x i32> @probe_vpkd3d128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vpkd3d128(<4 x i32> %a0, <4 x i32> %a1, i32 1, i32 1, i32 1)
  ret <4 x i32> %result
}

declare <16 x i8> @llvm.ppc.altivec.vpkshss128(<8 x i16>, <8 x i16>)
define <16 x i8> @probe_vpkshss128(<8 x i16> %a0, <8 x i16> %a1) {
  %result = call <16 x i8> @llvm.ppc.altivec.vpkshss128(<8 x i16> %a0, <8 x i16> %a1)
  ret <16 x i8> %result
}

declare <16 x i8> @llvm.ppc.altivec.vpkshus128(<8 x i16>, <8 x i16>)
define <16 x i8> @probe_vpkshus128(<8 x i16> %a0, <8 x i16> %a1) {
  %result = call <16 x i8> @llvm.ppc.altivec.vpkshus128(<8 x i16> %a0, <8 x i16> %a1)
  ret <16 x i8> %result
}

declare <8 x i16> @llvm.ppc.altivec.vpkswss128(<4 x i32>, <4 x i32>)
define <8 x i16> @probe_vpkswss128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <8 x i16> @llvm.ppc.altivec.vpkswss128(<4 x i32> %a0, <4 x i32> %a1)
  ret <8 x i16> %result
}

declare <8 x i16> @llvm.ppc.altivec.vpkswus128(<4 x i32>, <4 x i32>)
define <8 x i16> @probe_vpkswus128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <8 x i16> @llvm.ppc.altivec.vpkswus128(<4 x i32> %a0, <4 x i32> %a1)
  ret <8 x i16> %result
}

declare <16 x i8> @llvm.ppc.altivec.vpkuhum128(<8 x i16>, <8 x i16>)
define <16 x i8> @probe_vpkuhum128(<8 x i16> %a0, <8 x i16> %a1) {
  %result = call <16 x i8> @llvm.ppc.altivec.vpkuhum128(<8 x i16> %a0, <8 x i16> %a1)
  ret <16 x i8> %result
}

declare <16 x i8> @llvm.ppc.altivec.vpkuhus128(<8 x i16>, <8 x i16>)
define <16 x i8> @probe_vpkuhus128(<8 x i16> %a0, <8 x i16> %a1) {
  %result = call <16 x i8> @llvm.ppc.altivec.vpkuhus128(<8 x i16> %a0, <8 x i16> %a1)
  ret <16 x i8> %result
}

declare <8 x i16> @llvm.ppc.altivec.vpkuwum128(<4 x i32>, <4 x i32>)
define <8 x i16> @probe_vpkuwum128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <8 x i16> @llvm.ppc.altivec.vpkuwum128(<4 x i32> %a0, <4 x i32> %a1)
  ret <8 x i16> %result
}

declare <8 x i16> @llvm.ppc.altivec.vpkuwus128(<4 x i32>, <4 x i32>)
define <8 x i16> @probe_vpkuwus128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <8 x i16> @llvm.ppc.altivec.vpkuwus128(<4 x i32> %a0, <4 x i32> %a1)
  ret <8 x i16> %result
}

declare <8 x i16> @llvm.ppc.altivec.vupkhsb128(<16 x i8>)
define <8 x i16> @probe_vupkhsb128(<16 x i8> %a0) {
  %result = call <8 x i16> @llvm.ppc.altivec.vupkhsb128(<16 x i8> %a0)
  ret <8 x i16> %result
}

declare <8 x i16> @llvm.ppc.altivec.vupklsb128(<16 x i8>)
define <8 x i16> @probe_vupklsb128(<16 x i8> %a0) {
  %result = call <8 x i16> @llvm.ppc.altivec.vupklsb128(<16 x i8> %a0)
  ret <8 x i16> %result
}

declare <4 x i32> @llvm.ppc.altivec.vrlimi128(<4 x i32>, <4 x i32>, i32, i32)
define <4 x i32> @probe_vrlimi128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vrlimi128(<4 x i32> %a0, <4 x i32> %a1, i32 1, i32 1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vrlw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vrlw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vrlw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vsel128(<4 x i32>, <4 x i32>, <4 x i32>)
define <4 x i32> @probe_vsel128(<4 x i32> %a0, <4 x i32> %a1, <4 x i32> %a2) {
  %result = call <4 x i32> @llvm.ppc.altivec.vsel128(<4 x i32> %a0, <4 x i32> %a1, <4 x i32> %a2)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vsldoi128(<4 x i32>, <4 x i32>, i32)
define <4 x i32> @probe_vsldoi128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vsldoi128(<4 x i32> %a0, <4 x i32> %a1, i32 1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vslo128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vslo128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vslo128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vslw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vslw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vslw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vspltisw128(i32)
define <4 x i32> @probe_vspltisw128() {
  %result = call <4 x i32> @llvm.ppc.altivec.vspltisw128(i32 1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vspltw128(<4 x i32>, i32)
define <4 x i32> @probe_vspltw128(<4 x i32> %a0) {
  %result = call <4 x i32> @llvm.ppc.altivec.vspltw128(<4 x i32> %a0, i32 1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vsraw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vsraw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vsraw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vsro128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vsro128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vsro128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vsrw128(<4 x i32>, <4 x i32>)
define <4 x i32> @probe_vsrw128(<4 x i32> %a0, <4 x i32> %a1) {
  %result = call <4 x i32> @llvm.ppc.altivec.vsrw128(<4 x i32> %a0, <4 x i32> %a1)
  ret <4 x i32> %result
}

declare <4 x i32> @llvm.ppc.altivec.vupkd3d128(<4 x i32>, i32)
define <4 x i32> @probe_vupkd3d128(<4 x i32> %a0) {
  %result = call <4 x i32> @llvm.ppc.altivec.vupkd3d128(<4 x i32> %a0, i32 1)
  ret <4 x i32> %result
}

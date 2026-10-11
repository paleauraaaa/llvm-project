; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,XBOX
; RUN: llc -mtriple=powerpc-unknown-linux-gnu -mcpu=xenon -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,PPC
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=xenon -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,PPC

declare <4 x i32> @llvm.ppc.altivec.lvx128(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.lvlx128(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr, i32)
declare <16 x i8> @llvm.ppc.altivec.lvsl128(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.vcfpsxws128(<4 x float>, i32 immarg)
declare <4 x i32> @llvm.ppc.altivec.vspltisw128(i32 immarg)
declare <4 x i32> @llvm.ppc.altivec.vpermwi128(<4 x i32>, i32 immarg)
declare <4 x i32> @llvm.ppc.altivec.vrlimi128(<4 x i32>, <4 x i32>, i32 immarg, i32 immarg)
declare <4 x i32> @llvm.ppc.altivec.vpkd3d128(<4 x i32>, <4 x i32>, i32 immarg, i32 immarg, i32 immarg)
declare <4 x i32> @llvm.ppc.altivec.vperm128(<4 x i32>, <4 x i32>, <16 x i8>)
declare <4 x float> @llvm.ppc.altivec.vmaddcfp128(<4 x float>, <4 x float>, <4 x float>)
declare <16 x i8> @llvm.ppc.altivec.vpkshss128(<8 x i16>, <8 x i16>)
declare <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float>, <4 x float>, ptr)

define <4 x i32> @load_address(ptr %p, i32 %off) {
; CHECK-LABEL: load_address:
; XBOX: add [[SUM:[0-9]+]], 3, 4
; XBOX: clrldi [[ADDR:[0-9]+]], [[SUM]], 32
; XBOX: lvx128 1, 0, [[ADDR]]
; PPC: lvx128
  %v = call <4 x i32> @llvm.ppc.altivec.lvx128(ptr %p, i32 %off)
  ret <4 x i32> %v
}

define <16 x i8> @permute_address(ptr %p, i32 %off) {
; CHECK-LABEL: permute_address:
; XBOX: add [[SUM:[0-9]+]], 3, 4
; XBOX: clrldi [[ADDR:[0-9]+]], [[SUM]], 32
; XBOX: lvsl128 1, 0, [[ADDR]]
; PPC: lvsl128
  %v = call <16 x i8> @llvm.ppc.altivec.lvsl128(ptr %p, i32 %off)
  ret <16 x i8> %v
}

define <4 x i32> @scale_max(<4 x float> %v) {
; CHECK-LABEL: scale_max:
; CHECK: vcfpsxws128 {{[0-9]+}}, {{[0-9]+}}, 31
  %r = call <4 x i32> @llvm.ppc.altivec.vcfpsxws128(<4 x float> %v, i32 31)
  ret <4 x i32> %r
}

define <4 x i32> @splat_min() {
; CHECK-LABEL: splat_min:
; CHECK: vspltisw128 {{[0-9]+}}, 0, -16
  %r = call <4 x i32> @llvm.ppc.altivec.vspltisw128(i32 -16)
  ret <4 x i32> %r
}

define <4 x i32> @permute_max(<4 x i32> %v) {
; CHECK-LABEL: permute_max:
; CHECK: vpermwi128 {{[0-9]+}}, {{[0-9]+}}, 255
  %r = call <4 x i32> @llvm.ppc.altivec.vpermwi128(<4 x i32> %v, i32 255)
  ret <4 x i32> %r
}

define <4 x i32> @insert_words(<4 x i32> %dst, <4 x i32> %src) {
; CHECK-LABEL: insert_words:
; XBOX: vrlimi128 1, 2, 9, 2
; PPC: vrlimi128 2, 3, 9, 2
  %r = call <4 x i32> @llvm.ppc.altivec.vrlimi128(<4 x i32> %dst, <4 x i32> %src, i32 9, i32 2)
  ret <4 x i32> %r
}

define <4 x i32> @pack_d3d(<4 x i32> %dst, <4 x i32> %src) {
; CHECK-LABEL: pack_d3d:
; XBOX: vpkd3d128 1, 2, 7, 3, 3
; PPC: vpkd3d128 2, 3, 7, 3, 3
  %r = call <4 x i32> @llvm.ppc.altivec.vpkd3d128(<4 x i32> %dst, <4 x i32> %src, i32 7, i32 3, i32 3)
  ret <4 x i32> %r
}

define <4 x i32> @permute_mask(<4 x i32> %a, <4 x i32> %b, <16 x i8> %mask) {
; CHECK-LABEL: permute_mask:
; CHECK: vperm128 {{[0-9]+}}, {{[0-9]+}}, {{[0-9]+}}, {{[0-7]}}{{$}}
  %r = call <4 x i32> @llvm.ppc.altivec.vperm128(<4 x i32> %a, <4 x i32> %b, <16 x i8> %mask)
  ret <4 x i32> %r
}

define <4 x float> @madd_tied_multiplier(<4 x float> %a, <4 x float> %b, <4 x float> %c) {
; CHECK-LABEL: madd_tied_multiplier:
; XBOX: vmaddcfp128 2, 1, 2, 3
; PPC: vmaddcfp128 3, 2, 3, 4
  %r = call <4 x float> @llvm.ppc.altivec.vmaddcfp128(<4 x float> %a, <4 x float> %b, <4 x float> %c)
  ret <4 x float> %r
}

define void @unused_loads(ptr %p, i32 %off) {
; CHECK-LABEL: unused_loads:
; CHECK: lvlx128
; CHECK-NOT: lvlx128
; CHECK: blr
  %a = call <4 x i32> @llvm.ppc.altivec.lvlx128(ptr %p, i32 %off)
  %b = call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr %p, i32 %off)
  ret void
}

define void @unused_saturating_pack(<8 x i16> %a, <8 x i16> %b) {
; CHECK-LABEL: unused_saturating_pack:
; CHECK: vpkshss128
  %r = call <16 x i8> @llvm.ppc.altivec.vpkshss128(<8 x i16> %a, <8 x i16> %b)
  ret void
}

define void @unused_record_result(<4 x float> %a, <4 x float> %b, ptr %cr) {
; CHECK-LABEL: unused_record_result:
; CHECK: vcmpeqfp128.
; CHECK: mfocrf [[CR:[0-9]+]], 2
; CHECK: stw [[CR]],
  %r = call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float> %a, <4 x float> %b, ptr %cr)
  ret void
}

define void @two_record_forms(<4 x float> %a, <4 x float> %b, ptr %first, ptr %second) {
; CHECK-LABEL: two_record_forms:
; CHECK: vcmpeqfp128.
; CHECK: mfocrf [[CR1:[0-9]+]], 2
; CHECK: stw [[CR1]],
; CHECK: vcmpeqfp128.
; CHECK: mfocrf [[CR2:[0-9]+]], 2
; CHECK: stw [[CR2]],
  %r1 = call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float> %a, <4 x float> %b, ptr %first)
  %r2 = call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float> %b, <4 x float> %a, ptr %second)
  ret void
}

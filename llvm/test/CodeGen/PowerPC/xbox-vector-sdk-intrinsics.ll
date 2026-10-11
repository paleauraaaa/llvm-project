; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -verify-machineinstrs %s -o %t.O0.s
; RUN: FileCheck %s < %t.O0.s
; RUN: FileCheck %s --check-prefix=STORES < %t.O0.s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs %s -o %t.O2.s
; RUN: FileCheck %s < %t.O2.s
; RUN: FileCheck %s --check-prefix=STORES < %t.O2.s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -filetype=obj -o %t.obj

; The SDK record comparisons return the vector mask and write the unshifted
; CR6 image, including when the vector result is unused.
; STORES-LABEL: records:
; STORES-COUNT-8: stw
; STORES: blr
; CHECK-LABEL: records:
; CHECK: vcmpequb.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpgtsb.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpgtub.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpequh.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpgtsh.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpgtuh.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpgtsw.
; CHECK: mfocrf {{.*}}, 2
; CHECK: vcmpgtuw.
; CHECK: mfocrf {{.*}}, 2
; CHECK: stw
define void @records(ptr %a, ptr %b, ptr %cr) {
  %ab = load <16 x i8>, ptr %a, align 16
  %bb = load <16 x i8>, ptr %b, align 16
  %ah = bitcast <16 x i8> %ab to <8 x i16>
  %bh = bitcast <16 x i8> %bb to <8 x i16>
  %aw = bitcast <16 x i8> %ab to <4 x i32>
  %bw = bitcast <16 x i8> %bb to <4 x i32>
  %eqb = call <16 x i8> @llvm.ppc.altivec.vcmpequb.rec(<16 x i8> %ab, <16 x i8> %bb, ptr %cr)
  %gtsb = call <16 x i8> @llvm.ppc.altivec.vcmpgtsb.rec(<16 x i8> %ab, <16 x i8> %bb, ptr %cr)
  %gtub = call <16 x i8> @llvm.ppc.altivec.vcmpgtub.rec(<16 x i8> %ab, <16 x i8> %bb, ptr %cr)
  %eqh = call <8 x i16> @llvm.ppc.altivec.vcmpequh.rec(<8 x i16> %ah, <8 x i16> %bh, ptr %cr)
  %gtsh = call <8 x i16> @llvm.ppc.altivec.vcmpgtsh.rec(<8 x i16> %ah, <8 x i16> %bh, ptr %cr)
  %gtuh = call <8 x i16> @llvm.ppc.altivec.vcmpgtuh.rec(<8 x i16> %ah, <8 x i16> %bh, ptr %cr)
  %gtsw = call <4 x i32> @llvm.ppc.altivec.vcmpgtsw.rec(<4 x i32> %aw, <4 x i32> %bw, ptr %cr)
  %gtuw = call <4 x i32> @llvm.ppc.altivec.vcmpgtuw.rec(<4 x i32> %aw, <4 x i32> %bw, ptr %cr)
  ret void
}

; CHECK-LABEL: volatile_loads:
; CHECK-COUNT-2: lvebx
; CHECK-COUNT-2: lvehx
; CHECK-COUNT-2: lvewx128
; CHECK-COUNT-2: lvx128
; CHECK-COUNT-2: lvlx128
; CHECK-COUNT-2: lvrx128
define void @volatile_loads(ptr %p, i32 %offset) {
  %p1 = getelementptr i8, ptr %p, i32 %offset
  %b0 = call <16 x i8> @llvm.ppc.altivec.lvebx.volatile(ptr %p1)
  %b1 = call <16 x i8> @llvm.ppc.altivec.lvebx.volatile(ptr %p1)
  %h0 = call <8 x i16> @llvm.ppc.altivec.lvehx.volatile(ptr %p1)
  %h1 = call <8 x i16> @llvm.ppc.altivec.lvehx.volatile(ptr %p1)
  %w0 = call <4 x i32> @llvm.ppc.altivec.lvewx128.volatile(ptr %p, i32 %offset)
  %w1 = call <4 x i32> @llvm.ppc.altivec.lvewx128.volatile(ptr %p, i32 %offset)
  %v0 = call <4 x i32> @llvm.ppc.altivec.lvx128.volatile(ptr %p, i32 %offset)
  %v1 = call <4 x i32> @llvm.ppc.altivec.lvx128.volatile(ptr %p, i32 %offset)
  %l0 = call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr %p, i32 %offset)
  %l1 = call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr %p, i32 %offset)
  %r0 = call <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr %p, i32 %offset)
  %r1 = call <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr %p, i32 %offset)
  ret void
}

; CHECK-LABEL: volatile_stores:
; CHECK-COUNT-2: stvebx
; CHECK-COUNT-2: stvehx
; CHECK-COUNT-2: stvewx128
; CHECK-COUNT-2: stvx128
define void @volatile_stores(ptr %p, ptr %v, i32 %offset) {
  %a = load <4 x i32>, ptr %v, align 16
  %ab = bitcast <4 x i32> %a to <16 x i8>
  %ah = bitcast <4 x i32> %a to <8 x i16>
  %p1 = getelementptr i8, ptr %p, i32 %offset
  call void @llvm.ppc.altivec.stvebx.volatile(<16 x i8> %ab, ptr %p1)
  call void @llvm.ppc.altivec.stvebx.volatile(<16 x i8> %ab, ptr %p1)
  call void @llvm.ppc.altivec.stvehx.volatile(<8 x i16> %ah, ptr %p1)
  call void @llvm.ppc.altivec.stvehx.volatile(<8 x i16> %ah, ptr %p1)
  call void @llvm.ppc.altivec.stvewx128.volatile(<4 x i32> %a, ptr %p, i32 %offset)
  call void @llvm.ppc.altivec.stvewx128.volatile(<4 x i32> %a, ptr %p, i32 %offset)
  call void @llvm.ppc.altivec.stvx128.volatile(<4 x i32> %a, ptr %p, i32 %offset)
  call void @llvm.ppc.altivec.stvx128.volatile(<4 x i32> %a, ptr %p, i32 %offset)
  ret void
}

declare <16 x i8> @llvm.ppc.altivec.vcmpequb.rec(<16 x i8>, <16 x i8>, ptr)
declare <16 x i8> @llvm.ppc.altivec.vcmpgtsb.rec(<16 x i8>, <16 x i8>, ptr)
declare <16 x i8> @llvm.ppc.altivec.vcmpgtub.rec(<16 x i8>, <16 x i8>, ptr)
declare <8 x i16> @llvm.ppc.altivec.vcmpequh.rec(<8 x i16>, <8 x i16>, ptr)
declare <8 x i16> @llvm.ppc.altivec.vcmpgtsh.rec(<8 x i16>, <8 x i16>, ptr)
declare <8 x i16> @llvm.ppc.altivec.vcmpgtuh.rec(<8 x i16>, <8 x i16>, ptr)
declare <4 x i32> @llvm.ppc.altivec.vcmpgtsw.rec(<4 x i32>, <4 x i32>, ptr)
declare <4 x i32> @llvm.ppc.altivec.vcmpgtuw.rec(<4 x i32>, <4 x i32>, ptr)
declare <16 x i8> @llvm.ppc.altivec.lvebx.volatile(ptr)
declare <8 x i16> @llvm.ppc.altivec.lvehx.volatile(ptr)
declare <4 x i32> @llvm.ppc.altivec.lvewx128.volatile(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.lvx128.volatile(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr, i32)
declare void @llvm.ppc.altivec.stvebx.volatile(<16 x i8>, ptr)
declare void @llvm.ppc.altivec.stvehx.volatile(<8 x i16>, ptr)
declare void @llvm.ppc.altivec.stvewx128.volatile(<4 x i32>, ptr, i32)
declare void @llvm.ppc.altivec.stvx128.volatile(<4 x i32>, ptr, i32)

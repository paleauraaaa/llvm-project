; RUN: llc -mtriple=powerpc-unknown-linux-gnu -mcpu=g5 -mattr=+vmx128 < %s | \
; RUN:   FileCheck %s --check-prefixes=CHECK,PPC32
; RUN: llc -mtriple=powerpc64-unknown-linux-gnu -mcpu=g5 -mattr=+vmx128 < %s | \
; RUN:   FileCheck %s --check-prefixes=CHECK,PPC64

declare <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr, i32)
declare <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr, i32)
declare void @llvm.ppc.altivec.stvlx128.volatile(<4 x i32>, ptr, i32)
declare void @llvm.ppc.altivec.stvrx128.volatile(<4 x i32>, ptr, i32)

define <4 x i32> @load_left(ptr %p, i32 %off) {
; CHECK-LABEL: load_left:
; PPC32: lvlx128 2, 3, 4
; PPC64: extsw 4, 4
; PPC64: lvlx128 2, 3, 4
  %v = call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr %p, i32 %off)
  ret <4 x i32> %v
}

define <4 x i32> @load_right(ptr %p, i32 %off) {
; CHECK-LABEL: load_right:
; PPC32: lvrx128 2, 3, 4
; PPC64: extsw 4, 4
; PPC64: lvrx128 2, 3, 4
  %v = call <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr %p, i32 %off)
  ret <4 x i32> %v
}

define void @store_left(<4 x i32> %v, ptr %p, i32 %off) {
; CHECK-LABEL: store_left:
; PPC32: stvlx128 2, 3, 4
; PPC64: extsw [[OFF:[0-9]+]], 6
; PPC64: stvlx128 2, 5, [[OFF]]
  call void @llvm.ppc.altivec.stvlx128.volatile(<4 x i32> %v, ptr %p, i32 %off)
  ret void
}

define void @store_right(<4 x i32> %v, ptr %p, i32 %off) {
; CHECK-LABEL: store_right:
; PPC32: stvrx128 2, 3, 4
; PPC64: extsw [[OFF:[0-9]+]], 6
; PPC64: stvrx128 2, 5, [[OFF]]
  call void @llvm.ppc.altivec.stvrx128.volatile(<4 x i32> %v, ptr %p, i32 %off)
  ret void
}

; Volatile accesses must not be removed even when the result is unused.
define void @unused_load(ptr %p, i32 %off) {
; CHECK-LABEL: unused_load:
; CHECK: lvlx128
  %v = call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr %p, i32 %off)
  ret void
}

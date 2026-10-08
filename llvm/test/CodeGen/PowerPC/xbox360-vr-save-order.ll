; RUN: llc -mtriple=ppc64-ibm-xbox360 -verify-machineinstrs %s -o - | FileCheck %s

; MSVC's __savevmx_64 places VR64 at -1024 and VR127 at -16
; relative to the aligned end of the preceding save area. Keep the
; register-numbered slots even when only a sparse subset is modified.
; CHECK-LABEL: sparse_vectors:
; CHECK: li [[INDEX:[0-9]+]], -1024
; CHECK-NEXT: stvx128 64, 1, [[INDEX]]
; CHECK: li [[INDEX]], -528
; CHECK-NEXT: stvx128 95, 1, [[INDEX]]
; CHECK: li [[INDEX]], -16
; CHECK-NEXT: stvx128 127, 1, [[INDEX]]
; CHECK: #APP
; CHECK: lvx128 127, 1, [[INDEX]]
; CHECK: li [[INDEX]], -528
; CHECK-NEXT: lvx128 95, 1, [[INDEX]]
; CHECK: li [[INDEX]], -1024
; CHECK-NEXT: lvx128 64, 1, [[INDEX]]
; CHECK: blr
define void @sparse_vectors() {
  call void asm sideeffect "", "~{v64},~{v95},~{v127}"()
  ret void
}

; Saving just VR127 must not reserve space for all 64 registers.
; CHECK-LABEL: last_vector:
; CHECK: li [[LAST:[0-9]+]], -16
; CHECK-NEXT: stvx128 127, 1, [[LAST]]
; CHECK: #APP
; CHECK: lvx128 127, 1, [[LAST]]
; CHECK: blr
define void @last_vector() {
  call void asm sideeffect "", "~{v127}"()
  ret void
}

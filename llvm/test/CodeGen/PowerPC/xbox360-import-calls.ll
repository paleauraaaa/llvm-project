; Verify Xbox 360 call lowering matches ABI call rules:
; - No TOC/function-descriptor call sequence for imported functions.
; - No linker-patch NOP slot after direct external calls.
; - Indirect function-pointer calls lower to mtctr + bctrl.
;
; RUN: llc -verify-machineinstrs -mtriple=ppc64-ibm-xbox360 %s -o - | FileCheck %s
; RUN: llc -verify-machineinstrs -relocation-model=static -mtriple=ppc64-ibm-xbox360 %s -o - | FileCheck --check-prefix=GLOBAL %s

target triple = "ppc64-ibm-xbox360"

@global_data = global i32 0, align 4

declare dllimport void @impfunc()
declare void @extfunc()

define dllexport void @expfunc() {
entry:
  ret void
}

define void @call_dllimport() {
entry:
  call void @impfunc()
  ret void
}

; CHECK-LABEL: call_dllimport:
; CHECK: bl impfunc
; CHECK-NOT: nop
; CHECK: blr

define void @call_external() {
entry:
  call void @extfunc()
  ret void
}

; CHECK-LABEL: call_external:
; CHECK: bl extfunc
; CHECK-NOT: nop
; CHECK: blr

define void @call_dllexport() {
entry:
  call void @expfunc()
  ret void
}

; CHECK-LABEL: call_dllexport:
; CHECK: bl expfunc
; CHECK-NOT: nop
; CHECK: blr

define void @call_indirect(ptr %fp) {
entry:
  call void %fp()
  ret void
}

; CHECK-LABEL: call_indirect:
; CHECK-NOT: ..r12_indirect
; CHECK-NOT: lwz 2,
; CHECK: mtctr
; CHECK: bctrl

define i32 @load_global_data() {
entry:
  %value = load i32, ptr @global_data, align 4
  ret i32 %value
}

; GLOBAL-LABEL: load_global_data:
; GLOBAL-NOT: @toc
; GLOBAL: lis 3, global_data@ha
; GLOBAL-NEXT: la 3, global_data@l(3)
; GLOBAL-NEXT: clrldi 3, 3, 32
; GLOBAL-NEXT: lwz 3, 0(3)
; GLOBAL-NEXT: blr

define void @store_global_data(i32 %value) {
entry:
  store i32 %value, ptr @global_data, align 4
  ret void
}

; GLOBAL-LABEL: store_global_data:
; GLOBAL-NOT: @toc
; GLOBAL: lis 4, global_data@ha
; GLOBAL-NEXT: la 4, global_data@l(4)
; GLOBAL-NEXT: clrldi 4, 4, 32
; GLOBAL-NEXT: stw 3, 0(4)
; GLOBAL-NEXT: blr

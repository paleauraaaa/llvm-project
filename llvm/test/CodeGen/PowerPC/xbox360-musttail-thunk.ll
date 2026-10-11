; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=ASM,OPT
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -tailcallopt -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static -O0 -stop-after=finalize-isel -verify-machineinstrs < %s | FileCheck %s --check-prefix=MIR

; The fixed pointer may be reloaded or adjusted. All unnamed register arguments
; and the caller's stack argument image must survive the tail transfer.
define i64 @integer_thunk(ptr %this, ...) #0 {
  %buffer = alloca [2048 x i8], align 16
  %address = ptrtoint ptr %this to i32
  %index = and i32 %address, 2047
  %byte = getelementptr [2048 x i8], ptr %buffer, i32 0, i32 %index
  store volatile i8 1, ptr %byte, align 1
  %slot = alloca ptr, align 4
  store volatile ptr %this, ptr %slot, align 4
  %reload = load volatile ptr, ptr %slot, align 4
  %adjusted = getelementptr i8, ptr %reload, i32 4
  %vtable = load ptr, ptr %adjusted, align 4
  %target = load ptr, ptr %vtable, align 4
  %result = musttail call i64 (ptr, ...) %target(ptr %adjusted, ...)
  ret i64 %result
}

; ASM-LABEL: integer_thunk:
; ASM: stwu 1, -[[FRAME:[0-9]+]](1)
; ASM-NOT: bctrl
; ASM: mtctr
; ASM: addi 1, 1, [[FRAME]]
; ASM-NEXT: ld 31, -8(1)
; ASM-NEXT: bctr
; MIR-LABEL: name: integer_thunk
; MIR: liveins: $x3, $x4, $x5, $x6, $x7, $x8, $x9, $x10, $f1, $f2, $f3, $f4, $f5, $f6, $f7, $f8, $f9, $f10, $f11, $f12, $f13, $v1, $v2, $v3, $v4, $v5, $v6, $v7, $v8, $v9, $v10, $v11, $v12, $v13
; MIR: TCRETURNri8 $ctr8, 0,
; MIR-SAME: implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit $x7, implicit $x8, implicit $x9, implicit $x10, implicit $f1, implicit $f2, implicit $f3, implicit $f4, implicit $f5, implicit $f6, implicit $f7, implicit $f8, implicit $f9, implicit $f10, implicit $f11, implicit $f12, implicit $f13, implicit $v1, implicit $v2, implicit $v3, implicit $v4, implicit $v5, implicit $v6, implicit $v7, implicit $v8, implicit $v9, implicit $v10, implicit $v11, implicit $v12, implicit $v13

define void @aggregate_thunk(ptr sret([9 x i8]) %result, ptr %this, ...) #0 {
  %vtable = load ptr, ptr %this, align 4
  %target = load ptr, ptr %vtable, align 4
  musttail call void (ptr, ptr, ...) %target(ptr sret([9 x i8]) %result, ptr %this, ...)
  ret void
}

; ASM-LABEL: aggregate_thunk:
; ASM-NOT: bctrl
; OPT: clrldi [[VTABLE:[0-9]+]], 4, 32
; OPT-NOT: lwz {{[0-9]+}}, 0(3)
; OPT: lwz [[VTABLE]], 0([[VTABLE]])
; OPT: lwz [[VTABLE]], 0([[VTABLE]])
; ASM: mtctr
; ASM: bctr
; MIR-LABEL: name: aggregate_thunk
; MIR: TCRETURNri8 $ctr8, 0,
; MIR-SAME: implicit $x3, implicit $x4, implicit $x5
; MIR-SAME: implicit $x10, implicit $f1
; MIR-SAME: implicit $f13, implicit $v1
; MIR-SAME: implicit $v13

define <4 x float> @vector_thunk(ptr %this, ...) #0 {
  %vtable = load ptr, ptr %this, align 4
  %target = load ptr, ptr %vtable, align 4
  %result = musttail call <4 x float> (ptr, ...) %target(ptr %this, ...)
  ret <4 x float> %result
}

; ASM-LABEL: vector_thunk:
; ASM-NOT: bctrl
; ASM: mtctr
; ASM: bctr
; MIR-LABEL: name: vector_thunk
; MIR: TCRETURNri8 $ctr8, 0,
; MIR-SAME: implicit $v1
; MIR-SAME: implicit $v13

attributes #0 = { "thunk" "frame-pointer"="all" }

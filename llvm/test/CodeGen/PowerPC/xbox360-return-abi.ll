; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=false -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR0
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=MIR2

; Xbox MSVC preserves the incoming LR around direct and indirect calls, returns
; direct vectors in V1, and returns the original hidden result pointer in R3.

%Result = type { i64, i64 }

declare i64 @callee(i64)
declare <4 x float> @vector_callee()
declare void @fill(ptr)

; CHECK-LABEL: direct_call:
; CHECK: mflr 12
; CHECK: stw 12, -8(1)
; CHECK: bl callee
; CHECK: lwz 12, -8(1)
; CHECK: mtlr 12
; CHECK: blr
define i64 @direct_call(i64 %arg) {
  %value = call i64 @callee(i64 %arg)
  %result = add i64 %value, 1
  ret i64 %result
}

; CHECK-LABEL: indirect_call:
; CHECK: mflr 12
; CHECK: stw 12, -8(1)
; CHECK: bctrl
; CHECK: lwz 12, -8(1)
; CHECK: mtlr 12
; CHECK: blr
define i64 @indirect_call(ptr %fn, i64 %arg) {
  %value = call i64 %fn(i64 %arg)
  %result = add i64 %value, 1
  ret i64 %result
}

; Loading the result immediately before the return makes the destination
; register observable independently of the vector argument convention.
; CHECK-LABEL: vector_return:
; CHECK: lvx 1,
; CHECK: blr
define <4 x float> @vector_return(ptr %source) {
  %value = load <4 x float>, ptr %source, align 16
  ret <4 x float> %value
}

; CHECK-LABEL: vector_call_result:
; CHECK: bl vector_callee
; CHECK: stvx 1,
; CHECK: blr
define void @vector_call_result(ptr %destination) {
  %value = call <4 x float> @vector_callee()
  store <4 x float> %value, ptr %destination, align 16
  ret void
}

; CHECK-LABEL: aggregate_return:
; CHECK: bl fill
; CHECK: {{(mr|clrldi)}} 3,
; CHECK: blr
; MIR0-LABEL: name: aggregate_return
; MIR0: %[[IN:[0-9]+]]:g8rc = COPY $x3
; MIR0: %[[PTR:[0-9]+]]:gprc{{[^ ]*}} = COPY %[[IN]].sub_32
; MIR0: %[[SAVED:[0-9]+]]:gprc = COPY %[[PTR]]
; MIR0: BL8_NOP @fill
; MIR0: %[[RET:[0-9]+]]:g8rc = RLDICL_32_64 %[[SAVED]], 0, 32
; MIR0-NEXT: $x3 = COPY %[[RET]]
; MIR0-NEXT: BLR8 implicit $lr8, implicit $rm, implicit $x3
; MIR2-LABEL: name: aggregate_return
; MIR2: %[[IN:[0-9]+]]:g8rc = COPY $x3
; MIR2: %[[PTR:[0-9]+]]:gprc{{[^ ]*}} = COPY %[[IN]].sub_32
; MIR2: BL @fill
; MIR2: %[[WIDE:[0-9]+]]:g8rc = INSERT_SUBREG %{{[0-9]+}}, %[[PTR]], %subreg.sub_32
; MIR2-NEXT: %[[RET:[0-9]+]]:g8rc = RLDICL killed %[[WIDE]], 0, 32
; MIR2-NEXT: $x3 = COPY %[[RET]]
; MIR2-NEXT: BLR8 implicit $lr8, implicit $rm, implicit $x3
define void @aggregate_return(ptr sret(%Result) %destination) {
  %second = getelementptr %Result, ptr %destination, i32 0, i32 1
  call void @fill(ptr %second)
  ret void
}

; The hidden pointer must survive calls on both return paths.
; CHECK-LABEL: aggregate_return_paths:
; CHECK: bl fill
; CHECK: {{(mr|clrldi)}} 3,
; CHECK: blr
; MIR0-LABEL: name: aggregate_return_paths
; MIR0: %[[IN:[0-9]+]]:g8rc = COPY $x3
; MIR0: %[[PTR:[0-9]+]]:gprc{{[^ ]*}} = COPY %[[IN]].sub_32
; MIR0: %[[SAVED:[0-9]+]]:gprc = COPY %[[PTR]]
; MIR0: BL8_NOP @fill
; MIR0: %[[RET1:[0-9]+]]:g8rc = RLDICL_32_64 %[[SAVED]], 0, 32
; MIR0-NEXT: $x3 = COPY %[[RET1]]
; MIR0-NEXT: BLR8 implicit $lr8, implicit $rm, implicit $x3
; MIR0: BL8_NOP @fill
; MIR0: %[[RET2:[0-9]+]]:g8rc = RLDICL_32_64 %[[SAVED]], 0, 32
; MIR0-NEXT: $x3 = COPY %[[RET2]]
; MIR0-NEXT: BLR8 implicit $lr8, implicit $rm, implicit $x3
define void @aggregate_return_paths(ptr sret(%Result) %destination, i1 %condition) {
  br i1 %condition, label %left, label %right
left:
  call void @fill(ptr %destination)
  ret void
right:
  %second = getelementptr %Result, ptr %destination, i32 0, i32 1
  call void @fill(ptr %second)
  ret void
}

; CHECK-LABEL: aggregate_return_leaf:
; CHECK-NOT: mflr
; CHECK: stb
; CHECK: blr
define void @aggregate_return_leaf(ptr sret(%Result) %destination) {
  %last = getelementptr i8, ptr %destination, i32 15
  store i8 42, ptr %last
  ret void
}

; The hidden pointer need not be the first IR argument (for example, after this
; in a member function). Return the destination, not the value passed in R3.
; CHECK-LABEL: aggregate_return_after_this:
; CHECK: bl fill
; CHECK: {{(mr|clrldi)}} 3,
; CHECK: blr
; MIR0-LABEL: name: aggregate_return_after_this
; MIR0: %[[IN:[0-9]+]]:g8rc = COPY $x4
; MIR0: %[[PTR:[0-9]+]]:gprc{{[^ ]*}} = COPY %[[IN]].sub_32
; MIR0: %[[SAVED:[0-9]+]]:gprc = COPY %[[PTR]]
; MIR0: BL8_NOP @fill
; MIR0: %[[RET:[0-9]+]]:g8rc = RLDICL_32_64 %[[SAVED]], 0, 32
; MIR0-NEXT: $x3 = COPY %[[RET]]
; MIR0-NEXT: BLR8 implicit $lr8, implicit $rm, implicit $x3
define void @aggregate_return_after_this(ptr %this, ptr sret(%Result) %destination) {
  store ptr %this, ptr %destination
  %second = getelementptr %Result, ptr %destination, i32 0, i32 1
  call void @fill(ptr %second)
  ret void
}

; SelectionDAG can introduce the hidden pointer when the IR return value does
; not fit in the return registers; it must be returned in R3 too.
; CHECK-LABEL: implicit_aggregate_return:
; CHECK: clrldi 3, 3, 32
; CHECK: blr
define [8 x i64] @implicit_aggregate_return() {
  ret [8 x i64] zeroinitializer
}

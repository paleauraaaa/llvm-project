; RUN: llc -verify-machineinstrs -mtriple=ppc64-ibm-xbox360 -ppc-min-jump-table-entries=4 %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -verify-machineinstrs -mtriple=ppc64-ibm-xbox360 -ppc-min-jump-table-entries=4 -stop-after=machine-cp %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -verify-machineinstrs -mtriple=ppc64-ibm-xbox360 -ppc-min-jump-table-entries=4 %s -filetype=obj -o %t.obj
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=COFF

; COFF: Section {{.*}} .rdata
; COFF-COUNT-8: IMAGE_REL_PPC_ADDR32 .text

target triple = "ppc64-ibm-xbox360"

; Stack arguments begin after the 16-byte linkage area, not at offset 24.
; ASM-LABEL: ninth_integer:
; ASM: ld 3, 80(1)
; ASM: blr
; MIR-LABEL: name: ninth_integer
; MIR: offset: 80, size: 8
define i64 @ninth_integer(i64 %a, i64 %b, i64 %c, i64 %d,
                         i64 %e, i64 %f, i64 %g, i64 %h, i64 %i) {
  ret i64 %i
}

; Sub-doubleword scalars are right-justified in the big-endian stack image.
; ASM-LABEL: ninth_word:
; ASM: lwz 3, 84(1)
; ASM: blr
; MIR-LABEL: name: ninth_word
; MIR: offset: 84, size: 4
define i32 @ninth_word(i64 %a, i64 %b, i64 %c, i64 %d,
                      i64 %e, i64 %f, i64 %g, i64 %h, i32 %i) {
  ret i32 %i
}

; Vector slots are aligned relative to the same parameter image.
; ASM-LABEL: fourteenth_vector:
; ASM: addi [[VADDR:[0-9]+]], 1, 224
; ASM: lvx 1, 0, [[VADDR]]
; ASM: blr
; MIR-LABEL: name: fourteenth_vector
; MIR: offset: 224, size: 16
define <4 x i32> @fourteenth_vector(
    <4 x i32> %a, <4 x i32> %b, <4 x i32> %c, <4 x i32> %d,
    <4 x i32> %e, <4 x i32> %f, <4 x i32> %g, <4 x i32> %h,
    <4 x i32> %i, <4 x i32> %j, <4 x i32> %k, <4 x i32> %l,
    <4 x i32> %m, <4 x i32> %n) {
  ret <4 x i32> %n
}

%S3 = type <{ i8, i8, i8 }>
%S4 = type <{ i8, i8, i8, i8 }>
%S9 = type <{ i64, i8 }>

; Odd-sized aggregates are left-justified, including their register chunks.
; ASM-LABEL: first_s3:
; ASM: std 3, 16(1)
; ASM: addi 3, 1, 16
; ASM: lbz 3, 0(3)
; ASM: blr
; MIR-LABEL: name: first_s3
define i8 @first_s3(ptr byval(%S3) align 1 %p) {
  %v = load volatile i8, ptr %p, align 1
  ret i8 %v
}

; ASM-LABEL: first_s4:
; ASM: stw 3, 20(1)
; ASM: addi 3, 1, 20
; ASM: lbz 3, 0(3)
; ASM: blr
; MIR-LABEL: name: first_s4
define i8 @first_s4(ptr byval(%S4) align 1 %p) {
  %v = load volatile i8, ptr %p, align 1
  ret i8 %v
}

; ASM-LABEL: ninth_s3:
; ASM: addi 3, 1, 80
; ASM: lbz 3, 0(3)
; ASM: blr
; MIR-LABEL: name: ninth_s3
define i8 @ninth_s3(i64 %a, i64 %b, i64 %c, i64 %d,
                    i64 %e, i64 %f, i64 %g, i64 %h,
                    ptr byval(%S3) align 1 %p) {
  %v = load volatile i8, ptr %p, align 1
  ret i8 %v
}

; ASM-LABEL: last_byte_s9:
; ASM: {{srdi|rldicl}} {{[0-9]+}}, 4, {{(56|8, 56)}}
; ASM: stb {{[0-9]+}}, 24(1)
; ASM: addi 3, 1, 16
; ASM: lbz 3, 8(3)
; ASM: blr
; MIR-LABEL: name: last_byte_s9
define i8 @last_byte_s9(ptr byval(%S9) align 1 %p) {
  %last = getelementptr %S9, ptr %p, i32 0, i32 1
  %v = load volatile i8, ptr %last, align 1
  ret i8 %v
}

@s9 = global %S9 <{ i64 1, i8 2 }>, align 1
declare void @take_s9(ptr byval(%S9) align 1)

; ASM-LABEL: call_s9:
; ASM: {{sldi|rldicr}} 4, {{[0-9]+}}, 56{{(, 7)?}}
; ASM: bl take_s9
; MIR-LABEL: name: call_s9
define void @call_s9() {
  call void @take_s9(ptr byval(%S9) align 1 @s9)
  ret void
}

; A variadic function cannot receive its ninth doubleword in an FPR or VR.
; ASM-LABEL: ninth_vararg_double:
; ASM: lfd 1, 80(1)
; ASM: blr
; MIR-LABEL: name: ninth_vararg_double
; MIR: liveins: []
; MIR: offset: 80, size: 8
define double @ninth_vararg_double(i64 %a, i64 %b, i64 %c, i64 %d,
                                   i64 %e, i64 %f, i64 %g, i64 %h,
                                   double %i, ...) {
  ret double %i
}

; ASM-LABEL: ninth_vararg_vector:
; ASM: addi [[VADDR2:[0-9]+]], 1, 80
; ASM: lvx 1, 0, [[VADDR2]]
; ASM: blr
; MIR-LABEL: name: ninth_vararg_vector
; MIR: liveins: []
; MIR: offset: 80, size: 16
define <4 x i32> @ninth_vararg_vector(i64 %a, i64 %b, i64 %c, i64 %d,
                                     i64 %e, i64 %f, i64 %g, i64 %h,
                                     <4 x i32> %i, ...) {
  ret <4 x i32> %i
}

declare void @llvm.va_start(ptr)
declare void @llvm.va_end(ptr)

; va_start must address the byte immediately after the fixed argument slot.
; ASM-LABEL: vararg_start:
; ASM: addi {{[0-9]+}}, 1, 24
; ASM: std 4, 24(1)
; ASM: blr
; MIR-LABEL: name: vararg_start
; MIR: offset: 24, size: 8
define ptr @vararg_start(i64 %fixed, ...) {
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %p = load ptr, ptr %ap, align 4
  call void @llvm.va_end(ptr %ap)
  ret ptr %p
}

declare ptr @llvm.returnaddress(i32 immarg)
declare ptr @llvm.stacksave.p0()
declare void @llvm.stackrestore.p0(ptr)

; The saved LR is a 32-bit word at incoming SP - 8, with separate padding.
; ASM-LABEL: return_address:
; ASM: mflr 12
; ASM: stw 12, -8(1)
; ASM: lwz 3, {{[0-9]+}}(1)
; ASM: lwz 12, -8(1)
; ASM: blr
; MIR-LABEL: name: return_address
; MIR: offset: -8, size: 8
define ptr @return_address() {
  %p = call ptr @llvm.returnaddress(i32 0)
  ret ptr %p
}

; Even an alloca-only leaf reserves 16 bytes of linkage and 64 of parameters.
; ASM-LABEL: dynamic_leaf:
; ASM: extsw
; ASM: stwux
; ASM: addi 3, 1, 80
; ASM: lwz 1, 0(1)
; ASM: blr
; MIR-LABEL: name: dynamic_leaf
define ptr @dynamic_leaf(i32 %n) {
  %p = alloca i8, i32 %n, align 16
  store volatile i8 1, ptr %p, align 1
  ret ptr %p
}

declare void @callee(ptr)

; Save LR before materializing a large stack displacement in the same scratch.
; ASM-LABEL: large_nonleaf:
; ASM: mflr 12
; ASM-NEXT: stw 12, -8(1)
; ASM: stwux 1,
; ASM: bl callee
; ASM: lwz 1, 0(1)
; ASM: lwz 12, -8(1)
; ASM: mtlr 12
; ASM: blr
; MIR-LABEL: name: large_nonleaf
define void @large_nonleaf() {
  %p = alloca [65536 x i8], align 16
  call void @callee(ptr %p)
  ret void
}

; The base pointer and frame pointer must preserve full-width GPRs in
; different slots, even though stack pointers have a 32-bit ABI type.
; ASM-LABEL: aligned_dynamic:
; ASM-DAG: std 31, -16(1)
; ASM-DAG: std 30, -24(1)
; ASM: stwux 1,
; ASM: bl callee
; ASM-DAG: ld 31, -16(1)
; ASM-DAG: ld 30, -24(1)
; ASM: blr
; MIR-LABEL: name: aligned_dynamic
define void @aligned_dynamic(i32 %n) {
  %fixed = alloca [32 x i8], align 32
  %p = alloca i8, i32 %n, align 16
  store volatile i8 1, ptr %p, align 1
  call void @callee(ptr %fixed)
  ret void
}

; Large frames must not turn prologue saves into stdx/stfdx through a
; materialized frame address. Save nonvolatiles relative to incoming R1.
; ASM-LABEL: large_nonvolatile:
; ASM: stw 12, -8(1)
; ASM-DAG: std 14, -152(1)
; ASM-DAG: stfd 14, -296(1)
; ASM-DAG: stvx128 127, 1, {{[0-9]+}}
; ASM: stwux 1,
; ASM: bl callee
; ASM: lwz 1, 0(1)
; ASM-DAG: ld 14, -152(1)
; ASM-DAG: lfd 14, -296(1)
; ASM-DAG: lvx128 127, 1, {{[0-9]+}}
; ASM: blr
; MIR-LABEL: name: large_nonvolatile
define void @large_nonvolatile() {
  %p = alloca [65536 x i8], align 16
  call void asm sideeffect "", "~{r14},~{f14},~{v127}"()
  call void @callee(ptr %p)
  ret void
}

; Save nonvolatile state at function entry, not in a shrink-wrapped block.
; ASM-LABEL: conditional_call:
; ASM: mflr 12
; ASM: stw 12, -8(1)
; ASM: stwu 1,
; ASM: cmp
; ASM: bl callee
; MIR-LABEL: name: conditional_call
define void @conditional_call(i32 %condition, ptr %p) {
  %test = icmp eq i32 %condition, 0
  br i1 %test, label %call, label %exit
call:
  call void @callee(ptr %p)
  br label %exit
exit:
  ret void
}

; All 64 nonvolatile vector registers fit in distinct 16-byte save slots
; inside the 1344-byte red zone.
; ASM-LABEL: all_nonvolatile_vectors:
; ASM-NOT: stwu
; ASM-COUNT-64: stvx128 {{(6[4-9]|[7-9][0-9]|1[01][0-9]|12[0-7])}}, 1,
; ASM-NOT: stvx128
; ASM: #APP
; ASM-COUNT-64: lvx128 {{(6[4-9]|[7-9][0-9]|1[01][0-9]|12[0-7])}}, 1,
; ASM-NOT: lvx128
; ASM: blr
; MIR-LABEL: name: all_nonvolatile_vectors
define void @all_nonvolatile_vectors() {
  call void asm sideeffect "", "~{v64},~{v65},~{v66},~{v67},~{v68},~{v69},~{v70},~{v71},~{v72},~{v73},~{v74},~{v75},~{v76},~{v77},~{v78},~{v79},~{v80},~{v81},~{v82},~{v83},~{v84},~{v85},~{v86},~{v87},~{v88},~{v89},~{v90},~{v91},~{v92},~{v93},~{v94},~{v95},~{v96},~{v97},~{v98},~{v99},~{v100},~{v101},~{v102},~{v103},~{v104},~{v105},~{v106},~{v107},~{v108},~{v109},~{v110},~{v111},~{v112},~{v113},~{v114},~{v115},~{v116},~{v117},~{v118},~{v119},~{v120},~{v121},~{v122},~{v123},~{v124},~{v125},~{v126},~{v127}"()
  ret void
}

; Restoring a scoped allocation retains the original 32-bit backchain.
; ASM-LABEL: scoped_alloca:
; ASM: stwux
; ASM: bl callee
; ASM: lwz [[LINK:[0-9]+]], 0(1)
; ASM: mr 1,
; ASM: stw [[LINK]], 0(1)
; ASM: bl callee
; ASM: lwz 12, -8(1)
; ASM: blr
; MIR-LABEL: name: scoped_alloca
define void @scoped_alloca(i32 %n) {
  %saved = call ptr @llvm.stacksave.p0()
  %p = alloca i8, i32 %n, align 16
  call void @callee(ptr %p)
  call void @llvm.stackrestore.p0(ptr %saved)
  call void @callee(ptr null)
  ret void
}

; Internal fastcc calls still need the non-leaf frame's minimum home area.
; ASM-LABEL: internal_fast_call:
; ASM: stw 12, -8(1)
; ASM: stwu 1, -96(1)
; ASM: bl internal_fast_target
; ASM: addi 1, 1, 96
; ASM: lwz 12, -8(1)
; ASM: blr
; MIR-LABEL: name: internal_fast_call
; MIR: ADJCALLSTACKDOWN 80
define void @internal_fast_call(i64 %value) {
  call fastcc void @internal_fast_target(i64 %value)
  ret void
}

declare fastcc void @internal_fast_target(i64)

; PPC COFF has no relocation for a 32-bit cross-section label difference.
; Jump tables therefore contain absolute 32-bit code addresses.
; ASM-LABEL: jump_table_dispatch:
; ASM: mtctr
; ASM: bctr
; MIR-LABEL: name: jump_table_dispatch
; MIR: kind: block-address
define void @jump_table_dispatch(i32 %n) {
  switch i32 %n, label %exit [
    i32 0, label %case0
    i32 1, label %case1
    i32 2, label %case2
    i32 3, label %case3
    i32 4, label %case4
    i32 5, label %case5
    i32 6, label %case6
    i32 7, label %case7
  ]
case0:
  call void @case_0()
  br label %exit
case1:
  call void @case_1()
  br label %exit
case2:
  call void @case_2()
  br label %exit
case3:
  call void @case_3()
  br label %exit
case4:
  call void @case_4()
  br label %exit
case5:
  call void @case_5()
  br label %exit
case6:
  call void @case_6()
  br label %exit
case7:
  call void @case_7()
  br label %exit
exit:
  ret void
}

declare void @case_0()
declare void @case_1()
declare void @case_2()
declare void @case_3()
declare void @case_4()
declare void @case_5()
declare void @case_6()
declare void @case_7()

; ASM-LABEL: caller_frame_address:
; ASM: lwz {{[0-9]+}}, 0({{[0-9]+}})
define ptr @caller_frame_address() {
  %p = call ptr @llvm.frameaddress.p0(i32 1)
  ret ptr %p
}

; ASM-LABEL: caller_return_address:
; ASM: lwz [[CALLER_FRAME:[0-9]+]], 0({{[0-9]+}})
; ASM-NEXT: lwz [[CALLER_SP:[0-9]+]], 0([[CALLER_FRAME]])
; ASM-NEXT: lwz {{[0-9]+}}, -8([[CALLER_SP]])
define ptr @caller_return_address() {
  %p = call ptr @llvm.returnaddress(i32 1)
  ret ptr %p
}

declare ptr @llvm.frameaddress.p0(i32 immarg)

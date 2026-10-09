; RUN: llc -mtriple=ppc64-ibm-xbox360 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s
; RUN: llc -mtriple=ppc64-ibm-xbox360 -verify-machineinstrs -filetype=obj %s -o %t.obj

%B4 = type { [4 x i8] }
%B8 = type { [8 x i8] }
%B9 = type { [9 x i8] }
%B17 = type { [17 x i8] }
%B32 = type { [32 x i8] }

declare void @take4(i64, ptr byval(%B4) align 1, i64)
declare void @take8(i64, ptr byval(%B8) align 1, i64)
declare void @take9(i64, ptr byval(%B9) align 1, i64)
declare void @take17(i64, ptr byval(%B17) align 1, i64)
declare void @take32(ptr byval(%B32) align 32, i64)

; A small right-justified aggregate retains one eight-byte slot.
; CHECK-LABEL: name: small4
; CHECK: offset: 24, size: 8
; CHECK: liveins: $x3, $x4, $x5
; CHECK: STW8 {{.*}}, 4, {{.*}}%fixed-stack
; CHECK: BL @take4{{.*}}implicit $x3, implicit $x4, implicit $x5
define void @small4(i64 %lead, ptr byval(%B4) align 1 %p, i64 %tail) {
  call void @take4(i64 %lead, ptr byval(%B4) align 1 %p, i64 %tail)
  ret void
}

; Eight bytes is still aligned to eight, not 16.
; CHECK-LABEL: name: small8
; CHECK: offset: 24, size: 8
; CHECK: liveins: $x3, $x4, $x5
; CHECK: BL @take8{{.*}}implicit $x3, implicit $x4, implicit $x5
define void @small8(i64 %lead, ptr byval(%B8) align 1 %p, i64 %tail) {
  call void @take8(i64 %lead, ptr byval(%B8) align 1 %p, i64 %tail)
  ret void
}

; Nine bytes starts at +32, but the padding at +24 consumes no GPR.
; CHECK-LABEL: name: large9
; CHECK: offset: 32, size: 16
; CHECK: liveins: $x3, $x4, $x5, $x6
; CHECK: BL @take9{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6
define void @large9(i64 %lead, ptr byval(%B9) align 1 %p, i64 %tail) {
  call void @take9(i64 %lead, ptr byval(%B9) align 1 %p, i64 %tail)
  ret void
}

; Seventeen bytes occupies 24 bytes, not 32.
; CHECK-LABEL: name: large17
; CHECK: offset: 32, size: 24
; CHECK: liveins: $x3, $x4, $x5, $x6, $x7
; CHECK: BL @take17{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit $x7
define void @large17(i64 %lead, ptr byval(%B17) align 1 %p, i64 %tail) {
  call void @take17(i64 %lead, ptr byval(%B17) align 1 %p, i64 %tail)
  ret void
}

; Even an explicitly over-aligned type uses the 16-byte parameter boundary.
; CHECK-LABEL: name: capped_alignment
; CHECK: offset: 16, size: 32
; CHECK: liveins: $x3, $x4, $x5, $x6, $x7
; CHECK: BL @take32{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit $x7
define void @capped_alignment(ptr byval(%B32) align 32 %p, i64 %tail) {
  call void @take32(ptr byval(%B32) align 32 %p, i64 %tail)
  ret void
}

declare void @split(i64, i64, i64, i64, i64, i64, i64,
                    ptr byval(%B9) align 1, i64)

; After seven integers the aggregate starts at +80, with its first chunk still
; in R10. Its ninth byte is at +88 and the trailing integer is at +96.
; CHECK-LABEL: name: split9
; CHECK-DAG: offset: 96, size: 8
; CHECK-DAG: offset: 80, size: 16
; CHECK: liveins: $x3, $x4, $x5, $x6, $x7, $x8, $x9, $x10
; CHECK: ADJCALLSTACKDOWN 104
; CHECK: STD {{.*}}, 96,
; CHECK: BL @split{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit $x7, implicit $x8, implicit $x9, implicit $x10
define void @split9(i64 %a, i64 %b, i64 %c, i64 %d, i64 %e, i64 %f, i64 %g,
                    ptr byval(%B9) align 1 %p, i64 %tail) {
  call void @split(i64 %a, i64 %b, i64 %c, i64 %d, i64 %e, i64 %f, i64 %g,
                   ptr byval(%B9) align 1 %p, i64 %tail)
  ret void
}

; A stack-only 17-byte aggregate is followed by the integer at +104.
; CHECK-LABEL: name: stack17
; CHECK-DAG: offset: 104, size: 8
; CHECK-DAG: offset: 80, size: 24
define i64 @stack17(i64 %a, i64 %b, i64 %c, i64 %d, i64 %e, i64 %f, i64 %g,
                    i64 %h, ptr byval(%B17) align 1 %p, i64 %tail) {
  %v = load volatile i64, ptr %p, align 1
  %sum = add i64 %v, %tail
  ret i64 %sum
}

; Do not change the existing variadic ABI without a complete varargs oracle.
declare void @vararg(i64, ...)
; CHECK-LABEL: name: variadic9
; CHECK: offset: 24, size: 16
; CHECK: BL @vararg{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6
define void @variadic9(i64 %lead, ptr byval(%B9) align 1 %p, ...) {
  call void (i64, ...) @vararg(i64 %lead, ptr byval(%B9) align 1 %p, i64 42)
  ret void
}

%B3 = type { [3 x i8] }
declare void @original(i64, ptr byval(%B3) align 1, ptr byval(%B4) align 1,
                       ptr byval(%B9) align 1, ptr byval(%B32) align 8, i64)

; Reproduce MSVC's original stress-test layout: aggregate homes at +24, +32
; (payload +36), +48 and +64, followed by the stack integer at +96.
; CHECK-LABEL: name: original_sequence
; CHECK-DAG: offset: 96, size: 8
; CHECK-DAG: offset: 64, size: 32
; CHECK-DAG: offset: 48, size: 16
; CHECK-DAG: offset: 32, size: 8
; CHECK-DAG: offset: 24, size: 8
; CHECK: liveins: $x3, $x4, $x5, $x6, $x7, $x8, $x9, $x10
; CHECK: ADJCALLSTACKDOWN 104
; CHECK: STD {{.*}}, 96,
; CHECK: BL @original{{.*}}implicit $x3, implicit $x4, implicit $x5, implicit $x6, implicit $x7, implicit $x8, implicit $x9, implicit $x10
define void @original_sequence(i64 %lead, ptr byval(%B3) align 1 %a,
                               ptr byval(%B4) align 1 %b,
                               ptr byval(%B9) align 1 %c,
                               ptr byval(%B32) align 8 %d, i64 %tail) {
  call void @original(i64 %lead, ptr byval(%B3) align 1 %a,
                       ptr byval(%B4) align 1 %b, ptr byval(%B9) align 1 %c,
                       ptr byval(%B32) align 8 %d, i64 %tail)
  ret void
}

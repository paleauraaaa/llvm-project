; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj < %s -o %t
; RUN: llvm-readobj --sections %t | FileCheck %s --check-prefix=OBJ

; Explicit init_seg tables are constant globals rather than global_ctors.
; They must still merge with the writable XDK CRT tables.
@init = private constant ptr @ctor, section ".CRT$XCM", align 4
@term = private constant ptr @dtor, section ".CRT$XTM", align 4
@user_init = private constant ptr @ctor, section ".CRT$XCU", align 4
@unrelated = private constant ptr @ctor, section ".custom", align 4

@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [
  { i32, ptr, ptr } { i32 65535, ptr @ctor, ptr null }
]

declare void @ctor()
declare void @dtor()

; ASM: .section .CRT$XCM,"dw"
; ASM: .long ctor
; ASM: .section .CRT$XTM,"dw"
; ASM: .long dtor
; ASM: .section .CRT$XCU,"dw"
; ASM: .long ctor
; ASM: .section .custom,"dr"
; ASM: .long ctor
; ASM: .section .CRT$XCU,"dw"
; ASM: .long ctor

; OBJ: Name: .CRT$XCM
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XTM
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XCU
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .custom
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0x40300040)

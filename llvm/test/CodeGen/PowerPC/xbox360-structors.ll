; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj < %s -o %t
; RUN: llvm-readobj --sections %t | FileCheck %s --check-prefix=OBJ

; Both default and priority-specific CRT contributions must be writable to
; merge with the XDK CRT's sentinel sections, including associative COMDATs.
; ASM: .section .CRT$XCA00042,"dw"
; ASM: .long early
; ASM: .section .CRT$XCC,"dw"
; ASM: .long compiler
; ASM: .section .CRT$XCC00250,"dw"
; ASM: .long middle
; ASM: .section .CRT$XCL,"dw"
; ASM: .long library
; ASM: .section .CRT$XCT23456,"dw",associative,key
; ASM: .long associated
; ASM: .section .CRT$XCU,"dw"
; ASM: .long normal
; ASM: .section .CRT$XCU,"dw",associative,key
; ASM: .long associated
; ASM: .section .CRT$XTA00042,"dw"
; ASM: .long early
; ASM: .section .CRT$XTT23456,"dw",associative,key
; ASM: .long associated
; ASM: .section .CRT$XTX,"dw"
; ASM: .long normal
; ASM: .section .CRT$XTX,"dw",associative,key
; ASM: .long associated

; OBJ: Name: .CRT$XCA00042
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XCC
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XCC00250
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XCL
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XCU
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XTA00042
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XTX
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0300040)
; OBJ: Name: .CRT$XCT23456
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0301040)
; OBJ: Name: .CRT$XCU
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0301040)
; OBJ: Name: .CRT$XTT23456
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0301040)
; OBJ: Name: .CRT$XTX
; OBJ-NOT: Name:
; OBJ: Characteristics [ (0xC0301040)

$key = comdat any
@key = linkonce_odr global i32 0, comdat, align 4

@llvm.global_ctors = appending global [7 x { i32, ptr, ptr }] [
  { i32, ptr, ptr } { i32 42, ptr @early, ptr null },
  { i32, ptr, ptr } { i32 200, ptr @compiler, ptr null },
  { i32, ptr, ptr } { i32 250, ptr @middle, ptr null },
  { i32, ptr, ptr } { i32 400, ptr @library, ptr null },
  { i32, ptr, ptr } { i32 23456, ptr @associated, ptr @key },
  { i32, ptr, ptr } { i32 65535, ptr @normal, ptr null },
  { i32, ptr, ptr } { i32 65535, ptr @associated, ptr @key }
]

@llvm.global_dtors = appending global [4 x { i32, ptr, ptr }] [
  { i32, ptr, ptr } { i32 42, ptr @early, ptr null },
  { i32, ptr, ptr } { i32 23456, ptr @associated, ptr @key },
  { i32, ptr, ptr } { i32 65535, ptr @normal, ptr null },
  { i32, ptr, ptr } { i32 65535, ptr @associated, ptr @key }
]

declare void @early()
declare void @compiler()
declare void @middle()
declare void @library()
declare void @associated()
declare void @normal()

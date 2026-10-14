; RUN: split-file %s %t
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/data.ll -o %t/data.obj
; RUN: llvm-readobj --sections --section-data --relocations %t/data.obj | FileCheck %s --check-prefix=DATA
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/zero.ll -o %t/zero.obj
; RUN: llvm-readobj --sections --section-data %t/zero.obj | FileCheck %s --check-prefix=ZERO
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/aligned.ll -o %t/aligned.obj
; RUN: llvm-readobj --sections --section-data %t/aligned.obj | FileCheck %s --check-prefix=ALIGNED
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/large.ll -o %t/large.obj
; RUN: llvm-readobj --sections --section-data %t/large.obj | FileCheck %s --check-prefix=LARGE

; XDK TLS is writable initialized data, including zero-initialized variables.
; Initializers and address relocations are big-endian, not CodeView byte order.
; The CRT, not these modules, owns .tls/.tls$ZZZ and the TLS directory.

; DATA: Name: .tls$
; DATA: RawDataSize: 20
; DATA: IMAGE_SCN_ALIGN_8BYTES
; DATA: IMAGE_SCN_CNT_INITIALIZED_DATA
; DATA-NOT: IMAGE_SCN_CNT_UNINITIALIZED_DATA
; DATA: IMAGE_SCN_MEM_READ
; DATA: IMAGE_SCN_MEM_WRITE
; DATA: SectionData (
; DATA-NEXT: 0000: 00000000 00000007 3FF40000 00000000
; DATA-NEXT: 0010: 00000000
; DATA: {{.*}}.tls$ {
; DATA-NEXT: 0x10 IMAGE_REL_PPC_ADDR32 ordinary_global

; ZERO: Name: .tls$
; ZERO: RawDataSize: 4100
; ZERO: IMAGE_SCN_CNT_INITIALIZED_DATA
; ZERO-NOT: IMAGE_SCN_CNT_UNINITIALIZED_DATA
; ZERO: IMAGE_SCN_MEM_READ
; ZERO: IMAGE_SCN_MEM_WRITE
; ZERO: SectionData (
; ZERO-NEXT: 0000: 00000000 00000000 00000000 00000000
; ZERO: 1000: 00000000

; ALIGNED: Name: .tls$
; ALIGNED: RawDataSize: 128
; ALIGNED: IMAGE_SCN_ALIGN_128BYTES
; ALIGNED: IMAGE_SCN_CNT_INITIALIZED_DATA
; ALIGNED: IMAGE_SCN_MEM_READ
; ALIGNED: IMAGE_SCN_MEM_WRITE
; ALIGNED: SectionData (
; ALIGNED-NEXT: 0000: 01000000 00000000 00000000 00000000

; LARGE: Name: .tls$
; LARGE: RawDataSize: 70000
; LARGE: IMAGE_SCN_CNT_INITIALIZED_DATA
; LARGE: IMAGE_SCN_MEM_READ
; LARGE: IMAGE_SCN_MEM_WRITE
; LARGE: SectionData (
; LARGE-NEXT: 0000: 01000000 00000000 00000000 00000000
; LARGE: 11160: 00000000 00000000 00000000 00000000

;--- data.ll
target datalayout = "E-m:w-S128-p:32:32-Fi32-i64:64-i128:128-n32:64-v128:128"
@tls_zero = thread_local global i32 0, align 4
@tls_initialized = thread_local global i32 7, align 4
@tls_double = thread_local global double 1.25, align 8
@tls_pointer = thread_local global ptr @ordinary_global, align 4
@ordinary_global = global i32 19, align 4

;--- zero.ll
@tls_zero_large = thread_local global [4096 x i8] zeroinitializer, align 16
@tls_zero_only = thread_local global i32 0, align 4

;--- aligned.ll
@tls_align128 = thread_local global [128 x i8] c"\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00", align 128

;--- large.ll
@tls_large = thread_local global <{ i8, [69999 x i8] }> <{ i8 1, [69999 x i8] zeroinitializer }>, align 16

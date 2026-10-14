; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=false -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM --implicit-check-not=__tls_get_addr --implicit-check-not=__emutls
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM --implicit-check-not=__tls_get_addr --implicit-check-not=__emutls
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs -filetype=obj %s -o %t.obj
; RUN: llvm-readobj --relocations --symbols %t.obj | FileCheck %s --check-prefix=OBJ --implicit-check-not=IMAGE_REL_PPC_SECRELHI --implicit-check-not=IMAGE_REL_PPC_SECRELLO
; RUN: llvm-objdump -dr --mcpu=xenon %t.obj | FileCheck %s --check-prefix=DIS
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=true -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 %s -o %t.s
; RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t.s -o %t.asm.obj
; RUN: llvm-readobj --relocations --symbols %t.asm.obj | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -stop-after=finalize-isel %s -o %t.mir
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -start-after=finalize-isel -verify-machineinstrs -filetype=obj %t.mir -o %t.mir.obj
; RUN: llvm-readobj --relocations --symbols %t.mir.obj | FileCheck %s --check-prefix=OBJ

; XDK cl 16.00.11886 uses a 32-bit block pointer at 0(r13) and signed
; SECREL16 variable offsets, including for references defined in another TU.
; A large array is legal if its base fits: element offsets are added separately.
; XDK linking accepts a symbol at 0x7ff8, but rejects one at 0x8000.
; Do not require its optional CSE of the block pointer or retention across calls.

target datalayout = "E-m:w-S128-p:32:32-Fi32-i64:64-i128:128-n32:64-v128:128"

@tls_zero = thread_local global i32 0, align 4
@tls_initialized = external thread_local global i32
@tls_double = external thread_local global double
@tls_pointer = external thread_local global ptr
@tls_static = internal thread_local global i32 11, align 4
@tls_large = external thread_local global [70000 x i8]

; ASM-LABEL: tls_address:
; ASM-DAG: lwz {{[0-9]+}}, 0(13)
; ASM-DAG: li {{[0-9]+}}, {{.*}}tls_zero
; ASM: add {{[0-9]+}},
; ASM: blr
; DIS-LABEL: <tls_address>:
; DIS: li {{[0-9]+}}, 0
; DIS-NEXT: {{.*}}IMAGE_REL_PPC_SECREL16 tls_zero
define ptr @tls_address() {
  ret ptr @tls_zero
}

; ASM-LABEL: tls_read:
; ASM: lwz {{[0-9]+}}, 0(13)
; ASM: {{lwz|lwzx}} 3,
; ASM: blr
define i32 @tls_read() {
  %value = load i32, ptr @tls_initialized, align 4
  ret i32 %value
}

; ASM-LABEL: tls_write:
; ASM: lwz {{[0-9]+}}, 0(13)
; ASM: {{stw|stwx}} {{[0-9]+}},
; ASM: blr
define void @tls_write(i32 %value) {
  store i32 %value, ptr @tls_zero, align 4
  ret void
}

; ASM-LABEL: tls_read_double:
; ASM: lwz {{[0-9]+}}, 0(13)
; ASM: {{lfd|lfdx}} 1,
; ASM: blr
define double @tls_read_double() {
  %value = load double, ptr @tls_double, align 8
  ret double %value
}

; ASM-LABEL: tls_read_pointer:
; ASM: lwz {{[0-9]+}}, 0(13)
; ASM: {{lwz|lwzx}} 3,
; ASM: blr
define ptr @tls_read_pointer() {
  %value = load ptr, ptr @tls_pointer, align 4
  ret ptr %value
}

; ASM-LABEL: tls_static_address:
; ASM: lwz {{[0-9]+}}, 0(13)
; ASM: add {{[0-9]+}},
; ASM: blr
define ptr @tls_static_address() {
  ret ptr @tls_static
}

; DIS-LABEL: <tls_large_tail>:
; DIS: li {{[0-9]+}}, 0
; DIS-NEXT: {{.*}}IMAGE_REL_PPC_SECREL16 tls_large
; DIS: {{lbz|lbzx}}
define i8 @tls_large_tail() {
  %value = load i8, ptr getelementptr ([70000 x i8], ptr @tls_large,
                                      i32 0, i32 69999), align 1
  ret i8 %value
}

; DIS-LABEL: <tls_negative_offset>:
; DIS: li {{[0-9]+}}, 0
; DIS-NEXT: {{.*}}IMAGE_REL_PPC_SECREL16 tls_large
; DIS: addi {{[0-9]+}}, {{[0-9]+}}, -4
define ptr @tls_negative_offset() {
  ret ptr getelementptr (i8, ptr @tls_large, i32 -4)
}

declare void @opaque_call()
; DIS-LABEL: <tls_across_call>:
; DIS: lwz {{[0-9]+}}, 0(13)
; DIS: {{lwz|lwzx}}
; DIS: bl
; DIS-NEXT: {{.*}}IMAGE_REL_PPC_REL24 opaque_call
; DIS: {{lwz|lwzx}}
; DIS: add 3,
define i32 @tls_across_call() {
  %before = load i32, ptr @tls_zero, align 4
  call void @opaque_call()
  %after = load i32, ptr @tls_zero, align 4
  %sum = add i32 %before, %after
  ret i32 %sum
}

; OBJ-DAG: IMAGE_REL_PPC_SECREL16 tls_zero
; OBJ-DAG: IMAGE_REL_PPC_SECREL16 tls_initialized
; OBJ-DAG: IMAGE_REL_PPC_SECREL16 tls_double
; OBJ-DAG: IMAGE_REL_PPC_SECREL16 tls_pointer
; OBJ-DAG: IMAGE_REL_PPC_SECREL16 tls_static
; OBJ-DAG: IMAGE_REL_PPC_SECREL16 tls_large
; The unused external _tls_index symbol pulls in the XDK CRT TLS support.
; OBJ: Name: _tls_index
; OBJ-NEXT: Value: 0
; OBJ-NEXT: Section: IMAGE_SYM_UNDEFINED

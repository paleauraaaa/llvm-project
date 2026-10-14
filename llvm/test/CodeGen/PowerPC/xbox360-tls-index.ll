; RUN: split-file %s %t
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/definition.ll -o %t/definition.obj
; RUN: llvm-readobj --symbols %t/definition.obj | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/reference.ll -o %t/reference.obj
; RUN: llvm-readobj --symbols %t/reference.obj | FileCheck %s --check-prefix=NATIVE
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t/ordinary.ll -o %t/ordinary.obj
; RUN: llvm-readobj --symbols %t/ordinary.obj | FileCheck %s --check-prefix=ORDINARY --implicit-check-not=_tls_index
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -emulated-tls -verify-machineinstrs %t/reference.ll -o - | FileCheck %s --check-prefix=EMUTLS --implicit-check-not=_tls_index --implicit-check-not=SECREL --implicit-check-not="0(13)"

; A definition-only object must pull in CRT TLS support, as must an external
; reference. Ordinary globals and explicitly emulated TLS must not do so.
; NATIVE: Name: _tls_index
; NATIVE-NEXT: Value: 0
; NATIVE-NEXT: Section: IMAGE_SYM_UNDEFINED (0)
; NATIVE-NEXT: BaseType: Null (0x0)
; NATIVE-NEXT: ComplexType: Null (0x0)
; NATIVE-NEXT: StorageClass: External (0x2)
; ORDINARY: Name: ordinary
; EMUTLS: __emutls_v.external
; EMUTLS: bl __emutls_get_address

;--- definition.ll
@unused = thread_local global i32 0, align 4

;--- reference.ll
@external = external thread_local global i32, align 4
define ptr @address() {
  ret ptr @external
}

;--- ordinary.ll
@ordinary = global i32 0, align 4

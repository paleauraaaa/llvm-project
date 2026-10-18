; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=false -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -fast-isel=true -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs %s -o %t.s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -fast-isel=true -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
; RUN: FileCheck %s --check-prefix=ASM < %t.s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -verify-machineinstrs -filetype=obj %s -o %t.obj
; RUN: llvm-readobj --relocations --coff-directives %t.obj | FileCheck %s --check-prefix=OBJ
; RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t.s -o %t.asm.obj
; RUN: llvm-readobj --relocations --coff-directives %t.asm.obj | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -stop-after=finalize-isel %s -o %t.mir
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O2 -start-after=finalize-isel -verify-machineinstrs -filetype=obj %t.mir -o %t.mir.obj
; RUN: llvm-readobj --relocations --coff-directives %t.mir.obj | FileCheck %s --check-prefix=OBJ

target datalayout = "E-m:w-S128-p:32:32-Fi32-i64:64-i128:128-n32:64-v128:128"

@imported_data = external dllimport global i32
@imported_array = external dllimport global [8 x i32]
@"\01?value@Library@@3HA" = external dllimport global i32
@exported_data = dllexport global i32 42
@stored_function = global ptr @imported_function
declare dllimport i32 @imported_function(i32)

; ASM-LABEL: data_address:
; ASM: lis [[HI:[0-9]+]], {{.*}}__imp_imported_data
; ASM: lwz {{[0-9]+}}, 0({{[0-9]+}})
; ASM: blr
define ptr @data_address() {
  ret ptr @imported_data
}

; ASM-LABEL: array_address:
; ASM: lis [[HI:[0-9]+]], {{.*}}__imp_imported_array
; ASM: lwz [[BASE:[0-9]+]], 0({{[0-9]+}})
; ASM: addi {{[0-9]+}}, [[BASE]], 12
; ASM: blr
define ptr @array_address() {
  ret ptr getelementptr ([8 x i32], ptr @imported_array, i32 0, i32 3)
}

; ASM-LABEL: negative_offset:
; ASM: lis [[HI:[0-9]+]], {{.*}}__imp_imported_array
; ASM: lwz [[BASE:[0-9]+]], 0({{[0-9]+}})
; ASM: addi {{[0-9]+}}, [[BASE]], -4
; ASM: blr
define ptr @negative_offset() {
  ret ptr getelementptr (i8, ptr @imported_array, i32 -4)
}

; ASM-LABEL: function_address:
; ASM: lis [[HI:[0-9]+]], {{.*}}__imp_imported_function
; ASM: lwz {{[0-9]+}}, 0({{[0-9]+}})
; ASM: blr
define ptr @function_address() {
  ret ptr @imported_function
}

; ASM-LABEL: read_import:
; ASM: lis {{[0-9]+}}, __imp_imported_data@ha
; ASM: lwz [[BASE:[0-9]+]], 0({{[0-9]+}})
; ASM: {{lwa|lwz}}{{x?}} {{[0-9]+}}, {{.*}}[[BASE]]
; ASM: blr
define i32 @read_import() {
  %value = load i32, ptr @imported_data, align 4
  ret i32 %value
}

; ASM-LABEL: write_import:
; ASM: lis {{[0-9]+}}, __imp_imported_data@ha
; ASM: lwz [[BASE:[0-9]+]], 0({{[0-9]+}})
; ASM: stw {{[0-9]+}}, 0([[BASE]])
; ASM: blr
define void @write_import(i32 %value) {
  store i32 %value, ptr @imported_data, align 4
  ret void
}

; ASM-LABEL: decorated_address:
; ASM: lis [[HI:[0-9]+]], {{.*}}__imp_?value@Library@@3HA
; ASM: lwz {{[0-9]+}}, 0({{[0-9]+}})
; ASM: blr
define ptr @decorated_address() {
  ret ptr @"\01?value@Library@@3HA"
}

; Direct calls and static function-pointer initializers reference the import
; thunk, as in XDK cl. Only runtime function addresses load the IAT entry.
; ASM-LABEL: exported_function:
; ASM: bl imported_function
define dllexport i32 @exported_function(i32 %value) {
  %result = call i32 @imported_function(i32 %value)
  ret i32 %result
}

; OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_imported_data
; OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_imported_data
; OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_imported_array
; OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_imported_array
; OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_imported_function
; OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_imported_function
; OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_?value@Library@@3HA
; OBJ-DAG: IMAGE_REL_PPC_REL24 imported_function
; OBJ-DAG: IMAGE_REL_PPC_ADDR32 imported_function
; OBJ: Directive(s):
; OBJ-DAG: /EXPORT:exported_function
; OBJ-DAG: /EXPORT:exported_data,DATA

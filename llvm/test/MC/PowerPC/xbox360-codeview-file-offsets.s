# RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -filetype=obj %s -o %t.obj
# RUN: llvm-readobj --codeview %t.obj | FileCheck %s

.cv_file 1 "main.c"
.cv_file 2 "header.h"
.cv_func_id 0
.cv_func_id 1

.text
.globl f
f:
  .cv_loc 0 1 10 0
  nop
  .cv_loc 0 2 20 0
  nop
  .cv_loc 0 1 30 0
  blr
end:

.globl g
g:
  .cv_loc 1 2 40 0
  blr
g_end:

.section .debug$S,"dr"
.long 4
.cv_linetable 0, f, end
.cv_filechecksums
.cv_linetable 1, g, g_end
.cv_stringtable

# CHECK: FunctionLineTable [
# CHECK: LinkageName: f
# CHECK: CodeSize: 0xC
# CHECK: Filename: main.c (0x0)
# CHECK: LineNumberStart: 10
# CHECK: Filename: header.h (0x8)
# CHECK: +0x4 [
# CHECK: LineNumberStart: 20
# CHECK: Filename: main.c (0x0)
# CHECK: +0x8 [
# CHECK: LineNumberStart: 30
# CHECK: FunctionLineTable [
# CHECK: LinkageName: g
# CHECK: CodeSize: 0x4
# CHECK: Filename: header.h (0x8)
# CHECK: LineNumberStart: 40

; RUN: not llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t 2>&1 | FileCheck %s
; CHECK: Xbox 360 function or prologue length cannot be represented in .pdata

define void @too_large() {
  call void asm sideeffect ".space 16777216", ""()
  ret void
}

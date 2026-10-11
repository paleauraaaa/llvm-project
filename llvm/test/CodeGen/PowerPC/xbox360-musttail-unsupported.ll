; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static %t/ordinary.ll -o /dev/null 2>&1 | FileCheck %s
; RUN: not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -relocation-model=static %t/stack-fixed.ll -o /dev/null 2>&1 | FileCheck %s
; CHECK: failed to perform tail call elimination on a call site marked musttail

;--- ordinary.ll
define i64 @ordinary(ptr %this, ...) {
  %target = load ptr, ptr %this
  %result = musttail call i64 (ptr, ...) %target(ptr %this, ...)
  ret i64 %result
}

;--- stack-fixed.ll
define i64 @stack_fixed(ptr %a, ptr %b, ptr %c, ptr %d, ptr %e,
                        ptr %f, ptr %g, ptr %h, ptr %i, ...) #0 {
  %target = load ptr, ptr %a
  %result = musttail call i64 (ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ...)
    %target(ptr %a, ptr %b, ptr %c, ptr %d, ptr %e,
            ptr %f, ptr %g, ptr %h, ptr %i, ...)
  ret i64 %result
}
attributes #0 = { "thunk" }

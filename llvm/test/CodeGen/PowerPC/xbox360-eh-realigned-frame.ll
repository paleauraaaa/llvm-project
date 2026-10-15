; RUN: not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon %s -o - 2>&1 | FileCheck %s
; RUN: sed 's/__CxxFrameHandler/__C_specific_handler/g' %s | not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -o - 2>&1 | FileCheck %s
; CHECK: Xbox 360 exception handling does not support stack realignment

declare i32 @__CxxFrameHandler(...)
declare void @use(ptr)
define void @realigned_frame() personality ptr @__CxxFrameHandler {
entry:
  %object = alloca i8, align 32
  invoke void @use(ptr %object) to label %return unwind label %cleanup
return:
  ret void
cleanup:
  %pad = cleanuppad within none []
  cleanupret from %pad unwind to caller
}

; RUN: not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon %s -o - 2>&1 | FileCheck %s
; CHECK: Xbox 360 handler-bearing unwind records are not supported

declare i32 @personality(...)
define void @handler() personality ptr @personality {
  ret void
}

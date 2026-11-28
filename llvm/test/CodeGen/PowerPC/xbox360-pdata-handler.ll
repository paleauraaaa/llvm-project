; RUN: not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon %s -o - 2>&1 | FileCheck %s
; RUN: sed 's/@personality/@__CxxFrameHandler3/g' %s | not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -o - 2>&1 | FileCheck %s
; RUN: sed 's/@personality/@__gxx_personality_seh0/g' %s | not --crash llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -o - 2>&1 | FileCheck %s
; CHECK: Xbox 360 handler-bearing unwind records require __CxxFrameHandler

declare i32 @personality(...)
define void @handler() #0 personality ptr @personality {
  ret void
}

attributes #0 = { "xbox360-libunwind" }

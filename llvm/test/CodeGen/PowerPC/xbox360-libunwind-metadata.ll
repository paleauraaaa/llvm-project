; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O0 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t
; RUN: llvm-readobj --sections --relocations %t | FileCheck %s --check-prefix=OBJ

declare i32 @__C_specific_handler(...)
declare i32 @__CxxFrameHandler(...)
declare void @risky()
declare void @observe()
@int_type = external global i8

; CHECK-LABEL: opted_in:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long [[PARENT:Lxbox_begin[0-9]+]]
; CHECK-NEXT: .long opted_in
; CHECK-NEXT: .long [[SIZE:[0-9]+]]
; CHECK-NEXT: .long 4
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long [[CLEANUP:Lxbox_begin[0-9]+]]
; CHECK-NEXT: .long opted_in
; CHECK-NEXT: .long [[SIZE]]
; CHECK-NEXT: .long 1
; OBJ: Name: .xseh
; OBJ: IMAGE_REL_PPC_ADDR32 opted_in
define void @opted_in() #0 personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %cleanup
done:
  ret void
cleanup:
  %pad = cleanuppad within none []
  call void @observe() [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; CHECK-LABEL: kernel_mode:
; CHECK-NOT: .section .xseh
; CHECK: $sehxdata$kernel_mode:
define void @kernel_mode() personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %cleanup
done:
  ret void
cleanup:
  %pad = cleanuppad within none []
  call void @observe() [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; CHECK-LABEL: nested:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long nested
; CHECK-NEXT: .long [[NESTSIZE:[0-9]+]]
; CHECK-NEXT: .long 4
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long nested
; CHECK-NEXT: .long [[NESTSIZE]]
; CHECK-NEXT: .long 5
define void @nested() #0 personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %cleanup
done:
  ret void
cleanup:
  %pad = cleanuppad within none []
  invoke void @risky() [ "funclet"(token %pad) ]
      to label %cleanupret unwind label %dispatch
dispatch:
  %switch = catchswitch within %pad [label %handler] unwind to caller
handler:
  %catch = catchpad within %switch [ptr null]
  call void @observe() [ "funclet"(token %catch) ]
  catchret from %catch to label %cleanupret
cleanupret:
  cleanupret from %pad unwind to caller
}

; CHECK-LABEL: cxx_cleanup:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long cxx_cleanup
; CHECK-NEXT: .long [[CXXSIZE:[0-9]+]]
; CHECK-NEXT: .long 0
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long cxx_cleanup
; CHECK-NEXT: .long [[CXXSIZE]]
; CHECK-NEXT: .long 1
define void @cxx_cleanup() #0 personality ptr @__CxxFrameHandler {
entry:
  invoke void @risky() to label %done unwind label %cleanup
done:
  ret void
cleanup:
  %pad = cleanuppad within none []
  call void @observe() [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; CHECK-LABEL: cxx_catch:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long cxx_catch
; CHECK-NEXT: .long [[CATCHSIZE:[0-9]+]]
; CHECK-NEXT: .long 0
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long cxx_catch
; CHECK-NEXT: .long [[CATCHSIZE]]
; CHECK-NEXT: .long 2
define void @cxx_catch() #0 personality ptr @__CxxFrameHandler {
entry:
  %caught = alloca i32, align 4
  invoke void @risky() to label %done unwind label %dispatch
dispatch:
  %switch = catchswitch within none [label %catch] unwind to caller
catch:
  %pad = catchpad within %switch [ptr @int_type, i32 0, ptr %caught]
  call void @observe() [ "funclet"(token %pad) ]
  catchret from %pad to label %done
done:
  ret void
}

$comdat_fn = comdat any

; CHECK-LABEL: comdat_fn:
; CHECK: .section .xseh,"dr",associative,comdat_fn
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long comdat_fn
; CHECK-NEXT: .long {{[0-9]+}}
; CHECK-NEXT: .long 0
define linkonce_odr void @comdat_fn() #0 comdat {
  ret void
}

attributes #0 = { "xbox360-libunwind" }

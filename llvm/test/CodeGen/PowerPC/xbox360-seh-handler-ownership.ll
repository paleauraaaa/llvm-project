; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O0 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s

declare i32 @__C_specific_handler(...)
declare void @risky()
declare void @observe(ptr)
declare ptr @llvm.eh.xbox.exceptionobject.p0(token)

; CHECK-LABEL: owned:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long owned
; CHECK-NEXT: .long {{[0-9]+}}
; CHECK-NEXT: .long 12
define void @owned() #0 personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %dispatch
dispatch:
  %cs = catchswitch within none [label %handler] unwind to caller
handler:
  %cp = catchpad within %cs [ptr null]
  %object = call ptr @llvm.eh.xbox.exceptionobject.p0(token %cp)
  catchret from %cp to label %handled
handled:
  call void @observe(ptr %object)
  br label %done
done:
  ret void
}

; A function attribute alone cannot upgrade a legacy catchpad.
; CHECK-LABEL: legacy:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long legacy
; CHECK-NEXT: .long {{[0-9]+}}
; CHECK-NEXT: .long 4
define void @legacy() #0 personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %dispatch
dispatch:
  %cs = catchswitch within none [label %handler] unwind to caller
handler:
  %cp = catchpad within %cs [ptr null]
  catchret from %cp to label %done
done:
  ret void
}

; Mixed old and new handlers must not claim ownership for the whole range.
; CHECK-LABEL: mixed:
; CHECK: .section .xseh
; CHECK: .long 1481852161
; CHECK-NEXT: .long Lxbox_begin
; CHECK-NEXT: .long mixed
; CHECK-NEXT: .long {{[0-9]+}}
; CHECK-NEXT: .long 4
define void @mixed(i1 %condition) #0 personality ptr @__C_specific_handler {
entry:
  br i1 %condition, label %first, label %second
first:
  invoke void @risky() to label %done unwind label %dispatch1
second:
  invoke void @risky() to label %done unwind label %dispatch2
dispatch1:
  %cs1 = catchswitch within none [label %handler1] unwind to caller
handler1:
  %cp1 = catchpad within %cs1 [ptr null]
  %object = call ptr @llvm.eh.xbox.exceptionobject.p0(token %cp1)
  catchret from %cp1 to label %handled
handled:
  call void @observe(ptr %object)
  br label %done
dispatch2:
  %cs2 = catchswitch within none [label %handler2] unwind to caller
handler2:
  %cp2 = catchpad within %cs2 [ptr null]
  catchret from %cp2 to label %done
done:
  ret void
}

attributes #0 = { "xbox360-libunwind" "xbox360-libunwind-handler-ownership" }

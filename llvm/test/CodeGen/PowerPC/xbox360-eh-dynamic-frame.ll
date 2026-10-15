; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O0 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s --check-prefix=INLINE
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs %s -filetype=obj -o %t.obj
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=COFF

declare i32 @__CxxFrameHandler(...)
declare void @use(ptr)
declare ptr @llvm.stacksave()
declare void @llvm.stackrestore(ptr)

; Every invocation saves r30 before allocating its fixed frame, then anchors
; that frame independently of r31 (which addresses parent locals in funclets).
; CHECK-LABEL: dynamic_frame:
; CHECK: std 30, {{-?[0-9]+}}(1)
; CHECK: stwu 1, -[[SIZE:[0-9]+]](1)
; CHECK: mr 30, 1
; CHECK: mr 31, 1
; CHECK: stw {{[0-9]+}}, [[SHARED:[0-9]+]](31)
; CHECK: bl _RtlCheckStack12
; CHECK: stwux
; CHECK: lwz 1, 0(30)
; CHECK: ld 30, {{-?[0-9]+}}(1)
; CHECK: blr
; CHECK: addi 31, 12, -[[SIZE]]
; CHECK: stwu 1, -[[SIZE]](1)
; CHECK: mr 30, 1
; CHECK: lwz {{[0-9]+}}, [[SHARED]](31)
; CHECK: bl _RtlCheckStack12
; CHECK: stwux
; CHECK: lwz 1, 0(30)
; CHECK: ld 30, {{-?[0-9]+}}(1)
; CHECK: blr
define void @dynamic_frame(i32 %size) personality ptr @__CxxFrameHandler {
entry:
  %object = alloca i8, i32 %size, align 16
  invoke void @use(ptr %object) to label %return unwind label %cleanup
return:
  ret void
cleanup:
  %pad = cleanuppad within none []
  %temporary = alloca i8, i32 %size, align 16
  call void @use(ptr %temporary) [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; A catch may allocate more than once, restore an intermediate stack snapshot,
; and return a continuation without losing either parent locals or its own
; private frame. Exercise a fixed frame beyond the signed immediate range.
; CHECK-LABEL: dynamic_catch:
; CHECK: mr 30, 1
; CHECK: mr 31, 1
; CHECK: bl _RtlCheckStack12
; CHECK: bl _RtlCheckStack12
; CHECK: lwz 1, 0(30)
; CHECK: blr
; CHECK: mr 30, 1
; CHECK: bl _RtlCheckStack12
; CHECK: bl _RtlCheckStack12
; CHECK: lwz 1, 0(30)
; CHECK: blr
define void @dynamic_catch(i32 %size) personality ptr @__CxxFrameHandler {
entry:
  %large = alloca [40000 x i8], align 16
  %value = alloca i32, align 4
  store volatile i32 %size, ptr %value
  call void @use(ptr %large)
  %first = alloca i8, i32 %size, align 16
  call void @use(ptr %first)
  %saved = call ptr @llvm.stacksave()
  %second = alloca i8, i32 %size, align 16
  invoke void @use(ptr %second) to label %restore unwind label %dispatch
restore:
  call void @llvm.stackrestore(ptr %saved)
  br label %return
dispatch:
  %switch = catchswitch within none [label %catch] unwind to caller
catch:
  %pad = catchpad within %switch [ptr null, i32 64, ptr null]
  %count = load volatile i32, ptr %value
  %buffer = alloca i8, i32 %count, align 16
  call void @use(ptr %buffer) [ "funclet"(token %pad) ]
  %catchsaved = call ptr @llvm.stacksave() [ "funclet"(token %pad) ]
  %another = alloca i8, i32 %count, align 16
  call void @use(ptr %another) [ "funclet"(token %pad) ]
  call void @llvm.stackrestore(ptr %catchsaved) [ "funclet"(token %pad) ]
  catchret from %pad to label %return
return:
  call void @use(ptr %value)
  ret void
}

; Inline probing must preserve this invocation's backchain, not reconstruct it
; from r31, which belongs to the parent when entering the cleanup.
; INLINE-LABEL: inline_dynamic:
; INLINE: mr 30, 1
; INLINE: mr 31, 1
; INLINE: lwz {{[0-9]+}}, 0(1)
; INLINE: stwux
; INLINE: lwz 1, 0(30)
; INLINE: blr
; INLINE: addi 31, 12, -{{[0-9]+}}
; INLINE: mr 30, 1
; INLINE: lwz {{[0-9]+}}, 0(1)
; INLINE: stwux
; INLINE: lwz 1, 0(30)
; INLINE: blr
define void @inline_dynamic(i32 %size) #0 personality ptr @__CxxFrameHandler {
entry:
  %savedsize = alloca i32, align 4
  store volatile i32 %size, ptr %savedsize
  %object = alloca i8, i32 %size, align 1
  invoke void @use(ptr %object) to label %return unwind label %cleanup
return:
  ret void
cleanup:
  %pad = cleanuppad within none []
  %count = load volatile i32, ptr %savedsize
  %temporary = alloca i8, i32 %count, align 1
  call void @use(ptr %temporary) [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}
attributes #0 = { "probe-stack"="inline-asm" }

; COFF-DAG: IMAGE_REL_PPC_REL24 _RtlCheckStack12
; COFF-DAG: IMAGE_REL_PPC_ADDR32 __CxxFrameHandler
; COFF-DAG: IMAGE_REL_PPC_ADDR32 $cppxdata$dynamic_frame
; COFF-DAG: IMAGE_REL_PPC_ADDR32 $cppxdata$dynamic_catch

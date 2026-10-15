; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O0 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t
; RUN: llvm-readobj --relocations %t | FileCheck %s --check-prefix=RELOC
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon %s -o %t.s
; RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t.s -o %t.roundtrip

@int_type = external global i8
declare i32 @__CxxFrameHandler(...)
declare void @may_throw()
declare void @destroy(ptr) nounwind
declare void @use(ptr)

; CHECK: .long __CxxFrameHandler
; CHECK-NEXT: .long ($cppxdata$catch_int)
; CHECK-NEXT: catch_int:
; CHECK: li 0, 0
; CHECK-NEXT: stw 0, 4(1)
; CHECK: stwu 1, -[[FRAME:[0-9]+]](1)
; CHECK: mr 31, 1
; CHECK: addi 1, 1, [[FRAME]]
; CHECK: .long (0xc0000000
; CHECK: .long __CxxFrameHandler
; CHECK-NEXT: .long ($cppxdata$catch_int)
; CHECK: addi 31, 12, -[[FRAME]]
; CHECK: stwu 1, -[[FRAME]](1)
; CHECK: lwz {{[0-9]+}}, {{[0-9]+}}(31)
; CHECK: lis 3, {{.*}}@ha
; CHECK-NEXT: addi 3, 3, {{.*}}@l
; CHECK-NEXT: blr
; CHECK: .long (0xc0000000
; CHECK: $cppxdata$catch_int:
; CHECK-NEXT: .long 429065506
; CHECK-NEXT: .long 2
; CHECK-NEXT: .long ($stateUnwindMap$catch_int)
; CHECK-NEXT: .long 1
; CHECK-NEXT: .long ($tryMap$catch_int)
; CHECK-NEXT: .long 4
; CHECK-NEXT: .long ($ip2state$catch_int)
; CHECK-NEXT: .long 0
; CHECK-NEXT: .long 1
; CHECK-NEXT: $stateUnwindMap$catch_int:
; CHECK: $handlerMap$0$catch_int:
; CHECK-NEXT: .long 0
; CHECK-NEXT: .long int_type
; CHECK-NEXT: .long -{{[0-9]+}}
; CHECK-NEXT: .long {{.*}}
; CHECK-NEXT: $ip2state$catch_int:
; CHECK-NOT: +1
define i32 @catch_int() personality ptr @__CxxFrameHandler {
entry:
  %caught = alloca i32, align 4
  %result = alloca i32, align 4
  invoke void @may_throw() to label %normal unwind label %dispatch
normal:
  store i32 0, ptr %result
  br label %return
dispatch:
  %switch = catchswitch within none [label %catch] unwind to caller
catch:
  %pad = catchpad within %switch [ptr @int_type, i32 0, ptr %caught]
  %value = load i32, ptr %caught
  store i32 %value, ptr %result
  catchret from %pad to label %return
return:
  %retval = load i32, ptr %result
  ret i32 %retval
}

; CHECK: .long __CxxFrameHandler
; CHECK-NEXT: .long ($cppxdata$cleanup)
; CHECK-NEXT: cleanup:
; CHECK: .long (0xc0000000
; CHECK: addi 31, 12, -{{[0-9]+}}
; CHECK: addi {{[0-9]+}}, 31, {{[0-9]+}}
; CHECK: bl destroy
; CHECK: blr
; CHECK: .long (0x40000000
; CHECK: $cppxdata$cleanup:
; CHECK-NEXT: .long 429065506
; CHECK-NEXT: .long 1
; CHECK: $stateUnwindMap$cleanup:
; CHECK-NEXT: .long -1
; CHECK-NEXT: .long {{.*}}
define void @cleanup() personality ptr @__CxxFrameHandler {
entry:
  %object = alloca i32, align 4
  store i32 7, ptr %object
  invoke void @may_throw() to label %normal unwind label %unwind
normal:
  call void @destroy(ptr %object)
  ret void
unwind:
  %pad = cleanuppad within none []
  call void @destroy(ptr %object) [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; CHECK: large_frame:
; CHECK: stwux 1, 1, {{[0-9]+}}
; CHECK: .long (0xc0000000
; CHECK: lis 0, -1
; CHECK: ori 0, 0, {{[0-9]+}}
; CHECK: add 31, 12, 0
; CHECK: stwux 1, 1, {{[0-9]+}}
; CHECK: blr
; CHECK: .long (0x40000000
; CHECK: $cppxdata$large_frame:
define void @large_frame() personality ptr @__CxxFrameHandler {
entry:
  %object = alloca [40000 x i8], align 16
  invoke void @use(ptr %object) to label %return unwind label %cleanup
return:
  ret void
cleanup:
  %pad = cleanuppad within none []
  call void @destroy(ptr %object) [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; CHECK: catch_only:
; CHECK: .long (0xc0000000
; CHECK: lis 3, [[CONT:LBB[0-9_]+]]@ha
; CHECK-NEXT: addi 3, 3, [[CONT]]@l
; CHECK-NEXT: blr
; CHECK: $cppxdata$catch_only:
declare void @always_throw() noreturn
define i32 @catch_only() personality ptr @__CxxFrameHandler {
entry:
  invoke void @always_throw() to label %unreachable unwind label %dispatch
unreachable:
  unreachable
dispatch:
  %switch = catchswitch within none [label %catch] unwind to caller
catch:
  %pad = catchpad within %switch [ptr null, i32 64, ptr null]
  catchret from %pad to label %return
return:
  ret i32 9
}

; RELOC: IMAGE_REL_PPC_ADDR32 __CxxFrameHandler
; RELOC: IMAGE_REL_PPC_REFHI
; RELOC-NEXT: {{.*}}IMAGE_REL_PPC_PAIR
; RELOC: IMAGE_REL_PPC_REFLO
; RELOC: IMAGE_REL_PPC_ADDR32 int_type
; RELOC-NOT: IMAGE_REL_PPC_ADDR32NB

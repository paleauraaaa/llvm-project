; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O0 %s -o - | FileCheck %s --check-prefixes=CHECK,O0
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s --check-prefixes=CHECK,O2
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t.obj
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=RELOC
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon %s -o %t.s
; RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %t.s -o %t.roundtrip.obj
; RUN: llvm-as %s -o - | llvm-dis -o - | FileCheck %s --check-prefix=ROUNDTRIP

declare i32 @__C_specific_handler(...)
declare void @risky(ptr)
declare void @observe(i32)
declare ppc_xbox360_sehcc i32 @filter(ptr, ptr)
declare ppc_xbox360_sehcc void @termination(i8, ptr)
declare void @llvm.seh.try.begin()
declare void @llvm.seh.try.end()
declare i32 @llvm.eh.exceptioncode(token)
declare ptr @llvm.localaddress()
declare void @llvm.localescape(...)
declare ptr @llvm.localrecover(ptr, ptr, i32)
declare ptr @llvm.eh.recoverfp(ptr, ptr)

; CHECK: .long __C_specific_handler
; CHECK-NEXT: .long ($sehxdata$except)
; CHECK-NEXT: except:
; CHECK: stwu 1, -[[FRAME:[0-9]+]](1)
; CHECK: bl risky
; Except bodies stay in the parent and return normally, not an address in r3.
; CHECK-NOT: addi 31, 12,
; CHECK: blr
; CHECK: .long (0xc0000000
; CHECK: $sehxdata$except:
; CHECK: .long filter
; CHECK-NEXT: .long LBB
define i32 @except(ptr %p) personality ptr @__C_specific_handler {
entry:
  invoke void @risky(ptr %p) to label %normal unwind label %dispatch
normal:
  ret i32 0
dispatch:
  %cs = catchswitch within none [label %handler] unwind to caller
handler:
  %cp = catchpad within %cs [ptr @filter]
  %code = call i32 @llvm.eh.exceptioncode(token %cp)
  catchret from %cp to label %handled
handled:
  ret i32 %code
}

; CHECK: .long __C_specific_handler
; CHECK-NEXT: .long ($sehxdata$finally)
; CHECK-NEXT: finally:
; CHECK: addi {{[0-9]+}}, 31, [[PARENT:[0-9]+]]
; CHECK: clrldi 12,
; CHECK: li 3, 0
; CHECK: bl termination
; CHECK: blr
; CHECK: addi 31, 12, -[[PARENT]]
; CHECK: bl termination
; CHECK: blr
; CHECK: .long (0x40000000
; CHECK: $sehxdata$finally:
; CHECK: .long LBB
; CHECK-NEXT: .long 0
define void @finally(ptr %p) personality ptr @__C_specific_handler {
entry:
  invoke void @risky(ptr %p) to label %normal unwind label %cleanup
normal:
  %fp = call ptr @llvm.localaddress()
  call ppc_xbox360_sehcc void @termination(i8 0, ptr %fp)
  ret void
cleanup:
  %pad = cleanuppad within none []
  %parent = call ptr @llvm.localaddress() [ "funclet"(token %pad) ]
  call ppc_xbox360_sehcc void @termination(i8 1, ptr %parent) [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; A catch-all in hand-written IR also gets a real native filter.
; CHECK: catch_all:
; CHECK: $sehfilter$catch_all:
; CHECK-NEXT: li 3, 1
; CHECK-NEXT: blr
; CHECK: .long (0x40000000
; CHECK: $sehxdata$catch_all:
; CHECK: .long ($sehfilter$catch_all)
define i32 @catch_all(ptr %p) personality ptr @__C_specific_handler {
entry:
  invoke void @risky(ptr %p) to label %normal unwind label %dispatch
normal:
  ret i32 0
dispatch:
  %cs = catchswitch within none [label %handler] unwind to caller
handler:
  %cp = catchpad within %cs [ptr null]
  catchret from %cp to label %handled
handled:
  ret i32 1
}

; Memory-only scopes must survive without an eh-asynch module flag.
; CHECK: memory:
; CHECK: %body
; CHECK: [[BEGIN:Ltmp[0-9]+]]:
; CHECK: lwz
; CHECK: stw
; CHECK: [[END:Ltmp[0-9]+]]:
; CHECK: $sehxdata$memory:
; CHECK: .long [[BEGIN]]
; CHECK-NEXT: .long [[END]]
; CHECK-NEXT: .long filter
define void @memory(ptr %p) personality ptr @__C_specific_handler {
entry:
  invoke void @llvm.seh.try.begin() to label %body unwind label %dispatch
body:
  %v = load volatile i32, ptr %p
  store volatile i32 %v, ptr %p
  invoke void @llvm.seh.try.end() to label %done unwind label %dispatch
done:
  ret void
dispatch:
  %cs = catchswitch within none [label %handler] unwind to caller
handler:
  %cp = catchpad within %cs [ptr @filter]
  catchret from %cp to label %done
}

; Escaped offsets are relative to the incoming SP, not the allocated frame.
; CHECK: escaped:
; CHECK: .set [[LOCAL:L[^ ,]+]], -{{[4-9][0-9][0-9][0-9][0-9]}}
define void @escaped() {
entry:
  %local = alloca [40000 x i8], align 16
  call void (...) @llvm.localescape(ptr %local)
  call void @risky(ptr %local)
  ret void
}

; CHECK: recover:
; O0: mr 3, 12
; CHECK: lis [[OFFSET:[1-9][0-9]*]], [[LOCAL]]@ha
; CHECK-NEXT: addi [[OFFSET]], [[OFFSET]], [[LOCAL]]@l
; O0: add 3, 3, [[OFFSET]]
; O2: add 3, 12, [[OFFSET]]
; CHECK: blr
define ppc_xbox360_sehcc ptr @recover(ptr %info, ptr %parent) {
entry:
  %fp = call ptr @llvm.eh.recoverfp(ptr @escaped, ptr %parent)
  %local = call ptr @llvm.localrecover(ptr @escaped, ptr %fp, i32 0)
  ret ptr %local
}

; CHECK: large_frame:
; CHECK: bl _RtlCheckStack12
; CHECK: add 31, 12, 0
; CHECK-NOT: bl _RtlCheckStack12
; CHECK: stwu 1, -[[PRIVATE:96|112]](1)
; CHECK: bl termination
; CHECK: addi 1, 1, [[PRIVATE]]
; CHECK: blr
; CHECK: $sehxdata$large_frame:
define void @large_frame() personality ptr @__C_specific_handler {
entry:
  %buffer = alloca [40000 x i8], align 16
  invoke void @risky(ptr %buffer) to label %done unwind label %cleanup
done:
  ret void
cleanup:
  %pad = cleanuppad within none []
  %parent = call ptr @llvm.localaddress() [ "funclet"(token %pad) ]
  call ppc_xbox360_sehcc void @termination(i8 1, ptr %parent) [ "funclet"(token %pad) ]
  cleanupret from %pad unwind to caller
}

; The incoming r12 is an argument, not an available LR scratch register.
; CHECK: nonleaf_helper:
; CHECK-NOT: mflr 12
; CHECK: mflr 11
; CHECK-NOT: mflr 12
; CHECK: bl risky
; CHECK: blr
define ppc_xbox360_sehcc void @nonleaf_helper(ptr %info, ptr %parent) #0 {
  call void @risky(ptr %parent)
  ret void
}

; The stack-probing helper needs r12 for the displacement. Preserve the
; parent pointer in an argument register that the probing helper preserves.
; CHECK: large_helper:
; CHECK: mr 10, 12
; CHECK: bl _RtlCheckStack12
; CHECK: mr 12, 10
; CHECK: bl risky
; CHECK: bl risky
; CHECK: blr
define ppc_xbox360_sehcc void @large_helper(ptr %info, ptr %parent) #0 {
  %buffer = alloca [40000 x i8], align 16
  call void @risky(ptr %buffer)
  call void @risky(ptr %parent)
  ret void
}

attributes #0 = { "disable-tail-calls"="true" }

; RELOC: IMAGE_REL_PPC_ADDR32 __C_specific_handler
; RELOC: IMAGE_REL_PPC_ADDR32 $sehxdata$except
; RELOC: IMAGE_REL_PPC_ADDR32 filter
; RELOC-NOT: IMAGE_REL_PPC_ADDR32NB
; ROUNDTRIP: declare ppc_xbox360_sehcc i32 @filter(ptr, ptr)
; ROUNDTRIP: define ppc_xbox360_sehcc ptr @recover(ptr %info, ptr %parent)

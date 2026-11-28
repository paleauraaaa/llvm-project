; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O0 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs -O2 %s -o - | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t
; RUN: llvm-readobj --relocations %t | FileCheck %s --check-prefix=RELOC

declare i32 @__C_specific_handler(...)
declare void @risky()
declare void @observe()
declare void @llvm.seh.try.begin()
declare void @llvm.seh.try.end()

; CHECK-LABEL: nested_cleanup:
; CHECK: Ltmp{{[0-9]+}}:
; CHECK-NEXT: [[PBEGIN:Ltmp[0-9]+]]:
; CHECK-NEXT: bl risky
; CHECK-NEXT: [[PEND:Ltmp[0-9]+]]:
; CHECK: .long (0xc0000000
; CHECK: .long __C_specific_handler
; CHECK-NEXT: .long [[TABLE:Lxbox_seh_cleanup[0-9]+]]
; CHECK-NEXT: [[OUTER:LBB[0-9]+_[0-9]+]]:
; CHECK: Ltmp{{[0-9]+}}:
; CHECK-NEXT: [[CBEGIN:Ltmp[0-9]+]]:
; CHECK-NEXT: bl risky
; CHECK-NEXT: [[CEND:Ltmp[0-9]+]]:
; CHECK: .long (0xc0000000
; CHECK: [[INNER:LBB[0-9]+_[0-9]+]]:
; CHECK: .long (0x40000000
; CHECK: $sehxdata$nested_cleanup:
; CHECK: .long [[PBEGIN]]
; CHECK-NEXT: .long [[PEND]]
; CHECK-NEXT: .long [[OUTER]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
; CHECK: [[TABLE]]:
; CHECK: .long [[CBEGIN]]
; CHECK-NEXT: .long [[CEND]]
; CHECK-NEXT: .long [[INNER]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
; RELOC: IMAGE_REL_PPC_ADDR32 $sehxdata$nested_cleanup
; RELOC: IMAGE_REL_PPC_ADDR32 .xdata
define void @nested_cleanup() personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %outer
done:
  ret void
outer:
  %outerpad = cleanuppad within none []
  invoke void @risky() [ "funclet"(token %outerpad) ]
      to label %outerret unwind label %inner
outerret:
  cleanupret from %outerpad unwind to caller
inner:
  %innerpad = cleanuppad within %outerpad []
  call void @observe() [ "funclet"(token %innerpad) ]
  cleanupret from %innerpad unwind to caller
}

; CHECK-LABEL: three_levels:
; CHECK: .long [[TABLE1:Lxbox_seh_cleanup[0-9]+]]
; CHECK-NEXT: [[OUTER1:LBB[0-9]+_[0-9]+]]:
; CHECK: .long [[TABLE2:Lxbox_seh_cleanup[0-9]+]]
; CHECK-NEXT: [[MIDDLE:LBB[0-9]+_[0-9]+]]:
; CHECK: [[INNER1:LBB[0-9]+_[0-9]+]]: {{.*}}%inner
; CHECK: $sehxdata$three_levels:
; CHECK: .long [[OUTER1]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
; CHECK: [[TABLE1]]:
; CHECK: .long [[MIDDLE]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
; CHECK: [[TABLE2]]:
; CHECK: .long [[INNER1]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
define void @three_levels() personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %outer
done:
  ret void
outer:
  %outerpad = cleanuppad within none []
  invoke void @risky() [ "funclet"(token %outerpad) ]
      to label %outerret unwind label %middle
outerret:
  cleanupret from %outerpad unwind to caller
middle:
  %middlepad = cleanuppad within %outerpad []
  invoke void @risky() [ "funclet"(token %middlepad) ]
      to label %middleret unwind label %inner
middleret:
  cleanupret from %middlepad unwind to caller
inner:
  %innerpad = cleanuppad within %middlepad []
  call void @observe() [ "funclet"(token %innerpad) ]
  cleanupret from %innerpad unwind to caller
}

; CHECK-LABEL: cleanup_catch:
; CHECK: .long [[CATCHTABLE:Lxbox_seh_cleanup[0-9]+]]
; CHECK-NEXT: [[CATCHOUTER:LBB[0-9]+_[0-9]+]]:
; CHECK: [[CATCHHANDLER:LBB[0-9]+_[0-9]+]]: {{.*}}%handler
; CHECK: .long (0xc0000000
; CHECK: $sehxdata$cleanup_catch:
; CHECK: .long [[CATCHOUTER]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
; CHECK: [[CATCHTABLE]]:
; CHECK: .long $sehfilter$cleanup_catch
; CHECK-NEXT: .long [[CATCHHANDLER]]
; CHECK-NEXT: Llsda_end
define void @cleanup_catch() personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %outer
done:
  ret void
outer:
  %outerpad = cleanuppad within none []
  invoke void @risky() [ "funclet"(token %outerpad) ]
      to label %outerret unwind label %dispatch
dispatch:
  %switch = catchswitch within %outerpad [label %handler] unwind to caller
handler:
  %catch = catchpad within %switch [ptr null]
  call void @observe() [ "funclet"(token %catch) ]
  catchret from %catch to label %outerret
outerret:
  cleanupret from %outerpad unwind to caller
}

; CHECK-LABEL: cleanup_async:
; CHECK: .long [[ASYNCTABLE:Lxbox_seh_cleanup[0-9]+]]
; CHECK: %body
; CHECK: [[ASYNCBEGIN:Ltmp[0-9]+]]:
; CHECK: lwz
; CHECK: stw
; CHECK: [[ASYNCEND:Ltmp[0-9]+]]:
; CHECK: [[ASYNCINNER:LBB[0-9]+_[0-9]+]]: {{.*}}Block address taken
; CHECK: [[ASYNCTABLE]]:
; CHECK: .long [[ASYNCBEGIN]]
; CHECK-NEXT: .long [[ASYNCEND]]
; CHECK-NEXT: .long [[ASYNCINNER]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
define void @cleanup_async(ptr %p) personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %outer
done:
  ret void
outer:
  %outerpad = cleanuppad within none []
  invoke void @llvm.seh.try.begin() [ "funclet"(token %outerpad) ]
      to label %body unwind label %inner
body:
  %value = load volatile i32, ptr %p
  store volatile i32 %value, ptr %p
  invoke void @llvm.seh.try.end() [ "funclet"(token %outerpad) ]
      to label %outerret unwind label %inner
outerret:
  cleanupret from %outerpad unwind to caller
inner:
  %innerpad = cleanuppad within %outerpad []
  call void @observe() [ "funclet"(token %innerpad) ]
  cleanupret from %innerpad unwind to caller
}

; CHECK-LABEL: cleanup_outer_handler:
; CHECK: .long [[EXTERNALTABLE:Lxbox_seh_cleanup[0-9]+]]
; CHECK: [[EXTERNALINNER:LBB[0-9]+_[0-9]+]]: {{.*}}%inner
; CHECK: [[EXTERNALTABLE]]:
; CHECK: .long [[EXTERNALINNER]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
define void @cleanup_outer_handler() personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %outer
done:
  ret void
outer:
  %outerpad = cleanuppad within none []
  invoke void @risky() [ "funclet"(token %outerpad) ]
      to label %outerret unwind label %inner
outerret:
  cleanupret from %outerpad unwind label %dispatch
inner:
  %innerpad = cleanuppad within %outerpad []
  call void @observe() [ "funclet"(token %innerpad) ]
  cleanupret from %innerpad unwind label %dispatch
dispatch:
  %switch = catchswitch within none [label %handler] unwind to caller
handler:
  %catch = catchpad within %switch [ptr null]
  catchret from %catch to label %done
}

; CHECK-LABEL: cleanup_chain:
; CHECK: .long [[CHAINTABLE:Lxbox_seh_cleanup[0-9]+]]
; CHECK: [[FIRST:LBB[0-9]+_[0-9]+]]: {{.*}}%first
; CHECK: [[SECOND:LBB[0-9]+_[0-9]+]]: {{.*}}%second
; CHECK: [[CHAINTABLE]]:
; CHECK: .long [[FIRST]]
; CHECK-NEXT: .long 0
; CHECK: .long [[SECOND]]
; CHECK-NEXT: .long 0
; CHECK-NEXT: Llsda_end
define void @cleanup_chain() personality ptr @__C_specific_handler {
entry:
  invoke void @risky() to label %done unwind label %outer
done:
  ret void
outer:
  %outerpad = cleanuppad within none []
  invoke void @risky() [ "funclet"(token %outerpad) ]
      to label %outerret unwind label %first
outerret:
  cleanupret from %outerpad unwind to caller
first:
  %firstpad = cleanuppad within %outerpad []
  call void @observe() [ "funclet"(token %firstpad) ]
  cleanupret from %firstpad unwind label %second
second:
  %secondpad = cleanuppad within %outerpad []
  call void @observe() [ "funclet"(token %secondpad) ]
  cleanupret from %secondpad unwind to caller
}

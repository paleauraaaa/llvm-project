; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -stop-after=finalize-isel < %s | FileCheck %s --check-prefix=MIR

declare void @llvm.ppc.xbox.read.barrier(...)
declare void @llvm.ppc.xbox.write.barrier(...)
declare void @llvm.ppc.xbox.read.write.barrier(...)

; CHECK-LABEL: read:
; CHECK: #MEMBARRIER
; CHECK-NEXT: blr
; MIR-LABEL: name: read
; MIR: MEMBARRIER
define void @read() {
  call void (...) @llvm.ppc.xbox.read.barrier()
  ret void
}

; CHECK-LABEL: write:
; CHECK: #MEMBARRIER
; CHECK-NEXT: blr
; MIR-LABEL: name: write
; MIR: MEMBARRIER
define void @write() {
  call void (...) @llvm.ppc.xbox.write.barrier()
  ret void
}

; CHECK-LABEL: readwrite:
; CHECK: #MEMBARRIER
; CHECK-NEXT: blr
; MIR-LABEL: name: readwrite
; MIR: MEMBARRIER
define void @readwrite() {
  call void (...) @llvm.ppc.xbox.read.write.barrier()
  ret void
}

; CHECK-LABEL: ordered:
; CHECK: lwz
; CHECK: #MEMBARRIER
; CHECK: lwz
; CHECK: #MEMBARRIER
; CHECK: stw
; CHECK: #MEMBARRIER
; CHECK: stw
; CHECK: blr
define i32 @ordered(ptr %p) {
  %a = load i32, ptr %p
  call void (...) @llvm.ppc.xbox.read.barrier()
  %b = load i32, ptr %p
  call void (...) @llvm.ppc.xbox.write.barrier()
  store i32 1, ptr %p
  call void (...) @llvm.ppc.xbox.write.barrier()
  store i32 2, ptr %p
  %sum = add i32 %a, %b
  ret i32 %sum
}

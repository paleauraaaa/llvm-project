; RUN: split-file %s %t
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -verify-machineinstrs < %t/xbox.ll | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -verify-machineinstrs < %t/xbox.ll | FileCheck %s
; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -stop-after=finalize-isel < %t/xbox.ll | FileCheck %s --check-prefix=MEM
; RUN: llc -mtriple=powerpc64-unknown-linux -mcpu=pwr7 < %t/vsx.ll | FileCheck %s --check-prefix=VSX

;--- xbox.ll
declare double @llvm.ppc.fcfid(double)
declare double @llvm.ppc.fctid(double)
declare i32 @llvm.ppc.load2r.volatile(ptr)
declare i32 @llvm.ppc.load4r.volatile(ptr)
declare i64 @llvm.ppc.load8r.volatile(ptr)
declare void @llvm.ppc.store2r.volatile(i32, ptr)
declare void @llvm.ppc.store4r.volatile(i32, ptr)
declare void @llvm.ppc.store8r.volatile(i64, ptr)

; CHECK-LABEL: convert:
; CHECK-NOT: xscv
; CHECK: fcfid 1, 1
; CHECK: blr
define double @convert(double %bits) {
  %result = call double @llvm.ppc.fcfid(double %bits)
  ret double %result
}

; CHECK-LABEL: round:
; CHECK: fctid
; CHECK-NOT: xscv
; CHECK: fcfid
; CHECK: blr
define double @round(double %value) {
  %bits = call double @llvm.ppc.fctid(double %value)
  %result = call double @llvm.ppc.fcfid(double %bits)
  ret double %result
}

; CHECK-LABEL: volatile_accesses:
; CHECK: lhbrx
; CHECK: lwbrx
; CHECK: sthbrx
; CHECK: stwbrx
; CHECK: lhbrx
; CHECK: lwbrx
; CHECK: blr
; MEM-LABEL: name: volatile_accesses
; MEM: LHBRX_volatile {{.*}} :: (volatile load (s16) from %ir.pointer, align 1)
; MEM: LWBRX_volatile {{.*}} :: (volatile load (s32) from %ir.pointer, align 1)
; MEM: STHBRX_volatile {{.*}} :: (volatile store (s16) into %ir.pointer, align 1)
; MEM: STWBRX_volatile {{.*}} :: (volatile store (s32) into %ir.pointer, align 1)
define void @volatile_accesses(ptr %pointer) {
  %a = call i32 @llvm.ppc.load2r.volatile(ptr %pointer)
  %b = call i32 @llvm.ppc.load4r.volatile(ptr %pointer)
  call void @llvm.ppc.store2r.volatile(i32 %a, ptr %pointer)
  call void @llvm.ppc.store4r.volatile(i32 %b, ptr %pointer)
  %unused1 = call i32 @llvm.ppc.load2r.volatile(ptr %pointer)
  %unused2 = call i32 @llvm.ppc.load4r.volatile(ptr %pointer)
  ret void
}

; CHECK-LABEL: volatile_doubleword:
; CHECK: ldbrx
; CHECK: stdbrx
; CHECK: ldbrx
; CHECK: blr
; MEM-LABEL: name: volatile_doubleword
; MEM: LDBRX_volatile {{.*}} :: (volatile load (s64) from %ir.pointer, align 1)
; MEM: STDBRX_volatile {{.*}} :: (volatile store (s64) into %ir.pointer, align 1)
define void @volatile_doubleword(ptr %pointer) {
  %value = call i64 @llvm.ppc.load8r.volatile(ptr %pointer)
  call void @llvm.ppc.store8r.volatile(i64 %value, ptr %pointer)
  %unused = call i64 @llvm.ppc.load8r.volatile(ptr %pointer)
  ret void
}

;--- vsx.ll
declare double @llvm.ppc.fcfid(double)

; VSX-LABEL: convert:
; VSX: xscvsxddp
define double @convert(double %bits) {
  %result = call double @llvm.ppc.fcfid(double %bits)
  ret double %result
}

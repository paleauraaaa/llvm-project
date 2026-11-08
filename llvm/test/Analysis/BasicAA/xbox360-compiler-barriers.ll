; RUN: opt -aa-pipeline=basic-aa -passes=aa-eval -print-all-alias-modref-info -disable-output < %s 2>&1 | FileCheck %s
; RUN: opt -aa-pipeline=basic-aa,globals-aa,scoped-noalias-aa,tbaa -passes=aa-eval -print-all-alias-modref-info -disable-output < %s 2>&1 | FileCheck %s
; RUN: opt -S -passes='gvn,dse' < %s | FileCheck %s --check-prefix=OPT
; RUN: opt -S -passes='default<O2>' < %s | FileCheck %s --check-prefix=OPT

declare void @llvm.ppc.xbox.read.barrier(...)
declare void @llvm.ppc.xbox.write.barrier(...)
declare void @llvm.ppc.xbox.read.write.barrier(...)

; CHECK-LABEL: Function: ordinary:
; CHECK: Just Mod: {{.*}}%p{{.*}}@llvm.ppc.xbox.read.barrier()
; CHECK: Just Ref: {{.*}}%p{{.*}}@llvm.ppc.xbox.write.barrier()
; CHECK: Both ModRef: {{.*}}%p{{.*}}@llvm.ppc.xbox.read.write.barrier()
define i32 @ordinary(ptr %p) {
  %v = load i32, ptr %p
  call void (...) @llvm.ppc.xbox.read.barrier()
  call void (...) @llvm.ppc.xbox.write.barrier()
  call void (...) @llvm.ppc.xbox.read.write.barrier()
  ret i32 %v
}

; CHECK-LABEL: Function: restricted:
; CHECK: Both ModRef: {{.*}}%p{{.*}}@llvm.ppc.xbox.read.barrier(ptr %p)
; CHECK: Both ModRef: {{.*}}%p{{.*}}@llvm.ppc.xbox.write.barrier(ptr %p)
; CHECK: Both ModRef: {{.*}}%p{{.*}}@llvm.ppc.xbox.read.write.barrier(ptr %p)
define i32 @restricted(ptr noalias %p) {
  %v = load i32, ptr %p
  call void (...) @llvm.ppc.xbox.read.barrier(ptr %p)
  call void (...) @llvm.ppc.xbox.write.barrier(ptr %p)
  call void (...) @llvm.ppc.xbox.read.write.barrier(ptr %p)
  ret i32 %v
}

; OPT-LABEL: define {{.*}} @scoped_reads(
; OPT: load i32
; OPT: call void (...) @llvm.ppc.xbox.write.barrier()
; OPT-NOT: load i32
; OPT: ret i32
define i32 @scoped_reads(ptr %p) {
  %a = load i32, ptr %p, !alias.scope !0
  call void (...) @llvm.ppc.xbox.write.barrier()
  %b = load i32, ptr %p, !alias.scope !0
  %sum = add i32 %a, %b
  ret i32 %sum
}

; OPT-LABEL: define {{.*}} @scoped_writes(
; OPT-NOT: store i32 1
; OPT: call void (...) @llvm.ppc.xbox.read.barrier()
; OPT: store i32 2
; OPT: ret void
define void @scoped_writes(ptr %p) {
  store i32 1, ptr %p, !alias.scope !0
  call void (...) @llvm.ppc.xbox.read.barrier()
  store i32 2, ptr %p, !alias.scope !0
  ret void
}

; OPT-LABEL: define {{.*}} @selected_restricted(
; OPT: load i32
; OPT: call void (...) @llvm.ppc.xbox.read.barrier(ptr %p)
; OPT: load i32
; OPT: ret i32
define i32 @selected_restricted(ptr noalias %p, ptr %q, i1 %cond) {
  %base = select i1 %cond, ptr %p, ptr %q
  %ptr = getelementptr i32, ptr %base, i32 1
  %a = load i32, ptr %ptr
  call void (...) @llvm.ppc.xbox.read.barrier(ptr %p)
  %b = load i32, ptr %ptr
  %sum = add i32 %a, %b
  ret i32 %sum
}

!0 = !{!1}
!1 = distinct !{!1, !2, !"restricted pointer"}
!2 = distinct !{!2, !"scope domain"}

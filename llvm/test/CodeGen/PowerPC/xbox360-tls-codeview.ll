; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -O0 -filetype=obj %s -o %t.obj
; RUN: llvm-readobj --codeview %t.obj | FileCheck %s --check-prefix=CV --implicit-check-not=S_GDATA32 --implicit-check-not=S_LDATA32
; RUN: llvm-readobj --relocations %t.obj | FileCheck %s --check-prefix=RELOC

; XDK emits little-endian S_GTHREAD32/S_LTHREAD32 records, with ordinary
; SECREL/SECTION debug relocations, distinct from code's SECREL16 offsets.

@tls_global = thread_local global i32 7, align 4, !dbg !0
@tls_static = internal thread_local global i32 11, align 4, !dbg !3

; CV: GlobalTLS {
; CV-NEXT: Kind: S_GTHREAD32 (0x1113)
; CV-NEXT: DataOffset: tls_global+0x0
; CV-NEXT: Type: int (0x74)
; CV-NEXT: DisplayName: tls_global
; CV: ThreadLocalDataSym {
; CV-NEXT: Kind: S_LTHREAD32 (0x1112)
; CV-NEXT: DataOffset: tls_static+0x0
; CV-NEXT: Type: int (0x74)
; CV-NEXT: DisplayName: tls_static

; RELOC: .debug$S {
; RELOC-DAG: IMAGE_REL_PPC_SECREL tls_global
; RELOC-DAG: IMAGE_REL_PPC_SECTION tls_global
; RELOC-DAG: IMAGE_REL_PPC_SECREL tls_static
; RELOC-DAG: IMAGE_REL_PPC_SECTION tls_static

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!8, !9}
!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "tls_global", scope: !2, file: !5,
  line: 1, type: !6, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "clang",
  isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !7)
!3 = !DIGlobalVariableExpression(var: !4, expr: !DIExpression())
!4 = distinct !DIGlobalVariable(name: "tls_static", scope: !2, file: !5,
  line: 2, type: !6, isLocal: true, isDefinition: true)
!5 = !DIFile(filename: "tls.c", directory: "")
!6 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!7 = !{!0, !3}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = !{i32 2, !"CodeView", i32 1}

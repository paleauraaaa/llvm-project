; RUN: llc -mtriple=ppc64-ibm-xbox360 -mcpu=xenon -O2 -filetype=obj < %s -o %t.obj
; RUN: llvm-readobj --codeview %t.obj | FileCheck %s

; CHECK-NOT: TypeLeafKind: LF_FUNC_ID
; CHECK-NOT: TypeLeafKind: LF_STRING_ID
; CHECK-NOT: TypeLeafKind: LF_BUILDINFO
; CHECK: Machine: PPC604 (0x42)
; CHECK-NOT: S_GPROC32_ID
; CHECK: GlobalProcSym {
; CHECK: FunctionType: void () (0x1002)
; CHECK: Register: PPC_VR1 (0x4B5)
; CHECK: ScopeEndSym {
; CHECK: Kind: S_END (0x6)
; CHECK-NOT: BuildInfoSym

define void @debug_vector(<4 x i32> %v) !dbg !4 {
entry:
  call void @llvm.dbg.value(metadata <4 x i32> %v, metadata !8,
                            metadata !DIExpression()), !dbg !9
  ret void, !dbg !10
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "clang",
    isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "xbox-codeview.c", directory: "")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 2, !"CodeView", i32 1}
!4 = distinct !DISubprogram(name: "debug_vector", scope: !1, file: !1,
    line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0,
    retainedNodes: !14)
!5 = !DISubroutineType(types: !6)
!6 = !{null, !7}
!7 = !DICompositeType(tag: DW_TAG_array_type, baseType: !11, size: 128,
    elements: !12, flags: DIFlagVector)
!8 = !DILocalVariable(name: "v", arg: 1, scope: !4, file: !1, line: 1,
    type: !7)
!9 = !DILocation(line: 1, column: 1, scope: !4)
!10 = !DILocation(line: 2, column: 1, scope: !4)
!11 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!12 = !{!13}
!13 = !DISubrange(count: 4)
!14 = !{!8}

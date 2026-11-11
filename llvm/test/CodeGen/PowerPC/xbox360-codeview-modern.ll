; RUN: llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t.modern.obj
; RUN: llvm-readobj --codeview %t.modern.obj | FileCheck %s --check-prefix=MODERN
; RUN: sed 's/!"CodeViewXbox360Modern", i32 1/!"CodeViewXbox360Modern", i32 0/' %s | llc -mtriple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj -o %t.legacy.obj
; RUN: llvm-readobj --codeview %t.legacy.obj | FileCheck %s --check-prefix=LEGACY --implicit-check-not=LF_FUNC_ID --implicit-check-not=LF_STRING_ID --implicit-check-not=LF_UDT_SRC_LINE --implicit-check-not=LF_BUILDINFO --implicit-check-not=InlineSiteSym --implicit-check-not=BuildInfoSym

; Modern records must not change the target's register numbering or endianness.
; MODERN-DAG: TypeLeafKind: LF_FUNC_ID
; MODERN-DAG: TypeLeafKind: LF_UDT_SRC_LINE
; MODERN-DAG: UDT: Sample
; MODERN-DAG: SourceFile: {{.*}}modern.cpp
; MODERN-DAG: LineNumber: 3
; MODERN-DAG: TypeLeafKind: LF_BUILDINFO
; MODERN: Machine: PPCBE
; MODERN: SubSectionType: InlineeLines
; MODERN: Inlinee: outer
; MODERN: SourceLineNum: 20
; MODERN: Inlinee: inner
; MODERN: SourceLineNum: 10
; MODERN: Kind: S_GPROC32_ID
; MODERN: InlineSiteSym {
; MODERN: Inlinee: outer
; MODERN: VarName: outer_local
; MODERN: Register: PPC_GPR3
; MODERN: LocalVariableAddrRange
; MODERN: InlineSiteSym {
; MODERN: Inlinee: inner
; MODERN: VarName: inner_local
; MODERN: Register: PPC_GPR3
; MODERN: InlineSiteEnd
; MODERN: InlineSiteEnd
; MODERN: Kind: S_PROC_ID_END
; MODERN: BuildInfoSym

; LEGACY: Machine: PPCBE
; LEGACY: Kind: S_GPROC32
; LEGACY: Register: PPC_GPR3
; LEGACY: Kind: S_END

@sample = global i32 0, !dbg !20

define i32 @debug_entry(i32 %value) !dbg !4 {
entry:
  call void @llvm.dbg.value(metadata i32 %value, metadata !14, metadata !DIExpression()), !dbg !15
  store volatile i32 %value, ptr @sample, !dbg !15
  call void @llvm.dbg.value(metadata i32 %value, metadata !16, metadata !DIExpression()), !dbg !17
  store volatile i32 %value, ptr @sample, !dbg !17
  %result = load volatile i32, ptr @sample, !dbg !18
  ret i32 %result, !dbg !18
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !19}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !1, producer: "clang", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, globals: !21)
!1 = !DIFile(filename: "modern.cpp", directory: "/src")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 2, !"CodeView", i32 1}
!4 = distinct !DISubprogram(name: "debug_entry", scope: !1, file: !1, line: 30, type: !5, scopeLine: 30, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!5 = !DISubroutineType(types: !6)
!6 = !{!7, !7}
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!8 = distinct !DISubprogram(name: "outer", scope: !1, file: !1, line: 20, type: !5, scopeLine: 20, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!9 = distinct !DISubprogram(name: "inner", scope: !1, file: !1, line: 10, type: !5, scopeLine: 10, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!10 = !DILocation(line: 31, column: 3, scope: !4)
!11 = !DILocation(line: 22, column: 3, scope: !8, inlinedAt: !10)
!14 = !DILocalVariable(name: "outer_local", scope: !8, file: !1, line: 21, type: !7)
!15 = !DILocation(line: 21, column: 3, scope: !8, inlinedAt: !10)
!16 = !DILocalVariable(name: "inner_local", scope: !9, file: !1, line: 11, type: !7)
!17 = !DILocation(line: 11, column: 3, scope: !9, inlinedAt: !11)
!18 = !DILocation(line: 32, column: 3, scope: !4)
!19 = !{i32 7, !"CodeViewXbox360Modern", i32 1}
!20 = !DIGlobalVariableExpression(var: !22, expr: !DIExpression())
!21 = !{!20}
!22 = distinct !DIGlobalVariable(name: "sample", scope: !0, file: !1, line: 5, type: !23, isLocal: false, isDefinition: true)
!23 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Sample", file: !1, line: 3, size: 32, elements: !24, identifier: ".?AUSample@@")
!24 = !{!25}
!25 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !23, file: !1, line: 4, baseType: !7, size: 32)

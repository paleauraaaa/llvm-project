// REQUIRES: powerpc-registered-target
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O1 -g -gcodeview -gcodeview-xbox360-modern -c %s -o %t.modern.obj
// RUN: llvm-readobj --codeview %t.modern.obj | FileCheck %s --check-prefix=MODERN
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O1 -g -gcodeview -c %s -o %t.legacy.obj
// RUN: llvm-readobj --codeview %t.legacy.obj | FileCheck %s --check-prefix=LEGACY --implicit-check-not=LF_FUNC_ID --implicit-check-not=LF_STRING_ID --implicit-check-not=LF_BUILDINFO --implicit-check-not=LF_UDT_SRC_LINE --implicit-check-not=InlineSiteSym --implicit-check-not=BuildInfoSym
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O1 -g -gcodeview -gcodeview-xbox360-modern -gno-codeview-command-line -gno-inline-line-tables -c %s -o %t.controls.obj
// RUN: llvm-readobj --codeview %t.controls.obj | FileCheck %s --check-prefix=CONTROLS --implicit-check-not=InlineSiteSym --implicit-check-not='"-cc1"'

#line 1 "xbox360-modern.cpp"
struct Sample {
  int value;
};
volatile Sample sample = {0};

__attribute__((always_inline)) inline int inner(int value) {
  int inner_local = value + 2;
  sample.value = inner_local;
  return inner_local;
}

__attribute__((always_inline)) inline int outer(int value) {
  int outer_local = value + 1;
  sample.value = outer_local;
  return inner(outer_local);
}

extern "C" int debug_entry(int value) { return outer(value); }

// MODERN-DAG: TypeLeafKind: LF_UDT_SRC_LINE
// MODERN-DAG: UDT: Sample
// MODERN-DAG: SourceFile: {{.*}}xbox360-modern.cpp
// MODERN-DAG: LineNumber: 1
// MODERN-DAG: TypeLeafKind: LF_BUILDINFO
// MODERN-DAG: StringData: {{.*}}-cc1
// MODERN: Machine: PPCBE
// MODERN: SubSectionType: InlineeLines
// MODERN: Kind: S_GPROC32_ID
// MODERN: InlineSiteSym {
// MODERN: Inlinee: outer
// MODERN: VarName: outer_local
// MODERN: InlineSiteSym {
// MODERN: Inlinee: inner
// MODERN: VarName: inner_local
// MODERN: InlineSiteEnd
// MODERN: InlineSiteEnd
// MODERN: BuildInfoSym

// LEGACY: Machine: PPCBE
// LEGACY: Kind: S_GPROC32
// LEGACY: Kind: S_END

// CONTROLS-DAG: TypeLeafKind: LF_UDT_SRC_LINE
// CONTROLS-DAG: TypeLeafKind: LF_BUILDINFO
// CONTROLS: Kind: S_GPROC32_ID
// CONTROLS: BuildInfoSym

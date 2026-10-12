// REQUIRES: powerpc-registered-target
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -g -gcodeview -c %s -o %t.obj
// RUN: llvm-readobj --sections --codeview %t.obj | FileCheck %s

int debug_function(int value) { return value + 1; }

// CHECK: Name: .debug$S
// CHECK: Name: .debug$T
// CHECK: Machine: PPC604
// CHECK: GlobalProcSym {
// CHECK: FunctionType: int (int)
// CHECK: Kind: S_END

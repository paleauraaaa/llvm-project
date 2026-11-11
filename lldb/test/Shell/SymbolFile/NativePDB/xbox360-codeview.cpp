// REQUIRES: lld, powerpc
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -g -gcodeview -O1 -c %s -o %t.obj
// RUN: lld-link /machine:ppcbe /subsystem:xbox /entry:debug_function /nodefaultlib /debug:full /out:%t.exe /pdb:%t.pdb %t.obj
// RUN: %lldb -b -f %t.exe -o "b debug_function" -o "image lookup -a 0x82001000 -v" -o "image dump symfile" -o quit | FileCheck %s
// RUN: llvm-pdbutil pdb2yaml -dbi-stream %t.pdb | FileCheck %s --check-prefix=MACHINE
// RUN: llvm-pdbutil dump -summary %t.pdb | FileCheck %s --check-prefix=STREAM
// RUN: lldb-test object-file %t.pdb | FileCheck %s --check-prefix=OBJECT
// RUN: %lldb -b -f %t.pdb -o "image list -t" -o quit | FileCheck %s --check-prefix=STANDALONE
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -g -gcodeview -O0 -c %s -o %t.stack.obj
// RUN: lld-link /machine:ppcbe /subsystem:xbox /entry:debug_function /nodefaultlib /debug:full /out:%t.stack.exe /pdb:%t.stack.pdb %t.stack.obj
// RUN: %lldb -b -f %t.stack.exe -o "b debug_function" -o "image lookup -a 0x82001000 -v" -o "image dump symfile" -o quit | FileCheck %s --check-prefix=STACK

typedef unsigned int Vector __attribute__((vector_size(16)));
struct Pair {
  int count;
  double scale;
};
struct Pair global = {7, 1.5};

extern "C" int debug_function(int value, double scale, Vector lanes) {
  volatile int local = value;
  return local + (int)scale + lanes[0] + global.count;
}

// CHECK: Breakpoint 1: where = {{.*}}`debug_function{{.*}} at xbox360-codeview.cpp:
// CHECK: FuncType: {{.*}}compiler_type = "int (int, double, {{.*}})"
// CHECK: name = "value", type = "int"
// CHECK-SAME: DW_OP_reg3
// CHECK: name = "scale", type = "double"
// CHECK-SAME: DW_OP_regx F1
// CHECK: name = "lanes"
// CHECK-SAME: DW_OP_regx VF1
// CHECK: SymbolFile native-pdb
// CHECK: name = "local"
// CHECK: DW_OP_breg1 {{.*}}DW_OP_constu 0xffffffff, DW_OP_and
// MACHINE: MachineType: PowerPCBE
// STREAM: Has IDs: false
// OBJECT: Plugin name: pdb
// OBJECT: Architecture: powerpc-ibm-xbox360
// STANDALONE: powerpc-ibm-xbox360
// STACK: Breakpoint 1: where = {{.*}}`debug_function
// STACK: SymbolFile native-pdb
// STACK: name = "value"
// STACK: DW_OP_breg1 {{.*}}DW_OP_constu 0xffffffff, DW_OP_and

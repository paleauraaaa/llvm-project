// RUN: llvm-tblgen -gen-asm-matcher -I %p/../../include %s | FileCheck %s

include "llvm/Target/Target.td"

def ArchInstrInfo : InstrInfo;
def Arch : Target {
  let InstructionSet = ArchInstrInfo;
}

def Reg : Register<"reg">;
def RegClass : RegisterClass<"Arch", [i32], 32, (add Reg)>;

class TiedInst<string Asm> : Instruction {
  let Namespace = "Arch";
  let Size = 4;
  let OutOperandList = (outs RegClass:$dst);
  let InOperandList = (ins RegClass:$src, RegClass:$a);
  let Constraints = "$src = $dst";
  let AsmString = Asm;
}

def Explicit : TiedInst<"explicit $dst, $a, $src">;
def Implicit : TiedInst<"implicit $dst, $a">;

// Both forms have a tied MC operand, but only the explicit form has a
// separately spelled source that needs its own parsed-to-MC mapping.
// CHECK: /* Tie0_1_1 */ { 0, 1, 1 },
// CHECK: /* Tie0_1_3 */ { 0, 1, 3 },
// CHECK: convertToMapAndConstraints(
// CHECK: case CVT_Tied:
// CHECK-NEXT: if (TiedAsmOperandTable[*(p + 1)][1] !=
// CHECK-NEXT: TiedAsmOperandTable[*(p + 1)][2])
// CHECK-NEXT: Operands[TiedAsmOperandTable[*(p + 1)][2]]->
// CHECK-NEXT: setMCOperandNum(NumMCOperands);
// CHECK-NEXT: ++NumMCOperands;

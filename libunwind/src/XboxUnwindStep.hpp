//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef __XBOXUNWINDSTEP_HPP__
#define __XBOXUNWINDSTEP_HPP__

#include "XboxSEHCallFrame.h"
#include "XboxUnwind.hpp"
#include "libunwind.h"
#include "libunwind_ext.h"

namespace libunwind {

namespace XboxFrame {
constexpr unsigned GPRCount = 32;
constexpr unsigned FirstNonvolatile = 14;
constexpr unsigned SP = 1;
constexpr unsigned FP = 31;
constexpr unsigned FirstVector = 64;
constexpr unsigned VectorCount = 128;
constexpr unsigned StackAlignment = 16;
constexpr unsigned MaxHelperInstructions = 128;
constexpr unsigned MaxEpilogueBytes = 512;
constexpr unsigned MaxBodyInstructions = 4096;
constexpr unsigned WordSize = 4;
constexpr unsigned DoublewordSize = 8;
constexpr unsigned VectorSize = 16;
constexpr unsigned FirstNonvolatileCRField = 2;
constexpr unsigned LastNonvolatileCRField = 5;
constexpr unsigned CRHighFieldMask = 0x80;
constexpr unsigned CRInstructionMaskShift = 12;

static_assert(XBOX_SEH_CALL_SCOPE_OFFSET + WordSize <=
                  XBOX_SEH_CALL_FRAME_SIZE - 12,
              "Xbox SEH state must not overlap the saved CR or LR");
static_assert(XboxPdata::PowerPCInstructions ==
                  UNW_INFO_FORMAT_XBOX_PDATA_POWERPC,
              "Xbox .pdata format flags must agree");

namespace Opcode {
enum : unsigned {
  VMX128 = 4,
  Mulli = 7,
  Subfic = 8,
  CmplImmediate = 10,
  CmpImmediate = 11,
  Addic = 12,
  AddicRecord = 13,
  Addi = 14,
  Addis = 15,
  ConditionalBranch = 16,
  SystemCall = 17,
  Branch = 18,
  BranchExtended = 19,
  Rlwimi = 20,
  Rlwinm = 21,
  Rlwnm = 23,
  Ori = 24,
  Oris = 25,
  Xori = 26,
  Xoris = 27,
  Andi = 28,
  Andis = 29,
  RotateDoubleword = 30,
  Extended = 31,
  Lwz = 32,
  Lwzu = 33,
  Lbz = 34,
  Lbzu = 35,
  Stw = 36,
  Stwu = 37,
  Stb = 38,
  Stbu = 39,
  Lhz = 40,
  Lhzu = 41,
  Lha = 42,
  Lhau = 43,
  Sth = 44,
  Sthu = 45,
  Lmw = 46,
  Stmw = 47,
  Lfs = 48,
  Lfsu = 49,
  Lfd = 50,
  Lfdu = 51,
  Stfs = 52,
  Stfsu = 53,
  Stfd = 54,
  Stfdu = 55,
  Ld = 58,
  FloatingSingle = 59,
  Std = 62,
  Floating = 63,
};
}

namespace ExtendedOpcode {
enum : unsigned {
  Cmp = 0,
  Ldx = 21,
  Lwzx = 23,
  And = 28,
  Cmpl = 32,
  Subf = 40,
  Andc = 60,
  Lbzx = 87,
  Lvx = 103,
  Nor = 124,
  Mtcrf = 144,
  Stwux = 183,
  Stvx = 231,
  Add = 266,
  Xor = 316,
  Lvxl = 359,
  Or = 444,
  Mtspr = 467,
  Nand = 476,
  Stvxl = 487,
};
}

struct Instruction {
  static constexpr uint32_t IgnoreRTMask = 0xfc1fffff;
  static constexpr uint32_t Mflr = 0x7c0802a6;
  static constexpr uint32_t Mfcr = 0x7c000026;
  static constexpr uint32_t Mtlr = 0x7c0803a6;
  static constexpr uint32_t Blr = 0x4e800020;
  static constexpr uint32_t Bctr = 0x4e800420;
  static constexpr uint32_t VMXMemoryMask = 0x7f3;
  static constexpr uint32_t VMXRegisterHighMask = 0xc;
  static constexpr uint32_t Stvx128 = 0x1c3;
  static constexpr uint32_t Lvx128 = 0xc3;
  static constexpr uint32_t BranchDisplacementMask = 0x03fffffc;
  static constexpr uint32_t BranchDisplacementSign = 0x02000000;
  static constexpr uint32_t BranchDisplacementRange = 0x04000000;
  static constexpr uint32_t BranchAbsolute = 2;
  static constexpr uint32_t BranchLink = 1;
  uint32_t bits;
  unsigned op() const { return bits >> 26; }
  unsigned rt() const { return (bits >> 21) & 31; }
  unsigned ra() const { return (bits >> 16) & 31; }
  unsigned rb() const { return (bits >> 11) & 31; }
  unsigned xo() const { return (bits >> 1) & 1023; }
  int32_t imm() const { return int16_t(bits); }
  int32_t ds() const { return int16_t(bits & ~3u); }
  bool plainDS() const { return !(bits & 3); }
  bool mflr() const { return (bits & IgnoreRTMask) == Mflr; }
  bool mfcr() const { return (bits & IgnoreRTMask) == Mfcr; }
  bool mtlr() const { return (bits & IgnoreRTMask) == Mtlr; }
  bool blr() const { return bits == Blr; }
  bool bctr() const { return bits == Bctr; }
  bool linkedBranch() const {
    return op() == Opcode::Branch && (bits & BranchLink);
  }
  bool vectorStore() const {
    return (op() == Opcode::Extended &&
            (xo() == ExtendedOpcode::Stvx || xo() == ExtendedOpcode::Stvxl)) ||
           (op() == Opcode::VMX128 && (bits & VMXMemoryMask) == Stvx128);
  }
  bool vectorLoad() const {
    return (op() == Opcode::Extended &&
            (xo() == ExtendedOpcode::Lvx || xo() == ExtendedOpcode::Lvxl)) ||
           (op() == Opcode::VMX128 && (bits & VMXMemoryMask) == Lvx128);
  }
  unsigned vector() const {
    return rt() |
           (op() == Opcode::VMX128 ? (bits & VMXRegisterHighMask) << 3 : 0);
  }
  int64_t branchTarget(uint32_t address) const {
    int32_t disp = int32_t(bits & BranchDisplacementMask);
    if (disp & BranchDisplacementSign)
      disp -= BranchDisplacementRange;
    return (bits & BranchAbsolute) ? disp : int64_t(address) + disp;
  }
};

struct Value {
  enum Kind { Unknown, Constant, Stack, Register, CR } kind;
  int64_t value;
};

struct Save {
  int32_t offset;
  bool valid;
};

inline bool fitsOffset(int64_t value) {
  return value >= INT32_MIN && value <= INT32_MAX;
}

inline bool addOffset(uint32_t base, int32_t offset, uint32_t &result) {
  if (offset >= 0) {
    uint32_t amount = uint32_t(offset);
    if (base > UINT32_MAX - amount)
      return false;
    result = base + amount;
  } else {
    uint32_t amount = uint32_t(-int64_t(offset));
    if (base < amount)
      return false;
    result = base - amount;
  }
  return true;
}

template <typename A>
bool getCallParent(A &as, uint64_t trampolineSP, uint32_t target, uint32_t kind,
                   uint32_t &parent) {
  if (!trampolineSP || trampolineSP > UINT32_MAX -
                                          XBOX_SEH_CALL_CLEANUP_OFFSET -
                                          (WordSize - 1) ||
      trampolineSP % StackAlignment)
    return false;
  uint32_t sp = uint32_t(trampolineSP);
  if (as.get32(sp + XBOX_SEH_CALL_CLEANUP_OFFSET) != kind ||
      as.get32(sp + XBOX_SEH_CALL_TARGET_OFFSET) != target)
    return false;
  uint32_t value = as.get32(sp + XBOX_SEH_CALL_PARENT_OFFSET);
  if (!value || value % StackAlignment)
    return false;
  parent = value;
  return true;
}

template <typename A>
bool getCleanupParent(A &as, uint64_t trampolineSP, uint32_t cleanup,
                      uint32_t &parent) {
  return getCallParent(as, trampolineSP, cleanup, XBOX_SEH_CALL_CLEANUP,
                       parent);
}

struct CleanupState {
  uint32_t exception, cursor, scopeIndex;
  bool abandoned;
};

template <typename A>
bool getCallState(A &as, uint64_t trampolineSP, uint32_t target, uint32_t kind,
                  uint32_t &parent, CleanupState &state) {
  uint32_t value;
  if (trampolineSP > UINT32_MAX - XBOX_SEH_CALL_SCOPE_OFFSET - (WordSize - 1) ||
      !getCallParent(as, trampolineSP, target, kind, value))
    return false;
  uint32_t sp = uint32_t(trampolineSP);
  uint32_t taggedException = as.get32(sp + XBOX_SEH_CALL_EXCEPTION_OFFSET);
  CleanupState saved = {taggedException & ~XBOX_SEH_EXCEPTION_ABANDONED,
                        as.get32(sp + XBOX_SEH_CALL_CURSOR_OFFSET),
                        as.get32(sp + XBOX_SEH_CALL_SCOPE_OFFSET),
                        bool(taggedException & XBOX_SEH_EXCEPTION_ABANDONED)};
  if (!saved.exception || saved.exception % WordSize || !saved.cursor ||
      saved.cursor % WordSize)
    return false;
  parent = value;
  state = saved;
  return true;
}

template <typename A>
bool getCleanupState(A &as, uint64_t trampolineSP, uint32_t cleanup,
                     uint32_t &parent, CleanupState &state) {
  return getCallState(as, trampolineSP, cleanup, XBOX_SEH_CALL_CLEANUP, parent,
                      state);
}

template <typename A, typename R> class Interpreter {
public:
  Interpreter(A &as, R &regs, const unw_proc_info_t &info, uint32_t imageBase,
              uint32_t imageSize)
      : as(as), regs(regs), info(info), imageBase(imageBase),
        imageSize(imageSize) {
    for (unsigned i = 0; i != GPRCount; ++i)
      values[i] = {Value::Register, i};
    values[SP] = {Value::Stack, 0};
  }

  int step(bool requireEstablished = false,
           const uint32_t *parentFrameSize = nullptr);

private:
  A &as;
  R &regs;
  const unw_proc_info_t &info;
  uint32_t imageBase;
  uint32_t imageSize;
  Value values[GPRCount];
  Save gprSaves[GPRCount] = {};
  Save fprSaves[GPRCount] = {};
  Save vectorSaves[VectorCount - FirstVector] = {};
  Save lrSave = {};
  Save crSave = {};
  bool lrLive = true;
  unsigned crSavedFields = 0;

  bool save(Value value, int64_t offset, unsigned size);
  bool executePrologue(Instruction ins);
  bool executeHelper(uint32_t callAddress, Instruction call);
  bool executeEpilogue(Instruction ins);
  bool restore(uint32_t cfa);
  static bool isEpilogueInstruction(Instruction ins);
  static bool mayWriteRegister(Instruction ins, unsigned reg);
  bool isTerminal(Instruction ins, uint32_t address) const;
};

template <typename A, typename R>
bool Interpreter<A, R>::save(Value value, int64_t offset, unsigned size) {
  if (!fitsOffset(offset))
    return false;
  Save *slot = nullptr;
  if (value.kind == Value::CR && size == WordSize) {
    slot = &crSave;
    crSavedFields = 0;
    for (unsigned field = FirstNonvolatileCRField;
         field <= LastNonvolatileCRField; ++field)
      crSavedFields |= CRHighFieldMask >> field;
  } else if (value.kind == Value::Register) {
    unsigned reg = unsigned(value.value);
    if (reg == UNW_PPC64_LR && size == WordSize)
      slot = &lrSave;
    else if (reg >= FirstNonvolatile && reg < GPRCount &&
             size == DoublewordSize)
      slot = &gprSaves[reg];
    else if (reg >= UNW_PPC64_F14 && reg <= UNW_PPC64_F31 &&
             size == DoublewordSize)
      slot = &fprSaves[reg - UNW_PPC64_F0];
    else if (reg >= UNW_PPC64_V32 + (FirstVector - 32) &&
             reg <= UNW_PPC64_V127 && size == VectorSize)
      slot = &vectorSaves[reg - UNW_PPC64_V32 - (FirstVector - 32)];
    else if (reg == UNW_PPC64_LR ||
             (reg >= FirstNonvolatile && reg < GPRCount) ||
             (reg >= UNW_PPC64_F14 && reg <= UNW_PPC64_F31) ||
             reg >= UNW_PPC64_V32 + (FirstVector - 32))
      return false;
  }
  if (slot)
    *slot = {int32_t(offset), true};
  return true;
}

template <typename A, typename R>
bool Interpreter<A, R>::executePrologue(Instruction ins) {
  unsigned op = ins.op(), rt = ins.rt(), ra = ins.ra(), rb = ins.rb();
  int64_t imm = ins.imm();
  if (op == Opcode::Addi || op == Opcode::Addis) {
    Value value = ra ? values[ra] : Value{Value::Constant, 0};
    if (value.kind != Value::Constant && value.kind != Value::Stack)
      value = {Value::Unknown, 0};
    else
      value.value += imm * (op == Opcode::Addis ? 65536 : 1);
    if (!fitsOffset(value.value))
      value = {Value::Unknown, 0};
    values[rt] = value;
    return rt != SP;
  }
  if (op == Opcode::Ori || op == Opcode::Oris) {
    Value value = values[rt];
    unsigned immediate = uint16_t(ins.bits);
    if (value.kind == Value::Constant)
      value.value |= int64_t(immediate) << (op == Opcode::Oris ? 16 : 0);
    else if (immediate)
      value = {Value::Unknown, 0};
    values[ra] = value;
    return ra != SP;
  }
  if (op == Opcode::Extended && ins.xo() == ExtendedOpcode::Or) {
    values[ra] = rt == rb ? values[rt] : Value{Value::Unknown, 0};
    return ra != SP;
  }
  if (op == Opcode::Rlwinm) {
    values[ra] = {Value::Unknown, 0};
    return ra != SP;
  }
  if (op == Opcode::Extended && ins.xo() == ExtendedOpcode::Add) {
    Value a = values[ra], b = values[rb];
    if (a.kind == Value::Constant) {
      Value tmp = a;
      a = b;
      b = tmp;
    }
    values[rt] = {Value::Unknown, 0};
    if ((a.kind == Value::Constant || a.kind == Value::Stack) &&
        b.kind == Value::Constant && fitsOffset(a.value + b.value))
      values[rt] = {a.kind, a.value + b.value};
    return rt != SP;
  }
  if (ins.mflr()) {
    values[rt] = lrLive ? Value{Value::Register, UNW_PPC64_LR}
                        : Value{Value::Unknown, 0};
    return rt != SP;
  }
  if (ins.mfcr()) {
    values[rt] = {Value::CR, 0};
    return rt != SP;
  }
  if (op == Opcode::Stwu ||
      (op == Opcode::Extended && ins.xo() == ExtendedOpcode::Stwux)) {
    if (rt != SP || ra != SP || values[SP].kind != Value::Stack)
      return false;
    if (op == Opcode::Extended) {
      if (values[rb].kind != Value::Constant)
        return false;
      imm = values[rb].value;
    }
    if (imm >= 0 || imm % StackAlignment || !fitsOffset(values[SP].value + imm))
      return false;
    values[SP].value += imm;
    return true;
  }
  if (op == Opcode::Stw || op == Opcode::Stfd ||
      (op == Opcode::Std && ins.plainDS())) {
    if (!ra || values[ra].kind != Value::Stack)
      return false;
    if (op == Opcode::Std)
      imm = ins.ds();
    Value value = op == Opcode::Stfd ? Value{Value::Register, UNW_PPC64_F0 + rt}
                                     : values[rt];
    return save(value, values[ra].value + imm,
                op == Opcode::Stw ? WordSize : DoublewordSize);
  }
  if (ins.vectorStore()) {
    Value base = ra ? values[ra] : Value{Value::Constant, 0};
    Value index = values[rb];
    if (base.kind == Value::Constant) {
      Value tmp = base;
      base = index;
      index = tmp;
    }
    if (base.kind != Value::Stack || index.kind != Value::Constant)
      return false;
    unsigned v = ins.vector();
    if (v < FirstVector)
      return true;
    return save({Value::Register, UNW_PPC64_V32 + v - 32},
                base.value + index.value, VectorSize);
  }
  if (op == Opcode::Ld && ins.plainDS()) {
    values[rt] = {Value::Unknown, 0};
    return rt != SP;
  }
  return false;
}

template <typename A, typename R>
bool Interpreter<A, R>::executeHelper(uint32_t address, Instruction call) {
  int64_t target = call.branchTarget(address);
  lrLive = false;
  for (unsigned i = 0; i != MaxHelperInstructions; ++i, target += 4) {
    if (target < 0 || target > UINT32_MAX - 3 || (target & 3))
      return false;
    if (!as.isXboxCodeRange(uint32_t(target), XboxPdata::InstructionSize,
                            imageBase, imageSize))
      return false;
    Instruction ins{as.get32(uint32_t(target))};
    if (ins.blr())
      return true;
    if (!executePrologue(ins))
      return false;
  }
  return false;
}

template <typename A, typename R>
bool Interpreter<A, R>::isEpilogueInstruction(Instruction ins) {
  unsigned op = ins.op();
  return op == Opcode::Addi || op == Opcode::Addis || op == Opcode::Ori ||
         op == Opcode::Oris || op == Opcode::Lwz || op == Opcode::Lfd ||
         (op == Opcode::Ld && ins.plainDS()) ||
         (op == Opcode::Extended && (ins.xo() == ExtendedOpcode::Or ||
                                     ins.xo() == ExtendedOpcode::Mtspr ||
                                     ins.xo() == ExtendedOpcode::Mtcrf)) ||
         ins.vectorLoad();
}

template <typename A, typename R>
bool Interpreter<A, R>::mayWriteRegister(Instruction ins, unsigned reg) {
  unsigned op = ins.op(), xo = ins.xo();
  if (op == Opcode::Lwzu || op == Opcode::Lbzu || op == Opcode::Lhzu ||
      op == Opcode::Lhau || (op == Opcode::Ld && (ins.bits & 3) == 1))
    return ins.rt() == reg || ins.ra() == reg;
  if (op == Opcode::Lmw)
    return ins.rt() <= reg;
  if (op == Opcode::Mulli || op == Opcode::Subfic ||
      (op >= Opcode::Addic && op <= Opcode::Addis) ||
      (op >= Opcode::Lwz && op <= Opcode::Lbzu) ||
      (op >= Opcode::Lhz && op <= Opcode::Lhau) || op == Opcode::Ld)
    return ins.rt() == reg;
  if ((op >= Opcode::Rlwimi && op <= Opcode::Rlwinm) || op == Opcode::Rlwnm ||
      (op >= Opcode::Ori && op <= Opcode::RotateDoubleword))
    return ins.ra() == reg;
  if (op == Opcode::Stwu || op == Opcode::Stbu || op == Opcode::Sthu ||
      op == Opcode::Lfsu || op == Opcode::Lfdu || op == Opcode::Stfsu ||
      op == Opcode::Stfdu || (op == Opcode::Std && !ins.plainDS()))
    return ins.ra() == reg;
  if (op != Opcode::Extended) {
    if (op == Opcode::VMX128 || op == Opcode::CmplImmediate ||
        op == Opcode::CmpImmediate ||
        (op >= Opcode::ConditionalBranch && op <= Opcode::BranchExtended) ||
        op == Opcode::Stw || op == Opcode::Stb || op == Opcode::Sth ||
        op == Opcode::Stmw || op == Opcode::Lfs || op == Opcode::Lfd ||
        op == Opcode::Stfs || op == Opcode::Stfd || op == Opcode::Std ||
        op == Opcode::FloatingSingle || op == Opcode::Floating)
      return false;
    return ins.rt() == reg || ins.ra() == reg;
  }
  if (ins.vectorLoad() || ins.vectorStore())
    return false;
  if (xo == ExtendedOpcode::Stwux)
    return ins.ra() == reg;
  if (xo == ExtendedOpcode::Or || xo == ExtendedOpcode::And ||
      xo == ExtendedOpcode::Andc || xo == ExtendedOpcode::Nor ||
      xo == ExtendedOpcode::Xor || xo == ExtendedOpcode::Nand)
    return ins.ra() == reg;
  if (xo == ExtendedOpcode::Mtspr || xo == ExtendedOpcode::Mtcrf ||
      xo == ExtendedOpcode::Cmp || xo == ExtendedOpcode::Cmpl)
    return false;
  if (xo == ExtendedOpcode::Lwzx || xo == ExtendedOpcode::Ldx ||
      xo == ExtendedOpcode::Lbzx || xo == ExtendedOpcode::Add ||
      xo == ExtendedOpcode::Subf)
    return ins.rt() == reg;
  return ins.rt() == reg || ins.ra() == reg;
}

template <typename A, typename R>
bool Interpreter<A, R>::isTerminal(Instruction ins, uint32_t address) const {
  if (ins.blr() || ins.bctr())
    return true;
  if (ins.op() != Opcode::Branch || (ins.bits & Instruction::BranchLink))
    return false;
  int64_t target = ins.branchTarget(address);
  return target < int64_t(info.start_ip) || target >= int64_t(info.end_ip);
}

template <typename A, typename R>
bool Interpreter<A, R>::executeEpilogue(Instruction ins) {
  unsigned op = ins.op(), rt = ins.rt(), ra = ins.ra(), rb = ins.rb();
  int64_t imm = ins.imm();
  if (op == Opcode::Addi && rt == SP && ra == SP) {
    if (values[SP].kind != Value::Stack || !fitsOffset(values[SP].value + imm))
      return false;
    values[SP].value += imm;
    return true;
  }
  if (op == Opcode::Extended && ins.xo() == ExtendedOpcode::Or && ra == SP &&
      rt == rb) {
    if (values[rt].kind != Value::Stack)
      return false;
    values[SP] = values[rt];
    return true;
  }
  if (op == Opcode::Lwz && rt == SP && ra == SP && imm == 0) {
    values[SP] = {Value::Stack, 0};
    return true;
  }
  if (op == Opcode::Lwz || op == Opcode::Lfd ||
      (op == Opcode::Ld && ins.plainDS()) || ins.vectorLoad()) {
    bool vector = ins.vectorLoad();
    unsigned size = vector              ? VectorSize
                    : op == Opcode::Lwz ? WordSize
                                        : DoublewordSize;
    Value base = ra ? values[ra] : Value{Value::Constant, 0};
    if (vector) {
      Value index = values[rb];
      if (base.kind == Value::Constant) {
        Value tmp = base;
        base = index;
        index = tmp;
      }
      if (index.kind != Value::Constant)
        return false;
      imm = index.value;
    } else if (op == Opcode::Ld) {
      imm = ins.ds();
    }
    if (base.kind != Value::Stack)
      return false;
    int64_t offset = base.value + imm;
    if (op == Opcode::Lwz || op == Opcode::Ld)
      values[rt] = {Value::Unknown, 0};
    if (op == Opcode::Lwz && lrSave.valid && lrSave.offset == offset) {
      values[rt] = {Value::Register, UNW_PPC64_LR};
    } else if (op == Opcode::Lwz && crSave.valid && crSave.offset == offset) {
      values[rt] = {Value::CR, 0};
    } else if (op == Opcode::Ld && rt >= FirstNonvolatile &&
               gprSaves[rt].valid && gprSaves[rt].offset == offset &&
               size == DoublewordSize) {
      gprSaves[rt].valid = false;
      values[rt] = {Value::Register, rt};
    } else if (op == Opcode::Lfd && rt >= FirstNonvolatile &&
               fprSaves[rt].valid && fprSaves[rt].offset == offset) {
      fprSaves[rt].valid = false;
    } else if (vector && ins.vector() >= FirstVector &&
               vectorSaves[ins.vector() - FirstVector].valid &&
               vectorSaves[ins.vector() - FirstVector].offset == offset) {
      vectorSaves[ins.vector() - FirstVector].valid = false;
    }
    return vector || op == Opcode::Lfd || rt != SP;
  }
  if (ins.mtlr()) {
    if (values[rt].kind != Value::Register || values[rt].value != UNW_PPC64_LR)
      return false;
    lrLive = true;
    lrSave.valid = false;
    return true;
  }
  if (op == Opcode::Extended && ins.xo() == ExtendedOpcode::Mtcrf) {
    if (values[rt].kind != Value::CR)
      return false;
    unsigned mask = (ins.bits >> CRInstructionMaskShift) & 0xff;
    crSavedFields &= ~mask;
    if (!crSavedFields)
      crSave.valid = false;
    return true;
  }
  return executePrologue(ins);
}

template <typename A, typename R>
bool Interpreter<A, R>::restore(uint32_t cfa) {
  auto address = [cfa](const Save &save, unsigned size, uint32_t &out) {
    if (!addOffset(cfa, save.offset, out) || out > UINT32_MAX - (size - 1) ||
        out % size)
      return false;
    return true;
  };
  uint32_t ignored;
  if ((lrSave.valid && !address(lrSave, WordSize, ignored)) ||
      (crSave.valid && !address(crSave, WordSize, ignored)))
    return false;
  for (unsigned i = FirstNonvolatile; i != GPRCount; ++i)
    if ((gprSaves[i].valid && !address(gprSaves[i], DoublewordSize, ignored)) ||
        (fprSaves[i].valid && !address(fprSaves[i], DoublewordSize, ignored)))
      return false;
  for (const Save &slot : vectorSaves)
    if (slot.valid && !address(slot, VectorSize, ignored))
      return false;

  uint64_t lr = regs.getLR();
  if (lrSave.valid) {
    address(lrSave, WordSize, ignored);
    lr = as.get32(ignored);
  } else if (!lrLive) {
    bool found = false;
    for (unsigned i = 0; i != GPRCount; ++i)
      if (values[i].kind == Value::Register &&
          values[i].value == UNW_PPC64_LR) {
        lr = regs.getRegister(UNW_PPC64_R0 + i);
        found = true;
        break;
      }
    if (!found)
      return false;
  }
  if (lr > UINT32_MAX || (lr & 3) ||
      (lr == regs.getIP() && cfa == regs.getSP()))
    return false;
  uint64_t gprValues[GPRCount - FirstNonvolatile];
  for (unsigned i = FirstNonvolatile; i != GPRCount; ++i) {
    if (gprSaves[i].valid) {
      address(gprSaves[i], DoublewordSize, ignored);
      gprValues[i - FirstNonvolatile] = as.get64(ignored);
      continue;
    }
    bool found = false;
    for (unsigned source = 0; source != GPRCount; ++source)
      if (values[source].kind == Value::Register && values[source].value == i) {
        gprValues[i - FirstNonvolatile] =
            regs.getRegister(UNW_PPC64_R0 + source);
        found = true;
        break;
      }
    if (!found)
      return false;
  }
  for (unsigned i = FirstNonvolatile; i != GPRCount; ++i) {
    regs.setRegister(UNW_PPC64_R0 + i, gprValues[i - FirstNonvolatile]);
    if (fprSaves[i].valid) {
      address(fprSaves[i], DoublewordSize, ignored);
      regs.setFloatRegister(UNW_PPC64_F0 + i, as.getDouble(ignored));
    }
  }
  for (unsigned i = 0; i != VectorCount - FirstVector; ++i) {
    if (vectorSaves[i].valid) {
      address(vectorSaves[i], VectorSize, ignored);
      regs.setVectorRegister(UNW_PPC64_V32 + FirstVector - 32 + i,
                             as.getVector(ignored));
    }
  }
  if (crSave.valid) {
    address(crSave, WordSize, ignored);
    uint32_t cr = as.get32(ignored);
    for (unsigned field = FirstNonvolatileCRField;
         field <= LastNonvolatileCRField; ++field)
      if (crSavedFields & (CRHighFieldMask >> field))
        regs.setRegister(UNW_PPC64_CR0 + field, cr);
  }
  regs.setSP(cfa);
  regs.setLR(lr);
  regs.setIP(lr);
  return true;
}

template <typename A, typename R>
int Interpreter<A, R>::step(bool requireEstablished,
                            const uint32_t *parentFrameSize) {
  uint64_t rawPC = regs.getIP();
  if (rawPC > UINT32_MAX || (rawPC & 3) || rawPC < info.start_ip ||
      rawPC > info.end_ip)
    return UNW_EBADFRAME;
  uint32_t pc = uint32_t(rawPC);
  if (info.end_ip > UINT32_MAX || info.end_ip <= info.start_ip ||
      !as.isXboxCodeRange(uint32_t(info.start_ip),
                          uint32_t(info.end_ip - info.start_ip), imageBase,
                          imageSize))
    return UNW_EBADFRAME;
  uint32_t prologueSize = (info.format & XboxPdata::PrologueLengthMask) *
                          XboxPdata::InstructionSize;
  uint32_t prologueEnd = uint32_t(info.start_ip) + prologueSize;
  if (requireEstablished && pc < prologueEnd)
    return UNW_EBADFRAME;
  uint32_t prologuePC = pc < prologueEnd ? pc : prologueEnd;
  for (uint32_t address = uint32_t(info.start_ip); address < prologuePC;
       address += XboxPdata::InstructionSize) {
    Instruction ins{as.get32(address)};
    if (ins.linkedBranch()) {
      if (!executeHelper(address, ins))
        return UNW_EUNSPEC;
    } else if (!executePrologue(ins)) {
      return UNW_EUNSPEC;
    }
  }
  if (pc >= prologueEnd) {
    for (unsigned i = 0; i != FirstNonvolatile; ++i)
      if (i != SP && i != 2 && i != 13)
        values[i] = {Value::Unknown, 0};
    uint32_t limit = uint32_t(info.end_ip);
    if (limit - pc > MaxEpilogueBytes)
      limit = pc + MaxEpilogueBytes;
    uint32_t bodyEnd = pc;
    for (uint32_t end = pc; end < limit; end += XboxPdata::InstructionSize) {
      Instruction terminal{as.get32(end)};
      if (!isTerminal(terminal, end))
        continue;
      uint32_t begin = end;
      while (begin > prologueEnd && end - begin < MaxEpilogueBytes &&
             isEpilogueInstruction({as.get32(begin - 4)}))
        begin -= 4;
      if (begin <= pc) {
        if (requireEstablished)
          return UNW_EBADFRAME;
        bodyEnd = begin;
        for (uint32_t address = begin; address < pc; address += 4)
          if (!executeEpilogue({as.get32(address)}))
            return UNW_EUNSPEC;
      }
      break;
    }
    if ((bodyEnd - prologueEnd) / XboxPdata::InstructionSize >
        MaxBodyInstructions)
      return UNW_EUNSPEC;
    // Outside a recognized epilogue, a changed stack or frame pointer makes
    // the prologue's symbolic frame layout unreliable.
    // Cleanup funclets use r31 for the parent rather than their own frame.
    bool hasFramePointer = values[FP].kind == Value::Stack;
    for (uint32_t address = prologueEnd; address < bodyEnd;
         address += XboxPdata::InstructionSize) {
      Instruction ins{as.get32(address)};
      if ((!hasFramePointer && mayWriteRegister(ins, SP)) ||
          ((hasFramePointer || (requireEstablished && !parentFrameSize)) &&
           mayWriteRegister(ins, FP)))
        return UNW_EUNSPEC;
    }
  }
  if (parentFrameSize && (values[FP].kind != Value::Stack ||
                          values[FP].value != -int64_t(*parentFrameSize)))
    return UNW_EBADFRAME;
  unsigned base = values[FP].kind == Value::Stack ? FP : SP;
  if (values[base].kind != Value::Stack)
    return UNW_EBADFRAME;
  uint64_t baseValue = regs.getRegister(UNW_PPC64_R0 + base);
  int64_t cfaOffset = -values[base].value;
  uint32_t cfa;
  if (baseValue > UINT32_MAX || !fitsOffset(cfaOffset) ||
      !addOffset(uint32_t(baseValue), int32_t(cfaOffset), cfa) ||
      cfa % StackAlignment || !restore(cfa))
    return UNW_EBADFRAME;
  return UNW_STEP_SUCCESS;
}

} // namespace XboxFrame
} // namespace libunwind

#endif // __XBOXUNWINDSTEP_HPP__

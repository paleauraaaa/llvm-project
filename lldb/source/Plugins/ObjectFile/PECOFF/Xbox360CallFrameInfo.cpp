//===-- Xbox360CallFrameInfo.cpp
//-------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Xbox360CallFrameInfo.h"
#include "ObjectFilePECOFF.h"
#include "lldb/Core/AddressRange.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/MathExtras.h"
#include <algorithm>
#include <array>
#include <map>

using namespace lldb;
using namespace lldb_private;
using namespace llvm::dwarf;

namespace {
constexpr unsigned kInstructionSize = 4;
constexpr unsigned kWordSize = 4;
constexpr unsigned kDoublewordSize = 8;
constexpr unsigned kVectorSize = 16;
constexpr unsigned kStackAlignment = 16;
constexpr unsigned kRuntimeFunctionSize = 8;
constexpr unsigned kMaxSaveHelperInstructions = 128;
constexpr unsigned kMaxEpilogueBytes = 512;

namespace PData {
constexpr unsigned kFunctionLengthShift = 8;
constexpr uint32_t kFunctionLengthMask = 0x3fffff;
constexpr uint32_t kPrologueLengthMask = 0xff;
constexpr uint32_t kPowerPCInstructions = 0x40000000;
} // namespace PData

// Match the compiler DWARF numbering, not the remote register packet layout.
namespace Reg {
constexpr unsigned kGPRCount = 32;
constexpr unsigned kSP = 1;
constexpr unsigned kReserved = 2;
constexpr unsigned kThreadPointer = 13;
constexpr unsigned kFirstNonvolatileGPR = 14;
constexpr unsigned kFP = 31;
constexpr unsigned kFPRBase = 32;
constexpr unsigned kFirstNonvolatileFPR = kFPRBase + 14;
constexpr unsigned kLastFPR = kFPRBase + 31;
constexpr unsigned kLR = 65;
constexpr unsigned kCRBase = 68;
constexpr unsigned kFirstNonvolatileCRField = 2;
constexpr unsigned kLastNonvolatileCRField = 5;
constexpr unsigned kCRFieldCount = 8;
constexpr unsigned kCRFieldBits = 4;
constexpr unsigned kAltivecBase = 77;
constexpr unsigned kAltivecCount = 32;
constexpr unsigned kVMX128Base = 1268;
constexpr unsigned kFirstNonvolatileVector = kVMX128Base + 64;

bool IsNonvolatileCR(unsigned reg) {
  return reg >= kCRBase + kFirstNonvolatileCRField &&
         reg <= kCRBase + kLastNonvolatileCRField;
}

unsigned VectorToDWARF(unsigned vr) {
  return vr < kAltivecCount ? kAltivecBase + vr : kVMX128Base + vr;
}
} // namespace Reg

namespace Opcode {
enum : unsigned {
  VMX128 = 4,
  Addi = 14,
  Addis = 15,
  Branch = 18,
  Rlwinm = 21,
  Ori = 24,
  Oris = 25,
  Extended = 31,
  Lwz = 32,
  Stw = 36,
  Stwu = 37,
  Lfd = 50,
  Stfd = 54,
  Ld = 58,
  Std = 62,
};
} // namespace Opcode

namespace ExtendedOpcode {
enum : unsigned {
  Lvx = 103,
  Mtcrf = 144,
  Stwux = 183,
  Stvx = 231,
  Add = 266,
  Lvxl = 359,
  Or = 444,
  Mtspr = 467,
  Stvxl = 487,
};
} // namespace ExtendedOpcode

/// Fields shared by the instruction forms used in Xbox prologues/epilogues.
struct PPCInstruction {
  uint32_t bits;

  unsigned Opcode() const { return bits >> 26; }
  unsigned RT() const { return (bits >> 21) & 0x1f; }
  unsigned RA() const { return (bits >> 16) & 0x1f; }
  unsigned RB() const { return (bits >> 11) & 0x1f; }
  unsigned ExtendedOpcode() const { return (bits >> 1) & 0x3ff; }
  int16_t Immediate() const { return int16_t(bits); }
  uint16_t UnsignedImmediate() const { return uint16_t(bits); }

  unsigned ImmediateShift() const {
    return Opcode() == Opcode::Addis || Opcode() == Opcode::Oris ? 16 : 0;
  }

  bool IsPlainDSForm() const { return !(bits & kDSOpcodeMask); }
  int16_t DSDisplacement() const { return int16_t(bits & ~kDSOpcodeMask); }

  unsigned VectorRegister() const {
    return RT() |
           (Opcode() == Opcode::VMX128 ? (bits & kVMX128RegisterHighMask) << 3
                                       : 0);
  }
  
  bool IsMflr() const { return (bits & kIgnoreRTMask) == kMflr; }
  bool IsMfcr() const { return (bits & kIgnoreRTMask) == kMfcr; }
  bool IsMtlr() const { return (bits & kIgnoreRTMask) == kMtlr; }
  bool IsBLR() const { return bits == kBLR; }
  bool IsBCTR() const { return bits == kBCTR; }

  bool IsVectorStore() const {
    return (Opcode() == Opcode::Extended &&
            (ExtendedOpcode() == ExtendedOpcode::Stvx ||
             ExtendedOpcode() == ExtendedOpcode::Stvxl)) ||
           (Opcode() == Opcode::VMX128 &&
            (bits & kVMX128MemoryOpcodeMask) == kStvx128);
  }

  bool IsVectorLoad() const {
    return (Opcode() == Opcode::Extended &&
            (ExtendedOpcode() == ExtendedOpcode::Lvx ||
             ExtendedOpcode() == ExtendedOpcode::Lvxl)) ||
           (Opcode() == Opcode::VMX128 &&
            (bits & kVMX128MemoryOpcodeMask) == kLvx128);
  }

  bool HasLink() const { return bits & kBranchLink; }

  int64_t BranchTarget(uint32_t address) const {
    int64_t target = llvm::SignExtend64<kBranchDisplacementBits>(
        bits & kBranchDisplacementMask);
    if (!(bits & kBranchAbsolute))
      target += uint64_t(address);
    return target;
  }

  bool WritesCRField(unsigned field) const {
    return bits & (1u << (kCRMaskHighBit - field));
  }

private:
  static constexpr uint32_t kDSOpcodeMask = 0x3;
  static constexpr uint32_t kIgnoreRTMask = 0xfc1fffff;
  static constexpr uint32_t kMflr = 0x7c0802a6;
  static constexpr uint32_t kMfcr = 0x7c000026;
  static constexpr uint32_t kMtlr = 0x7c0803a6;
  static constexpr uint32_t kBLR = 0x4e800020;
  static constexpr uint32_t kBCTR = 0x4e800420;
  static constexpr uint32_t kVMX128MemoryOpcodeMask = 0x7f3;
  static constexpr uint32_t kVMX128RegisterHighMask = 0xc;
  static constexpr uint32_t kStvx128 = 0x1c3;
  static constexpr uint32_t kLvx128 = 0xc3;
  static constexpr uint32_t kBranchLink = 0x1;
  static constexpr uint32_t kBranchAbsolute = 0x2;
  static constexpr uint32_t kBranchDisplacementMask = 0x03fffffc;
  static constexpr unsigned kBranchDisplacementBits = 26;
  static constexpr unsigned kCRMaskHighBit = 19;
};

bool Fail(llvm::StringRef message) {
  LLDB_LOG(GetLog(LLDBLog::Unwind), "Xbox 360 unwind: {0}", message);
  return false;
}

void AppendSigned(std::vector<uint8_t> &bytes, int64_t value) {
  uint8_t encoded[llvm::divideCeil(64, 7)];
  unsigned size = llvm::encodeSLEB128(value, encoded);
  bytes.insert(bytes.end(), encoded, encoded + size);
}

std::vector<uint8_t> SavedWord(int32_t offset) {
  // RegisterContextUnwind supplies CFA as the initial expression stack value.
  std::vector<uint8_t> expression{DW_OP_consts};
  AppendSigned(expression, offset);
  expression.insert(expression.end(),
                    {DW_OP_plus, DW_OP_deref_size, kWordSize});
  return expression;
}

struct PrologueValue {
  enum Kind { Unknown, Constant, Stack, Register, CR } kind = Unknown;
  int64_t value = 0;
};

struct Save {
  int32_t offset;
  unsigned size;
  // A CR field is extracted from the saved complete CR word.
  unsigned shift = 0;
};
} // namespace

Xbox360CallFrameInfo::Xbox360CallFrameInfo(ObjectFilePECOFF &object_file,
                                           uint32_t rva, uint32_t size)
    : m_object_file(object_file) {
  auto data = object_file.ReadImageDataByRVA(rva, size);
  data.SetByteOrder(eByteOrderBig);
  
  if (size % kRuntimeFunctionSize || data.GetByteSize() != size) {
    Fail("truncated exception directory");
    return;
  }

  offset_t offset = 0;
  uint64_t previous_end = 0;
  while (offset < size) {
    uint32_t start = data.GetU32(&offset);
    uint32_t packed = data.GetU32(&offset);
    if (!start && !packed)
      continue;
    uint32_t length =
        ((packed >> PData::kFunctionLengthShift) & PData::kFunctionLengthMask) *
        kInstructionSize;
    uint32_t prologue =
        (packed & PData::kPrologueLengthMask) * kInstructionSize;
    // Bit 30 selects four-byte PowerPC instructions; bit 31 is the
    // exception-handler flag and does not change the unwind recipe.
    if (!(packed & PData::kPowerPCInstructions) || !length ||
        start % kInstructionSize || prologue > length || start < previous_end ||
        uint64_t(start) + length > uint64_t(UINT32_MAX) + 1 ||
        start < object_file.GetFileAddress(0)) {
      Fail("invalid packed runtime function");
      m_functions.clear();
      return;
    }

    m_functions.push_back({start, length, prologue});
    previous_end = uint64_t(start) + length;
  }
}

const Xbox360CallFrameInfo::RuntimeFunction *
Xbox360CallFrameInfo::FindFunction(const AddressRange &range) const {
  addr_t address = range.GetBaseAddress().GetFileAddress();
  auto it = std::upper_bound(m_functions.begin(), m_functions.end(), address,
                             [](addr_t address, const RuntimeFunction &f) {
                               return address < f.start;
                             });
  if (it == m_functions.begin())
    return nullptr;
  --it;

  uint64_t end = uint64_t(it->start) + it->size;

  if (address >= end || !range.GetByteSize() ||
      range.GetByteSize() > end - address)
  {
    return nullptr;
  }

  return &*it;
}

bool Xbox360CallFrameInfo::GetAddressRange(Address addr, AddressRange &range) {
  range.Clear();

  const auto *function = FindFunction(AddressRange(addr, 1));
  if (!function)
    return false;

  range = AddressRange(m_object_file.GetAddress(
                           function->start - m_object_file.GetFileAddress(0)),
                       function->size);

  return true;
}

std::optional<uint32_t>
Xbox360CallFrameInfo::ReadInstruction(uint32_t address) {
  if (address < m_object_file.GetFileAddress(0) || address % kInstructionSize)
    return std::nullopt;

  auto data = m_object_file.ReadImageDataByRVA(
      address - m_object_file.GetFileAddress(0), kInstructionSize);

  if (data.GetByteSize() != kInstructionSize)
    return std::nullopt;

  data.SetByteOrder(eByteOrderBig);
  offset_t offset = 0;

  return data.GetU32(&offset);
}

const uint8_t *
Xbox360CallFrameInfo::KeepExpression(std::vector<uint8_t> expression) {
  m_expressions.push_back(std::move(expression));
  return m_expressions.back().data();
}

bool Xbox360CallFrameInfo::GetUnwindPlan(const Address &addr,
                                         UnwindPlan &plan) {
  return GetUnwindPlan(AddressRange(addr, 1), plan);
}

bool Xbox360CallFrameInfo::GetUnwindPlan(const AddressRange &range,
                                         UnwindPlan &plan) {
  std::lock_guard<std::mutex> guard(m_mutex);
  plan.Clear();

  const auto *function = FindFunction(range);
  if (!function)
    return false;

  auto code = m_object_file.ReadImageDataByRVA(
      function->start - m_object_file.GetFileAddress(0), function->size);

  if (code.GetByteSize() != function->size)
    return Fail("unreadable function code");

  code.SetByteOrder(eByteOrderBig);
  auto instruction_at = [&](uint32_t offset) {
    offset_t cursor = offset;
    return code.GetU32(&cursor);
  };

  std::array<PrologueValue, Reg::kGPRCount> registers;

  for (unsigned i = 0; i != Reg::kGPRCount; ++i)
    registers[i] = {PrologueValue::Register, i};

  registers[Reg::kSP] = {PrologueValue::Stack, 0};
  std::map<unsigned, Save> saves;
  std::vector<UnwindPlan::RowSP> rows;
  bool lr_live = true;

  auto append_row = [&](uint32_t offset) {
    auto row = std::make_shared<UnwindPlan::Row>();
    row->SetOffset(offset);

    // A nonvolatile frame pointer, when present, remains usable across alloca.
    unsigned base = Reg::kSP;

    if (registers[Reg::kFP].kind == PrologueValue::Stack)
      base = Reg::kFP;

    row->GetCFAValue().SetIsRegisterPlusOffset(base, -registers[base].value);
    row->SetRegisterLocationToIsCFAPlusOffset(Reg::kSP, 0, true);
    row->SetRegisterLocationToRegister(Reg::kLR, Reg::kLR, true);

    if (!lr_live && !saves.count(Reg::kLR))
      row->SetRegisterLocationToUndefined(Reg::kLR, true, false);

    for (unsigned reg = Reg::kFirstNonvolatileGPR; reg != Reg::kGPRCount; reg++) {
      if (saves.count(reg))
        continue;

      row->SetRegisterLocationToUndefined(reg, true, false);

      for (unsigned current = 0; current != Reg::kGPRCount; ++current)
        if (registers[current].kind == PrologueValue::Register &&
            registers[current].value == reg) 
        {
          row->SetRegisterLocationToRegister(reg, current, true);
          break;
        }
    }

    for (const auto &[reg, save] : saves) {
      if (save.size == kWordSize) {
        auto expression = SavedWord(save.offset);

        if (Reg::IsNonvolatileCR(reg)) {
          expression.insert(expression.end(),
                            {uint8_t(DW_OP_lit0 + save.shift), DW_OP_shr,
                             DW_OP_lit15, DW_OP_and});
        }

        unsigned size = expression.size();
        row->SetRegisterLocationToIsDWARFExpression(
            reg, KeepExpression(std::move(expression)), size, true);
      } else {
        row->SetRegisterLocationToAtCFAPlusOffset(reg, save.offset, true);
      }
    }

    // Before the LR spill, the prologue may have moved the caller's LR to a
    // scratch register and then made a helper call.
    if (!saves.count(Reg::kLR) && !lr_live)
      for (unsigned i = 0; i != Reg::kGPRCount; ++i) {
        if (registers[i].kind == PrologueValue::Register &&
            registers[i].value == Reg::kLR) {
          row->SetRegisterLocationToRegister(Reg::kLR, i, true);
          break;
        }
      }
    rows.push_back(row);
  };

  auto store = [&](PrologueValue value, int64_t offset, unsigned size) {
    if (!llvm::isInt<32>(offset))
      return false;

    if (value.kind == PrologueValue::CR && size == kWordSize) {
      for (unsigned field = Reg::kFirstNonvolatileCRField;
           field <= Reg::kLastNonvolatileCRField; field++) {
        saves[Reg::kCRBase + field] = {(int32_t)offset, kWordSize,
                                       (Reg::kCRFieldCount - 1 - field) *
                                           Reg::kCRFieldBits};
      }
    } else if (value.kind == PrologueValue::Register) {
      unsigned reg = value.value;
      if (reg == Reg::kLR ||
          (reg >= Reg::kFirstNonvolatileGPR && reg < Reg::kGPRCount) ||
          (reg >= Reg::kFirstNonvolatileFPR && reg <= Reg::kLastFPR) ||
          reg >= Reg::kFirstNonvolatileVector) {
        if ((reg == Reg::kLR && size != kWordSize) ||
            (reg >= Reg::kFirstNonvolatileGPR && reg <= Reg::kLastFPR &&
             size != kDoublewordSize)) 
        {
          return false;
        }  
        saves[reg] = {(int32_t)offset, size};
      }
    }

    return true;
  };

  // Interpret only straight-line prologues and bounded, straight-line save
  // helpers. Unknown instructions are not assumed to preserve unwind state.
  auto execute = [&](PPCInstruction instruction) {
    unsigned op = instruction.Opcode();
    unsigned rt = instruction.RT();
    unsigned ra = instruction.RA();
    unsigned rb = instruction.RB();
    int64_t imm = instruction.Immediate();
    unsigned xo = instruction.ExtendedOpcode();

    if (op == Opcode::Addi || op == Opcode::Addis) {
      PrologueValue value =
          ra ? registers[ra] : PrologueValue{PrologueValue::Constant, 0};

      if (value.kind != PrologueValue::Constant &&
          value.kind != PrologueValue::Stack) {
        value = {};
      } else {
        value.value += imm * (int64_t(1) << instruction.ImmediateShift());
      }

      if (!llvm::isInt<32>(value.value))
        value = {};

      registers[rt] = value;
      return rt != Reg::kSP;
    }

    if (op == Opcode::Ori || op == Opcode::Oris) {
      PrologueValue value = registers[rt];
      
      if (value.kind == PrologueValue::Constant) {
        value.value |= int64_t(instruction.UnsignedImmediate())
                       << instruction.ImmediateShift();
      } else if (instruction.UnsignedImmediate()) {
        value = {};
      }

      registers[ra] = value;
      return ra != Reg::kSP;
    }

    if (op == Opcode::Extended && xo == ExtendedOpcode::Or) {
      registers[ra] = rt == rb ? registers[rt] : PrologueValue{};
      return ra != Reg::kSP;
    }

    if (op == Opcode::Rlwinm) { // Argument alignment scheduled in a prologue.
      registers[ra] = {};
      return ra != Reg::kSP;
    }

    if (op == Opcode::Extended && xo == ExtendedOpcode::Add) {
      auto a = registers[ra], b = registers[rb];
      if (a.kind == PrologueValue::Constant)
        std::swap(a, b);

      registers[rt] = {};

      if ((a.kind == PrologueValue::Constant ||
           a.kind == PrologueValue::Stack) &&
          b.kind == PrologueValue::Constant &&
          llvm::isInt<32>(a.value + b.value)) 
      {
        registers[rt] = {a.kind, a.value + b.value};
      }

      return rt != Reg::kSP;
    }

    if (instruction.IsMflr()) {
      registers[rt] = lr_live ? PrologueValue{PrologueValue::Register, Reg::kLR}
                              : PrologueValue{};
      return rt != Reg::kSP;
    }

    if (instruction.IsMfcr()) {
      registers[rt] = {PrologueValue::CR, 0};
      return rt != Reg::kSP;
    }

    if (op == Opcode::Stwu ||
        (op == Opcode::Extended && xo == ExtendedOpcode::Stwux)) 
    {
      if (rt != Reg::kSP || ra != Reg::kSP ||
          registers[Reg::kSP].kind != PrologueValue::Stack) 
      {
        return false;
      }
            
      if (op == Opcode::Extended) {
        if (registers[rb].kind != PrologueValue::Constant)
          return false;

        imm = registers[rb].value;
      }

      if (imm >= 0 || imm % kStackAlignment ||
          registers[Reg::kSP].value + imm < -(int64_t)INT32_MAX)
      {
        return false;
      }

      registers[Reg::kSP].value += imm;
      return true;
    }
    if (op == Opcode::Stw || op == Opcode::Stfd ||
        (op == Opcode::Std && instruction.IsPlainDSForm())) 
    {
      if (!ra || registers[ra].kind != PrologueValue::Stack)
        return false;

      if (op == Opcode::Std)
        imm = instruction.DSDisplacement();

      PrologueValue value =
          op == Opcode::Stfd
              ? PrologueValue{PrologueValue::Register, Reg::kFPRBase + rt}
              : registers[rt];

      return store(value, registers[ra].value + imm,
                   op == Opcode::Stw ? kWordSize : kDoublewordSize);
    }

    if (instruction.IsVectorStore()) {
      auto base =
          ra ? registers[ra] : PrologueValue{PrologueValue::Constant, 0};
      auto index = registers[rb];

      if (base.kind == PrologueValue::Constant)
        std::swap(base, index);

      if (base.kind != PrologueValue::Stack ||
          index.kind != PrologueValue::Constant)
      {
        return false;
      }

      unsigned dwarf = Reg::VectorToDWARF(instruction.VectorRegister());
      return store({PrologueValue::Register, dwarf}, base.value + index.value,
                   kVectorSize);
    }

    if (op == Opcode::Ld && instruction.IsPlainDSForm()) { // Stack probes too.
      registers[rt] = {};
      return rt != Reg::kSP;
    }

    // nop is ori r0,r0,0 and was handled above.
    return false;
  };

  append_row(0);
  for (uint32_t offset = 0; offset < function->prologue_size;
       offset += kInstructionSize) 
  {
    PPCInstruction instruction{instruction_at(offset)};
    if (instruction.Opcode() == Opcode::Branch && instruction.HasLink()) {
      // bl a register-save helper. Its instructions, not its name, describe
      // which registers are saved; never follow arbitrary control flow.
      int64_t target = instruction.BranchTarget(function->start + offset);
      lr_live = false;
      bool returned = false;

      for (unsigned count = 0; count != kMaxSaveHelperInstructions;
           count++, target += kInstructionSize) 
      {
        if (target < 0 || target > UINT32_MAX)
          break;

        auto helper = ReadInstruction(target);
        if (!helper)
          break;

        if (PPCInstruction{*helper}.IsBLR()) {
          returned = true;
          break;
        }

        if (!execute(PPCInstruction{*helper}))
          break;
      }
      if (!returned)
        return Fail("unsupported prologue helper");
    } else if (!execute(instruction)) {
      return Fail("unsupported prologue instruction");
    }
    append_row(offset + kInstructionSize);
  }

  const auto body_registers = registers;
  const auto body_saves = saves;
  const bool body_lr_live = lr_live;

  // Recognize restore sequences ending in blr/bctr or a direct tail branch.
  // Other body instructions are not interpreted as prologue instructions.
  auto restore_instruction = [](PPCInstruction instruction) {
    unsigned op = instruction.Opcode();
    unsigned xo = instruction.ExtendedOpcode();
    return op == Opcode::Addi || op == Opcode::Addis || op == Opcode::Ori ||
           op == Opcode::Oris || op == Opcode::Lwz || op == Opcode::Lfd ||
           (op == Opcode::Ld && instruction.IsPlainDSForm()) ||
           (op == Opcode::Extended &&
            (xo == ExtendedOpcode::Or || xo == ExtendedOpcode::Mtspr ||
             xo == ExtendedOpcode::Mtcrf)) ||
           instruction.IsVectorLoad();
  };

  for (uint32_t end = function->prologue_size; end < function->size;
       end += kInstructionSize) 
  {
    PPCInstruction terminal{instruction_at(end)};

    bool tail = terminal.Opcode() == Opcode::Branch && !terminal.HasLink();
    if (tail) {
      int64_t target = terminal.BranchTarget(function->start + end);
      tail = target < function->start ||
             target >= (uint64_t)function->start + function->size;
    }

    if (!terminal.IsBLR() && !terminal.IsBCTR() && !tail)
      continue;

    uint32_t begin = end;
    while (begin > function->prologue_size && end - begin < kMaxEpilogueBytes) {
      PPCInstruction instruction{instruction_at(begin - kInstructionSize)};
      if (!restore_instruction(instruction))
        break;
      begin -= kInstructionSize;
    }

    registers = body_registers;
    // Volatile registers no longer hold their prologue values in the body.
    for (unsigned i = 0; i != Reg::kFirstNonvolatileGPR; ++i) {
      if (i != Reg::kSP && i != Reg::kReserved && i != Reg::kThreadPointer)
        registers[i] = {};
    }

    saves = body_saves;
    lr_live = body_lr_live;

    for (uint32_t offset = begin; offset < end; offset += kInstructionSize) {
      PPCInstruction instruction{instruction_at(offset)};
      unsigned op = instruction.Opcode();
      unsigned rt = instruction.RT();
      unsigned ra = instruction.RA();
      unsigned rb = instruction.RB();
      unsigned xo = instruction.ExtendedOpcode();
      int64_t imm = instruction.Immediate();
      bool valid = true;

      if (op == Opcode::Addi && rt == Reg::kSP && ra == Reg::kSP) {
        valid = registers[Reg::kSP].kind == PrologueValue::Stack &&
                llvm::isInt<32>(registers[Reg::kSP].value + imm);

        if (valid)
          registers[Reg::kSP].value += imm;
      } else if (op == Opcode::Extended && xo == ExtendedOpcode::Or &&
                 ra == Reg::kSP && rt == rb) {
        valid = registers[rt].kind == PrologueValue::Stack;

        if (valid)
          registers[Reg::kSP] = registers[rt];
      } else if (op == Opcode::Lwz && rt == Reg::kSP && ra == Reg::kSP &&
                 imm == 0) 
      {
        // Xbox 360's backchain is a word, including for dynamic stack allocations.
        registers[Reg::kSP] = {PrologueValue::Stack, 0};
      } else if (op == Opcode::Lwz || op == Opcode::Lfd || op == Opcode::Ld ||
                 (op == Opcode::Extended &&
                  (xo == ExtendedOpcode::Lvx || xo == ExtendedOpcode::Lvxl)) ||
                 op == Opcode::VMX128) 
      {
        unsigned size = op == Opcode::Lwz ? kWordSize : kDoublewordSize;
        unsigned reg = op == Opcode::Lfd ? Reg::kFPRBase + rt : rt;
        bool vector = op == Opcode::Extended || op == Opcode::VMX128;

        PrologueValue base =
            ra ? registers[ra] : PrologueValue{PrologueValue::Constant, 0};

        if (vector) {
          auto index = registers[rb];

          if (base.kind == PrologueValue::Constant)
            std::swap(base, index);

          valid = index.kind == PrologueValue::Constant;
          imm = index.value;
          reg = Reg::VectorToDWARF(instruction.VectorRegister());
          size = kVectorSize;
        } else if (op == Opcode::Ld) {
          imm = instruction.DSDisplacement();
        }

        valid &= base.kind == PrologueValue::Stack;

        if (op == Opcode::Lwz || op == Opcode::Ld)
          registers[rt] = {};
          
        if (valid) {
          for (auto it = saves.begin(); it != saves.end();) {
            if (it->second.offset != base.value + imm ||
                it->second.size != size) {
              ++it;
              continue;
            }
            if (it->first == Reg::kLR && op == Opcode::Lwz) {
              registers[rt] = {PrologueValue::Register, Reg::kLR};
              ++it;
            } else if (Reg::IsNonvolatileCR(it->first) && op == Opcode::Lwz) {
              registers[rt] = {PrologueValue::CR, 0};
              ++it;
            } else if (it->first == reg) {
              if (reg < Reg::kGPRCount)
                registers[reg] = {PrologueValue::Register, reg};
              it = saves.erase(it);
            } else {
              ++it;
            }
          }
        }
      } else if (instruction.IsMtlr()) {
        valid = registers[rt].kind == PrologueValue::Register &&
                registers[rt].value == Reg::kLR;

        if (valid) {
          lr_live = true;
          saves.erase(Reg::kLR);
        } else {
          lr_live = false;
        }
      } else if (op == Opcode::Extended && xo == ExtendedOpcode::Mtcrf) {
        valid = registers[rt].kind == PrologueValue::CR;

        if (valid)
          for (unsigned field = Reg::kFirstNonvolatileCRField;
               field <= Reg::kLastNonvolatileCRField; field++) 
          {
            if (instruction.WritesCRField(field))
              saves.erase(Reg::kCRBase + field);
          }
      } else {
        valid = execute(instruction);
      }
      if (!valid) {
        // A return-value load can precede the actual restores in this suffix.
        // Keep spill locations when its effective address is not known.
        Fail("unrecognized epilogue value; retaining saved locations");
        if (((op == Opcode::Addi || op == Opcode::Addis || op == Opcode::Lwz ||
              op == Opcode::Ld) &&
             rt == Reg::kSP) ||
            ((op == Opcode::Ori || op == Opcode::Oris ||
              (op == Opcode::Extended && xo == ExtendedOpcode::Or)) &&
             ra == Reg::kSP)) 
        {
          auto unavailable = std::make_shared<UnwindPlan::Row>();
          unavailable->SetOffset(offset + kInstructionSize);
          rows.push_back(unavailable);
          break;
        }
      }
      append_row(offset + kInstructionSize);
    }
    // A later basic block starts in the ordinary body state, not the state
    // reached by linearly executing a preceding return.
    registers = body_registers;
    saves = body_saves;
    lr_live = body_lr_live;

    if (end + kInstructionSize < function->size)
      append_row(end + kInstructionSize);
  }

  std::stable_sort(rows.begin(), rows.end(), [](const auto &a, const auto &b) {
    return a->GetOffset() < b->GetOffset();
  });

  plan.SetRegisterKind(eRegisterKindDWARF);
  plan.SetReturnAddressRegister(Reg::kLR);
  plan.SetSourceName("Xbox 360 packed unwind info");
  plan.SetSourcedFromCompiler(eLazyBoolYes);
  // Packed metadata plus recognized restores does not describe arbitrary
  // hand-written epilogues or control flow.
  plan.SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  plan.SetUnwindPlanForSignalTrap(eLazyBoolNo);
  plan.SetPlanValidAddressRange(
      AddressRange(m_object_file.GetAddress(function->start -
                                            m_object_file.GetFileAddress(0)),
                   function->size));
  for (auto &row : rows)
    plan.AppendRow(row);
  return true;
}

//===-- ABIXbox360.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIXbox360.h"

#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Value.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/Xbox360RegisterInfo.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "llvm/Support/MathExtras.h"

using namespace lldb;
using namespace lldb_private;

LLDB_PLUGIN_DEFINE(ABIXbox360)

namespace {
bool Fail(llvm::StringRef message) {
  LLDB_LOG(GetLog(LLDBLog::Expressions), "Xbox 360 ABI: {0}", message);
  return false;
}

bool IsNumberedRegister(llvm::StringRef name, llvm::StringRef prefix,
                        unsigned first, unsigned last) {
  if (!name.consume_front(prefix))
    return false;
  unsigned number;
  return !name.empty() && !name.getAsInteger(10, number) && number >= first &&
         number <= last;
}
} // namespace

void ABIXbox360::Initialize() {
  PluginManager::RegisterPlugin(GetPluginNameStatic(), "Xbox 360 PowerPC ABI",
                                CreateInstance);
}

void ABIXbox360::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

ABISP ABIXbox360::CreateInstance(ProcessSP process_sp, const ArchSpec &arch) {
  const auto &triple = arch.GetTriple();
  if (!triple.isOSXbox360() ||
      (triple.getArch() != llvm::Triple::ppc &&
       triple.getArch() != llvm::Triple::ppc64))
    return {};
  return ABISP(new ABIXbox360(std::move(process_sp), MakeMCRegisterInfo(arch)));
}

void ABIXbox360::AugmentRegisterInfo(
    std::vector<DynamicRegisterInfo::Register> &regs) {
  AugmentXbox360RegisterInfo(ArchSpec("powerpc-ibm-xbox360"), regs);
}

bool ABIXbox360::RegisterIsVolatile(const RegisterInfo *reg_info) {
  if (!reg_info)
    return true;
  auto is_preserved = [](llvm::StringRef name) {
    // r2 and r13 are reserved state, not allocatable callee-saved registers.
    // The complete CR is volatile: only its individual fields 2-5 survive.
    return name == "sp" || name == "r1" || name == "r2" || name == "r13" ||
           IsNumberedRegister(name, "r", 14, 31) ||
           IsNumberedRegister(name, "f", 14, 31) ||
           IsNumberedRegister(name, "v", 64, 127) ||
           IsNumberedRegister(name, "vr", 64, 127) ||
           IsNumberedRegister(name, "cr", 2, 5);
  };
  return !is_preserved(llvm::StringRef(reg_info->name ? reg_info->name : "")) &&
         !is_preserved(
             llvm::StringRef(reg_info->alt_name ? reg_info->alt_name : ""));
}

bool ABIXbox360::PrepareTrivialCall(Thread &thread, addr_t sp,
                                   addr_t function_address,
                                   addr_t return_address,
                                   llvm::ArrayRef<addr_t> args) const {
  auto reg_ctx = thread.GetRegisterContext();
  auto process_sp = thread.GetProcess();
  if (!reg_ctx || !process_sp)
    return Fail("missing register context or process");
  if (sp > UINT32_MAX || !CodeAddressIsValid(function_address) ||
      !CodeAddressIsValid(return_address))
    return Fail("invalid call address");

  // Eight doubleword home slots are mandatory even for register-only calls.
  const uint64_t max_slots = (UINT32_MAX - 31ull) / 8;
  if (args.size() > max_slots)
    return Fail("argument area exceeds the address space");
  uint64_t frame_size =
      llvm::alignTo(16 + std::max<size_t>(8, args.size()) * 8, 16);
  sp = llvm::alignDown(sp, 16);
  if (sp <= frame_size)
    return Fail("insufficient stack space");
  sp -= frame_size;

  const RegisterInfo *pc = reg_ctx->GetRegisterInfoByName("pc");
  const RegisterInfo *lr = reg_ctx->GetRegisterInfoByName("lr");
  const RegisterInfo *stack = reg_ctx->GetRegisterInfoByName("r1");
  if (!pc || !lr || !stack)
    return Fail("missing PC, LR or SP register");
  std::vector<const RegisterInfo *> arg_regs;
  for (size_t i = 0; i < std::min<size_t>(args.size(), 8); ++i) {
    const RegisterInfo *reg =
        reg_ctx->GetRegisterInfoByName("r" + std::to_string(3 + i));
    if (!reg || reg->byte_size != 8)
      return Fail("missing full-width argument register");
    arg_regs.push_back(reg);
  }
  uint64_t original_sp = reg_ctx->GetSP(LLDB_INVALID_ADDRESS);
  if (original_sp > UINT32_MAX)
    return Fail("cannot read the original stack pointer");
  Status error;
  if (!process_sp->WriteScalarToMemory(sp, original_sp, 4, error))
    return Fail("cannot write stack back chain");
  for (size_t i = 8; i < args.size(); ++i)
    if (!process_sp->WriteScalarToMemory(sp + 16 + i * 8, args[i], 8, error))
      return Fail("cannot write stack argument");
  for (size_t i = 0; i < arg_regs.size(); ++i)
    if (!reg_ctx->WriteRegisterFromUnsigned(arg_regs[i], args[i]))
      return Fail("cannot write argument register");
  if (!reg_ctx->WriteRegisterFromUnsigned(lr, return_address) ||
      !reg_ctx->WriteRegisterFromUnsigned(stack, sp) ||
      !reg_ctx->WriteRegisterFromUnsigned(pc, function_address))
    return Fail("cannot write call control registers");
  return true;
}

bool ABIXbox360::GetArgumentValues(Thread &thread, ValueList &values) const {
  auto reg_ctx = thread.GetRegisterContext();
  auto process_sp = thread.GetProcess();
  if (!reg_ctx || !process_sp)
    return Fail("missing register context or process");
  addr_t sp = reg_ctx->GetSP(LLDB_INVALID_ADDRESS);
  if (sp == LLDB_INVALID_ADDRESS || sp > UINT32_MAX)
    return Fail("cannot read the stack pointer");

  uint64_t offset = 16;
  uint64_t non_gpr_bytes = 0;
  unsigned fpr = 1, vr = 1;
  ValueList result;
  for (size_t i = 0; i < values.GetSize(); ++i) {
    Value value = *values.GetValueAtIndex(i);
    CompilerType type = value.GetCompilerType();
    auto bit_size = type.GetBitSize(&thread);
    if (!bit_size || !*bit_size)
      return Fail("unknown argument size");
    bool is_signed = false, is_complex = false;
    uint32_t count = 0;
    bool integer = type.IsIntegerOrEnumerationType(is_signed) ||
                   type.IsPointerOrReferenceType();
    bool floating = type.IsFloatingPointType(count, is_complex) && !is_complex &&
                    count == 1;
    bool vector = type.IsVectorType();
    if ((!integer && !floating && !vector) ||
        (integer && *bit_size > 64) ||
        (floating && *bit_size != 32 && *bit_size != 64) ||
        (vector && *bit_size != 128))
      return Fail("unsupported argument type; aggregate/C++ lowering requires "
                  "function ABI metadata");

    uint64_t aligned_offset = llvm::alignTo(offset, vector ? 16 : 8);
    if (vector)
      non_gpr_bytes += aligned_offset - offset;
    offset = aligned_offset;
    unsigned gpr = std::min<uint64_t>((offset - 16 - non_gpr_bytes) / 8, 8);
    uint32_t size = (*bit_size + 7) / 8;
    uint8_t bytes[16] = {};
    Status error;
    if (vector && vr <= 13) {
      const RegisterInfo *reg =
          reg_ctx->GetRegisterInfoByName("v" + std::to_string(vr));
      if (!reg)
        reg = reg_ctx->GetRegisterInfoByName("vr" + std::to_string(vr));
      ++vr;
      RegisterValue reg_value;
      if (!reg || reg->byte_size != 16 ||
          !reg_ctx->ReadRegister(reg, reg_value) ||
          reg_value.GetAsMemoryData(*reg, bytes, size, eByteOrderBig, error) !=
              size ||
          error.Fail())
        return Fail("cannot read vector argument register");
      value.SetBytes(bytes, size);
    } else if (floating && fpr <= 13) {
      const RegisterInfo *reg =
          reg_ctx->GetRegisterInfoByName("f" + std::to_string(fpr++));
      RegisterValue reg_value;
      if (!reg || reg->byte_size != 8 ||
          !reg_ctx->ReadRegister(reg, reg_value) ||
          reg_value.GetAsMemoryData(*reg, bytes, 8, eByteOrderBig, error) != 8 ||
          error.Fail())
        return Fail("cannot read floating-point argument register");
      DataExtractor data(bytes, 8, eByteOrderBig, 4);
      lldb::offset_t data_offset = 0;
      double number = data.GetDouble(&data_offset);
      if (*bit_size == 32)
        value.GetScalar() = static_cast<float>(number);
      else
        value.GetScalar() = number;
      value.SetValueType(Value::ValueType::Scalar);
    } else {
      if (!vector && gpr < 8) {
        const RegisterInfo *reg =
            reg_ctx->GetRegisterInfoByName("r" + std::to_string(3 + gpr));
        RegisterValue reg_value;
        if (!reg || reg->byte_size != 8 ||
            !reg_ctx->ReadRegister(reg, reg_value) ||
            reg_value.GetAsMemoryData(*reg, bytes, size, eByteOrderBig, error) !=
                size ||
            error.Fail())
          return Fail("cannot read integer argument register");
      } else {
        // Scalars are right-justified in doubleword slots on big-endian Xbox.
        uint64_t address = sp + offset + (vector ? 0 : 8 - size);
        if (address > UINT32_MAX || size - 1 > UINT32_MAX - address ||
            process_sp->ReadMemory(address, bytes, size, error) != size ||
            error.Fail())
          return Fail("cannot read stack argument");
      }
      if (vector) {
        value.SetBytes(bytes, size);
      } else {
        DataExtractor data(bytes, size, eByteOrderBig, 4);
        Encoding encoding = floating ? eEncodingIEEE754 : eEncodingUint;
        error = value.GetScalar().SetValueFromData(data, encoding, size);
        if (error.Fail())
          return Fail("cannot decode argument");
        if (integer)
          value.GetScalar().TruncOrExtendTo(*bit_size, is_signed);
        value.SetValueType(Value::ValueType::Scalar);
      }
    }
    offset += vector ? 16 : 8;
    if (vector)
      non_gpr_bytes += 16;
    result.PushValue(value);
  }
  values = result;
  return true;
}

bool ABIXbox360::CreateFunctionEntryUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  unwind_plan.SetRegisterKind(eRegisterKindGeneric);
  auto row = std::make_shared<UnwindPlan::Row>();
  row->GetCFAValue().SetIsRegisterPlusOffset(LLDB_REGNUM_GENERIC_SP, 0);
  row->SetRegisterLocationToRegister(LLDB_REGNUM_GENERIC_PC,
                                     LLDB_REGNUM_GENERIC_RA, true);
  unwind_plan.AppendRow(row);
  unwind_plan.SetReturnAddressRegister(LLDB_REGNUM_GENERIC_RA);
  unwind_plan.SetSourceName("Xbox 360 function entry");
  unwind_plan.SetSourcedFromCompiler(eLazyBoolNo);
  unwind_plan.SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  return true;
}

bool ABIXbox360::CreateDefaultUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  return false;
}

bool ABIXbox360::CallFrameAddressIsValid(addr_t cfa) {
  return cfa && cfa <= UINT32_MAX && (cfa & 15) == 0;
}

bool ABIXbox360::CodeAddressIsValid(addr_t pc) {
  return pc && pc <= UINT32_MAX && (pc & 3) == 0;
}

Status ABIXbox360::SetReturnValueObject(StackFrameSP &frame_sp,
                                       ValueObjectSP &new_value) {
  return Status::FromErrorString(
      "Setting Xbox 360 return values is not implemented");
}

ValueObjectSP
ABIXbox360::GetReturnValueObjectImpl(Thread &thread, CompilerType &type) const {
  Fail("reading Xbox 360 return values is not implemented");
  return {};
}

//===-- ABI.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Target/ABI.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Value.h"
#include "lldb/Expression/ExpressionVariable.h"
#include "lldb/Symbol/CompilerType.h"
#include "lldb/Symbol/TypeSystem.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/ValueObject/ValueObjectConstResult.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FormatVariadic.h"
#include <cctype>

using namespace lldb;
using namespace lldb_private;

ABISP
ABI::FindPlugin(lldb::ProcessSP process_sp, const ArchSpec &arch) {
  ABISP abi_sp;
  ABICreateInstance create_callback;

  for (uint32_t idx = 0;
       (create_callback = PluginManager::GetABICreateCallbackAtIndex(idx)) !=
       nullptr;
       ++idx) {
    abi_sp = create_callback(process_sp, arch);

    if (abi_sp)
      return abi_sp;
  }
  abi_sp.reset();
  return abi_sp;
}

ABI::~ABI() = default;

namespace {
// Mirror Microsoft's exception constants without depending on Windows headers.
enum class MicrosoftExceptionCode : uint32_t {
  EXCEPTION_GUARD_PAGE = 0x80000001,
  EXCEPTION_DATATYPE_MISALIGNMENT = 0x80000002,
  EXCEPTION_BREAKPOINT = 0x80000003,
  EXCEPTION_SINGLE_STEP = 0x80000004,
  EXCEPTION_ACCESS_VIOLATION = 0xc0000005,
  EXCEPTION_IN_PAGE_ERROR = 0xc0000006,
  EXCEPTION_ILLEGAL_INSTRUCTION = 0xc000001d,
  EXCEPTION_NONCONTINUABLE_EXCEPTION = 0xc0000025,
  EXCEPTION_INVALID_DISPOSITION = 0xc0000026,
  EXCEPTION_ARRAY_BOUNDS_EXCEEDED = 0xc000008c,
  EXCEPTION_FLT_DENORMAL_OPERAND = 0xc000008d,
  EXCEPTION_FLT_DIVIDE_BY_ZERO = 0xc000008e,
  EXCEPTION_FLT_INEXACT_RESULT = 0xc000008f,
  EXCEPTION_FLT_INVALID_OPERATION = 0xc0000090,
  EXCEPTION_FLT_OVERFLOW = 0xc0000091,
  EXCEPTION_FLT_STACK_CHECK = 0xc0000092,
  EXCEPTION_FLT_UNDERFLOW = 0xc0000093,
  EXCEPTION_INT_DIVIDE_BY_ZERO = 0xc0000094,
  EXCEPTION_INT_OVERFLOW = 0xc0000095,
  EXCEPTION_PRIV_INSTRUCTION = 0xc0000096,
  EXCEPTION_STACK_OVERFLOW = 0xc00000fd,
  EH_EXCEPTION_NUMBER = 0xe06d7363,
};
} // namespace

std::string ABI::ExceptionRecord::GetDescription() const {
  const char *name = "unknown exception";
  auto exception_code = static_cast<MicrosoftExceptionCode>(code);

  switch (exception_code) {
  case MicrosoftExceptionCode::EXCEPTION_GUARD_PAGE:
    name = "guard page violation";
    break;
  case MicrosoftExceptionCode::EXCEPTION_DATATYPE_MISALIGNMENT:
    name = "datatype misalignment";
    break;
  case MicrosoftExceptionCode::EXCEPTION_BREAKPOINT:
    name = "breakpoint";
    break;
  case MicrosoftExceptionCode::EXCEPTION_SINGLE_STEP:
    name = "single step";
    break;
  case MicrosoftExceptionCode::EXCEPTION_ACCESS_VIOLATION:
    name = "access violation";
    break;
  case MicrosoftExceptionCode::EXCEPTION_IN_PAGE_ERROR:
    name = "in-page error";
    break;
  case MicrosoftExceptionCode::EXCEPTION_ILLEGAL_INSTRUCTION:
    name = "illegal instruction";
    break;
  case MicrosoftExceptionCode::EXCEPTION_NONCONTINUABLE_EXCEPTION:
    name = "noncontinuable exception";
    break;
  case MicrosoftExceptionCode::EXCEPTION_INVALID_DISPOSITION:
    name = "invalid disposition";
    break;
  case MicrosoftExceptionCode::EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    name = "array bounds exceeded";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_DENORMAL_OPERAND:
    name = "floating-point denormal operand";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_DIVIDE_BY_ZERO:
    name = "floating-point divide by zero";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_INEXACT_RESULT:
    name = "floating-point inexact result";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_INVALID_OPERATION:
    name = "floating-point invalid operation";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_OVERFLOW:
    name = "floating-point overflow";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_STACK_CHECK:
    name = "floating-point stack check";
    break;
  case MicrosoftExceptionCode::EXCEPTION_FLT_UNDERFLOW:
    name = "floating-point underflow";
    break;
  case MicrosoftExceptionCode::EXCEPTION_INT_DIVIDE_BY_ZERO:
    name = "integer divide by zero";
    break;
  case MicrosoftExceptionCode::EXCEPTION_INT_OVERFLOW:
    name = "integer overflow";
    break;
  case MicrosoftExceptionCode::EXCEPTION_PRIV_INSTRUCTION:
    name = "privileged instruction";
    break;
  case MicrosoftExceptionCode::EXCEPTION_STACK_OVERFLOW:
    name = "stack overflow";
    break;
  case MicrosoftExceptionCode::EH_EXCEPTION_NUMBER:
    name = "Microsoft C++ exception";
    break;
  }

  std::string description =
      llvm::formatv("{0} ({1:x8}) at {2:x8}", name, code, address).str();

  if ((exception_code == MicrosoftExceptionCode::EXCEPTION_ACCESS_VIOLATION ||
       exception_code == MicrosoftExceptionCode::EXCEPTION_IN_PAGE_ERROR) &&
      parameters.size() >= 2) {
    const char *operation = nullptr;
    switch (parameters[0]) {
    case 0:
      operation = "read";
      break;
    case 1:
      operation = "write";
      break;
    case 8:
      operation = "execute";
      break;
    }
    if (operation)
      description +=
          llvm::formatv(": {0} at {1:x8}", operation, parameters[1]).str();
    else
      description += llvm::formatv(": access operation {0:x} at {1:x8}",
                                   parameters[0], parameters[1])
                         .str();
  }

  if (flags & 1)
    description += " (noncontinuable)";
  return description;
}

StructuredData::DictionarySP ABI::ExceptionRecord::GetStructuredData() const {
  auto info = std::make_shared<StructuredData::Dictionary>();
  info->AddIntegerItem("code", code);
  info->AddIntegerItem("flags", flags);
  info->AddIntegerItem("record_address", record_address);
  info->AddIntegerItem("address", address);
  info->AddIntegerItem("next_record", next_record);
  info->AddBooleanItem("continuable", !(flags & 1));
  info->AddStringItem("description", GetDescription());
  auto args = std::make_shared<StructuredData::Array>();
  for (uint64_t parameter : parameters)
    args->AddIntegerItem(parameter);
  info->AddItem("parameters", args);
  return info;
}

bool RegInfoBasedABI::GetRegisterInfoByName(llvm::StringRef name,
                                            RegisterInfo &info) {
  uint32_t count = 0;
  const RegisterInfo *register_info_array = GetRegisterInfoArray(count);
  if (register_info_array) {
    uint32_t i;
    for (i = 0; i < count; ++i) {
      const char *reg_name = register_info_array[i].name;
      if (reg_name == name) {
        info = register_info_array[i];
        return true;
      }
    }
    for (i = 0; i < count; ++i) {
      const char *reg_alt_name = register_info_array[i].alt_name;
      if (reg_alt_name == name) {
        info = register_info_array[i];
        return true;
      }
    }
  }
  return false;
}

ValueObjectSP ABI::GetReturnValueObject(
    Thread &thread, CompilerType &ast_type, bool persistent,
    const CompilerDeclContext *function_decl_context) const {
  if (!ast_type.IsValid())
    return ValueObjectSP();

  ValueObjectSP return_valobj_sp;

  return_valobj_sp =
      GetReturnValueObjectImpl(thread, ast_type, function_decl_context);
  if (!return_valobj_sp)
    return return_valobj_sp;

  // Now turn this into a persistent variable.
  // FIXME: This code is duplicated from Target::EvaluateExpression, and it is
  // used in similar form in a couple
  // of other places.  Figure out the correct Create function to do all this
  // work.

  if (persistent) {
    Target &target = *thread.CalculateTarget();
    PersistentExpressionState *persistent_expression_state =
        target.GetPersistentExpressionStateForLanguage(
            ast_type.GetMinimumLanguage());

    if (!persistent_expression_state)
      return {};

    ConstString persistent_variable_name =
        persistent_expression_state->GetNextPersistentVariableName();

    lldb::ValueObjectSP const_valobj_sp;

    // Check in case our value is already a constant value
    if (return_valobj_sp->GetIsConstant()) {
      const_valobj_sp = return_valobj_sp;
      const_valobj_sp->SetName(persistent_variable_name);
    } else
      const_valobj_sp =
          return_valobj_sp->CreateConstantValue(persistent_variable_name);

    lldb::ValueObjectSP live_valobj_sp = return_valobj_sp;

    return_valobj_sp = const_valobj_sp;

    ExpressionVariableSP expr_variable_sp(
        persistent_expression_state->CreatePersistentVariable(
            return_valobj_sp));

    assert(expr_variable_sp);

    // Set flags and live data as appropriate

    const Value &result_value = live_valobj_sp->GetValue();

    switch (result_value.GetValueType()) {
    case Value::ValueType::Invalid:
      return {};
    case Value::ValueType::HostAddress:
    case Value::ValueType::FileAddress:
      // we odon't do anything with these for now
      break;
    case Value::ValueType::Scalar:
      expr_variable_sp->m_flags |=
          ExpressionVariable::EVIsFreezeDried;
      expr_variable_sp->m_flags |=
          ExpressionVariable::EVIsLLDBAllocated;
      expr_variable_sp->m_flags |=
          ExpressionVariable::EVNeedsAllocation;
      break;
    case Value::ValueType::LoadAddress:
      expr_variable_sp->m_live_sp = live_valobj_sp;
      expr_variable_sp->m_flags |=
          ExpressionVariable::EVIsProgramReference;
      break;
    }

    return_valobj_sp = expr_variable_sp->GetValueObject();
  }
  return return_valobj_sp;
}

ValueObjectSP ABI::GetReturnValueObjectImpl(
    Thread &thread, CompilerType &ast_type,
    const CompilerDeclContext *function_decl_context) const {
  return GetReturnValueObjectImpl(thread, ast_type);
}

addr_t ABI::FixCodeAddress(lldb::addr_t pc) {
  ProcessSP process_sp(GetProcessSP());

  addr_t mask = process_sp->GetCodeAddressMask();
  if (mask == LLDB_INVALID_ADDRESS_MASK)
    return pc;

  // Assume the high bit is used for addressing, which
  // may not be correct on all architectures e.g. AArch64
  // where Top Byte Ignore mode is often used to store
  // metadata in the top byte, and b55 is the bit used for
  // differentiating between low- and high-memory addresses.
  // That target's ABIs need to override this method.
  bool is_highmem = pc & (1ULL << 63);
  return is_highmem ? pc | mask : pc & (~mask);
}

addr_t ABI::FixDataAddress(lldb::addr_t pc) {
  ProcessSP process_sp(GetProcessSP());
  addr_t mask = process_sp->GetDataAddressMask();
  if (mask == LLDB_INVALID_ADDRESS_MASK)
    return pc;

  // Assume the high bit is used for addressing, which
  // may not be correct on all architectures e.g. AArch64
  // where Top Byte Ignore mode is often used to store
  // metadata in the top byte, and b55 is the bit used for
  // differentiating between low- and high-memory addresses.
  // That target's ABIs need to override this method.
  bool is_highmem = pc & (1ULL << 63);
  return is_highmem ? pc | mask : pc & (~mask);
}

ValueObjectSP ABI::GetReturnValueObject(Thread &thread, llvm::Type &ast_type,
                                        bool persistent) const {
  ValueObjectSP return_valobj_sp;
  return_valobj_sp = GetReturnValueObjectImpl(thread, ast_type);
  return return_valobj_sp;
}

// specialized to work with llvm IR types
//
// for now we will specify a default implementation so that we don't need to
// modify other ABIs
lldb::ValueObjectSP ABI::GetReturnValueObjectImpl(Thread &thread,
                                                  llvm::Type &ir_type) const {
  ValueObjectSP return_valobj_sp;

  /* this is a dummy and will only be called if an ABI does not override this */

  return return_valobj_sp;
}

bool ABI::PrepareTrivialCall(Thread &thread, lldb::addr_t sp,
                             lldb::addr_t functionAddress,
                             lldb::addr_t returnAddress, llvm::Type &returntype,
                             llvm::ArrayRef<ABI::CallArgument> args) const {
  // dummy prepare trivial call
  llvm_unreachable("Should never get here!");
}

bool ABI::GetFallbackRegisterLocation(
    const RegisterInfo *reg_info,
    UnwindPlan::Row::AbstractRegisterLocation &unwind_regloc) {
  // Did the UnwindPlan fail to give us the caller's stack pointer? The stack
  // pointer is defined to be the same as THIS frame's CFA, so return the CFA
  // value as the caller's stack pointer.  This is true on x86-32/x86-64 at
  // least.
  if (reg_info->kinds[eRegisterKindGeneric] == LLDB_REGNUM_GENERIC_SP) {
    unwind_regloc.SetIsCFAPlusOffset(0);
    return true;
  }

  // If a volatile register is being requested, we don't want to forward the
  // next frame's register contents up the stack -- the register is not
  // retrievable at this frame.
  if (RegisterIsVolatile(reg_info)) {
    unwind_regloc.SetUndefined();
    return true;
  }

  return false;
}

std::unique_ptr<llvm::MCRegisterInfo> ABI::MakeMCRegisterInfo(const ArchSpec &arch) {
  std::string triple = arch.GetTriple().getTriple();
  std::string lookup_error;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(triple, lookup_error);
  if (!target) {
    LLDB_LOG(GetLog(LLDBLog::Process),
             "Failed to create an llvm target for {0}: {1}", triple,
             lookup_error);
    return nullptr;
  }
  std::unique_ptr<llvm::MCRegisterInfo> info_up(
      target->createMCRegInfo(triple));
  assert(info_up);
  return info_up;
}

void RegInfoBasedABI::AugmentRegisterInfo(
    std::vector<DynamicRegisterInfo::Register> &regs) {
  for (DynamicRegisterInfo::Register &info : regs) {
    if (info.regnum_ehframe != LLDB_INVALID_REGNUM &&
        info.regnum_dwarf != LLDB_INVALID_REGNUM)
      continue;

    RegisterInfo abi_info;
    if (!GetRegisterInfoByName(info.name.GetStringRef(), abi_info))
      continue;

    if (info.regnum_ehframe == LLDB_INVALID_REGNUM)
      info.regnum_ehframe = abi_info.kinds[eRegisterKindEHFrame];
    if (info.regnum_dwarf == LLDB_INVALID_REGNUM)
      info.regnum_dwarf = abi_info.kinds[eRegisterKindDWARF];
    if (info.regnum_generic == LLDB_INVALID_REGNUM)
      info.regnum_generic = abi_info.kinds[eRegisterKindGeneric];
  }
}

void MCBasedABI::AugmentRegisterInfo(
    std::vector<DynamicRegisterInfo::Register> &regs) {
  for (DynamicRegisterInfo::Register &info : regs) {
    uint32_t eh, dwarf;
    std::tie(eh, dwarf) = GetEHAndDWARFNums(info.name.GetStringRef());

    if (info.regnum_ehframe == LLDB_INVALID_REGNUM)
      info.regnum_ehframe = eh;
    if (info.regnum_dwarf == LLDB_INVALID_REGNUM)
      info.regnum_dwarf = dwarf;
    if (info.regnum_generic == LLDB_INVALID_REGNUM)
      info.regnum_generic = GetGenericNum(info.name.GetStringRef());
  }
}

std::pair<uint32_t, uint32_t>
MCBasedABI::GetEHAndDWARFNums(llvm::StringRef name) {
  std::string mc_name = GetMCName(name.str());
  for (char &c : mc_name)
    c = std::toupper(c);
  int eh = -1;
  int dwarf = -1;
  for (unsigned reg = 0; reg < m_mc_register_info_up->getNumRegs(); ++reg) {
    if (m_mc_register_info_up->getName(reg) == mc_name) {
      eh = m_mc_register_info_up->getDwarfRegNum(reg, /*isEH=*/true);
      dwarf = m_mc_register_info_up->getDwarfRegNum(reg, /*isEH=*/false);
      break;
    }
  }
  return std::pair<uint32_t, uint32_t>(eh == -1 ? LLDB_INVALID_REGNUM : eh,
                                       dwarf == -1 ? LLDB_INVALID_REGNUM
                                                   : dwarf);
}

void MCBasedABI::MapRegisterName(std::string &name, llvm::StringRef from_prefix,
                                 llvm::StringRef to_prefix) {
  llvm::StringRef name_ref = name;
  if (!name_ref.consume_front(from_prefix))
    return;
  uint64_t _;
  if (name_ref.empty() || to_integer(name_ref, _, 10))
    name = (to_prefix + name_ref).str();
}

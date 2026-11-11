//===-- ABIXbox360.cpp
//-----------------------------------------------------===//
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
#include <algorithm>

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

bool IsCodeAddressValid(addr_t pc) {
  return pc && pc <= UINT32_MAX && (pc & 3) == 0;
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
  if (!triple.isOSXbox360() || (triple.getArch() != llvm::Triple::ppc &&
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
  return Fail("PrepareTrivialCall is not implemented for Xbox 360 ABI");
}

bool ABIXbox360::GetArgumentValues(Thread &thread, ValueList &values) const {
  return Fail("GetArgumentValues is not implemented for Xbox 360 ABI");
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
  return IsCodeAddressValid(pc);
}

Status ABIXbox360::SetReturnValueObject(StackFrameSP &frame_sp,
                                        ValueObjectSP &new_value) {
  return Status::FromErrorString(
      "Setting Xbox 360 return values is not implemented");
}

ValueObjectSP ABIXbox360::GetReturnValueObjectImpl(Thread &thread,
                                                   CompilerType &type) const {
  Fail("reading Xbox 360 return values is not implemented");
  return {};
}

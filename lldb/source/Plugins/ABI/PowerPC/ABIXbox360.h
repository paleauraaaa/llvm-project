//===-- ABIXbox360.h ---------------------------------------------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_ABI_POWERPC_ABIXBOX360_H
#define LLDB_SOURCE_PLUGINS_ABI_POWERPC_ABIXBOX360_H

#include "lldb/Target/ABI.h"

/// Xbox's 32-bit pointer ABI with 64-bit GPRs and VMX128 registers.
class ABIXbox360 : public lldb_private::ABI {
public:
  static void Initialize();
  static void Terminate();
  static lldb::ABISP CreateInstance(lldb::ProcessSP process_sp,
                                    const lldb_private::ArchSpec &arch);
  static llvm::StringRef GetPluginNameStatic() { return "xbox360"; }
  llvm::StringRef GetPluginName() override { return GetPluginNameStatic(); }

  size_t GetRedZoneSize() const override { return 1344; }

  bool PrepareTrivialCall(lldb_private::Thread &thread, lldb::addr_t sp,
                          lldb::addr_t function_address,
                          lldb::addr_t return_address,
                          llvm::ArrayRef<lldb::addr_t> args) const override;

  /// Decode standard prototyped scalar and vector arguments at function entry.
  /// ValueList does not describe varargs, hidden arguments or C++ ABI lowering.
  bool GetArgumentValues(lldb_private::Thread &thread,
                         lldb_private::ValueList &values) const override;

  lldb_private::Status
  SetReturnValueObject(lldb::StackFrameSP &frame_sp,
                       lldb::ValueObjectSP &new_value) override;

  bool
  CreateFunctionEntryUnwindPlan(lldb_private::UnwindPlan &unwind_plan) override;
  bool CreateDefaultUnwindPlan(lldb_private::UnwindPlan &unwind_plan) override;
  bool RegisterIsVolatile(const lldb_private::RegisterInfo *reg_info) override;
  bool CallFrameAddressIsValid(lldb::addr_t cfa) override;
  bool CodeAddressIsValid(lldb::addr_t pc) override;
  void AugmentRegisterInfo(
      std::vector<lldb_private::DynamicRegisterInfo::Register> &regs) override;

protected:
  lldb::ValueObjectSP
  GetReturnValueObjectImpl(lldb_private::Thread &thread,
                           lldb_private::CompilerType &type) const override;

private:
  using ABI::ABI;
};

#endif

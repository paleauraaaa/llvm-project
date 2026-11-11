//===-- CodeViewRegisterMappingTests.cpp
//-----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/SymbolFile/NativePDB/CodeViewRegisterMapping.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/lldb-defines.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::npdb;
using llvm::codeview::RegisterId;

TEST(CodeViewRegisterMapping, XboxStableRegisterNumbers) {
  for (const char *triple : {"powerpc-ibm-xbox360", "powerpc64-ibm-xbox360"}) {
    ArchSpec arch(triple);
    auto check = [&](RegisterId cv, RegisterKind expected_kind,
                     unsigned expected_number, unsigned expected_size) {
      RegisterKind kind;
      EXPECT_EQ(GetRegisterNumber(arch, cv, kind), expected_number);
      EXPECT_EQ(kind, expected_kind);
      EXPECT_EQ(GetRegisterSize(arch, cv), expected_size);
    };
    for (unsigned i = 0; i < 32; ++i) {
      check(static_cast<RegisterId>(unsigned(RegisterId::PPC_GPR0) + i),
            eRegisterKindDWARF, i, 8);
      check(static_cast<RegisterId>(unsigned(RegisterId::PPC_FPR0) + i),
            eRegisterKindDWARF, 32 + i, 8);
    }
    for (unsigned i = 0; i < 8; ++i)
      check(static_cast<RegisterId>(unsigned(RegisterId::PPC_CR0) + i),
            eRegisterKindDWARF, 68 + i, 4);
    for (unsigned i = 0; i < 128; ++i) {
      check(static_cast<RegisterId>(unsigned(RegisterId::PPC_VR0) + 5 * i),
            eRegisterKindDWARF, i < 32 ? 77 + i : 1268 + i, 16);
      for (unsigned lane = 1; lane < 5; ++lane) {
        RegisterId cv = static_cast<RegisterId>(unsigned(RegisterId::PPC_VR0) +
                                                5 * i + lane);
        RegisterKind kind;
        EXPECT_EQ(GetRegisterNumber(arch, cv, kind), LLDB_INVALID_REGNUM);
        EXPECT_EQ(GetRegisterSize(arch, cv), 0u);
      }
    }
    check(RegisterId::PPC_LR, eRegisterKindDWARF, 65, 8);
    check(RegisterId::PPC_CTR, eRegisterKindDWARF, 66, 8);
    check(RegisterId::PPC_XER, eRegisterKindDWARF, 76, 4);
    check(RegisterId::PPC_CR, eRegisterKindGeneric, LLDB_REGNUM_GENERIC_FLAGS,
          4);
    check(RegisterId::PPC_PC, eRegisterKindGeneric, LLDB_REGNUM_GENERIC_PC, 4);
    for (RegisterId cv :
         {RegisterId::NONE, RegisterId::VFRAME, RegisterId::PPC_MQ,
          RegisterId::PPC_SR0, RegisterId::PPC_FPSCR, RegisterId::PPC_MSR}) {
      RegisterKind kind;
      EXPECT_EQ(GetRegisterNumber(arch, cv, kind), LLDB_INVALID_REGNUM);
      EXPECT_EQ(GetRegisterSize(arch, cv), 0u);
    }
  }
}

TEST(CodeViewRegisterMapping, OtherArchitecturesKeepNativeNumbering) {
  for (const char *triple :
       {"i386-pc-windows", "x86_64-pc-windows", "aarch64-pc-windows"}) {
    ArchSpec arch(triple);
    RegisterId cv = arch.GetMachine() == llvm::Triple::aarch64
                        ? RegisterId::ARM64_X0
                        : RegisterId::EAX;
    RegisterKind kind;
    EXPECT_EQ(GetRegisterNumber(arch, cv, kind),
              GetLLDBRegisterNumber(arch.GetMachine(), cv));
    EXPECT_EQ(kind, eRegisterKindLLDB);
    EXPECT_EQ(GetRegisterSize(arch, cv), GetRegisterSize(cv));
  }
  RegisterKind kind;
  EXPECT_EQ(GetRegisterNumber(ArchSpec("powerpc-unknown-linux"),
                              RegisterId::PPC_GPR3, kind),
            LLDB_INVALID_REGNUM);
}

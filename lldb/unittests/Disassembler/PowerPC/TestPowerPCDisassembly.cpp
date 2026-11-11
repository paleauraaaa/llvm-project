//===-- TestPowerPCDisassembly.cpp
//-----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/Disassembler/LLVMC/DisassemblerLLVMC.h"
#include "lldb/Core/Address.h"
#include "lldb/Core/Disassembler.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Utility/ArchSpec.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/TargetSelect.h"
#include "gtest/gtest.h"
#include <utility>

using namespace lldb;
using namespace lldb_private;

namespace {
class TestPowerPCDisassembly : public testing::Test {
public:
  static void SetUpTestSuite() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllDisassemblers();
    DisassemblerLLVMC::Initialize();
  }

  static void TearDownTestSuite() { DisassemblerLLVMC::Terminate(); }

protected:
  DisassemblerSP Decode(const char *triple, llvm::ArrayRef<uint8_t> bytes,
                        const char *cpu = nullptr,
                        const char *features = nullptr) {
    return Disassembler::DisassembleBytes(
        ArchSpec(triple), nullptr, nullptr, cpu, features, Address(0x82001000),
        bytes.data(), bytes.size(), bytes.size() / 4, false);
  }

  void CheckInstruction(const DisassemblerSP &disassembler, size_t index,
                        const char *mnemonic, const char *operands) {
    auto instruction =
        disassembler->GetInstructionList().GetInstructionAtIndex(index);
    ASSERT_TRUE(instruction);
    ExecutionContext context(nullptr, nullptr, nullptr);
    EXPECT_STREQ(instruction->GetMnemonic(&context), mnemonic);
    EXPECT_STREQ(instruction->GetOperands(&context), operands);
    EXPECT_EQ(instruction->GetAddress().GetFileAddress(),
              0x82001000u + index * 4);
  }
};

// This word is also a valid standard PPC vinsw instruction.
constexpr uint8_t g_overlapping_opcode[] = {0x13, 0xe3, 0x20, 0xcf};
} // namespace

TEST_F(TestPowerPCDisassembly, XboxDefaultsToXenon) {
  const uint8_t bytes[] = {
      0x13, 0xe3, 0x20, 0xcf, // lvx128 127, 3, 4
      0x17, 0xe0, 0x04, 0x3e, // vaddfp128 127, 96, 64
      0x10, 0x22, 0x18, 0x0a, // vaddfp 1, 2, 3
      0xe8, 0x64, 0x00, 0x00, // ld 3, 0(4)
      0xf8, 0x64, 0x00, 0x08, // std 3, 8(4)
      0xec, 0x20, 0x10, 0x30, // fres 1, 2
      0x4e, 0x80, 0x00, 0x20, // blr
  };
  for (const char *triple : {"powerpc-ibm-xbox360", "powerpc64-ibm-xbox360"}) {
    SCOPED_TRACE(triple);
    if (ArchSpec(triple).GetMachine() == llvm::Triple::ppc)
      EXPECT_EQ(ArchSpec(triple).GetAddressByteSize(), 4u);
    EXPECT_EQ(ArchSpec(triple).GetByteOrder(), eByteOrderBig);
    auto disassembler = Decode(triple, bytes);
    ASSERT_TRUE(disassembler);
    ASSERT_EQ(disassembler->GetInstructionList().GetSize(), 7u);
    CheckInstruction(disassembler, 0, "lvx128", "127, 3, 4");
    CheckInstruction(disassembler, 1, "vaddfp128", "127, 96, 64");
    CheckInstruction(disassembler, 2, "vaddfp", "1, 2, 3");
    CheckInstruction(disassembler, 3, "ld", "3, 0(4)");
    CheckInstruction(disassembler, 4, "std", "3, 8(4)");
    CheckInstruction(disassembler, 5, "fres", "1, 2");
    CheckInstruction(disassembler, 6, "blr", "");
  }
}

TEST_F(TestPowerPCDisassembly, ExplicitOverridesArePreserved) {
  for (const char *triple : {"powerpc-ibm-xbox360", "powerpc64-ibm-xbox360"}) {
    for (auto [cpu, features] :
         {std::pair<const char *, const char *>{"g5", nullptr},
          {nullptr, "+altivec"},
          {"xenon", "-vmx128"}}) {
      auto disassembler = Decode(triple, g_overlapping_opcode, cpu, features);
      ASSERT_TRUE(disassembler);
      CheckInstruction(disassembler, 0, "vinsw", "31, 4, 3");
    }
    auto disassembler = Decode(triple, g_overlapping_opcode, "g5", "+vmx128");
    ASSERT_TRUE(disassembler);
    CheckInstruction(disassembler, 0, "lvx128", "127, 3, 4");
  }
}

TEST_F(TestPowerPCDisassembly, NonXboxPPCDoesNotDefaultToXenon) {
  for (const char *triple :
       {"powerpc-unknown-linux", "powerpc64-unknown-linux"}) {
    auto disassembler = Decode(triple, g_overlapping_opcode);
    ASSERT_TRUE(disassembler);
    CheckInstruction(disassembler, 0, "vinsw", "31, 4, 3");
  }
}

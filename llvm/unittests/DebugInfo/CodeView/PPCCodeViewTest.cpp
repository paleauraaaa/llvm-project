//===- PPCCodeViewTest.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/DebugInfo/CodeView/EnumTables.h"
#include "llvm/Support/ScopedPrinter.h"
#include "gtest/gtest.h"

using namespace llvm::codeview;

TEST(PPCCodeViewTest, RegisterIDs) {
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_GPR0), 1);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_GPR31), 32);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_CR), 33);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_CR0), 34);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_CR7), 41);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_FPR0), 42);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_FPR31), 73);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_FPSCR), 74);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_XER), 101);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_LR), 108);
  EXPECT_EQ(static_cast<uint16_t>(RegisterId::PPC_CTR), 109);
}

TEST(PPCCodeViewTest, RegisterNames) {
  llvm::ArrayRef<llvm::EnumEntry<uint16_t>> Names =
      getRegisterNames(CPUType::PPC604);
  EXPECT_EQ(Names[0].Name, "PPC_GPR0");
  EXPECT_EQ(Names[0].Value, 1);
  EXPECT_EQ(Names[31].Name, "PPC_GPR31");
  EXPECT_EQ(Names[31].Value, 32);

  Names = getRegisterNames(CPUType::PPC620);
  EXPECT_EQ(Names[0].Name, "PPC_GPR0");
  EXPECT_EQ(Names[0].Value, 1);
}

TEST(PPCCodeViewTest, FramePointerRegisters) {
  for (CPUType CPU : {CPUType::PPC601, CPUType::PPC603, CPUType::PPC604,
                      CPUType::PPC620, CPUType::PPCFP, CPUType::PPCBE}) {
    EXPECT_EQ(decodeFramePtrReg(EncodedFramePtrReg::StackPtr, CPU),
              RegisterId::PPC_GPR1);
    EXPECT_EQ(decodeFramePtrReg(EncodedFramePtrReg::FramePtr, CPU),
              RegisterId::PPC_GPR31);
    EXPECT_EQ(decodeFramePtrReg(EncodedFramePtrReg::BasePtr, CPU),
              RegisterId::PPC_GPR30);

    EXPECT_EQ(encodeFramePtrReg(RegisterId::PPC_GPR1, CPU),
              EncodedFramePtrReg::StackPtr);
    EXPECT_EQ(encodeFramePtrReg(RegisterId::PPC_GPR31, CPU),
              EncodedFramePtrReg::FramePtr);
    EXPECT_EQ(encodeFramePtrReg(RegisterId::PPC_GPR30, CPU),
              EncodedFramePtrReg::BasePtr);
  }
}

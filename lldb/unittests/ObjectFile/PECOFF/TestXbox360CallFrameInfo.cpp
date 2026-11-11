//===-- TestXbox360CallFrameInfo.cpp
//---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/ObjectFile/PECOFF/ObjectFilePECOFF.h"
#include "TestingSupport/SubsystemRAII.h"
#include "TestingSupport/TestUtilities.h"
#include "lldb/Core/Module.h"
#include "lldb/Symbol/CallFrameInfo.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;
using namespace llvm::dwarf;

class Xbox360CallFrameInfoTest : public testing::Test {
  SubsystemRAII<FileSystem, ObjectFilePECOFF> subsystems;

protected:
  void Load(llvm::ArrayRef<uint32_t> code, llvm::ArrayRef<uint32_t> pdata,
            std::optional<unsigned> directory_size = std::nullopt) {
    auto hex = [](llvm::ArrayRef<uint32_t> words) {
      std::string result;
      for (uint32_t word : words)
        result += llvm::formatv("{0:X-8}", word).str();
      return result;
    };
    auto file = TestFile::fromYaml(
        llvm::formatv(R"(
--- !COFF
OptionalHeader:
  ImageBase: 0x82000000
  AddressOfEntryPoint: 4096
  SectionAlignment: 4096
  FileAlignment: 512
  Subsystem: IMAGE_SUBSYSTEM_XBOX
  ExceptionTable:
    RelativeVirtualAddress: 8192
    Size: {0}
header:
  Machine: IMAGE_FILE_MACHINE_I386
  Characteristics: [ IMAGE_FILE_EXECUTABLE_IMAGE ]
sections:
  - Name: .text
    VirtualAddress: 4096
    VirtualSize: {1}
    Characteristics: [ IMAGE_SCN_CNT_CODE, IMAGE_SCN_MEM_READ, IMAGE_SCN_MEM_EXECUTE ]
    SectionData: {2}
  - Name: .pdata
    VirtualAddress: 8192
    VirtualSize: {3}
    Characteristics: [ IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_MEM_READ ]
    SectionData: {4}
symbols: []
...
)",
                      directory_size.value_or(pdata.size() * 4),
                      code.size() * 4, hex(code), pdata.size() * 4, hex(pdata))
            .str());
    ASSERT_THAT_EXPECTED(file, llvm::Succeeded());
    // Xbox images use PE32, independently of COFF::is64Bit(PPCBE).
    auto data = std::make_shared<DataBufferHeap>(*file->moduleSpec().GetData());
    uint32_t pe = llvm::support::endian::read32le(data->GetBytes() + 0x3c);
    llvm::support::endian::write16le(data->GetBytes() + pe + 4,
                                     llvm::COFF::IMAGE_FILE_MACHINE_PPCBE);
    module = std::make_shared<Module>(ModuleSpec(FileSpec(), UUID(), data));
    object = llvm::dyn_cast_or_null<ObjectFilePECOFF>(module->GetObjectFile());
    ASSERT_NE(object, nullptr);
    cfi = object->CreateCallFrameInfo();
    ASSERT_TRUE(cfi);
  }

  bool Plan(unsigned offset, UnwindPlan &plan) {
    return cfi->GetUnwindPlan(object->GetAddress(0x1000 + offset), plan);
  }

  void AtCFA(const UnwindPlan::RowSP &row, unsigned reg, int32_t offset) {
    UnwindPlan::Row::AbstractRegisterLocation location;
    ASSERT_TRUE(row->GetRegisterInfo(reg, location));
    ASSERT_TRUE(location.IsAtCFAPlusOffset());
    EXPECT_EQ(location.GetOffset(), offset);
  }

  void Expression(const UnwindPlan::RowSP &row, unsigned reg,
                  llvm::ArrayRef<uint8_t> expected) {
    UnwindPlan::Row::AbstractRegisterLocation location;
    ASSERT_TRUE(row->GetRegisterInfo(reg, location));
    ASSERT_TRUE(location.IsDWARFExpression());
    const uint8_t *bytes;
    uint16_t size;
    location.GetDWARFExpr(&bytes, size);
    EXPECT_EQ(llvm::ArrayRef(bytes, size), expected);
  }

  ModuleSP module;
  ObjectFilePECOFF *object = nullptr;
  std::unique_ptr<lldb_private::CallFrameInfo> cfi;
};

TEST_F(Xbox360CallFrameInfoTest, LeafAndRangeLookup) {
  Load({0x38600001, 0x4e800020, 0x38600002, 0x4e800020},
       {0, 0, 0x82001000, 0x40000200, 0x82001008, 0xc0000200});
  ASSERT_TRUE(cfi);
  AddressRange range;
  ASSERT_TRUE(cfi->GetAddressRange(object->GetAddress(0x1007), range));
  EXPECT_EQ(range.GetBaseAddress().GetFileAddress(), 0x82001000u);
  EXPECT_EQ(range.GetByteSize(), 8u);
  ASSERT_TRUE(cfi->GetAddressRange(object->GetAddress(0x1008), range));
  EXPECT_EQ(range.GetBaseAddress().GetFileAddress(), 0x82001008u);
  EXPECT_FALSE(cfi->GetAddressRange(object->GetAddress(0x1010), range));
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(Plan(0, plan));
  EXPECT_EQ(plan.GetRegisterKind(), eRegisterKindDWARF);
  EXPECT_EQ(plan.GetReturnAddressRegister(), 65u);
  auto row = plan.GetRowAtIndex(0);
  ASSERT_TRUE(row);
  EXPECT_EQ(row->GetCFAValue().GetRegisterNumber(), 1u);
  EXPECT_EQ(row->GetCFAValue().GetOffset(), 0);
  UnwindPlan::Row::AbstractRegisterLocation lr;
  ASSERT_TRUE(row->GetRegisterInfo(65, lr));
  EXPECT_TRUE(lr.IsInOtherRegister());
  EXPECT_EQ(lr.GetRegisterNumber(), 65u);
}

TEST_F(Xbox360CallFrameInfoTest, MixedWidthsAndPartialPrologue) {
  // mflr r12; stw r12,-8(r1); std r31,-16(r1); stfd f31,-24(r1);
  // mfcr r12; stw r12,-28(r1); stwu r1,-128(r1); mr r31,r1; blr
  Load({0x7d8802a6, 0x9181fff8, 0xfbe1fff0, 0xdbe1ffe8, 0x7d800026, 0x9181ffe4,
        0x9421ff80, 0x7c3f0b78, 0x4e800020},
       {0x82001000, 0x40000908});
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(Plan(32, plan));
  ASSERT_EQ(plan.GetRowCount(), 9);
  auto before_save = plan.GetRowForFunctionOffset(4);
  UnwindPlan::Row::AbstractRegisterLocation lr;
  ASSERT_TRUE(before_save->GetRegisterInfo(65, lr));
  EXPECT_TRUE(lr.IsInOtherRegister());
  Expression(plan.GetRowForFunctionOffset(8), 65,
             {DW_OP_consts, 0x78, DW_OP_plus, DW_OP_deref_size, 4});
  auto body = plan.GetLastRow();
  EXPECT_EQ(body->GetCFAValue().GetRegisterNumber(), 31u);
  EXPECT_EQ(body->GetCFAValue().GetOffset(), 128);
  AtCFA(body, 31, -16);
  AtCFA(body, 63, -24);
  for (unsigned field = 2; field <= 5; ++field)
    Expression(body, 68 + field,
               {DW_OP_consts, 0x64, DW_OP_plus, DW_OP_deref_size, 4,
                uint8_t(DW_OP_lit0 + (7 - field) * 4), DW_OP_shr, DW_OP_lit15,
                DW_OP_and});
}

TEST_F(Xbox360CallFrameInfoTest, ExtendedVectorAndPostAllocationSaves) {
  // stwu r1,-256(r1); li r11,32; stvx128 v127,r1,r11; std r30,48(r1)
  Load({0x9421ff00, 0x39600020, 0x13e159cf, 0xfbc10030, 0x4e800020},
       {0x82001000, 0x40000504});
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(Plan(16, plan));
  auto row = plan.GetLastRow();
  EXPECT_EQ(row->GetCFAValue().GetOffset(), 256);
  AtCFA(row, 1395, -224);
  AtCFA(row, 30, -208);
}

TEST_F(Xbox360CallFrameInfoTest, LargeFrameAndSaveHelper) {
  // mflr r12; bl helper; lis r11,-1; ori r11,r11,0x8000;
  // stwux r1,r1,r11; blr
  // helper: std r30,-16(r1); std r31,-8(r1); stw r12,-20(r1); blr
  Load({0x7d8802a6, 0x48000015, 0x3d60ffff, 0x616b8000, 0x7c21596e, 0x4e800020,
        0xfbc1fff0, 0xfbe1fff8, 0x9181ffec, 0x4e800020},
       {0x82001000, 0x40000605});
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(Plan(20, plan));
  EXPECT_EQ(plan.GetLastRow()->GetCFAValue().GetOffset(), 32768);
  AtCFA(plan.GetLastRow(), 30, -16);
  AtCFA(plan.GetLastRow(), 31, -8);
  Expression(plan.GetLastRow(), 65,
             {DW_OP_consts, 0x6c, DW_OP_plus, DW_OP_deref_size, 4});
}

TEST_F(Xbox360CallFrameInfoTest, RejectsInvalidMetadata) {
  for (uint32_t packed : {0x00000200u, 0x40000000u, 0x40000203u}) {
    Load({0x38600001, 0x4e800020}, {0x82001000, packed});
    UnwindPlan plan(eRegisterKindDWARF);
    EXPECT_FALSE(Plan(0, plan));
    EXPECT_EQ(plan.GetRowCount(), 0);
  }
  Load({0x4e800020}, {0x82001000, 0x40000100}, 7);
  UnwindPlan plan(eRegisterKindDWARF);
  EXPECT_FALSE(Plan(0, plan));
  Load({0x4e800020}, {0x82001000, 0x40000200, 0x82001004, 0x40000100});
  EXPECT_FALSE(Plan(0, plan));
}

TEST_F(Xbox360CallFrameInfoTest, TracksRestoresThroughMultipleEpilogues) {
  // Two return paths, both restoring SP, LR and r31.
  Load({0x7d8802a6, 0x9181fff8, 0xfbe1fff0, 0x9421ff80, 0x48000101, 0x38210080,
        0x8181fff8, 0x7d8803a6, 0xebe1fff0, 0x4e800020, 0x48000101, 0x38210080,
        0x8181fff8, 0x7d8803a6, 0xebe1fff0, 0x4e800020},
       {0x82001000, 0x40001004});
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(Plan(16, plan));
  EXPECT_EQ(plan.GetRowForFunctionOffset(20)->GetCFAValue().GetOffset(), 128);
  EXPECT_EQ(plan.GetRowForFunctionOffset(24)->GetCFAValue().GetOffset(), 0);
  AtCFA(plan.GetRowForFunctionOffset(24), 31, -16);
  UnwindPlan::Row::AbstractRegisterLocation lr, r31;
  auto row = plan.GetRowForFunctionOffset(36);
  ASSERT_TRUE(row->GetRegisterInfo(65, lr));
  EXPECT_TRUE(lr.IsInOtherRegister());
  EXPECT_EQ(lr.GetRegisterNumber(), 65u);
  ASSERT_TRUE(row->GetRegisterInfo(31, r31));
  EXPECT_TRUE(r31.IsInOtherRegister());
  EXPECT_EQ(r31.GetRegisterNumber(), 31u);
  EXPECT_EQ(plan.GetRowForFunctionOffset(40)->GetCFAValue().GetOffset(), 128);
  EXPECT_EQ(plan.GetRowForFunctionOffset(64 - 4)->GetCFAValue().GetOffset(), 0);
}

TEST_F(Xbox360CallFrameInfoTest,
       DynamicAllocaUsesFramePointerAndWordBackchain) {
  // mr r31,r1 anchors the CFA across the body; lwz r1,0(r1) restores SP.
  Load({0x7d8802a6, 0x9181fff8, 0xfbe1fff0, 0x9421ff80, 0x7c3f0b78, 0x48000101,
        0x80210000, 0x8181fff8, 0x7d8803a6, 0xebe1fff0, 0x4e800020},
       {0x82001000, 0x40000b05});
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(Plan(20, plan));
  EXPECT_EQ(plan.GetRowForFunctionOffset(24)->GetCFAValue().GetRegisterNumber(),
            31u);
  EXPECT_EQ(plan.GetRowForFunctionOffset(24)->GetCFAValue().GetOffset(), 128);
  EXPECT_EQ(plan.GetLastRow()->GetCFAValue().GetRegisterNumber(), 1u);
  EXPECT_EQ(plan.GetLastRow()->GetCFAValue().GetOffset(), 0);
}

TEST_F(Xbox360CallFrameInfoTest, RejectsUnknownOrUnreadablePrologues) {
  for (uint32_t instruction :
       {0xffffffffu, 0x48000000u, 0x48000001u, 0x94210010u, 0x93e1fff8u}) {
    Load({instruction, 0x4e800020}, {0x82001000, 0x40000201});
    UnwindPlan plan(eRegisterKindDWARF);
    EXPECT_FALSE(Plan(0, plan));
    EXPECT_EQ(plan.GetRowCount(), 0);
  }
  Load({0x60000000}, {0x82001000, 0x40000202});
  UnwindPlan plan(eRegisterKindDWARF);
  EXPECT_FALSE(Plan(0, plan));
}

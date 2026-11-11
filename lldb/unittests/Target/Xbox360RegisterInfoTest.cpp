//===-- Xbox360RegisterInfoTest.cpp
//----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Target/Xbox360RegisterInfo.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/RegisterValue.h"
#include "llvm/Support/Endian.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;

TEST(Xbox360RegisterInfo, CompleteDescription) {
  auto registers = GetXbox360RegisterInfo();
  ASSERT_EQ(registers.size(), 32u + 32u + 128u + 9u + 8u);
  for (unsigned i = 0; i != registers.size(); ++i) {
    EXPECT_EQ(registers[i].regnum_remote, LLDB_INVALID_REGNUM);
    EXPECT_EQ(registers[i].byte_offset, LLDB_INVALID_INDEX32);
    for (unsigned j = 0; j != i; ++j)
      EXPECT_NE(registers[i].name, registers[j].name);
  }
  for (unsigned i = 0; i != 32; ++i) {
    EXPECT_EQ(registers[i].name, ConstString("r" + std::to_string(i)));
    EXPECT_EQ(registers[i].byte_size, 8u);
    EXPECT_EQ(registers[i].regnum_dwarf, i);
    EXPECT_EQ(registers[32 + i].byte_size, 8u);
    EXPECT_EQ(registers[32 + i].encoding, eEncodingIEEE754);
    EXPECT_EQ(registers[32 + i].regnum_dwarf, 32 + i);
  }
  for (unsigned i = 0; i != 128; ++i) {
    const auto &reg = registers[64 + i];
    EXPECT_EQ(reg.name, ConstString("v" + std::to_string(i)));
    EXPECT_EQ(reg.byte_size, 16u);
    EXPECT_EQ(reg.encoding, eEncodingVector);
    EXPECT_EQ(reg.regnum_dwarf, i < 32 ? 77 + i : 1268 + i);
  }
  EXPECT_EQ(registers[13].regnum_generic, LLDB_REGNUM_GENERIC_TP);
  EXPECT_EQ(registers[192].name, ConstString("pc"));
  EXPECT_EQ(registers[192].byte_size, 4u);
  EXPECT_EQ(registers[193].name, ConstString("lr"));
  EXPECT_EQ(registers[193].regnum_dwarf, 65u);
  EXPECT_EQ(registers[194].regnum_dwarf, 66u);
  EXPECT_EQ(registers[196].regnum_dwarf, 76u);
  for (unsigned i = 0; i != 8; ++i)
    EXPECT_EQ(registers[201 + i].regnum_dwarf, 68 + i);
}

TEST(Xbox360RegisterInfo, RemoteLayoutRemainsAuthoritative) {
  const ArchSpec arch("powerpc-unknown-xbox360");
  ASSERT_EQ(arch.GetAddressByteSize(), 4u);
  std::vector<DynamicRegisterInfo::Register> registers;
  for (const char *name : {"vr127", "pc", "lr", "r1", "r3", "cr", "unknown"}) {
    DynamicRegisterInfo::Register reg;
    reg.name = ConstString(name);
    reg.set_name = ConstString("remote");
    reg.byte_size = 4;
    reg.byte_offset = registers.size() * 4;
    reg.regnum_remote = 100 + registers.size();
    registers.push_back(reg);
  }
  // Preserve explicit server mappings, including ones unknown to LLDB.
  registers.back().regnum_dwarf = 9000;
  registers.back().regnum_ehframe = 9001;
  registers.back().regnum_generic = LLDB_REGNUM_GENERIC_ARG8;
  ASSERT_TRUE(AugmentXbox360RegisterInfo(arch, registers));
  ASSERT_EQ(registers.size(), 7u);
  for (unsigned i = 0; i != registers.size(); ++i) {
    EXPECT_EQ(registers[i].byte_size, 4u);
    EXPECT_EQ(registers[i].byte_offset, i * 4);
    EXPECT_EQ(registers[i].regnum_remote, 100 + i);
    EXPECT_EQ(registers[i].set_name, ConstString("remote"));
  }
  EXPECT_EQ(registers[0].regnum_dwarf, 1395u);
  EXPECT_EQ(registers[1].regnum_generic, LLDB_REGNUM_GENERIC_PC);
  EXPECT_EQ(registers[2].regnum_generic, LLDB_REGNUM_GENERIC_RA);
  EXPECT_EQ(registers[3].regnum_generic, LLDB_REGNUM_GENERIC_SP);
  EXPECT_EQ(registers[4].regnum_generic, LLDB_REGNUM_GENERIC_ARG1);
  EXPECT_EQ(registers[5].regnum_generic, LLDB_REGNUM_GENERIC_FLAGS);
  EXPECT_EQ(registers[6].regnum_dwarf, 9000u);
  EXPECT_EQ(registers[6].regnum_ehframe, 9001u);
  EXPECT_EQ(registers[6].regnum_generic, LLDB_REGNUM_GENERIC_ARG8);

  DynamicRegisterInfo info;
  ASSERT_EQ(info.SetRegisterInfo(std::move(registers), arch), 7u);
  EXPECT_EQ(info.ConvertRegisterKindToRegisterNumber(eRegisterKindDWARF, 1395),
            0u);
  EXPECT_EQ(info.ConvertRegisterKindToRegisterNumber(eRegisterKindGeneric,
                                                     LLDB_REGNUM_GENERIC_SP),
            3u);
}

TEST(Xbox360RegisterInfo, DoesNotChangeOtherTargets) {
  for (const char *triple : {"powerpc-unknown-linux", "powerpc64-unknown-linux",
                             "i386-pc-windows"}) {
    auto registers = GetXbox360RegisterInfo();
    registers[0].regnum_dwarf = LLDB_INVALID_REGNUM;
    EXPECT_FALSE(AugmentXbox360RegisterInfo(ArchSpec(triple), registers));
    EXPECT_EQ(registers[0].regnum_dwarf, LLDB_INVALID_REGNUM);
  }
}

TEST(Xbox360RegisterInfo, AugmentsThreadPointerWithoutChangingWireLayout) {
  DynamicRegisterInfo::Register reg;
  reg.name = ConstString("r13");
  reg.byte_size = 8;
  reg.byte_offset = 104;
  reg.regnum_remote = 13;
  std::vector<DynamicRegisterInfo::Register> registers{reg};
  ASSERT_TRUE(
      AugmentXbox360RegisterInfo(ArchSpec("powerpc-ibm-xbox360"), registers));
  EXPECT_EQ(registers[0].regnum_generic, LLDB_REGNUM_GENERIC_TP);
  EXPECT_EQ(registers[0].regnum_dwarf, 13u);
  EXPECT_EQ(registers[0].byte_size, 8u);
  EXPECT_EQ(registers[0].byte_offset, 104u);
  EXPECT_EQ(registers[0].regnum_remote, 13u);
}

TEST(Xbox360RegisterInfo, FullWidthBigEndianValues) {
  const ArchSpec arch("powerpc-unknown-xbox360");
  auto registers = GetXbox360RegisterInfo();
  for (unsigned i = 0; i != registers.size(); ++i)
    registers[i].regnum_remote = i;
  DynamicRegisterInfo info;
  info.SetRegisterInfo(std::move(registers), arch);
  EXPECT_EQ(arch.GetAddressByteSize(), 4u);

  std::vector<uint8_t> bytes(info.GetRegisterDataByteSize(), 0);
  const RegisterInfo *r0 = info.GetRegisterInfo(eRegisterKindDWARF, 0);
  ASSERT_NE(r0, nullptr);
  ASSERT_EQ(r0->byte_size, 8u);
  llvm::support::endian::write64be(bytes.data() + r0->byte_offset,
                                   0x1234567887654321ULL);
  for (unsigned i : {0u, 31u, 32u, 127u}) {
    const RegisterInfo *vector =
        info.GetRegisterInfo(eRegisterKindDWARF, i < 32 ? 77 + i : 1268 + i);
    ASSERT_NE(vector, nullptr);
    ASSERT_EQ(vector->byte_size, 16u);
    for (unsigned lane = 0; lane != 4; ++lane)
      llvm::support::endian::write32be(
          bytes.data() + vector->byte_offset + lane * 4, (i << 16) | lane);
  }
  DataExtractor data(bytes.data(), bytes.size(), eByteOrderBig, 4);
  RegisterValue value;
  ASSERT_TRUE(
      value.SetValueFromData(*r0, data, r0->byte_offset, false).Success());
  EXPECT_EQ(value.GetAsUInt64(), 0x1234567887654321ULL);
  for (unsigned i : {0u, 31u, 32u, 127u}) {
    const RegisterInfo *vector =
        info.GetRegisterInfo(eRegisterKindDWARF, i < 32 ? 77 + i : 1268 + i);
    ASSERT_TRUE(
        value.SetValueFromData(*vector, data, vector->byte_offset, false)
            .Success());
    DataExtractor lanes(value.GetBytes(), 16, value.GetByteOrder(), 4);
    offset_t offset = 0;
    for (unsigned lane = 0; lane != 4; ++lane)
      EXPECT_EQ(lanes.GetU32(&offset), (i << 16) | lane);
  }
}

//===-- TestByteOrder.cpp --------------------------------------------------===//
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
#include "lldb/Core/Section.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/DataExtractor.h"
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/Object/COFF.h"
#include "llvm/Support/Endian.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;

class PECOFFByteOrderTest : public testing::Test {
  SubsystemRAII<FileSystem, ObjectFilePECOFF> subsystems;

protected:
  void CheckByteOrder(llvm::COFF::MachineTypes machine, ByteOrder byte_order) {
    auto file = TestFile::fromYaml(R"(
--- !COFF
OptionalHeader:
  AddressOfEntryPoint: 4096
  ImageBase:       0x82000000
  SectionAlignment: 4096
  FileAlignment:   512
  Subsystem:       IMAGE_SUBSYSTEM_XBOX
header:
  Machine:         IMAGE_FILE_MACHINE_I386
  Characteristics: [ IMAGE_FILE_EXECUTABLE_IMAGE ]
sections:
  - Name:            .text
    VirtualAddress:  4096
    VirtualSize:     4
    Characteristics: [ IMAGE_SCN_CNT_CODE, IMAGE_SCN_MEM_EXECUTE, IMAGE_SCN_MEM_READ ]
    SectionData:     38600001
  - Name:            .data
    VirtualAddress:  8192
    VirtualSize:     4
    Characteristics: [ IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_MEM_READ, IMAGE_SCN_MEM_WRITE ]
    SectionData:     12345678
  - Name:            .pdata
    VirtualAddress:  12288
    VirtualSize:     8
    Characteristics: [ IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_MEM_READ ]
    SectionData:     8200100040000100
symbols: []
...
)");
    ASSERT_THAT_EXPECTED(file, llvm::Succeeded());

    // Generate PE32 using i386, since the YAML emitter selects PE32+ for PPCBE.
    // Changing only the machine preserves the Xbox 360 PE32 optional header layout.
    auto data =
        std::make_shared<DataBufferHeap>(*file->moduleSpec().GetData());
    ASSERT_GE(data->GetByteSize(), 0x40u);
    uint32_t pe_offset = llvm::support::endian::read32le(data->GetBytes() + 0x3c);
    ASSERT_GE(data->GetByteSize(),
              uint64_t(pe_offset) + 4 +
                  sizeof(llvm::object::coff_file_header) + 2);
    auto *coff_header = data->GetBytes() + pe_offset + 4;
    llvm::support::endian::write16le(coff_header, machine);
    EXPECT_EQ(llvm::support::endian::read16le(
                  coff_header + sizeof(llvm::object::coff_file_header)),
              llvm::COFF::PE32Header::PE32);

    auto module =
        std::make_shared<Module>(ModuleSpec(FileSpec(), UUID(), data));
    auto *object_file =
        llvm::dyn_cast_or_null<ObjectFilePECOFF>(module->GetObjectFile());
    ASSERT_NE(object_file, nullptr);
    EXPECT_EQ(object_file->GetByteOrder(), byte_order);
    EXPECT_EQ(object_file->GetAddressByteSize(), 4u);

    // Re-parsing must decode the headers as little-endian even after exposing
    // the section contents in target byte order.
    for (unsigned i = 0; i != 2; ++i) {
      ASSERT_TRUE(object_file->ParseHeader());
      EXPECT_EQ(object_file->GetBaseAddress().GetFileAddress(), 0x82000000u);
      EXPECT_EQ(object_file->GetEntryPointAddress().GetFileAddress(),
                0x82001000u);
    }

    auto *sections = object_file->GetSectionList();
    ASSERT_NE(sections, nullptr);
    ASSERT_EQ(sections->GetNumSections(0), 4u);
    auto data_section = sections->FindSectionByName(ConstString(".data"));
    ASSERT_NE(data_section, nullptr);
    EXPECT_EQ(data_section->GetFileAddress(), 0x82002000u);
    EXPECT_EQ(data_section->GetByteSize(), 4u);

    const uint32_t expected =
        byte_order == eByteOrderBig ? 0x12345678 : 0x78563412;
    DataExtractor section_data;
    ASSERT_EQ(object_file->ReadSectionData(data_section.get(), section_data),
              4u);
    EXPECT_EQ(section_data.GetByteOrder(), byte_order);
    EXPECT_EQ(section_data.GetAddressByteSize(), 4u);
    offset_t offset = 0;
    EXPECT_EQ(section_data.GetU32(&offset), expected);

    DataExtractor file_data;
    ASSERT_EQ(object_file->GetData(data_section->GetFileOffset(), 4, file_data),
              4u);
    EXPECT_EQ(file_data.GetByteOrder(), byte_order);
    offset = 0;
    EXPECT_EQ(file_data.GetU32(&offset), expected);

    DataExtractor image_data = object_file->ReadImageDataByRVA(8192, 4);
    ASSERT_EQ(image_data.GetByteSize(), 4u);
    EXPECT_EQ(image_data.GetByteOrder(), byte_order);
    EXPECT_EQ(image_data.GetAddressByteSize(), 4u);
    offset = 0;
    EXPECT_EQ(image_data.GetU32(&offset), expected);

    DataExtractor pdata = object_file->ReadImageDataByRVA(12288, 8);
    ASSERT_EQ(pdata.GetByteSize(), 8u);
    offset = 0;
    EXPECT_EQ(pdata.GetU32(&offset),
              byte_order == eByteOrderBig ? 0x82001000u : 0x00100082u);
    EXPECT_EQ(pdata.GetU32(&offset),
              byte_order == eByteOrderBig ? 0x40000100u : 0x00010040u);
  }
};

TEST_F(PECOFFByteOrderTest, PPCBE) {
  CheckByteOrder(llvm::COFF::IMAGE_FILE_MACHINE_PPCBE, eByteOrderBig);
}

TEST_F(PECOFFByteOrderTest, I386) {
  CheckByteOrder(llvm::COFF::IMAGE_FILE_MACHINE_I386, eByteOrderLittle);
}

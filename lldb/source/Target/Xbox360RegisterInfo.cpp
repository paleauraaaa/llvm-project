//===-- Xbox360RegisterInfo.cpp
//--------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Target/Xbox360RegisterInfo.h"
#include "lldb/Utility/ArchSpec.h"

using namespace lldb;
using namespace lldb_private;

std::vector<DynamicRegisterInfo::Register>
lldb_private::GetXbox360RegisterInfo() {
  std::vector<DynamicRegisterInfo::Register> registers;
  auto add =
      [&](llvm::StringRef name, llvm::StringRef alias, llvm::StringRef group,
          uint32_t size, uint32_t dwarf, uint32_t generic = LLDB_INVALID_REGNUM,
          Encoding encoding = eEncodingUint, Format format = eFormatHex) {
        DynamicRegisterInfo::Register reg;
        reg.name = ConstString(name);
        if (!alias.empty())
          reg.alt_name = ConstString(alias);
        reg.set_name = ConstString(group);
        reg.byte_size = size;
        reg.regnum_dwarf = dwarf;
        reg.regnum_ehframe = dwarf;
        reg.regnum_generic = generic;
        reg.encoding = encoding;
        reg.format = format;
        registers.push_back(reg);
      };

  for (unsigned i = 0; i != 32; ++i) {
    uint32_t generic = LLDB_INVALID_REGNUM;
    if (i == 1)
      generic = LLDB_REGNUM_GENERIC_SP;
    else if (i >= 3 && i <= 10)
      generic = LLDB_REGNUM_GENERIC_ARG1 + i - 3;
    add("r" + std::to_string(i), i == 1 ? "sp" : "", "General Purpose", 8, i,
        generic);
  }
  for (unsigned i = 0; i != 32; ++i)
    add("f" + std::to_string(i), "", "Floating Point", 8, 32 + i,
        LLDB_INVALID_REGNUM, eEncodingIEEE754, eFormatFloat);
  for (unsigned i = 0; i != 128; ++i)
    // Match the compiler's PPCRegisterInfo.td, including extended VMX128 regs.
    add("v" + std::to_string(i), "vr" + std::to_string(i), "Vector", 16,
        i < 32 ? 77 + i : 1268 + i, LLDB_INVALID_REGNUM, eEncodingVector,
        eFormatVectorOfUInt32);

  add("pc", "", "General Purpose", 4, LLDB_INVALID_REGNUM,
      LLDB_REGNUM_GENERIC_PC);
  add("lr", "ra", "General Purpose", 8, 65, LLDB_REGNUM_GENERIC_RA);
  add("ctr", "", "General Purpose", 8, 66);
  add("cr", "", "General Purpose", 4, LLDB_INVALID_REGNUM,
      LLDB_REGNUM_GENERIC_FLAGS);
  add("xer", "", "General Purpose", 4, 76);
  add("msr", "", "General Purpose", 8, LLDB_INVALID_REGNUM);
  add("fpscr", "", "Floating Point", 4, LLDB_INVALID_REGNUM);
  add("vscr", "", "Vector", 4, LLDB_INVALID_REGNUM);
  add("vrsave", "", "Vector", 4, 109);

  // CR fields are separate compiler debug registers, not byte slices of CR.
  // They may be supplied by a remote, but are not additional wire registers.
  for (unsigned i = 0; i != 8; ++i)
    add("cr" + std::to_string(i), "", "General Purpose", 4, 68 + i);
  return registers;
}

bool lldb_private::AugmentXbox360RegisterInfo(
    const ArchSpec &arch,
    std::vector<DynamicRegisterInfo::Register> &registers) {
  if (!arch.GetTriple().isOSXbox360() ||
      (arch.GetMachine() != llvm::Triple::ppc &&
       arch.GetMachine() != llvm::Triple::ppc64))
    return false;

  const auto descriptions = GetXbox360RegisterInfo();
  for (auto &reg : registers) {
    for (const auto &description : descriptions) {
      if (reg.name != description.name &&
          (!description.alt_name || reg.name != description.alt_name))
        continue;
      if (reg.regnum_dwarf == LLDB_INVALID_REGNUM)
        reg.regnum_dwarf = description.regnum_dwarf;
      if (reg.regnum_ehframe == LLDB_INVALID_REGNUM)
        reg.regnum_ehframe = description.regnum_ehframe;
      if (reg.regnum_generic == LLDB_INVALID_REGNUM)
        reg.regnum_generic = description.regnum_generic;
      if (!reg.alt_name && reg.name == description.name)
        reg.alt_name = description.alt_name;
      break;
    }
  }
  return true;
}

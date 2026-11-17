//===-- PPCWinCOFFObjectWriter.cpp - PowerPC WinCOFF Writer -------------------===//
//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/PPCFixupKinds.h"
#include "MCTargetDesc/PPCMCAsmInfo.h"
#include "MCTargetDesc/PPCMCTargetDesc.h"
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCWinCOFFObjectWriter.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

namespace {
class PPCWinCOFFObjectWriter : public MCWinCOFFObjectTargetWriter {
  static constexpr uint8_t SignBitMask = 0x80;

public:
  PPCWinCOFFObjectWriter();

  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                                const MCFixup &Fixup, bool IsCrossSection,
                                const MCAsmBackend &MAB) const override;
  bool recordRelocation(const MCFixup &) const override;
};
} // end anonymous namespace

PPCWinCOFFObjectWriter::PPCWinCOFFObjectWriter()
    : MCWinCOFFObjectTargetWriter(llvm::COFF::IMAGE_FILE_MACHINE_PPCBE) { }

std::unique_ptr<MCObjectTargetWriter>
llvm::createPPCWinCOFFObjectWriter() {
  return std::make_unique<PPCWinCOFFObjectWriter>();
}

unsigned PPCWinCOFFObjectWriter::getRelocType(MCContext &Ctx, const MCValue &Target,
                                const MCFixup &Fixup, bool IsCrossSection,
                                const MCAsmBackend &MAB) const
{

  unsigned Modifier = Target.getSpecifier();
  const unsigned FixupKind = Fixup.getKind();
  if (mc::isRelocation(FixupKind))
    return mc::isRelocRelocation(FixupKind)
               ? FixupKind - FirstLiteralRelocationKind
               : FixupKind;

  assert(Modifier != PPC::S_TOC &&
         Modifier != PPC::S_TOC_HA &&
         Modifier != PPC::S_TOC_HI &&
         Modifier != PPC::S_TOC_LO &&
         Modifier != PPC::S_TOCBASE &&
         "WinCOFF doesn't use TOC for any implemented platform");
  switch(FixupKind) {
  // TODO: make sure these are all valid for the target's variant kind.
  case FK_NONE:
  case PPC::fixup_ppc_nofixup:
    return llvm::COFF::IMAGE_REL_PPC_ABSOLUTE;
  case PPC::fixup_ppc_half16ds:
  case PPC::fixup_ppc_half16dq:
    return llvm::COFF::IMAGE_REL_PPC_ADDR14;
  case FK_Data_2:
  case PPC::fixup_ppc_codeview_data_2:
    if (Modifier == PPC::S_SECREL) {
      Ctx.reportError(Fixup.getLoc(),
                      "16-bit section-relative data relocation is not "
                      "supported; use a 32-bit section-relative field");
      return COFF::IMAGE_REL_PPC_ABSOLUTE;
    }
    return llvm::COFF::IMAGE_REL_PPC_ADDR16;
  case FK_Data_4:
  case PPC::fixup_ppc_codeview_data_4:
    switch (Modifier) {
    case MCSymbolRefExpr::VK_COFF_IMGREL32:
      return COFF::IMAGE_REL_PPC_ADDR32NB;
    case PPC::S_SECREL:
      return COFF::IMAGE_REL_PPC_SECREL;
    default:
      return COFF::IMAGE_REL_PPC_ADDR32;
    }
  case FK_Data_8:
  case PPC::fixup_ppc_codeview_data_8:
    return llvm::COFF::IMAGE_REL_PPC_ADDR64;
  case FK_SecRel_2:
    return llvm::COFF::IMAGE_REL_PPC_SECTION;
  case FK_SecRel_4:
    return llvm::COFF::IMAGE_REL_PPC_SECREL;
  case PPC::fixup_ppc_br24:
  case PPC::fixup_ppc_br24_notoc:
    return llvm::COFF::IMAGE_REL_PPC_REL24;
  case PPC::fixup_ppc_br24abs:
    return llvm::COFF::IMAGE_REL_PPC_ADDR24;
  case PPC::fixup_ppc_brcond14:
    return llvm::COFF::IMAGE_REL_PPC_REL14;
  case PPC::fixup_ppc_brcond14abs:
    return llvm::COFF::IMAGE_REL_PPC_ADDR14;
  case PPC::fixup_ppc_half16:
    switch (Modifier) {
    case PPC::S_None:
    case PPC::S_LO:
      return llvm::COFF::IMAGE_REL_PPC_REFLO;
    // COFF REFHI is high-adjusted, not the unadjusted @h.
    case PPC::S_HA:
      return llvm::COFF::IMAGE_REL_PPC_REFHI;
    case PPC::S_SECREL:
      return llvm::COFF::IMAGE_REL_PPC_SECREL16;
    default:
      ;
    }
    [[fallthrough]];
  default:
    dbgs() << "FixupKind=" << FixupKind << "\n";
    llvm_unreachable("Unimplemented PPC fixup.");
  }
}

bool PPCWinCOFFObjectWriter::recordRelocation(const MCFixup &) const {
  return true; // FIXME: Not sure if valid.
}

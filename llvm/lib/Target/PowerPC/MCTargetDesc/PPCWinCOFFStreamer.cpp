//===-------- PPCWinCOFFStreamer.cpp - COFF Object Output ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is a custom MCWinCOFFStreamer for PowerPC.
//
// The purpose of the custom WinCOFF streamer is to allow us to intercept
// instructions as they are being emitted and align all 8 byte instructions
// to a 64 byte boundary if required (by adding a 4 byte nop). This is important
// because 8 byte instructions are not allowed to cross 64 byte boundaries
// and by aligning anything that is within 4 bytes of the boundary we can
// guarantee that the 8 byte instructions do not cross that boundary.
//
//===----------------------------------------------------------------------===//

#include "PPCWinCOFFStreamer.h"
#include "PPCFixupKinds.h"
#include "PPCMCCodeEmitter.h"
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSectionCOFF.h"
#include "llvm/MC/MCSectionXCOFF.h"
#include "llvm/MC/MCSymbolXCOFF.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Endian.h"

using namespace llvm;

PPCWinCOFFStreamer::PPCWinCOFFStreamer(MCContext &Context,
                                   std::unique_ptr<MCAsmBackend> MAB,
                                   std::unique_ptr<MCObjectWriter> OW,
                                   std::unique_ptr<MCCodeEmitter> Emitter)
    : MCWinCOFFStreamer(Context, std::move(MAB), std::move(Emitter),
        std::move(OW)) {}

void PPCWinCOFFStreamer::emitIntValue(uint64_t Value, unsigned Size) {
  const MCSection *Section = getCurrentSectionOnly();
  if (Section && Section->getName().starts_with(".debug$")) {
    uint64_t LittleEndianValue =
        support::endian::byte_swap(Value, endianness::big) >>
        (8 * (sizeof(Value) - Size));
    MCWinCOFFStreamer::emitIntValue(
        LittleEndianValue, Size);
    return;
  }
  MCWinCOFFStreamer::emitIntValue(Value, Size);
}

void PPCWinCOFFStreamer::emitIntValue(const APInt &Value) {
  const MCSection *Section = getCurrentSectionOnly();
  if (Section && Section->getName().starts_with(".debug$")) {
    MCWinCOFFStreamer::emitIntValue(Value.byteSwap());
    return;
  }
  MCWinCOFFStreamer::emitIntValue(Value);
}

void PPCWinCOFFStreamer::emitValueImpl(const MCExpr *Value, unsigned Size,
                                     SMLoc Loc) {
  const MCSection *Section = getCurrentSectionOnly();
  if (!Section || !Section->getName().starts_with(".debug$")) {
    MCWinCOFFStreamer::emitValueImpl(Value, Size, Loc);
    return;
  }

  MCDataFragment *DF = getOrCreateDataFragment();
  size_t NumFixups = DF->getFixups().size();
  MCWinCOFFStreamer::emitValueImpl(Value, Size, Loc);
  if (DF->getFixups().size() == NumFixups)
    return;

  // Absolute symbols and differences between code labels do not identify the
  // CodeView section containing the fixup. Preserve its byte order explicitly.
  assert(DF->getFixups().size() == NumFixups + 1);
  MCFixup &Fixup = DF->getFixups().back();
  MCFixupKind Kind;
  switch (Fixup.getKind()) {
  case FK_Data_2:
    Kind = static_cast<MCFixupKind>(PPC::fixup_ppc_codeview_data_2);
    break;
  case FK_Data_4:
    Kind = static_cast<MCFixupKind>(PPC::fixup_ppc_codeview_data_4);
    break;
  case FK_Data_8:
    Kind = static_cast<MCFixupKind>(PPC::fixup_ppc_codeview_data_8);
    break;
  default:
    return;
  }
  Fixup = MCFixup::create(Fixup.getOffset(), Fixup.getValue(), Kind,
                         Fixup.getLoc());
}

void PPCWinCOFFStreamer::emitPrefixedInstruction(const MCInst &Inst,
                                               const MCSubtargetInfo &STI) {
  // Prefixed instructions must not cross a 64-byte boundary (i.e. prefix is
  // before the boundary and the remaining 4-bytes are after the boundary). In
  // order to achieve this, a nop is added prior to any such boundary-crossing
  // prefixed instruction. Align to 64 bytes if possible but add a maximum of 4
  // bytes when trying to do that. If alignment requires adding more than 4
  // bytes then the instruction won't be aligned.
  emitCodeAlignment(Align(64), &STI, 4);

  // Emit the instruction.
  // Since the previous emit created a new fragment then adding this instruction
  // also forces the addition of a new fragment. Inst is now the first
  // instruction in that new fragment.
  MCWinCOFFStreamer::emitInstruction(Inst, STI);
}

void PPCWinCOFFStreamer::emitInstruction(const MCInst &Inst,
                                       const MCSubtargetInfo &STI) {
  PPCMCCodeEmitter *Emitter =
      static_cast<PPCMCCodeEmitter *>(getAssembler().getEmitterPtr());

  // Special handling is only for prefixed instructions.
  if (!Emitter->isPrefixedInstruction(Inst)) {
    MCWinCOFFStreamer::emitInstruction(Inst, STI);
    return;
  }
  emitPrefixedInstruction(Inst, STI);
}

MCWinCOFFStreamer *
llvm::createPPCWinCOFFStreamer(MCContext &Context,
                             std::unique_ptr<MCAsmBackend> MAB,
                             std::unique_ptr<MCObjectWriter> OW,
                             std::unique_ptr<MCCodeEmitter> Emitter) {
  return new PPCWinCOFFStreamer(Context, std::move(MAB), std::move(OW),
                              std::move(Emitter));
}

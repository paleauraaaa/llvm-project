//===-- PPCXboxPDataExpr.cpp - Xbox runtime-function descriptor -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PPCXboxPDataExpr.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

const PPCXboxPDataExpr *PPCXboxPDataExpr::create(
    MCContext &Ctx, const MCSymbol *Begin, const MCSymbol *PrologueEnd,
    const MCSymbol *End, bool HasHandler) {
  return new (Ctx) PPCXboxPDataExpr(Ctx, Begin, PrologueEnd, End, HasHandler);
}

void PPCXboxPDataExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  OS << (HasHandler ? "(0xc0000000 | ((" : "(0x40000000 | ((");
  End->print(OS, MAI);
  OS << " - ";
  Begin->print(OS, MAI);
  OS << ") / 4 << 8) | ((";
  PrologueEnd->print(OS, MAI);
  OS << " - ";
  Begin->print(OS, MAI);
  OS << ") / 4))";
}

bool PPCXboxPDataExpr::evaluateAsRelocatableImpl(
    MCValue &Res, const MCAssembler *Asm) const {
  if (!Asm || !Asm->hasLayout())
    return false;

  if (!Begin->isInSection() || !PrologueEnd->isInSection() ||
      !End->isInSection() || &Begin->getSection() != &PrologueEnd->getSection() ||
      &Begin->getSection() != &End->getSection()) {
    Ctx.reportError(SMLoc(), "Xbox 360 .pdata labels must be defined in the "
                             "same section");
    Res = MCValue::get(int64_t(0));
    return true;
  }

  uint64_t Start = Asm->getSymbolOffset(*Begin);
  uint64_t EndOffset = Asm->getSymbolOffset(*End);
  uint64_t PrologueOffset = Asm->getSymbolOffset(*PrologueEnd);
  uint64_t Length = EndOffset - Start;
  uint64_t PrologueLength = PrologueOffset - Start;
  if (EndOffset <= Start || PrologueOffset < Start || Length % 4 ||
      PrologueLength % 4 || PrologueLength > Length ||
      Length / 4 > 0x3fffff || PrologueLength / 4 > 255) {
    Ctx.reportError(SMLoc(), "Xbox 360 function or prologue length cannot be "
                             "represented in .pdata");
    Res = MCValue::get(int64_t(0));
    return true;
  }
  Res = MCValue::get((HasHandler ? 0xc0000000 : 0x40000000) |
                    ((Length / 4) << 8) | (PrologueLength / 4));
  return true;
}

void PPCXboxPDataExpr::visitUsedExpr(MCStreamer &Streamer) const {
  for (const MCSymbol *Symbol : {Begin, PrologueEnd, End})
    Streamer.visitUsedExpr(*MCSymbolRefExpr::create(Symbol, Ctx));
}

MCFragment *PPCXboxPDataExpr::findAssociatedFragment() const {
  return Begin->getFragment();
}

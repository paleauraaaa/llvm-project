//===-- PPCXboxPDataExpr.h - Xbox runtime-function descriptor -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_POWERPC_MCTARGETDESC_PPCXBOXPDATAEXPR_H
#define LLVM_LIB_TARGET_POWERPC_MCTARGETDESC_PPCXBOXPDATAEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {

/// Packed Xbox runtime-function descriptor, validated after assembler layout.
class PPCXboxPDataExpr final : public MCTargetExpr {
  MCContext &Ctx;
  const MCSymbol *Begin;
  const MCSymbol *PrologueEnd;
  const MCSymbol *End;
  bool HasHandler;

  PPCXboxPDataExpr(MCContext &Ctx, const MCSymbol *Begin,
                  const MCSymbol *PrologueEnd, const MCSymbol *End,
                  bool HasHandler)
      : Ctx(Ctx), Begin(Begin), PrologueEnd(PrologueEnd), End(End),
        HasHandler(HasHandler) {}

public:
  static const PPCXboxPDataExpr *
  create(MCContext &Ctx, const MCSymbol *Begin, const MCSymbol *PrologueEnd,
         const MCSymbol *End, bool HasHandler = false);
  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res,
                                const MCAssembler *Asm) const override;
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override;
};
} // namespace llvm

#endif

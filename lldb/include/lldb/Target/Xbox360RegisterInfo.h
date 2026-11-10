//===-- Xbox360RegisterInfo.h ------------------------------------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_TARGET_XBOX360REGISTERINFO_H
#define LLDB_TARGET_XBOX360REGISTERINFO_H

#include "lldb/Target/DynamicRegisterInfo.h"

namespace lldb_private {
class ArchSpec;

/// Describe guest registers without assigning a remote packet layout.
std::vector<DynamicRegisterInfo::Register> GetXbox360RegisterInfo();

/// Supply missing register metadata for Xbox targets, preserving the remote's
/// widths, numbering, offsets and explicit debug-register mappings.
/// Returns false when the architecture is not an Xbox PowerPC target.
bool AugmentXbox360RegisterInfo(
    const ArchSpec &arch,
    std::vector<DynamicRegisterInfo::Register> &registers);
} // namespace lldb_private

#endif // LLDB_TARGET_XBOX360REGISTERINFO_H

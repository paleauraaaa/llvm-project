//===-- Xbox360CallFrameInfo.h -----------------------------------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_OBJECTFILE_PECOFF_XBOX360CALLFRAMEINFO_H
#define LLDB_SOURCE_PLUGINS_OBJECTFILE_PECOFF_XBOX360CALLFRAMEINFO_H

#include "lldb/Symbol/CallFrameInfo.h"
#include <deque>
#include <mutex>
#include <optional>
#include <vector>

class ObjectFilePECOFF;

/// Decode Xbox packed .pdata and the prologues it describes.
class Xbox360CallFrameInfo : public lldb_private::CallFrameInfo {
public:
  Xbox360CallFrameInfo(ObjectFilePECOFF &object_file, uint32_t rva,
                       uint32_t size);

  bool GetAddressRange(lldb_private::Address addr,
                       lldb_private::AddressRange &range) override;
  bool GetUnwindPlan(const lldb_private::Address &addr,
                     lldb_private::UnwindPlan &plan) override;
  bool GetUnwindPlan(const lldb_private::AddressRange &range,
                     lldb_private::UnwindPlan &plan) override;

private:
  struct RuntimeFunction {
    uint32_t start;
    uint32_t size;
    uint32_t prologue_size;
  };
  const RuntimeFunction *
  FindFunction(const lldb_private::AddressRange &range) const;
  std::optional<uint32_t> ReadInstruction(uint32_t address);
  const uint8_t *KeepExpression(std::vector<uint8_t> expression);

  ObjectFilePECOFF &m_object_file;
  std::vector<RuntimeFunction> m_functions;
  // UnwindPlan expressions, like section-backed DWARF expressions, borrow
  // their storage from the module's CallFrameInfo.
  std::deque<std::vector<uint8_t>> m_expressions;
  std::mutex m_mutex;
};

#endif

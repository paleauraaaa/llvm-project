//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef __XBOXUNWIND_HPP__
#define __XBOXUNWIND_HPP__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace libunwind {

namespace XboxPdata {
constexpr size_t WordSize = 4;
constexpr size_t RecordSize = 2 * WordSize;
constexpr size_t HandlerPrefixSize = 2 * WordSize;
constexpr uint32_t InstructionSize = 4;
constexpr unsigned FunctionLengthShift = 8;
constexpr uint32_t FunctionLengthMask = 0x3fffff;
constexpr uint32_t PrologueLengthMask = 0xff;
constexpr uint32_t PowerPCInstructions = 0x40000000;
constexpr uint32_t HasHandler = 0x80000000;
} // namespace XboxPdata

namespace XboxSEH {
constexpr size_t WordSize = 4;
constexpr size_t ScopeCountSize = WordSize;
constexpr size_t ScopeRecordSize = 4 * WordSize;
constexpr size_t MetadataRecordSize = 5 * WordSize;
constexpr uint32_t MetadataVersion = 0x58534501;
constexpr uint32_t CleanupFunclet = 1;
constexpr uint32_t CatchFunclet = 2;
constexpr uint32_t CHandler = 4;
constexpr uint32_t KnownFlags = CleanupFunclet | CatchFunclet | CHandler;
} // namespace XboxSEH

namespace XboxPE {
constexpr size_t DosHeaderSize = 64;
constexpr size_t DosNTHeaderOffset = 0x3c;
constexpr size_t NTHeaderPrefixSize = 24;
constexpr size_t NTSignatureSize = 4;
constexpr size_t MachineOffset = 4;
constexpr size_t SectionCountOffset = 6;
constexpr size_t OptionalHeaderSizeOffset = 20;
constexpr size_t FileCharacteristicsOffset = 22;
constexpr uint16_t PowerPCBigEndianMachine = 0x1f2;
constexpr uint16_t File32BitMachine = 0x100;
constexpr size_t MinOptionalHeaderSize = 64;
constexpr uint16_t PE32Magic = 0x10b;
constexpr size_t ImageSizeOffset = 56;
constexpr size_t HeadersSizeOffset = 60;
constexpr size_t SectionHeaderSize = 40;
constexpr size_t SectionNameSize = 8;
constexpr size_t SectionVirtualSizeOffset = 8;
constexpr size_t SectionVirtualAddressOffset = 12;
constexpr size_t SectionCharacteristicsOffset = 36;
constexpr uint32_t SectionExecutable = 0x20000000;
} // namespace XboxPE

struct XboxUnwindSection {
  uint32_t offset;
  uint32_t length;
};

inline uint16_t xboxReadLE16(const uint8_t *p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

inline uint32_t xboxReadLE32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
         (uint32_t(p[3]) << 24);
}

inline uint32_t xboxReadBE32(const uint8_t *p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
         (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

enum class XboxPdataResult { Found, NotFound, Invalid, Unsupported };

struct XboxRuntimeFunction {
  uint32_t start;
  uint32_t end;
  uint32_t prologueSize;
  uint32_t packed;
  uint32_t handler;
  uint32_t handlerData;
  size_t recordOffset;
};

// Unlike the PE headers, Xbox runtime-function records contain big-endian
// absolute addresses and big-endian packed lengths.
inline XboxPdataResult findXboxRuntimeFunction(const uint8_t *image,
                                               size_t imageSize,
                                               uint32_t imageBase,
                                               size_t pdataOffset,
                                               size_t pdataSize, uint32_t pc,
                                               XboxRuntimeFunction &result) {
  if (!image || imageSize > UINT32_MAX - imageBase || pdataOffset > imageSize ||
      pdataSize > imageSize - pdataOffset || pdataSize % XboxPdata::RecordSize)
    return XboxPdataResult::Invalid;

  uint64_t previousEnd = imageBase;
  bool found = false;
  XboxRuntimeFunction candidate = {};
  for (size_t offset = 0; offset < pdataSize; offset += XboxPdata::RecordSize) {
    const uint8_t *record = image + pdataOffset + offset;
    uint32_t start = xboxReadBE32(record);
    uint32_t packed = xboxReadBE32(record + XboxPdata::WordSize);
    if (!start && !packed)
      continue;
    uint32_t length = ((packed >> XboxPdata::FunctionLengthShift) &
                       XboxPdata::FunctionLengthMask) *
                      XboxPdata::InstructionSize;
    uint32_t prologue =
        (packed & XboxPdata::PrologueLengthMask) * XboxPdata::InstructionSize;
    uint64_t end = uint64_t(start) + length;
    if (!length || start % XboxPdata::InstructionSize || prologue > length ||
        start < previousEnd || start < imageBase ||
        end > uint64_t(imageBase) + imageSize)
      return XboxPdataResult::Invalid;
    previousEnd = end;
    if (pc >= start && uint64_t(pc) < end) {
      candidate = {start, uint32_t(end),       prologue, packed, 0,
                   0,     pdataOffset + offset};
      found = true;
    }
  }
  if (!found)
    return XboxPdataResult::NotFound;
  if (!(candidate.packed & XboxPdata::PowerPCInstructions))
    return XboxPdataResult::Unsupported;
  if (candidate.packed & XboxPdata::HasHandler) {
    size_t startOffset = candidate.start - imageBase;
    if (startOffset < XboxPdata::HandlerPrefixSize)
      return XboxPdataResult::Invalid;
    candidate.handler =
        xboxReadBE32(image + startOffset - XboxPdata::HandlerPrefixSize);
    candidate.handlerData =
        xboxReadBE32(image + startOffset - XboxPdata::WordSize);
    if (!candidate.handler || candidate.handler % XboxPdata::InstructionSize ||
        candidate.handlerData < imageBase ||
        candidate.handlerData % XboxPdata::WordSize ||
        candidate.handlerData - imageBase > imageSize ||
        imageSize - (candidate.handlerData - imageBase) < XboxPdata::WordSize)
      return XboxPdataResult::Invalid;
  }
  result = candidate;
  return XboxPdataResult::Found;
}

struct XboxSEHScope {
  uint32_t begin;
  uint32_t end;
  uint32_t filterOrFinally;
  uint32_t handler;
};

struct XboxSEHMetadata {
  uint32_t start;
  uint32_t parent;
  uint32_t parentFrameSize;
  uint32_t flags;
};

inline XboxPdataResult findXboxSEHMetadata(const uint8_t *image,
                                          size_t imageSize, uint32_t imageBase,
                                          size_t offset, size_t size,
                                          const XboxRuntimeFunction &function,
                                          XboxSEHMetadata &result) {
  if (!image || imageSize > UINT32_MAX - imageBase || offset > imageSize ||
      size > imageSize - offset || size % XboxSEH::MetadataRecordSize)
    return XboxPdataResult::Invalid;
  bool found = false;
  XboxSEHMetadata candidate = {};
  for (size_t pos = 0; pos < size; pos += XboxSEH::MetadataRecordSize) {
    const uint8_t *record = image + offset + pos;
    if (xboxReadBE32(record) != XboxSEH::MetadataVersion)
      return XboxPdataResult::Unsupported;
    XboxSEHMetadata entry = {
        xboxReadBE32(record + 4), xboxReadBE32(record + 8),
        xboxReadBE32(record + 12), xboxReadBE32(record + 16)};
    if ((entry.flags & ~XboxSEH::KnownFlags) ||
        (entry.flags & (XboxSEH::CleanupFunclet | XboxSEH::CatchFunclet)) ==
            (XboxSEH::CleanupFunclet | XboxSEH::CatchFunclet) ||
        entry.start < imageBase || entry.start - imageBase >= imageSize ||
        entry.parent < imageBase || entry.parent - imageBase >= imageSize ||
        (entry.start & 3) || (entry.parent & 3) ||
        (entry.parentFrameSize & 15) ||
        (!(entry.flags & (XboxSEH::CleanupFunclet | XboxSEH::CatchFunclet)) &&
         entry.start != entry.parent))
      return XboxPdataResult::Invalid;
    if (entry.start == function.start) {
      if (found || ((entry.flags & XboxSEH::CHandler) && !function.handler))
        return XboxPdataResult::Invalid;
      candidate = entry;
      found = true;
    }
  }
  if (!found)
    return XboxPdataResult::NotFound;
  result = candidate;
  return XboxPdataResult::Found;
}

// Call only for __C_specific_handler data, not for C++ personality data.
inline bool readXboxSEHScope(const uint8_t *table, size_t size, uint32_t index,
                             XboxSEHScope &result) {
  if (!table || size < XboxSEH::ScopeCountSize)
    return false;
  uint32_t count = xboxReadBE32(table);
  if (count > (size - XboxSEH::ScopeCountSize) / XboxSEH::ScopeRecordSize ||
      index >= count)
    return false;
  const uint8_t *entry = table + XboxSEH::ScopeCountSize +
                         size_t(index) * XboxSEH::ScopeRecordSize;
  XboxSEHScope scope = {xboxReadBE32(entry),
                        xboxReadBE32(entry + XboxSEH::WordSize),
                        xboxReadBE32(entry + 2 * XboxSEH::WordSize),
                        xboxReadBE32(entry + 3 * XboxSEH::WordSize)};
  if (scope.begin >= scope.end || !scope.filterOrFinally)
    return false;
  result = scope;
  return true;
}

// PE headers are little-endian even though the Xbox 360 CPU is big-endian.
inline bool findXboxPdata(const uint8_t *image, size_t mappedSize,
                          uintptr_t pcOffset, XboxUnwindSection &result,
                          XboxUnwindSection *codeResult = nullptr,
                          XboxUnwindSection *sehResult = nullptr) {
  if (!image || mappedSize < XboxPE::DosHeaderSize || image[0] != 'M' ||
      image[1] != 'Z')
    return false;
  uint32_t nt = xboxReadLE32(image + XboxPE::DosNTHeaderOffset);
  if (nt > mappedSize || mappedSize - nt < XboxPE::NTHeaderPrefixSize ||
      memcmp(image + nt, "PE\0\0", XboxPE::NTSignatureSize) != 0 ||
      xboxReadLE16(image + nt + XboxPE::MachineOffset) !=
          XboxPE::PowerPCBigEndianMachine ||
      !(xboxReadLE16(image + nt + XboxPE::FileCharacteristicsOffset) &
        XboxPE::File32BitMachine))
    return false;

  uint16_t count = xboxReadLE16(image + nt + XboxPE::SectionCountOffset);
  uint16_t optionalSize =
      xboxReadLE16(image + nt + XboxPE::OptionalHeaderSizeOffset);
  size_t optional = size_t(nt) + XboxPE::NTHeaderPrefixSize;
  if (optionalSize < XboxPE::MinOptionalHeaderSize ||
      optionalSize > mappedSize - optional ||
      xboxReadLE16(image + optional) != XboxPE::PE32Magic)
    return false;
  uint32_t imageSize = xboxReadLE32(image + optional + XboxPE::ImageSizeOffset);
  uint32_t headersSize =
      xboxReadLE32(image + optional + XboxPE::HeadersSizeOffset);
  size_t sections = optional + optionalSize;
  if (!imageSize || imageSize > mappedSize || headersSize > imageSize ||
      sections > headersSize ||
      count > (headersSize - sections) / XboxPE::SectionHeaderSize ||
      pcOffset >= imageSize)
    return false;

  XboxUnwindSection pdata = {0, 0};
  XboxUnwindSection code = {0, 0};
  XboxUnwindSection seh = {0, 0};
  bool foundCode = false;
  for (uint16_t i = 0; i != count; ++i) {
    const uint8_t *section =
        image + sections + size_t(i) * XboxPE::SectionHeaderSize;
    uint32_t size = xboxReadLE32(section + XboxPE::SectionVirtualSizeOffset);
    uint32_t offset =
        xboxReadLE32(section + XboxPE::SectionVirtualAddressOffset);
    if (offset > imageSize || size > imageSize - offset ||
        (size && offset < headersSize))
      return false;
    if ((xboxReadLE32(section + XboxPE::SectionCharacteristicsOffset) &
         XboxPE::SectionExecutable) &&
        pcOffset >= offset && pcOffset - offset < size) {
      foundCode = true;
      code = {offset, size};
    }
    if (memcmp(section, ".pdata\0\0", XboxPE::SectionNameSize) == 0) {
      if (pdata.length || !size)
        return false;
      pdata = {offset, size};
    }
    if (sehResult &&
        memcmp(section, ".xseh\0\0\0", XboxPE::SectionNameSize) == 0) {
      if (seh.length || !size)
        return false;
      seh = {offset, size};
    }
  }
  if (!foundCode || !pdata.length)
    return false;
  result = pdata;
  if (codeResult)
    *codeResult = code;
  if (sehResult)
    *sehResult = seh;
  return true;
}

} // namespace libunwind

#endif // __XBOXUNWIND_HPP__

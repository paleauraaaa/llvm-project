//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: target={{.*xbox360.*}}

#undef NDEBUG
#include "../src/XboxUnwind.hpp"
#include "../src/XboxUnwindState.h"
#include <assert.h>
#include <string.h>

using namespace libunwind;

#if defined(_XBOX) && _XBOX_VER == 200
extern "C" int DbgPrint(const char *, ...);
#endif

static constexpr uint32_t Base = 0x82000000;
static uint8_t image[4096];

static void put32(size_t offset, uint32_t value) {
  image[offset] = uint8_t(value >> 24);
  image[offset + 1] = uint8_t(value >> 16);
  image[offset + 2] = uint8_t(value >> 8);
  image[offset + 3] = uint8_t(value);
}

static void makeRecords() {
  memset(image, 0, sizeof(image));
  put32(0x800, Base + 0x100);
  put32(0x804, 0x40000201);
  put32(0x808, Base + 0x200);
  put32(0x80c, 0xc0000402);
  put32(0x1f8, 0x83000000);
  put32(0x1fc, Base + 0x600);
}

static XboxPdataResult find(uint32_t pc, XboxRuntimeFunction &function,
                            size_t size = 16) {
  return findXboxRuntimeFunction(image, sizeof(image), Base, 0x800, size, pc,
                                 function);
}

int main() {
  _Unwind_Exception exception = {};
  assert(!__libunwind_xbox_is_forced(&exception));
  assert(__libunwind_xbox_set_seh_target(&exception, 0x1000, 0));
  assert(__libunwind_xbox_has_seh_target(&exception));
  assert(!__libunwind_xbox_is_forced(&exception));
  assert(exception.private_2 == 0x1000);
  assert(__libunwind_xbox_seh_target_scope(&exception) == 0);
  exception.private_1 = 0;
  assert(__libunwind_xbox_set_seh_target(&exception, 0x2000, UINTPTR_MAX >> 1));
  assert(__libunwind_xbox_seh_target_scope(&exception) == (UINTPTR_MAX >> 1));
  exception.private_1 = 0;
  assert(!__libunwind_xbox_set_seh_target(&exception, 0x2000,
                                          (UINTPTR_MAX >> 1) + 1));
  assert(!__libunwind_xbox_set_seh_target(&exception, 0x2001, 0));
  assert(!__libunwind_xbox_set_seh_target(&exception, 0, 0));
  assert(exception.private_2 == 0x2000);
  exception.private_1 = 0x1000;
  exception.private_2 = 0x1234;
  assert(__libunwind_xbox_is_forced(&exception));
  assert(!__libunwind_xbox_has_seh_target(&exception));
  assert(!__libunwind_xbox_set_seh_target(&exception, 0x1000, 0));
  assert(exception.private_2 == 0x1234);

  XboxRuntimeFunction f = {};
  makeRecords();
  assert(find(Base + 0x100, f) == XboxPdataResult::Found);
  assert(f.start == Base + 0x100 && f.end == Base + 0x108);
  assert(f.prologueSize == 4 && f.packed == 0x40000201);
  assert(f.handler == 0 && f.handlerData == 0 && f.recordOffset == 0x800);
  assert(find(Base + 0x107, f) == XboxPdataResult::Found);
  assert(find(Base + 0x108, f) == XboxPdataResult::NotFound);
  assert(find(Base + 0x200, f) == XboxPdataResult::Found);
  assert(f.start == Base + 0x200 && f.end == Base + 0x210);
  assert(f.prologueSize == 8 && f.handler == 0x83000000);
  assert(f.handlerData == Base + 0x600 && f.recordOffset == 0x808);
  assert(find(Base + 0x20f, f) == XboxPdataResult::Found);
  assert(find(Base + 0x210, f) == XboxPdataResult::NotFound);
  assert(find(Base + 0x100, f, 15) == XboxPdataResult::Invalid);

  makeRecords();
  put32(0x804, 0x40000000);
  assert(find(Base + 0x100, f) == XboxPdataResult::Invalid);
  makeRecords();
  put32(0x804, 0x40000203);
  assert(find(Base + 0x100, f) == XboxPdataResult::Invalid);
  makeRecords();
  put32(0x808, Base + 0x104);
  assert(find(Base + 0x100, f) == XboxPdataResult::Invalid);
  makeRecords();
  put32(0x808, Base + 0xffc);
  assert(find(Base + 0x100, f) == XboxPdataResult::Invalid);
  makeRecords();
  put32(0x804, 0x00000201);
  assert(find(Base + 0x100, f) == XboxPdataResult::Unsupported);
  assert(find(Base + 0x200, f) == XboxPdataResult::Found);
  makeRecords();
  put32(0x1f8, 0);
  assert(find(Base + 0x200, f) == XboxPdataResult::Invalid);
  makeRecords();
  put32(0x1fc, Base + sizeof(image));
  assert(find(Base + 0x200, f) == XboxPdataResult::Invalid);
  assert(findXboxRuntimeFunction(image, sizeof(image), Base, 4090, 8,
                                 Base + 0x100, f) == XboxPdataResult::Invalid);

  makeRecords();
  assert(find(Base + 0x200, f) == XboxPdataResult::Found);
  put32(0x700, XboxSEH::MetadataVersion);
  put32(0x704, Base + 0x200);
  put32(0x708, Base + 0x100);
  put32(0x70c, 96);
  put32(0x710, XboxSEH::CleanupFunclet | XboxSEH::CHandler);
  XboxSEHMetadata metadata = {};
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 20, f,
                             metadata) == XboxPdataResult::Found);
  assert(metadata.parent == Base + 0x100 && metadata.parentFrameSize == 96 &&
         metadata.flags == (XboxSEH::CleanupFunclet | XboxSEH::CHandler));
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 19, f,
                             metadata) == XboxPdataResult::Invalid);
  put32(0x700, XboxSEH::MetadataVersion + 1);
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 20, f,
                             metadata) == XboxPdataResult::Unsupported);
  put32(0x700, XboxSEH::MetadataVersion);
  put32(0x710, XboxSEH::CleanupFunclet | XboxSEH::CatchFunclet);
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 20, f,
                             metadata) == XboxPdataResult::Invalid);
  put32(0x710, 0);
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 20, f,
                             metadata) == XboxPdataResult::Invalid);
  put32(0x710, XboxSEH::CleanupFunclet | XboxSEH::CHandler);
  memcpy(image + 0x714, image + 0x700, XboxSEH::MetadataRecordSize);
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 40, f,
                             metadata) == XboxPdataResult::Invalid);
  put32(0x704, Base + 0x100);
  put32(0x708, Base + 0x100);
  put32(0x710, 0);
  assert(findXboxSEHMetadata(image, sizeof(image), Base, 0x700, 20, f,
                             metadata) == XboxPdataResult::NotFound);

  makeRecords();
  put32(0x600, 2);
  put32(0x604, Base + 0x100);
  put32(0x608, Base + 0x108);
  put32(0x60c, 0x83000100);
  put32(0x610, 0);
  put32(0x614, Base + 0x200);
  put32(0x618, Base + 0x210);
  put32(0x61c, 1);
  put32(0x620, Base + 0x300);
  XboxSEHScope scope = {};
  assert(readXboxSEHScope(image + 0x600, 36, 0, scope));
  assert(scope.begin == Base + 0x100 && scope.end == Base + 0x108);
  assert(scope.filterOrFinally == 0x83000100 && scope.handler == 0);
  assert(readXboxSEHScope(image + 0x600, 36, 1, scope));
  assert(scope.filterOrFinally == 1 && scope.handler == Base + 0x300);
  assert(!readXboxSEHScope(image + 0x600, 35, 0, scope));
  assert(!readXboxSEHScope(image + 0x600, 36, 2, scope));
  put32(0x608, Base + 0x100);
  assert(!readXboxSEHScope(image + 0x600, 36, 0, scope));
  put32(0x608, Base + 0x108);
  put32(0x60c, 0);
  assert(!readXboxSEHScope(image + 0x600, 36, 0, scope));
#if defined(_XBOX) && _XBOX_VER == 200
  DbgPrint("xbox360 seh parsing: PASS\n");
#endif
  return 0;
}

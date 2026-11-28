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
#include <assert.h>
#include <string.h>

#if defined(_XBOX) && _XBOX_VER == 200
#include "../src/AddressSpace.hpp"
#include "../src/RWMutex.hpp"
#endif

using namespace libunwind;

static void put16(uint8_t *p, uint16_t value) {
  p[0] = uint8_t(value);
  p[1] = uint8_t(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
  put16(p, uint16_t(value));
  put16(p + 2, uint16_t(value >> 16));
}

static constexpr size_t ImageSize = 4096;
static constexpr size_t Nt = 0x80;
static constexpr size_t Optional = Nt + 24;
static constexpr size_t Sections = Optional + 224;
static uint8_t image[ImageSize];

static void makeImage(uint8_t *base = image) {
  memset(base, 0, ImageSize);
  memcpy(base, "MZ", 2);
  put32(base + 0x3c, Nt);
  memcpy(base + Nt, "PE\0\0", 4);
  put16(base + Nt + 4, 0x1f2);
  put16(base + Nt + 6, 2);
  put16(base + Nt + 20, 224);
  put16(base + Nt + 22, 0x100);
  put16(base + Optional, 0x10b);
  put32(base + Optional + 56, ImageSize);
  put32(base + Optional + 60, 0x400);
  memcpy(base + Sections, ".text", 5);
  put32(base + Sections + 8, 0x100);
  put32(base + Sections + 12, 0x400);
  put32(base + Sections + 36, 0x60000020);
  memcpy(base + Sections + 40, ".pdata", 6);
  put32(base + Sections + 48, 0x80);
  put32(base + Sections + 52, 0x800);
}

static bool find(size_t mappedSize, uintptr_t pc) {
  XboxUnwindSection section = {0, 0};
  bool found = findXboxPdata(image, mappedSize, pc, section);
  if (found) {
    assert(section.offset == 0x800);
    assert(section.length == 0x80);
  }
  return found;
}

#if defined(_XBOX) && _XBOX_VER == 200
static uint8_t image2[ImageSize];
static XboxLoaderEntry modules[2];

extern "C" void *mockPcToFileHeader(void *, XboxLoaderEntry **)
    __asm__("XexPcToFileHeader");
extern "C" void *mockPcToFileHeader(void *pc, XboxLoaderEntry **out) {
  uintptr_t address = reinterpret_cast<uintptr_t>(pc);
  for (XboxLoaderEntry &module : modules) {
    if (address >= module.imageBase &&
        address - module.imageBase < module.imageSize) {
      *out = &module;
      return nullptr;
    }
  }
  *out = nullptr;
  return nullptr;
}

extern "C" {
int DbgPrint(const char *, ...);
extern const uint8_t __ImageBase;
int __unw_getcontext(unw_context_t *);
int __unw_init_local(unw_cursor_t *, unw_context_t *);
int __unw_get_proc_info(unw_cursor_t *, unw_proc_info_t *);
int __unw_step(unw_cursor_t *);
}

#if defined(LIBUNWIND_TEST_XBOX_FAIL_FAST_STACK_PROBE)
// This test must not execute DWARF parsing, which needs a stack-probe helper.
extern "C" [[noreturn]] void _RtlCheckStack12() { abort(); }
#endif

struct TestRegisters {
  typedef uint64_t link_hardened_reg_arg_t;
};

static void testLoaderIntegration() {
  makeImage();
  makeImage(image2);
  modules[0].imageBase = reinterpret_cast<uintptr_t>(image);
  modules[1].imageBase = reinterpret_cast<uintptr_t>(image2);
  modules[0].imageSize = modules[1].imageSize = ImageSize;
  LocalAddressSpace space;
  for (XboxLoaderEntry &module : modules) {
    UnwindInfoSections sections;
    assert(space.findUnwindSections<TestRegisters>(module.imageBase + 0x400,
                                                  sections));
    assert(sections.dso_base == module.imageBase);
    assert(sections.xbox_pdata_section == module.imageBase + 0x800);
    assert(sections.xbox_pdata_section_length == 0x80);
    assert(sections.dwarf_section == 0);
    assert(!space.findUnwindSections<TestRegisters>(module.imageBase + 0x500,
                                                   sections));
  }
  UnwindInfoSections sections;
  assert(!space.findUnwindSections<TestRegisters>(0, sections));
  assert(!space.findUnwindSections<TestRegisters>(UINT64_C(0x100000000),
                                                 sections));
  image2[Sections + 40] = 'x';
  assert(!space.findUnwindSections<TestRegisters>(modules[1].imageBase + 0x400,
                                                 sections));

  const uint8_t *base = &__ImageBase;
  uint32_t nt = xboxReadLE32(base + 0x3c);
  modules[0].imageBase = reinterpret_cast<uintptr_t>(base);
  modules[0].imageSize = xboxReadLE32(base + nt + 24 + 56);
  uintptr_t pc = reinterpret_cast<uintptr_t>(&testLoaderIntegration);
  assert(space.findUnwindSections<TestRegisters>(pc, sections));
  assert(sections.dso_base == modules[0].imageBase);
  assert(sections.xbox_pdata_section_length != 0);

  RWMutex lock;
  assert(lock.lock_shared());
  assert(lock.unlock_shared());
  assert(lock.lock());
  assert(lock.unlock());

  unw_context_t context;
  unw_cursor_t cursor;
  unw_proc_info_t info;
  assert(__unw_getcontext(&context) == UNW_ESUCCESS);
  assert(__unw_init_local(&cursor, &context) == UNW_ESUCCESS);
  assert(__unw_get_proc_info(&cursor, &info) == UNW_ENOINFO);
  assert(__unw_step(&cursor) == UNW_STEP_END);
}
#endif

int main() {
  makeImage();
  assert(find(ImageSize, 0x400));
  assert(find(ImageSize, 0x4ff));
  assert(!find(ImageSize, 0x3ff));
  assert(!find(ImageSize, 0x500));
  assert(!find(ImageSize, ImageSize));
  assert(!find(63, 0x400));

  makeImage();
  image[0] = 0;
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + 0x3c, 0xfffffff0);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put16(image + Nt + 4, 0x14c);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put16(image + Nt + 20, 63);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Optional + 56, ImageSize + 1);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Optional + 60, Sections + 79);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put16(image + Nt + 6, 0xffff);
  assert(!find(ImageSize, 0x400));
  makeImage();
  image[Sections + 40] = 'x';
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Sections + 48, 0);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Sections + 52, 0xfffffff0);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Sections + 48, ImageSize);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Sections + 52, 0x100);
  assert(!find(ImageSize, 0x400));
  makeImage();
  put32(image + Sections + 36, 0x40000040);
  assert(!find(ImageSize, 0x400));
#if defined(_XBOX) && _XBOX_VER == 200
  testLoaderIntegration();
  DbgPrint("xbox360 unwind discovery: PASS\n");
#endif
  return 0;
}

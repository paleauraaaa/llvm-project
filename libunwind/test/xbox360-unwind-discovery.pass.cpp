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
#include "../src/Registers.hpp"
#include "../src/RWMutex.hpp"
#include "../src/libunwind_ext.h"
#include <unwind.h>
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
int __unw_get_reg(unw_cursor_t *, unw_regnum_t, unw_word_t *);
int __unw_set_reg(unw_cursor_t *, unw_regnum_t, unw_word_t);
int main();
}

#if defined(LIBUNWIND_TEST_XBOX_FAIL_FAST_STACK_PROBE)
// This test must not execute DWARF parsing, which needs a stack-probe helper.
extern "C" [[noreturn]] void _RtlCheckStack12() { abort(); }
#endif

static _Unwind_Reason_Code countFrames(_Unwind_Context *, void *arg) {
  unsigned &count = *static_cast<unsigned *>(arg);
  return ++count == 2 ? _URC_END_OF_STACK : _URC_NO_REASON;
}

static __attribute__((noinline)) void checkRestoredNonvolatile() {
  __asm__ volatile("li 14, 17" ::: "r14");
  unw_context_t context;
  unw_cursor_t cursor;
  unw_word_t value;
  assert(__unw_getcontext(&context) == UNW_ESUCCESS);
  assert(__unw_init_local(&cursor, &context) == UNW_ESUCCESS);
  assert(__unw_step(&cursor) == 1);
  assert(__unw_get_reg(&cursor, UNW_PPC64_R14, &value) == UNW_ESUCCESS);
  assert(value == UINT64_C(0x123456789abcdef0));
}

static __attribute__((noinline)) void testRestoredNonvolatile() {
  register uint64_t value __asm__("r14") = UINT64_C(0x123456789abcdef0);
  __asm__ volatile("" : "+r"(value));
  checkRestoredNonvolatile();
  __asm__ volatile("" : "+r"(value));
  assert(value == UINT64_C(0x123456789abcdef0));
}

#if defined(LIBUNWIND_TEST_XBOX_SEH_FIXTURE)
extern "C" [[noreturn]] int __C_specific_handler(...) { abort(); }
extern "C" int sehFixture(volatile int *);
static _Unwind_Reason_Code stopAtNativeFrame(
    int, _Unwind_Action, _Unwind_Exception_Class, _Unwind_Exception *,
    _Unwind_Context *context, void *arg) {
  if (_Unwind_GetRegionStart(context) ==
      reinterpret_cast<uintptr_t>(&sehFixture))
    *static_cast<bool *>(arg) = true;
  return _URC_NO_REASON;
}
extern "C" __attribute__((noinline)) int sehFixture(volatile int *value) {
  __try {
    ++*value;
    _Unwind_Exception exception = {};
    assert(_Unwind_RaiseException(&exception) == _URC_FATAL_PHASE1_ERROR);
    bool seenNative = false;
    assert(_Unwind_ForcedUnwind(&exception, stopAtNativeFrame, &seenNative) ==
           _URC_FATAL_PHASE2_ERROR);
    assert(seenNative);
  } __except (1) {
    return -1;
  }
  return 0;
}
#endif

struct TestRegisters {
  typedef uint64_t link_hardened_reg_arg_t;
};

static int expectedCleanupKind;
static uintptr_t expectedParents[2];
static __unw_xbox_seh_cleanup_state expectedStates[2];

extern "C" __attribute__((noinline)) int testCleanupParent(uintptr_t depth) {
  unw_context_t context;
  unw_cursor_t cursor;
  assert(__unw_getcontext(&context) == UNW_ESUCCESS);
  assert(__unw_init_local(&cursor, &context) == UNW_ESUCCESS);
  uintptr_t parent = 0;
  int result = __unw_xbox_get_seh_cleanup_parent(&cursor, &parent);
  __unw_xbox_seh_cleanup_state saved = {};
  if (!expectedCleanupKind) {
    assert(result == UNW_EBADFRAME && parent == 0);
    assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, &saved) ==
           UNW_EBADFRAME);
    return 0;
  }
  assert(result == UNW_ESUCCESS);
  assert(parent == expectedParents[depth]);
  if (expectedCleanupKind == 2) {
    assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, &saved) ==
           UNW_EBADFRAME);
    return 0;
  }
  assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, &saved) ==
         UNW_ESUCCESS);
  assert(saved.exception == expectedStates[depth].exception &&
         saved.cursor == expectedStates[depth].cursor &&
         saved.scope_index == expectedStates[depth].scope_index);
  __unw_xbox_seh_cleanup_state original = expectedStates[depth];
  expectedStates[depth] = {nullptr, nullptr, original.scope_index + 1};
  assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, &saved) ==
         UNW_ESUCCESS);
  assert(saved.exception == original.exception &&
         saved.cursor == original.cursor &&
         saved.scope_index == original.scope_index);
  expectedStates[depth] = original;
  if (depth) {
    Registers_ppc64 registers(&context);
    assert(__libunwind_xbox_seh_call(
               &registers, reinterpret_cast<uintptr_t>(&testCleanupParent),
               0, expectedParents[0], XBOX_SEH_CALL_CLEANUP,
               &expectedStates[0]) == 0);
    parent = 0;
    assert(__unw_xbox_get_seh_cleanup_parent(&cursor, &parent) ==
           UNW_ESUCCESS);
    assert(parent == expectedParents[1]);
    assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, &saved) ==
           UNW_ESUCCESS);
    assert(saved.exception == expectedStates[1].exception &&
           saved.cursor == expectedStates[1].cursor &&
           saved.scope_index == expectedStates[1].scope_index);
  }
  return 0;
}

static __attribute__((noinline)) void testLoaderIntegration() {
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
    assert(space.isXboxCodeRange(module.imageBase + 0x4fc, 4,
                                 module.imageBase, module.imageSize));
    assert(!space.isXboxCodeRange(module.imageBase + 0x4fc, 8,
                                  module.imageBase, module.imageSize));
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
  assert(__unw_get_proc_info(&cursor, &info) == UNW_ESUCCESS);
  assert(info.start_ip <= pc && pc < info.end_ip);
  assert(info.unwind_info_size == 8 && (info.format & 0x40000000));
  uintptr_t parent = 0;
  __unw_xbox_seh_cleanup_state saved = {};
  assert(__unw_xbox_get_seh_cleanup_parent(nullptr, &parent) == UNW_EINVAL);
  assert(__unw_xbox_get_seh_cleanup_parent(&cursor, nullptr) == UNW_EINVAL);
  assert(__unw_xbox_get_seh_cleanup_parent(&cursor, &parent) != UNW_ESUCCESS);
  assert(__unw_xbox_get_seh_cleanup_state(nullptr, &parent, &saved) ==
         UNW_EINVAL);
  assert(__unw_xbox_get_seh_cleanup_state(&cursor, nullptr, &saved) ==
         UNW_EINVAL);
  assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, nullptr) ==
         UNW_EINVAL);
  assert(__unw_xbox_get_seh_cleanup_state(&cursor, &parent, &saved) !=
         UNW_ESUCCESS);
  assert(parent == 0);
  Registers_ppc64 registers(&context);
  expectedParents[0] = uintptr_t(registers.getSP());
  expectedParents[1] = space.get32(expectedParents[0]);
  assert(expectedParents[0] && expectedParents[1] != expectedParents[0]);
  _Unwind_Exception exceptions[2] = {};
  unw_cursor_t otherCursor;
  assert(__unw_init_local(&otherCursor, &context) == UNW_ESUCCESS);
  expectedStates[0] = {&exceptions[0], &otherCursor, 3};
  expectedStates[1] = {&exceptions[1], &cursor, 7};
  expectedCleanupKind = 1;
  assert(__libunwind_xbox_seh_call(
             &registers, reinterpret_cast<uintptr_t>(&testCleanupParent),
             1, expectedParents[1], XBOX_SEH_CALL_CLEANUP,
             &expectedStates[1]) == 0);
  expectedCleanupKind = 2;
  assert(__libunwind_xbox_seh_call(
             &registers, reinterpret_cast<uintptr_t>(&testCleanupParent),
             1, expectedParents[1], XBOX_SEH_CALL_CLEANUP, nullptr) == 0);
  expectedCleanupKind = 0;
  assert(__libunwind_xbox_seh_call(
             &registers, reinterpret_cast<uintptr_t>(&testCleanupParent),
             0, expectedParents[1], XBOX_SEH_CALL_FILTER,
             &expectedStates[1]) == 0);
  assert(__unw_step(&cursor) == 1);
  assert(__unw_get_proc_info(&cursor, &info) == UNW_ESUCCESS);
  assert(info.start_ip == reinterpret_cast<uintptr_t>(&main));
  unsigned frames = 0;
  assert(_Unwind_Backtrace(countFrames, &frames) == _URC_END_OF_STACK);
  assert(frames == 2);
  testRestoredNonvolatile();

#if defined(LIBUNWIND_TEST_XBOX_SEH_FIXTURE)
  uintptr_t sehPC = reinterpret_cast<uintptr_t>(&sehFixture);
  assert(__unw_set_reg(&cursor, UNW_REG_IP, sehPC) == UNW_ESUCCESS);
  assert(__unw_get_proc_info(&cursor, &info) == UNW_ESUCCESS);
  assert(info.start_ip == sehPC &&
         info.handler == reinterpret_cast<uintptr_t>(&__C_specific_handler));
  assert(info.lsda >= modules[0].imageBase &&
         info.lsda - modules[0].imageBase < modules[0].imageSize);
  XboxSEHScope scope;
  assert(readXboxSEHScope(
      reinterpret_cast<const uint8_t *>(info.lsda),
      modules[0].imageSize - (info.lsda - modules[0].imageBase), 0, scope));
  assert(scope.begin >= info.start_ip && scope.end <= info.end_ip);
  assert(scope.filterOrFinally && scope.handler);
  volatile int value = 0;
  assert(sehFixture(&value) == 0 && value == 1);
#endif

  makeImage();
  uintptr_t imageBase = reinterpret_cast<uintptr_t>(image);
  modules[0].imageBase = imageBase;
  modules[0].imageSize = ImageSize;
  auto putBE32 = [](uint8_t *p, uint32_t value) {
    p[0] = uint8_t(value >> 24);
    p[1] = uint8_t(value >> 16);
    p[2] = uint8_t(value >> 8);
    p[3] = uint8_t(value);
  };
  putBE32(image + 0x800, imageBase + 0x420);
  putBE32(image + 0x804, 0xc0000402);
  putBE32(image + 0x418, pc);
  putBE32(image + 0x41c, imageBase + 0xa00);
  assert(__unw_set_reg(&cursor, UNW_REG_IP, imageBase + 0x424) == UNW_ESUCCESS);
  assert(__unw_get_proc_info(&cursor, &info) == UNW_ESUCCESS);
  assert(info.start_ip == imageBase + 0x420 &&
         info.end_ip == imageBase + 0x430);
  assert(info.handler == pc && info.lsda == imageBase + 0xa00);
  assert(info.unwind_info == imageBase + 0x800);
  assert(__unw_step(&cursor) == UNW_EUNSPEC);
  dwarf_eh_bases bases = {};
  assert(_Unwind_Find_FDE(reinterpret_cast<void *>(imageBase + 0x424),
                          &bases) == nullptr);
  putBE32(image + 0x804, 0x40000000);
  assert(__unw_set_reg(&cursor, UNW_REG_IP, imageBase + 0x424) == UNW_ESUCCESS);
  assert(__unw_get_proc_info(&cursor, &info) == UNW_ENOINFO);
  assert(__unw_step(&cursor) == UNW_EBADFRAME);

  makeImage();
  makeImage(image2);
  modules[1].imageBase = reinterpret_cast<uintptr_t>(image2);
  modules[1].imageSize = ImageSize;
  putBE32(image + 0x800, imageBase + 0x420);
  putBE32(image + 0x804, 0x40000200);
  putBE32(image + 0x420, 0x60000000);
  putBE32(image + 0x424, 0x4e800020);
  putBE32(image2 + 0x800, modules[1].imageBase + 0x420);
  putBE32(image2 + 0x804, 0x40000000);
  assert(__unw_set_reg(&cursor, UNW_PPC64_LR,
                       modules[1].imageBase + 0x424) == UNW_ESUCCESS);
  assert(__unw_set_reg(&cursor, UNW_REG_IP, imageBase + 0x420) == UNW_ESUCCESS);
  unw_word_t originalSP, originalIP, originalLR;
  assert(__unw_get_reg(&cursor, UNW_REG_SP, &originalSP) == UNW_ESUCCESS);
  assert(__unw_get_reg(&cursor, UNW_REG_IP, &originalIP) == UNW_ESUCCESS);
  assert(__unw_get_reg(&cursor, UNW_PPC64_LR, &originalLR) == UNW_ESUCCESS);
  for (unsigned attempt = 0; attempt != 2; ++attempt) {
    assert(__unw_step(&cursor) == UNW_EBADFRAME);
    unw_word_t sp, ip, lr;
    assert(__unw_get_reg(&cursor, UNW_REG_SP, &sp) == UNW_ESUCCESS);
    assert(__unw_get_reg(&cursor, UNW_REG_IP, &ip) == UNW_ESUCCESS);
    assert(__unw_get_reg(&cursor, UNW_PPC64_LR, &lr) == UNW_ESUCCESS);
    assert(sp == originalSP && ip == originalIP && lr == originalLR);
    assert(__unw_get_proc_info(&cursor, &info) == UNW_ESUCCESS);
    assert(info.start_ip == imageBase + 0x420);
  }
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

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: target={{.*xbox360.*}}

#undef NDEBUG
#include "../src/Registers.hpp"
#if defined(LIBUNWIND_TEST_SYNTHETIC_CURSOR)
#include "../src/UnwindCursor.hpp"
char __eh_frame_start, __eh_frame_end;
char __eh_frame_hdr_start, __eh_frame_hdr_end;
// This synthetic cursor test does not execute DWARF parsing or stack probes.
extern "C" [[noreturn]] void _RtlCheckStack12() { abort(); }
#endif

#include <assert.h>
#include <string.h>

using namespace libunwind;

struct alignas(16) Results {
  uint64_t r14;
  double f14;
  v128 v96;
  uint32_t cr;
  uint32_t xer;
  uint64_t lr;
};

static const v128 Expected = {{0x12345678, 0x9abcdef0, 0x11223344, 0x55667788}};

extern "C" int test_capture(unw_context_t *, const v128 *, const double *,
                            uint64_t *, v128 *, const v128 *);
extern "C" void test_resume(unw_context_t *, Results *);
extern "C" int DbgPrint(const char *, ...);

extern "C" [[noreturn]] void test_do_resume(unw_context_t *context) {
  Registers_ppc64 registers(context);
  registers.setRegister(UNW_PPC64_R14, UINT64_C(0x123456789abcdef0));
  registers.setRegister(UNW_PPC64_CR2, 0x00500000);
  registers.setRegister(UNW_PPC64_CR5, 0x00000500);
  registers.setRegister(UNW_PPC64_XER, 0x20000000);
  registers.setFloatRegister(UNW_PPC64_F14, 123.5);
  registers.setVectorRegister(UNW_PPC64_V32 + 64, Expected);
  registers.jumpto();
  __builtin_unreachable();
}

asm(R"(
  .text
  .globl test_capture
test_capture:
  stdu 1, -144(1)
  mflr 0
  std 0, 128(1)
  stfd 14, 96(1)
  lfd 14, 0(5)
  mffs 0
  stfd 0, 80(1)
  mtfsfi 7, 1
  mffs 0
  stfd 0, 0(6)
  mfvscr 0
  addi 9, 1, 112
  stvx 0, 0, 9
  lvx 0, 0, 8
  mtvscr 0
  mfvscr 0
  stvx 0, 0, 7
  lfd 0, 0(5)
  addi 5, 1, 16
  stvx128 64, 0, 5
  addi 5, 1, 32
  stvx128 95, 0, 5
  addi 5, 1, 48
  stvx128 96, 0, 5
  addi 5, 1, 64
  stvx128 127, 0, 5
  lvx128 0, 0, 4
  lvx128 31, 0, 4
  lvx128 32, 0, 4
  lvx128 63, 0, 4
  lvx128 64, 0, 4
  lvx128 95, 0, 4
  lvx128 96, 0, 4
  lvx128 127, 0, 4
  bl __unw_getcontext
  addi 5, 1, 16
  lvx128 64, 0, 5
  addi 5, 1, 32
  lvx128 95, 0, 5
  addi 5, 1, 48
  lvx128 96, 0, 5
  addi 5, 1, 64
  lvx128 127, 0, 5
  addi 5, 1, 112
  lvx 0, 0, 5
  mtvscr 0
  lfd 0, 80(1)
  mtfsf 255, 0
  lfd 14, 96(1)
  ld 0, 128(1)
  mtlr 0
  addi 1, 1, 144
  blr

  .globl test_resume
test_resume:
  stdu 1, -128(1)
  mflr 0
  std 0, 112(1)
  std 14, 96(1)
  stfd 14, 88(1)
  mfcr 0
  stw 0, 80(1)
  stw 3, 32(1)
  stw 4, 36(1)
  addi 5, 1, 48
  stvx128 96, 0, 5
  li 0, 0
  stw 0, 40(1)
  bl __unw_getcontext
  lwz 0, 40(1)
  cmpwi 0, 0
  bne .Ltest_resumed
  li 0, 1
  stw 0, 40(1)
  lwz 3, 32(1)
  bl test_do_resume
  trap
.Ltest_resumed:
  lwz 4, 36(1)
  std 14, 0(4)
  stfd 14, 8(4)
  addi 5, 4, 16
  stvx128 96, 0, 5
  mfcr 0
  stw 0, 32(4)
  mfxer 0
  stw 0, 36(4)
  mflr 0
  std 0, 40(4)
  addi 5, 1, 48
  lvx128 96, 0, 5
  ld 14, 96(1)
  lfd 14, 88(1)
  lwz 0, 80(1)
  mtcr 0
  ld 0, 112(1)
  mtlr 0
  addi 1, 1, 128
  blr
)");

int main() {
  static_assert(sizeof(unw_word_t) == 8, "Xbox GPRs are 64 bits");
  static_assert(sizeof(uintptr_t) == 4, "Xbox addresses are 32 bits");
  static_assert(sizeof(Registers_ppc64) == sizeof(unw_context_t),
                "register context size");
  static_assert(offsetof(Results, v96) == 16 && offsetof(Results, cr) == 32 &&
                    offsetof(Results, lr) == 40,
                "assembly result layout");

  alignas(16) v128 input = Expected;
  double inputFloat = 45.25;
  unw_context_t context;
  memset(&context, 0xa5, sizeof(context));
  uint64_t expectedFPSCR;
  alignas(16) v128 expectedVSCR;
  alignas(16) v128 inputVSCR = {{0, 0, 0, 0x10000}};
  assert(test_capture(&context, &input, &inputFloat, &expectedFPSCR,
                      &expectedVSCR, &inputVSCR) == 0);
  Registers_ppc64 registers(&context);
  assert(context.data[1] == 0);
  assert((expectedFPSCR & 3) != 0);
  assert((expectedVSCR.vec[3] & 0x10000) != 0);
  assert(registers.getXboxFPSCR() == expectedFPSCR);
  v128 vscr = registers.getXboxVSCR();
  assert(memcmp(&vscr, &expectedVSCR, sizeof(vscr)) == 0);
  assert(registers.getFloatRegister(UNW_PPC64_F0) == inputFloat);
  assert(registers.getFloatRegister(UNW_PPC64_F14) == inputFloat);
  const int vectors[] = {UNW_PPC64_V0,       UNW_PPC64_V31,
                         UNW_PPC64_V32,      UNW_PPC64_V32 + 31,
                         UNW_PPC64_V32 + 32, UNW_PPC64_V32 + 63,
                         UNW_PPC64_V32 + 64, UNW_PPC64_V127};
  for (int vector : vectors) {
    assert(registers.validVectorRegister(vector));
    v128 value = registers.getVectorRegister(vector);
    assert(memcmp(&value, &Expected, sizeof(value)) == 0);
  }
  assert(!registers.validVectorRegister(UNW_PPC64_V127 + 1));
  assert(strcmp(registers.getRegisterName(UNW_PPC64_V32), "v32") == 0);
  assert(strcmp(registers.getRegisterName(UNW_PPC64_V127), "v127") == 0);

#if defined(LIBUNWIND_TEST_SYNTHETIC_CURSOR)
  LocalAddressSpace addressSpace;
  uint64_t savedRegister = UINT64_C(0x0123456789abcdef);
  assert(addressSpace.getRegister((uintptr_t)&savedRegister) == savedRegister);
  UnwindCursor<LocalAddressSpace, Registers_ppc64> cursor(&context,
                                                          addressSpace);
  unw_cursor_t *opaqueCursor = reinterpret_cast<unw_cursor_t *>(&cursor);
  unw_word_t word = 0;
  assert(__unw_set_reg(opaqueCursor, UNW_PPC64_R14,
                       UINT64_C(0xfedcba9876543210)) == UNW_ESUCCESS);
  assert(__unw_get_reg(opaqueCursor, UNW_PPC64_R14, &word) == UNW_ESUCCESS);
  assert(word == UINT64_C(0xfedcba9876543210));
#endif

  Results results = {};
  test_resume(&context, &results);
  assert(results.r14 == UINT64_C(0x123456789abcdef0));
  assert(results.f14 == 123.5);
  assert(memcmp(&results.v96, &Expected, sizeof(results.v96)) == 0);
  assert((results.cr & 0x00f00f00) == 0x00500500);
  assert((results.xer & 0x20000000) != 0);
  Registers_ppc64 resumed(&context);
  assert(results.lr == resumed.getRegister(UNW_PPC64_LR));
  DbgPrint("libunwind xbox register context: PASS\n");
  return 0;
}

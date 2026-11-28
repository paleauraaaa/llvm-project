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
#include "../src/XboxUnwindStep.hpp"
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

struct alignas(16) SEHCallValues {
  uint64_t gpr[18];
  double fpr[18];
  v128 vectors[64];
  uint32_t cr, parent, argument;
  int32_t result;
};

static const v128 Expected = {{0x12345678, 0x9abcdef0, 0x11223344, 0x55667788}};

static v128 sehVector(unsigned value) {
  return {{0x12340000 + value, 0xabcd0000 + value, 0x23450000 + value,
           0xbcde0000 + value}};
}

extern "C" int test_capture(unw_context_t *, const v128 *, const double *,
                            uint64_t *, v128 *, const v128 *);
extern "C" void test_resume(unw_context_t *, Results *);
extern "C" int test_seh_call(const Registers_ppc64 *, SEHCallValues *,
                             SEHCallValues *, uintptr_t, const v128 *,
                             const double *);
extern "C" int DbgPrint(const char *, ...);
extern "C" const uint8_t __ImageBase;
extern "C" const uint8_t test_seh_after_call;

static const Registers_ppc64 *sehTarget;

struct SEHFrameMemory {
  bool isXboxCodeRange(uint32_t address, uint32_t size, uint32_t base,
                       uint32_t imageSize) const {
    return address >= base && address - base <= imageSize &&
           size <= imageSize - (address - base);
  }
  uint32_t get32(uint32_t address) const {
    return *reinterpret_cast<const uint32_t *>(address);
  }
  uint64_t get64(uint32_t address) const {
    return *reinterpret_cast<const uint64_t *>(address);
  }
  double getDouble(uint32_t address) const {
    return *reinterpret_cast<const double *>(address);
  }
  v128 getVector(uint32_t address) const {
    v128 value;
    memcpy(&value, reinterpret_cast<const void *>(address), sizeof(value));
    return value;
  }
};

struct SEHFrameRegisters {
  uint64_t sp, ip, lr, r14 = 0, r31 = 0;
  double f14 = 0, f31 = 0;
  v128 v64 = {}, v127 = {};
  uint32_t cr = 0;

  uint64_t getSP() const { return sp; }
  uint64_t getIP() const { return ip; }
  uint64_t getLR() const { return lr; }
  uint64_t getRegister(int reg) const {
    return reg == UNW_PPC64_R1 ? sp : sehTarget->getRegister(reg);
  }
  void setSP(uint64_t value) { sp = value; }
  void setIP(uint64_t value) { ip = value; }
  void setLR(uint64_t value) { lr = value; }
  void setRegister(int reg, uint64_t value) {
    if (reg == UNW_PPC64_R14)
      r14 = value;
    else if (reg == UNW_PPC64_R31)
      r31 = value;
    else if (reg >= UNW_PPC64_CR2 && reg <= UNW_PPC64_CR5)
      cr |= uint32_t(value);
  }
  void setFloatRegister(int reg, double value) {
    if (reg == UNW_PPC64_F14)
      f14 = value;
    else if (reg == UNW_PPC64_F31)
      f31 = value;
  }
  void setVectorRegister(int reg, v128 value) {
    if (reg == UNW_PPC64_V32 + 32)
      v64 = value;
    else if (reg == UNW_PPC64_V127)
      v127 = value;
  }
};

extern "C" void test_seh_frame(uint32_t pc, uint32_t sp) {
  const uint8_t *image = &__ImageBase;
  uint32_t base = reinterpret_cast<uintptr_t>(image);
  uint32_t nt = xboxReadLE32(image + XboxPE::DosNTHeaderOffset);
  uint32_t size =
      xboxReadLE32(image + nt + XboxPE::NTHeaderPrefixSize +
                   XboxPE::ImageSizeOffset);
  uint32_t trampoline = reinterpret_cast<uintptr_t>(&__libunwind_xbox_seh_call);
  XboxUnwindSection pdata = {};
  assert(findXboxPdata(image, size, trampoline - base, pdata));
  XboxRuntimeFunction function = {};
  assert(findXboxRuntimeFunction(image, size, base, pdata.offset, pdata.length,
                                 trampoline, function) == XboxPdataResult::Found);
  assert(function.start == trampoline && function.handler == 0);

  unw_proc_info_t info = {};
  info.start_ip = function.start;
  info.end_ip = function.end;
  info.format = function.packed;
  SEHFrameMemory memory;
  SEHFrameRegisters regs = {sp, pc, pc};
  XboxFrame::Interpreter<SEHFrameMemory, SEHFrameRegisters> interpreter(
      memory, regs, info, base, size);
  assert(interpreter.step() == UNW_STEP_SUCCESS);
  assert(regs.sp == memory.get32(sp));
  assert(regs.ip == reinterpret_cast<uintptr_t>(&test_seh_after_call));
  assert(regs.r14 == 34 && regs.r31 == 51);
  assert(regs.f14 == 300.5 && regs.f31 == 317.5);
  const v128 callerFirst = sehVector(100);
  const v128 callerLast = sehVector(163);
  assert(memcmp(&regs.v64, &callerFirst, sizeof(callerFirst)) == 0);
  assert(memcmp(&regs.v127, &callerLast, sizeof(callerLast)) == 0);
  assert((regs.cr & 0x00ffff00) == 0x006a5b00);
}

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

  .globl test_seh_funclet
test_seh_funclet:
  mflr 0
  stw 0, -8(1)
  stwu 1, -96(1)
  .irp n, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    std \n, ((\n - 14) * 8)(3)
    stfd \n, (144 + (\n - 14) * 8)(3)
  .endr
  .irp n, 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
    addi 5, 3, (288 + (\n - 64) * 16)
    stvx128 \n, 0, 5
  .endr
  mfcr 0
  stw 0, 1312(3)
  stw 12, 1316(3)
  stw 3, 1320(3)
  lwz 3, 88(1)
  lwz 4, 0(1)
  bl test_seh_frame
  lwz 0, 88(1)
  mtlr 0
  addi 1, 1, 96
  li 3, -7
  blr

  .globl test_seh_call
test_seh_call:
  mflr 0
  stw 0, -8(1)
  mfcr 0
  stw 0, -12(1)
  stwu 1, -1440(1)
  .irp n, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    std \n, (96 + (\n - 14) * 8)(1)
    stfd \n, (240 + (\n - 14) * 8)(1)
  .endr
  .irp n, 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
    addi 0, 1, (384 + (\n - 64) * 16)
    stvx128 \n, 0, 0
  .endr
  stw 4, 80(1)
  stw 5, 84(1)
  stw 6, 88(1)
  .irp n, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    li \n, (20 + \n)
    lfd \n, ((\n - 14) * 8)(8)
  .endr
  .irp n, 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
    addi 0, 7, ((\n - 64) * 16)
    lvx128 \n, 0, 0
  .endr
  lis 0, 0x006a
  ori 0, 0, 0x5b00
  mtcrf 0x3c, 0
  lis 4, test_seh_funclet@ha
  addi 4, 4, test_seh_funclet@l
  lwz 5, 80(1)
  lwz 6, 88(1)
  bl __libunwind_xbox_seh_call
  .globl test_seh_after_call
test_seh_after_call:
  lwz 4, 84(1)
  stw 3, 1324(4)
  .irp n, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    std \n, ((\n - 14) * 8)(4)
    stfd \n, (144 + (\n - 14) * 8)(4)
  .endr
  .irp n, 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
    addi 5, 4, (288 + (\n - 64) * 16)
    stvx128 \n, 0, 5
  .endr
  mfcr 0
  stw 0, 1312(4)
  .irp n, 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
    addi 0, 1, (384 + (\n - 64) * 16)
    lvx128 \n, 0, 0
  .endr
  .irp n, 14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    ld \n, (96 + (\n - 14) * 8)(1)
    lfd \n, (240 + (\n - 14) * 8)(1)
  .endr
  lwz 0, 1428(1)
  mtcrf 0x3c, 0
  lwz 0, 1432(1)
  mtlr 0
  addi 1, 1, 1440
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
  static_assert(offsetof(SEHCallValues, vectors) == 288 &&
                    offsetof(SEHCallValues, cr) == 1312 &&
                    offsetof(SEHCallValues, result) == 1324,
                "SEH call result layout");

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

  static SEHCallValues observed = {}, restored = {};
  alignas(16) Registers_ppc64 sehRegisters = registers;
  alignas(16) static v128 callerVectors[64];
  static double callerFloats[18];
  for (unsigned i = 0; i != 18; ++i) {
    sehRegisters.setRegister(UNW_PPC64_R14 + i,
                             UINT64_C(0x1234567800000000) + i);
    sehRegisters.setFloatRegister(UNW_PPC64_F14 + i, 100.5 + i);
    callerFloats[i] = 300.5 + i;
  }
  for (unsigned i = 0; i != 64; ++i) {
    sehRegisters.setVectorRegister(UNW_PPC64_V32 + 32 + i, sehVector(i));
    callerVectors[i] = sehVector(100 + i);
  }
  sehRegisters.setCR(0x005a5a00);
  sehTarget = &sehRegisters;
  const uintptr_t parent = 0x12345670;
  assert(test_seh_call(&sehRegisters, &observed, &restored, parent,
                       callerVectors, callerFloats) == -7);
  for (unsigned i = 0; i != 18; ++i) {
    assert(observed.gpr[i] == UINT64_C(0x1234567800000000) + i);
    assert(observed.fpr[i] == 100.5 + i);
    assert(restored.gpr[i] == 34 + i);
    assert(restored.fpr[i] == callerFloats[i]);
  }
  for (unsigned i = 0; i != 64; ++i) {
    v128 expected = sehVector(i);
    assert(memcmp(&observed.vectors[i], &expected, sizeof(expected)) == 0);
    assert(memcmp(&restored.vectors[i], &callerVectors[i], sizeof(expected)) == 0);
  }
  assert((observed.cr & 0x00ffff00) == 0x005a5a00);
  assert(observed.parent == parent);
  assert(observed.argument == reinterpret_cast<uintptr_t>(&observed));
  assert(restored.result == -7);
  assert((restored.cr & 0x00ffff00) == 0x006a5b00);

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

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: target={{.*xbox360.*}}

#undef NDEBUG
#include "../src/XboxUnwindStep.hpp"
#include <assert.h>
#include <string.h>

using namespace libunwind;

#if defined(__powerpc64__) && defined(_XBOX) && _XBOX_VER == 200
extern "C" int DbgPrint(const char *, ...);
#endif

struct Vector {
  uint32_t words[4];
};

struct Memory {
  uint8_t bytes[0x4000] = {};
  bool isXboxCodeRange(uint32_t address, uint32_t size, uint32_t imageBase,
                       uint32_t imageSize) const {
    return address >= imageBase && address - imageBase <= imageSize &&
           size <= imageSize - (address - imageBase);
  }
  void put32(uint32_t address, uint32_t value) {
    assert(address <= sizeof(bytes) - 4);
    for (unsigned i = 0; i != 4; ++i)
      bytes[address + i] = uint8_t(value >> (24 - 8 * i));
  }
  void put64(uint32_t address, uint64_t value) {
    put32(address, uint32_t(value >> 32));
    put32(address + 4, uint32_t(value));
  }
  uint32_t get32(uint32_t address) const {
    assert(address <= sizeof(bytes) - 4);
    return xboxReadBE32(bytes + address);
  }
  uint64_t get64(uint32_t address) const {
    return uint64_t(get32(address)) << 32 | get32(address + 4);
  }
  double getDouble(uint32_t address) const {
    uint64_t bits = get64(address);
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
  }
  Vector getVector(uint32_t address) const {
    Vector value;
    for (unsigned i = 0; i != 4; ++i)
      value.words[i] = get32(address + 4 * i);
    return value;
  }
};

struct Registers {
  uint64_t gpr[32] = {};
  double fpr[32] = {};
  Vector vectors[64] = {};
  uint64_t ip = 0;
  uint64_t lr = 0;
  uint32_t cr = 0;
  uint64_t getIP() const { return ip; }
  uint64_t getLR() const { return lr; }
  uint64_t getSP() const { return gpr[1]; }
  uint64_t getRegister(int reg) const { return gpr[reg]; }
  void setIP(uint64_t value) { ip = value; }
  void setLR(uint64_t value) { lr = value; }
  void setSP(uint64_t value) { gpr[1] = value; }
  void setRegister(int reg, uint64_t value) {
    if (reg >= UNW_PPC64_CR0 && reg <= UNW_PPC64_CR7) {
      unsigned shift = 4 * (7 - (reg - UNW_PPC64_CR0));
      cr = (cr & ~(uint32_t(15) << shift)) | (uint32_t(value) & (15 << shift));
    } else {
      gpr[reg] = value;
    }
  }
  void setFloatRegister(int reg, double value) {
    fpr[reg - UNW_PPC64_F0] = value;
  }
  void setVectorRegister(int reg, Vector value) {
    vectors[reg - UNW_PPC64_V32 - 32] = value;
  }
};

static constexpr uint32_t Code = 0x1000;
static constexpr uint32_t CallerSP = 0x3000;
static constexpr uint32_t ReturnPC = 0x1100;

static unw_proc_info_t function(unsigned count, unsigned prologue) {
  unw_proc_info_t info = {};
  info.start_ip = Code;
  info.end_ip = Code + 4 * count;
  info.format = XboxPdata::PowerPCInstructions | (count << 8) | prologue;
  return info;
}

static int step(Memory &memory, Registers &registers,
                const unw_proc_info_t &info) {
  XboxFrame::Interpreter<Memory, Registers> frame(memory, registers, info, Code,
                                                  0x1000);
  return frame.step();
}

static void putInstructions(Memory &memory, const uint32_t *ins,
                            unsigned count) {
  for (unsigned i = 0; i != count; ++i)
    memory.put32(Code + 4 * i, ins[i]);
}

static void testBodyAndPartialPrologue() {
  const uint32_t ins[] = {
      0x7d8802a6, // mflr r12
      0x9181fff8, // stw r12,-8(r1)
      0xfbe1fff0, // std r31,-16(r1)
      0xdbe1ffe8, // stfd f31,-24(r1)
      0x7d800026, // mfcr r12
      0x9181ffe4, // stw r12,-28(r1)
      0x9421ff80, // stwu r1,-128(r1)
      0x7c3f0b78, // mr r31,r1
      0x48000101, // body call
      0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 10);
  memory.put32(CallerSP - 8, ReturnPC);
  memory.put64(CallerSP - 16, UINT64_C(0x0123456789abcdef));
  memory.put64(CallerSP - 24, UINT64_C(0x405ee00000000000));
  memory.put32(CallerSP - 28, 0x005a5a00);
  Registers regs;
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP - 128;
  regs.gpr[31] = CallerSP - 128;
  regs.lr = Code + 36;
  regs.cr = 0xff0000ff;
  regs.ip = Code + 36;
  Registers body = regs;
  body.ip = Code + 32;
  unw_proc_info_t info = function(10, 8);
  XboxFrame::Interpreter<Memory, Registers> established(memory, body, info,
                                                        Code, 0x1000);
  uint32_t frameSize = 128;
  assert(established.step(true, &frameSize) == UNW_STEP_SUCCESS);
  body = regs;
  body.ip = Code + 32;
  XboxFrame::Interpreter<Memory, Registers> wrongSize(memory, body, info, Code,
                                                      0x1000);
  frameSize = 144;
  assert(wrongSize.step(true, &frameSize) == UNW_EBADFRAME);
  assert(step(memory, regs, function(10, 8)) == UNW_STEP_SUCCESS);
  assert(regs.ip == ReturnPC && regs.lr == ReturnPC);
  assert(regs.gpr[1] == CallerSP);
  assert(regs.gpr[31] == UINT64_C(0x0123456789abcdef));
  assert(regs.fpr[31] == 123.5);
  assert(regs.cr == 0xff5a5aff);

  Registers partial;
  for (unsigned i = 14; i != 32; ++i)
    partial.gpr[i] = i;
  partial.gpr[1] = CallerSP;
  partial.lr = ReturnPC;
  partial.gpr[12] = ReturnPC;
  partial.ip = Code + 4;
  Registers rejected = partial;
  XboxFrame::Interpreter<Memory, Registers> prologue(memory, rejected, info,
                                                     Code, 0x1000);
  assert(prologue.step(true) == UNW_EBADFRAME);
  assert(memcmp(&rejected, &partial, sizeof(partial)) == 0);
  assert(step(memory, partial, function(10, 8)) == UNW_STEP_SUCCESS);
  assert(partial.gpr[1] == CallerSP && partial.gpr[31] == 31);
  assert(partial.ip == ReturnPC);

  regs = partial;
  regs.ip = Code + 16;
  memory.put32(Code + 12, 0xffffffff);
  Registers before = regs;
  assert(step(memory, regs, function(10, 8)) == UNW_EUNSPEC);
  assert(memcmp(&regs, &before, sizeof(regs)) == 0);
}

static void testVectorAndHelper() {
  const uint32_t ins[] = {
      0x9421ff00, // stwu r1,-256(r1)
      0x39600020, // li r11,32
      0x13e159cf, // stvx128 v127,r1,r11
      0xfbc10030, // std r30,48(r1)
      0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 5);
  for (unsigned i = 0; i != 4; ++i)
    memory.put32(CallerSP - 256 + 32 + 4 * i, 0x12340000 + i);
  memory.put64(CallerSP - 256 + 48, UINT64_C(0xaabbccddeeff1234));
  Registers regs;
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP - 256;
  regs.lr = ReturnPC;
  regs.ip = Code + 16;
  assert(step(memory, regs, function(5, 4)) == UNW_STEP_SUCCESS);
  assert(regs.gpr[30] == UINT64_C(0xaabbccddeeff1234));
  for (unsigned i = 0; i != 4; ++i)
    assert(regs.vectors[63].words[i] == 0x12340000 + i);

  const uint32_t helper[] = {
      0x7d8802a6, 0x48000015, 0x3d60ffff, 0x616b8000, 0x7c21596e,
      0x4e800020, 0xfbc1fff0, 0xfbe1fff8, 0x9181ffec, 0x4e800020,
  };
  putInstructions(memory, helper, 10);
  memory.put64(CallerSP - 16, 0x1234);
  memory.put64(CallerSP - 8, 0x5678);
  memory.put32(CallerSP - 20, ReturnPC);
  regs = {};
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = UINT64_C(0xffffb000);
  regs.ip = Code + 20;
  assert(step(memory, regs, function(6, 5)) == UNW_EBADFRAME);
  regs.gpr[1] = CallerSP;
  regs.ip = Code + 8;
  assert(step(memory, regs, function(6, 5)) == UNW_STEP_SUCCESS);
  assert(regs.ip == ReturnPC && regs.gpr[30] == 0x1234 &&
         regs.gpr[31] == 0x5678);
}

static void testEpilogue() {
  const uint32_t ins[] = {
      0x7d8802a6, 0x9181fff8, 0xfbe1fff0, 0x9421ff80, 0x48000101,
      0x38210080, 0x8181fff8, 0x7d8803a6, 0xebe1fff0, 0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 10);
  memory.put32(CallerSP - 8, ReturnPC);
  memory.put64(CallerSP - 16, 0x12345678);
  for (unsigned pc = 5; pc <= 9; ++pc) {
    Registers regs;
    for (unsigned i = 14; i != 32; ++i)
      regs.gpr[i] = i;
    regs.gpr[1] = pc > 5 ? CallerSP : CallerSP - 128;
    regs.gpr[12] = pc > 6 ? ReturnPC : 0;
    regs.gpr[31] = pc > 8 ? 0x12345678 : 31;
    regs.lr = pc > 7 ? ReturnPC : Code + 20;
    regs.ip = Code + 4 * pc;
    Registers rejected = regs;
    unw_proc_info_t info = function(10, 4);
    XboxFrame::Interpreter<Memory, Registers> epilogue(memory, rejected, info,
                                                       Code, 0x1000);
    assert(epilogue.step(true) == UNW_EBADFRAME);
    assert(memcmp(&rejected, &regs, sizeof(regs)) == 0);
    assert(step(memory, regs, function(10, 4)) == UNW_STEP_SUCCESS);
    assert(regs.ip == ReturnPC && regs.gpr[1] == CallerSP);
    assert(regs.gpr[31] == 0x12345678);
  }
}

static void testCleanupParentFramePointer() {
  const uint32_t ins[] = {
      0xfbe1fff0, // std r31,-16(r1)
      0x3becff80, // addi r31,r12,-128
      0x9421ffc0, // stwu r1,-64(r1)
      0x48000101, // body call
      0x3bff0010, // addi r31,r31,16
      0x48000101, // body call
      0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 7);
  memory.put64(CallerSP - 16, 0x1234);
  Registers regs;
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP - 64;
  regs.lr = ReturnPC;
  regs.ip = Code + 12;
  unw_proc_info_t info = function(7, 3);
  Registers before = regs;
  XboxFrame::Interpreter<Memory, Registers> valid(memory, regs, info, Code,
                                                   0x1000);
  assert(valid.step(true) == UNW_STEP_SUCCESS);
  regs = before;
  regs.ip = Code + 20;
  XboxFrame::Interpreter<Memory, Registers> changed(memory, regs, info, Code,
                                                     0x1000);
  assert(changed.step(true) == UNW_EUNSPEC);
}

static void testUnsupportedState() {
  const uint32_t ins[] = {
      0x7d8802a6, // mflr r12
      0x9181fff8, // stw r12,-8(r1)
      0x9421ff80, // stwu r1,-128(r1)
      0x48000101, // body call
      0x38210080, // addi r1,r1,128
      0x7c0102a6, // mfxer r0, not a recognized epilogue instruction
      0x60000000, // nop
      0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 8);
  memory.put32(CallerSP - 8, ReturnPC);
  Registers regs;
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP;
  regs.lr = Code + 16;
  regs.ip = Code + 24;
  Registers before = regs;
  assert(step(memory, regs, function(8, 3)) == UNW_EUNSPEC);
  assert(memcmp(&regs, &before, sizeof(regs)) == 0);

  memory.put32(Code, 0x48002001); // Helper target is outside the image.
  regs.ip = Code + 4;
  before = regs;
  assert(step(memory, regs, function(8, 1)) == UNW_EUNSPEC);
  assert(memcmp(&regs, &before, sizeof(regs)) == 0);

  memory.put32(Code, 0x4e800020); // blr with no frame progress
  regs.lr = regs.ip = Code;
  before = regs;
  assert(step(memory, regs, function(1, 0)) == UNW_EBADFRAME);
  assert(memcmp(&regs, &before, sizeof(regs)) == 0);
}

static void testBodyComparisons() {
  const uint32_t ins[] = {
      0x9421ff80, // stwu r1,-128(r1)
      0x28230000, // cmpldi r3,0
      0x2c210000, // cmpdi r1,0
      0x7c211840, // cmpld r1,r3
      0x7c211800, // cmpd r1,r3
      0x60000000, // nop
      0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 7);
  Registers regs;
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP - 128;
  regs.lr = ReturnPC;
  regs.ip = Code + 20;
  assert(step(memory, regs, function(7, 1)) == UNW_STEP_SUCCESS);
  assert(regs.ip == ReturnPC && regs.gpr[1] == CallerSP);
}

static void testDynamicFrame() {
  const uint32_t ins[] = {
      0x7d8802a6, 0x9181fff8, 0xfbe1fff0, 0x9421ff80,
      0x7c3f0b78, // mr r31,r1
      0x80010000, // lwz r0,0(r1)
      0x7c01596e, // stwux r0,r1,r11
      0x48000101, // body call
      0x80210000, // lwz r1,0(r1)
      0x8181fff8, 0x7d8803a6, 0xebe1fff0, 0x4e800020,
  };
  static Memory memory;
  putInstructions(memory, ins, 13);
  memory.put32(CallerSP - 8, ReturnPC);
  memory.put64(CallerSP - 16, 0x87654321);
  Registers regs;
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP - 256;
  regs.gpr[31] = CallerSP - 128;
  regs.lr = Code + 32;
  regs.ip = Code + 32;
  assert(step(memory, regs, function(13, 5)) == UNW_STEP_SUCCESS);
  assert(regs.ip == ReturnPC && regs.gpr[1] == CallerSP);
  assert(regs.gpr[31] == 0x87654321);

  regs = {};
  for (unsigned i = 14; i != 32; ++i)
    regs.gpr[i] = i;
  regs.gpr[1] = CallerSP;
  regs.gpr[31] = CallerSP - 128;
  regs.lr = Code + 32;
  regs.ip = Code + 36;
  assert(step(memory, regs, function(13, 5)) == UNW_STEP_SUCCESS);
  assert(regs.ip == ReturnPC && regs.gpr[1] == CallerSP);
  assert(regs.gpr[31] == 0x87654321);
}

static void testCleanupParent() {
  static Memory memory;
  const uint32_t sp = 0x2000, cleanup = 0x1200;
  memory.put32(sp + XBOX_SEH_CALL_TARGET_OFFSET, cleanup);
  memory.put32(sp + XBOX_SEH_CALL_PARENT_OFFSET, 0x3800);
  memory.put32(sp + XBOX_SEH_CALL_CLEANUP_OFFSET, 1);
  uint32_t parent = 0;
  assert(XboxFrame::getCleanupParent(memory, sp, cleanup, parent));
  assert(parent == 0x3800 && parent != sp);
  assert(!XboxFrame::getCleanupParent(memory, sp, cleanup + 4, parent));
  memory.put32(sp + XBOX_SEH_CALL_CLEANUP_OFFSET, 0);
  assert(!XboxFrame::getCleanupParent(memory, sp, cleanup, parent));
  memory.put32(sp + XBOX_SEH_CALL_CLEANUP_OFFSET, 2);
  assert(!XboxFrame::getCleanupParent(memory, sp, cleanup, parent));
  memory.put32(sp + XBOX_SEH_CALL_CLEANUP_OFFSET, 1);
  memory.put32(sp + XBOX_SEH_CALL_PARENT_OFFSET, 0);
  assert(!XboxFrame::getCleanupParent(memory, sp, cleanup, parent));
  memory.put32(sp + XBOX_SEH_CALL_PARENT_OFFSET, 0x3804);
  assert(!XboxFrame::getCleanupParent(memory, sp, cleanup, parent));
  assert(!XboxFrame::getCleanupParent(memory, sp + 4, cleanup, parent));
  assert(!XboxFrame::getCleanupParent(memory, 0, cleanup, parent));
  assert(!XboxFrame::getCleanupParent(memory, UINT64_C(0x100000000), cleanup,
                                      parent));
  assert(!XboxFrame::getCleanupParent(memory, UINT64_C(0xfffffff0), cleanup,
                                      parent));
  assert(parent == 0x3800);

  memory.put32(sp + XBOX_SEH_CALL_PARENT_OFFSET, 0x3800);
  memory.put32(sp + XBOX_SEH_CALL_EXCEPTION_OFFSET, 0x3100);
  memory.put32(sp + XBOX_SEH_CALL_CURSOR_OFFSET, 0x3200);
  memory.put32(sp + XBOX_SEH_CALL_SCOPE_OFFSET, 7);
  XboxFrame::CleanupState state = {};
  assert(XboxFrame::getCleanupState(memory, sp, cleanup, parent, state));
  assert(parent == 0x3800 && state.exception == 0x3100 &&
         state.cursor == 0x3200 && state.scopeIndex == 7);
  memory.put32(sp + XBOX_SEH_CALL_EXCEPTION_OFFSET, 0);
  assert(!XboxFrame::getCleanupState(memory, sp, cleanup, parent, state));
  memory.put32(sp + XBOX_SEH_CALL_EXCEPTION_OFFSET, 0x3101);
  assert(!XboxFrame::getCleanupState(memory, sp, cleanup, parent, state));
  memory.put32(sp + XBOX_SEH_CALL_EXCEPTION_OFFSET, 0x3100);
  memory.put32(sp + XBOX_SEH_CALL_CURSOR_OFFSET, 0);
  assert(!XboxFrame::getCleanupState(memory, sp, cleanup, parent, state));
  memory.put32(sp + XBOX_SEH_CALL_CURSOR_OFFSET, 0x3201);
  assert(!XboxFrame::getCleanupState(memory, sp, cleanup, parent, state));
  assert(!XboxFrame::getCleanupState(memory, sp, cleanup + 4, parent, state));
  assert(!XboxFrame::getCleanupState(memory, UINT64_C(0xfffffa70), cleanup,
                                     parent, state));
  assert(parent == 0x3800 && state.exception == 0x3100 &&
         state.cursor == 0x3200 && state.scopeIndex == 7);
}

int main() {
  testBodyAndPartialPrologue();
  testVectorAndHelper();
  testEpilogue();
  testCleanupParentFramePointer();
  testUnsupportedState();
  testBodyComparisons();
  testDynamicFrame();
  testCleanupParent();
#if defined(__powerpc64__) && defined(_XBOX) && _XBOX_VER == 200
  DbgPrint("libunwind xbox frame restoration: PASS\n");
#endif
}

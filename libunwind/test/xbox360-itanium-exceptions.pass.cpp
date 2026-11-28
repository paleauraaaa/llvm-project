//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: target={{.*xbox360.*}}
// ADDITIONAL_COMPILE_FLAGS: -mxbox360-libunwind -fc++-abi=itanium -fms-compatibility-version=19.40

#undef NDEBUG
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>

extern "C" const uint8_t __ImageBase;

struct LoaderEntry {
  uint8_t reserved0[0x1c];
  uintptr_t imageBase;
  uint8_t reserved1[0x18];
  uint32_t imageSize;
};

static LoaderEntry module;
static int ready;
static int cleanups;

static uint32_t readLE32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
         (uint32_t(p[3]) << 24);
}

extern "C" void *mockPcToFileHeader(void *, LoaderEntry **)
    __asm__("XexPcToFileHeader");
extern "C" void *mockPcToFileHeader(void *pc, LoaderEntry **out) {
  uintptr_t address = reinterpret_cast<uintptr_t>(pc);
  *out = address >= module.imageBase &&
                 address - module.imageBase < module.imageSize
             ? &module
             : nullptr;
  return nullptr;
}

#if defined(LIBUNWIND_TEST_XBOX_FAIL_FAST_STACK_PROBE)
// The mock is valid only when DWARF frame parsing is not used.
extern "C" [[noreturn]] void _RtlCheckStack12() { __builtin_trap(); }
#endif

struct Cleanup {
  ~Cleanup() { __atomic_fetch_add(&cleanups, 1, __ATOMIC_RELAXED); }
};

__attribute__((noinline)) static void raiseException(int value) {
  Cleanup cleanup;
  throw value;
}

static void *worker(void *argument) {
  int value = static_cast<int>(reinterpret_cast<uintptr_t>(argument));
  try {
    try {
      raiseException(value);
    } catch (int caught) {
      assert(caught == value);
      __atomic_fetch_add(&ready, 1, __ATOMIC_SEQ_CST);
      while (__atomic_load_n(&ready, __ATOMIC_SEQ_CST) != 2)
        sched_yield();
      throw;
    }
  } catch (int caught) {
    assert(caught == value);
    return nullptr;
  }
  return reinterpret_cast<void *>(1);
}

int main() {
  const uint8_t *base = &__ImageBase;
  module.imageBase = reinterpret_cast<uintptr_t>(base);
  module.imageSize = readLE32(base + readLE32(base + 0x3c) + 24 + 56);

  pthread_t threads[2];
  for (int i = 0; i != 2; ++i)
    assert(pthread_create(&threads[i], nullptr, worker,
                          reinterpret_cast<void *>(uintptr_t(i + 41))) == 0);
  for (pthread_t thread : threads) {
    void *result;
    assert(pthread_join(thread, &result) == 0 && result == nullptr);
  }
  assert(__atomic_load_n(&cleanups, __ATOMIC_RELAXED) == 2);
}

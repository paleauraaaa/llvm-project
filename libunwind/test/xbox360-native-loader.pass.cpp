//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: target={{.*xbox360.*}}
// clang-format off
// ADDITIONAL_COMPILE_FLAGS: -mxbox360-libunwind -fc++-abi=itanium -fms-compatibility-version=19.40
// clang-format on

#undef NDEBUG
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>

struct LoaderEntry {
  uint8_t reserved0[0x1c];
  uintptr_t imageBase;
  uint8_t reserved1[0x18];
  uint32_t imageSize;
};

extern "C" const uint8_t __ImageBase;
extern "C" __declspec(dllimport) void *XexPcToFileHeader(void *,
                                                         LoaderEntry **);

static int ready;
static int cleanups;

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
  LoaderEntry *entry = nullptr;
  void *base = XexPcToFileHeader(const_cast<uint8_t *>(&__ImageBase), &entry);
  assert(base == &__ImageBase && entry);
  assert(entry->imageBase == reinterpret_cast<uintptr_t>(base));
  assert(entry->imageSize != 0);

  LoaderEntry *lastEntry = nullptr;
  assert(entry->imageSize <= UINTPTR_MAX - entry->imageBase);
  assert(XexPcToFileHeader(
             reinterpret_cast<void *>(entry->imageBase + entry->imageSize - 1),
             &lastEntry) == base);
  assert(lastEntry == entry);

  LoaderEntry *codeEntry = nullptr;
  assert(XexPcToFileHeader(reinterpret_cast<void *>(&worker), &codeEntry) ==
         base);
  assert(codeEntry == entry);

  LoaderEntry *unchanged = entry;
  assert(XexPcToFileHeader(nullptr, &unchanged) == nullptr);
  assert(unchanged == entry);

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

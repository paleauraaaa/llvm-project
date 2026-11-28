//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LIBUNWIND_XBOX_UNWIND_STATE_H
#define LIBUNWIND_XBOX_UNWIND_STATE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <unwind.h>

// Xbox function pointers are instruction-aligned, so their low bit is free.
// Keep the selected SEH scope in private_1 and the target SP in private_2.
static inline bool
__libunwind_xbox_has_seh_target(const _Unwind_Exception *exception) {
  return (exception->private_1 & 1) != 0;
}

static inline bool
__libunwind_xbox_is_forced(const _Unwind_Exception *exception) {
  return exception->private_1 != 0 &&
         !__libunwind_xbox_has_seh_target(exception);
}

static inline bool __libunwind_xbox_set_seh_target(
    _Unwind_Exception *exception, uintptr_t sp, uintptr_t scope) {
  if (exception->private_1 != 0 || !sp || (sp & 15) ||
      scope > (UINTPTR_MAX >> 1))
    return false;
  exception->private_1 = (scope << 1) | 1;
  exception->private_2 = sp;
  return true;
}

static inline uintptr_t
__libunwind_xbox_seh_target_scope(const _Unwind_Exception *exception) {
  assert(__libunwind_xbox_has_seh_target(exception));
  return exception->private_1 >> 1;
}

#endif

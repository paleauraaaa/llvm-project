//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//
// Implements C++ ABI Exception Handling Level 1 as documented at:
//      https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html
// using libunwind
//
//===----------------------------------------------------------------------===//

// ARM EHABI does not specify _Unwind_{Get,Set}{GR,IP}().  Thus, we are
// defining inline functions to delegate the function calls to
// _Unwind_VRS_{Get,Set}().  However, some applications might declare the
// function prototype directly (instead of including <unwind.h>), thus we need
// to export these functions from libunwind.so as well.
#define _LIBUNWIND_UNWIND_LEVEL1_EXTERNAL_LINKAGE 1

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "libunwind.h"
#include "libunwind_ext.h"
#include "shadow_stack_unwind.h"
#include "unwind.h"

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(__USING_SJLJ_EXCEPTIONS__)
#include "XboxUnwindState.h"
#endif

#if !defined(_LIBUNWIND_ARM_EHABI) && !defined(__USING_SJLJ_EXCEPTIONS__) &&   \
    !defined(__wasm__)

#ifndef _LIBUNWIND_SUPPORT_SEH_UNWIND

// When shadow stack is enabled, a separate stack containing only return
// addresses would be maintained. On function return, the return address would
// be compared to the popped address from shadow stack to ensure the return
// target is not tempered with. When unwinding, we're skipping the normal return
// procedure for multiple frames and thus need to pop the return addresses of
// the skipped frames from shadow stack to avoid triggering an exception (using
// `_LIBUNWIND_POP_SHSTK_SSP()`). Also, some architectures, like the x86-family
// CET, push the return adddresses onto shadow stack with common call
// instructions, so for these architectures, normal function calls should be
// avoided when invoking the `jumpto()` function. To do this, we use inline
// assemblies to "goto" the `jumpto()` for these architectures.
#if !defined(_LIBUNWIND_USE_CET) && !defined(_LIBUNWIND_USE_GCS)
#define __unw_phase2_resume(cursor, payload)                                   \
  do {                                                                         \
    __unw_resume_with_frames_walked((cursor), (payload));                      \
  } while (0)
#elif defined(_LIBUNWIND_TARGET_I386)
#define __shstk_step_size (4)
#define __unw_phase2_resume(cursor, payload)                                   \
  do {                                                                         \
    _LIBUNWIND_POP_SHSTK_SSP((payload));                                       \
    void *shstkRegContext = __libunwind_shstk_get_registers((cursor));         \
    void *shstkJumpAddress = __libunwind_shstk_get_jump_target();              \
    __asm__ volatile("push %%edi\n\t"                                          \
                     "sub $4, %%esp\n\t"                                       \
                     "jmp *%%edx\n\t" ::"D"(shstkRegContext),                  \
                     "d"(shstkJumpAddress));                                   \
  } while (0)
#elif defined(_LIBUNWIND_TARGET_X86_64)
#define __shstk_step_size (8)
#define __unw_phase2_resume(cursor, payload)                                   \
  do {                                                                         \
    _LIBUNWIND_POP_SHSTK_SSP((payload));                                       \
    void *shstkRegContext = __libunwind_shstk_get_registers((cursor));         \
    void *shstkJumpAddress = __libunwind_shstk_get_jump_target();              \
    __asm__ volatile("jmpq *%%rdx\n\t" ::"D"(shstkRegContext),                 \
                     "d"(shstkJumpAddress));                                   \
  } while (0)
#elif defined(_LIBUNWIND_TARGET_AARCH64)
#define __shstk_step_size (8)
#define __unw_phase2_resume(cursor, payload)                                   \
  do {                                                                         \
    _LIBUNWIND_POP_SHSTK_SSP((payload));                                       \
    void *shstkRegContext = __libunwind_shstk_get_registers((cursor));         \
    void *shstkJumpAddress = __libunwind_shstk_get_jump_target();              \
    __asm__ volatile("mov x0, %0\n\t"                                          \
                     "mov x1, #0\n\t"                                         \
                     "br %1\n\t"                                               \
                     :                                                         \
                     : "r"(shstkRegContext), "r"(shstkJumpAddress)             \
                     : "x0", "x1");                                            \
  } while (0)
#endif

// We need this helper function as the semantics of casting between integers and
// function pointers mean that we end up with a function pointer without the
// correct signature. Instead we assign to an integer with a matching schema,
// and then memmove the result into a variable of the correct type. This memmove
// is possible as `_Unwind_Personality_Fn` is a standard function pointer, and
// as such is not address diversified.
static _Unwind_Personality_Fn get_handler_function(unw_proc_info_t *frameInfo) {
  uintptr_t __unwind_ptrauth_restricted_intptr(ptrauth_key_function_pointer,
                                               0,
                                               ptrauth_function_pointer_type_discriminator(_Unwind_Personality_Fn))
    reauthenticatedIntegerHandler = frameInfo->handler;
  _Unwind_Personality_Fn handler;
  memmove(&handler, (void *)&reauthenticatedIntegerHandler,
          sizeof(_Unwind_Personality_Fn));
  return handler;
}

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
#define XBOX_SEH_EXCEPTION_CODE 0x20474343U
#define XBOX_UNWIND_NOINLINE __attribute__((noinline))

// These layouts match the Xbox 360 EXCEPTION_RECORD and CONTEXT ABI.
typedef struct {
  uint32_t flags, msr, iar, lr;
  uint64_t ctr, gpr[32];
  uint32_t cr, xer;
  double fpscr, fpr[32];
  uint32_t user_control, fill;
  float vscr[4], vr[128][4];
} xbox_seh_context;

typedef struct {
  uint32_t code, flags;
  void *nested, *address;
  uint32_t parameter_count;
  uintptr_t parameters[15];
} xbox_seh_exception_record;

typedef struct {
  xbox_seh_exception_record *record;
  xbox_seh_context *context;
} xbox_seh_exception_pointers;

static int xbox_seh_control_pc(unw_cursor_t *cursor, uintptr_t *pc) {
  unw_word_t ip;
  if (__unw_get_reg(cursor, UNW_REG_IP, &ip) != UNW_ESUCCESS || !ip ||
      ip > UINT32_MAX)
    return UNW_EBADFRAME;
  *pc = (uintptr_t)ip - 1;
  return UNW_ESUCCESS;
}

static int xbox_seh_frame(unw_cursor_t *cursor, unw_proc_info_t *info,
                          __unw_xbox_seh_frame *frame) {
  memset(frame, 0, sizeof(*frame));
  if (!(info->format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC))
    return UNW_ESUCCESS;
  int status = __unw_xbox_get_seh_frame(cursor, frame);
  if (status == UNW_ENOINFO && !info->handler)
    return UNW_ESUCCESS;
  if (status != UNW_ESUCCESS ||
      (info->handler && !(frame->flags & UNW_XBOX_SEH_C_HANDLER)))
    return UNW_EBADFRAME;
  return UNW_ESUCCESS;
}

// A boundary is encountered either at the cleanup itself, after processing
// its local scopes, or at the marked displaced-exception callback.
static int xbox_seh_boundary(unw_cursor_t *cursor, bool cleanup,
                             __unw_xbox_seh_cleanup_state *state) {
  uintptr_t parent;
  if (cleanup)
    return __unw_xbox_get_seh_cleanup_state(cursor, &parent, state);
  return __unw_xbox_get_seh_collision_state(cursor, &parent, state);
}

static int xbox_seh_continue(unw_cursor_t *cursor,
                             __unw_xbox_seh_cleanup_state *state,
                             _Unwind_Exception *replacement, bool commit,
                             uint32_t *next_scope) {
  uintptr_t pc;
  unw_cursor_t continuation;
  uint32_t next;
  if (xbox_seh_control_pc(state->cursor, &pc) != UNW_ESUCCESS ||
      __unw_xbox_continue_seh_cleanup(state, pc, &continuation, &next) !=
          UNW_ESUCCESS)
    return UNW_EBADFRAME;
  if (commit) {
    uintptr_t parent;
    __unw_xbox_seh_cleanup_state transferred;
    if (__unw_xbox_replace_seh_cleanup_exception(
            cursor, state->exception, replacement, &parent, &transferred) !=
        UNW_ESUCCESS)
      return UNW_EBADFRAME;
    *state = transferred;
  }
  // The committed callback may have changed control flow; never return into
  // the abandoned unwind if the saved continuation cannot be reconstructed.
  if (__unw_xbox_continue_seh_cleanup(state, pc, cursor, &next) != UNW_ESUCCESS) {
    if (commit)
      _LIBUNWIND_ABORT("invalid Xbox collided-unwind continuation");
    return UNW_EBADFRAME;
  }
  *next_scope = next;
  return UNW_ESUCCESS;
}

static int xbox_seh_search(unw_cursor_t *cursor, uintptr_t pc, uint32_t first,
                           __unw_xbox_seh_frame *frame,
                           xbox_seh_exception_pointers *pointers,
                           _Unwind_Exception *exception) {
  for (uint32_t index = first;; ++index) {
    __unw_xbox_seh_scope scope;
    int status = __unw_xbox_get_seh_scope(cursor, pc, index, &scope);
    if (status == UNW_XBOX_SEH_SCOPE_END)
      return 0;
    if (status < 0)
      return -1;
    if (status == UNW_XBOX_SEH_SCOPE_ACTIVE && scope.handler) {
      int disposition;
      if (__unw_xbox_call_seh_filter(cursor, pc, index, pointers,
                                     &disposition) != UNW_ESUCCESS ||
          disposition < 0)
        return -1;
      if (disposition > 0) {
        unw_word_t sp;
        if (!(frame->flags & UNW_XBOX_SEH_HANDLER_OWNERSHIP) ||
            __unw_get_reg(cursor, UNW_REG_SP, &sp) != UNW_ESUCCESS ||
            !__libunwind_xbox_set_seh_target(exception, (uintptr_t)sp, index))
          return -1;
        return 1;
      }
    }
    if (index == UINT32_MAX)
      return -1;
  }
}

static int xbox_seh_cleanup(unw_cursor_t *cursor, uintptr_t pc, uint32_t first,
                            bool target, uint32_t target_scope,
                            _Unwind_Exception *exception) {
  for (uint32_t index = first;; ++index) {
    __unw_xbox_seh_scope scope;
    int status = __unw_xbox_get_seh_scope(cursor, pc, index, &scope);
    if (status == UNW_XBOX_SEH_SCOPE_END)
      return target ? UNW_EBADFRAME : UNW_ESUCCESS;
    if (status < 0)
      return UNW_EBADFRAME;
    if (target && index == target_scope) {
      if (status != UNW_XBOX_SEH_SCOPE_ACTIVE || !scope.handler)
        return UNW_EBADFRAME;
      return __unw_xbox_transfer_seh_handler(
          cursor, pc, index, XBOX_SEH_EXCEPTION_CODE, exception);
    }
    if (status == UNW_XBOX_SEH_SCOPE_ACTIVE && !scope.handler &&
        __unw_xbox_call_seh_cleanup(cursor, pc, index, exception) !=
            UNW_ESUCCESS)
      return UNW_EBADFRAME;
    if (index == UINT32_MAX)
      return UNW_EBADFRAME;
  }
}

static _Unwind_Reason_Code xbox_phase2_error(bool committed,
                                              _Unwind_Reason_Code result) {
  if (committed)
    _LIBUNWIND_ABORT("Xbox collided unwind cannot return to abandoned frames");
  return result;
}
#endif

#if !defined(XBOX_UNWIND_NOINLINE)
#define XBOX_UNWIND_NOINLINE
#endif
XBOX_UNWIND_NOINLINE
static _Unwind_Reason_Code
unwind_phase1(unw_context_t *uc, unw_cursor_t *cursor, _Unwind_Exception *exception_object) {
  __unw_init_local(cursor, uc);

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
  xbox_seh_context context = {0};
  xbox_seh_exception_record record = {0};
  xbox_seh_exception_pointers pointers = {&record, &context};
  record.code = XBOX_SEH_EXCEPTION_CODE;
  record.flags = 1; // A libunwind exception cannot continue execution.
  record.parameter_count = 1;
  record.parameters[0] = (uintptr_t)exception_object;
  bool have_origin = false;
  bool process_current = false;
  uint32_t first_scope = 0;
#endif
  // Walk each frame looking for a place to stop.
  while (true) {
    // Ask libunwind to get next frame (skip over first which is
    // _Unwind_RaiseException).
    int stepResult;
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    if (process_current) {
      process_current = false;
      stepResult = UNW_STEP_SUCCESS;
    } else
#endif
      stepResult = __unw_step(cursor);
    if (stepResult == 0) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase1(ex_obj=%p): __unw_step() reached "
          "bottom => _URC_END_OF_STACK",
          (void *)exception_object);
      return _URC_END_OF_STACK;
    } else if (stepResult < 0) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase1(ex_obj=%p): __unw_step failed => "
          "_URC_FATAL_PHASE1_ERROR",
          (void *)exception_object);
      return _URC_FATAL_PHASE1_ERROR;
    }

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    uintptr_t pc;
    if (xbox_seh_control_pc(cursor, &pc) != UNW_ESUCCESS)
      return _URC_FATAL_PHASE1_ERROR;
    if (!have_origin) {
      // The captured call-site state is not a full fault context. A zero
      // ContextFlags explicitly marks all register groups unavailable.
      context.iar = (uint32_t)(pc & ~(uintptr_t)3);
      record.address = (void *)(uintptr_t)context.iar;
      have_origin = true;
    }
    __unw_xbox_seh_cleanup_state boundary;
    int boundary_status = xbox_seh_boundary(cursor, false, &boundary);
    if (boundary_status != UNW_ESUCCESS && boundary_status != UNW_ENOINFO)
      return _URC_FATAL_PHASE1_ERROR;
    if (boundary_status == UNW_ESUCCESS) {
      if (xbox_seh_continue(cursor, &boundary, exception_object, false,
                            &first_scope) != UNW_ESUCCESS)
        return _URC_FATAL_PHASE1_ERROR;
      process_current = true;
      continue;
    }
#endif
    // See if frame has code to run (has personality routine).
    unw_proc_info_t frameInfo;
    unw_word_t sp;
    if (__unw_get_proc_info(cursor, &frameInfo) != UNW_ESUCCESS) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase1(ex_obj=%p): __unw_get_proc_info "
          "failed => _URC_FATAL_PHASE1_ERROR",
          (void *)exception_object);
      return _URC_FATAL_PHASE1_ERROR;
    }

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    __unw_xbox_seh_frame xbox_frame;
    if (xbox_seh_frame(cursor, &frameInfo, &xbox_frame) != UNW_ESUCCESS)
      return _URC_FATAL_PHASE1_ERROR;
#endif
#ifndef NDEBUG
    // When tracing, print state information.
    if (_LIBUNWIND_TRACING_UNWINDING) {
      char functionBuf[512];
      const char *functionName = functionBuf;
      unw_word_t offset;
      if ((__unw_get_proc_name(cursor, functionBuf, sizeof(functionBuf),
                               &offset) != UNW_ESUCCESS) ||
          (frameInfo.start_ip + offset > frameInfo.end_ip))
        functionName = ".anonymous.";
      unw_word_t pc;
      __unw_get_reg(cursor, UNW_REG_IP, &pc);
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase1(ex_obj=%p): pc=0x%" PRIxPTR ", start_ip=0x%" PRIxPTR
          ", func=%s, lsda=0x%" PRIxPTR ", personality=0x%" PRIxPTR "",
          (void *)exception_object, (uintptr_t)pc, frameInfo.start_ip,
          functionName, frameInfo.lsda, frameInfo.handler);
    }
#endif

    // If there is a personality routine, ask it if it will want to stop at
    // this frame.
    if (frameInfo.handler != 0) {
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      if (frameInfo.format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC) {
        int found = xbox_seh_search(cursor, pc, first_scope, &xbox_frame,
                                    &pointers, exception_object);
        if (found < 0)
          return _URC_FATAL_PHASE1_ERROR;
        if (found > 0)
          return _URC_NO_REASON;
      } else
#elif defined(_XBOX) && _XBOX_VER == 200
      if (frameInfo.format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC)
        return _URC_FATAL_PHASE1_ERROR;
#endif
      {
      _Unwind_Personality_Fn p = get_handler_function(&frameInfo);
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase1(ex_obj=%p): calling personality function %p",
          (void *)exception_object, (void *)(uintptr_t)p);
      _Unwind_Reason_Code personalityResult =
          (*p)(1, _UA_SEARCH_PHASE, exception_object->exception_class,
               exception_object, (struct _Unwind_Context *)(cursor));
      switch (personalityResult) {
      case _URC_HANDLER_FOUND:
        // found a catch clause or locals that need destructing in this frame
        // stop search and remember stack pointer at the frame
        __unw_get_reg(cursor, UNW_REG_SP, &sp);
        exception_object->private_2 = (uintptr_t)sp;
        _LIBUNWIND_TRACE_UNWINDING(
            "unwind_phase1(ex_obj=%p): _URC_HANDLER_FOUND",
            (void *)exception_object);
        return _URC_NO_REASON;

      case _URC_CONTINUE_UNWIND:
        _LIBUNWIND_TRACE_UNWINDING(
            "unwind_phase1(ex_obj=%p): _URC_CONTINUE_UNWIND",
            (void *)exception_object);
        // continue unwinding
        break;

      default:
        // something went wrong
        _LIBUNWIND_TRACE_UNWINDING(
            "unwind_phase1(ex_obj=%p): _URC_FATAL_PHASE1_ERROR",
            (void *)exception_object);
        return _URC_FATAL_PHASE1_ERROR;
      }
      }
    }
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    first_scope = 0;
    if (xbox_frame.flags & UNW_XBOX_SEH_CLEANUP_FUNCLET) {
      if (xbox_seh_boundary(cursor, true, &boundary) != UNW_ESUCCESS ||
          xbox_seh_continue(cursor, &boundary, exception_object, false,
                            &first_scope) != UNW_ESUCCESS)
        return _URC_FATAL_PHASE1_ERROR;
      process_current = true;
    }
#endif
  }
  return _URC_NO_REASON;
}

#if defined(_LIBUNWIND_USE_GCS)
// Enable the GCS target feature to permit gcspop instructions to be used.
__attribute__((target("+gcs")))
#else
_LIBUNWIND_TRACE_NO_INLINE
#endif
XBOX_UNWIND_NOINLINE
static _Unwind_Reason_Code
unwind_phase2(unw_context_t *uc, unw_cursor_t *cursor,
              _Unwind_Exception *exception_object) {
  __unw_init_local(cursor, uc);

  _LIBUNWIND_TRACE_UNWINDING("unwind_phase2(ex_obj=%p)",
                             (void *)exception_object);

  // uc is initialized by __unw_getcontext in the parent frame. The first stack
  // frame walked is unwind_phase2.
  unsigned framesWalked = 1;
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
  bool process_current = false;
  bool committed = false;
  uint32_t first_scope = 0;
#endif
#if defined(_LIBUNWIND_USE_CET)
  unsigned long shadowStackTop = _get_ssp();
#elif defined(_LIBUNWIND_USE_GCS)
  unsigned long shadowStackTop = 0;
  if (__chkfeat(_CHKFEAT_GCS))
    shadowStackTop = (unsigned long)__gcspr();
#endif
  // Walk each frame until we reach where search phase said to stop.
  while (true) {

    // Ask libunwind to get next frame (skip over first which is
    // _Unwind_RaiseException).
    int stepResult;
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    if (process_current) {
      process_current = false;
      stepResult = UNW_STEP_SUCCESS;
    } else
#endif
      stepResult = __unw_step_stage2(cursor);
    if (stepResult == 0) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2(ex_obj=%p): __unw_step_stage2() reached "
          "bottom => _URC_END_OF_STACK",
          (void *)exception_object);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      return xbox_phase2_error(committed, _URC_END_OF_STACK);
#else
      return _URC_END_OF_STACK;
#endif
    } else if (stepResult < 0) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2(ex_obj=%p): __unw_step_stage2 failed => "
          "_URC_FATAL_PHASE1_ERROR",
          (void *)exception_object);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
      return _URC_FATAL_PHASE2_ERROR;
#endif
    }

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    __unw_xbox_seh_cleanup_state boundary;
    int boundary_status = xbox_seh_boundary(cursor, false, &boundary);
    if (boundary_status != UNW_ESUCCESS && boundary_status != UNW_ENOINFO)
      return xbox_phase2_error(true, _URC_FATAL_PHASE2_ERROR);
    if (boundary_status == UNW_ESUCCESS) {
      if (xbox_seh_continue(cursor, &boundary, exception_object, true,
                            &first_scope) != UNW_ESUCCESS)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
      committed = true;
      process_current = true;
      continue;
    }
#endif
    // Get info about this frame.
    unw_word_t sp;
    unw_proc_info_t frameInfo;
    __unw_get_reg(cursor, UNW_REG_SP, &sp);
    if (__unw_get_proc_info(cursor, &frameInfo) != UNW_ESUCCESS) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2(ex_obj=%p): __unw_get_proc_info "
          "failed => _URC_FATAL_PHASE1_ERROR",
          (void *)exception_object);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
      return _URC_FATAL_PHASE2_ERROR;
#endif
    }

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    __unw_xbox_seh_frame xbox_frame;
    uintptr_t pc;
    if (xbox_seh_control_pc(cursor, &pc) != UNW_ESUCCESS ||
        xbox_seh_frame(cursor, &frameInfo, &xbox_frame) != UNW_ESUCCESS)
      return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#endif
#ifndef NDEBUG
    // When tracing, print state information.
    if (_LIBUNWIND_TRACING_UNWINDING) {
      char functionBuf[512];
      const char *functionName = functionBuf;
      unw_word_t offset;
      if ((__unw_get_proc_name(cursor, functionBuf, sizeof(functionBuf),
                               &offset) != UNW_ESUCCESS) ||
          (frameInfo.start_ip + offset > frameInfo.end_ip))
        functionName = ".anonymous.";
      _LIBUNWIND_TRACE_UNWINDING("unwind_phase2(ex_obj=%p): start_ip=0x%" PRIxPTR
                                 ", func=%s, sp=0x%" PRIxPTR ", lsda=0x%" PRIxPTR
                                 ", personality=0x%" PRIxPTR,
                                 (void *)exception_object, frameInfo.start_ip,
                                 functionName, (uintptr_t)sp, frameInfo.lsda,
                                 frameInfo.handler);
    }
#endif

// In shadow stack enabled environment, we check return address stored in normal
// stack against return address stored in shadow stack, if the 2 addresses don't
// match, it means return address in normal stack has been corrupted, we return
// _URC_FATAL_PHASE2_ERROR.
#if defined(_LIBUNWIND_USE_CET) || defined(_LIBUNWIND_USE_GCS)
    if (shadowStackTop != 0) {
      unw_word_t retInNormalStack;
      __unw_get_reg(cursor, UNW_REG_IP, &retInNormalStack);
      unsigned long retInShadowStack =
          *(unsigned long *)(shadowStackTop + __shstk_step_size * framesWalked);
      if (retInNormalStack != retInShadowStack)
        return _URC_FATAL_PHASE2_ERROR;
    }
#endif
    ++framesWalked;
    // If there is a personality routine, tell it we are unwinding.
    if (frameInfo.handler != 0) {
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      if (frameInfo.format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC) {
        bool target = __libunwind_xbox_has_seh_target(exception_object) &&
                      sp == exception_object->private_2;
        uint32_t scope = target ? (uint32_t)__libunwind_xbox_seh_target_scope(
                                      exception_object)
                                : 0;
        if (xbox_seh_cleanup(cursor, pc, first_scope, target, scope,
                             exception_object) != UNW_ESUCCESS || target)
          return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
      } else
#elif defined(_XBOX) && _XBOX_VER == 200
      if (frameInfo.format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC)
        return _URC_FATAL_PHASE2_ERROR;
#endif
      {
      _Unwind_Personality_Fn p = get_handler_function(&frameInfo);
      _Unwind_Action action = _UA_CLEANUP_PHASE;
      if (sp == exception_object->private_2) {
        // Tell personality this was the frame it marked in phase 1.
        action = (_Unwind_Action)(_UA_CLEANUP_PHASE | _UA_HANDLER_FRAME);
      }
       _Unwind_Reason_Code personalityResult =
          (*p)(1, action, exception_object->exception_class, exception_object,
               (struct _Unwind_Context *)(cursor));
      switch (personalityResult) {
      case _URC_CONTINUE_UNWIND:
        // Continue unwinding
        _LIBUNWIND_TRACE_UNWINDING(
            "unwind_phase2(ex_obj=%p): _URC_CONTINUE_UNWIND",
            (void *)exception_object);
        if (sp == exception_object->private_2) {
          // Phase 1 said we would stop at this frame, but we did not...
          _LIBUNWIND_ABORT("during phase1 personality function said it would "
                           "stop here, but now in phase2 it did not stop here");
        }
        break;
      case _URC_INSTALL_CONTEXT:
        _LIBUNWIND_TRACE_UNWINDING(
            "unwind_phase2(ex_obj=%p): _URC_INSTALL_CONTEXT",
            (void *)exception_object);
        // Personality routine says to transfer control to landing pad.
        // We may get control back if landing pad calls _Unwind_Resume().
        if (_LIBUNWIND_TRACING_UNWINDING) {
          unw_word_t pc;
          __unw_get_reg(cursor, UNW_REG_IP, &pc);
          __unw_get_reg(cursor, UNW_REG_SP, &sp);
          _LIBUNWIND_TRACE_UNWINDING("unwind_phase2(ex_obj=%p): re-entering "
                                     "user code with ip=0x%" PRIxPTR
                                     ", sp=0x%" PRIxPTR,
                                     (void *)exception_object, (uintptr_t)pc,
                                     (uintptr_t)sp);
        }

        __unw_phase2_resume(cursor, framesWalked);
        // __unw_phase2_resume() only returns if there was an error.
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
        return _URC_FATAL_PHASE2_ERROR;
#endif
      default:
        // Personality routine returned an unknown result code.
        _LIBUNWIND_DEBUG_LOG("personality function returned unknown result %d",
                             personalityResult);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
        return _URC_FATAL_PHASE2_ERROR;
#endif
      }
      }
    }
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    first_scope = 0;
    if (xbox_frame.flags & UNW_XBOX_SEH_CLEANUP_FUNCLET) {
      if (xbox_seh_boundary(cursor, true, &boundary) != UNW_ESUCCESS ||
          xbox_seh_continue(cursor, &boundary, exception_object, true,
                            &first_scope) != UNW_ESUCCESS)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
      committed = true;
      process_current = true;
    }
#endif
  }

  // Clean up phase did not resume at the frame that the search phase
  // said it would...
  return _URC_FATAL_PHASE2_ERROR;
}

#if defined(_LIBUNWIND_USE_GCS)
// Enable the GCS target feature to permit gcspop instructions to be used.
__attribute__((target("+gcs")))
#else
_LIBUNWIND_TRACE_NO_INLINE
#endif
XBOX_UNWIND_NOINLINE
static _Unwind_Reason_Code
unwind_phase2_forced(unw_context_t *uc, unw_cursor_t *cursor,
                     _Unwind_Exception *exception_object, _Unwind_Stop_Fn stop,
                     void *stop_parameter) {
  __unw_init_local(cursor, uc);

  // uc is initialized by __unw_getcontext in the parent frame. The first stack
  // frame walked is unwind_phase2_forced.
  unsigned framesWalked = 1;
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
  bool process_current = false;
  bool committed = false;
  uint32_t first_scope = 0;
#endif
  // Walk each frame until we reach where search phase said to stop
  int stepResult;
  while (true) {
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    if (process_current) {
      process_current = false;
      stepResult = UNW_STEP_SUCCESS;
    } else
#endif
      stepResult = __unw_step_stage2(cursor);
    if (stepResult <= 0)
      break;

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    __unw_xbox_seh_cleanup_state boundary;
    int boundary_status = xbox_seh_boundary(cursor, false, &boundary);
    if (boundary_status != UNW_ESUCCESS && boundary_status != UNW_ENOINFO)
      return xbox_phase2_error(true, _URC_FATAL_PHASE2_ERROR);
    if (boundary_status == UNW_ESUCCESS) {
      if (xbox_seh_continue(cursor, &boundary, exception_object, true,
                            &first_scope) != UNW_ESUCCESS)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
      committed = true;
      process_current = true;
      continue;
    }
#endif

    // Update info about this frame.
    unw_proc_info_t frameInfo;
    if (__unw_get_proc_info(cursor, &frameInfo) != UNW_ESUCCESS) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2_forced(ex_obj=%p): __unw_get_proc_info "
          "failed => _URC_END_OF_STACK",
          (void *)exception_object);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
      return _URC_FATAL_PHASE2_ERROR;
#endif
    }

#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    __unw_xbox_seh_frame xbox_frame;
    uintptr_t pc;
    if (xbox_seh_control_pc(cursor, &pc) != UNW_ESUCCESS ||
        xbox_seh_frame(cursor, &frameInfo, &xbox_frame) != UNW_ESUCCESS)
      return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#endif
#ifndef NDEBUG
    // When tracing, print state information.
    if (_LIBUNWIND_TRACING_UNWINDING) {
      char functionBuf[512];
      const char *functionName = functionBuf;
      unw_word_t offset;
      if ((__unw_get_proc_name(cursor, functionBuf, sizeof(functionBuf),
                               &offset) != UNW_ESUCCESS) ||
          (frameInfo.start_ip + offset > frameInfo.end_ip))
        functionName = ".anonymous.";
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2_forced(ex_obj=%p): start_ip=0x%" PRIxPTR
          ", func=%s, lsda=0x%" PRIxPTR ", personality=0x%" PRIxPTR,
          (void *)exception_object, frameInfo.start_ip, functionName,
          frameInfo.lsda, frameInfo.handler);
    }
#endif

    // Call stop function at each frame.
    _Unwind_Action action =
        (_Unwind_Action)(_UA_FORCE_UNWIND | _UA_CLEANUP_PHASE);
    _Unwind_Reason_Code stopResult =
        (*stop)(1, action, exception_object->exception_class, exception_object,
                (struct _Unwind_Context *)(cursor), stop_parameter);
    _LIBUNWIND_TRACE_UNWINDING(
        "unwind_phase2_forced(ex_obj=%p): stop function returned %d",
        (void *)exception_object, stopResult);
    if (stopResult != _URC_NO_REASON) {
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2_forced(ex_obj=%p): stopped by stop function",
          (void *)exception_object);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
      return _URC_FATAL_PHASE2_ERROR;
#endif
    }

    ++framesWalked;
    // If there is a personality routine, tell it we are unwinding.
    if (frameInfo.handler != 0) {
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
      if (frameInfo.format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC) {
        if (xbox_seh_cleanup(cursor, pc, first_scope, false, 0,
                             exception_object) != UNW_ESUCCESS)
          return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
      } else
#elif defined(_XBOX) && _XBOX_VER == 200
      if (frameInfo.format & UNW_INFO_FORMAT_XBOX_PDATA_POWERPC)
        return _URC_FATAL_PHASE2_ERROR;
#endif
      {
      _Unwind_Personality_Fn p = get_handler_function(&frameInfo);
      _LIBUNWIND_TRACE_UNWINDING(
          "unwind_phase2_forced(ex_obj=%p): calling personality function %p",
          (void *)exception_object, (void *)(uintptr_t)p);
      _Unwind_Reason_Code personalityResult =
          (*p)(1, action, exception_object->exception_class, exception_object,
               (struct _Unwind_Context *)(cursor));
      switch (personalityResult) {
      case _URC_CONTINUE_UNWIND:
        _LIBUNWIND_TRACE_UNWINDING("unwind_phase2_forced(ex_obj=%p): "
                                   "personality returned "
                                   "_URC_CONTINUE_UNWIND",
                                   (void *)exception_object);
        // Destructors called, continue unwinding
        break;
      case _URC_INSTALL_CONTEXT:
        _LIBUNWIND_TRACE_UNWINDING("unwind_phase2_forced(ex_obj=%p): "
                                   "personality returned "
                                   "_URC_INSTALL_CONTEXT",
                                   (void *)exception_object);
        // We may get control back if landing pad calls _Unwind_Resume().
        __unw_phase2_resume(cursor, framesWalked);
        break;
      default:
        // Personality routine returned an unknown result code.
        _LIBUNWIND_TRACE_UNWINDING("unwind_phase2_forced(ex_obj=%p): "
                                   "personality returned %d, "
                                   "_URC_FATAL_PHASE2_ERROR",
                                   (void *)exception_object, personalityResult);
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
        return _URC_FATAL_PHASE2_ERROR;
#endif
      }
      }
    }
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
    first_scope = 0;
    if (xbox_frame.flags & UNW_XBOX_SEH_CLEANUP_FUNCLET) {
      if (xbox_seh_boundary(cursor, true, &boundary) != UNW_ESUCCESS ||
          xbox_seh_continue(cursor, &boundary, exception_object, true,
                            &first_scope) != UNW_ESUCCESS)
        return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
      committed = true;
      process_current = true;
    }
#endif
  }

#if defined(_XBOX) && _XBOX_VER == 200
  if (stepResult < 0)
#if !defined(_LIBUNWIND_IS_BAREMETAL)
    return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
    return _URC_FATAL_PHASE2_ERROR;
#endif
#endif
  // Call stop function one last time and tell it we've reached the end
  // of the stack.
  _LIBUNWIND_TRACE_UNWINDING("unwind_phase2_forced(ex_obj=%p): calling stop "
                             "function with _UA_END_OF_STACK",
                             (void *)exception_object);
  _Unwind_Action lastAction =
      (_Unwind_Action)(_UA_FORCE_UNWIND | _UA_CLEANUP_PHASE | _UA_END_OF_STACK);
  (*stop)(1, lastAction, exception_object->exception_class, exception_object,
          (struct _Unwind_Context *)(cursor), stop_parameter);

  // Clean up phase did not resume at the frame that the search phase said it
  // would.
#if defined(_XBOX) && _XBOX_VER == 200 && !defined(_LIBUNWIND_IS_BAREMETAL)
  return xbox_phase2_error(committed, _URC_FATAL_PHASE2_ERROR);
#else
  return _URC_FATAL_PHASE2_ERROR;
#endif
}

/// Called by __cxa_throw.  Only returns if there is a fatal error.
_LIBUNWIND_EXPORT _Unwind_Reason_Code
_Unwind_RaiseException(_Unwind_Exception *exception_object) {
  _LIBUNWIND_TRACE_API("_Unwind_RaiseException(ex_obj=%p)",
                       (void *)exception_object);
  unw_context_t uc;
  unw_cursor_t cursor;
  __unw_getcontext(&uc);

  // Mark that this is a non-forced unwind, so _Unwind_Resume()
  // can do the right thing.
  exception_object->private_1 = 0;
  exception_object->private_2 = 0;

  // phase 1: the search phase
  _Unwind_Reason_Code phase1 = unwind_phase1(&uc, &cursor, exception_object);
  if (phase1 != _URC_NO_REASON)
    return phase1;

  // phase 2: the clean up phase
  return unwind_phase2(&uc, &cursor, exception_object);
}



/// When _Unwind_RaiseException() is in phase2, it hands control
/// to the personality function at each frame.  The personality
/// may force a jump to a landing pad in that function, the landing
/// pad code may then call _Unwind_Resume() to continue with the
/// unwinding.  Note: the call to _Unwind_Resume() is from compiler
/// generated user code.  All other _Unwind_* routines are called
/// by the C++ runtime __cxa_* routines.
///
/// Note: re-throwing an exception (as opposed to continuing the unwind)
/// is implemented by having the code call __cxa_rethrow() which
/// in turn calls _Unwind_Resume_or_Rethrow().
_LIBUNWIND_EXPORT void
_Unwind_Resume(_Unwind_Exception *exception_object) {
  _LIBUNWIND_TRACE_API("_Unwind_Resume(ex_obj=%p)", (void *)exception_object);
  unw_context_t uc;
  unw_cursor_t cursor;
  __unw_getcontext(&uc);

#if defined(_XBOX) && _XBOX_VER == 200
  if (__libunwind_xbox_is_forced(exception_object))
#else
  if (exception_object->private_1 != 0)
#endif
    unwind_phase2_forced(&uc, &cursor, exception_object,
                         (_Unwind_Stop_Fn) exception_object->private_1,
                         (void *)exception_object->private_2);
  else
    unwind_phase2(&uc, &cursor, exception_object);

  // Clients assume _Unwind_Resume() does not return, so all we can do is abort.
  _LIBUNWIND_ABORT("_Unwind_Resume() can't return");
}



/// Not used by C++.
/// Unwinds stack, calling "stop" function at each frame.
/// Could be used to implement longjmp().
_LIBUNWIND_EXPORT _Unwind_Reason_Code
_Unwind_ForcedUnwind(_Unwind_Exception *exception_object,
                     _Unwind_Stop_Fn stop, void *stop_parameter) {
  _LIBUNWIND_TRACE_API("_Unwind_ForcedUnwind(ex_obj=%p, stop=%p)",
                       (void *)exception_object, (void *)(uintptr_t)stop);
  unw_context_t uc;
  unw_cursor_t cursor;
  __unw_getcontext(&uc);

#if defined(_XBOX) && _XBOX_VER == 200
  if (!stop || ((uintptr_t)stop & 1)) {
    _LIBUNWIND_TRACE_UNWINDING("invalid Xbox forced-unwind stop function");
    return _URC_FATAL_PHASE2_ERROR;
  }
#endif

  // Mark that this is a forced unwind, so _Unwind_Resume() can do
  // the right thing.
  exception_object->private_1 = (uintptr_t) stop;
  exception_object->private_2 = (uintptr_t) stop_parameter;

  // do it
  return unwind_phase2_forced(&uc, &cursor, exception_object, stop, stop_parameter);
}


/// Called by personality handler during phase 2 to get LSDA for current frame.
_LIBUNWIND_EXPORT uintptr_t
_Unwind_GetLanguageSpecificData(struct _Unwind_Context *context) {
  unw_cursor_t *cursor = (unw_cursor_t *)context;
  unw_proc_info_t frameInfo;
  uintptr_t result = 0;
  if (__unw_get_proc_info(cursor, &frameInfo) == UNW_ESUCCESS)
    result = (uintptr_t)frameInfo.lsda;
  _LIBUNWIND_TRACE_API(
      "_Unwind_GetLanguageSpecificData(context=%p) => 0x%" PRIxPTR,
      (void *)context, result);
#if !defined(_LIBUNWIND_SUPPORT_TBTAB_UNWIND)
  if (result != 0) {
    if (*((uint8_t *)result) != 0xFF)
      _LIBUNWIND_DEBUG_LOG("lsda at 0x%" PRIxPTR " does not start with 0xFF",
                           result);
  }
#endif
  return result;
}


/// Called by personality handler during phase 2 to find the start of the
/// function.
_LIBUNWIND_EXPORT uintptr_t
_Unwind_GetRegionStart(struct _Unwind_Context *context) {
  unw_cursor_t *cursor = (unw_cursor_t *)context;
  unw_proc_info_t frameInfo;
  uintptr_t result = 0;
  if (__unw_get_proc_info(cursor, &frameInfo) == UNW_ESUCCESS)
    result = (uintptr_t)frameInfo.start_ip;
  _LIBUNWIND_TRACE_API("_Unwind_GetRegionStart(context=%p) => 0x%" PRIxPTR,
                       (void *)context, result);
  return result;
}

#endif // !_LIBUNWIND_SUPPORT_SEH_UNWIND

/// Called by personality handler during phase 2 if a foreign exception
// is caught.
_LIBUNWIND_EXPORT void
_Unwind_DeleteException(_Unwind_Exception *exception_object) {
  _LIBUNWIND_TRACE_API("_Unwind_DeleteException(ex_obj=%p)",
                       (void *)exception_object);
  if (exception_object->exception_cleanup != NULL)
    (*exception_object->exception_cleanup)(_URC_FOREIGN_EXCEPTION_CAUGHT,
                                           exception_object);
}

/// Called by personality handler during phase 2 to get register values.
_LIBUNWIND_EXPORT uintptr_t
_Unwind_GetGR(struct _Unwind_Context *context, int index) {
  unw_cursor_t *cursor = (unw_cursor_t *)context;
  unw_word_t result;
  __unw_get_reg(cursor, index, &result);
  _LIBUNWIND_TRACE_API("_Unwind_GetGR(context=%p, reg=%d) => 0x%" PRIx64,
                       (void *)context, index, (uint64_t)result);
  return (uintptr_t)result;
}

/// Called by personality handler during phase 2 to alter register values.
_LIBUNWIND_EXPORT void _Unwind_SetGR(struct _Unwind_Context *context, int index,
                                     uintptr_t value) {
  _LIBUNWIND_TRACE_API("_Unwind_SetGR(context=%p, reg=%d, value=0x%0" PRIxPTR
                       ")",
                       (void *)context, index, value);
  unw_cursor_t *cursor = (unw_cursor_t *)context;
  __unw_set_reg(cursor, index, value);
}

/// Called by personality handler during phase 2 to get instruction pointer.
_LIBUNWIND_EXPORT uintptr_t _Unwind_GetIP(struct _Unwind_Context *context) {
  unw_cursor_t *cursor = (unw_cursor_t *)context;
  unw_word_t result;
  __unw_get_reg(cursor, UNW_REG_IP, &result);

#if defined(_LIBUNWIND_TARGET_AARCH64_AUTHENTICATED_UNWINDING)
  // If we are in an arm64e frame, then the PC should have been signed with the
  // sp
  {
    unw_word_t sp;
    __unw_get_reg(cursor, UNW_REG_SP, &sp);
    result = (unw_word_t)ptrauth_auth_data((void *)result,
                                           ptrauth_key_return_address, sp);
  }
#endif

  _LIBUNWIND_TRACE_API("_Unwind_GetIP(context=%p) => 0x%" PRIxPTR,
                       (void *)context, (uintptr_t)result);
  return (uintptr_t)result;
}

/// Called by personality handler during phase 2 to alter instruction pointer,
/// such as setting where the landing pad is, so _Unwind_Resume() will
/// start executing in the landing pad.
_LIBUNWIND_EXPORT void _Unwind_SetIP(struct _Unwind_Context *context,
                                     uintptr_t value) {
  _LIBUNWIND_TRACE_API("_Unwind_SetIP(context=%p, value=0x%0" PRIxPTR ")",
                       (void *)context, value);
  unw_cursor_t *cursor = (unw_cursor_t *)context;
  __unw_set_reg(cursor, UNW_REG_IP, value);
}

#endif // !defined(_LIBUNWIND_ARM_EHABI) && !defined(__USING_SJLJ_EXCEPTIONS__)

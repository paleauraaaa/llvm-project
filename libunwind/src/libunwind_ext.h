//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//
//  Extensions to libunwind API.
//
//===----------------------------------------------------------------------===//

#ifndef __LIBUNWIND_EXT__
#define __LIBUNWIND_EXT__

#include "config.h"
#include <libunwind.h>
#include <unwind.h>

#define UNW_STEP_SUCCESS 1
#define UNW_STEP_END     0

#define UNW_INFO_FORMAT_XBOX_PDATA_POWERPC (1U << 30)

#ifdef __cplusplus
extern "C" {
#endif

extern int __unw_getcontext(unw_context_t *);
extern int __unw_init_local(unw_cursor_t *, unw_context_t *);
extern int __unw_step(unw_cursor_t *);
extern int __unw_step_stage2(unw_cursor_t *);
extern int __unw_get_reg(unw_cursor_t *, unw_regnum_t, unw_word_t *);
extern int __unw_get_fpreg(unw_cursor_t *, unw_regnum_t, unw_fpreg_t *);
extern int __unw_set_reg(unw_cursor_t *, unw_regnum_t, unw_word_t);
extern int __unw_set_fpreg(unw_cursor_t *, unw_regnum_t, unw_fpreg_t);
_LIBUNWIND_TRACE_NO_INLINE
  extern int __unw_resume_with_frames_walked(unw_cursor_t *, unsigned);
// `__unw_resume` is a legacy function. Use `__unw_resume_with_frames_walked` instead.
_LIBUNWIND_TRACE_NO_INLINE
  extern int __unw_resume(unw_cursor_t *);

#ifdef __arm__
/* Save VFP registers in FSTMX format (instead of FSTMD). */
extern void __unw_save_vfp_as_X(unw_cursor_t *);
#endif

extern const char *__unw_regname(unw_cursor_t *, unw_regnum_t);
extern int __unw_get_proc_info(unw_cursor_t *, unw_proc_info_t *);
#if defined(_XBOX) && _XBOX_VER == 200
// The scope index identifies the active cleanup in the suspended frame.
typedef struct {
  _Unwind_Exception *exception;
  unw_cursor_t *cursor;
  uint32_t scope_index;
  // Output only: a collision has committed and this cleanup is abandoned.
  uint32_t abandoned;
} __unw_xbox_seh_cleanup_state;

typedef struct {
  uintptr_t parent;
  uint32_t parent_frame_size;
  uint32_t flags;
} __unw_xbox_seh_frame;

typedef struct {
  uint32_t begin;
  uint32_t end;
  uint32_t filter_or_finally;
  uint32_t handler;
} __unw_xbox_seh_scope;

#define UNW_XBOX_SEH_SCOPE_INACTIVE 0
#define UNW_XBOX_SEH_SCOPE_ACTIVE 1
#define UNW_XBOX_SEH_SCOPE_END 2
#define UNW_XBOX_SEH_CLEANUP_FUNCLET 1
#define UNW_XBOX_SEH_CATCH_FUNCLET 2
#define UNW_XBOX_SEH_C_HANDLER 4
#define UNW_XBOX_SEH_HANDLER_OWNERSHIP 8
#define UNW_XBOX_ITANIUM_HANDLER 16

#if !defined(_LIBUNWIND_IS_BAREMETAL)
// Returns UNW_ENOINFO for frames without opt-in metadata and UNW_EBADFRAME
// for malformed or unsupported metadata.
extern int __unw_xbox_get_seh_frame(unw_cursor_t *, __unw_xbox_seh_frame *);
// Enumerate bounded C SEH scope records for the cursor's exact instruction
// address or return address minus one. The output changes only for an active
// scope, whose callable target must reside in the same executable image.
extern int __unw_xbox_get_seh_scope(unw_cursor_t *, uintptr_t, uint32_t,
                                    __unw_xbox_seh_scope *);
// Invoke an active C SEH filter or cleanup with the target frame's
// nonvolatile registers and parent entry SP. A filter receives an Xbox
// EXCEPTION_POINTERS-compatible object and returns its disposition through
// the output argument. A cleanup snapshots the exception, cursor, and scope
// index for collision handling. On error no funclet is called and the filter
// output is unchanged. The caller must keep the frame, module, and exception
// alive, and the cursor unchanged, until the call returns or a collision
// commits.
extern int __unw_xbox_call_seh_filter(unw_cursor_t *, uintptr_t, uint32_t,
                                      void *, int *);
extern int __unw_xbox_call_seh_cleanup(unw_cursor_t *, uintptr_t, uint32_t,
                                       _Unwind_Exception *);
// The control PC is the exact instruction address, or a return address minus
// one. Transfers to the selected active C SEH handler; returns only on error
// without modifying the cursor. The caller must keep the frame and module
// alive, and is responsible for cleanup before transfer. A non-null exception
// is released by an ownership-aware handler after it exits.
extern int __unw_xbox_transfer_seh_handler(unw_cursor_t *, uintptr_t, uint32_t,
                                           uint32_t, _Unwind_Exception *);
// Requires a cursor on a cleanup directly invoked by the SEH trampoline.
// Returns an error without modifying the cursor or output if it cannot prove
// that relationship.
extern int __unw_xbox_get_seh_cleanup_parent(unw_cursor_t *, uintptr_t *);
// The exception must stay alive until the cleanup returns or a collision
// commits. The cursor must stay alive and unchanged until the cleanup returns
// or the replacement unwind has resumed from it.
extern int __unw_xbox_get_seh_cleanup_state(
    unw_cursor_t *, uintptr_t *, __unw_xbox_seh_cleanup_state *);
// Identify a collision boundary after a displaced-exception callback. A new
// unwind crossing it must resume from the saved cursor after scope_index,
// rather than searching or unwinding the physically present abandoned frames.
// Returns UNW_ENOINFO when the cursor is not at a collision boundary.
extern int __unw_xbox_get_seh_collision_state(
    unw_cursor_t *, uintptr_t *, __unw_xbox_seh_cleanup_state *);
// Prepare a continuation from state recovered by either helper above. The
// control PC is the suspended frame's exact PC or return address minus one.
// The outputs must not overlap the input state or suspended cursor. On success
// the destination is a copy of that cursor, and next_scope is the index after
// the cleanup. The input must describe the cleanup actually invoked for that
// scope, not just an arbitrary scope in the saved frame.
// On error neither output changes. This does not transfer exception ownership;
// phase 2 must prepare it before committing the handoff separately. After a
// successful handoff, the abandoned unwind must never be resumed.
extern int __unw_xbox_continue_seh_cleanup(
    const __unw_xbox_seh_cleanup_state *, uintptr_t, unw_cursor_t *,
    uint32_t *next_scope);
// Commit only when the replacement unwind is leaving this cleanup. Publish the
// replacement and mark the cleanup abandoned before deleting the displaced
// exception. The cursor may identify the cleanup or a collision boundary.
// Only the thread owning the cleanup stack may call this. After success, the
// suspended unwind must never resume.
// On error, no ownership is transferred and the outputs are unchanged.
extern int __unw_xbox_replace_seh_cleanup_exception(
    unw_cursor_t *, _Unwind_Exception *, _Unwind_Exception *, uintptr_t *,
    __unw_xbox_seh_cleanup_state *);
#endif
#endif
extern int __unw_is_fpreg(unw_cursor_t *, unw_regnum_t);
extern int __unw_is_signal_frame(unw_cursor_t *);
extern int __unw_get_proc_name(unw_cursor_t *, char *, size_t, unw_word_t *);
extern const char *__unw_strerror(int);

#if defined(_AIX)
extern uintptr_t __unw_get_data_rel_base(unw_cursor_t *);
#endif

// SPI
extern void __unw_iterate_dwarf_unwind_cache(void (*func)(
    unw_word_t ip_start, unw_word_t ip_end, unw_word_t fde, unw_word_t mh));

// IPI
extern void __unw_add_dynamic_fde(unw_word_t fde);
extern void __unw_remove_dynamic_fde(unw_word_t fde);

extern void __unw_add_dynamic_eh_frame_section(unw_word_t eh_frame_start);
extern void __unw_remove_dynamic_eh_frame_section(unw_word_t eh_frame_start);

#ifdef __APPLE__

// Holds a description of the object-format-header (if any) and unwind info
// sections for a given address:
//
// * dso_base should point to a header for the JIT'd object containing the
//   given address. The header's type should match the format type that
//   libunwind was compiled for (so a mach_header or mach_header_64 on Darwin).
//   A value of zero indicates that no such header exists.
//
// * dwarf_section and dwarf_section_length hold the address range of a DWARF
//   eh-frame section associated with the given address, if any. If the
//   dwarf_section_length field is zero it indicates that no such section
//   exists (and in this case dwarf_section should also be set to zero).
//
// * compact_unwind_section and compact_unwind_section_length hold the address
//   range of a compact-unwind info section associated with the given address,
//   if any. If the compact_unwind_section_length field is zero it indicates
//   that no such section exists (and in this case compact_unwind_section
//   should also be set to zero).
//
// See the unw_find_dynamic_unwind_sections type below for more details.
struct unw_dynamic_unwind_sections {
  unw_word_t dso_base;
  unw_word_t dwarf_section;
  size_t     dwarf_section_length;
  unw_word_t compact_unwind_section;
  size_t     compact_unwind_section_length;
};

// Typedef for unwind-info lookup callbacks. Functions of this type can be
// registered and deregistered using __unw_add_find_dynamic_unwind_sections
// and __unw_remove_find_dynamic_unwind_sections respectively.
//
// An unwind-info lookup callback should return 1 to indicate that it found
// unwind-info for the given address, or 0 to indicate that it did not find
// unwind-info for the given address. If found, the callback should populate
// some or all of the fields of the info argument (which is guaranteed to be
// non-null with all fields zero-initialized):
typedef int (*unw_find_dynamic_unwind_sections)(
    unw_word_t addr, struct unw_dynamic_unwind_sections *info);

// Register a dynamic unwind-info lookup callback. If libunwind does not find
// unwind info for a given frame in the executable program or normal dynamic
// shared objects then it will call all registered dynamic lookup functions
// in registration order until either one of them returns true, or the end
// of the list is reached. This lookup will happen before libunwind searches
// any eh-frames registered via __register_frame or
// __unw_add_dynamic_eh_frame_section.
//
// Returns UNW_ESUCCESS for successful registrations. If the given callback
// has already been registered then UNW_EINVAL will be returned. If all
// available callback entries are in use then UNW_ENOMEM will be returned.
extern int __unw_add_find_dynamic_unwind_sections(
    unw_find_dynamic_unwind_sections find_dynamic_unwind_sections);

// Deregister a dynamic unwind-info lookup callback.
//
// Returns UNW_ESUCCESS for successful deregistrations. If the given callback
// is not present then UNW_EINVAL will be returned.
extern int __unw_remove_find_dynamic_unwind_sections(
    unw_find_dynamic_unwind_sections find_dynamic_unwind_sections);

#endif

#if defined(_LIBUNWIND_ARM_EHABI)
extern const uint32_t* decode_eht_entry(const uint32_t*, size_t*, size_t*);
extern _Unwind_Reason_Code _Unwind_VRS_Interpret(_Unwind_Context *context,
                                                 const uint32_t *data,
                                                 size_t offset, size_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif // __LIBUNWIND_EXT__

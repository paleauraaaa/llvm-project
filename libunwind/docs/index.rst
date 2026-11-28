.. _index:

=======================
libunwind LLVM Unwinder
=======================

Overview
========

libunwind is an implementation of the interface defined by the HP libunwind
project. It was contributed by Apple as a way to enable clang++ to port to
platforms that do not have a system unwinder. It is intended to be a small and
fast implementation of the ABI, leaving off some features of HP's libunwind
that never materialized (e.g. remote unwinding).

The unwinder has two levels of API. The high level APIs are the `_Unwind_*`
functions which implement functionality required by `__cxa_*` exception
functions. The low level APIs are the `unw_*` functions which are an interface
defined by the old HP libunwind project.

Getting Started with libunwind
------------------------------

.. toctree::
   :maxdepth: 2

   BuildingLibunwind

Current Status
--------------

libunwind is a production-quality unwinder, with platform support for DWARF
unwind info, SjLj, and ARM EHABI.

The low level libunwind API was designed to work either in-process (aka local)
or to operate on another process (aka remote), but only the local path has been
implemented. Remote unwinding remains as future work.

Platform and Compiler Support
-----------------------------

libunwind is known to work on the following platforms:

============ ======================== ============ ========================
OS           Arch                     Compilers    Unwind Info
============ ======================== ============ ========================
Any          i386, x86_64, ARM        Clang        SjLj
Bare Metal   ARM                      Clang, GCC   EHABI
FreeBSD      i386, x86_64, ARM64      Clang        DWARF CFI
iOS          ARM                      Clang        SjLj
Linux        ARM                      Clang, GCC   EHABI
Linux        i386, x86_64, ARM64      Clang, GCC   DWARF CFI
macOS        i386, x86_64             Clang, GCC   DWARF CFI
NetBSD       x86_64                   Clang, GCC   DWARF CFI
Windows      i386, x86_64, ARM, ARM64 Clang        DWARF CFI
============ ======================== ============ ========================

The following minimum compiler versions are strongly recommended.

* Clang 3.5 and above
* GCC 4.7 and above.

Anything older *may* work.

Notes and Known Issues
----------------------

* Xbox 360 register contexts support 64-bit general registers and VMX128
  registers with 32-bit addresses. Capture also records FPSCR and VSCR at
  the capture call; MSR is unavailable in problem state. These values do not
  reconstruct volatile state at an earlier call or fault, and the resume
  sequence does not restore FPSCR or VSCR. Module discovery uses the expected
  ``XexPcToFileHeader`` loader entry and mapped PE headers to locate native
  ``.pdata``. The kernel contract, loader entry layout, header availability,
  and module lifetime have not been verified on hardware; the module must
  remain loaded during unwinding. A local Xenia build implements loader lookup
  for guest user modules; Xenia does not provide guest kernel module images.
  The cursor parses packed ``.pdata`` records and exposes native handler and
  handler-data addresses through ``unw_get_proc_info``; handlers may use the
  native SEH ABI or the explicitly marked Itanium personality ABI. Frame
  walking interprets supported straight-line prologues, bounded save helpers
  and recognized epilogues to restore nonvolatile registers, including VMX128. The decoder
  rejects detected unsupported or ambiguous sequences; arbitrary hand-written
  control flow and stack-probe helpers are not fully supported. For
  libunwind-raised exceptions, the Xbox C SEH dispatcher searches native
  scopes, runs abnormal cleanups and transfers to an ownership-aware handler.
  It does not dispatch hardware or kernel exceptions. Experimental Itanium C++
  support requires both ``-mxbox360-libunwind`` and ``-fc++-abi=itanium``.
  Concurrent typed throws, cleanups, and rethrows have passed Xenia tests using
  that loader lookup, but hardware behavior and mixed C SEH frames remain
  unverified. An experimental static libc++ archive builds and links with
  Xbox picolibc. Rust integration is not implemented.
  Filters receive a noncontinuable synthetic
  exception with code ``0x20474343`` and the ``_Unwind_Exception`` pointer in
  its first information parameter. Its address and context IAR identify the
  raising call site, but context flags are zero and the remaining registers
  are unavailable; continuing execution is rejected. Xbox SEH filters take
  exception pointers in r3 and the parent entry stack pointer in r12. Cleanup
  funclets also require the parent entry stack pointer in r12; the generated
  funclet calls the outlined finally helper with the abnormal-termination flag
  in r3. An internal returning-call trampoline switches the nonvolatile
  registers to a target-frame snapshot and restores the caller's registers.
  It records the original parent for cleanup calls; an internal cursor helper
  recovers it only after identifying the matching trampoline invocation.
  The trampoline can also preserve the active exception, suspended cursor and
  cleanup scope index for the dispatcher. The exception must remain alive
  until the cleanup returns or a collision commits; the cursor must remain
  alive and unchanged until the cleanup returns or the replacement unwind has
  resumed from it.
  An internal collision handoff marks the cleanup abandoned and publishes the
  replacement exception before deleting the displaced exception through a
  marked trampoline. The dispatcher recognizes that boundary, skips abandoned
  frames and resumes at the next scope using the saved cursor. The same
  mechanism is used when a forced unwind crosses a cleanup boundary.
  Xbox code generation emits separate scope tables for cleanup funclets with
  nested SEH actions.
  With ``-mxbox360-libunwind``, the compiler also emits ``.xseh`` records for
  compiler-generated unwind ranges except the synthetic catch-all filter.
  Records need not be sorted and may be absent in mixed-mode images.
  Each record consists of five big-endian 32-bit words: version
  magic ``0x58534501``, range start, parent function entry, parent fixed frame
  size, and flags. Flag 1 denotes a cleanup funclet, flag 2 a catch funclet,
  flag 4 a native C SEH handler, flag 8 an ownership-aware C handler, and
  flag 16 an Itanium personality. Absence of these flags does not identify a
  handler's ABI. For a funclet after its
  prologue, its parent entry stack
  pointer in the established funclet body is the parent frame pointer in r31
  plus the recorded frame size; this is not valid throughout the prologue or
  epilogue.
  The section is auxiliary; existing kernel ``.pdata`` is unchanged. The
  internal cursor can validate and read these records, including the parent
  record of a funclet. It can enumerate bounded C SEH scopes and validate
  active callable targets before invoking a filter or cleanup with a verified
  parent frame. Cleanup calls also save the exception and scope state for a
  the dispatcher. An internal handler-transfer primitive checks C SEH
  metadata, a bounded active scope and an established frame before resuming
  at the handler with the exception code in r3 and an optional exception
  object in r4. A non-null object requires flag 8; the handler emits cleanup
  to release it on exit. Cleanup-local transfers
  require a matching trampoline invocation; catch funclets are not supported.
  The caller must keep the frame and module alive. The unwind resume state
  reserves a tagged scope index for Xbox SEH handler selection.
  Nested and collided unwinds can use substantial stack space.
  ``_Unwind_Find_FDE``
  does not return native ``.pdata`` as a DWARF FDE. This is not a complete port.
  Builds that emit ``_RtlCheckStack12`` need a stack-probe helper; the Xbox
  picolibc runtime supplies one. Its guard-page behavior has not been tested on
  hardware.
  The context resume sequence uses the volatile CTR register for the saved PC.
  The low-level ``unw_word_t`` is 64 bits, while the ``_Unwind_GetGR`` and
  ``_Unwind_SetGR`` interfaces retain their pointer-sized ABI.


Getting Involved
================

First please review our `Developer's Policy <https://llvm.org/docs/DeveloperPolicy.html>`__
and `Getting started with LLVM <https://llvm.org/docs/GettingStarted.html>`__.

**Bug Reports**

If you think you've found a bug in libunwind, please report it using
the `LLVM bug tracker`_. If you're not sure, you
can ask for support on the `Runtimes forum`_ or on Discord.
Please use the tag "libunwind" for new threads.

**Patches**

If you want to contribute a patch to libunwind, please start by reading the LLVM
`documentation about contributing <https://www.llvm.org/docs/Contributing.html>`__.

**Discussion and Questions**

Send discussions and questions to the `Runtimes forum`_. Please add the tag "libunwind" to your post.


Quick Links
===========
* `LLVM Homepage <https://llvm.org/>`_
* `LLVM Bug Tracker <https://github.com/llvm/llvm-project/labels/libunwind/>`_
* `Clang Discourse Forums <https://discourse.llvm.org/c/clang/6>`_
* `cfe-commits Mailing List <http://lists.llvm.org/mailman/listinfo/cfe-commits>`_
* `Runtimes Forum <https://discourse.llvm.org/tags/c/runtimes>`_
* `Browse libunwind Sources <https://github.com/llvm/llvm-project/blob/main/libunwind/>`_

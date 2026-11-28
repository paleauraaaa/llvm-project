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
  registers with 32-bit addresses. Module discovery uses the expected
  ``XexPcToFileHeader`` loader entry and mapped PE headers to locate native
  ``.pdata``. The kernel contract, loader entry layout, header availability,
  and module lifetime have not been verified on hardware; the module must
  remain loaded during unwinding. Xenia does not implement this loader API.
  The cursor parses packed ``.pdata`` records and exposes native handler and
  handler-data addresses through ``unw_get_proc_info``; these handlers use the
  native SEH ABI, not the Itanium personality ABI. Frame walking interprets
  supported straight-line prologues, bounded save helpers and recognized
  epilogues to restore nonvolatile registers, including VMX128. The decoder
  rejects detected unsupported or ambiguous sequences; arbitrary hand-written
  control flow and stack-probe helpers are not fully supported. Native
  exception dispatch is not implemented, and Itanium unwinding fails rather
  than calling a native SEH handler with the wrong ABI. ``_Unwind_Find_FDE``
  does not return native ``.pdata`` as a DWARF FDE. This is not a complete port.
  Builds that emit ``_RtlCheckStack12`` also need a real stack-probe helper;
  discovery tests may use a fail-fast stub only while DWARF parsing is unused.
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

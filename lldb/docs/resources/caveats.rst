Caveats
=======

Xbox 360 disassembly
--------------------

For PowerPC Xbox 360 targets, LLDB defaults to the ``xenon`` CPU model. This enables
VMX128, standard VMX and Xenon's 64-bit scalar instructions while retaining
four-byte big-endian instruction words and 32-bit addresses. VMX128 decoding
takes precedence over overlapping standard PowerPC encodings. Explicit CPU or
feature overrides, including ``target.disassembly-cpu`` and
``target.disassembly-features``, replace the automatic CPU selection.

Xbox 360 PDB debugging
----------------------

Both the native PDB reader and the Windows DIA-backed reader support Xbox 360
source-line breakpoints and C++ record reconstruction. Namespace-qualified
types and globals, nested records, nonvirtual bases, instantiated template
record layouts, arrays, pointers, enums, static members and const/volatile
method signatures can be inspected. CodeView bitfield positions are converted
to the big-endian layout used by Clang and LLDB, including for expression reads
and writes. Static data members with linker symbols can also be read and written
by interpreted expressions without executing code on the target.

The executable and its matching PDB must be available to LLDB. If the remote
does not report module mappings, load the executable's sections explicitly
after connecting, for example ``target modules load -f game.exe --slide 0``
when the image is loaded at its preferred base. Source paths can be remapped
using ``target.source-map``.

This coverage is tested with Clang/LLD-generated Xbox 360 images and a mock remote,
not with a live console or every historical Xbox 360 PDB producer. Reconstruction cannot recover
information absent from the PDB. Standard CodeView has no dedicated C++
scoped-enum marker; its ``Scoped`` flag describes lexical scoping, not
``enum class`` identity.

Clang defaults to legacy-compatible CodeView on Xbox 360, omitting modern
type-definition source records, build information and inline-site hierarchies.
Use ``-g -gcodeview -gcodeview-xbox360-modern`` with a modern linker such as LLD
to emit these records instead. The resulting debug information is not compatible
with older Xbox 360 readers. ``-gno-codeview-xbox360-modern`` restores the default.

Xbox 360 return values
----------------------

The Xbox 360 ABI plugin interprets integer, enumeration, pointer and reference
returns from ``r3``, floating-point returns from ``f1``, and direct 128-bit vector
returns from ``v1``. It also supports split 128-bit integers and complex values.
Single-precision results are converted from the double-precision FPR contents;
32-bit pointers ignore the upper half of the 64-bit GPR.

Ordinary aggregates of 1, 2, 4 or 8 bytes are interpreted as raw big-endian
memory images in ``r3``. Other aggregates are read from the returned 32-bit
destination pointer in ``r3``. Indirect results are copied immediately so that
later memory changes do not alter the recorded return value.

Step-out supplies the returning function's declaration context. Function-call
plans also supply it when available. This distinguishes C++ instance methods,
which return aggregates and 128-bit vectors indirectly, from free and static
functions. Microsoft C++ record restrictions and Xbox 360 ``intrin_type`` and
``passinreg`` overrides are honored when represented in the type information.
Missing or unusable supplied context prevents ambiguous aggregate/vector
decoding. Calls to the type-only ABI interface without a context use
free-function conventions.

``thread return`` can write register-backed Xbox 360 results, including split
integers, complex values, vectors and direct small aggregates. Signed scalar
integers are sign-extended to the 64-bit GPR; single-precision results are
promoted to the FPR's double-precision representation. The returning function's
declaration context is used to distinguish instance and static methods.
Supply a value with the intended return type; this interface does not perform
implicit conversion to the function's declared return type.

Forced indirect returns are rejected: at an arbitrary stop, the original
hidden destination pointer cannot safely be inferred from ``r3``. Reading a
completed indirect result from ``r3`` remains supported.

Xbox 360 unwinding
------------------

PPCBE PE images use a dedicated reader for the big-endian, eight-byte packed
``.pdata`` records. Function boundaries come from these records even without
symbols, including separate records for exception funclets. PE headers remain
little-endian.

The reader tracks straight-line prologues, bounded register-save helpers,
fixed and frame-pointer-based stacks, and conventional restore sequences ending
in returns or tail branches. It preserves eight-byte GPR/FPR spills and
sixteen-byte VMX128 spills, while reading saved LR and CR words as four bytes.
CR2 through CR5 are recovered individually from a saved CR. Code and stack
addresses use the low 32 bits of the register value.

Unwind plans are not advertised as valid at every instruction: arbitrary
hand-written control flow, nonstandard epilogues and loop-shaped stack-probe
helpers are not fully modeled. Unsupported prologues or unreadable save helpers
are rejected and reported through the unwind log. Without usable unwind
metadata, LLDB does not guess a SysV stack layout for Xbox 360 code.

Xbox 360 exception events
--------------------------

GDB-remote ``reason:exception`` stops are reported as exception events, including
when received through ``jstopinfo``. ``thread exception`` displays the event's
description without requiring a C++ exception object. ``thread info -s`` and
``SBThread.GetStopReasonExtendedInfoAsJSON`` expose the transport signal,
thread ID, available guest PC and description for Xbox 360 exception events.
These fields describe the stop; the signal is not a Windows exception code.
The exception stop value remains unknown when the remote has not supplied a
guest code. Explicit exception stops are not reclassified as debugger
breakpoints merely because their transport signal is ``SIGTRAP``.

``thread exception-record <address>`` inspects a Windows-style guest exception
record at an explicitly supplied numeric address without running guest code.
Xbox 360 records have five four-byte big-endian header fields followed by at most
15 four-byte parameters. The command displays a decoded description and JSON
containing the raw code, flags, record and instruction addresses, next-record
pointer and parameters. Access violations distinguish the instruction address
from the accessed address and decode read, write and execute operations.
Unknown codes, flags and parameters are preserved. Record chains are not
followed, so cyclic or stale next-record pointers cannot cause unbounded reads.
This inspection does not attach the record to the current stop, infer a record
from argument registers, or replace live registers with a saved context.

Only existing remote packets are used. They do not provide a structured Xbox 360
exception code, flags, parameters, first-/second-chance status, or a saved
exception-context address. Descriptions are opaque text, not a metadata
protocol. Consequently LLDB does not guess those fields or implement per-code
or chance-based filtering from them. Explicit exception events stop normally;
signal-only stops retain ordinary signal policy. Existing thread-plan and
expression-error handling applies, and DAP reports the exception reason and
description through its existing stopped event and ``exceptionInfo`` request.

Handled/not-handled continuation, noncontinuable-disposition validation,
automatic saved-context recovery, and structured exception filters require an
agreed remote representation and are not implemented. Ordinary ``continue``
uses the existing remote resume operation; it does not promise Windows
exception-disposition semantics. Microsoft C++ runtime integration, throw/catch
breakpoints and exception-object decoding remain separate work.

Xbox 360 thread-local storage
-----------------------------

Static ``__declspec(thread)`` variables in the executable are resolved using the
four-byte big-endian block pointer at ``0(r13)`` plus the variable's offset in
``.tls``. The upper half of the 64-bit ``r13`` register is ignored. Both PDB
readers produce thread-relative locations for Xbox 360 TLS symbols rather than
ordinary relocated global addresses. DWARF ``DW_OP_form_tls_address`` and
``DW_OP_GNU_push_tls_address`` use the same section-relative offsets.

Resolution uses the selected thread's live register context and does not
require dynamic-loader support, a Windows TLS index, or a TOC. Missing thread
state, unreadable block pointers, invalid offsets and address overflow are
reported as unavailable TLS. Independently loaded modules' TLS and runtime
``TlsAlloc`` slots are not interpreted as executable static TLS.

``thread tls`` locates the selected thread's runtime TLS array through
``KPCR.PrcbData.CurrentThread`` at ``r13 + 0x100`` and the four-byte pointer at
``KTHREAD + 0x68``. These are separate from the static block at ``0(r13)``.
``thread tls <index>`` reads one of the 256 four-byte, big-endian runtime slots
without executing guest code. The command checks pointers, address overflow and
slot bounds, but does not check the kernel's allocation bitmap: a zero slot does
not imply that it is unallocated.

``thread tls bind $tls`` creates a new persistent ``unsigned int *`` for
expression access, for example ``expression -- $tls[slot]`` or
``expression -- $tls[slot] = value``. The binding captures the current thread's
array address; it does not follow thread selection or thread termination.
Use a new name to bind another thread or stop. Expressions use ordinary pointer
semantics and do not enforce slot bounds, so use only indices 0 through 255 and
only slots owned by the application. Neither command allocates or frees slots,
and no ``TlsGetValue``/``TlsSetValue`` calls are intercepted.

Xbox 360 CodeView and PDB
-------------------------

Both PDB readers translate Xbox 360 CodeView register locations through the target
register descriptions, independently of the remote stub's register ordering.
Supported registers are ``r0`` through ``r31``, ``f0`` through ``f31``,
``v0`` through ``v127``, ``pc``, ``lr``, ``ctr``, ``xer``, the full ``cr`` and
its eight fields. Register-relative addresses, including stack and frame-base
locations, are truncated to 32 bits; enregistered values retain their full
register width. NativePDB uses the CodeView frame flags to distinguish stack,
frame and base pointers.

Standalone PDBs with the PPCBE machine type are recognized as Xbox 360 targets.
PDB record structures remain little-endian even though target memory and
location expressions are big-endian. NativePDB also accepts older CodeView
output without an optional IPI (ID) stream.

Unsupported registers (including ``fpscr`` and ``msr``), vector lane IDs and
composite locations requiring mixed register-numbering schemes are rejected
and reported through the symbols log. This does not add support for every
historical Xbox 360 CodeView record format or for x86 FPO programs on Xbox 360.

Xbox 360 debugger-injected calls
--------------------------------

Debugger-injected scalar calls reserve the interrupted function's 1344-byte red
zone before allocating a 16-byte-aligned frame. The frame contains a four-byte,
big-endian backchain to the interrupted stack pointer, a 16-byte linkage area,
and at least eight eight-byte argument home slots. Arguments after ``r3`` through
``r10`` occupy subsequent doubleword slots. The complete frame, including unused
home slots and alignment padding, is initialized before argument or control
registers are changed. The synthetic return address is supplied in ``lr``;
no SysV linkage-area LR or TOC save is synthesized.
Xbox 360 function pointers are direct code addresses, not ELF function descriptors.
``r2`` is preserved as reserved state; call setup neither reads it as a TOC
pointer nor initializes it for the callee. The SysV PPC and PPC64 ABI plugins
are excluded for both supported Xbox 360 target triples.

Stack addresses ignore the upper half of the 64-bit ``r1`` register. Unreadable
or misaligned original stack pointers, stack underflow, frames above the active
stack, and incomplete frame writes are rejected. This interface handles integer
scalar arguments; it does not perform typed floating-point, vector, aggregate,
varargs or C++ argument lowering.

.. _python_caveat:

Python
------

LLDB has a powerful scripting interface which is accessible through Python.
Python is available either from within LLDB through a (interactive) script
interpreter, or as a Python module which you can import from the Python
interpreter.

To make this possible, LLDB links against the Python shared library. Linking
against Python comes with some constraints to be aware of.

1.  It is not possible to build and link LLDB against a Python 3 library and
    use it from Python 2 and vice versa.

2.  It is not possible to build and link LLDB against one distribution on
    Python and use it through an interpreter coming from another distribution.
    For example, on macOS, if you build and link against Python from
    python.org, you cannot import the lldb module from the Python interpreter
    installed with Homebrew.

3.  To use third party Python packages from inside LLDB, you need to install
    them using a utility (such as ``pip``) from the same Python distribution as
    the one used to build and link LLDB.

The previous considerations are especially important during development, but
apply to binary distributions of LLDB as well.

LLDB in Xcode on macOS
``````````````````````

Users of lldb in Xcode on macOS commonly run into these issues when they
install Python, often unknowingly as a dependency pulled in by Homebrew or
other package managers. The problem is the symlinks that get created in
``/usr/local/bin``, which comes before ``/usr/bin`` in your path. You can use
``which python3`` to check to what it resolves.

To be sure you use the Python that matches with the lldb in Xcode use ``xcrun``
or use the absolute path to the shims in ``/usr/bin``.

::

   $ xcrun python3
   $ /usr/bin/python3

Similarly, to install packages and be able to use them from within lldb, you'll
need to install them with the matching ``pip3``.

::

   $ xcrun pip3
   $ /usr/bin/pip3

The same is true for Python 2. Although Python 2 comes with the operating
system rather than Xcode, you can still use ``xcrun`` to launch the system
variant.

::

   $ xcrun python
   $ /usr/bin/python

Keep in mind that Python 2 is deprecated and no longer maintained. Future
versions of macOS will not include Python 2.7.

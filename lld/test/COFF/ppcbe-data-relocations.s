# RUN: llvm-mc -filetype=obj -triple=ppc64-ibm-xbox360 %s -o %t.obj
# RUN: llvm-objdump -s --section=.data %t.obj | FileCheck %s --check-prefix=OBJECT
# RUN: lld-link /machine:ppcbe /subsystem:xbox /entry:main /nodefaultlib /fixed /base:0x82000000 /out:%t.exe %t.obj
# RUN: llvm-objdump -s --section=.data %t.exe | FileCheck %s --check-prefix=LINKED

# These non-instruction COFF relocation fields and their addends are
# little-endian, except for ADDR32, which is target-endian.
.text
.globl main
main:
  blr

.data
  .short target+0x11f2
  .long target@imgrel+0x112233e4
  .secrel32 target+0x1234
  .secidx target
  .long target+0x1234
  .space 0x20
target:
  .long 0
section_with_addend:
  .short 0x0201
  .reloc section_with_addend, IMAGE_REL_PPC_SECTION, target
  .long target@SECREL32+0x102
  .short 0x1234

# OBJECT: 0000 f211e433 22113412 00000000 00001234
# OBJECT: 0030 00000000 02010201 00001234
# LINKED: 82002000 22321454 22116412 00000200 82003264
# LINKED: 82002030 00000000 04013201 00001234

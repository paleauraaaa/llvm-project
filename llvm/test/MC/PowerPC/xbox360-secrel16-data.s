# RUN: split-file %s %t.dir
# RUN: not llvm-mc -triple=powerpc64-ibm-xbox360 -filetype=obj %t.dir/invalid.s -o %t.invalid.obj 2>&1 | FileCheck %s --check-prefix=ERR
# RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -filetype=obj %t.dir/valid.s -o %t.valid.obj
# RUN: llvm-readobj --relocations --sections --section-data %t.valid.obj | FileCheck %s --check-prefix=VALID

# ERR-COUNT-4: error: 16-bit section-relative data relocation is not supported; use a 32-bit section-relative field

# VALID: Name: .text
# VALID: SectionData (
# VALID-NEXT: 0000: 39400000 4E800020
# VALID: Name: .data
# VALID: SectionData (
# VALID-NEXT: 0000: 04000000 00000000 0000
# VALID: Section (1) .text {
# VALID: IMAGE_REL_PPC_SECREL16 target
# VALID: Section (2) .data {
# VALID: IMAGE_REL_PPC_SECREL target
# VALID: IMAGE_REL_PPC_ADDR16 target
# VALID: IMAGE_REL_PPC_SECREL16 target

#--- invalid.s
.data
  .short target@SECREL32
  .2byte target@SECREL32+3
local:
  .short local@SECREL32
.section .debug$S,"dr"
  .short target@SECREL32

#--- valid.s
.text
  li 10, target@SECREL32
  blr
.data
  .long target@SECREL32+4
  .short target
word:
  .long 0
  .reloc word, IMAGE_REL_PPC_SECREL16, target

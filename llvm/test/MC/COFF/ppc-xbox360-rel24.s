# RUN: llvm-mc -filetype=obj -triple=ppc64-ibm-xbox360 %s -o %t.obj
# RUN: llvm-objdump -dr %t.obj | FileCheck %s

# XDK LINK expects REL24 addends to include the negative input-section offset.
# Preserve explicit addends, LK/AA bits, and temporary-symbol section offsets.
.text
.globl caller
caller:
  bl external
  bl external
  bl external+8
  b external-4
  .p2align 4
  bl defined
  bl 1f+4
  bla external+8
  b .Llocal
  nop
.Llocal:
  blr

.section .text$callee,"xr"
  nop
1:
  nop
.globl defined
defined:
  blr

# CHECK-LABEL: <caller>:
# CHECK:      0: 48 00 00 01
# CHECK-NEXT: IMAGE_REL_PPC_REL24 external
# CHECK-NEXT: 4: 4b ff ff fd
# CHECK-NEXT: IMAGE_REL_PPC_REL24 external
# CHECK-NEXT: 8: 48 00 00 01
# CHECK-NEXT: IMAGE_REL_PPC_REL24 external
# CHECK-NEXT: c: 4b ff ff f0
# CHECK-NEXT: IMAGE_REL_PPC_REL24 external
# CHECK-NEXT: 10: 4b ff ff f1
# CHECK-NEXT: IMAGE_REL_PPC_REL24 defined
# CHECK-NEXT: 14: 4b ff ff f5
# CHECK-NEXT: IMAGE_REL_PPC_REL24 .text$callee
# CHECK-NEXT: 18: 48 00 00 0b
# CHECK-NEXT: IMAGE_REL_PPC_ADDR24 external
# CHECK-NEXT: 1c: 48 00 00 08
# CHECK-NEXT: 20: 60 00 00 00
# CHECK:      24: 4e 80 00 20
# CHECK-NOT: IMAGE_REL_PPC

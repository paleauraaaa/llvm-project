# RUN: llvm-mc -filetype=obj -triple=ppc64-ibm-xbox360 %s -o %t.obj
# RUN: llvm-objdump -dr %t.obj | FileCheck %s

# XDK LINK expects REL14 addends to include the negative input-section offset.
# Preserve explicit addends, BO/BI/LK/AA bits, and temporary-symbol offsets.
.text
.globl caller
caller:
  bne external
  bne external
  bne external+8
  beql external-4
  .p2align 4
  bne defined
  bne 1f+4
  bnea external+8
  bne .Llocal
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
# CHECK:      0: 40 82 00 00
# CHECK-NEXT: IMAGE_REL_PPC_REL14 external
# CHECK-NEXT: 4: 40 82 ff fc
# CHECK-NEXT: IMAGE_REL_PPC_REL14 external
# CHECK-NEXT: 8: 40 82 00 00
# CHECK-NEXT: IMAGE_REL_PPC_REL14 external
# CHECK-NEXT: c: 41 82 ff f1
# CHECK-NEXT: IMAGE_REL_PPC_REL14 external
# CHECK-NEXT: 10: 40 82 ff f0
# CHECK-NEXT: IMAGE_REL_PPC_REL14 defined
# CHECK-NEXT: 14: 40 82 ff f4
# CHECK-NEXT: IMAGE_REL_PPC_REL14 .text$callee
# CHECK-NEXT: 18: 40 82 00 0a
# CHECK-NEXT: IMAGE_REL_PPC_ADDR14 external
# CHECK-NEXT: 1c: 40 82 00 08
# CHECK-NEXT: 20: 60 00 00 00
# CHECK:      24: 4e 80 00 20
# CHECK-NOT: IMAGE_REL_PPC

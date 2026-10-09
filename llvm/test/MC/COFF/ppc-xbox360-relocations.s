# RUN: llvm-mc -filetype=obj -triple=ppc64-ibm-xbox360 %s -o %t.obj
# RUN: llvm-readobj --file-headers --relocations --expand-relocs %t.obj | FileCheck %s --check-prefix=RELOCS
# RUN: llvm-objdump -dr %t.obj | FileCheck %s --check-prefix=ADDENDS

# Each half has its own PAIR containing the complementary addend half.
# Check instruction bytes too: fixups must not overwrite opcode/register bits.
.text
lis 3, target+0x1234@ha
addi 3, 3, target+0x1234@l
lis 4, target+0x8000@ha
addi 4, 4, target+0x8000@l
lis 5, target-1@ha
addi 5, 5, target-1@l
lis 7, target+0x18000@ha
addi 7, 7, target+0x18000@l
lis 6, 1f+0x10@ha
addi 6, 6, 1f+0x10@l
# REFLO does not depend on a preceding REFHI, even for the same symbol.
addi 8, 8, target-0x18000@l
lis 8, target-0x18000@ha
addi 9, 9, target@l

.globl target
target:
1:

# RELOCS: Machine: 0x1F2
# RELOCS: Type: IMAGE_REL_PPC_REFHI (16)
# RELOCS: Type: IMAGE_REL_PPC_PAIR (18)
# RELOCS: Type: IMAGE_REL_PPC_REFLO (17)
# RELOCS: Type: IMAGE_REL_PPC_PAIR (18)
# ADDENDS: 3c 60 00 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFHI target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x1234
# ADDENDS-NEXT: {{.*}}38 63 12 34
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0000
# ADDENDS-NEXT: {{.*}}3c 80 00 01
# ADDENDS-NEXT: IMAGE_REL_PPC_REFHI target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x8000
# ADDENDS-NEXT: {{.*}}38 84 80 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0001
# ADDENDS-NEXT: {{.*}}3c a0 00 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFHI target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0xffff
# ADDENDS-NEXT: {{.*}}38 a5 ff ff
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0000
# ADDENDS-NEXT: {{.*}}3c e0 00 02
# ADDENDS-NEXT: IMAGE_REL_PPC_REFHI target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x8000
# ADDENDS-NEXT: {{.*}}38 e7 80 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0002
# ADDENDS-NEXT: {{.*}}3c c0 00 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFHI .text
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0044
# ADDENDS-NEXT: {{.*}}38 c6 00 44
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO .text
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0000
# ADDENDS-NEXT: {{.*}}39 08 80 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0xffff
# ADDENDS-NEXT: {{.*}}3d 00 ff ff
# ADDENDS-NEXT: IMAGE_REL_PPC_REFHI target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x8000
# ADDENDS-NEXT: {{.*}}39 29 00 00
# ADDENDS-NEXT: IMAGE_REL_PPC_REFLO target
# ADDENDS-NEXT: IMAGE_REL_PPC_PAIR 0x0000

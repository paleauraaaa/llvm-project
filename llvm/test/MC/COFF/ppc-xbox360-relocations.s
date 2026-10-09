# RUN: llvm-mc -filetype=obj -triple=ppc64-ibm-xbox360 %s -o %t.obj
# RUN: llvm-readobj --file-headers --relocations --expand-relocs %t.obj | FileCheck %s --check-prefix=RELOCS
# RUN: od -An -t x1 -j 186 -N 4 %t.obj | FileCheck %s --check-prefix=ADDEND-1234
# RUN: od -An -t x1 -j 216 -N 4 %t.obj | FileCheck %s --check-prefix=ADDEND-8000
# RUN: od -An -t x1 -j 246 -N 4 %t.obj | FileCheck %s --check-prefix=ADDEND-NEGATIVE
# RUN: od -An -t x1 -j 276 -N 4 %t.obj | FileCheck %s --check-prefix=ADDEND-SECTION

# PAIR stores a signed 16-bit displacement in the symbol-index field. These
# offsets select the four PAIR records in the emitted COFF object.
.text
lis 3, target+0x1234@ha
addi 3, 3, target+0x1234@l
lis 4, target+0x8000@ha
addi 4, 4, target+0x8000@l
lis 5, target-1@ha
addi 5, 5, target-1@l
lis 6, 1f+0x10@ha
addi 6, 6, 1f+0x10@l

.globl target
target:
1:

# RELOCS: Machine: 0x1F2
# RELOCS: Type: IMAGE_REL_PPC_REFHI (16)
# RELOCS: Type: IMAGE_REL_PPC_PAIR (18)
# RELOCS: Type: IMAGE_REL_PPC_REFLO (17)
# ADDEND-1234: 34 12 00 00
# ADDEND-8000: 00 80 00 00
# ADDEND-NEGATIVE: ff ff 00 00
# ADDEND-SECTION: 30 00 00 00

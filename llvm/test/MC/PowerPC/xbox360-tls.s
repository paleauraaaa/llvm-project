# RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -filetype=obj %s -o %t
# RUN: llvm-readobj --relocations --section-data --sections %t | FileCheck %s

        .text
        lwz 11, 0(13)
        li 10, tls_value@SECREL32
        lwzx 3, 10, 11
        blr
        .section .tls$,"dw"
        .p2align 2
tls_value:
        .long 7

# CHECK: Name: .text
# CHECK: SectionData (
# CHECK-NEXT: 0000: 816D0000 39400000 7C6A582E 4E800020
# CHECK: Name: .tls$
# CHECK: SectionData (
# CHECK-NEXT: 0000: 00000007
# CHECK: Section (1) .text {
# CHECK-NEXT: 0x4 IMAGE_REL_PPC_SECREL16 tls_value

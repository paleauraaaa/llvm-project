# RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -filetype=obj %s -o %t.obj
# RUN: llvm-readobj --hex-dump=.debug$H --hex-dump=.data %t.obj | FileCheck %s

.section .debug$H,"dr"
.short value16
.long value32
.quad value64

.data
.short value16
.long value32
.quad value64

.set value16, 0x1234
.set value32, 0x12345678
.set value64, 0x1122334455667788

# CHECK: Hex dump of section '.data':
# CHECK: 12341234 56781122 33445566 7788
# CHECK: Hex dump of section '.debug$H':
# CHECK: 34127856 34128877 66554433 2211

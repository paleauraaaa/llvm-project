# RUN: llvm-mc -triple=powerpc64-ibm-xbox360 -mcpu=xenon -show-encoding %s | FileCheck %s

# CHECK: ldbrx 3, 3, 4 # encoding: [0x7c,0x63,0x24,0x28]
ldbrx 3, 3, 4
# CHECK: stdbrx 3, 4, 5 # encoding: [0x7c,0x64,0x2d,0x28]
stdbrx 3, 4, 5

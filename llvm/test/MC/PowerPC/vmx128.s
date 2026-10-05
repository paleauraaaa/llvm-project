# RUN: llvm-mc -triple=powerpc64-unknown-unknown -mcpu=g5 -mattr=+vmx128 --show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=powerpc64-unknown-unknown -mcpu=g5 -mattr=+vmx128 -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --mcpu=g5 --mattr=+vmx128 %t.o | FileCheck %s --check-prefix=DIS

# Memory operations.
# ENC: lvsl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x20,0x0f]
lvsl128 127, 3, 4
# ENC: lvsr128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x20,0x4f]
lvsr128 127, 3, 4
# ENC: lvewx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x20,0x8f]
lvewx128 127, 3, 4
# ENC: lvx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x20,0xcf]
lvx128 127, 3, 4
# ENC: lvxl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x22,0xcf]
lvxl128 127, 3, 4
# ENC: lvlx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x24,0x0f]
lvlx128 127, 3, 4
# ENC: lvrx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x24,0x4f]
lvrx128 127, 3, 4
# ENC: lvlxl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x26,0x0f]
lvlxl128 127, 3, 4
# ENC: lvrxl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x26,0x4f]
lvrxl128 127, 3, 4
# ENC: stvewx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x21,0x8f]
stvewx128 127, 3, 4
# ENC: stvx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x21,0xcf]
stvx128 127, 3, 4
# ENC: stvxl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x23,0xcf]
stvxl128 127, 3, 4
# ENC: stvlx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x25,0x0f]
stvlx128 127, 3, 4
# ENC: stvrx128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x25,0x4f]
stvrx128 127, 3, 4
# ENC: stvlxl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x27,0x0f]
stvlxl128 127, 3, 4
# ENC: stvrxl128 127, 3, 4
# ENC-SAME: encoding: [0x13,0xe3,0x27,0x4f]
stvrxl128 127, 3, 4

# Three-register arithmetic, logical, pack, and compare operations.
# ENC: vaddfp128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x04,0x3e]
vaddfp128 127, 96, 64
# ENC: vsubfp128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x04,0x7e]
vsubfp128 127, 96, 64
# ENC: vmulfp128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x04,0xbe]
vmulfp128 127, 96, 64
# ENC: vmsum3fp128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x05,0xbe]
vmsum3fp128 127, 96, 64
# ENC: vmsum4fp128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x05,0xfe]
vmsum4fp128 127, 96, 64
# ENC: vand128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0x3e]
vand128 127, 96, 64
# ENC: vandc128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0x7e]
vandc128 127, 96, 64
# ENC: vnor128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0xbe]
vnor128 127, 96, 64
# ENC: vor128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0xfe]
vor128 127, 96, 64
# ENC: vxor128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0x3e]
vxor128 127, 96, 64
# ENC: vslo128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0xbe]
vslo128 127, 96, 64
# ENC: vpkshss128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0x2e]
vpkshss128 127, 96, 64
# ENC: vpkshus128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0x6e]
vpkshus128 127, 96, 64
# ENC: vpkswss128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0xae]
vpkswss128 127, 96, 64
# ENC: vpkswus128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x06,0xee]
vpkswus128 127, 96, 64
# ENC: vpkuhum128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0x2e]
vpkuhum128 127, 96, 64
# ENC: vpkuhus128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0x6e]
vpkuhus128 127, 96, 64
# ENC: vpkuwum128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0xae]
vpkuwum128 127, 96, 64
# ENC: vpkuwus128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0xee]
vpkuwus128 127, 96, 64
# ENC: vcmpeqfp128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x04,0x2e]
vcmpeqfp128 127, 96, 64
# ENC: vcmpeqfp128. 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x04,0x6e]
vcmpeqfp128. 127, 96, 64
# ENC: vcmpgefp128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x04,0xae]
vcmpgefp128 127, 96, 64
# ENC: vcmpgefp128. 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x04,0xee]
vcmpgefp128. 127, 96, 64
# ENC: vcmpgtfp128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x05,0x2e]
vcmpgtfp128 127, 96, 64
# ENC: vcmpgtfp128. 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x05,0x6e]
vcmpgtfp128. 127, 96, 64
# ENC: vcmpbfp128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x05,0xae]
vcmpbfp128 127, 96, 64
# ENC: vcmpbfp128. 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x05,0xee]
vcmpbfp128. 127, 96, 64
# ENC: vcmpequw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0x2e]
vcmpequw128 127, 96, 64
# ENC: vcmpequw128. 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0x6e]
vcmpequw128. 127, 96, 64

# Tied destination/source instructions.
# ENC: vmaddfp128 127, 96, 64, 127
# ENC-SAME: encoding: [0x17,0xe0,0x04,0xfe]
vmaddfp128 127, 96, 64, 127
# ENC: vmaddcfp128 127, 96, 127, 64
# ENC-SAME: encoding: [0x17,0xe0,0x05,0x3e]
vmaddcfp128 127, 96, 127, 64
# ENC: vnmsubfp128 127, 96, 64, 127
# ENC-SAME: encoding: [0x17,0xe0,0x05,0x7e]
vnmsubfp128 127, 96, 64, 127
# ENC: vsel128 127, 96, 64, 127
# ENC-SAME: encoding: [0x17,0xe0,0x07,0x7e]
vsel128 127, 96, 64, 127

# Other three-register operations.
# ENC: vmaxfp128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0xae]
vmaxfp128 127, 96, 64
# ENC: vminfp128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0xee]
vminfp128 127, 96, 64
# ENC: vmrghw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x07,0x2e]
vmrghw128 127, 96, 64
# ENC: vmrglw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x07,0x6e]
vmrglw128 127, 96, 64
# ENC: vrlw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x04,0x7e]
vrlw128 127, 96, 64
# ENC: vslw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x04,0xfe]
vslw128 127, 96, 64
# ENC: vsraw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x05,0x7e]
vsraw128 127, 96, 64
# ENC: vsrw128 127, 96, 64
# ENC-SAME: encoding: [0x1b,0xe0,0x05,0xfe]
vsrw128 127, 96, 64
# ENC: vsro128 127, 96, 64
# ENC-SAME: encoding: [0x17,0xe0,0x07,0xfe]
vsro128 127, 96, 64

# Two-register operations and immediates.
# ENC: vcfpsxws128 127, 96, -16
# ENC-SAME: encoding: [0x1b,0xf0,0x02,0x3f]
vcfpsxws128 127, 96, -16
# ENC: vcfpuxws128 127, 96, 31
# ENC-SAME: encoding: [0x1b,0xff,0x02,0x7f]
vcfpuxws128 127, 96, 31
# ENC: vcsxwfp128 127, 96, -16
# ENC-SAME: encoding: [0x1b,0xf0,0x02,0xbf]
vcsxwfp128 127, 96, -16
# ENC: vcuxwfp128 127, 96, 31
# ENC-SAME: encoding: [0x1b,0xff,0x02,0xff]
vcuxwfp128 127, 96, 31
# ENC: vrfim128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x03,0x3f]
vrfim128 127, 96
# ENC: vrfin128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x03,0x7f]
vrfin128 127, 96
# ENC: vrfip128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x03,0xbf]
vrfip128 127, 96
# ENC: vrfiz128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x03,0xff]
vrfiz128 127, 96
# ENC: vupkhsb128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x03,0x8f]
vupkhsb128 127, 96
# ENC: vupklsb128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x03,0xcf]
vupklsb128 127, 96
# ENC: vrefp128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0x3f]
vrefp128 127, 96
# ENC: vrsqrtefp128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0x7f]
vrsqrtefp128 127, 96
# ENC: vexptefp128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0xbf]
vexptefp128 127, 96
# ENC: vlogefp128 127, 96
# ENC-SAME: encoding: [0x1b,0xe0,0x06,0xff]
vlogefp128 127, 96
# ENC: vspltw128 127, 96, 31
# ENC-SAME: encoding: [0x1b,0xff,0x07,0x3f]
vspltw128 127, 96, 31
# ENC: vspltisw128 127, 96, -16
# ENC-SAME: encoding: [0x1b,0xf0,0x07,0x7f]
vspltisw128 127, 96, -16
# ENC: vupkd3d128 127, 96, 31
# ENC-SAME: encoding: [0x1b,0xff,0x07,0xff]
vupkd3d128 127, 96, 31

# Permute and immediate-format operations.
# ENC: vperm128 127, 96, 64, 7
# ENC-SAME: encoding: [0x17,0xe0,0x05,0xee]
vperm128 127, 96, 64, 7
# ENC: vsldoi128 127, 96, 64, 15
# ENC-SAME: encoding: [0x13,0xe0,0x07,0xfe]
vsldoi128 127, 96, 64, 15
# ENC: vpermwi128 127, 96, 255
# ENC-SAME: encoding: [0x1b,0xff,0x03,0xdf]
vpermwi128 127, 96, 255
# ENC: vpkd3d128 127, 96, 7, 3, 3
# ENC-SAME: encoding: [0x1b,0xff,0x06,0xdf]
vpkd3d128 127, 96, 7, 3, 3
# ENC: vrlimi128 127, 96, 31, 3
# ENC-SAME: encoding: [0x1b,0xff,0x07,0xdf]
vrlimi128 127, 96, 31, 3

# DIS: lvsl128 127, 3, 4
# DIS: lvsr128 127, 3, 4
# DIS: lvewx128 127, 3, 4
# DIS: lvx128 127, 3, 4
# DIS: lvxl128 127, 3, 4
# DIS: lvlx128 127, 3, 4
# DIS: lvrx128 127, 3, 4
# DIS: lvlxl128 127, 3, 4
# DIS: lvrxl128 127, 3, 4
# DIS: stvewx128 127, 3, 4
# DIS: stvx128 127, 3, 4
# DIS: stvxl128 127, 3, 4
# DIS: stvlx128 127, 3, 4
# DIS: stvrx128 127, 3, 4
# DIS: stvlxl128 127, 3, 4
# DIS: stvrxl128 127, 3, 4
# DIS: vaddfp128 127, 96, 64
# DIS: vsubfp128 127, 96, 64
# DIS: vmulfp128 127, 96, 64
# DIS: vmsum3fp128 127, 96, 64
# DIS: vmsum4fp128 127, 96, 64
# DIS: vand128 127, 96, 64
# DIS: vandc128 127, 96, 64
# DIS: vnor128 127, 96, 64
# DIS: vor128 127, 96, 64
# DIS: vxor128 127, 96, 64
# DIS: vslo128 127, 96, 64
# DIS: vpkshss128 127, 96, 64
# DIS: vpkshus128 127, 96, 64
# DIS: vpkswss128 127, 96, 64
# DIS: vpkswus128 127, 96, 64
# DIS: vpkuhum128 127, 96, 64
# DIS: vpkuhus128 127, 96, 64
# DIS: vpkuwum128 127, 96, 64
# DIS: vpkuwus128 127, 96, 64
# DIS: vcmpeqfp128 127, 96, 64
# DIS: vcmpeqfp128. 127, 96, 64
# DIS: vcmpgefp128 127, 96, 64
# DIS: vcmpgefp128. 127, 96, 64
# DIS: vcmpgtfp128 127, 96, 64
# DIS: vcmpgtfp128. 127, 96, 64
# DIS: vcmpbfp128 127, 96, 64
# DIS: vcmpbfp128. 127, 96, 64
# DIS: vcmpequw128 127, 96, 64
# DIS: vcmpequw128. 127, 96, 64
# DIS: vmaddfp128 127, 96, 64, 127
# DIS: vmaddcfp128 127, 96, 127, 64
# DIS: vnmsubfp128 127, 96, 64, 127
# DIS: vsel128 127, 96, 64, 127
# DIS: vmaxfp128 127, 96, 64
# DIS: vminfp128 127, 96, 64
# DIS: vmrghw128 127, 96, 64
# DIS: vmrglw128 127, 96, 64
# DIS: vrlw128 127, 96, 64
# DIS: vslw128 127, 96, 64
# DIS: vsraw128 127, 96, 64
# DIS: vsrw128 127, 96, 64
# DIS: vsro128 127, 96, 64
# DIS: vcfpsxws128 127, 96, -16
# DIS: vcfpuxws128 127, 96, 31
# DIS: vcsxwfp128 127, 96, -16
# DIS: vcuxwfp128 127, 96, 31
# DIS: vrfim128 127, 96
# DIS: vrfin128 127, 96
# DIS: vrfip128 127, 96
# DIS: vrfiz128 127, 96
# DIS: vupkhsb128 127, 96
# DIS: vupklsb128 127, 96
# DIS: vrefp128 127, 96
# DIS: vrsqrtefp128 127, 96
# DIS: vexptefp128 127, 96
# DIS: vlogefp128 127, 96
# DIS: vspltw128 127, 96, 31
# DIS: vspltisw128 127, 96, -16
# DIS: vupkd3d128 127, 96, 31
# DIS: vperm128 127, 96, 64, 7
# DIS: vsldoi128 127, 96, 64, 15
# DIS: vpermwi128 127, 96, 255
# DIS: vpkd3d128 127, 96, 7, 3, 3
# DIS: vrlimi128 127, 96, 31, 3

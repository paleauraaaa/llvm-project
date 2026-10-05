# RUN: llvm-mc -triple=powerpc64-unknown-unknown -mcpu=pwr8 --show-encoding %s | FileCheck %s --check-prefix=ALTIVEC
# RUN: not llvm-mc -triple=powerpc64-unknown-unknown -mcpu=g5 -mattr=+vmx128 --show-encoding %s 2>&1 | FileCheck %s --check-prefix=VMX128
# RUN: llvm-mc -triple=powerpc64-unknown-unknown -mcpu=pwr8 -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --mcpu=g5 --mattr=+vmx128 %t.o | FileCheck %s --check-prefix=DIS

# These legacy Altivec instructions are accepted without VMX128.
# ALTIVEC: vmuloub 1, 2, 3
vmuloub 1, 2, 3
# ALTIVEC: vmulouh 1, 2, 3
vmulouh 1, 2, 3
# ALTIVEC: vmulosb 1, 2, 3
vmulosb 1, 2, 3
# ALTIVEC: vmulosh 1, 2, 3
vmulosh 1, 2, 3
# ALTIVEC: vmuleub 1, 2, 3
vmuleub 1, 2, 3
# ALTIVEC: vmuleuh 1, 2, 3
vmuleuh 1, 2, 3
# ALTIVEC: vmulesb 1, 2, 3
vmulesb 1, 2, 3
# ALTIVEC: vmulesh 1, 2, 3
vmulesh 1, 2, 3
# ALTIVEC: vsum4ubs 1, 2, 3
vsum4ubs 1, 2, 3
# ALTIVEC: vsum4shs 1, 2, 3
vsum4shs 1, 2, 3
# ALTIVEC: vsum2sws 1, 2, 3
vsum2sws 1, 2, 3
# ALTIVEC: vsum4sbs 1, 2, 3
vsum4sbs 1, 2, 3
# ALTIVEC: vsumsws 1, 2, 3
vsumsws 1, 2, 3
# ALTIVEC: vmhaddshs 1, 2, 3, 4
vmhaddshs 1, 2, 3, 4
# ALTIVEC: vmhraddshs 1, 2, 3, 4
vmhraddshs 1, 2, 3, 4
# ALTIVEC: vmladduhm 1, 2, 3, 4
vmladduhm 1, 2, 3, 4
# ALTIVEC: vmsumubm 1, 2, 3, 4
vmsumubm 1, 2, 3, 4
# ALTIVEC: vmsummbm 1, 2, 3, 4
vmsummbm 1, 2, 3, 4
# ALTIVEC: vmsumuhm 1, 2, 3, 4
vmsumuhm 1, 2, 3, 4
# ALTIVEC: vmsumuhs 1, 2, 3, 4
vmsumuhs 1, 2, 3, 4
# ALTIVEC: vmsumshm 1, 2, 3, 4
vmsumshm 1, 2, 3, 4
# ALTIVEC: vmsumshs 1, 2, 3, 4
vmsumshs 1, 2, 3, 4

# VMX128 assembly must reject these instructions, and its disassembler must
# leave their encodings undecoded.
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled
# VMX128: error: instruction use requires an option to be enabled

# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>
# DIS: <unknown>

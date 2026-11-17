; RUN: llvm-as < %s | llvm-dis | FileCheck %s

; CHECK: declare ppc_xbox360_sehcc void @seh(ptr, ptr)
declare ppc_xbox360_sehcc void @seh(ptr, ptr)

; CHECK: declare ppc_xbox360_sehcc void @seh_numeric(ptr, ptr)
declare cc 128 void @seh_numeric(ptr, ptr)

; CHECK: declare riscv_vls_cc(128) void @vls()
declare riscv_vls_cc(128) void @vls()

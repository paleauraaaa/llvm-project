// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fcxx-exceptions -fexceptions -fasync-exceptions -O1 -mllvm -verify-machineinstrs -emit-obj -o %t.O1 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fcxx-exceptions -fexceptions -fasync-exceptions -Os -mllvm -verify-machineinstrs -emit-obj -o %t.Os %s
// RUN: llvm-readobj --relocations %t.O1 | FileCheck %s
// RUN: llvm-readobj --relocations %t.Os | FileCheck %s

// Hardware-loop preparation must run before WinEHPrepare: it can introduce
// LCSSA PHIs in the cleanup pads for partial array construction.
// CHECK: IMAGE_REL_PPC_ADDR32 __CxxFrameHandler
// CHECK: IMAGE_REL_PPC_ADDR32 $cppxdata$array_cleanup
int risky(int);
struct Guard {
  int value;
  Guard(int);
  ~Guard();
};

extern "C" int array_cleanup(int n) {
  Guard array[3] = {Guard(n), Guard(n + 1), Guard(n + 2)};
  return risky(array[2].value);
}

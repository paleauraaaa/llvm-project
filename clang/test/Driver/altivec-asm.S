// RUN: %clang -target powerpc64-linux-gnu -maltivec -S %s -o - | FileCheck %s
// RUN: %clang -target powerpc64le-linux-gnu -maltivec -S %s -o - | FileCheck %s
// RUN: %clang -target powerpc64-ibm-xbox360 -S %s -o - | FileCheck %s
// RUN: %clang -target powerpc64-ibm-xbox360 -c %s -o %t.o
// Verify that assembling an empty file does not auto-include altivec.h.
// CHECK-NOT: static vector

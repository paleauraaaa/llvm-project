// RUN: split-file %s %t
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c -std=c11 -fms-extensions -verify %t/void.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -std=c++11 -fms-extensions -verify %t/void.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c -std=c11 -verify=standard %t/void.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -std=c++11 -verify=standard %t/void.c
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -x c++ -std=c++11 -fms-extensions -verify=standard %t/void.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c -std=c11 -fms-extensions -verify %t/float.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -std=c++11 -fms-extensions -verify %t/float.c
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c -std=c11 -fms-extensions -emit-llvm -o - %t/void.c | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c++ -std=c++11 -fms-extensions -emit-llvm -o - %t/void.c | FileCheck %s
// CHECK-LABEL: define{{.*}} void @main(
// CHECK: ret void

//--- void.c
// expected-no-diagnostics
void main(void) {} // standard-error {{'main' must return 'int'}}

//--- float.c
float main(void) { return 0; } // expected-error {{'main' must return 'int'}}

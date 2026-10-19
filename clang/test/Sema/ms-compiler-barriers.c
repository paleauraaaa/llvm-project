// RUN: %clang_cc1 -triple aarch64-unknown-linux-gnu -fms-extensions -fsyntax-only -verify=MS %s
// RUN: %clang_cc1 -triple riscv64-unknown-linux-gnu -fms-extensions -fsyntax-only -verify=MS %s
// RUN: %clang_cc1 -triple aarch64-unknown-linux-gnu -fsyntax-only -verify=NO-MS %s

void undeclared(void) {
  _ReadBarrier(); // MS-error {{call to undeclared library function '_ReadBarrier'}} NO-MS-error {{call to undeclared function '_ReadBarrier'}}
  _WriteBarrier(); // MS-error {{call to undeclared library function '_WriteBarrier'}} NO-MS-error {{call to undeclared function '_WriteBarrier'}}
  _ReadWriteBarrier(); // MS-error {{call to undeclared library function '_ReadWriteBarrier'}} NO-MS-error {{call to undeclared function '_ReadWriteBarrier'}}
}

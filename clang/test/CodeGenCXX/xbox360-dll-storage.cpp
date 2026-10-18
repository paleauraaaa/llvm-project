// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -Werror -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -Werror -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.obj %s
// RUN: llvm-readobj --relocations --coff-directives %t.obj | FileCheck %s --check-prefix=OBJ
// RUN: %clang_cl --target=powerpc64-ibm-xbox360 /c /WX /Fo%t.cl.obj -- %s
// RUN: llvm-readobj --relocations --coff-directives %t.cl.obj | FileCheck %s --check-prefix=OBJ

struct __declspec(dllimport) Library {
  static int value;
  static int function(int);
  int method();
};

struct __declspec(dllexport) Client {
  static int value;
  int method();
};

int Client::value = 42;
int Client::method() { return Library::value + Library::function(value); }
int *data_address() { return &Library::value; }
int (*function_address())(int) { return Library::function; }
int call_method(Library *object) { return object->method(); }

// IR-DAG: @"?value@Library@@2HA" = external dllimport global i32
// IR-DAG: @"?value@Client@@2HA" = dso_local dllexport global i32 42
// IR-DAG: define dso_local dllexport {{.*}} @"?method@Client@@QAAHXZ"
// IR-DAG: declare dllimport {{.*}} @"?function@Library@@SAHH@Z"
// IR-DAG: declare dllimport {{.*}} @"?method@Library@@QAAHXZ"

// OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_?value@Library@@2HA
// OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_?value@Library@@2HA
// OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_?function@Library@@SAHH@Z
// OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_?function@Library@@SAHH@Z
// OBJ-DAG: IMAGE_REL_PPC_REL24 ?function@Library@@SAHH@Z
// OBJ-DAG: IMAGE_REL_PPC_REL24 ?method@Library@@QAAHXZ
// OBJ: Directive(s):
// OBJ-DAG: /EXPORT:"?value@Client@@2HA",DATA
// OBJ-DAG: /EXPORT:"?method@Client@@QAAHXZ"

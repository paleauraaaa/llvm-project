// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fms-extensions -Werror -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -Werror -O0 -mllvm -fast-isel=true -mllvm -verify-machineinstrs -emit-obj -o %t.o0.obj %s
// RUN: llvm-readobj --relocations --coff-directives %t.o0.obj | FileCheck %s --check-prefix=OBJ
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -Werror -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.o2.obj %s
// RUN: llvm-readobj --relocations --coff-directives %t.o2.obj | FileCheck %s --check-prefix=OBJ

__declspec(dllimport) int imported_function(int);
__declspec(dllimport) extern int imported_data;
__declspec(dllexport) int exported_data = 42;
int (*stored_function)(int) = imported_function;

// IR-DAG: @imported_data = external dllimport global i32
// IR-DAG: @exported_data = dso_local dllexport global i32 42
// IR-DAG: declare dllimport {{.*}} @imported_function(
// IR-LABEL: define dso_local dllexport {{.*}} @exported_function(
// IR: call {{.*}} @imported_function(
// IR: load i32, ptr @imported_data
__declspec(dllexport) int exported_function(int value) {
  imported_data = value;
  return imported_function(value) + imported_data;
}

int *data_address(void) { return &imported_data; }
int (*function_address(void))(int) { return imported_function; }

// OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_imported_data
// OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_imported_data
// OBJ-DAG: IMAGE_REL_PPC_REFHI __imp_imported_function
// OBJ-DAG: IMAGE_REL_PPC_REFLO __imp_imported_function
// OBJ-DAG: IMAGE_REL_PPC_REL24 imported_function
// OBJ-DAG: IMAGE_REL_PPC_ADDR32 imported_function
// OBJ: Directive(s):
// OBJ-DAG: /EXPORT:exported_function
// OBJ-DAG: /EXPORT:exported_data,DATA

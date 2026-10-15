// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fcxx-exceptions -fexceptions -mllvm -verify-machineinstrs -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fcxx-exceptions -fexceptions -mllvm -verify-machineinstrs -O2 -emit-obj -o %t.O2 %s
// RUN: llvm-readobj --symbols --relocations %t.O0 | FileCheck %s
// RUN: llvm-readobj --symbols --relocations %t.O2 | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -fcxx-exceptions -fexceptions -O2 -mllvm -verify-machineinstrs -S -o - %s | FileCheck %s --check-prefix=ASM

void may_throw();
void observe(int);
struct Object {
  int value;
  Object();
  Object(const Object &);
  ~Object();
};

extern "C" int ordered() {
  Object guard;
  try {
    may_throw();
  } catch (const Object &object) {
    return object.value;
  } catch (int value) {
    return value;
  } catch (...) {
    return -1;
  }
  return 0;
}

extern "C" int by_value() {
  try {
    may_throw();
  } catch (Object object) {
    return object.value;
  }
  return 0;
}

extern "C" int nested() {
  Object outer;
  try {
    Object inner;
    try {
      may_throw();
    } catch (int value) {
      observe(value);
      throw;
    }
  } catch (...) {
    return 1;
  }
  return 0;
}

template <class T> __attribute__((noinline)) int comdat_catch() {
  try {
    may_throw();
  } catch (T value) {
    return value;
  }
  return 0;
}
template int comdat_catch<int>();

void use_buffer(void *);
// Shared SSA spills, including the destructor's object pointer, must use the
// parent frame rather than the freshly allocated private catch/cleanup frame.
// ASM-LABEL: dynamic_catch:
// ASM: mr 30, 1
// ASM: mr 31, 1
// ASM: bl _RtlCheckStack12
// ASM: lwz 1, 0(30)
// ASM: blr
// ASM: addi 31, 12, -{{[0-9]+}}
// ASM: mr 30, 1
// ASM: lwz {{[0-9]+}}, {{[0-9]+}}(31)
// ASM: bl _RtlCheckStack12
// ASM: lwz 1, 0(30)
// ASM: blr
// ASM: addi 31, 12, -{{[0-9]+}}
// ASM: mr 30, 1
// ASM: ld {{[0-9]+}}, {{[0-9]+}}(31)
// ASM: lwz 1, 0(30)
// ASM: blr
extern "C" int dynamic_catch(unsigned size) {
  Object guard;
  void *buffer = __builtin_alloca(size);
  try {
    use_buffer(buffer);
  } catch (int value) {
    void *temporary = __builtin_alloca(size);
    use_buffer(temporary);
    return value;
  }
  return 0;
}

// CHECK: IMAGE_REL_PPC_ADDR32 __CxxFrameHandler
// CHECK: IMAGE_REL_PPC_REFHI
// CHECK: IMAGE_REL_PPC_REFLO
// CHECK-DAG: Name: $cppxdata$ordered
// CHECK-DAG: Name: $cppxdata$by_value
// CHECK-DAG: Name: $cppxdata$nested
// CHECK-DAG: Name: $cppxdata$dynamic_catch
// CHECK-DAG: Name: $cppxdata$??$comdat_catch@H

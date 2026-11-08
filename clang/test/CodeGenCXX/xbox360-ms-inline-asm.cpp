// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -emit-obj -o %t.obj %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -O2 -emit-obj -o %t.opt.obj %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -fno-elide-constructors -emit-obj -o %t.no-nrvo.obj %s

namespace data {
int value;
void callee();
}

// CHECK-LABEL: define {{.*}} @qualified(
// CHECK: store ptr @"?value@data@@3HA"
// CHECK: asm sideeffect {{.*}}addi 11, 9, 1{{.*}}stw 11, 0(12)
extern "C" int qualified(int a) {
  using data::value;
  __asm { addi value, a, 1 }
  return data::value;
}

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float x, y, z, w;
} __vector4;

// CHECK-LABEL: define {{.*}} @vector_variables(
// CHECK: %msasm.retval = alloca %struct.__vector4
// CHECK: [[ASM:%.*]] = call <4 x float> asm sideeffect "lwz
// CHECK-SAME: lvx128
// CHECK-SAME: lvx128
// CHECK-SAME: vaddfp128
// CHECK-SAME: stvx128
// CHECK: store <4 x float> [[ASM]], ptr %msasm.retval
// CHECK-NOT: store <4 x float> [[ASM]], ptr %retval
// CHECK: load <4 x float>, ptr %retval
extern "C" __vector4 vector_variables(__vector4 a, __vector4 b) {
  __vector4 result;
  __asm { vaddfp128 result, a, b }
  return result;
}

// CHECK-LABEL: define {{.*}} @intrinsic_return(
// CHECK: call <4 x float> asm sideeffect {{.*}}"=&{v1},~{memory}"
// CHECK: call void @llvm.memcpy{{.*}}(ptr {{.*}}%retval, ptr {{.*}}%msasm.retval
extern "C" __vector4 intrinsic_return() {
  __asm { vxor128 vr1, vr1, vr1 }
}

// CHECK-LABEL: define {{.*}} @conditional_return(
// CHECK: [[IMPLICIT:%.*]] = call <4 x float> asm sideeffect
// CHECK: store <4 x float> [[IMPLICIT]], ptr %msasm.retval
// CHECK: if.then:
// CHECK: call void @llvm.memcpy{{.*}}(ptr {{.*}}%retval, ptr {{.*}}%b
// CHECK: br label %return
// CHECK: if.end:
// CHECK: call void @llvm.memcpy{{.*}}(ptr {{.*}}%retval, ptr {{.*}}%msasm.retval
// CHECK: return:
extern "C" __vector4 conditional_return(__vector4 a, __vector4 b, int explicit_return) {
  __asm { vaddfp128 vr1, a, a }
  if (explicit_return)
    return b;
}

// CHECK-LABEL: define {{.*}} @cpp_call(
// CHECK: asm sideeffect {{.*}}bl ${0:c}{{.*}}bl ${1:c}
// CHECK-SAME: "i,i,
// CHECK-SAME: (ptr @"?callee@data@@YAXXZ", ptr @"?callee@data@@YAXXZ")
extern "C" void cpp_call() {
  using data::callee;
  __asm {
    bl callee
    bl data::callee
  }
}

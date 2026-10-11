// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float lanes[4];
} __vector4;

enum PackType { Type = 7 };
enum PackMask { Mask = 3 };

extern "C" {
__vector4 __vaddfp(__vector4, __vector4);
__vector4 __vcfsx(__vector4, unsigned short);
__vector4 __vspltisw(int);
__vector4 __vspltisb(int);
__vector4 __vspltish(int);
__vector4 __vpermwi(__vector4, unsigned int);
__vector4 __vpkd3d(__vector4, __vector4, PackType, PackMask, unsigned int);
__vector4 __vrlimi(__vector4, __vector4, unsigned int, unsigned int);
void __VMXSetReg(int, __vector4);
__vector4 __VMXGetReg(int);
__vector4 __lvebx_volatile(const volatile void *, int);
__vector4 __lvehx_volatile(const volatile void *, int);
__vector4 __lvewx_volatile(const volatile void *, int);
__vector4 __lvx_volatile(const volatile void *, int);
void __stvebx_volatile(__vector4, volatile void *, int);
void __stvehx_volatile(__vector4, volatile void *, int);
void __stvewx_volatile(__vector4, volatile void *, int);
void __stvx_volatile(__vector4, volatile void *, int);
__vector4 __vcmpequbR(__vector4, __vector4, unsigned int *);
__vector4 next_vector();

#define __vcsxwfp __vcfsx

// CHECK-LABEL: define{{.*}} @once(
// CHECK: call{{.*}} @next_vector()
// CHECK-NOT: call{{.*}} @next_vector()
// CHECK: @llvm.ppc.altivec.vaddfp128(
// CHECK-NOT: call{{.*}} @next_vector()
__vector4 once(__vector4 a) {
  return __vaddfp(next_vector(), a);
}

// CHECK-LABEL: define{{.*}} @immediates(
// CHECK: @llvm.ppc.altivec.vcsxwfp128({{.*}}, i32 31)
// CHECK: @llvm.ppc.altivec.vpermwi128({{.*}}, i32 255)
// CHECK: @llvm.ppc.altivec.vpkd3d128({{.*}}, i32 7, i32 3, i32 3)
// CHECK: @llvm.ppc.altivec.vrlimi128({{.*}}, i32 15, i32 3)
__vector4 immediates(__vector4 a, __vector4 b) {
  a = __vcsxwfp(a, 31);
  a = __vpermwi(a, 255);
  a = __vpkd3d(a, b, Type, Mask, 3);
  return __vrlimi(a, b, 15, 3);
}

// CHECK-LABEL: define{{.*}} @registers(
// CHECK: @llvm.ppc.altivec.set.vr128(<4 x i32> {{.*}}, i32 127)
// CHECK: @llvm.ppc.altivec.get.vr128(i32 127)
__vector4 registers(__vector4 a) {
  __VMXSetReg(127, a);
  return __VMXGetReg(127);
}

// CHECK-LABEL: define{{.*}} @byte_splat(
// CHECK: store <16 x i8> splat (i8 -16)
__vector4 byte_splat() { return __vspltisb(-16); }

// CHECK-LABEL: define{{.*}} @half_splat(
// CHECK: store <8 x i16> splat (i16 15)
__vector4 half_splat() { return __vspltish(15); }

// OPT-LABEL: define{{.*}} @unused_volatile(
// OPT-COUNT-2: call <16 x i8> @llvm.ppc.altivec.lvebx.volatile(
// OPT-COUNT-2: call <8 x i16> @llvm.ppc.altivec.lvehx.volatile(
// OPT-COUNT-2: call <4 x i32> @llvm.ppc.altivec.lvewx128.volatile(ptr {{.*}}, i32 -1)
// OPT-COUNT-2: call <4 x i32> @llvm.ppc.altivec.lvx128.volatile(ptr {{.*}}, i32 -1)
// OPT: call void @llvm.ppc.altivec.stvebx.volatile(
// OPT: call void @llvm.ppc.altivec.stvehx.volatile(
// OPT: call void @llvm.ppc.altivec.stvewx128.volatile(
// OPT: call void @llvm.ppc.altivec.stvx128.volatile(
void unused_volatile(__vector4 a, volatile void *p) {
  __lvebx_volatile(p, -1);
  __lvebx_volatile(p, -1);
  __lvehx_volatile(p, -1);
  __lvehx_volatile(p, -1);
  __lvewx_volatile(p, -1);
  __lvewx_volatile(p, -1);
  __lvx_volatile(p, -1);
  __lvx_volatile(p, -1);
  __stvebx_volatile(a, p, -1);
  __stvehx_volatile(a, p, -1);
  __stvewx_volatile(a, p, -1);
  __stvx_volatile(a, p, -1);
}

// OPT-LABEL: define{{.*}} @unused_record(
// OPT: call <16 x i8> @llvm.ppc.altivec.vcmpequb.rec(
void unused_record(__vector4 a, __vector4 b, unsigned int *cr) {
  __vcmpequbR(a, b, cr);
}
}

namespace user {
// CHECK-LABEL: define{{.*}}ordinary
// CHECK-NOT: llvm.ppc.altivec.vaddfp128
// CHECK: call{{.*}}__vaddfp
__vector4 __vaddfp(__vector4, __vector4);
__vector4 ordinary(__vector4 a) { return user::__vaddfp(a, a); }
}

// REQUIRES: powerpc-registered-target
// RUN: %clang_cc1 -triple powerpc-unknown-linux-gnu -target-cpu g5 -target-feature +vmx128 \
// RUN:   -emit-llvm -o - %s | FileCheck %s

typedef __vector char v16c;
typedef __vector signed char v16sc;
typedef __vector unsigned char v16u8;
typedef __vector signed short v8i16;
typedef __vector signed int v4i32;
typedef __vector float v4f32;

v4i32 test_get_vr128(void) {
  return __builtin_altivec_get_vr128(64);
}
// CHECK-LABEL: define{{.*}} @test_get_vr128
// CHECK: call <4 x i32> @llvm.ppc.altivec.get.vr128(i32 64)

void test_set_vr128(v4i32 v) {
  __builtin_altivec_set_vr128(v, 65);
}
// CHECK-LABEL: define{{.*}} @test_set_vr128
// CHECK: call void @llvm.ppc.altivec.set.vr128(<4 x i32>{{.*}}, i32 65)

v4i32 test_lvewx128(const char *p, int offset) {
  return __builtin_altivec_lvewx128(p, offset);
}
// CHECK-LABEL: define{{.*}} @test_lvewx128
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvewx128(ptr{{.*}}, i32

v16c test_lvsl128(const char *p, int offset) {
  return __builtin_altivec_lvsl128(p, offset);
}
// CHECK-LABEL: define{{.*}} @test_lvsl128
// CHECK: call <16 x i8> @llvm.ppc.altivec.lvsl128(ptr{{.*}}, i32

void test_stvx128(v4i32 v, int *p, int offset) {
  __builtin_altivec_stvx128(v, p, offset);
}
// CHECK-LABEL: define{{.*}} @test_stvx128
// CHECK: call void @llvm.ppc.altivec.stvx128(<4 x i32>{{.*}}, ptr{{.*}}, i32

v4i32 test_lvlx128_volatile(const volatile void *p, int offset) {
  return __builtin_altivec_lvlx128_volatile(p, offset);
}
// CHECK-LABEL: define{{.*}} @test_lvlx128_volatile
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(ptr{{.*}}, i32

v4i32 test_lvrx128_volatile(const volatile void *p, int offset) {
  return __builtin_altivec_lvrx128_volatile(p, offset);
}
// CHECK-LABEL: define{{.*}} @test_lvrx128_volatile
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(ptr{{.*}}, i32

void test_stvlx128_volatile(v4i32 v, volatile void *p, int offset) {
  __builtin_altivec_stvlx128_volatile(v, p, offset);
}
// CHECK-LABEL: define{{.*}} @test_stvlx128_volatile
// CHECK: call void @llvm.ppc.altivec.stvlx128.volatile(<4 x i32>{{.*}}, ptr{{.*}}, i32

void test_stvrx128_volatile(v4i32 v, volatile void *p, int offset) {
  __builtin_altivec_stvrx128_volatile(v, p, offset);
}
// CHECK-LABEL: define{{.*}} @test_stvrx128_volatile
// CHECK: call void @llvm.ppc.altivec.stvrx128.volatile(<4 x i32>{{.*}}, ptr{{.*}}, i32

v4f32 test_vaddfp128(v4f32 a, v4f32 b) {
  return __builtin_altivec_vaddfp128(a, b);
}
// CHECK-LABEL: define{{.*}} @test_vaddfp128
// CHECK: call <4 x float> @llvm.ppc.altivec.vaddfp128

v4i32 test_vand128(v4i32 a, v4i32 b) {
  return __builtin_altivec_vand128(a, b);
}
// CHECK-LABEL: define{{.*}} @test_vand128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vand128

v4i32 test_vcmpeqfp128_rec(v4f32 a, v4f32 b, unsigned int *cr) {
  return __builtin_altivec_vcmpeqfp128_rec(a, b, cr);
}
// CHECK-LABEL: define{{.*}} @test_vcmpeqfp128_rec
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(<4 x float>{{.*}}, ptr

v4i32 test_vcfpsxws128(v4f32 a) {
  return __builtin_altivec_vcfpsxws128(a, 3);
}
// CHECK-LABEL: define{{.*}} @test_vcfpsxws128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcfpsxws128

v4f32 test_vexptefp128(v4f32 a) {
  return __builtin_altivec_vexptefp128(a);
}
// CHECK-LABEL: define{{.*}} @test_vexptefp128
// CHECK: call <4 x float> @llvm.ppc.altivec.vexptefp128

v4f32 test_vmaddfp128(v4f32 a, v4f32 b, v4f32 c) {
  return __builtin_altivec_vmaddfp128(a, b, c);
}
// CHECK-LABEL: define{{.*}} @test_vmaddfp128
// CHECK: call <4 x float> @llvm.ppc.altivec.vmaddfp128

v4i32 test_vperm128(v4i32 a, v4i32 b, v16u8 c) {
  return __builtin_altivec_vperm128(a, b, c);
}
// CHECK-LABEL: define{{.*}} @test_vperm128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vperm128

v4i32 test_vpermwi128(v4i32 a) {
  return __builtin_altivec_vpermwi128(a, 27);
}
// CHECK-LABEL: define{{.*}} @test_vpermwi128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vpermwi128

v16sc test_vpkshss128(v8i16 a, v8i16 b) {
  return __builtin_altivec_vpkshss128(a, b);
}
// CHECK-LABEL: define{{.*}} @test_vpkshss128
// CHECK: call <16 x i8> @llvm.ppc.altivec.vpkshss128

v4i32 test_vpkd3d128(v4i32 a, v4i32 b) {
  return __builtin_altivec_vpkd3d128(a, b, 1, 2, 3);
}
// CHECK-LABEL: define{{.*}} @test_vpkd3d128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vpkd3d128

v4i32 test_vrlimi128(v4i32 a, v4i32 b) {
  return __builtin_altivec_vrlimi128(a, b, 1, 15);
}
// CHECK-LABEL: define{{.*}} @test_vrlimi128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vrlimi128

v4i32 test_vspltisw128(void) {
  return __builtin_altivec_vspltisw128(3);
}
// CHECK-LABEL: define{{.*}} @test_vspltisw128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vspltisw128

v4i32 test_vsldoi128(v4i32 a, v4i32 b) {
  return __builtin_altivec_vsldoi128(a, b, 4);
}
// CHECK-LABEL: define{{.*}} @test_vsldoi128
// CHECK: call <4 x i32> @llvm.ppc.altivec.vsldoi128

v8i16 test_vupkhsb128(v16sc a) {
  return __builtin_altivec_vupkhsb128(a);
}
// CHECK-LABEL: define{{.*}} @test_vupkhsb128
// CHECK: call <8 x i16> @llvm.ppc.altivec.vupkhsb128

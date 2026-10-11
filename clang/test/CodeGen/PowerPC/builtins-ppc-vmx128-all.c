// Cover every VMX128 builtin-to-intrinsic mapping in BuiltinsPPC.def.
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc-unknown-linux-gnu -target-cpu g5 -target-feature +vmx128 -emit-llvm -o - %s | FileCheck %s
typedef int v4i __attribute__((vector_size(16)));
typedef signed int v4si __attribute__((vector_size(16)));
typedef unsigned int v4ui __attribute__((vector_size(16)));
typedef float v4f __attribute__((vector_size(16)));
typedef signed short v8ss __attribute__((vector_size(16)));
typedef unsigned short v8us __attribute__((vector_size(16)));
typedef char v16c __attribute__((vector_size(16)));
typedef signed char v16sc __attribute__((vector_size(16)));
typedef unsigned char v16uc __attribute__((vector_size(16)));

v4i test_get_vr128(void) {
  return __builtin_altivec_get_vr128(64);
}
// CHECK-LABEL: define{{.*}} @test_get_vr128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.get.vr128(

void test_set_vr128(v4i a0) {
  __builtin_altivec_set_vr128(a0, 65);
}
// CHECK-LABEL: define{{.*}} @test_set_vr128(
// CHECK: call void @llvm.ppc.altivec.set.vr128(

v4i test_lvewx128(const void * a0, int a1) {
  return __builtin_altivec_lvewx128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvewx128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvewx128(

v4i test_lvlx128(const void * a0, int a1) {
  return __builtin_altivec_lvlx128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvlx128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvlx128(

v4i test_lvlx128_volatile(const volatile void * a0, int a1) {
  return __builtin_altivec_lvlx128_volatile(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvlx128_volatile(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvlx128.volatile(

v4i test_lvlxl128(const void * a0, int a1) {
  return __builtin_altivec_lvlxl128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvlxl128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvlxl128(

v4i test_lvrx128(const void * a0, int a1) {
  return __builtin_altivec_lvrx128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvrx128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvrx128(

v4i test_lvrx128_volatile(const volatile void * a0, int a1) {
  return __builtin_altivec_lvrx128_volatile(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvrx128_volatile(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvrx128.volatile(

v4i test_lvrxl128(const void * a0, int a1) {
  return __builtin_altivec_lvrxl128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvrxl128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvrxl128(

v16c test_lvsl128(const void * a0, int a1) {
  return __builtin_altivec_lvsl128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvsl128(
// CHECK: call <16 x i8> @llvm.ppc.altivec.lvsl128(

v16c test_lvsr128(const void * a0, int a1) {
  return __builtin_altivec_lvsr128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvsr128(
// CHECK: call <16 x i8> @llvm.ppc.altivec.lvsr128(

v4i test_lvx128(const void * a0, int a1) {
  return __builtin_altivec_lvx128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvx128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvx128(

v4i test_lvxl128(const void * a0, int a1) {
  return __builtin_altivec_lvxl128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_lvxl128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.lvxl128(

void test_stvewx128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvewx128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvewx128(
// CHECK: call void @llvm.ppc.altivec.stvewx128(

void test_stvlx128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvlx128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvlx128(
// CHECK: call void @llvm.ppc.altivec.stvlx128(

void test_stvlx128_volatile(v4i a0, volatile void * a1, int a2) {
  __builtin_altivec_stvlx128_volatile(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvlx128_volatile(
// CHECK: call void @llvm.ppc.altivec.stvlx128.volatile(

void test_stvlxl128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvlxl128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvlxl128(
// CHECK: call void @llvm.ppc.altivec.stvlxl128(

void test_stvrx128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvrx128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvrx128(
// CHECK: call void @llvm.ppc.altivec.stvrx128(

void test_stvrx128_volatile(v4i a0, volatile void * a1, int a2) {
  __builtin_altivec_stvrx128_volatile(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvrx128_volatile(
// CHECK: call void @llvm.ppc.altivec.stvrx128.volatile(

void test_stvrxl128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvrxl128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvrxl128(
// CHECK: call void @llvm.ppc.altivec.stvrxl128(

void test_stvx128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvx128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvx128(
// CHECK: call void @llvm.ppc.altivec.stvx128(

void test_stvxl128(v4i a0, void * a1, int a2) {
  __builtin_altivec_stvxl128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_stvxl128(
// CHECK: call void @llvm.ppc.altivec.stvxl128(

v4f test_vaddfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vaddfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vaddfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vaddfp128(

v4i test_vand128(v4i a0, v4i a1) {
  return __builtin_altivec_vand128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vand128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vand128(

v4i test_vandc128(v4i a0, v4i a1) {
  return __builtin_altivec_vandc128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vandc128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vandc128(

v4f test_vmaxfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vmaxfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vmaxfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vmaxfp128(

v4f test_vminfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vminfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vminfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vminfp128(

v4i test_vnor128(v4i a0, v4i a1) {
  return __builtin_altivec_vnor128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vnor128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vnor128(

v4i test_vor128(v4i a0, v4i a1) {
  return __builtin_altivec_vor128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vor128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vor128(

v4i test_vxor128(v4i a0, v4i a1) {
  return __builtin_altivec_vxor128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vxor128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vxor128(

v4f test_vsubfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vsubfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vsubfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vsubfp128(

v4i test_vcmpbfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vcmpbfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vcmpbfp128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpbfp128(

v4i test_vcmpbfp128_rec(v4f a0, v4f a1, unsigned int * a2) {
  return __builtin_altivec_vcmpbfp128_rec(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vcmpbfp128_rec(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpbfp128.rec(

v4i test_vcmpeqfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vcmpeqfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vcmpeqfp128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128(

v4i test_vcmpeqfp128_rec(v4f a0, v4f a1, unsigned int * a2) {
  return __builtin_altivec_vcmpeqfp128_rec(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vcmpeqfp128_rec(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpeqfp128.rec(

v4i test_vcmpgefp128(v4f a0, v4f a1) {
  return __builtin_altivec_vcmpgefp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vcmpgefp128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpgefp128(

v4i test_vcmpgefp128_rec(v4f a0, v4f a1, unsigned int * a2) {
  return __builtin_altivec_vcmpgefp128_rec(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vcmpgefp128_rec(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpgefp128.rec(

v4i test_vcmpgtfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vcmpgtfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vcmpgtfp128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpgtfp128(

v4i test_vcmpgtfp128_rec(v4f a0, v4f a1, unsigned int * a2) {
  return __builtin_altivec_vcmpgtfp128_rec(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vcmpgtfp128_rec(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpgtfp128.rec(

v4i test_vcmpequw128(v4i a0, v4i a1) {
  return __builtin_altivec_vcmpequw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vcmpequw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpequw128(

v4i test_vcmpequw128_rec(v4i a0, v4i a1, unsigned int * a2) {
  return __builtin_altivec_vcmpequw128_rec(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vcmpequw128_rec(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcmpequw128.rec(

v4i test_vcfpsxws128(v4f a0) {
  return __builtin_altivec_vcfpsxws128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vcfpsxws128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcfpsxws128(

v4i test_vcfpuxws128(v4f a0) {
  return __builtin_altivec_vcfpuxws128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vcfpuxws128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vcfpuxws128(

v4f test_vcsxwfp128(v4i a0) {
  return __builtin_altivec_vcsxwfp128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vcsxwfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vcsxwfp128(

v4f test_vcuxwfp128(v4i a0) {
  return __builtin_altivec_vcuxwfp128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vcuxwfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vcuxwfp128(

v4f test_vexptefp128(v4f a0) {
  return __builtin_altivec_vexptefp128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vexptefp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vexptefp128(

v4f test_vlogefp128(v4f a0) {
  return __builtin_altivec_vlogefp128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vlogefp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vlogefp128(

v4f test_vrefp128(v4f a0) {
  return __builtin_altivec_vrefp128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vrefp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vrefp128(

v4f test_vrsqrtefp128(v4f a0) {
  return __builtin_altivec_vrsqrtefp128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vrsqrtefp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vrsqrtefp128(

v4f test_vrfim128(v4f a0) {
  return __builtin_altivec_vrfim128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vrfim128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vrfim128(

v4f test_vrfin128(v4f a0) {
  return __builtin_altivec_vrfin128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vrfin128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vrfin128(

v4f test_vrfip128(v4f a0) {
  return __builtin_altivec_vrfip128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vrfip128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vrfip128(

v4f test_vrfiz128(v4f a0) {
  return __builtin_altivec_vrfiz128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vrfiz128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vrfiz128(

v4f test_vmaddcfp128(v4f a0, v4f a1, v4f a2) {
  return __builtin_altivec_vmaddcfp128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vmaddcfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vmaddcfp128(

v4f test_vmaddfp128(v4f a0, v4f a1, v4f a2) {
  return __builtin_altivec_vmaddfp128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vmaddfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vmaddfp128(

v4f test_vmsum3fp128(v4f a0, v4f a1) {
  return __builtin_altivec_vmsum3fp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vmsum3fp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vmsum3fp128(

v4f test_vmsum4fp128(v4f a0, v4f a1) {
  return __builtin_altivec_vmsum4fp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vmsum4fp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vmsum4fp128(

v4f test_vmulfp128(v4f a0, v4f a1) {
  return __builtin_altivec_vmulfp128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vmulfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vmulfp128(

v4f test_vnmsubfp128(v4f a0, v4f a1, v4f a2) {
  return __builtin_altivec_vnmsubfp128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vnmsubfp128(
// CHECK: call <4 x float> @llvm.ppc.altivec.vnmsubfp128(

v4i test_vmrghw128(v4i a0, v4i a1) {
  return __builtin_altivec_vmrghw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vmrghw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vmrghw128(

v4i test_vmrglw128(v4i a0, v4i a1) {
  return __builtin_altivec_vmrglw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vmrglw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vmrglw128(

v4i test_vperm128(v4i a0, v4i a1, v16uc a2) {
  return __builtin_altivec_vperm128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vperm128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vperm128(

v4i test_vpermwi128(v4i a0) {
  return __builtin_altivec_vpermwi128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vpermwi128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vpermwi128(

v4i test_vpkd3d128(v4i a0, v4i a1) {
  return __builtin_altivec_vpkd3d128(a0, a1, 1, 1, 1);
}
// CHECK-LABEL: define{{.*}} @test_vpkd3d128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vpkd3d128(

v16sc test_vpkshss128(v8ss a0, v8ss a1) {
  return __builtin_altivec_vpkshss128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkshss128(
// CHECK: call <16 x i8> @llvm.ppc.altivec.vpkshss128(

v16uc test_vpkshus128(v8ss a0, v8ss a1) {
  return __builtin_altivec_vpkshus128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkshus128(
// CHECK: call <16 x i8> @llvm.ppc.altivec.vpkshus128(

v8ss test_vpkswss128(v4si a0, v4si a1) {
  return __builtin_altivec_vpkswss128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkswss128(
// CHECK: call <8 x i16> @llvm.ppc.altivec.vpkswss128(

v8us test_vpkswus128(v4si a0, v4si a1) {
  return __builtin_altivec_vpkswus128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkswus128(
// CHECK: call <8 x i16> @llvm.ppc.altivec.vpkswus128(

v16uc test_vpkuhum128(v8us a0, v8us a1) {
  return __builtin_altivec_vpkuhum128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkuhum128(
// CHECK: call <16 x i8> @llvm.ppc.altivec.vpkuhum128(

v16uc test_vpkuhus128(v8us a0, v8us a1) {
  return __builtin_altivec_vpkuhus128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkuhus128(
// CHECK: call <16 x i8> @llvm.ppc.altivec.vpkuhus128(

v8us test_vpkuwum128(v4ui a0, v4ui a1) {
  return __builtin_altivec_vpkuwum128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkuwum128(
// CHECK: call <8 x i16> @llvm.ppc.altivec.vpkuwum128(

v8us test_vpkuwus128(v4ui a0, v4ui a1) {
  return __builtin_altivec_vpkuwus128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vpkuwus128(
// CHECK: call <8 x i16> @llvm.ppc.altivec.vpkuwus128(

v8ss test_vupkhsb128(v16sc a0) {
  return __builtin_altivec_vupkhsb128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vupkhsb128(
// CHECK: call <8 x i16> @llvm.ppc.altivec.vupkhsb128(

v8ss test_vupklsb128(v16sc a0) {
  return __builtin_altivec_vupklsb128(a0);
}
// CHECK-LABEL: define{{.*}} @test_vupklsb128(
// CHECK: call <8 x i16> @llvm.ppc.altivec.vupklsb128(

v4i test_vrlimi128(v4i a0, v4i a1) {
  return __builtin_altivec_vrlimi128(a0, a1, 1, 1);
}
// CHECK-LABEL: define{{.*}} @test_vrlimi128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vrlimi128(

v4i test_vrlw128(v4i a0, v4i a1) {
  return __builtin_altivec_vrlw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vrlw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vrlw128(

v4i test_vsel128(v4i a0, v4i a1, v4i a2) {
  return __builtin_altivec_vsel128(a0, a1, a2);
}
// CHECK-LABEL: define{{.*}} @test_vsel128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vsel128(

v4i test_vsldoi128(v4i a0, v4i a1) {
  return __builtin_altivec_vsldoi128(a0, a1, 1);
}
// CHECK-LABEL: define{{.*}} @test_vsldoi128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vsldoi128(

v4i test_vslo128(v4i a0, v4i a1) {
  return __builtin_altivec_vslo128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vslo128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vslo128(

v4i test_vslw128(v4i a0, v4i a1) {
  return __builtin_altivec_vslw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vslw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vslw128(

v4i test_vspltisw128(void) {
  return __builtin_altivec_vspltisw128(1);
}
// CHECK-LABEL: define{{.*}} @test_vspltisw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vspltisw128(

v4i test_vspltw128(v4i a0) {
  return __builtin_altivec_vspltw128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vspltw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vspltw128(

v4i test_vsraw128(v4i a0, v4i a1) {
  return __builtin_altivec_vsraw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vsraw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vsraw128(

v4i test_vsro128(v4i a0, v4i a1) {
  return __builtin_altivec_vsro128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vsro128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vsro128(

v4i test_vsrw128(v4i a0, v4i a1) {
  return __builtin_altivec_vsrw128(a0, a1);
}
// CHECK-LABEL: define{{.*}} @test_vsrw128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vsrw128(

v4i test_vupkd3d128(v4i a0) {
  return __builtin_altivec_vupkd3d128(a0, 1);
}
// CHECK-LABEL: define{{.*}} @test_vupkd3d128(
// CHECK: call <4 x i32> @llvm.ppc.altivec.vupkd3d128(

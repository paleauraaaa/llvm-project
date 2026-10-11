// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -x c++ -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -fno-builtin -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -fms-extensions -mllvm -verify-machineinstrs -O2 -emit-obj -o %t.opt.o %s

// Independently declared SDK API: no compiler-provided header or builtin aliases.
typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float lanes[4];
} __vector4;

#ifdef __cplusplus
extern "C" {
#endif

__vector4 __lvx_volatile(const volatile void *, int);
void __stvx_volatile(__vector4, volatile void *, int);
__vector4 __lvsl(const void *, int);
__vector4 __lvsr(const void *, int);
__vector4 __lvsl_volatile(const volatile void *, int);
__vector4 __lvsr_volatile(const volatile void *, int);
void __stvebx(__vector4, void *, int);
void __stvehx(__vector4, void *, int);
void __stvewx(__vector4, void *, int);
__vector4 __lvebx(const void *, int);
__vector4 __lvehx(const void *, int);
__vector4 __lvewx(const void *, int);
void __stvebx_volatile(__vector4, volatile void *, int);
void __stvehx_volatile(__vector4, volatile void *, int);
void __stvewx_volatile(__vector4, volatile void *, int);
__vector4 __lvebx_volatile(const volatile void *, int);
__vector4 __lvehx_volatile(const volatile void *, int);
__vector4 __lvewx_volatile(const volatile void *, int);
__vector4 __vaddfp(__vector4, __vector4);
__vector4 __vaddcuw(__vector4, __vector4);
__vector4 __vaddsbs(__vector4, __vector4);
__vector4 __vaddshs(__vector4, __vector4);
__vector4 __vaddsws(__vector4, __vector4);
__vector4 __vaddubm(__vector4, __vector4);
__vector4 __vaddubs(__vector4, __vector4);
__vector4 __vadduhm(__vector4, __vector4);
__vector4 __vadduhs(__vector4, __vector4);
__vector4 __vadduwm(__vector4, __vector4);
__vector4 __vadduws(__vector4, __vector4);
__vector4 __vand(__vector4, __vector4);
__vector4 __vandc(__vector4, __vector4);
__vector4 __vavgsb(__vector4, __vector4);
__vector4 __vavgsh(__vector4, __vector4);
__vector4 __vavgsw(__vector4, __vector4);
__vector4 __vavgub(__vector4, __vector4);
__vector4 __vavguh(__vector4, __vector4);
__vector4 __vavguw(__vector4, __vector4);
__vector4 __vcfsx(__vector4, unsigned short);
__vector4 __vcfux(__vector4, unsigned short);
__vector4 __vcmpbfp(__vector4, __vector4);
__vector4 __vcmpeqfp(__vector4, __vector4);
__vector4 __vcmpequb(__vector4, __vector4);
__vector4 __vcmpequh(__vector4, __vector4);
__vector4 __vcmpequw(__vector4, __vector4);
__vector4 __vcmpgefp(__vector4, __vector4);
__vector4 __vcmpgtfp(__vector4, __vector4);
__vector4 __vcmpgtsb(__vector4, __vector4);
__vector4 __vcmpgtsh(__vector4, __vector4);
__vector4 __vcmpgtsw(__vector4, __vector4);
__vector4 __vcmpgtub(__vector4, __vector4);
__vector4 __vcmpgtuh(__vector4, __vector4);
__vector4 __vcmpgtuw(__vector4, __vector4);
__vector4 __vcmpbfpR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpeqfpR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpequbR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpequhR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpequwR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgefpR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtfpR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtsbR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtshR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtswR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtubR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtuhR(__vector4, __vector4, unsigned int *);
__vector4 __vcmpgtuwR(__vector4, __vector4, unsigned int *);
__vector4 __vctsxs(__vector4, unsigned int);
__vector4 __vctuxs(__vector4, unsigned int);
__vector4 __vexptefp(__vector4);
__vector4 __vlogefp(__vector4);
__vector4 __vmaddfp(__vector4, __vector4, __vector4);
__vector4 __vmaxfp(__vector4, __vector4);
__vector4 __vmaxsb(__vector4, __vector4);
__vector4 __vmaxsh(__vector4, __vector4);
__vector4 __vmaxsw(__vector4, __vector4);
__vector4 __vmaxub(__vector4, __vector4);
__vector4 __vmaxuh(__vector4, __vector4);
__vector4 __vmaxuw(__vector4, __vector4);
__vector4 __vminfp(__vector4, __vector4);
__vector4 __vminsb(__vector4, __vector4);
__vector4 __vminsh(__vector4, __vector4);
__vector4 __vminsw(__vector4, __vector4);
__vector4 __vminub(__vector4, __vector4);
__vector4 __vminuh(__vector4, __vector4);
__vector4 __vminuw(__vector4, __vector4);
__vector4 __vmrghb(__vector4, __vector4);
__vector4 __vmrghh(__vector4, __vector4);
__vector4 __vmrghw(__vector4, __vector4);
__vector4 __vmrglb(__vector4, __vector4);
__vector4 __vmrglh(__vector4, __vector4);
__vector4 __vmrglw(__vector4, __vector4);
__vector4 __vnmsubfp(__vector4, __vector4, __vector4);
__vector4 __vnor(__vector4, __vector4);
__vector4 __vor(__vector4, __vector4);
__vector4 __vperm(__vector4, __vector4, __vector4);
__vector4 __vpkpx(__vector4, __vector4);
__vector4 __vpkshss(__vector4, __vector4);
__vector4 __vpkshus(__vector4, __vector4);
__vector4 __vpkswss(__vector4, __vector4);
__vector4 __vpkswus(__vector4, __vector4);
__vector4 __vpkuhum(__vector4, __vector4);
__vector4 __vpkuhus(__vector4, __vector4);
__vector4 __vpkuwum(__vector4, __vector4);
__vector4 __vpkuwus(__vector4, __vector4);
__vector4 __vrefp(__vector4);
__vector4 __vrfim(__vector4);
__vector4 __vrfin(__vector4);
__vector4 __vrfip(__vector4);
__vector4 __vrfiz(__vector4);
__vector4 __vrlb(__vector4, __vector4);
__vector4 __vrlh(__vector4, __vector4);
__vector4 __vrlw(__vector4, __vector4);
__vector4 __vrsqrtefp(__vector4);
__vector4 __vsel(__vector4, __vector4, __vector4);
__vector4 __vsl(__vector4, __vector4);
__vector4 __vslb(__vector4, __vector4);
__vector4 __vsldoi(__vector4, __vector4, unsigned int);
__vector4 __vslh(__vector4, __vector4);
__vector4 __vslo(__vector4, __vector4);
__vector4 __vslw(__vector4, __vector4);
__vector4 __vspltb(__vector4, unsigned int);
__vector4 __vsplth(__vector4, unsigned int);
__vector4 __vspltisb(int);
__vector4 __vspltish(int);
__vector4 __vspltisw(int);
__vector4 __vspltw(__vector4, unsigned int);
__vector4 __vsr(__vector4, __vector4);
__vector4 __vsrab(__vector4, __vector4);
__vector4 __vsrah(__vector4, __vector4);
__vector4 __vsraw(__vector4, __vector4);
__vector4 __vsrb(__vector4, __vector4);
__vector4 __vsrh(__vector4, __vector4);
__vector4 __vsro(__vector4, __vector4);
__vector4 __vsrw(__vector4, __vector4);
__vector4 __vsubcuw(__vector4, __vector4);
__vector4 __vsubfp(__vector4, __vector4);
__vector4 __vsubsbs(__vector4, __vector4);
__vector4 __vsubshs(__vector4, __vector4);
__vector4 __vsubsws(__vector4, __vector4);
__vector4 __vsububm(__vector4, __vector4);
__vector4 __vsububs(__vector4, __vector4);
__vector4 __vsubuhm(__vector4, __vector4);
__vector4 __vsubuhs(__vector4, __vector4);
__vector4 __vsubuwm(__vector4, __vector4);
__vector4 __vsubuws(__vector4, __vector4);
__vector4 __vupkhpx(__vector4);
__vector4 __vupkhsb(__vector4);
__vector4 __vupkhsh(__vector4);
__vector4 __vupklpx(__vector4);
__vector4 __vupklsb(__vector4);
__vector4 __vupklsh(__vector4);
__vector4 __vxor(__vector4, __vector4);
__vector4 __vmsum3fp(__vector4, __vector4);
__vector4 __vmsum4fp(__vector4, __vector4);
__vector4 __vmulfp(__vector4, __vector4);
__vector4 __vpermwi(__vector4, unsigned int);
__vector4 __vrlimi(__vector4, __vector4, unsigned int, unsigned int);
__vector4 __vpkd3d(__vector4, __vector4, int, int, unsigned int);
__vector4 __vupkd3d(__vector4, int);
__vector4 __lvlx(const void *, int);
__vector4 __lvrx(const void *, int);
void __stvlx(__vector4, void *, int);
void __stvrx(__vector4, void *, int);
__vector4 __lvlx_volatile(const volatile void *, int);
__vector4 __lvrx_volatile(const volatile void *, int);
void __stvlx_volatile(__vector4, volatile void *, int);
void __stvrx_volatile(__vector4, volatile void *, int);
__vector4 __VMXGetReg(int);
void __VMXSetReg(int, __vector4);

// CHECK-LABEL: define{{.*}} @test__lvx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvx128.volatile(
__vector4 test__lvx_volatile(const volatile void * a0, int a1) {
  return __lvx_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__stvx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvx128.volatile(
void test__stvx_volatile(__vector4 a0, volatile void * a1, int a2) {
  __stvx_volatile(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__lvsl(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvsl128(
__vector4 test__lvsl(const void * a0, int a1) {
  return __lvsl(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvsr(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvsr128(
__vector4 test__lvsr(const void * a0, int a1) {
  return __lvsr(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvsl_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvsl128(
__vector4 test__lvsl_volatile(const volatile void * a0, int a1) {
  return __lvsl_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvsr_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvsr128(
__vector4 test__lvsr_volatile(const volatile void * a0, int a1) {
  return __lvsr_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__stvebx(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvebx(
void test__stvebx(__vector4 a0, void * a1, int a2) {
  __stvebx(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__stvehx(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvehx(
void test__stvehx(__vector4 a0, void * a1, int a2) {
  __stvehx(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__stvewx(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvewx128(
void test__stvewx(__vector4 a0, void * a1, int a2) {
  __stvewx(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__lvebx(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvebx(
__vector4 test__lvebx(const void * a0, int a1) {
  return __lvebx(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvehx(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvehx(
__vector4 test__lvehx(const void * a0, int a1) {
  return __lvehx(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvewx(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvewx128(
__vector4 test__lvewx(const void * a0, int a1) {
  return __lvewx(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__stvebx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvebx.volatile(
void test__stvebx_volatile(__vector4 a0, volatile void * a1, int a2) {
  __stvebx_volatile(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__stvehx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvehx.volatile(
void test__stvehx_volatile(__vector4 a0, volatile void * a1, int a2) {
  __stvehx_volatile(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__stvewx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvewx128.volatile(
void test__stvewx_volatile(__vector4 a0, volatile void * a1, int a2) {
  __stvewx_volatile(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__lvebx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvebx.volatile(
__vector4 test__lvebx_volatile(const volatile void * a0, int a1) {
  return __lvebx_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvehx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvehx.volatile(
__vector4 test__lvehx_volatile(const volatile void * a0, int a1) {
  return __lvehx_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvewx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvewx128.volatile(
__vector4 test__lvewx_volatile(const volatile void * a0, int a1) {
  return __lvewx_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vaddfp128(
__vector4 test__vaddfp(__vector4 a0, __vector4 a1) {
  return __vaddfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddcuw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vaddcuw(
__vector4 test__vaddcuw(__vector4 a0, __vector4 a1) {
  return __vaddcuw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddsbs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vaddsbs(
__vector4 test__vaddsbs(__vector4 a0, __vector4 a1) {
  return __vaddsbs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddshs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vaddshs(
__vector4 test__vaddshs(__vector4 a0, __vector4 a1) {
  return __vaddshs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddsws(
// CHECK: call {{.*}} @llvm.ppc.altivec.vaddsws(
__vector4 test__vaddsws(__vector4 a0, __vector4 a1) {
  return __vaddsws(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddubm(
// CHECK: add <16 x i8>
__vector4 test__vaddubm(__vector4 a0, __vector4 a1) {
  return __vaddubm(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vaddubs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vaddubs(
__vector4 test__vaddubs(__vector4 a0, __vector4 a1) {
  return __vaddubs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vadduhm(
// CHECK: add <8 x i16>
__vector4 test__vadduhm(__vector4 a0, __vector4 a1) {
  return __vadduhm(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vadduhs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vadduhs(
__vector4 test__vadduhs(__vector4 a0, __vector4 a1) {
  return __vadduhs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vadduwm(
// CHECK: add <4 x i32>
__vector4 test__vadduwm(__vector4 a0, __vector4 a1) {
  return __vadduwm(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vadduws(
// CHECK: call {{.*}} @llvm.ppc.altivec.vadduws(
__vector4 test__vadduws(__vector4 a0, __vector4 a1) {
  return __vadduws(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vand(
// CHECK: call {{.*}} @llvm.ppc.altivec.vand128(
__vector4 test__vand(__vector4 a0, __vector4 a1) {
  return __vand(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vandc(
// CHECK: call {{.*}} @llvm.ppc.altivec.vandc128(
__vector4 test__vandc(__vector4 a0, __vector4 a1) {
  return __vandc(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vavgsb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vavgsb(
__vector4 test__vavgsb(__vector4 a0, __vector4 a1) {
  return __vavgsb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vavgsh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vavgsh(
__vector4 test__vavgsh(__vector4 a0, __vector4 a1) {
  return __vavgsh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vavgsw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vavgsw(
__vector4 test__vavgsw(__vector4 a0, __vector4 a1) {
  return __vavgsw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vavgub(
// CHECK: call {{.*}} @llvm.ppc.altivec.vavgub(
__vector4 test__vavgub(__vector4 a0, __vector4 a1) {
  return __vavgub(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vavguh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vavguh(
__vector4 test__vavguh(__vector4 a0, __vector4 a1) {
  return __vavguh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vavguw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vavguw(
__vector4 test__vavguw(__vector4 a0, __vector4 a1) {
  return __vavguw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcfsx(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcsxwfp128(
__vector4 test__vcfsx(__vector4 a0) {
  return __vcfsx(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vcfux(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcuxwfp128(
__vector4 test__vcfux(__vector4 a0) {
  return __vcfux(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vcmpbfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpbfp128(
__vector4 test__vcmpbfp(__vector4 a0, __vector4 a1) {
  return __vcmpbfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpeqfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpeqfp128(
__vector4 test__vcmpeqfp(__vector4 a0, __vector4 a1) {
  return __vcmpeqfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpequb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpequb(
__vector4 test__vcmpequb(__vector4 a0, __vector4 a1) {
  return __vcmpequb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpequh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpequh(
__vector4 test__vcmpequh(__vector4 a0, __vector4 a1) {
  return __vcmpequh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpequw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpequw128(
__vector4 test__vcmpequw(__vector4 a0, __vector4 a1) {
  return __vcmpequw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgefp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgefp128(
__vector4 test__vcmpgefp(__vector4 a0, __vector4 a1) {
  return __vcmpgefp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtfp128(
__vector4 test__vcmpgtfp(__vector4 a0, __vector4 a1) {
  return __vcmpgtfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtsb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtsb(
__vector4 test__vcmpgtsb(__vector4 a0, __vector4 a1) {
  return __vcmpgtsb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtsh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtsh(
__vector4 test__vcmpgtsh(__vector4 a0, __vector4 a1) {
  return __vcmpgtsh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtsw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtsw(
__vector4 test__vcmpgtsw(__vector4 a0, __vector4 a1) {
  return __vcmpgtsw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtub(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtub(
__vector4 test__vcmpgtub(__vector4 a0, __vector4 a1) {
  return __vcmpgtub(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtuh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtuh(
__vector4 test__vcmpgtuh(__vector4 a0, __vector4 a1) {
  return __vcmpgtuh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtuw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtuw(
__vector4 test__vcmpgtuw(__vector4 a0, __vector4 a1) {
  return __vcmpgtuw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vcmpbfpR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpbfp128.rec(
__vector4 test__vcmpbfpR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpbfpR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpeqfpR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpeqfp128.rec(
__vector4 test__vcmpeqfpR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpeqfpR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpequbR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpequb.rec(
__vector4 test__vcmpequbR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpequbR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpequhR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpequh.rec(
__vector4 test__vcmpequhR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpequhR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpequwR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpequw128.rec(
__vector4 test__vcmpequwR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpequwR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgefpR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgefp128.rec(
__vector4 test__vcmpgefpR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgefpR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtfpR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtfp128.rec(
__vector4 test__vcmpgtfpR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtfpR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtsbR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtsb.rec(
__vector4 test__vcmpgtsbR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtsbR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtshR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtsh.rec(
__vector4 test__vcmpgtshR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtshR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtswR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtsw.rec(
__vector4 test__vcmpgtswR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtswR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtubR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtub.rec(
__vector4 test__vcmpgtubR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtubR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtuhR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtuh.rec(
__vector4 test__vcmpgtuhR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtuhR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vcmpgtuwR(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcmpgtuw.rec(
__vector4 test__vcmpgtuwR(__vector4 a0, __vector4 a1, unsigned int * a2) {
  return __vcmpgtuwR(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vctsxs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcfpsxws128(
__vector4 test__vctsxs(__vector4 a0) {
  return __vctsxs(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vctuxs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vcfpuxws128(
__vector4 test__vctuxs(__vector4 a0) {
  return __vctuxs(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vexptefp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vexptefp128(
__vector4 test__vexptefp(__vector4 a0) {
  return __vexptefp(a0);
}

// CHECK-LABEL: define{{.*}} @test__vlogefp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vlogefp128(
__vector4 test__vlogefp(__vector4 a0) {
  return __vlogefp(a0);
}

// CHECK-LABEL: define{{.*}} @test__vmaddfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaddfp128(
__vector4 test__vmaddfp(__vector4 a0, __vector4 a1, __vector4 a2) {
  return __vmaddfp(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vmaxfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxfp128(
__vector4 test__vmaxfp(__vector4 a0, __vector4 a1) {
  return __vmaxfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmaxsb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxsb(
__vector4 test__vmaxsb(__vector4 a0, __vector4 a1) {
  return __vmaxsb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmaxsh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxsh(
__vector4 test__vmaxsh(__vector4 a0, __vector4 a1) {
  return __vmaxsh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmaxsw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxsw(
__vector4 test__vmaxsw(__vector4 a0, __vector4 a1) {
  return __vmaxsw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmaxub(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxub(
__vector4 test__vmaxub(__vector4 a0, __vector4 a1) {
  return __vmaxub(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmaxuh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxuh(
__vector4 test__vmaxuh(__vector4 a0, __vector4 a1) {
  return __vmaxuh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmaxuw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmaxuw(
__vector4 test__vmaxuw(__vector4 a0, __vector4 a1) {
  return __vmaxuw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminfp128(
__vector4 test__vminfp(__vector4 a0, __vector4 a1) {
  return __vminfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminsb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminsb(
__vector4 test__vminsb(__vector4 a0, __vector4 a1) {
  return __vminsb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminsh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminsh(
__vector4 test__vminsh(__vector4 a0, __vector4 a1) {
  return __vminsh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminsw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminsw(
__vector4 test__vminsw(__vector4 a0, __vector4 a1) {
  return __vminsw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminub(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminub(
__vector4 test__vminub(__vector4 a0, __vector4 a1) {
  return __vminub(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminuh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminuh(
__vector4 test__vminuh(__vector4 a0, __vector4 a1) {
  return __vminuh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vminuw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vminuw(
__vector4 test__vminuw(__vector4 a0, __vector4 a1) {
  return __vminuw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmrghb(
// CHECK: shufflevector <16 x i8>
__vector4 test__vmrghb(__vector4 a0, __vector4 a1) {
  return __vmrghb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmrghh(
// CHECK: shufflevector <8 x i16>
__vector4 test__vmrghh(__vector4 a0, __vector4 a1) {
  return __vmrghh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmrghw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmrghw128(
__vector4 test__vmrghw(__vector4 a0, __vector4 a1) {
  return __vmrghw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmrglb(
// CHECK: shufflevector <16 x i8>
__vector4 test__vmrglb(__vector4 a0, __vector4 a1) {
  return __vmrglb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmrglh(
// CHECK: shufflevector <8 x i16>
__vector4 test__vmrglh(__vector4 a0, __vector4 a1) {
  return __vmrglh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmrglw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmrglw128(
__vector4 test__vmrglw(__vector4 a0, __vector4 a1) {
  return __vmrglw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vnmsubfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vnmsubfp128(
__vector4 test__vnmsubfp(__vector4 a0, __vector4 a1, __vector4 a2) {
  return __vnmsubfp(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vnor(
// CHECK: call {{.*}} @llvm.ppc.altivec.vnor128(
__vector4 test__vnor(__vector4 a0, __vector4 a1) {
  return __vnor(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vor(
// CHECK: call {{.*}} @llvm.ppc.altivec.vor128(
__vector4 test__vor(__vector4 a0, __vector4 a1) {
  return __vor(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vperm(
// CHECK: call {{.*}} @llvm.ppc.altivec.vperm128(
__vector4 test__vperm(__vector4 a0, __vector4 a1, __vector4 a2) {
  return __vperm(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vpkpx(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkpx(
__vector4 test__vpkpx(__vector4 a0, __vector4 a1) {
  return __vpkpx(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkshss(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkshss128(
__vector4 test__vpkshss(__vector4 a0, __vector4 a1) {
  return __vpkshss(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkshus(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkshus128(
__vector4 test__vpkshus(__vector4 a0, __vector4 a1) {
  return __vpkshus(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkswss(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkswss128(
__vector4 test__vpkswss(__vector4 a0, __vector4 a1) {
  return __vpkswss(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkswus(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkswus128(
__vector4 test__vpkswus(__vector4 a0, __vector4 a1) {
  return __vpkswus(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkuhum(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkuhum128(
__vector4 test__vpkuhum(__vector4 a0, __vector4 a1) {
  return __vpkuhum(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkuhus(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkuhus128(
__vector4 test__vpkuhus(__vector4 a0, __vector4 a1) {
  return __vpkuhus(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkuwum(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkuwum128(
__vector4 test__vpkuwum(__vector4 a0, __vector4 a1) {
  return __vpkuwum(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpkuwus(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkuwus128(
__vector4 test__vpkuwus(__vector4 a0, __vector4 a1) {
  return __vpkuwus(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vrefp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrefp128(
__vector4 test__vrefp(__vector4 a0) {
  return __vrefp(a0);
}

// CHECK-LABEL: define{{.*}} @test__vrfim(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrfim128(
__vector4 test__vrfim(__vector4 a0) {
  return __vrfim(a0);
}

// CHECK-LABEL: define{{.*}} @test__vrfin(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrfin128(
__vector4 test__vrfin(__vector4 a0) {
  return __vrfin(a0);
}

// CHECK-LABEL: define{{.*}} @test__vrfip(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrfip128(
__vector4 test__vrfip(__vector4 a0) {
  return __vrfip(a0);
}

// CHECK-LABEL: define{{.*}} @test__vrfiz(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrfiz128(
__vector4 test__vrfiz(__vector4 a0) {
  return __vrfiz(a0);
}

// CHECK-LABEL: define{{.*}} @test__vrlb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrlb(
__vector4 test__vrlb(__vector4 a0, __vector4 a1) {
  return __vrlb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vrlh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrlh(
__vector4 test__vrlh(__vector4 a0, __vector4 a1) {
  return __vrlh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vrlw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrlw128(
__vector4 test__vrlw(__vector4 a0, __vector4 a1) {
  return __vrlw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vrsqrtefp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrsqrtefp128(
__vector4 test__vrsqrtefp(__vector4 a0) {
  return __vrsqrtefp(a0);
}

// CHECK-LABEL: define{{.*}} @test__vsel(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsel128(
__vector4 test__vsel(__vector4 a0, __vector4 a1, __vector4 a2) {
  return __vsel(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__vsl(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsl(
__vector4 test__vsl(__vector4 a0, __vector4 a1) {
  return __vsl(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vslb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vslb(
__vector4 test__vslb(__vector4 a0, __vector4 a1) {
  return __vslb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsldoi(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsldoi128(
__vector4 test__vsldoi(__vector4 a0, __vector4 a1) {
  return __vsldoi(a0, a1, 0);
}

// CHECK-LABEL: define{{.*}} @test__vslh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vslh(
__vector4 test__vslh(__vector4 a0, __vector4 a1) {
  return __vslh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vslo(
// CHECK: call {{.*}} @llvm.ppc.altivec.vslo128(
__vector4 test__vslo(__vector4 a0, __vector4 a1) {
  return __vslo(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vslw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vslw128(
__vector4 test__vslw(__vector4 a0, __vector4 a1) {
  return __vslw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vspltb(
// CHECK: shufflevector <16 x i8>
__vector4 test__vspltb(__vector4 a0) {
  return __vspltb(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vsplth(
// CHECK: shufflevector <8 x i16>
__vector4 test__vsplth(__vector4 a0) {
  return __vsplth(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vspltisb(
// CHECK: store <16 x i8> zeroinitializer
__vector4 test__vspltisb(void) {
  return __vspltisb(0);
}

// CHECK-LABEL: define{{.*}} @test__vspltish(
// CHECK: store <8 x i16> zeroinitializer
__vector4 test__vspltish(void) {
  return __vspltish(0);
}

// CHECK-LABEL: define{{.*}} @test__vspltisw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vspltisw128(
__vector4 test__vspltisw(void) {
  return __vspltisw(0);
}

// CHECK-LABEL: define{{.*}} @test__vspltw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vspltw128(
__vector4 test__vspltw(__vector4 a0) {
  return __vspltw(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vsr(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsr(
__vector4 test__vsr(__vector4 a0, __vector4 a1) {
  return __vsr(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsrab(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsrab(
__vector4 test__vsrab(__vector4 a0, __vector4 a1) {
  return __vsrab(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsrah(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsrah(
__vector4 test__vsrah(__vector4 a0, __vector4 a1) {
  return __vsrah(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsraw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsraw128(
__vector4 test__vsraw(__vector4 a0, __vector4 a1) {
  return __vsraw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsrb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsrb(
__vector4 test__vsrb(__vector4 a0, __vector4 a1) {
  return __vsrb(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsrh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsrh(
__vector4 test__vsrh(__vector4 a0, __vector4 a1) {
  return __vsrh(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsro(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsro128(
__vector4 test__vsro(__vector4 a0, __vector4 a1) {
  return __vsro(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsrw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsrw128(
__vector4 test__vsrw(__vector4 a0, __vector4 a1) {
  return __vsrw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubcuw(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubcuw(
__vector4 test__vsubcuw(__vector4 a0, __vector4 a1) {
  return __vsubcuw(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubfp128(
__vector4 test__vsubfp(__vector4 a0, __vector4 a1) {
  return __vsubfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubsbs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubsbs(
__vector4 test__vsubsbs(__vector4 a0, __vector4 a1) {
  return __vsubsbs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubshs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubshs(
__vector4 test__vsubshs(__vector4 a0, __vector4 a1) {
  return __vsubshs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubsws(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubsws(
__vector4 test__vsubsws(__vector4 a0, __vector4 a1) {
  return __vsubsws(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsububm(
// CHECK: sub <16 x i8>
__vector4 test__vsububm(__vector4 a0, __vector4 a1) {
  return __vsububm(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsububs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsububs(
__vector4 test__vsububs(__vector4 a0, __vector4 a1) {
  return __vsububs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubuhm(
// CHECK: sub <8 x i16>
__vector4 test__vsubuhm(__vector4 a0, __vector4 a1) {
  return __vsubuhm(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubuhs(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubuhs(
__vector4 test__vsubuhs(__vector4 a0, __vector4 a1) {
  return __vsubuhs(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubuwm(
// CHECK: sub <4 x i32>
__vector4 test__vsubuwm(__vector4 a0, __vector4 a1) {
  return __vsubuwm(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vsubuws(
// CHECK: call {{.*}} @llvm.ppc.altivec.vsubuws(
__vector4 test__vsubuws(__vector4 a0, __vector4 a1) {
  return __vsubuws(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vupkhpx(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupkhpx(
__vector4 test__vupkhpx(__vector4 a0) {
  return __vupkhpx(a0);
}

// CHECK-LABEL: define{{.*}} @test__vupkhsb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupkhsb128(
__vector4 test__vupkhsb(__vector4 a0) {
  return __vupkhsb(a0);
}

// CHECK-LABEL: define{{.*}} @test__vupkhsh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupkhsh(
__vector4 test__vupkhsh(__vector4 a0) {
  return __vupkhsh(a0);
}

// CHECK-LABEL: define{{.*}} @test__vupklpx(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupklpx(
__vector4 test__vupklpx(__vector4 a0) {
  return __vupklpx(a0);
}

// CHECK-LABEL: define{{.*}} @test__vupklsb(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupklsb128(
__vector4 test__vupklsb(__vector4 a0) {
  return __vupklsb(a0);
}

// CHECK-LABEL: define{{.*}} @test__vupklsh(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupklsh(
__vector4 test__vupklsh(__vector4 a0) {
  return __vupklsh(a0);
}

// CHECK-LABEL: define{{.*}} @test__vxor(
// CHECK: call {{.*}} @llvm.ppc.altivec.vxor128(
__vector4 test__vxor(__vector4 a0, __vector4 a1) {
  return __vxor(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmsum3fp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmsum3fp128(
__vector4 test__vmsum3fp(__vector4 a0, __vector4 a1) {
  return __vmsum3fp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmsum4fp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmsum4fp128(
__vector4 test__vmsum4fp(__vector4 a0, __vector4 a1) {
  return __vmsum4fp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vmulfp(
// CHECK: call {{.*}} @llvm.ppc.altivec.vmulfp128(
__vector4 test__vmulfp(__vector4 a0, __vector4 a1) {
  return __vmulfp(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__vpermwi(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpermwi128(
__vector4 test__vpermwi(__vector4 a0) {
  return __vpermwi(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vrlimi(
// CHECK: call {{.*}} @llvm.ppc.altivec.vrlimi128(
__vector4 test__vrlimi(__vector4 a0, __vector4 a1) {
  return __vrlimi(a0, a1, 0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vpkd3d(
// CHECK: call {{.*}} @llvm.ppc.altivec.vpkd3d128(
__vector4 test__vpkd3d(__vector4 a0, __vector4 a1) {
  return __vpkd3d(a0, a1, 0, 0, 0);
}

// CHECK-LABEL: define{{.*}} @test__vupkd3d(
// CHECK: call {{.*}} @llvm.ppc.altivec.vupkd3d128(
__vector4 test__vupkd3d(__vector4 a0) {
  return __vupkd3d(a0, 0);
}

// CHECK-LABEL: define{{.*}} @test__lvlx(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvlx128(
__vector4 test__lvlx(const void * a0, int a1) {
  return __lvlx(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvrx(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvrx128(
__vector4 test__lvrx(const void * a0, int a1) {
  return __lvrx(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__stvlx(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvlx128(
void test__stvlx(__vector4 a0, void * a1, int a2) {
  __stvlx(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__stvrx(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvrx128(
void test__stvrx(__vector4 a0, void * a1, int a2) {
  __stvrx(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__lvlx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvlx128.volatile(
__vector4 test__lvlx_volatile(const volatile void * a0, int a1) {
  return __lvlx_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__lvrx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.lvrx128.volatile(
__vector4 test__lvrx_volatile(const volatile void * a0, int a1) {
  return __lvrx_volatile(a0, a1);
}

// CHECK-LABEL: define{{.*}} @test__stvlx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvlx128.volatile(
void test__stvlx_volatile(__vector4 a0, volatile void * a1, int a2) {
  __stvlx_volatile(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__stvrx_volatile(
// CHECK: call {{.*}} @llvm.ppc.altivec.stvrx128.volatile(
void test__stvrx_volatile(__vector4 a0, volatile void * a1, int a2) {
  __stvrx_volatile(a0, a1, a2);
}

// CHECK-LABEL: define{{.*}} @test__VMXGetReg(
// CHECK: call {{.*}} @llvm.ppc.altivec.get.vr128(
__vector4 test__VMXGetReg(void) {
  return __VMXGetReg(0);
}

// CHECK-LABEL: define{{.*}} @test__VMXSetReg(
// CHECK: call {{.*}} @llvm.ppc.altivec.set.vr128(
void test__VMXSetReg(__vector4 a1) {
  __VMXSetReg(0, a1);
}

#ifdef __cplusplus
}
#endif

// CHECK-NOT: declare {{.*}} @__

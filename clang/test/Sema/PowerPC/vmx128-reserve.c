// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -target-feature +vmx128-reserve -verify=common,reserve %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -verify=common %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -target-feature +vmx128-reserve -target-feature -vmx128-reserve -verify=common %s

typedef int v4i __attribute__((vector_size(16)));
typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;
__vector4 __VMXGetReg(int);
void __VMXSetReg(int, __vector4);

void ranges(v4i v, __vector4 x, int n) {
  (void)__builtin_altivec_get_vr128(0); // reserve-error {{outside the valid range [64, 95]}}
  (void)__builtin_altivec_get_vr128(63); // reserve-error {{outside the valid range [64, 95]}}
  (void)__builtin_altivec_get_vr128(64);
  (void)__builtin_altivec_get_vr128(95);
  (void)__builtin_altivec_get_vr128(96); // reserve-error {{outside the valid range [64, 95]}}
  (void)__builtin_altivec_get_vr128(127); // reserve-error {{outside the valid range [64, 95]}}
  __builtin_altivec_set_vr128(v, 0); // reserve-error {{outside the valid range [64, 95]}}
  __builtin_altivec_set_vr128(v, 63); // reserve-error {{outside the valid range [64, 95]}}
  __builtin_altivec_set_vr128(v, 64);
  __builtin_altivec_set_vr128(v, 95);
  __builtin_altivec_set_vr128(v, 96); // reserve-error {{outside the valid range [64, 95]}}
  __builtin_altivec_set_vr128(v, 127); // reserve-error {{outside the valid range [64, 95]}}
  (void)__VMXGetReg(0); // reserve-error {{outside the valid range [64, 95]}}
  (void)__VMXGetReg(63); // reserve-error {{outside the valid range [64, 95]}}
  (void)__VMXGetReg(64);
  (void)__VMXGetReg(95);
  (void)__VMXGetReg(96); // reserve-error {{outside the valid range [64, 95]}}
  (void)__VMXGetReg(127); // reserve-error {{outside the valid range [64, 95]}}
  __VMXSetReg(0, x); // reserve-error {{outside the valid range [64, 95]}}
  __VMXSetReg(63, x); // reserve-error {{outside the valid range [64, 95]}}
  __VMXSetReg(64, x);
  __VMXSetReg(95, x);
  __VMXSetReg(96, x); // reserve-error {{outside the valid range [64, 95]}}
  __VMXSetReg(127, x); // reserve-error {{outside the valid range [64, 95]}}
  (void)__VMXGetReg(n); // common-error {{must be a constant integer}}
  __builtin_altivec_set_vr128(v, n); // common-error {{must be a constant integer}}
}

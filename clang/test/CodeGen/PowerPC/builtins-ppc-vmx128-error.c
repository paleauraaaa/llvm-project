// REQUIRES: powerpc-registered-target
// RUN: %clang_cc1 -DTEST_FEATURE -triple powerpc-unknown-linux-gnu -target-cpu pwr8 \
// RUN:   -emit-llvm-only -verify=feature %s
// RUN: %clang_cc1 -DTEST_IMMEDIATE -triple powerpc-unknown-linux-gnu -target-cpu g5 -target-feature +vmx128 \
// RUN:   -fsyntax-only -verify=immediate %s
// RUN: %clang_cc1 -fms-extensions -DTEST_MS_NAME -triple powerpc-unknown-linux-gnu \
// RUN:   -target-cpu g5 -target-feature +vmx128 -fsyntax-only -verify=ms %s

typedef __vector signed int v4i32;

#ifdef TEST_FEATURE
void test_feature_disabled(v4i32 a, v4i32 b) {
  (void)__builtin_altivec_vand128(a, b); // feature-error {{'__builtin_altivec_vand128' needs target feature vmx128}}
  (void)__builtin_altivec_get_vr128(0); // feature-error {{'__builtin_altivec_get_vr128' needs target feature vmx128}}
}
#endif

#ifdef TEST_IMMEDIATE
void test_nonconstant_immediate(v4i32 a, int n) {
  (void)__builtin_altivec_vpermwi128(a, n); // immediate-error {{argument to '__builtin_altivec_vpermwi128' must be a constant integer}}
  (void)__builtin_altivec_get_vr128(n); // immediate-error {{argument to '__builtin_altivec_get_vr128' must be a constant integer}}
}

void test_invalid_register(v4i32 a) {
  (void)__builtin_altivec_get_vr128(128); // immediate-error {{argument value 128 is outside the valid range [0, 127]}}
  __builtin_altivec_set_vr128(a, -1); // immediate-error {{argument value -1 is outside the valid range [0, 127]}}
}
#endif

#ifdef TEST_MS_NAME
void test_ms_name_is_not_implicit(void) {
  (void)__vand; // ms-error {{use of undeclared identifier '__vand'}}
}
#endif

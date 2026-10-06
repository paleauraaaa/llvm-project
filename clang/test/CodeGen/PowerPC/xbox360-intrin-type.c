// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -target-cpu xenon -fms-extensions -emit-llvm -o - %s | FileCheck %s

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;

struct Plain {
  int a;
};

// The intrinsic type is passed and returned directly as <4 x float>.
// CHECK-LABEL: define dso_local <4 x float> @add(
// CHECK-SAME: <4 x float> %a.coerce, <4 x float> %b.coerce, i32 noundef signext %n)
__vector4 add(__vector4 a, __vector4 b, int n) {
  __vector4 r = a;
  r.v[0] += b.v[0];
  return r;
}

// CHECK-LABEL: define dso_local signext i32 @use(
// CHECK: call <4 x float> @add(<4 x float> %{{.*}}, <4 x float> %{{.*}}, i32 noundef signext %{{.*}})
int use(int x) {
  __vector4 a = {{0, 0, 0, 0}};
  __vector4 b = add(a, a, x);
  return (int)b.v[1];
}

// Other aggregates keep the default classification.
// CHECK-LABEL: define dso_local signext i32 @plain(
// CHECK-SAME: ptr noundef byval(%struct.Plain) align 8 %p, i32 noundef signext %y)
int plain(struct Plain p, int y) { return p.a + y; }
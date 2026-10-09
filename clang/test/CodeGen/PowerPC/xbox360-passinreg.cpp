// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O0 -mframe-pointer=all -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;

struct __declspec(passinreg) PassInRegVector {
  __vector4 value;

  PassInRegVector copy();
};

PassInRegVector PassInRegVector::copy() { return *this; }

struct IntrinsicMethods {
  static __vector4 static_vector();
  __vector4 instance_vector();
};

__vector4 IntrinsicMethods::static_vector() { return {}; }
__vector4 IntrinsicMethods::instance_vector() { return {}; }

struct SmallWithDtor {
  int value;
  ~SmallWithDtor() {}
};

SmallWithDtor return_small_with_dtor() { return {}; }

typedef int vector4i __attribute__((vector_size(16)));

struct TwoVectors {
  vector4i first;
  vector4i second;
};

struct __declspec(align(16)) AlignedChars {
  char bytes[16];
};

extern "C" int take_two_vectors(TwoVectors value) { return value.first[0]; }
extern "C" int take_aligned_chars(AlignedChars value) {
  return value.bytes[0];
}

extern "C" int call_two_vectors(TwoVectors value) {
  return take_two_vectors(value);
}

extern "C" int call_aligned_chars(AlignedChars value) {
  return take_aligned_chars(value);
}

// passinreg overrides the Microsoft C++ ABI's default indirect instance return.
// CHECK-LABEL: define dso_local {{.*}}<4 x float> @"?copy@PassInRegVector@@
// CHECK-SAME: (ptr noundef nonnull
// CHECK: ret <4 x float>

// A static intrinsic-vector method returns in the vector register. An instance
// method retains the Xbox indirect-return rule and places sret before this.
// CHECK-LABEL: define dso_local {{.*}}<4 x float> @"?static_vector@IntrinsicMethods@@SA
// CHECK-SAME: ()
// CHECK: ret <4 x float>
// CHECK-LABEL: define dso_local void @"?instance_vector@IntrinsicMethods@@QAA
// CHECK-SAME: sret(
// CHECK-SAME: ptr noundef nonnull
// CHECK: ret void

// Nontrivial small aggregates still use the Microsoft C++ indirect return.
// CHECK-LABEL: define dso_local void @"?return_small_with_dtor@@YA
// CHECK-SAME: sret(%struct.SmallWithDtor)

// Aggregates containing vectors and explicitly over-aligned aggregates use
// the ABI's 8-byte argument alignment, while their local copies retain the
// type's natural 16-byte alignment.
// CHECK-LABEL: define dso_local signext i32 @take_two_vectors(ptr noundef byval(%struct.TwoVectors) align 8
// CHECK: alloca %struct.TwoVectors, align 16
// CHECK-LABEL: define dso_local signext i32 @take_aligned_chars(ptr noundef byval(%struct.AlignedChars) align 8
// CHECK: alloca %struct.AlignedChars, align 16
// CHECK-LABEL: define dso_local signext i32 @call_two_vectors(ptr noundef byval(%struct.TwoVectors) align 8
// CHECK: call signext i32 @take_two_vectors(ptr noundef byval(%struct.TwoVectors) align 8
// CHECK-LABEL: define dso_local signext i32 @call_aligned_chars(ptr noundef byval(%struct.AlignedChars) align 8
// CHECK: call signext i32 @take_aligned_chars(ptr noundef byval(%struct.AlignedChars) align 8

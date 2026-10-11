// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O0 -mllvm -fast-isel=false -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -fms-extensions -target-cpu xenon -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s

typedef float Vector __attribute__((vector_size(16)));
struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float data[4];
};
struct __declspec(passinreg) DirectVector { __vector4 value; };
struct Bytes { unsigned char data[9]; };
struct IntegerMethod { virtual unsigned long long dispatch(unsigned); };
struct VectorMethod { virtual Vector dispatch(Vector, double); };
struct AggregateMethod { virtual Bytes dispatch(unsigned); };
struct DirectMethod { virtual DirectVector dispatch(Vector); };

typedef unsigned long long (IntegerMethod::*IntegerPointer)(unsigned);
typedef Vector (VectorMethod::*VectorPointer)(Vector, double);
typedef Bytes (AggregateMethod::*AggregatePointer)(unsigned);
typedef DirectVector (DirectMethod::*DirectPointer)(Vector);

IntegerPointer integer_pointer() { return &IntegerMethod::dispatch; }
// CHECK-LABEL: define linkonce_odr noundef i64 @"??_9IntegerMethod@@
// CHECK-SAME: (ptr {{[^,]*}} %this, ...)
// CHECK: musttail call noundef i64 (ptr, ...) %{{[^ (]+}}(ptr {{[^,]*}}, ...)
// CHECK-NEXT: ret i64

VectorPointer vector_pointer() { return &VectorMethod::dispatch; }
// CHECK-LABEL: define linkonce_odr void @"??_9VectorMethod@@
// CHECK-SAME: (ptr {{[^,]*}}sret(<4 x float>) align 16 %agg.result, ptr {{[^,]*}} %this, ...)
// CHECK: musttail call void (ptr, ptr, ...) %{{[^ (]+}}(ptr {{[^,]*}}sret(<4 x float>) align 16 %agg.result, ptr {{[^,]*}}, ...)
// CHECK-NEXT: ret void

AggregatePointer aggregate_pointer() { return &AggregateMethod::dispatch; }
// CHECK-LABEL: define linkonce_odr void @"??_9AggregateMethod@@
// CHECK-SAME: (ptr {{[^,]*}}sret(%struct.Bytes) align 1 %agg.result, ptr {{[^,]*}} %this, ...)
// CHECK: load ptr, ptr %this
// CHECK: musttail call void (ptr, ptr, ...) %{{[^ (]+}}(ptr {{[^,]*}}sret(%struct.Bytes) align 1 %agg.result, ptr {{[^,]*}}, ...)
// CHECK-NEXT: ret void

DirectPointer direct_pointer() { return &DirectMethod::dispatch; }
// CHECK-LABEL: define linkonce_odr {{.*}}<4 x float> @"??_9DirectMethod@@
// CHECK-SAME: (ptr {{[^,]*}} %this, ...)
// CHECK: musttail call {{.*}}<4 x float> (ptr, ...) %{{[^ (]+}}(ptr {{[^,]*}}, ...)
// CHECK-NEXT: ret <4 x float>

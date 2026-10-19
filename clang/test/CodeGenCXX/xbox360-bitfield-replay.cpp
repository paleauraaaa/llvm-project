// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -include %s -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -emit-pch -x c++-header %s -o %t.pch
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -mrelocation-model static -include-pch %t.pch -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -include-pch %t.pch -ast-dump-all -ast-dump-filter Switch %s | FileCheck %s --check-prefix=AST

#ifndef REPLAY_PATTERNS
#define REPLAY_PATTERNS

#pragma bitfield_order(msb_to_lsb)
template<class T> struct Switch {
  unsigned a:3;
#pragma reverse_bitfield(on)
  unsigned b:5;
};
// AST: FieldDecl {{.*}} a 'unsigned int'
// AST: PragmaBitfieldOrderDecl {{.*}} set lsb_to_msb
// AST: FieldDecl {{.*}} b 'unsigned int'

template<class T> struct Push {
#pragma bitfield_order(push, body, lsb_to_msb)
  unsigned a:3; unsigned b:5;
};
#pragma bitfield_order(pop, body)

// A definition-time pop succeeds without consuming the instantiation's entry.
#pragma bitfield_order(push, caller)
template<class T> struct Pop {
#pragma bitfield_order(pop, caller)
  unsigned a:3; unsigned b:5;
};

template<class T> struct Outer {
  struct Inner {
#pragma bitfield_order(lsb_to_msb)
    unsigned a:3; unsigned b:5;
  };
  unsigned a:3; unsigned b:5;
};

template<class T> struct Forced {
  struct Inner {
#pragma bitfield_order(lsb_to_msb)
    unsigned a:3; unsigned b:5;
  };
  Inner inner;
  unsigned a:3; unsigned b:5;
};

template<class T> unsigned function() {
  struct Before { unsigned a:3; unsigned b:5; };
  static Before before = {5,17};
#pragma bitfield_order(lsb_to_msb)
  struct After { unsigned a:3; unsigned b:5; };
  static After after = {5,17};
  return before.b + after.b;
}

template<class T> struct Methods {
  static unsigned run() {
#pragma bitfield_order(lsb_to_msb)
    struct Local { unsigned a:3; unsigned b:5; };
    static Local local = {5,17};
    return local.b;
  }
  unsigned a:3; unsigned b:5;
};

#pragma bitfield_order(msb_to_lsb)
#else

#define SNAP(N) struct S_##N { unsigned a:3; unsigned b:5; }; \
  extern "C" S_##N N = {5,17}

#pragma bitfield_order(msb_to_lsb)
extern "C" Switch<int> first = {5,17};
// CHECK-DAG: @first = {{.*}}i8 -96, {{.*}}i8 -120
SNAP(after_first);
// CHECK-DAG: @after_first = {{.*}}i8 -115
#pragma bitfield_order(msb_to_lsb)
extern "C" Switch<int> reused = {5,17};
SNAP(after_reuse);
// CHECK-DAG: @reused = {{.*}}i8 -96, {{.*}}i8 -120
// CHECK-DAG: @after_reuse = {{.*}}i8 -79
extern "C" Switch<char> second = {5,17};
SNAP(after_second);
// CHECK-DAG: @after_second = {{.*}}i8 -115

#pragma bitfield_order(msb_to_lsb)
extern "C" Push<int> pushed = {5,17};
SNAP(after_push);
#pragma bitfield_order(pop, body)
SNAP(after_pop);
// CHECK-DAG: @after_push = {{.*}}i8 -115
// CHECK-DAG: @after_pop = {{.*}}i8 -79

#pragma bitfield_order(lsb_to_msb)
#pragma bitfield_order(push, caller, msb_to_lsb)
extern "C" Pop<int> popped = {5,17};
SNAP(after_caller_pop);
// CHECK-DAG: @popped = {{.*}}i8 -115
// CHECK-DAG: @after_caller_pop = {{.*}}i8 -115

#pragma bitfield_order(msb_to_lsb)
unsigned outer_size = sizeof(Outer<int>);
SNAP(after_outer);
// CHECK-DAG: @after_outer = {{.*}}i8 -79
extern "C" Outer<int>::Inner inner = {5,17};
SNAP(after_inner);
// CHECK-DAG: @inner = {{.*}}i8 -115
// CHECK-DAG: @after_inner = {{.*}}i8 -115

#pragma bitfield_order(msb_to_lsb)
extern "C" Forced<int> forced = {{5,17},5,17};
SNAP(after_forced);
// CHECK-DAG: @forced = {{.*}}i8 -115{{.*}}i8 -115
// CHECK-DAG: @after_forced = {{.*}}i8 -115

#pragma bitfield_order(msb_to_lsb)
extern "C" Methods<int> method_object = {5,17};
unsigned (*method)() = &Methods<int>::run;
SNAP(after_method_request);
// CHECK-DAG: @method_object = {{.*}}i8 -79
// CHECK-DAG: @after_method_request = {{.*}}i8 -79

unsigned (*function_first)() = &function<int>;
unsigned (*function_second)() = &function<char>;
SNAP(after_function_request);
// CHECK-DAG: @after_function_request = {{.*}}i8 -79

// Deferred bodies run against the live stack, without restoring it afterward.
// CHECK-DAG: @"?local@{{.*}} = {{.*}}i8 -115
// CHECK-DAG: @"?before@{{.*}}function@H{{.*}} = {{.*}}i8 -115
// CHECK-DAG: @"?after@{{.*}}function@H{{.*}} = {{.*}}i8 -115
// CHECK-DAG: @"?before@{{.*}}function@D{{.*}} = {{.*}}i8 -115
// CHECK-DAG: @"?after@{{.*}}function@D{{.*}} = {{.*}}i8 -115
#endif

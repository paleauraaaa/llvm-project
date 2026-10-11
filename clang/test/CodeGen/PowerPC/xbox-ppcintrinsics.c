// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -x c++ -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fno-builtin -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=ZERO
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O0 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O2 -S -mllvm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ASM
#include "Inputs/xbox-ppcintrinsics.h"
#ifdef __cplusplus
extern "C" {
#endif

#define UPDATE_LOAD(NAME, TYPE) \
TYPE test_##NAME(void **out, int offset) { \
  void *base = *out; \
  TYPE result = NAME(offset, base); \
  *out = base; \
  return result; \
}
UPDATE_LOAD(__loadbyteupdate, unsigned char)
UPDATE_LOAD(__loadshortupdate, unsigned short)
UPDATE_LOAD(__loadwordupdate, unsigned long)
UPDATE_LOAD(__loadfloatupdate, float)
UPDATE_LOAD(__loaddoubleupdate, double)
UPDATE_LOAD(__loaddoublewordupdate, unsigned long long)
// CHECK-LABEL: define{{.*}} @test___loadbyteupdate(
// CHECK: call { i32, ptr } asm sideeffect "lbzux $0, $1, $3", "=&r,=b,1,r,~{memory}"
// CHECK: extractvalue { i32, ptr } {{.*}}, 1
// CHECK: store ptr
// ASM-LABEL: test___loadbyteupdate:
// ASM: lbzux
// ASM-LABEL: test___loadshortupdate:
// ASM: lhzux
// ASM-LABEL: test___loadwordupdate:
// ASM: lwzux
// ASM-LABEL: test___loadfloatupdate:
// ASM: lfsux
// ASM-LABEL: test___loaddoubleupdate:
// ASM: lfdux
// ASM-LABEL: test___loaddoublewordupdate:
// ASM: ldux

#define UPDATE_STORE(NAME, TYPE) \
void test_##NAME(void **out, int offset, TYPE value) { \
  void *base = *out; \
  NAME(value, offset, base); \
  *out = base; \
}
UPDATE_STORE(__storebyteupdate, unsigned char)
UPDATE_STORE(__storeshortupdate, unsigned short)
UPDATE_STORE(__storewordupdate, unsigned long)
UPDATE_STORE(__storefloatupdate, float)
UPDATE_STORE(__storedoubleupdate, double)
UPDATE_STORE(__storedoublewordupdate, unsigned long long)
// ASM-LABEL: test___storebyteupdate:
// ASM: stbux
// ASM-LABEL: test___storeshortupdate:
// ASM: sthux
// ASM-LABEL: test___storewordupdate:
// ASM: stwux
// ASM-LABEL: test___storefloatupdate:
// ASM: stfsux
// ASM-LABEL: test___storedoubleupdate:
// ASM: stfdux
// ASM-LABEL: test___storedoublewordupdate:
// ASM: stdux

// CHECK-LABEL: define{{.*}} @update_negative(
// CHECK: asm sideeffect "lwzu $0, $3($1)", "=&r,=b,1,i,~{memory}"
// ASM-LABEL: update_negative:
// ASM: lwzu {{[0-9]+}}, -4(
unsigned long update_negative(void **out) {
  void *base = *out;
  unsigned long value = __loadwordupdate(-4, base);
  *out = base;
  return value;
}

// CHECK-LABEL: define{{.*}} @cache(
// CHECK: call void @llvm.ppc.dcbzl
// CHECK: call void @llvm.ppc.dcbz(
// CHECK: call void @llvm.ppc.dcbt.with.hint
// CHECK: call void @llvm.ppc.dcbst
// CHECK: call void @llvm.ppc.dcbf
// ASM-LABEL: cache:
// ASM: dcbzl
// ASM: dcbz
// ASM: dcbt
// ASM: dcbst
// ASM: dcbf
void cache(int offset, void *base) {
  __dcbz128(offset, base);
  __dcbz(offset, base);
  __dcbt(offset, base);
  __dcbst(offset, base);
  __dcbf(offset, base);
}

#define REVERSE_LOAD(NAME, TYPE, POINTER) \
TYPE test_##NAME(int offset, POINTER base) { return NAME(offset, base); }
#define REVERSE_STORE(NAME, TYPE, POINTER) \
void test_##NAME(TYPE value, int offset, POINTER base) { NAME(value, offset, base); }
#define REVERSE(SUFFIX) \
REVERSE_LOAD(__loadshortbytereverse##SUFFIX, unsigned short, const void *) \
REVERSE_LOAD(__loadwordbytereverse##SUFFIX, unsigned long, const void *) \
REVERSE_LOAD(__loaddoublewordbytereverse##SUFFIX, unsigned long long, const void *) \
REVERSE_STORE(__storeshortbytereverse##SUFFIX, unsigned short, void *) \
REVERSE_STORE(__storewordbytereverse##SUFFIX, unsigned int, void *) \
REVERSE_STORE(__storedoublewordbytereverse##SUFFIX, unsigned long long, void *) \
REVERSE_LOAD(__loadvolatileshortbytereverse##SUFFIX, unsigned short, const volatile void *) \
REVERSE_LOAD(__loadvolatilewordbytereverse##SUFFIX, unsigned long, const volatile void *) \
REVERSE_LOAD(__loadvolatiledoublewordbytereverse##SUFFIX, unsigned long long, const void *) \
REVERSE_STORE(__storevolatileshortbytereverse##SUFFIX, unsigned short, volatile void *) \
REVERSE_STORE(__storevolatilewordbytereverse##SUFFIX, unsigned int, volatile void *) \
REVERSE_STORE(__storevolatiledoublewordbytereverse##SUFFIX, unsigned long long, void *)
REVERSE()
REVERSE(G)
// CHECK-LABEL: define{{.*}} @test___loadshortbytereverse(
// CHECK: call i32 @llvm.ppc.load2r
// CHECK-LABEL: define{{.*}} @test___loadvolatileshortbytereverse(
// CHECK: call i32 @llvm.ppc.load2r.volatile
// CHECK-LABEL: define{{.*}} @test___storevolatilewordbytereverse(
// CHECK: call void @llvm.ppc.store4r.volatile
// CHECK-LABEL: define{{.*}} @test___loadvolatiledoublewordbytereverseG(
// CHECK: call i64 @llvm.ppc.load8r.volatile
// CHECK-LABEL: define{{.*}} @test___storevolatiledoublewordbytereverseG(
// CHECK: call void @llvm.ppc.store8r.volatile
// ASM-LABEL: test___loadshortbytereverse:
// ASM: lhbrx
// ASM-LABEL: test___loadwordbytereverse:
// ASM: lwbrx
// ASM-LABEL: test___loaddoublewordbytereverse:
// ASM: ldbrx
// ASM-LABEL: test___storeshortbytereverse:
// ASM: sthbrx
// ASM-LABEL: test___storewordbytereverse:
// ASM: stwbrx
// ASM-LABEL: test___storedoublewordbytereverse:
// ASM: stdbrx

unsigned short swap16(unsigned short value) { return _byteswap_ushort(value); }
unsigned long swap32(unsigned long value) { return _byteswap_ulong(value); }
unsigned long long swap64(unsigned long long value) { return _byteswap_uint64(value); }
long long mul_signed(long long a, long long b) { return __mulh(a, b); }
unsigned long long mul_unsigned(unsigned long long a, unsigned long long b) {
  return __umulh(a, b);
}
// ASM-LABEL: mul_signed:
// ASM: mulhd
// ASM-LABEL: mul_unsigned:
// ASM: mulhdu

#define UNARY(NAME, TYPE) TYPE test_##NAME(TYPE value) { return NAME(value); }
UNARY(__fsqrt, double)
UNARY(__fsqrts, float)
UNARY(__frsqrte, double)
float reciprocal(double value) { return __fres(value); }
double select_double(double a, double b, double c) { return __fsel(a, b, c); }
float select_float(float a, float b, float c) { return __fself(a, b, c); }
UNARY(__fctiwz, double)
UNARY(__fctidz, double)
UNARY(__fctiw, double)
UNARY(__fctid, double)
UNARY(__fcfid, double)
void store_fp_word(double value, int offset, void *base) {
  __stfiwx(value, offset, base);
}
UNARY(__frnd, double)
// CHECK-LABEL: define{{.*}} @reciprocal(
// CHECK: call float asm sideeffect "fres $0, $1", "=f,f,~{memory}"(double
// CHECK-LABEL: define{{.*}} @test___frnd(
// CHECK: call double asm sideeffect "fctid $0, $1\0A\09fcfid $0, $0", "=f,f,~{memory}"
// ASM-LABEL: test___fsqrt:
// ASM: fsqrt
// ASM-LABEL: test___fsqrts:
// ASM: fsqrts
// ASM-LABEL: test___frsqrte:
// ASM: frsqrte
// ASM-LABEL: reciprocal:
// ASM: fres
// ASM-LABEL: select_double:
// ASM: fsel
// ASM-LABEL: select_float:
// ASM: fsel
// ASM-LABEL: test___fctiwz:
// ASM: fctiwz
// ASM-LABEL: test___fctidz:
// ASM: fctidz
// ASM-LABEL: test___fctiw:
// ASM: fctiw
// ASM-LABEL: test___fctid:
// ASM: fctid
// ASM-LABEL: test___fcfid:
// ASM: fcfid
// ASM-LABEL: store_fp_word:
// ASM: stfiwx
// ASM-LABEL: test___frnd:
// ASM: fctid
// ASM: fcfid

unsigned int zeros32(long value) { return _CountLeadingZeros(value); }
unsigned int zeros64(long long value) { return _CountLeadingZeros64(value); }
unsigned int zeros32_zero(void) { return _CountLeadingZeros(0); }
unsigned int zeros64_zero(void) { return _CountLeadingZeros64(0); }
// ZERO-LABEL: define{{.*}} @zeros32_zero(
// ZERO: ret i32 32
// ZERO-LABEL: define{{.*}} @zeros64_zero(
// ZERO: ret i32 64
unsigned long long time_base(void) { return __mftb(); }
// ASM-LABEL: zeros32:
// ASM: cntlzw
// ASM-LABEL: zeros64:
// ASM: cntlzd
// CHECK-LABEL: define{{.*}} @time_base(
// CHECK: call i64 asm sideeffect "mftb $0", "=r"()
// ASM-LABEL: time_base:
// ASM: {{mftb|mfspr [0-9]+, 268}}

void scheduling(void) {
  __SetHWThreadPriorityHigh();
  __SetHWThreadPriorityMed();
  __SetHWThreadPriorityLow();
  __emit(0x7c0004ac);
}
// CHECK-LABEL: define{{.*}} @scheduling(
// CHECK: asm sideeffect "or 3, 3, 3", "~{memory}"
// CHECK: asm sideeffect "or 2, 2, 2", "~{memory}"
// CHECK: asm sideeffect "or 1, 1, 1", "~{memory}"
// CHECK: asm sideeffect ".long $0", "i,~{memory}"
// ASM-LABEL: scheduling:
// ASM: {{mr 3, 3|or 3, 3, 3}}
// ASM: {{mr 2, 2|or 2, 2, 2}}
// ASM: {{mr 1, 1|or 1, 1, 1}}

void traps(int a, int b, long long c, long long d) {
  __tw(4, a, b);
  __twi(16, a, -32768);
  __td(8, c, d);
  __tdi(31, c, 32767);
}
// CHECK-LABEL: define{{.*}} @traps(
// CHECK: call void @llvm.ppc.tw(i32 {{.*}}, i32 {{.*}}, i32 4)
// CHECK: call void @llvm.ppc.tw(i32 {{.*}}, i32 -32768, i32 16)
// CHECK: call void @llvm.ppc.tdw(i64 {{.*}}, i64 {{.*}}, i32 8)
// CHECK: call void @llvm.ppc.tdw(i64 {{.*}}, i64 32767, i32 31)
// ASM-LABEL: traps:
// ASM: tweq {{[0-9]+}}, {{[0-9]+}}
// ASM: twlti {{[0-9]+}}, -32768
// ASM: tdgt {{[0-9]+}}, {{[0-9]+}}
// ASM: tdui {{[0-9]+}}, 32767

double next_double(void);
// CHECK-LABEL: define{{.*}} @sqrt_once(
// CHECK: call{{.*}} double @next_double()
// CHECK-NOT: @next_double
// CHECK: call double @llvm.sqrt.f64
// CHECK-NOT: @next_double
// CHECK: ret double
double sqrt_once(void) { return __fsqrt(next_double()); }

// CHECK-LABEL: define{{.*}} @rounding_mode_reads(
// CHECK: call double asm sideeffect "fctid $0, $1"
// CHECK: call void asm sideeffect ".long $0"
// CHECK: call double asm sideeffect "fctid $0, $1"
// CHECK: ret double
double rounding_mode_reads(double value) {
  double first = __fctid(value);
  __emit(0x7c0004ac);
  return first + __fctid(value);
}

// CHECK-LABEL: define{{.*}} @unused_volatile(
// CHECK: call i32 @llvm.ppc.load2r.volatile
// CHECK: call i32 @llvm.ppc.load2r.volatile
// CHECK: call i32 @llvm.ppc.load4r.volatile
// CHECK: ret void
// ASM-LABEL: unused_volatile:
// ASM: lhbrx
// ASM: lhbrx
// ASM: lwbrx
void unused_volatile(void *base) {
  __loadvolatileshortbytereverse(0, base);
  __loadvolatileshortbytereverse(0, base);
  __loadvolatilewordbytereverse(4, base);
}

void **next_pointer(void);
int next_offset(void);
unsigned long next_value(void);
// CHECK-LABEL: define{{.*}} @update_evaluation(
// CHECK: call{{.*}} i32 @next_value()
// CHECK-NOT: @next_value
// CHECK: call{{.*}} i32 @next_offset()
// CHECK-NOT: @next_offset
// CHECK: call{{.*}} ptr @next_pointer()
// CHECK-NOT: @next_pointer
// CHECK: call ptr asm sideeffect "stwux
// CHECK-NOT: @next_pointer
// CHECK: store ptr
// CHECK-NOT: @next_pointer
// CHECK: ret void
void update_evaluation(void) {
  __storewordupdate(next_value(), next_offset(), *next_pointer());
}

// CHECK-LABEL: define{{.*}} @time_base_twice(
// CHECK: call i64 asm sideeffect "mftb $0"
// CHECK: call i64 asm sideeffect "mftb $0"
// CHECK: ret i64
unsigned long long time_base_twice(void) {
  unsigned long long first = __mftb();
  return __mftb() - first;
}

// CHECK-LABEL: define{{.*}} @doubleword_update_aligned(
// CHECK: asm sideeffect "ldu $0, $3($1)", "=&r,=b,1,i,~{memory}"
// ASM-LABEL: doubleword_update_aligned:
// ASM: ldu {{[0-9]+}}, 32764(
unsigned long long doubleword_update_aligned(void **out) {
  void *base = *out;
  unsigned long long value = __loaddoublewordupdate(32764, base);
  *out = base;
  return value;
}

// CHECK-LABEL: define{{.*}} @doubleword_update_unaligned(
// CHECK: asm sideeffect "ldux $0, $1, $3", "=&r,=b,1,r,~{memory}"
// ASM-LABEL: doubleword_update_unaligned:
// ASM: ldux
unsigned long long doubleword_update_unaligned(void **out) {
  void *base = *out;
  unsigned long long value = __loaddoublewordupdate(2, base);
  *out = base;
  return value;
}

// CHECK-LABEL: define{{.*}} @update_large_offset(
// CHECK: asm sideeffect "lwzux $0, $1, $3", "=&r,=b,1,r,~{memory}"
// ASM-LABEL: update_large_offset:
// ASM: lwzux
unsigned long update_large_offset(void **out) {
  void *base = *out;
  unsigned long value = __loadwordupdate(32768, base);
  *out = base;
  return value;
}

// CHECK-LABEL: define{{.*}} @store_update_negative(
// CHECK: asm sideeffect "stdu $2, $3($0)", "=b,0,r,i,~{memory}"
// ASM-LABEL: store_update_negative:
// ASM: stdu {{[0-9]+}}, -32768(
void store_update_negative(void **out, unsigned long long value) {
  void *base = *out;
  __storedoublewordupdate(value, -32768, base);
  *out = base;
}
#ifdef __cplusplus
}
#endif

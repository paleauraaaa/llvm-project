// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O2 -mllvm -verify-machineinstrs -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -target-cpu xenon -O0 -mllvm -fast-isel=false -mllvm -verify-machineinstrs -emit-obj -o %t.o %s

typedef unsigned long long U64;
typedef long long S64;

U64 byte(unsigned char value) { return value; }
// IR-LABEL: define dso_local i64 @byte(i8 noundef %value)
// ASM-LABEL: byte:
// ASM: clrldi 3, 3, 56
// ASM: blr
U64 half(unsigned short value) { return value; }
// IR-LABEL: define dso_local i64 @half(i16 noundef %value)
// ASM-LABEL: half:
// ASM: clrldi 3, 3, 48
// ASM: blr
U64 word(unsigned value) { return value; }
// IR-LABEL: define dso_local i64 @word(i32 noundef %value)
// ASM-LABEL: word:
// ASM: clrldi 3, 3, 32
// ASM: blr
S64 signed_byte(signed char value) { return value; }
// IR-LABEL: define dso_local i64 @signed_byte(i8 noundef %value)
// ASM-LABEL: signed_byte:
// ASM: extsb 3, 3
S64 signed_half(short value) { return value; }
// IR-LABEL: define dso_local i64 @signed_half(i16 noundef %value)
// ASM-LABEL: signed_half:
// ASM: extsh 3, 3
S64 signed_word(int value) { return value; }
// IR-LABEL: define dso_local i64 @signed_word(i32 noundef %value)
// ASM-LABEL: signed_word:
// ASM: extsw 3, 3

U64 weighted(unsigned char a, unsigned short b, unsigned c,
             unsigned char d, unsigned short e, unsigned f,
             unsigned char g, unsigned short h, unsigned i, unsigned j) {
  return (U64)a + 2ULL*b + 3ULL*c + 4ULL*d + 5ULL*e +
         6ULL*f + 7ULL*g + 8ULL*h + 9ULL*i + 10ULL*j;
}
// IR-LABEL: define dso_local i64 @weighted(i8 noundef %a, i16 noundef %b, i32 noundef %c, i8 noundef %d, i16 noundef %e, i32 noundef %f, i8 noundef %g, i16 noundef %h, i32 noundef %i, i32 noundef %j)
// ASM-LABEL: weighted:
// ASM-DAG: clrldi {{[0-9]+}}, 3, 56
// ASM-DAG: rlwinm {{[0-9]+}}, 4, 1, 15, 30
// ASM-DAG: clrldi {{[0-9]+}}, 5, 32
// ASM-DAG: rlwinm {{[0-9]+}}, 6, 2, 22, 29
// ASM-DAG: clrldi {{[0-9]+}}, 7, 48
// ASM-DAG: clrldi {{[0-9]+}}, 8, 32
// ASM-DAG: clrldi {{[0-9]+}}, 9, 56
// ASM-DAG: rlwinm {{[0-9]+}}, 10, 3, 13, 28

U64 stack_half(U64 a, U64 b, U64 c, U64 d, U64 e, U64 f, U64 g, U64 h,
               unsigned short value) {
  return value;
}
// IR-LABEL: define dso_local i64 @stack_half(
// IR-SAME: i16 noundef %value)
// ASM-LABEL: stack_half:
// ASM: lhz 3, 86(1)
// ASM: blr

typedef U64 (*HalfFunction)(unsigned short);
U64 direct_call(unsigned short value) { return half(value); }
// IR-LABEL: define dso_local i64 @direct_call(i16 noundef %value)
// IR: call i64 @half(i16 noundef
U64 indirect_call(HalfFunction function, unsigned short value) {
  return function(value);
}
// IR-LABEL: define dso_local i64 @indirect_call(ptr noundef %function, i16 noundef %value)
// IR: call i64 %{{[^ (]+}}(i16 noundef

unsigned short return_half(unsigned value) { return value; }
// Return extension remains independent of the argument convention.
// IR-LABEL: define dso_local zeroext i16 @return_half(i32 noundef %value)

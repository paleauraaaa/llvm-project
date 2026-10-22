// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -O2 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -emit-obj -o %t.obj %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -O2 -emit-obj -o %t.opt.obj %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -include-pch %t.pch -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -mrelocation-model static -include-pch %t.pch -emit-obj -o %t.pch.obj %s

#ifndef XBOX_MS_ASM_INCLUDED
#define XBOX_MS_ASM_INCLUDED

// IR-LABEL: define {{.*}} @arithmetic(
// IR: %msasm.operands = alloca [3 x ptr]
// IR: asm sideeffect "lwz 12, 0($1)\0A\09lwz 10, 4($1)\0A\09lwz 9, 0(10)\0A\09lwz 8, 8($1)\0A\09lwz 7, 0(8)\0A\09\09add 11, 9, 7\0A\09stw 11, 0(12)
// IR-SAME: "=&{r3},b,{{.*}}~{memory}"
// ASM-LABEL: arithmetic:
// ASM: #APP
// ASM: lwz 9,
// ASM: lwz 7,
// ASM: add 11, 9, 7
// ASM: stw 11,
// ASM: #NO_APP
int arithmetic(int a, int b) {
  int result;
  __asm { add result, a, b }
  return result;
}

// XDK expands mr to or and loads both source occurrences, even when volatile.
// ASM-LABEL: repeated:
// ASM: #APP
// ASM: lwz 9,
// ASM-NEXT: lwz 8,
// ASM-NEXT: or 11, 9, 8
// ASM: stw 11,
// ASM: lwz 9,
// ASM: addi 11, 9, 3
// ASM: stw 11,
// ASM: #NO_APP
int repeated(volatile int a) {
  int result;
  __asm {
    mr result, a
    addi result, result, 3
  }
  return result;
}

int value;

// The store must occur inside the block, before the aliased indirect load.
// ASM-LABEL: alias_observation:
// ASM: #APP
// ASM: li [[TEMP:[0-9]+]], 7
// ASM-NEXT: stw [[TEMP]],
// ASM: lwz [[TEMP]], 0(11)
// ASM-NEXT: stw [[TEMP]],
// ASM: #NO_APP
int alias_observation(void) {
  int *p = &value;
  int result;
  __asm {
    mr r11, p
    li value, 7
    lwz result, 0(r11)
  }
  return result;
}

// ASM-LABEL: direct_memory:
// ASM: #APP
// ASM: lwz 11,
// ASM: stw 11,
// ASM: #NO_APP
void direct_memory(int a) {
  __asm { stw a, value }
}

// IR-LABEL: define {{.*}} @implicit_return(
// IR: asm sideeffect {{.*}}"=&{r3},~{memory}"
// ASM-LABEL: implicit_return:
// ASM: li 3, 42
// ASM-NOT: li 3, 0
// ASM: blr
int implicit_return(void) {
  __asm li r3, 42
}

// IR-LABEL: define {{.*}} @implicit_pointer(
// IR: asm sideeffect {{.*}}"=&{r3},~{memory}"
int *implicit_pointer(void) {
  __asm li r3, 0
}

// IR-LABEL: define {{.*}} @implicit_float(
// IR: call float asm sideeffect {{.*}}"=&{f1},
float implicit_float(void) {
  __asm fmr fr1, fr2
}

// ASM-LABEL: narrow:
// ASM: #APP
// ASM: lbz 9,
// ASM: addi 11, 9, 1
// ASM: stb 11,
// ASM: #NO_APP
signed char narrow(signed char a) {
  signed char result;
  __asm { addi result, a, 1 }
  return result;
}

// ASM-LABEL: wide:
// ASM: #APP
// ASM: ld 9,
// ASM: addi 11, 9, 1
// ASM: std 11,
// ASM: #NO_APP
long long wide(long long a) {
  long long result;
  __asm { addi result, a, 1 }
  return result;
}

// ASM-LABEL: floats:
// ASM: #APP
// ASM: lfs 12,
// ASM: lfs 11,
// ASM: fadds 13, 12, 11
// ASM: stfs 13,
// ASM: #NO_APP
float floats(float a, float b) {
  float result;
  __asm { fadds result, a, b }
  return result;
}

// ASM-LABEL: doubles:
// ASM: #APP
// ASM: lfd 12,
// ASM: lfd 11,
// ASM: fadd 13, 12, 11
// ASM: stfd 13,
// ASM: #NO_APP
double doubles(double a, double b) {
  double result;
  __asm { fadd result, a, b }
  return result;
}

typedef float vector4 __attribute__((vector_size(16)));

// ASM-LABEL: vectors:
// ASM: #APP
// ASM: lvx128
// ASM: lvx128
// ASM: vaddfp128
// ASM: stvx128
// ASM: #NO_APP
vector4 vectors(vector4 a, vector4 b) {
  vector4 result;
  __asm { vaddfp128 result, a, b }
  return result;
}

// IR-LABEL: define {{.*}} @implicit_vector(
// IR: call <4 x float> asm sideeffect {{.*}}"=&{v1},
vector4 implicit_vector(void) {
  __asm vxor128 vr1, vr1, vr1
}

// ASM-LABEL: high_vector_registers:
// ASM: vxor128 127, 126, 125
void high_vector_registers(void) {
  __asm { vxor128 vr127, vr126, vr125 }
}

// ASM-LABEL: insert:
// ASM: #APP
// ASM: lwz 11,
// ASM: lwz 9,
// ASM: rlwimi 11, 9, 8, 0, 15
// ASM: stw 11,
// ASM: #NO_APP
int insert(int a, int b) {
  int result = b;
  __asm { rlwimi result, a, 8, 0, 15 }
  return result;
}

// IR-LABEL: define {{.*}} @conditions(
// IR: asm sideeffect {{.*}}~{cr0}
int conditions(int a) {
  int result;
  __asm {
    cmpwi a, 0
    beq zero
    li result, 3
    b done
  zero:
    li result, 1
  done:
  }
  return result;
}

// The two emissions of the same asm blob must have different local labels.
int inline_labels(int a) { return conditions(a) + conditions(a + 1); }

// IR-LABEL: define {{.*}} @nonvolatile(
// IR: asm sideeffect {{.*}}~{r14}
void nonvolatile(void) {
  __asm { li r14, 17 }
}

// IR-LABEL: define {{.*}} @multiple_registers(
// IR: asm sideeffect {{.*}}~{r28},~{r29},~{r30},~{r31}
// ASM-LABEL: multiple_registers:
// ASM-DAG: std 28,
// ASM-DAG: std 29,
// ASM-DAG: std 30,
// ASM-DAG: std 31,
// ASM: lmw 28, 0(11)
// ASM-DAG: ld 28,
// ASM-DAG: ld 29,
// ASM-DAG: ld 30,
// ASM-DAG: ld 31,
void multiple_registers(void *p) {
  __asm {
    mr r11, p
    lmw r28, 0(r11)
  }
}

// IR-LABEL: define {{.*}} @cr_mask(
// IR: asm sideeffect {{.*}}~{cr2}
void cr_mask(int a) {
  __asm { mtcrf 32, a }
}

// IR-LABEL: define {{.*}} @link_register(
// IR-NOT: ~{lr}
// IR: asm sideeffect
// IR-NOT: ~{lr}
// ASM-LABEL: link_register:
// ASM-NOT: mflr
// ASM: mtlr
// ASM-NOT: mtlr
// ASM: blr
void link_register(int a) {
  __asm { mtspr 8, a }
}

// The separately spelled fourth source is loaded into the destination scratch.
// ASM-LABEL: madd:
// ASM: #APP
// ASM: lvx128 [[A:[0-9]+]],
// ASM: lvx128 [[B:[0-9]+]],
// ASM: lvx128 [[D:[0-9]+]],
// ASM: vmaddfp128 [[D]], [[A]], [[B]], [[D]]
// ASM: stvx128 [[D]],
// ASM: #NO_APP
vector4 madd(vector4 a, vector4 b, vector4 c) {
  vector4 result;
  __asm { vmaddfp128 result, a, b, c }
  return result;
}

// ASM-LABEL: nmsub:
// ASM: lvx128 [[NA:[0-9]+]],
// ASM: lvx128 [[NB:[0-9]+]],
// ASM: lvx128 [[ND:[0-9]+]],
// ASM: vnmsubfp128 [[ND]], [[NA]], [[NB]], [[ND]]
// ASM: stvx128 [[ND]],
vector4 nmsub(vector4 a, vector4 b, vector4 c) {
  vector4 result;
  __asm { vnmsubfp128 result, a, b, c }
  return result;
}

// ASM-LABEL: perm:
// ASM: vperm128 {{[0-9]+}}, {{[0-9]+}}, {{[0-9]+}}, {{[0-7]}}
vector4 perm(vector4 a, vector4 b, vector4 control) {
  vector4 result;
  __asm { vperm128 result, a, b, control }
  return result;
}

// ASM-LABEL: mixed_tie:
// ASM: vmaddfp128 5, {{[0-9]+}}, {{[0-9]+}}, 5
// ASM: stvx128 5,
vector4 mixed_tie(vector4 a, vector4 b) {
  vector4 result;
  __asm { vmaddfp128 result, a, b, vr5 }
  return result;
}

// ASM-LABEL: physical_tie:
// ASM: lvx128 5,
// ASM: vmaddfp128 5, {{[0-9]+}}, {{[0-9]+}}, 5
void physical_tie(vector4 a, vector4 b, vector4 c) {
  __asm { vmaddfp128 vr5, a, b, c }
}

// ASM-LABEL: uppercase:
// ASM: add 3, 4, 5
int uppercase(void) {
  __asm { ADD R3, R4, R5 }
}

// ASM-LABEL: signed_immediates:
// ASM: addi 3, 4, 0
// ASM: addis 3, 4, 0
// ASM: addic 3, 4, 32767
// ASM: mulli 3, 4, 0
// ASM: subfic 3, 4, 32767
// ASM: cmpwi 3, 0
// ASM: cmpdi 3, 32767
// ASM: li 3, -32768
// ASM: lis 3, -1
void signed_immediates(void) {
  __asm {
    addi r3, r4, -32768
    addis r3, r4, 32768
    addic r3, r4, 65535
    mulli r3, r4, -32768
    subfic r3, r4, 65535
    cmpwi r3, -32768
    cmpdi r3, 65535
    li r3, -32768
    lis r3, 65535
  }
}

// ASM-LABEL: fpscr:
// ASM: mtfsf 255, 1
void fpscr(void) {
  __asm { mtfsf 255, fr1 }
}

// ASM-LABEL: dedicated_registers:
// ASM: li 1, 16
// ASM: li 2, 16
// ASM: li 13, 16
// ASM: lwzu 11, 4(2)
// ASM: stwu 3, -16(1)
void dedicated_registers(void) {
  __asm {
    li r1, 16
    li r2, 16
    li r13, 16
    lwzu r11, 4(r2)
    stwu r3, -16(r1)
  }
}

extern void callee(void);

// A C call still requires the ordinary nonleaf LR save/restore.
// ASM-LABEL: lr_nonleaf:
// ASM: mflr
// ASM: bl callee
// ASM: mtlr
// ASM: mtlr
// ASM: blr
void lr_nonleaf(int a) {
  callee();
  __asm { mtlr a }
}

// ASM-LABEL: pressure:
// ASM: #APP
// ASM: li 29, 29
// ASM: add
// ASM: #NO_APP
int pressure(int a, int b) {
  int result;
  __asm {
    li r4, 4
    li r5, 5
    li r6, 6
    li r7, 7
    li r8, 8
    li r9, 9
    li r10, 10
    li r11, 11
    li r12, 12
    li r14, 14
    li r15, 15
    li r16, 16
    li r17, 17
    li r18, 18
    li r19, 19
    li r20, 20
    li r21, 21
    li r22, 22
    li r23, 23
    li r24, 24
    li r25, 25
    li r26, 26
    li r27, 27
    li r28, 28
    li r29, 29
    add result, a, b
  }
  return result;
}

// A raw value used through a backedge cannot become a C operand scratch.
// ASM-LABEL: live_raw:
// ASM: li 11, 41
// ASM-NOT: {{(lwz|add) 11,}}
// ASM: add [[OUT:[0-9]+]], [[IN:[0-9]+]], [[IN2:[0-9]+]]
// ASM-NOT: {{(lwz|add) 11,}}
// ASM: add 3, 11, {{[0-9]+}}
int live_raw(int a, int b) {
  __asm {
    li r11, 41
  loop:
    add a, a, b
    cmpwi a, 0
    beq loop
    add r3, r11, a
  }
}

extern void target(void);
extern void target2(void);

// IR-LABEL: define {{.*}} @direct(
// IR: asm sideeffect {{.*}}bl ${0:c}
// IR-SAME: "i,{{.*}}~{ctr},~{fpscr},~{xer}
// IR-SAME: ~{f0}
// IR-SAME: ~{r0},~{r3}
// IR-SAME: ~{v63}
// IR-SAME: ~{memory}"(ptr @target)
// ASM-LABEL: direct:
// ASM-NOT: mflr
// ASM: bl target
// ASM-NOT: mtlr
// ASM: blr
void direct(void) {
  __asm { bl target }
}

// IR-LABEL: define {{.*}} @indirect(
// IR: call i64 asm sideeffect {{.*}}bctrl
// IR-SAME: "=&{r3},
// ASM-LABEL: indirect:
// ASM-NOT: mflr
// ASM: bctrl
// ASM-NOT: mtlr
// ASM: blr
int indirect(void) {
  __asm { bctrl }
}

// IR-LABEL: define {{.*}} @managed_lr(
// IR-NOT: ~{lr}
// IR: asm sideeffect
// IR-SAME: bl ${0:c}
// ASM-LABEL: managed_lr:
// ASM: #APP
// ASM: mflr 31
// ASM: bl target
// ASM: mtlr 31
// ASM: #NO_APP
void managed_lr(void) {
  __asm {
    mflr r31
    bl target
    mtlr r31
  }
}

// Function symbols follow the single address-table input, not its entries.
// IR-LABEL: define {{.*}} @mixed(
// IR: alloca [3 x ptr]
// IR: asm sideeffect {{.*}}bl ${2:c}{{.*}}bl ${3:c}{{.*}}bl ${2:c}
// IR-SAME: "=&{r3},b,i,i,
// IR-SAME: (ptr %msasm.operands, ptr @target, ptr @target2)
// ASM-LABEL: mixed:
// ASM: #APP
// ASM: bl target
// ASM: bl target2
// ASM: bl target
// ASM: lwz
// ASM: add
// ASM: #NO_APP
int mixed(int a, int b) {
  int result;
  __asm {
    add result, a, b
    bl target
    bl target2
    bl target
    add result, result, a
  }
  return result;
}

// ASM-LABEL: spelling:
// ASM: lwz 3, 0(1)
// ASM: lwz 3, 0(5)
int spelling(void) {
  __asm {
    lwz r3, 0(fr1)
    lwz r3, 0(vr5)
  }
}

// XDK lowers plain fmr of small C objects as an integer copy.
// ASM-LABEL: short_move:
// ASM: #APP
// ASM: lhz [[SRC:[0-9]+]],
// ASM-NOT: fmr
// ASM: sth [[SRC]],
// ASM: #NO_APP
short short_move(short a) {
  short result;
  __asm { fmr result, a }
  return result;
}

// ASM-LABEL: char_move:
// ASM: #APP
// ASM: lbz [[BYTE:[0-9]+]],
// ASM-NOT: fmr
// ASM: stb [[BYTE]],
// ASM: #NO_APP
char char_move(char a) {
  char result;
  __asm { fmr result, a }
  return result;
}

// Crossing banks in plain fmr uses register zero in the destination bank.
// ASM-LABEL: short_from_float:
// ASM: #APP
// ASM: lfs
// ASM-NOT: fmr
// ASM: sth 0,
// ASM: #NO_APP
short short_from_float(float a) {
  short result;
  __asm { fmr result, a }
  return result;
}

// ASM-LABEL: float_from_short:
// ASM: #APP
// ASM: lhz
// ASM-NOT: fmr
// ASM: stfs 0,
// ASM: #NO_APP
float float_from_short(short a) {
  float result;
  __asm { fmr result, a }
  return result;
}

// ASM-LABEL: short_from_physical:
// ASM: #APP
// ASM-NOT: fmr
// ASM: sth 0,
// ASM: #NO_APP
short short_from_physical(void) {
  short result;
  __asm { fmr result, fr3 }
  return result;
}

// Other FP opcodes retain their FP operation, using the GPR scratch numbers.
// ASM-LABEL: short_add:
// ASM: #APP
// ASM: lhz [[A:[0-9]+]],
// ASM: lhz [[B:[0-9]+]],
// ASM: fadd [[D:[0-9]+]], [[A]], [[B]]
// ASM: sth [[D]],
// ASM: #NO_APP
short short_add(short a, short b) {
  short result;
  __asm { fadd result, a, b }
  return result;
}

// ASM-LABEL: short_record_move:
// ASM: lhz [[REC:[0-9]+]],
// ASM: fmr. [[RECD:[0-9]+]], [[REC]]
// ASM: sth [[RECD]],
short short_record_move(short a) {
  short result;
  __asm { fmr. result, a }
  return result;
}

#endif

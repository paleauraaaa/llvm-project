// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -mrelocation-model static -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -mrelocation-model static -ffreestanding -fno-builtin -emit-llvm -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -mrelocation-model static -O2 -emit-llvm -o - %s | FileCheck %s --check-prefixes=OPT,COMMON
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -mrelocation-model static -O2 -DQUAL=__restrict -emit-llvm -o - %s | FileCheck %s --check-prefixes=RESTRICT,COMMON
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -mrelocation-model static -O2 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -mrelocation-model static -S -o - %s | FileCheck %s --check-prefix=ASM

void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadBarrier, _WriteBarrier, _ReadWriteBarrier)

// IR-LABEL: define {{.*}} @empty_read(
// IR: call void (...) @llvm.ppc.xbox.read.barrier()
// ASM-LABEL: empty_read:
// ASM-NOT: {{^[ \t]+(sync|lwsync|eieio|isync|bl)([ \t]|$)}}
// ASM: blr
void empty_read(void) { _ReadBarrier(); }

// IR-LABEL: define {{.*}} @empty_write(
// IR: call void (...) @llvm.ppc.xbox.write.barrier()
// ASM-LABEL: empty_write:
// ASM-NOT: {{^[ \t]+(sync|lwsync|eieio|isync|bl)([ \t]|$)}}
// ASM: blr
void empty_write(void) { _WriteBarrier(); }

// IR-LABEL: define {{.*}} @empty_readwrite(
// IR: call void (...) @llvm.ppc.xbox.read.write.barrier()
// ASM-LABEL: empty_readwrite:
// ASM-NOT: {{^[ \t]+(sync|lwsync|eieio|isync|bl)([ \t]|$)}}
// ASM: blr
void empty_readwrite(void) { _ReadWriteBarrier(); }

#ifndef QUAL
#define QUAL
#endif

#define PROBE(N, B) \
  int reads_##N(int *QUAL p) { int a = *p; B; return a + *p; } \
  void writes_##N(int *QUAL p) { *p = 1; B; *p = 2; } \
  int store_load_##N(int *QUAL p) { *p = 1; B; return *p; }

// XDK /O2: a read barrier prevents load reuse and forwarding but not DSE.
// OPT-LABEL: define {{.*}} @reads_read(
// OPT: load i32
// OPT: call void (...) @llvm.ppc.xbox.read.barrier()
// OPT: load i32
// OPT: ret i32
// OPT-LABEL: define {{.*}} @writes_read(
// OPT-NOT: store i32 1
// OPT: call void (...) @llvm.ppc.xbox.read.barrier()
// OPT: store i32 2
// OPT: ret void
// OPT-LABEL: define {{.*}} @store_load_read(
// OPT: store i32 1
// OPT: call void (...) @llvm.ppc.xbox.read.barrier()
// OPT: load i32
// OPT: ret i32
PROBE(read, _ReadBarrier())

// XDK /O2: a write barrier preserves stores but allows load reuse/forwarding.
// OPT-LABEL: define {{.*}} @reads_write(
// OPT: load i32
// OPT: call void (...) @llvm.ppc.xbox.write.barrier()
// OPT-NOT: load i32
// OPT: ret i32
// OPT-LABEL: define {{.*}} @writes_write(
// OPT: store i32 1
// OPT: call void (...) @llvm.ppc.xbox.write.barrier()
// OPT: store i32 2
// OPT: ret void
// OPT-LABEL: define {{.*}} @store_load_write(
// OPT: store i32 1
// OPT: call void (...) @llvm.ppc.xbox.write.barrier()
// OPT-NOT: load i32
// OPT: ret i32 1
PROBE(write, _WriteBarrier())

// OPT-LABEL: define {{.*}} @reads_readwrite(
// OPT: load i32
// OPT: call void (...) @llvm.ppc.xbox.read.write.barrier()
// OPT: load i32
// OPT: ret i32
// OPT-LABEL: define {{.*}} @writes_readwrite(
// OPT: store i32 1
// OPT: call void (...) @llvm.ppc.xbox.read.write.barrier()
// OPT: store i32 2
// OPT: ret void
// OPT-LABEL: define {{.*}} @store_load_readwrite(
// OPT: store i32 1
// OPT: call void (...) @llvm.ppc.xbox.read.write.barrier()
// OPT: load i32
// OPT: ret i32
PROBE(readwrite, _ReadWriteBarrier())

// XDK /O2 preserves both directions for restricted pointers, for all barriers.
// RESTRICT-LABEL: define {{.*}} @reads_read(
// RESTRICT: load i32
// RESTRICT: call void (...) @llvm.ppc.xbox.read.barrier(ptr {{.*}}%p)
// RESTRICT: load i32
// RESTRICT-LABEL: define {{.*}} @writes_read(
// RESTRICT: store i32 1
// RESTRICT: call void (...) @llvm.ppc.xbox.read.barrier(ptr {{.*}}%p)
// RESTRICT: store i32 2
// RESTRICT-LABEL: define {{.*}} @store_load_read(
// RESTRICT: store i32 1
// RESTRICT: call void (...) @llvm.ppc.xbox.read.barrier(ptr {{.*}}%p)
// RESTRICT: load i32
// RESTRICT-LABEL: define {{.*}} @reads_write(
// RESTRICT: load i32
// RESTRICT: call void (...) @llvm.ppc.xbox.write.barrier(ptr {{.*}}%p)
// RESTRICT: load i32
// RESTRICT-LABEL: define {{.*}} @writes_write(
// RESTRICT: store i32 1
// RESTRICT: call void (...) @llvm.ppc.xbox.write.barrier(ptr {{.*}}%p)
// RESTRICT: store i32 2
// RESTRICT-LABEL: define {{.*}} @store_load_write(
// RESTRICT: store i32 1
// RESTRICT: call void (...) @llvm.ppc.xbox.write.barrier(ptr {{.*}}%p)
// RESTRICT: load i32
// RESTRICT-LABEL: define {{.*}} @reads_readwrite(
// RESTRICT: load i32
// RESTRICT: call void (...) @llvm.ppc.xbox.read.write.barrier(ptr {{.*}}%p)
// RESTRICT: load i32
// RESTRICT-LABEL: define {{.*}} @writes_readwrite(
// RESTRICT: store i32 1
// RESTRICT: call void (...) @llvm.ppc.xbox.read.write.barrier(ptr {{.*}}%p)
// RESTRICT: store i32 2
// RESTRICT-LABEL: define {{.*}} @store_load_readwrite(
// RESTRICT: store i32 1
// RESTRICT: call void (...) @llvm.ppc.xbox.read.write.barrier(ptr {{.*}}%p)
// RESTRICT: load i32

static __forceinline int restricted_inline(int *__restrict p) {
  int a = *p;
  _WriteBarrier();
  return a + *p;
}

// XDK drops the helper's restrict information when inlining into this caller.
// COMMON-LABEL: define {{.*}} @inlined_restricted_write(
// COMMON: load i32
// COMMON: call void (...) @llvm.ppc.xbox.write.barrier(ptr {{.*}}%p)
// COMMON-NOT: load i32
// COMMON: ret i32
int inlined_restricted_write(int *p) { return restricted_inline(p); }

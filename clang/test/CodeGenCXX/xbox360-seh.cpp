// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -fcxx-exceptions -emit-llvm -disable-llvm-passes -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -fcxx-exceptions -O0 -mllvm -verify-machineinstrs -emit-obj -o %t.O0 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -fcxx-exceptions -O1 -mllvm -verify-machineinstrs -emit-obj -o %t.O1 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -fcxx-exceptions -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.O2 %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -fcxx-exceptions -fasync-exceptions -Os -mllvm -verify-machineinstrs -emit-obj -o %t.Os %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -O2 -mllvm -verify-machineinstrs -emit-obj -o %t.noeh %s
// RUN: llvm-readobj --relocations %t.O0 | FileCheck %s --check-prefix=OBJ
// RUN: llvm-readobj --relocations %t.O2 | FileCheck %s --check-prefix=OBJ
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -mrelocation-model static -fms-extensions -fexceptions -fcxx-exceptions -O2 -S -o - %s | FileCheck %s --check-prefix=ASM

extern "C" unsigned long _exception_code();
extern "C" void *_exception_info();
extern "C" int _abnormal_termination();
extern "C" void *_alloca(unsigned);
#pragma intrinsic(_exception_code, _exception_info, _abnormal_termination, _alloca)
extern "C" int risky(int);
extern "C" int filter(unsigned long, void *, int *);
extern "C" void observe(int);
extern "C" void escape(void *);

// IR-LABEL: define {{.*}}i32 @memory(
// IR-SAME: personality ptr @__C_specific_handler
// IR: invoke void @llvm.seh.try.begin()
// IR: load volatile i32
// IR: store volatile i32
// IR: invoke void @llvm.seh.try.end()
// IR: catchpad within {{.*}} [ptr @"?filt$
// IR: call i32 @llvm.eh.exceptioncode(
extern "C" int memory(int *p) {
  __try {
    *p += 1;
  } __except (1) {
    return (int)_exception_code();
  }
  return 0;
}

// IR: define internal ppc_xbox360_sehcc noundef i32 @"?filt$
extern "C" int captured(int n) {
  __try {
    n = risky(n);
  } __except (filter(_exception_code(), _exception_info(), &n)) {
    observe((int)_exception_code());
    n += 1;
  }
  return n;
}

extern "C" int search(int n) {
  __try { n = risky(n); }
  __except (0) { n = -1; }
  return n;
}

extern "C" int resume(int n) {
  __try { n = risky(n); }
  __except (-1) { n = -1; }
  return n;
}

// IR-LABEL: define {{.*}}i32 @finalizer(
// IR: call ptr @llvm.localaddress()
// IR: call ppc_xbox360_sehcc void @"?fin$
// IR: cleanuppad within none
// IR: call ppc_xbox360_sehcc void @"?fin$
extern "C" int finalizer(int n) {
  __try { n = risky(n); }
  __finally { observe(n + _abnormal_termination()); }
  return n;
}

// IR-LABEL: define {{.*}}i32 @early_return(
// IR: store volatile i32 1, ptr %cleanup.dest.slot
// IR: invoke void @llvm.seh.try.end()
// IR: [[DEST:%.*]] = load i32, ptr %cleanup.dest.slot
// IR: [[ABNORMAL:%.*]] = icmp ne i32 [[DEST]], 0
// IR: br i1 [[ABNORMAL]], label %seh.localunwind, label %seh.finally.normal
// IR: seh.localunwind:
// IR: invoke void @__jump_unwind(ptr {{%.*}}, ptr blockaddress(@early_return, %seh.localunwind.cont))
// IR-NEXT: to label %seh.localunwind.cont unwind label %ehcleanup
// IR: seh.finally.normal:
// IR: call ppc_xbox360_sehcc void @"?fin$0@0@early_return@@"(i8 noundef 0,
// IR: seh.localunwind.cont:
extern "C" int early_return(int n) {
  __try { return risky(n); }
  __finally { observe(n + _abnormal_termination()); }
}

// IR-LABEL: define {{.*}}i32 @leave(
// IR-NOT: @__jump_unwind
// IR: __try.__leave:
// IR-NEXT: invoke void @llvm.seh.try.end()
// IR: call ppc_xbox360_sehcc void @"?fin$0@0@leave@@"(i8 noundef 0,
extern "C" int leave(int n) {
  __try {
    if (n) __leave;
    n = risky(n);
  } __finally { observe(n + _abnormal_termination()); }
  return n;
}

extern "C" int dynamic(int n) {
  void *p = _alloca((unsigned)n);
  __try { escape(p); n = risky(n); }
  __except (filter(_exception_code(), _exception_info(), &n)) {
    escape(p);
    escape(_alloca((unsigned)n));
    n = -3;
  }
  return n;
}

extern "C" int large(int n) {
  char buffer[40000];
  __try { escape(buffer); n = risky(n); }
  __finally { escape(buffer); observe(_abnormal_termination()); }
  return n;
}

extern "C" int nested(int n) {
  __try {
    __try { n = risky(n); }
    __finally { observe(n + _abnormal_termination()); }
  } __except (filter(_exception_code(), _exception_info(), &n)) {
    n = (int)_exception_code();
  }
  return n;
}

extern "C" int nested_finally(int n) {
  __try { n = risky(n); }
  __finally {
    __try { n = risky(n); }
    __finally { observe(n + _abnormal_termination()); }
  }
  return n;
}

extern "C" int nested_except(int n) {
  __try {
    __try { n = risky(n); }
    __except (filter(_exception_code(), _exception_info(), &n)) {
      n = risky(n + 1);
    }
  } __except (1) { n = -2; }
  return n;
}

extern "C" int nested_local(int n) {
  __try { n = risky(n); }
  __finally {
    int local = n + 1;
    __try { n = risky(local); }
    __finally { observe(n + local + _abnormal_termination()); }
  }
  return n;
}

// IR-LABEL: define {{.*}}i32 @goto_except(
// IR: store volatile i32
// IR: invoke void @llvm.seh.try.end()
// IR: outside:
// IR-NEXT: {{%.*}} = load ptr
// IR-NEXT: {{%.*}} = load i32
extern "C" int goto_except(int *p) {
  __try {
    *p += 1;
    goto outside;
  } __except (1) {
    return -1;
  }
outside:
  return *p;
}

extern "C" int break_finally(int n) {
  while (n) {
    __try { if (risky(n)) break; }
    __finally { observe(_abnormal_termination()); }
    --n;
  }
  return n;
}

extern "C" int goto_finally(int n) {
  __try { if (risky(n)) goto outside; }
  __finally { observe(_abnormal_termination()); }
outside:
  return n;
}

// IR-LABEL: define {{.*}}i32 @nested_return(
// IR: invoke void @__jump_unwind
// IR: seh.localunwind.cont:
// IR: invoke void @__jump_unwind
// IR: seh.localunwind.cont{{[0-9]+}}:
// ASM-LABEL: nested_return:
// ASM: li {{[0-9]+}}, [[RESUME:Ltmp[0-9]+]]@l
// ASM: [[UNWIND_BEGIN:Ltmp[0-9]+]]:
// ASM-NEXT: bl __jump_unwind
// ASM-NEXT: [[UNWIND_END:Ltmp[0-9]+]]:
// ASM: [[RESUME]]:
// ASM: [[OUTER_BOUNDARY:Ltmp[0-9]+]]:
// ASM-NEXT: nop
// ASM-NEXT: [[OUTER_END:Ltmp[0-9]+]]:
// ASM: bl __jump_unwind
// ASM: [[INNER_HANDLER:LBB[0-9]+_[0-9]+]]: {{.*}}Block address taken
// ASM: addi 31, 12,
// ASM: stwu 1, -[[PRIVATE:[0-9]+]](1)
// ASM: bl "?fin$1@0@nested_return@@"
// ASM: addi 1, 1, [[PRIVATE]]
// ASM: $sehxdata$nested_return:
// ASM: .long [[SCOPE_BEGIN:Ltmp[0-9]+]]
// ASM-NEXT: .long {{Ltmp[0-9]+}}
// ASM-NEXT: .long [[INNER_HANDLER]]
// ASM-NEXT: .long 0
// ASM-NEXT: .long [[UNWIND_BEGIN]]
// ASM-NEXT: .long [[UNWIND_END]]
// ASM-NEXT: .long [[INNER_HANDLER]]
// ASM-NEXT: .long 0
// ASM-NEXT: .long [[SCOPE_BEGIN]]
// ASM-NEXT: .long [[OUTER_END]]
extern "C" int nested_return(int n) {
  __try {
    __try { return risky(n); }
    __finally { observe(10 + _abnormal_termination()); }
  } __finally { observe(20 + _abnormal_termination()); }
}

extern "C" int large_finally(int n) {
  __try { n = risky(n); }
  __finally {
    char buffer[40000];
    escape(buffer);
    observe(n + _abnormal_termination());
  }
  return n;
}

extern "C" int empty_except() {
  __try {} __except (1) { return -1; }
  return 0;
}

extern "C" void empty_finally() {
  __try {} __finally { observe(_abnormal_termination()); }
}

extern "C" int dynamic_return(int n) {
  void *p = _alloca((unsigned)n);
  __try { escape(p); return risky(n); }
  __finally { escape(p); observe(_abnormal_termination()); }
}

// ASM-LABEL: large_return:
// ASM: bl _RtlCheckStack12
// ASM: bl __jump_unwind
// ASM: add 1, 31, 12
extern "C" int large_return(int n) {
  char buffer[40000];
  __try { escape(buffer); return risky(n); }
  __finally { escape(buffer); observe(_abnormal_termination()); }
}

extern "C" int continue_finally(int n) {
  int result = 0;
  while (n--) {
    __try {
      if (risky(n)) continue;
      result += n;
    } __finally {
      observe(_abnormal_termination());
    }
  }
  return result;
}

extern "C" int nested_break(int n) {
  __try {
    while (n--) {
      __try { if (risky(n)) break; }
      __finally { observe(10 + _abnormal_termination()); }
    }
    observe(n);
  } __finally { observe(20 + _abnormal_termination()); }
  return n;
}

extern "C" void helper_local_exit(int n) {
  __try { observe(n); }
  __finally {
    __try { if (risky(n)) goto done; }
    __finally { observe(_abnormal_termination()); }
  done:
    observe(n);
  }
}

// OBJ-DAG: IMAGE_REL_PPC_ADDR32 __C_specific_handler
// OBJ-DAG: IMAGE_REL_PPC_ADDR32 $sehxdata$memory
// OBJ-DAG: IMAGE_REL_PPC_ADDR32 {{.*}}filt
// OBJ-DAG: IMAGE_REL_PPC_ADDR32 $sehxdata$nested_finally
// OBJ-DAG: IMAGE_REL_PPC_REL24 __jump_unwind

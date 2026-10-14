// REQUIRES: powerpc-registered-target
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O0 -g -gcodeview -c %s -o %t.o0.obj
// RUN: llvm-readobj --sections --relocations --codeview %t.o0.obj | FileCheck %s
// RUN: %clang -target powerpc64-ibm-xbox360 -mcpu=xenon -O2 -g -gcodeview -c %s -o %t.o2.obj
// RUN: llvm-readobj --sections --relocations --codeview %t.o2.obj | FileCheck %s

__declspec(thread) int tls_zero;
__declspec(thread) int tls_initialized = 7;
static __declspec(thread) int tls_static = 11;
extern __declspec(thread) int tls_external;

int *tls_address(void) { return &tls_zero; }
int tls_read(void) { return tls_initialized + tls_external; }
void tls_write(int value) { tls_zero = value; }
int *tls_static_address(void) { return &tls_static; }

// CHECK: Name: .tls$
// CHECK: IMAGE_SCN_CNT_INITIALIZED_DATA
// CHECK: IMAGE_SCN_MEM_READ
// CHECK: IMAGE_SCN_MEM_WRITE
// CHECK-DAG: IMAGE_REL_PPC_SECREL16 tls_zero
// CHECK-DAG: IMAGE_REL_PPC_SECREL16 tls_initialized
// CHECK-DAG: IMAGE_REL_PPC_SECREL16 tls_static
// CHECK-DAG: IMAGE_REL_PPC_SECREL16 tls_external
// CHECK-DAG: IMAGE_REL_PPC_SECREL tls_zero
// CHECK-DAG: IMAGE_REL_PPC_SECTION tls_zero
// CHECK-DAG: IMAGE_REL_PPC_SECREL tls_static
// CHECK-DAG: IMAGE_REL_PPC_SECTION tls_static
// CHECK-DAG: GlobalTLS {{{$}}
// CHECK-DAG: Kind: S_GTHREAD32 (0x1113){{[[:space:]]+}}DataOffset: tls_zero+0x0{{[[:space:]]+}}Type: int (0x74){{[[:space:]]+}}DisplayName: tls_zero
// CHECK-DAG: Kind: S_GTHREAD32 (0x1113){{[[:space:]]+}}DataOffset: tls_initialized+0x0{{[[:space:]]+}}Type: int (0x74){{[[:space:]]+}}DisplayName: tls_initialized
// CHECK-DAG: ThreadLocalDataSym {{{$}}
// CHECK-DAG: Kind: S_LTHREAD32 (0x1112){{[[:space:]]+}}DataOffset: tls_static+0x0{{[[:space:]]+}}Type: int (0x74){{[[:space:]]+}}DisplayName: tls_static

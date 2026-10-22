// RUN: %clang_cc1 -triple powerpc64-unknown-linux -target-cpu xenon -fsyntax-only -verify=VMX %s
// RUN: %clang_cc1 -triple powerpc64-unknown-linux -target-cpu pwr7 -fsyntax-only -verify=NOVMX %s
// VMX-no-diagnostics

void extended_vector_registers(void) {
  __asm__ volatile("" : : : "v32", "v127"); // NOVMX-error {{unknown register name 'v32' in asm}}
}

void ordinary_vector_registers(void) {
  __asm__ volatile("" : : : "v31");
}

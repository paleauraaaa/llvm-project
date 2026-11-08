// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -fsyntax-only -verify %s
// RUN: %clang_cc1 -x c++ -triple powerpc64-ibm-xbox360 -target-cpu xenon -fms-extensions -fasm-blocks -fsyntax-only -verify %s

void bad_base(int *p) {
  __asm { lwz r11, 0(p) } // expected-error {{invalid memory operand}}
}

void bad_register(void) {
  __asm { li r128, 1 } // expected-error {{expected a register or C variable}}
}

void bad_operand(int a) {
  __asm { addi r11, r12, a } // expected-error {{C variable requires a register operand}}
}

void bad_instruction(void) {
  __asm { not_an_instruction r11 } // expected-error {{invalid instruction}}
}

void bad_size(void) {
  struct { int a, b, c; } object;
  __asm { mr r11, object } // expected-error {{invalid C operand size for GPR}}
}

void stack_pointer(void) {
  __asm { li r1, 0 }
}

void calls(void) {
  __asm { bl function } // expected-error {{direct asm call requires a C function name}}
}

void nonfunction_call(int value, void (*pointer)(void)) {
  __asm { bl value } // expected-error {{direct asm call requires a C function name}}
  __asm { bl pointer } // expected-error {{direct asm call requires a C function name}}
}

#ifdef __cplusplus
struct nonfunction_prvalue_call {
  void method() {
    __asm { bl this } // expected-error {{direct asm call requires a C function name}}
  }
};
#endif

void wrong_register_class(void) {
  __asm { fadd r3, fr4, fr5 } // expected-error {{invalid register class for operand}}
}

void wrong_base_class(void) {
  __asm { lwz r11, 0(f1) }
}

void register_immediate(void) {
  __asm { addi r11, r12, r13 } // expected-error {{register is not valid in an immediate operand}}
}

void variable_register_range(int a) {
  __asm { lmw a, 0(r11) } // expected-error {{multiple-register transfer requires a physical register}}
}

void dedicated_register_range(void) {
  __asm { lmw r10, 0(r11) }
}

void bad_vector_register(void) {
  __asm { vxor128 vr128, vr0, vr0 } // expected-error {{expected a register or C variable}}
}

void bad_floating_register(void) {
  __asm { fmr fr32, fr1 } // expected-error {{expected a register or C variable}}
}

void unsupported_fpscr(void) {
  __asm { mtfsfi 0, 0 } // expected-error {{opcode not supported by XDK inline asm}}
  __asm { mtfsb0 0 } // expected-error {{opcode not supported by XDK inline asm}}
  __asm { mtfsb1 0 } // expected-error {{opcode not supported by XDK inline asm}}
}

void immediate_quirk(void) {
  __asm { addi r3, r4, -32768 } // expected-warning {{signed immediate value out of range in XDK inline asm}}
  __asm { addi r3, r4, 65535 } // expected-warning {{signed immediate value out of range in XDK inline asm}}
}

// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -fms-compatibility -x c++ -std=c++11 -fsyntax-only -verify %s

__declspec(naked) void declared(int);
void declared(int value) {}

__declspec(naked) void empty(void) {}
__declspec(naked) void null_statement(void) { ; }
__declspec(naked) int no_implicit_return(int value) {}

__declspec(naked) void non_asm(void) { // expected-note {{attribute is here}}
  return; // expected-error {{non-ASM statement in naked function is not supported}}
}

__declspec(naked) void local(void) { // expected-note {{attribute is here}}
  int value; // expected-error {{non-ASM statement in naked function is not supported}}
}

__declspec(naked) void parameter(int value) { // expected-note {{attribute is here}}
  __asm__("" : : "r"(value)); // expected-error {{parameter references not allowed in naked functions}}
}

#ifdef __cplusplus
struct Members {
  __declspec(naked) void member(); // expected-error {{'naked' attribute only applies to non-member functions}}
  __declspec(naked) static void static_member(); // expected-error {{'naked' attribute only applies to non-member functions}}
};
#endif

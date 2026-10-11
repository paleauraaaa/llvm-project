// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -fsyntax-only -verify=xbox %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -fms-extensions -x c++ -std=c++11 -fsyntax-only -verify=xbox %s
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fms-extensions -fsyntax-only -verify=other %s
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -fms-extensions -x c++ -std=c++11 -fsyntax-only -verify=other %s
// other-no-diagnostics

typedef struct __declspec(intrin_type) __declspec(align(16)) __vector4 {
  float v[4];
} __vector4;
typedef __vector4 VectorAlias;
typedef float NativeVector __attribute__((vector_size(16)));
struct Wrapper { __vector4 value; };
void variadic(unsigned marker, ...);
void named(__vector4 vector, ...);
__vector4 produce(void);

void calls(__vector4 value, const VectorAlias qualified, struct Wrapper wrapper,
           NativeVector native, void (*indirect)(unsigned, ...)) {
  variadic(1, value); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  variadic(1, qualified); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  variadic(1, produce()); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  indirect(1, value); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  named(value, qualified); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  if (0)
    variadic(1, value); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  named(value, 17);
  variadic(1, &value, wrapper, native);
}

void receiver(__vector4 *out, unsigned marker, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, marker);
  *out = __builtin_va_arg(args, __vector4);
  __builtin_va_end(args);
}

#ifdef __cplusplus
struct Methods {
  void member(unsigned, ...);
  Methods(unsigned, ...);
};
void member_calls(Methods *object, __vector4 value) {
  object->member(1, value); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
  Methods constructed(1, value); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
}
template <class T> void dependent(T value) {
  variadic(1, value); // xbox-error {{__vector4 arguments to variadic calls are not supported on Xbox 360}}
}
template void dependent(__vector4); // xbox-note {{in instantiation of function template specialization}}
#endif

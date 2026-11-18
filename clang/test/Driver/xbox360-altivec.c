// RUN: %clang --target=powerpc64-ibm-xbox360 -fsyntax-only %s
// RUN: %clang --target=powerpc64-ibm-xbox360 -x c++ -fsyntax-only %s
// RUN: %clang --target=powerpc64-ibm-xbox360 -DEXPLICIT_INCLUDE -include altivec.h -fsyntax-only %s
// RUN: %clang --target=powerpc64-ibm-xbox360 -x c++ -DEXPLICIT_INCLUDE -include altivec.h -fsyntax-only %s

#ifdef EXPLICIT_INCLUDE
#ifndef __ALTIVEC_H
#error "Explicitly including altivec.h must remain supported"
#endif
#else
#ifdef __ALTIVEC_H
#error "altivec.h must not be included automatically"
#endif
#endif

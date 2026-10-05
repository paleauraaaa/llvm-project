// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -target-cpu g5 -target-feature +vmx128 -E -dM %s | FileCheck %s --check-prefix=ON
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -target-cpu g5 -E -dM %s | FileCheck %s --check-prefix=OFF

// ON: #define __VMX128__ 1
// OFF-NOT: __VMX128__

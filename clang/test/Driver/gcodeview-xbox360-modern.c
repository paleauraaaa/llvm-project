// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -### -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gcodeview-xbox360-modern -### -c %s 2>&1 | FileCheck %s --check-prefix=MODERN
// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gcodeview-xbox360-modern -gno-codeview-xbox360-modern -### -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gno-codeview-xbox360-modern -gcodeview-xbox360-modern -### -c %s 2>&1 | FileCheck %s --check-prefix=MODERN
// RUN: %clang -target powerpc64-ibm-xbox360 -gcodeview-xbox360-modern -### -c %s 2>&1 | FileCheck %s --check-prefix=NO-DEBUG
// RUN: %clang_cl -target powerpc64-ibm-xbox360 /Z7 -gcodeview -gcodeview-xbox360-modern -### -c -- %s 2>&1 | FileCheck %s --check-prefix=MODERN
// RUN: %clang_cl -target powerpc64-ibm-xbox360 /Z7 -gcodeview -gcodeview-xbox360-modern -gno-codeview-xbox360-modern -### -c -- %s 2>&1 | FileCheck %s --check-prefix=DEFAULT

// DEFAULT: "-gcodeview"
// DEFAULT-NOT: "-gcodeview-xbox360-modern"
// MODERN: "-gcodeview"
// MODERN-SAME: "-gcodeview-xbox360-modern"
// NO-DEBUG-NOT: "-gcodeview"
// NO-DEBUG-NOT: "-gcodeview-xbox360-modern"

// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -### -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gcodeview-xbox360-legacy -### -c %s 2>&1 | FileCheck %s --check-prefix=LEGACY
// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gcodeview-xbox360-legacy -gno-codeview-xbox360-legacy -### -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gno-codeview-xbox360-legacy -gcodeview-xbox360-legacy -### -c %s 2>&1 | FileCheck %s --check-prefix=LEGACY
// RUN: %clang -target powerpc64-ibm-xbox360 -gcodeview-xbox360-legacy -### -c %s 2>&1 | FileCheck %s --check-prefix=NO-DEBUG
// RUN: %clang_cl -target powerpc64-ibm-xbox360 /Z7 -gcodeview -gcodeview-xbox360-legacy -### -c -- %s 2>&1 | FileCheck %s --check-prefix=LEGACY
// RUN: %clang_cl -target powerpc64-ibm-xbox360 /Z7 -gcodeview -gcodeview-xbox360-legacy -gno-codeview-xbox360-legacy -### -c -- %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: not %clang -target powerpc64-ibm-xbox360 -g -gcodeview -gcodeview-xbox360-modern -c %s -o %t.o 2>&1 | FileCheck %s --check-prefix=REMOVED

// DEFAULT: "-gcodeview"
// DEFAULT-NOT: "-gcodeview-xbox360-legacy"
// LEGACY: "-gcodeview"
// LEGACY-SAME: "-gcodeview-xbox360-legacy"
// NO-DEBUG-NOT: "-gcodeview"
// NO-DEBUG-NOT: "-gcodeview-xbox360-legacy"
// REMOVED: error: unknown argument: '-gcodeview-xbox360-modern'

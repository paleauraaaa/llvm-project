// RUN: %clang --target=ppc64-unknown-xbox360 -### -c %s 2>&1 | FileCheck %s --check-prefix=STATIC
// RUN: not %clang --target=ppc64-unknown-xbox360 -fPIC -### -c %s 2>&1 | FileCheck %s --check-prefix=PIC-ERROR
// RUN: not %clang --target=ppc64-unknown-xbox360 -fPIE -### -c %s 2>&1 | FileCheck %s --check-prefix=PIE-ERROR

// STATIC: "-mrelocation-model" "static"
// PIC-ERROR: error: unsupported option '-fPIC' for target 'ppc64-unknown-xbox360-msvc'
// PIE-ERROR: error: unsupported option '-fPIE' for target 'ppc64-unknown-xbox360-msvc'

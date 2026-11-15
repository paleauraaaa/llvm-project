// REQUIRES: system-windows, native
// RUN: %build --compiler=clang-cl -o %t.exe -- %s
// RUN: %lldb -b -f %t.exe -o "breakpoint set -n stop_here" -o run -o "thread list" -o "thread backtrace" -o continue -o "thread list" -o "thread backtrace" | FileCheck %s

#include <windows.h>

__declspec(noinline) void stop_here() {}

int main() {
  SetThreadDescription(GetCurrentThread(), L"first-name");
  stop_here();
  SetThreadDescription(GetCurrentThread(), L"second-name");
  stop_here();
  return 0;
}

// CHECK: name = 'first-name'
// CHECK: name = 'first-name'
// CHECK: frame #0:
// CHECK: name = 'second-name'
// CHECK: name = 'second-name'
// CHECK: frame #0:

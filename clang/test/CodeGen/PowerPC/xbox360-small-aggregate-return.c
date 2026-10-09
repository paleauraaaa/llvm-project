// RUN: %clang_cc1 -triple ppc64-ibm-xbox360 -mrelocation-model static -emit-llvm -o - %s | FileCheck %s

struct Byte { unsigned char value; };
struct Half { unsigned short value; };
struct Word { unsigned value; };
struct DoubleWord { unsigned long long value; };
struct Three { unsigned char bytes[3]; };
union FloatWord { float floating; unsigned integer; };

struct Byte byte(void) { struct Byte value = {0x12}; return value; }
// CHECK-LABEL: define dso_local i8 @byte()
struct Half half(void) { struct Half value = {0x1234}; return value; }
// CHECK-LABEL: define dso_local i16 @half()
struct Word word(void) { struct Word value = {0x12345678}; return value; }
// CHECK-LABEL: define dso_local i32 @word()
struct DoubleWord double_word(void) {
  struct DoubleWord value = {0x123456789abcdef0ULL};
  return value;
}
// CHECK-LABEL: define dso_local i64 @double_word()
struct Three three(void) { struct Three value = {{1, 2, 3}}; return value; }
// CHECK-LABEL: define dso_local void @three(ptr {{.*}}sret(%struct.Three)
union FloatWord float_word(void) {
  union FloatWord value = {1.0f};
  return value;
}
// CHECK-LABEL: define dso_local i32 @float_word()

unsigned call_word(void) { return word().value; }
// CHECK-LABEL: define dso_local {{.*}}i32 @call_word()
// CHECK: call i32 @word()

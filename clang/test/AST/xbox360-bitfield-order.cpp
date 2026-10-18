// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -ast-dump %s | FileCheck %s
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -x c -ast-dump %s | FileCheck %s --check-prefix=C
// RUN: %clang_cc1 -triple powerpc64-unknown-linux-gnu -Wno-unknown-pragmas -ast-dump %s | FileCheck %s --check-prefix=OTHER --implicit-check-not=BitfieldOrderAttr

// OTHER: RecordDecl {{.*}} struct Ordinary definition
// CHECK-LABEL: RecordDecl {{.*}} struct Ordinary definition
// C-LABEL: RecordDecl {{.*}} struct Ordinary definition
struct Ordinary {
  // CHECK: FieldDecl {{.*}} high 'unsigned int'
  // CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
  // C: FieldDecl {{.*}} high 'unsigned int'
  // C: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
  unsigned high : 3;
#pragma bitfield_order(lsb_to_msb)
  // CHECK: FieldDecl {{.*}} low 'unsigned int'
  // CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
  // C: FieldDecl {{.*}} low 'unsigned int'
  // C: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
  unsigned low : 5;
  // CHECK: FieldDecl {{.*}} ordinary 'int'
  // CHECK-NOT: BitfieldOrderAttr
  // C: FieldDecl {{.*}} ordinary 'int'
  // C-NOT: BitfieldOrderAttr
  int ordinary;
};

// CHECK-LABEL: RecordDecl {{.*}} union U definition
// C-LABEL: RecordDecl {{.*}} union U definition
union U {
  // CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
  // C: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
  unsigned low : 3;
};

// CHECK-LABEL: RecordDecl {{.*}} struct Forward definition
// C-LABEL: RecordDecl {{.*}} struct Forward definition
struct Forward;
#pragma bitfield_order(msb_to_lsb)
struct Forward {
  // CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
  // C: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
  unsigned high : 3;
  // CHECK: FieldDecl {{.*}} 'unsigned int'
  // CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
  // C: FieldDecl {{.*}} 'unsigned int'
  // C: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
  unsigned : 0;
};

#ifdef __cplusplus
// CHECK-LABEL: ClassTemplateDecl {{.*}} Pattern
// CHECK-NOT: BitfieldOrderAttr
// CHECK: ClassTemplateSpecializationDecl {{.*}} struct Pattern definition
// CHECK: TemplateArgument type 'int'
// CHECK: FieldDecl {{.*}} dependent 'int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
// CHECK: FieldDecl {{.*}} nondependent 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
// CHECK: ClassTemplateSpecializationDecl {{.*}} struct Pattern definition
// CHECK: TemplateArgument type 'unsigned int'
// CHECK: FieldDecl {{.*}} dependent 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
// CHECK: FieldDecl {{.*}} nondependent 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
template<class T> struct Pattern {
  T dependent : 3;
  unsigned nondependent : 5;
};
#pragma bitfield_order(lsb_to_msb)
Pattern<int> first;
#pragma bitfield_order(msb_to_lsb)
Pattern<int> reused;
Pattern<unsigned> second;

// CHECK-LABEL: ClassTemplateDecl {{.*}} LowPattern
// CHECK-NOT: BitfieldOrderAttr
// CHECK: ClassTemplateSpecializationDecl {{.*}} struct LowPattern definition
// CHECK: FieldDecl {{.*}} field 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
#pragma bitfield_order(lsb_to_msb)
template<class T> struct LowPattern { unsigned field : 3; };
#pragma bitfield_order(msb_to_lsb)
LowPattern<int> third;

// CHECK-LABEL: ClassTemplateSpecializationDecl {{.*}} struct LowPattern definition explicit_specialization
// CHECK: TemplateArgument type 'char'
// CHECK: FieldDecl {{.*}} explicitField 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
#pragma bitfield_order(lsb_to_msb)
template<> struct LowPattern<char> { unsigned explicitField : 4; };

// CHECK-LABEL: RecordDecl {{.*}} struct Nested definition
// CHECK: RecordDecl {{.*}} struct Inner definition
// CHECK: FieldDecl {{.*}} innerHigh 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB
// CHECK: FieldDecl {{.*}} outerLow 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
struct Nested {
#pragma bitfield_order(msb_to_lsb)
  struct Inner { unsigned innerHigh : 3; };
#pragma bitfield_order(lsb_to_msb)
  unsigned outerLow : 5;
};
#endif

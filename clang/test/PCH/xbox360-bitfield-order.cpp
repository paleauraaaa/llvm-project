// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -emit-pch -x c++-header %s -o %t.pch
// RUN: %clang_cc1 -triple powerpc64-ibm-xbox360 -include-pch %t.pch -ast-dump-all %s | FileCheck %s

#ifndef HEADER
#define HEADER
#pragma bitfield_order(lsb_to_msb)
struct Stored { unsigned field : 3; };
template<class T> struct Pattern { T field : 3; };
Pattern<int> stored;
#pragma bitfield_order(msb_to_lsb)
#else
Pattern<int> reused;
#pragma bitfield_order(msb_to_lsb)
Pattern<unsigned> instantiated;
#endif

// CHECK-LABEL: RecordDecl {{.*}} imported {{.*}}struct Stored definition
// CHECK: FieldDecl {{.*}} field 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
// CHECK-LABEL: ClassTemplateDecl {{.*}} Pattern
// CHECK: ClassTemplateSpecializationDecl {{.*}} struct Pattern definition
// CHECK: TemplateArgument type 'int'
// CHECK: FieldDecl {{.*}} field 'int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit LSBToMSB
// CHECK: ClassTemplateSpecializationDecl {{.*}} struct Pattern definition
// CHECK: TemplateArgument type 'unsigned int'
// CHECK: FieldDecl {{.*}} field 'unsigned int'
// CHECK: BitfieldOrderAttr {{.*}} Implicit MSBToLSB

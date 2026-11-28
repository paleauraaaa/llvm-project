; RUN: not llvm-as -disable-output %s 2>&1 | FileCheck %s

declare ptr @llvm.eh.xbox.exceptionobject.p0(token)

define void @invalid() {
  %object = call ptr @llvm.eh.xbox.exceptionobject.p0(token none)
  ; CHECK: eh.xbox.exceptionobject argument must be a catchpad
  ret void
}

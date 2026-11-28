//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LIBUNWIND_XBOX_SEH_CALL_FRAME_H
#define LIBUNWIND_XBOX_SEH_CALL_FRAME_H

#define XBOX_SEH_CALL_FRAME_SIZE 1456
#define XBOX_SEH_CALL_TARGET_OFFSET 1408
#define XBOX_SEH_CALL_PARENT_OFFSET 1412
#define XBOX_SEH_CALL_CLEANUP_OFFSET 1416
#define XBOX_SEH_CALL_EXCEPTION_OFFSET 1420
#define XBOX_SEH_CALL_CURSOR_OFFSET 1424
#define XBOX_SEH_CALL_SCOPE_OFFSET 1428
#define XBOX_SEH_STATE_EXCEPTION_OFFSET 0
#define XBOX_SEH_STATE_CURSOR_OFFSET 4
#define XBOX_SEH_STATE_SCOPE_OFFSET 8
#define XBOX_SEH_EXCEPTION_ABANDONED 1
#define XBOX_SEH_CALL_FILTER 0
#define XBOX_SEH_CALL_CLEANUP 1
#define XBOX_SEH_CALL_COLLISION 2

#endif

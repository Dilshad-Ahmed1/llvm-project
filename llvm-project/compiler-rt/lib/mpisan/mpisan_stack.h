//===-- mpisan_stack.h - MPI Usage Sanitizer stack traces -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef MPISAN_STACK_H
#define MPISAN_STACK_H

#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_stacktrace.h"

// sanitizer_stacktrace.h already defines __sanitizer::kStackTraceMax = 255.
// Use it directly via the namespace rather than redeclaring it.

#define GET_STACK_TRACE(max_size, fast)                                    \
  UNINITIALIZED __sanitizer::BufferedStackTrace stack;                     \
  if ((max_size) <= 2) {                                                   \
    stack.size = (max_size);                                               \
    if ((max_size) > 0) {                                                  \
      stack.top_frame_bp = GET_CURRENT_FRAME();                            \
      stack.trace_buffer[0] = __sanitizer::StackTrace::GetCurrentPc();    \
      if ((max_size) > 1)                                                  \
        stack.trace_buffer[1] = GET_CALLER_PC();                           \
    }                                                                      \
  } else {                                                                 \
    stack.Unwind(__sanitizer::StackTrace::GetCurrentPc(),                  \
                 GET_CURRENT_FRAME(), nullptr, (fast), (max_size));        \
  }

#define GET_STACK_TRACE_FATAL_HERE \
  GET_STACK_TRACE(__sanitizer::kStackTraceMax,                             \
                  __sanitizer::common_flags()->fast_unwind_on_fatal)

#endif // MPISAN_STACK_H

//===-- mpisan_flags.h - MPI Usage Sanitizer --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Runtime configuration flags for MPISan.
//===----------------------------------------------------------------------===//

#pragma once

namespace __mpisan {

struct Flags {
#define MPISAN_FLAG(Type, Name, DefaultValue, Description) \
  Type Name{DefaultValue};
#include "mpisan_flags.inc"
#undef MPISAN_FLAG
};

extern Flags flags_data;
inline Flags &flags() { return flags_data; }

void InitializeFlags();

} // namespace __mpisan

//===-- mpisan_flags.cpp - MPI Usage Sanitizer --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Runtime flag parsing for MPISan. Flags are read from the MPISAN_OPTIONS
// environment variable using the sanitizer_common flag parser.
//===----------------------------------------------------------------------===//

#include "mpisan/mpisan_flags.h"

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_flag_parser.h"
#include "sanitizer_common/sanitizer_flags.h"

using namespace __sanitizer;

namespace __mpisan {

Flags flags_data;

/// Register all MPISan flags with the common flag parser.
static void RegisterMpisanFlags(FlagParser *parser, Flags *f) {
#define MPISAN_FLAG(Type, Name, DefaultValue, Description) \
  RegisterFlag(parser, #Name, Description, &f->Name);
#include "mpisan/mpisan_flags.inc"
#undef MPISAN_FLAG
}

void InitializeFlags() {
  Flags *f = &flags_data;

  // Apply defaults.
#define MPISAN_FLAG(Type, Name, DefaultValue, Description) \
  f->Name = DefaultValue;
#include "mpisan/mpisan_flags.inc"
#undef MPISAN_FLAG

  FlagParser parser;
  RegisterMpisanFlags(&parser, f);
  RegisterCommonFlags(&parser);

  // Parse from environment variable.
  parser.ParseStringFromEnv("MPISAN_OPTIONS");

  // Validate flag combinations.
  if (f->lightweight_mode) {
    // In lightweight mode, disable the heavier checks.
    f->check_buffer_aliasing = false;
    f->check_deadlock = false;
    f->check_collective_mismatch = false;
  }

  InitializeCommonFlags();

  if (Verbosity())
    ReportUnrecognizedFlags();

  if (common_flags()->help)
    parser.PrintFlagDescriptions();
}

} // namespace __mpisan

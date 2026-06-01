//===-- mpisan_suppressions.h - MPI Usage Sanitizer -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Suppressions support for MPISan. Allows users to suppress known-false
// positives via a suppressions file specified with MPISAN_OPTIONS=suppressions=.
//===----------------------------------------------------------------------===//

#pragma once

namespace __mpisan {

/// Initialize the suppressions system. Must be called after InitializeFlags().
void InitializeSuppressions();

/// Returns true if the given error at the given function name should be
/// suppressed.
bool IsSuppressed(const char *error_type, const char *func_name);

} // namespace __mpisan

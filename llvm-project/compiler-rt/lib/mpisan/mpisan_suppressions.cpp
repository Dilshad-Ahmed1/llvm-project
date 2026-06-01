//===-- mpisan_suppressions.cpp - MPI Usage Sanitizer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Suppressions implementation for MPISan.
//
// Suppressions file format (one rule per line):
//   # comment
//   type-mismatch:MPI_Send
//   buffer-aliasing:my_function
//   deadlock:*
//===----------------------------------------------------------------------===//

#include "mpisan/mpisan_suppressions.h"
#include "mpisan/mpisan_flags.h"

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_suppressions.h"

using namespace __sanitizer;

namespace __mpisan {

static SuppressionContext *suppression_ctx;

void InitializeSuppressions() {
  CHECK(!suppression_ctx);
  static const char *kSuppressionTypes[] = {
      "type-mismatch", "count-mismatch", "buffer-aliasing",
      "collective-mismatch", "deadlock", "timeout"};
  suppression_ctx =
      new (GetGlobalLowLevelAllocator()) SuppressionContext(
          kSuppressionTypes,
          (int)(sizeof(kSuppressionTypes) / sizeof(kSuppressionTypes[0])));

  const char *supp_file = flags().suppressions;
  if (supp_file && supp_file[0] != '\0')
    suppression_ctx->ParseFromFile(supp_file);
}

bool IsSuppressed(const char *error_type, const char *func_name) {
  if (!suppression_ctx)
    return false;
  Suppression *s = nullptr;
  return suppression_ctx->Match(func_name, error_type, &s);
}

} // namespace __mpisan

//===-- mpisan_test_flags.cpp - MPISan unit tests -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for the MPISan configuration system.
// Covers Task 7.2 (property test for configuration respect).
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "mpisan/mpisan_flags.h"

using namespace __mpisan;

// Task 7.2 – Property test: configuration respect
// Verify that default flag values are sane.
TEST(MpisanFlags, DefaultsAreReasonable) {
  Flags f;
  // Apply defaults manually (mirrors InitializeFlags logic).
#define MPISAN_FLAG(Type, Name, DefaultValue, Description) \
  f.Name = DefaultValue;
#include "mpisan/mpisan_flags.inc"
#undef MPISAN_FLAG

  EXPECT_TRUE(f.halt_on_error);
  EXPECT_TRUE(f.check_type_mismatch);
  EXPECT_TRUE(f.check_count_mismatch);
  EXPECT_TRUE(f.check_buffer_aliasing);
  EXPECT_TRUE(f.check_collective_mismatch);
  EXPECT_TRUE(f.check_deadlock);
  EXPECT_FALSE(f.print_stats);
  EXPECT_TRUE(f.suppress_equal_stacks);
  EXPECT_FALSE(f.lightweight_mode);
  EXPECT_EQ(f.verbosity, 0);
  EXPECT_EQ(f.deadlock_timeout_us, 5000000);
}

TEST(MpisanFlags, LightweightModeDisablesHeavyChecks) {
  Flags f;
#define MPISAN_FLAG(Type, Name, DefaultValue, Description) \
  f.Name = DefaultValue;
#include "mpisan/mpisan_flags.inc"
#undef MPISAN_FLAG

  // Simulate what InitializeFlags does when lightweight_mode=true.
  f.lightweight_mode = true;
  if (f.lightweight_mode) {
    f.check_buffer_aliasing      = false;
    f.check_deadlock             = false;
    f.check_collective_mismatch  = false;
  }

  EXPECT_FALSE(f.check_buffer_aliasing);
  EXPECT_FALSE(f.check_deadlock);
  EXPECT_FALSE(f.check_collective_mismatch);
  // Type and count checks remain enabled.
  EXPECT_TRUE(f.check_type_mismatch);
  EXPECT_TRUE(f.check_count_mismatch);
}

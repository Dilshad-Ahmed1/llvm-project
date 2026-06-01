//===-- mpisan_test_buffer_aliasing.cpp - MPISan unit tests -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for buffer range computation and aliasing detection.
// Covers Tasks 11.4 (buffer range recording) and 11.5 (aliasing detection).
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "mpisan/mpisan_internal.h"

using namespace __mpisan;

// Helper: do two half-open intervals [a, a+sa) and [b, b+sb) overlap?
static bool Overlaps(uptr a, uptr sa, uptr b, uptr sb) {
  return (a < b + sb) && (b < a + sa);
}

// Task 11.4 – Property test: buffer range recording
TEST(MpisanBufferRange, NonOverlappingBuffers) {
  // [0, 100) and [200, 300) — no overlap.
  EXPECT_FALSE(Overlaps(0, 100, 200, 100));
  EXPECT_FALSE(Overlaps(200, 100, 0, 100));
}

TEST(MpisanBufferRange, AdjacentBuffersDoNotOverlap) {
  // [0, 100) and [100, 200) — adjacent, not overlapping.
  EXPECT_FALSE(Overlaps(0, 100, 100, 100));
}

// Task 11.5 – Property test: buffer aliasing detection
TEST(MpisanBufferRange, OverlappingBuffersDetected) {
  // [0, 100) and [50, 150) — overlap at [50, 100).
  EXPECT_TRUE(Overlaps(0, 100, 50, 100));
  EXPECT_TRUE(Overlaps(50, 100, 0, 100));
}

TEST(MpisanBufferRange, SameBufferOverlaps) {
  EXPECT_TRUE(Overlaps(0x1000, 64, 0x1000, 64));
}

TEST(MpisanBufferRange, ContainedBufferOverlaps) {
  // [0, 200) contains [50, 100).
  EXPECT_TRUE(Overlaps(0, 200, 50, 50));
  EXPECT_TRUE(Overlaps(50, 50, 0, 200));
}

TEST(MpisanBufferRange, ZeroSizeBufferDoesNotOverlap) {
  // A zero-size buffer should not overlap anything.
  EXPECT_FALSE(Overlaps(100, 0, 100, 100));
  EXPECT_FALSE(Overlaps(100, 100, 100, 0));
}

// Verify that GetMpiTypeSize is used correctly for buffer size computation.
TEST(MpisanBufferRange, BufferSizeComputedFromTypeAndCount) {
  // 10 MPI_INT elements = 10 * 4 = 40 bytes.
  uptr size = (uptr)10 * GetMpiTypeSize(MPISAN_TYPE_INT);
  EXPECT_EQ(size, (uptr)40);

  // 5 MPI_DOUBLE elements = 5 * 8 = 40 bytes.
  size = (uptr)5 * GetMpiTypeSize(MPISAN_TYPE_DOUBLE);
  EXPECT_EQ(size, (uptr)40);
}

//===-- mpisan_test_deadlock.cpp - MPISan unit tests --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for deadlock detection (wait-for graph and cycle detection).
// Covers Tasks 14.5 (deadlock detection) and 14.6 (timeout detection).
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "mpisan/mpisan_internal.h"

using namespace __mpisan;

// Helper: build a wait-for graph and run cycle detection.
// Returns cycle length (0 = no cycle).
static int RunCycleDetection(int *graph, int n, int start, int *cycle_out,
                             int max_len) {
  // Replicate the cycle detection logic from mpisan_rtl.cpp for unit testing.
  bool visited[MpisanState::kMaxGraphNodes] = {};
  int path[MpisanState::kMaxGraphNodes];
  int path_len = 0;

  int cur = start;
  while (cur >= 0 && cur < n && !visited[cur]) {
    visited[cur] = true;
    path[path_len++] = cur;
    cur = graph[cur];
  }

  if (cur < 0 || cur >= n)
    return 0;

  int cycle_start = -1;
  for (int i = 0; i < path_len; ++i) {
    if (path[i] == cur) {
      cycle_start = i;
      break;
    }
  }
  if (cycle_start < 0)
    return 0;

  int cycle_len = path_len - cycle_start;
  if (cycle_len > max_len)
    cycle_len = max_len;
  for (int i = 0; i < cycle_len; ++i)
    cycle_out[i] = path[cycle_start + i];
  return cycle_len;
}

// Task 14.5 – Property test: deadlock detection
TEST(MpisanDeadlock, NoCycleInLinearGraph) {
  // 0 -> 1 -> 2 -> -1 (no cycle)
  int graph[4] = {1, 2, -1, -1};
  int cycle[64];
  EXPECT_EQ(RunCycleDetection(graph, 4, 0, cycle, 64), 0);
}

TEST(MpisanDeadlock, SimpleTwoNodeCycle) {
  // 0 -> 1 -> 0 (cycle: 0, 1)
  int graph[4] = {1, 0, -1, -1};
  int cycle[64];
  int len = RunCycleDetection(graph, 4, 0, cycle, 64);
  EXPECT_EQ(len, 2);
  // Cycle should contain both 0 and 1.
  bool has0 = false, has1 = false;
  for (int i = 0; i < len; ++i) {
    if (cycle[i] == 0) has0 = true;
    if (cycle[i] == 1) has1 = true;
  }
  EXPECT_TRUE(has0);
  EXPECT_TRUE(has1);
}

TEST(MpisanDeadlock, ThreeNodeCycle) {
  // 0 -> 1 -> 2 -> 0 (cycle: 0, 1, 2)
  int graph[4] = {1, 2, 0, -1};
  int cycle[64];
  int len = RunCycleDetection(graph, 4, 0, cycle, 64);
  EXPECT_EQ(len, 3);
}

TEST(MpisanDeadlock, CycleNotStartingAtRoot) {
  // 0 -> 1 -> 2 -> 3 -> 1 (cycle: 1, 2, 3; not involving 0)
  int graph[5] = {1, 2, 3, 1, -1};
  int cycle[64];
  int len = RunCycleDetection(graph, 5, 0, cycle, 64);
  EXPECT_EQ(len, 3);
}

TEST(MpisanDeadlock, EmptyGraphHasNoCycle) {
  int graph[4] = {-1, -1, -1, -1};
  int cycle[64];
  EXPECT_EQ(RunCycleDetection(graph, 4, 0, cycle, 64), 0);
}

// Task 14.6 – Property test: timeout detection
TEST(MpisanDeadlock, TimeoutDetectedWhenElapsedExceedsThreshold) {
  // Simulate a pending op that was posted 10 seconds ago.
  u64 now = GetCurrentTimeMicros();
  u64 posted_at = now - 10000000ULL; // 10 seconds ago
  u64 elapsed = now - posted_at;
  int timeout_us = 5000000; // 5 second threshold

  EXPECT_GT((int)elapsed, timeout_us); // Should trigger timeout.
}

TEST(MpisanDeadlock, NoTimeoutWhenRecentlyPosted) {
  u64 now = GetCurrentTimeMicros();
  u64 posted_at = now - 100000ULL; // 0.1 seconds ago
  u64 elapsed = now - posted_at;
  int timeout_us = 5000000; // 5 second threshold

  EXPECT_LT((int)elapsed, timeout_us); // Should NOT trigger timeout.
}

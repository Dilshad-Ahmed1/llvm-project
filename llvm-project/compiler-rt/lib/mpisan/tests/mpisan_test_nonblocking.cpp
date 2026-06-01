//===-- mpisan_test_nonblocking.cpp - MPISan unit tests -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for non-blocking MPI operation tracking.
// Covers Task 17.3 (unit tests for non-blocking operations).
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"
#include "mpisan/mpisan_internal.h"

using namespace __mpisan;

class NonBlockingTest : public ::testing::Test {
protected:
  void SetUp() override {
    MpisanState &s = GetMpisanState();
    for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i)
      s.pending_ops[i].active = false;
    s.pending_ops_count = 0;
  }
};

TEST_F(NonBlockingTest, IsendRecordedWithRequestId) {
  PendingOp *op = AllocPendingOp();
  ASSERT_NE(op, nullptr);
  op->kind       = PendingOp::Kind::ISend;
  op->rank       = 1;
  op->tag        = 5;
  op->comm       = 0;
  op->datatype   = MPISAN_TYPE_FLOAT;
  op->count      = 64;
  op->request_id = 0xABCD1234ULL;

  EXPECT_EQ(op->kind,       PendingOp::Kind::ISend);
  EXPECT_EQ(op->request_id, 0xABCD1234ULL);
}

TEST_F(NonBlockingTest, IrecvRecordedWithRequestId) {
  PendingOp *op = AllocPendingOp();
  ASSERT_NE(op, nullptr);
  op->kind       = PendingOp::Kind::IRecv;
  op->rank       = 0;
  op->tag        = 10;
  op->comm       = 0;
  op->datatype   = MPISAN_TYPE_DOUBLE;
  op->count      = 32;
  op->request_id = 0xDEAD5678ULL;

  EXPECT_EQ(op->kind,       PendingOp::Kind::IRecv);
  EXPECT_EQ(op->request_id, 0xDEAD5678ULL);
}

TEST_F(NonBlockingTest, FreePendingOpByRequestIdRemovesCorrectSlot) {
  // Allocate two ops with different request IDs.
  PendingOp *op1 = AllocPendingOp();
  PendingOp *op2 = AllocPendingOp();
  ASSERT_NE(op1, nullptr);
  ASSERT_NE(op2, nullptr);

  op1->request_id = 111ULL;
  op2->request_id = 222ULL;

  FreePendingOpByRequestId(111ULL);

  EXPECT_FALSE(op1->active);
  EXPECT_TRUE(op2->active);
  EXPECT_EQ(GetMpisanState().pending_ops_count, 1);
}

TEST_F(NonBlockingTest, FreePendingOpByRequestIdNoOpWhenNotFound) {
  PendingOp *op = AllocPendingOp();
  ASSERT_NE(op, nullptr);
  op->request_id = 999ULL;

  // Freeing a non-existent request ID should not crash or corrupt state.
  FreePendingOpByRequestId(0xFFFFFFFFULL);
  EXPECT_TRUE(op->active);
  EXPECT_EQ(GetMpisanState().pending_ops_count, 1);
}

TEST_F(NonBlockingTest, MultipleIsendIrecvTrackedIndependently) {
  const int N = 8;
  u64 ids[N];
  for (int i = 0; i < N; ++i) {
    PendingOp *op = AllocPendingOp();
    ASSERT_NE(op, nullptr);
    op->kind       = (i % 2 == 0) ? PendingOp::Kind::ISend
                                   : PendingOp::Kind::IRecv;
    op->request_id = (u64)(i + 1) * 100;
    ids[i]         = op->request_id;
  }
  EXPECT_EQ(GetMpisanState().pending_ops_count, N);

  // Free them in reverse order.
  for (int i = N - 1; i >= 0; --i) {
    FreePendingOpByRequestId(ids[i]);
    EXPECT_EQ(GetMpisanState().pending_ops_count, i);
  }
}

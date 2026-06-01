//===-- mpisan_report.h - MPI Usage Sanitizer -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Error reporting interface for MPISan.
//===----------------------------------------------------------------------===//

#pragma once

#include "mpisan/mpisan_internal.h"
#include "sanitizer_common/sanitizer_stacktrace.h"

namespace __mpisan {

/// Categories of MPI usage errors that MPISan can detect.
enum class MpisanErrorKind {
  TypeMismatch,
  CountMismatch,
  BufferAliasing,
  CollectiveMismatch,
  CollectiveParamMismatch,
  Deadlock,
  Timeout,
};

/// Detailed information about a type/count mismatch error.
struct TypeMismatchInfo {
  int send_rank;
  int recv_rank;
  int tag;
  u32 send_type;
  u32 recv_type;
  int send_count;
  int recv_count;
};

/// Detailed information about a buffer aliasing error.
struct BufferAliasingInfo {
  uptr buf1_addr;
  uptr buf1_size;
  uptr buf2_addr;
  uptr buf2_size;
  const char *op1_name; ///< e.g. "MPI_Send"
  const char *op2_name; ///< e.g. "MPI_Recv"
};

/// Detailed information about a collective mismatch error.
struct CollectiveMismatchInfo {
  int comm;
  int rank;
  int expected_op;   ///< MPISAN_COLL_* constant
  int actual_op;     ///< MPISAN_COLL_* constant
  int expected_root; ///< -1 if not applicable
  int actual_root;
  int expected_count;
  int actual_count;
};

/// Detailed information about a deadlock or timeout error.
struct DeadlockInfo {
  int num_ranks_in_cycle;
  int cycle[64]; ///< Rank IDs forming the wait cycle
  bool is_timeout;
  u64 wait_duration_us;
};

/// Report a type or count mismatch between a send and receive.
void ReportTypeMismatch(const TypeMismatchInfo &info,
                        const __sanitizer::BufferedStackTrace &stack);

/// Report overlapping send/receive buffers.
void ReportBufferAliasing(const BufferAliasingInfo &info,
                          const __sanitizer::BufferedStackTrace &stack);

/// Report a collective operation mismatch.
void ReportCollectiveMismatch(const CollectiveMismatchInfo &info,
                              const __sanitizer::BufferedStackTrace &stack);

/// Report a detected deadlock or timeout.
void ReportDeadlock(const DeadlockInfo &info,
                    const __sanitizer::BufferedStackTrace &stack);

/// Print a summary of all errors detected during the run.
void PrintErrorSummary();

/// Returns the human-readable name for an MPI collective op code.
const char *CollectiveOpName(int op);

/// Returns the human-readable name for an MPI datatype.
const char *MpiTypeName(u32 type_id);

} // namespace __mpisan

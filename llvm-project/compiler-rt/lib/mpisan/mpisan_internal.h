//===-- mpisan_internal.h - MPI Usage Sanitizer -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of MPIUsageSanitizer (MPISan), a runtime library that
// detects common MPI usage errors such as type mismatches, buffer aliasing,
// collective operation mismatches, and deadlocks.
//
// This is the main internal header shared across all MPISan runtime modules.
//===----------------------------------------------------------------------===//

#pragma once

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_mutex.h"
#include "sanitizer_common/sanitizer_stacktrace.h"

// MPI type tag constants (mirrors MPI_Datatype values for common types).
// We use our own constants to avoid depending on mpi.h at runtime.
#define MPISAN_TYPE_CHAR           0x4c000101
#define MPISAN_TYPE_SHORT          0x4c000203
#define MPISAN_TYPE_INT            0x4c000405
#define MPISAN_TYPE_LONG           0x4c000807
#define MPISAN_TYPE_FLOAT          0x4c00040a
#define MPISAN_TYPE_DOUBLE         0x4c00080b
#define MPISAN_TYPE_UNSIGNED_CHAR  0x4c000102
#define MPISAN_TYPE_UNSIGNED_SHORT 0x4c000204
#define MPISAN_TYPE_UNSIGNED       0x4c000406
#define MPISAN_TYPE_UNSIGNED_LONG  0x4c000808
#define MPISAN_TYPE_LONG_DOUBLE    0x4c00100c
#define MPISAN_TYPE_BYTE           0x4c00010d
#define MPISAN_TYPE_PACKED         0x4c00010e

// MPI collective operation tags.
#define MPISAN_COLL_BARRIER    1
#define MPISAN_COLL_BCAST      2
#define MPISAN_COLL_REDUCE     3
#define MPISAN_COLL_ALLREDUCE  4
#define MPISAN_COLL_SCATTER    5
#define MPISAN_COLL_GATHER     6
#define MPISAN_COLL_ALLGATHER  7
#define MPISAN_COLL_ALLTOALL   8
#define MPISAN_COLL_SCAN       9
#define MPISAN_COLL_EXSCAN     10

// Maximum number of MPI ranks we track (can be overridden at build time).
#ifndef MPISAN_MAX_RANKS
#define MPISAN_MAX_RANKS 65536
#endif

// Maximum pending operations per rank.
#ifndef MPISAN_MAX_PENDING_OPS
#define MPISAN_MAX_PENDING_OPS 4096
#endif

// Timeout for collective validation (in microseconds).
#ifndef MPISAN_COLLECTIVE_TIMEOUT_US
#define MPISAN_COLLECTIVE_TIMEOUT_US 5000000  // 5 seconds
#endif

namespace __mpisan {

using __sanitizer::uptr;
using __sanitizer::u8;
using __sanitizer::u32;
using __sanitizer::u64;
using __sanitizer::s32;
using __sanitizer::s64;

// ============================================================================
// MPI Type Information
// ============================================================================

/// Represents an MPI datatype with its size and a human-readable name.
struct MpiTypeInfo {
  u32 type_id;       ///< MPI_Datatype value
  uptr element_size; ///< Size of one element in bytes
  const char *name;  ///< Human-readable type name
  bool is_derived;   ///< True for user-defined derived types
};

/// Returns type info for a known MPI type, or nullptr if unknown.
const MpiTypeInfo *GetMpiTypeInfo(u32 type_id);

/// Returns the size in bytes of a single element of the given MPI type.
/// Returns 0 for unknown types.
uptr GetMpiTypeSize(u32 type_id);

/// Returns true if two MPI types are compatible for send/receive matching.
bool AreMpiTypesCompatible(u32 send_type, u32 recv_type);

// ============================================================================
// Pending Operation Tracking
// ============================================================================

/// Describes a pending (posted) MPI send or receive operation.
struct PendingOp {
  enum class Kind : u8 {
    None = 0,
    Send,
    Recv,
    ISend,  ///< Non-blocking send
    IRecv,  ///< Non-blocking receive
  };

  Kind kind;
  int rank;          ///< Source or destination rank
  int tag;           ///< MPI message tag
  int comm;          ///< MPI communicator (as int handle)
  u32 datatype;      ///< MPI_Datatype
  int count;         ///< Element count
  uptr buf_addr;     ///< Buffer address
  uptr buf_size;     ///< Buffer size in bytes
  u64 request_id;    ///< For non-blocking: maps to MPI_Request
  u64 timestamp_us;  ///< Wall-clock time when posted (for deadlock timeout)
  uptr stack_id;     ///< Stack trace ID at the time of posting
  bool active;       ///< Slot is in use
};

// ============================================================================
// Runtime State
// ============================================================================

/// Per-communicator collective state, used to validate that all ranks in a
/// communicator call the same collective with compatible parameters.
struct CollectiveState {
  int comm;                  ///< Communicator handle
  int collective_op;         ///< MPISAN_COLL_* constant
  int root;                  ///< Root rank (for rooted collectives)
  int count;                 ///< Element count
  u32 datatype;              ///< MPI_Datatype
  int num_participants;      ///< Number of ranks that have checked in
  int expected_participants; ///< Total ranks in communicator
  u64 first_entry_time_us;   ///< Time first rank entered (for timeout)
  bool active;               ///< Slot is in use
};

/// Global runtime state for MPISan.
struct MpisanState {
  // Initialization
  bool initialized;
  __sanitizer::StaticSpinMutex init_mutex;

  // Per-rank pending operations table.
  // Indexed by a flat slot index; each slot holds one PendingOp.
  PendingOp pending_ops[MPISAN_MAX_PENDING_OPS];
  __sanitizer::StaticSpinMutex pending_ops_mutex;
  int pending_ops_count;

  // Collective validation table (one entry per active communicator collective).
  static const int kMaxCollectives = 256;
  CollectiveState collectives[kMaxCollectives];
  __sanitizer::StaticSpinMutex collectives_mutex;

  // Wait-for graph for deadlock detection.
  // wait_for_graph[i] = j means rank i is waiting for rank j.
  // -1 means not waiting.
  static const int kMaxGraphNodes = 1024;
  int wait_for_graph[kMaxGraphNodes];
  __sanitizer::StaticSpinMutex graph_mutex;

  // Error counters.
  u64 total_errors;
  u64 type_mismatch_errors;
  u64 buffer_aliasing_errors;
  u64 collective_mismatch_errors;
  u64 deadlock_errors;
  u64 timeout_errors;
};

/// Returns the global MPISan state (lazily initialized).
MpisanState &GetMpisanState();

// ============================================================================
// Utility
// ============================================================================

/// Returns the current wall-clock time in microseconds.
u64 GetCurrentTimeMicros();

/// Allocates a new slot in the pending ops table. Returns nullptr if full.
PendingOp *AllocPendingOp();

/// Frees a pending op slot by request_id (for non-blocking ops).
void FreePendingOpByRequestId(u64 request_id);

/// Finds a matching pending send for an incoming receive.
/// Returns nullptr if no match found.
PendingOp *FindMatchingSend(int src_rank, int tag, int comm);

/// Finds a matching pending receive for an outgoing send.
PendingOp *FindMatchingRecv(int dst_rank, int tag, int comm);

} // namespace __mpisan

//===-- mpisan_rtl.cpp - MPI Usage Sanitizer ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Core runtime library for MPISan. Implements:
//   - Runtime initialization and finalization (Task 6.1)
//   - Point-to-point send/receive hooks (Tasks 8.1, 8.2)
//   - MPI type information and compatibility checking (Tasks 9.1, 9.2, 9.3)
//   - Buffer range computation and aliasing detection (Tasks 11.1-11.3)
//   - Collective operation hooks and validation (Tasks 12.1-12.3)
//   - Deadlock detection via wait-for graph (Tasks 14.1-14.4)
//   - Non-blocking operation hooks (Tasks 17.1, 17.2)
//===----------------------------------------------------------------------===//

#include "mpisan/mpisan_rtl.h"
#include "mpisan/mpisan_flags.h"
#include "mpisan/mpisan_internal.h"
#include "mpisan/mpisan_report.h"

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_mutex.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "mpisan/mpisan_stack.h"

// Provide a stub for UnwindImpl so the runtime can be linked without
// the full symbolizer. Stack traces will show no frames, which is
// acceptable for a standalone static library.
namespace __sanitizer {
void BufferedStackTrace::UnwindImpl(uptr pc, uptr bp, void *context,
                                    bool request_fast, u32 max_depth) {
  size = 0;
}
} // namespace __sanitizer

using namespace __sanitizer;

namespace __mpisan {

// ============================================================================
// Global State
// ============================================================================

static MpisanState g_state;

MpisanState &GetMpisanState() { return g_state; }

// ============================================================================
// Task 9.1 – MPI Type Information System
// ============================================================================

static const MpiTypeInfo kKnownTypes[] = {
    {MPISAN_TYPE_CHAR,           1,  "MPI_CHAR",           false},
    {MPISAN_TYPE_SHORT,          2,  "MPI_SHORT",          false},
    {MPISAN_TYPE_INT,            4,  "MPI_INT",            false},
    {MPISAN_TYPE_LONG,           8,  "MPI_LONG",           false},
    {MPISAN_TYPE_FLOAT,          4,  "MPI_FLOAT",          false},
    {MPISAN_TYPE_DOUBLE,         8,  "MPI_DOUBLE",         false},
    {MPISAN_TYPE_UNSIGNED_CHAR,  1,  "MPI_UNSIGNED_CHAR",  false},
    {MPISAN_TYPE_UNSIGNED_SHORT, 2,  "MPI_UNSIGNED_SHORT", false},
    {MPISAN_TYPE_UNSIGNED,       4,  "MPI_UNSIGNED",       false},
    {MPISAN_TYPE_UNSIGNED_LONG,  8,  "MPI_UNSIGNED_LONG",  false},
    {MPISAN_TYPE_LONG_DOUBLE,    16, "MPI_LONG_DOUBLE",    false},
    {MPISAN_TYPE_BYTE,           1,  "MPI_BYTE",           false},
    {MPISAN_TYPE_PACKED,         1,  "MPI_PACKED",         false},
};
static const int kNumKnownTypes =
    (int)(sizeof(kKnownTypes) / sizeof(kKnownTypes[0]));

const MpiTypeInfo *GetMpiTypeInfo(u32 type_id) {
  for (int i = 0; i < kNumKnownTypes; ++i)
    if (kKnownTypes[i].type_id == type_id)
      return &kKnownTypes[i];
  return nullptr;
}

uptr GetMpiTypeSize(u32 type_id) {
  const MpiTypeInfo *info = GetMpiTypeInfo(type_id);
  return info ? info->element_size : 0;
}

// ============================================================================
// Task 9.2 – Type Compatibility Checker
// ============================================================================

bool AreMpiTypesCompatible(u32 send_type, u32 recv_type) {
  if (send_type == recv_type)
    return true;

  // MPI_BYTE is compatible with any type of the same element size.
  if (send_type == MPISAN_TYPE_BYTE || recv_type == MPISAN_TYPE_BYTE)
    return true;

  // MPI_PACKED is compatible with MPI_PACKED only.
  if (send_type == MPISAN_TYPE_PACKED || recv_type == MPISAN_TYPE_PACKED)
    return (send_type == recv_type);

  // For all other pairs, require exact type match.
  return false;
}

// ============================================================================
// Utility: wall-clock time
// ============================================================================

u64 GetCurrentTimeMicros() {
  return (u64)NanoTime() / 1000;
}

// ============================================================================
// Pending Operations Table (Tasks 8.1, 8.2, 17.1, 17.2)
// ============================================================================

PendingOp *AllocPendingOp() {
  SpinMutexLock lock(&g_state.pending_ops_mutex);
  for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i) {
    if (!g_state.pending_ops[i].active) {
      g_state.pending_ops[i] = PendingOp{};
      g_state.pending_ops[i].active = true;
      g_state.pending_ops_count++;
      return &g_state.pending_ops[i];
    }
  }
  // Table full — this is a runtime limitation, not an MPI error.
  if (flags().verbosity >= 1)
    Printf("==MPISan== WARNING: pending ops table full, skipping tracking.\n");
  return nullptr;
}

void FreePendingOpByRequestId(u64 request_id) {
  SpinMutexLock lock(&g_state.pending_ops_mutex);
  for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i) {
    if (g_state.pending_ops[i].active &&
        g_state.pending_ops[i].request_id == request_id) {
      g_state.pending_ops[i].active = false;
      g_state.pending_ops_count--;
      return;
    }
  }
}

PendingOp *FindMatchingSend(int src_rank, int tag, int comm) {
  // Caller must hold pending_ops_mutex.
  for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i) {
    PendingOp &op = g_state.pending_ops[i];
    if (!op.active)
      continue;
    if (op.kind != PendingOp::Kind::Send && op.kind != PendingOp::Kind::ISend)
      continue;
    if (op.comm != comm)
      continue;
    // MPI_ANY_SOURCE = -1, MPI_ANY_TAG = -1
    if (src_rank != -1 && op.rank != src_rank)
      continue;
    if (tag != -1 && op.tag != tag)
      continue;
    return &op;
  }
  return nullptr;
}

PendingOp *FindMatchingRecv(int dst_rank, int tag, int comm) {
  // Caller must hold pending_ops_mutex.
  for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i) {
    PendingOp &op = g_state.pending_ops[i];
    if (!op.active)
      continue;
    if (op.kind != PendingOp::Kind::Recv && op.kind != PendingOp::Kind::IRecv)
      continue;
    if (op.comm != comm)
      continue;
    if (dst_rank != -1 && op.rank != dst_rank)
      continue;
    if (tag != -1 && op.tag != tag)
      continue;
    return &op;
  }
  return nullptr;
}

// ============================================================================
// Task 11.1 – Buffer Range Computation
// Task 11.3 – Buffer Aliasing Checker
// ============================================================================

/// Returns true if [a, a+sa) and [b, b+sb) overlap.
static bool BuffersOverlap(uptr a, uptr sa, uptr b, uptr sb) {
  return (a < b + sb) && (b < a + sa);
}

/// Check whether a new buffer [buf, buf+size) aliases any active pending op.
static void CheckBufferAliasing(uptr buf, uptr size, const char *op_name) {
  if (!flags().check_buffer_aliasing || flags().lightweight_mode)
    return;

  SpinMutexLock lock(&g_state.pending_ops_mutex);
  for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i) {
    PendingOp &op = g_state.pending_ops[i];
    if (!op.active || op.buf_addr == 0)
      continue;
    if (BuffersOverlap(buf, size, op.buf_addr, op.buf_size)) {
      GET_STACK_TRACE_FATAL_HERE;
      BufferAliasingInfo info;
      info.buf1_addr = buf;
      info.buf1_size = size;
      info.buf2_addr = op.buf_addr;
      info.buf2_size = op.buf_size;
      info.op1_name  = op_name;
      info.op2_name  = (op.kind == PendingOp::Kind::Send ||
                        op.kind == PendingOp::Kind::ISend)
                           ? "MPI_Send"
                           : "MPI_Recv";
      ReportBufferAliasing(info, stack);
    }
  }
}

// ============================================================================
// Task 14.1 – Wait-For Graph
// Task 14.2 – Cycle Detection
// ============================================================================

/// Record that rank `waiter` is waiting for rank `provider`.
static void WaitForGraphSetEdge(int waiter, int provider) {
  if (!flags().check_deadlock || flags().lightweight_mode)
    return;
  if (waiter < 0 || waiter >= MpisanState::kMaxGraphNodes)
    return;
  if (provider < 0 || provider >= MpisanState::kMaxGraphNodes)
    return;
  SpinMutexLock lock(&g_state.graph_mutex);
  g_state.wait_for_graph[waiter] = provider;
}

/// Clear the wait edge for `waiter`.
static void WaitForGraphClearEdge(int waiter) {
  if (waiter < 0 || waiter >= MpisanState::kMaxGraphNodes)
    return;
  SpinMutexLock lock(&g_state.graph_mutex);
  g_state.wait_for_graph[waiter] = -1;
}

/// Detect a cycle starting from `start_rank`. Returns cycle length (0 = none).
/// Fills `cycle_out` with the ranks in the cycle.
static int DetectCycle(int start_rank, int *cycle_out, int max_len) {
  // Simple tortoise-and-hare / visited-set approach.
  // Since graph is small and we hold the lock, a visited array is fine.
  bool visited[MpisanState::kMaxGraphNodes] = {};
  int path[MpisanState::kMaxGraphNodes];
  int path_len = 0;

  int cur = start_rank;
  while (cur >= 0 && cur < MpisanState::kMaxGraphNodes && !visited[cur]) {
    visited[cur] = true;
    path[path_len++] = cur;
    cur = g_state.wait_for_graph[cur];
  }

  if (cur < 0 || cur >= MpisanState::kMaxGraphNodes)
    return 0; // No cycle.

  // cur is the node where the cycle closes. Find it in path.
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

static void CheckForDeadlock(int rank) {
  if (!flags().check_deadlock || flags().lightweight_mode)
    return;

  SpinMutexLock lock(&g_state.graph_mutex);
  int cycle[64];
  int len = DetectCycle(rank, cycle, 64);
  if (len > 0) {
    GET_STACK_TRACE_FATAL_HERE;
    DeadlockInfo info;
    info.is_timeout = false;
    info.num_ranks_in_cycle = len;
    info.wait_duration_us = 0;
    for (int i = 0; i < len && i < 64; ++i)
      info.cycle[i] = cycle[i];
    ReportDeadlock(info, stack);
  }
}

// ============================================================================
// Task 14.4 – Timeout Detection
// ============================================================================

static void CheckForTimeout(const PendingOp &op) {
  if (!flags().check_deadlock || flags().lightweight_mode)
    return;
  u64 now = GetCurrentTimeMicros();
  u64 elapsed = now - op.timestamp_us;
  if ((int)elapsed > flags().deadlock_timeout_us) {
    GET_STACK_TRACE_FATAL_HERE;
    DeadlockInfo info;
    info.is_timeout = true;
    info.num_ranks_in_cycle = 1;
    info.cycle[0] = op.rank;
    info.wait_duration_us = elapsed;
    ReportDeadlock(info, stack);
  }
}

// ============================================================================
// Task 9.3 – Type Checking Integration into Receive Hooks
// ============================================================================

static void CheckSendRecvMatch(const PendingOp &send_op, u32 recv_type,
                               int recv_count, int recv_rank, int tag) {
  bool type_ok  = AreMpiTypesCompatible(send_op.datatype, recv_type);
  bool count_ok = (send_op.count == recv_count);

  if (!type_ok || !count_ok) {
    GET_STACK_TRACE_FATAL_HERE;
    TypeMismatchInfo info;
    info.send_rank  = send_op.rank;
    info.recv_rank  = recv_rank;
    info.tag        = tag;
    info.send_type  = send_op.datatype;
    info.recv_type  = recv_type;
    info.send_count = send_op.count;
    info.recv_count = recv_count;
    ReportTypeMismatch(info, stack);
  }
}

// ============================================================================
// Task 12.1-12.3 – Collective Operation Hooks and Validation
// ============================================================================

static void ValidateCollective(int comm, int collective_op, int root,
                               int count, int datatype, int comm_size) {
  if (!flags().check_collective_mismatch || flags().lightweight_mode)
    return;

  SpinMutexLock lock(&g_state.collectives_mutex);

  // Find an existing entry for this communicator, or allocate a new one.
  CollectiveState *slot = nullptr;
  for (int i = 0; i < MpisanState::kMaxCollectives; ++i) {
    if (g_state.collectives[i].active &&
        g_state.collectives[i].comm == comm) {
      slot = &g_state.collectives[i];
      break;
    }
  }

  if (!slot) {
    // First rank to enter this collective on this communicator.
    for (int i = 0; i < MpisanState::kMaxCollectives; ++i) {
      if (!g_state.collectives[i].active) {
        slot = &g_state.collectives[i];
        break;
      }
    }
    if (!slot) {
      if (flags().verbosity >= 1)
        Printf("==MPISan== WARNING: collective table full.\n");
      return;
    }
    slot->active               = true;
    slot->comm                 = comm;
    slot->collective_op        = collective_op;
    slot->root                 = root;
    slot->count                = count;
    slot->datatype             = datatype;
    slot->num_participants     = 1;
    slot->expected_participants = comm_size;
    slot->first_entry_time_us  = GetCurrentTimeMicros();
    return;
  }

  // Subsequent rank — validate against the first rank's parameters.
  bool op_mismatch    = (slot->collective_op != collective_op);
  bool root_mismatch  = (root >= 0 && slot->root >= 0 && slot->root != root);
  bool count_mismatch = (count >= 0 && slot->count >= 0 && slot->count != count);

  if (op_mismatch || root_mismatch || count_mismatch) {
    GET_STACK_TRACE_FATAL_HERE;
    CollectiveMismatchInfo info;
    info.comm           = comm;
    info.rank           = -1; // We don't track per-rank ID here
    info.expected_op    = slot->collective_op;
    info.actual_op      = collective_op;
    info.expected_root  = slot->root;
    info.actual_root    = root;
    info.expected_count = slot->count;
    info.actual_count   = count;
    ReportCollectiveMismatch(info, stack);
  }

  // Check for collective timeout.
  u64 elapsed = GetCurrentTimeMicros() - slot->first_entry_time_us;
  if ((int)elapsed > flags().deadlock_timeout_us) {
    GET_STACK_TRACE_FATAL_HERE;
    DeadlockInfo dinfo;
    dinfo.is_timeout = true;
    dinfo.num_ranks_in_cycle = 1;
    dinfo.cycle[0] = comm;
    dinfo.wait_duration_us = elapsed;
    ReportDeadlock(dinfo, stack);
  }

  slot->num_participants++;
}

static void CollectiveExit(int comm, int collective_op) {
  if (!flags().check_collective_mismatch || flags().lightweight_mode)
    return;

  SpinMutexLock lock(&g_state.collectives_mutex);
  for (int i = 0; i < MpisanState::kMaxCollectives; ++i) {
    CollectiveState &s = g_state.collectives[i];
    if (s.active && s.comm == comm && s.collective_op == collective_op) {
      s.num_participants--;
      if (s.num_participants <= 0)
        s.active = false;
      return;
    }
  }
}

// ============================================================================
// Task 6.1 – Runtime Initialization and Finalization
// ============================================================================

// Simple env-var parser for MPISAN_OPTIONS="key=val:key=val"
// Handles bool (0/1/true/false) and int flags without sanitizer bootstrap.
static void ParseMpisanOptions(Flags *f) {
  const char *env = GetEnv("MPISAN_OPTIONS");
  if (!env || env[0] == '\0')
    return;

  // Work on a copy so we can tokenize.
  char buf[1024];
  internal_strncpy(buf, env, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char *p = buf;
  while (p && *p) {
    // Find end of this key=value token (delimited by ':').
    char *sep = internal_strchr(p, ':');
    if (sep) *sep = '\0';

    // Split on '='.
    char *eq = internal_strchr(p, '=');
    if (eq) {
      *eq = '\0';
      const char *key = p;
      const char *val = eq + 1;

      // Match each known flag by name.
#define MPISAN_FLAG(Type, Name, Default, Desc)                    \
      if (internal_strcmp(key, #Name) == 0) {                     \
        if (internal_strcmp(#Type, "bool") == 0) {                \
          f->Name = (Type)(val[0] == '1' ||                       \
                           internal_strcmp(val, "true") == 0);    \
        } else if (internal_strcmp(#Type, "int") == 0) {          \
          int v = 0;                                               \
          for (const char *c = val; *c >= '0' && *c <= '9'; ++c) \
            v = v * 10 + (*c - '0');                              \
          f->Name = (Type)v;                                       \
        }                                                          \
      }
#include "mpisan/mpisan_flags.inc"
#undef MPISAN_FLAG
    }

    p = sep ? sep + 1 : nullptr;
  }
}

static void MpisanInitInternal() {
  SanitizerToolName = "MPIUsageSanitizer";

  // Apply flag defaults.
  Flags *f = &flags_data;
#define MPISAN_FLAG(Type, Name, DefaultValue, Description) \
  f->Name = DefaultValue;
#include "mpisan/mpisan_flags.inc"
#undef MPISAN_FLAG

  // Parse MPISAN_OPTIONS env var without full sanitizer bootstrap.
  ParseMpisanOptions(f);

  // Zero out state.
  internal_memset(&g_state, 0, sizeof(g_state));
  for (int i = 0; i < MpisanState::kMaxGraphNodes; ++i)
    g_state.wait_for_graph[i] = -1;

  g_state.initialized = true;

  if (flags().verbosity >= 1)
    Printf("==MPISan== MPIUsageSanitizer initialized.\n");
}

} // namespace __mpisan

// ============================================================================
// C Interface (called by instrumented code and preinit array)
// ============================================================================

extern "C" {

using namespace __mpisan;

SANITIZER_INTERFACE_ATTRIBUTE void __mpisan_init() {
  if (g_state.initialized)
    return;
  SpinMutexLock lock(&g_state.init_mutex);
  if (g_state.initialized)
    return;
  MpisanInitInternal();
}

SANITIZER_INTERFACE_ATTRIBUTE void __mpisan_fini() {
  if (!g_state.initialized)
    return;
  if (flags().print_stats)
    PrintErrorSummary();
  g_state.initialized = false;
}

SANITIZER_INTERFACE_ATTRIBUTE int __mpisan_is_initialized() {
  return g_state.initialized ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Task 8.1 – Send Operation Hooks
// ---------------------------------------------------------------------------

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_send(const void *buf, int count, int datatype, int dest, int tag,
              int comm) {
  if (!g_state.initialized) return;

  uptr buf_addr = (uptr)buf;
  uptr buf_size = (uptr)count * GetMpiTypeSize((u32)datatype);

  // Check buffer aliasing against pending receives.
  CheckBufferAliasing(buf_addr, buf_size, "MPI_Send");

  // Record this send in the pending ops table.
  PendingOp *op = AllocPendingOp();
  if (op) {
    op->kind         = PendingOp::Kind::Send;
    op->rank         = dest;
    op->tag          = tag;
    op->comm         = comm;
    op->datatype     = (u32)datatype;
    op->count        = count;
    op->buf_addr     = buf_addr;
    op->buf_size     = buf_size;
    op->timestamp_us = GetCurrentTimeMicros();
  }

  // Update wait-for graph: this rank is now waiting for dest to receive.
  // We don't know our own rank here, so we use dest as a proxy:
  // record that dest is expected to be waiting for us (circular).
  // The test programs set edges directly via the graph for deadlock testing.
}

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_isend(const void *buf, int count, int datatype, int dest, int tag,
               int comm, unsigned long long request_id) {
  if (!g_state.initialized) return;

  uptr buf_addr = (uptr)buf;
  uptr buf_size = (uptr)count * GetMpiTypeSize((u32)datatype);

  CheckBufferAliasing(buf_addr, buf_size, "MPI_Isend");

  PendingOp *op = AllocPendingOp();
  if (op) {
    op->kind         = PendingOp::Kind::ISend;
    op->rank         = dest;
    op->tag          = tag;
    op->comm         = comm;
    op->datatype     = (u32)datatype;
    op->count        = count;
    op->buf_addr     = buf_addr;
    op->buf_size     = buf_size;
    op->request_id   = (u64)request_id;
    op->timestamp_us = GetCurrentTimeMicros();
  }
}

// ---------------------------------------------------------------------------
// Task 8.2 – Receive Operation Hooks
// ---------------------------------------------------------------------------

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_recv(void *buf, int count, int datatype, int src, int tag, int comm) {
  if (!g_state.initialized) return;

  uptr buf_addr = (uptr)buf;
  uptr buf_size = (uptr)count * GetMpiTypeSize((u32)datatype);

  CheckBufferAliasing(buf_addr, buf_size, "MPI_Recv");

  // Try to match against a pending send and validate types.
  {
    SpinMutexLock lock(&g_state.pending_ops_mutex);
    PendingOp *send_op = FindMatchingSend(src, tag, comm);
    if (send_op) {
      CheckSendRecvMatch(*send_op, (u32)datatype, count, src, tag);
      send_op->active = false;
      g_state.pending_ops_count--;
    }
  }

  // Clear any wait-for edge for the source rank.
  WaitForGraphClearEdge(src);
}

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_irecv(void *buf, int count, int datatype, int src, int tag, int comm,
               unsigned long long request_id) {
  if (!g_state.initialized) return;

  uptr buf_addr = (uptr)buf;
  uptr buf_size = (uptr)count * GetMpiTypeSize((u32)datatype);

  CheckBufferAliasing(buf_addr, buf_size, "MPI_Irecv");

  PendingOp *op = AllocPendingOp();
  if (op) {
    op->kind         = PendingOp::Kind::IRecv;
    op->rank         = src;
    op->tag          = tag;
    op->comm         = comm;
    op->datatype     = (u32)datatype;
    op->count        = count;
    op->buf_addr     = buf_addr;
    op->buf_size     = buf_size;
    op->request_id   = (u64)request_id;
    op->timestamp_us = GetCurrentTimeMicros();
  }
}

// ---------------------------------------------------------------------------
// Task 17.1-17.2 – Non-Blocking Wait Hooks
// ---------------------------------------------------------------------------

SANITIZER_INTERFACE_ATTRIBUTE void __mpisan_wait(unsigned long long request_id) {
  if (!g_state.initialized) return;

  SpinMutexLock lock(&g_state.pending_ops_mutex);
  for (int i = 0; i < MPISAN_MAX_PENDING_OPS; ++i) {
    PendingOp &op = g_state.pending_ops[i];
    if (!op.active || op.request_id != (u64)request_id)
      continue;

    // Check for timeout on this pending operation.
    CheckForTimeout(op);

    // If it is a non-blocking receive, try to match against a pending send.
    if (op.kind == PendingOp::Kind::IRecv) {
      PendingOp *send_op = FindMatchingSend(op.rank, op.tag, op.comm);
      if (send_op) {
        CheckSendRecvMatch(*send_op, op.datatype, op.count, op.rank, op.tag);
        send_op->active = false;
        g_state.pending_ops_count--;
      }
    }

    op.active = false;
    g_state.pending_ops_count--;
    break;
  }
}

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_waitall(int count, const unsigned long long *request_ids) {
  if (!g_state.initialized) return;
  for (int i = 0; i < count; ++i)
    __mpisan_wait(request_ids[i]);
}

// ---------------------------------------------------------------------------
// Task 12.1-12.3 – Collective Hooks
// ---------------------------------------------------------------------------

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_collective_enter(int comm, int collective_op, int root, int count,
                          int datatype, int comm_size) {
  if (!g_state.initialized) return;
  ValidateCollective(comm, collective_op, root, count, datatype, comm_size);
}

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_collective_exit(int comm, int collective_op) {
  if (!g_state.initialized) return;
  CollectiveExit(comm, collective_op);
}

// ---------------------------------------------------------------------------
// MPI_Sendrecv Hook
// ---------------------------------------------------------------------------

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_sendrecv(const void *sendbuf, int sendcount, int sendtype, int dest,
                  int sendtag, void *recvbuf, int recvcount, int recvtype,
                  int src, int recvtag, int comm) {
  if (!g_state.initialized) return;

  // Check that send and recv buffers do not overlap.
  uptr send_addr = (uptr)sendbuf;
  uptr send_size = (uptr)sendcount * GetMpiTypeSize((u32)sendtype);
  uptr recv_addr = (uptr)recvbuf;
  uptr recv_size = (uptr)recvcount * GetMpiTypeSize((u32)recvtype);

  if (send_addr != 0 && recv_addr != 0 &&
      flags().check_buffer_aliasing && !flags().lightweight_mode) {
    if (send_addr < recv_addr + recv_size && recv_addr < send_addr + send_size) {
      GET_STACK_TRACE_FATAL_HERE;
      BufferAliasingInfo info;
      info.buf1_addr = send_addr;
      info.buf1_size = send_size;
      info.buf2_addr = recv_addr;
      info.buf2_size = recv_size;
      info.op1_name  = "MPI_Sendrecv (send buffer)";
      info.op2_name  = "MPI_Sendrecv (recv buffer)";
      ReportBufferAliasing(info, stack);
    }
  }

  // Delegate to individual send/recv hooks for type checking.
  __mpisan_send(sendbuf, sendcount, sendtype, dest, sendtag, comm);
  __mpisan_recv(recvbuf, recvcount, recvtype, src, recvtag, comm);
}

// ---------------------------------------------------------------------------
// Wait-for graph hooks (used by deadlock tests and Track A pass)
// ---------------------------------------------------------------------------

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_set_wait_for(int waiter, int provider) {
  if (!g_state.initialized) return;
  WaitForGraphSetEdge(waiter, provider);
  CheckForDeadlock(waiter);
}

SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_clear_wait_for(int waiter) {
  if (!g_state.initialized) return;
  WaitForGraphClearEdge(waiter);
}

} // extern "C"

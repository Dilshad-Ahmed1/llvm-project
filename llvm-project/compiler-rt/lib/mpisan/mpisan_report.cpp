//===-- mpisan_report.cpp - MPI Usage Sanitizer -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mpisan/mpisan_report.h"
#include "mpisan/mpisan_flags.h"
#include "mpisan/mpisan_internal.h"

#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_posix.h"

using namespace __sanitizer;

namespace __mpisan {

// ---------------------------------------------------------------------------
// Output helper — writes directly to stderr fd without any sanitizer
// bootstrap dependencies (no allocator, no color detection, no locks).
// ---------------------------------------------------------------------------
static void MpisanOut(const char *msg) {
  internal_write(2, msg, internal_strlen(msg));
}

// Thin wrappers so call sites can use printf-style formatting.
// Each wrapper formats into a stack buffer then calls MpisanOut.
static void MpisanPrint1(const char *fmt, const char *a) {
  char buf[512];
  internal_snprintf(buf, sizeof(buf), fmt, a);
  MpisanOut(buf);
}
static void MpisanPrint2(const char *fmt, const char *a, const char *b) {
  char buf[512];
  internal_snprintf(buf, sizeof(buf), fmt, a, b);
  MpisanOut(buf);
}
static void MpisanPrint2i(const char *fmt, int a, int b) {
  char buf[256];
  internal_snprintf(buf, sizeof(buf), fmt, a, b);
  MpisanOut(buf);
}
static void MpisanPrintAddr(const char *fmt, const char *a,
                            uptr b, uptr c) {
  char buf[512];
  internal_snprintf(buf, sizeof(buf), fmt, a, b, c);
  MpisanOut(buf);
}
static void MpisanPrintU64(const char *fmt, u64 v) {
  char buf[256];
  internal_snprintf(buf, sizeof(buf), fmt, v);
  MpisanOut(buf);
}
static void MpisanPrintInt(const char *fmt, int v) {
  char buf[256];
  internal_snprintf(buf, sizeof(buf), fmt, v);
  MpisanOut(buf);
}

// ============================================================================
// Type / Collective name tables
// ============================================================================

const char *MpiTypeName(u32 type_id) {
  switch (type_id) {
  case MPISAN_TYPE_CHAR:           return "MPI_CHAR";
  case MPISAN_TYPE_SHORT:          return "MPI_SHORT";
  case MPISAN_TYPE_INT:            return "MPI_INT";
  case MPISAN_TYPE_LONG:           return "MPI_LONG";
  case MPISAN_TYPE_FLOAT:          return "MPI_FLOAT";
  case MPISAN_TYPE_DOUBLE:         return "MPI_DOUBLE";
  case MPISAN_TYPE_UNSIGNED_CHAR:  return "MPI_UNSIGNED_CHAR";
  case MPISAN_TYPE_UNSIGNED_SHORT: return "MPI_UNSIGNED_SHORT";
  case MPISAN_TYPE_UNSIGNED:       return "MPI_UNSIGNED";
  case MPISAN_TYPE_UNSIGNED_LONG:  return "MPI_UNSIGNED_LONG";
  case MPISAN_TYPE_LONG_DOUBLE:    return "MPI_LONG_DOUBLE";
  case MPISAN_TYPE_BYTE:           return "MPI_BYTE";
  case MPISAN_TYPE_PACKED:         return "MPI_PACKED";
  default:                         return "<derived/unknown>";
  }
}

const char *CollectiveOpName(int op) {
  switch (op) {
  case MPISAN_COLL_BARRIER:   return "MPI_Barrier";
  case MPISAN_COLL_BCAST:     return "MPI_Bcast";
  case MPISAN_COLL_REDUCE:    return "MPI_Reduce";
  case MPISAN_COLL_ALLREDUCE: return "MPI_Allreduce";
  case MPISAN_COLL_SCATTER:   return "MPI_Scatter";
  case MPISAN_COLL_GATHER:    return "MPI_Gather";
  case MPISAN_COLL_ALLGATHER: return "MPI_Allgather";
  case MPISAN_COLL_ALLTOALL:  return "MPI_Alltoall";
  case MPISAN_COLL_SCAN:      return "MPI_Scan";
  case MPISAN_COLL_EXSCAN:    return "MPI_Exscan";
  default:                    return "<unknown collective>";
  }
}

// ============================================================================
// Helpers
// ============================================================================

static void PrintHeader(const char *error_kind) {
  MpisanOut("==MPISan== ERROR: MPIUsageSanitizer: ");
  MpisanOut(error_kind);
  MpisanOut("\n");
}

static void MaybeDie() {
  if (flags().halt_on_error) {
    MpisanOut("==MPISan== Aborting due to halt_on_error=1\n");
    internal__exit(1);
  }
}

// ============================================================================
// Public reporting functions
// ============================================================================

void ReportTypeMismatch(const TypeMismatchInfo &info,
                        const BufferedStackTrace &stack) {
  bool type_differs  = (info.send_type  != info.recv_type);
  bool count_differs = (info.send_count != info.recv_count);

  if (type_differs && flags().check_type_mismatch) {
    PrintHeader("type-mismatch");
    MpisanPrint2("  Send type: %s  Recv type: %s\n",
                 MpiTypeName(info.send_type), MpiTypeName(info.recv_type));
    MpisanPrint2i("  Send rank: %d  Recv rank: %d\n",
                  info.send_rank, info.recv_rank);
    MpisanPrintInt("  Tag: %d\n", info.tag);
    GetMpisanState().type_mismatch_errors++;
    MaybeDie();
  }

  if (count_differs && flags().check_count_mismatch) {
    PrintHeader("count-mismatch");
    MpisanPrint2i("  Send count: %d  Recv count: %d\n",
                  info.send_count, info.recv_count);
    GetMpisanState().type_mismatch_errors++;
    MaybeDie();
  }
}

void ReportBufferAliasing(const BufferAliasingInfo &info,
                          const BufferedStackTrace &stack) {
  if (!flags().check_buffer_aliasing)
    return;
  PrintHeader("buffer-aliasing");
  MpisanPrintAddr("  Op1 (%s): buffer [0x%zx, 0x%zx)\n",
                  info.op1_name, info.buf1_addr,
                  info.buf1_addr + info.buf1_size);
  MpisanPrintAddr("  Op2 (%s): buffer [0x%zx, 0x%zx)\n",
                  info.op2_name, info.buf2_addr,
                  info.buf2_addr + info.buf2_size);
  MpisanOut("  Buffers overlap - undefined behavior in MPI.\n");
  GetMpisanState().buffer_aliasing_errors++;
  MaybeDie();
}

void ReportCollectiveMismatch(const CollectiveMismatchInfo &info,
                              const BufferedStackTrace &stack) {
  if (!flags().check_collective_mismatch)
    return;
  if (info.expected_op != info.actual_op) {
    PrintHeader("collective-mismatch");
    MpisanPrint2("  Called %s but others called %s\n",
                 CollectiveOpName(info.actual_op),
                 CollectiveOpName(info.expected_op));
  } else {
    PrintHeader("collective-parameter-mismatch");
    MpisanPrint1("  Collective: %s\n", CollectiveOpName(info.actual_op));
    if (info.expected_root != info.actual_root && info.expected_root >= 0)
      MpisanPrint2i("  Root mismatch: expected %d, got %d\n",
                    info.expected_root, info.actual_root);
    if (info.expected_count != info.actual_count && info.expected_count >= 0)
      MpisanPrint2i("  Count mismatch: expected %d, got %d\n",
                    info.expected_count, info.actual_count);
  }
  GetMpisanState().collective_mismatch_errors++;
  MaybeDie();
}

void ReportDeadlock(const DeadlockInfo &info,
                    const BufferedStackTrace &stack) {
  if (!flags().check_deadlock)
    return;
  if (info.is_timeout) {
    PrintHeader("potential-deadlock (timeout)");
    GetMpisanState().timeout_errors++;
  } else {
    PrintHeader("deadlock");
    MpisanPrintInt("  Deadlock cycle among %d ranks\n",
                   info.num_ranks_in_cycle);
    GetMpisanState().deadlock_errors++;
  }
  MaybeDie();
}

void PrintErrorSummary() {
  MpisanState &state = GetMpisanState();
  u64 total = state.type_mismatch_errors + state.buffer_aliasing_errors +
              state.collective_mismatch_errors + state.deadlock_errors +
              state.timeout_errors;
  MpisanPrintU64("==MPISan== Summary: %llu error(s) detected\n", total);
  if (state.type_mismatch_errors)
    MpisanPrintU64("  Type/count mismatches : %llu\n",
                   state.type_mismatch_errors);
  if (state.buffer_aliasing_errors)
    MpisanPrintU64("  Buffer aliasing       : %llu\n",
                   state.buffer_aliasing_errors);
  if (state.collective_mismatch_errors)
    MpisanPrintU64("  Collective mismatches : %llu\n",
                   state.collective_mismatch_errors);
  if (state.deadlock_errors)
    MpisanPrintU64("  Deadlocks             : %llu\n",
                   state.deadlock_errors);
  if (state.timeout_errors)
    MpisanPrintU64("  Timeouts              : %llu\n",
                   state.timeout_errors);
}

} // namespace __mpisan

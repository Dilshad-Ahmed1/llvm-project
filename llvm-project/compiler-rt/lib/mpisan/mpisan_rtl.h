//===-- mpisan_rtl.h - MPI Usage Sanitizer ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Public C interface exposed by the MPISan runtime library.
// These functions are called by the LLVM instrumentation pass (Track A).
//===----------------------------------------------------------------------===//

#pragma once

#include "sanitizer_common/sanitizer_internal_defs.h"

extern "C" {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/// Initialize the MPISan runtime. Called automatically via the preinit array
/// on Linux, or explicitly by the instrumented MPI_Init wrapper.
SANITIZER_INTERFACE_ATTRIBUTE void __mpisan_init();

/// Finalize the MPISan runtime. Called by the instrumented MPI_Finalize
/// wrapper. Prints the error summary if print_stats=1.
SANITIZER_INTERFACE_ATTRIBUTE void __mpisan_fini();

/// Returns non-zero if the runtime has been initialized.
SANITIZER_INTERFACE_ATTRIBUTE int __mpisan_is_initialized();

// ---------------------------------------------------------------------------
// Point-to-Point Send Hooks
// ---------------------------------------------------------------------------

/// Hook called before MPI_Send / MPI_Ssend / MPI_Rsend / MPI_Bsend.
/// @param buf       Send buffer address.
/// @param count     Number of elements.
/// @param datatype  MPI_Datatype (as int).
/// @param dest      Destination rank.
/// @param tag       Message tag.
/// @param comm      MPI_Comm (as int).
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_send(const void *buf, int count, int datatype, int dest, int tag,
              int comm);

/// Hook called before MPI_Isend / MPI_Issend.
/// @param request_id  Opaque ID derived from the MPI_Request pointer.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_isend(const void *buf, int count, int datatype, int dest, int tag,
               int comm, unsigned long long request_id);

// ---------------------------------------------------------------------------
// Point-to-Point Receive Hooks
// ---------------------------------------------------------------------------

/// Hook called before MPI_Recv.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_recv(void *buf, int count, int datatype, int src, int tag, int comm);

/// Hook called before MPI_Irecv.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_irecv(void *buf, int count, int datatype, int src, int tag, int comm,
               unsigned long long request_id);

// ---------------------------------------------------------------------------
// Non-Blocking Wait Hooks
// ---------------------------------------------------------------------------

/// Hook called before MPI_Wait.
/// @param request_id  Opaque ID derived from the MPI_Request pointer.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_wait(unsigned long long request_id);

/// Hook called before MPI_Waitall.
/// @param count        Number of requests.
/// @param request_ids  Array of request IDs.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_waitall(int count, const unsigned long long *request_ids);

// ---------------------------------------------------------------------------
// Collective Operation Hooks
// ---------------------------------------------------------------------------

/// Hook called before any collective operation.
/// @param comm          MPI_Comm (as int).
/// @param collective_op MPISAN_COLL_* constant.
/// @param root          Root rank (-1 if not applicable).
/// @param count         Element count (-1 if not applicable).
/// @param datatype      MPI_Datatype (-1 if not applicable).
/// @param comm_size     Number of ranks in the communicator.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_collective_enter(int comm, int collective_op, int root, int count,
                          int datatype, int comm_size);

/// Hook called after a collective operation completes.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_collective_exit(int comm, int collective_op);

// ---------------------------------------------------------------------------
// MPI_Sendrecv Hook
// ---------------------------------------------------------------------------

/// Hook called before MPI_Sendrecv.
SANITIZER_INTERFACE_ATTRIBUTE void
__mpisan_sendrecv(const void *sendbuf, int sendcount, int sendtype, int dest,
                  int sendtag, void *recvbuf, int recvcount, int recvtype,
                  int src, int recvtag, int comm);

} // extern "C"

//===- MPIFunctionDatabase.cpp - MPI Function Signature Database --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the MPIFunctionDatabase class which maintains a
// comprehensive database of MPI function signatures.
//
//===----------------------------------------------------------------------===//

#include "MPIFunctionDatabase.h"
#include "llvm/Support/Debug.h"
#include <tuple>
#include <algorithm>
#include <cctype>

using namespace llvm;

#define DEBUG_TYPE "mpi-function-database"

// Helper functions for string case conversion (C++17 compatible)
static std::string stringToLower(StringRef str) {
  std::string result = str.str();
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

static std::string stringToUpper(StringRef str) {
  std::string result = str.str();
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}

static std::string stringToLower(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

static std::string stringToUpper(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}

MPIFunctionDatabase::MPIFunctionDatabase() = default;

void MPIFunctionDatabase::initialize() {
  LLVM_DEBUG(dbgs() << "Initializing MPI Function Database\n");
  
  initializeCFunctions();
  initializeFortranFunctions();
  initializeCXXFunctions();
  
  LLVM_DEBUG(dbgs() << "Loaded " << AllFunctions.size() << " MPI function signatures\n");
}

void MPIFunctionDatabase::initializeCFunctions() {
  // Add environment functions
  addEnvironmentFunctions();
  
  // Add point-to-point communication functions
  addPointToPointFunctions();
  
  // Add collective communication functions
  addCollectiveFunctions();
  
  // Add non-blocking collective functions
  addNonBlockingCollectiveFunctions();
  
  // Add neighborhood collective functions
  addNeighborhoodCollectiveFunctions();
  
  // Add communicator management functions
  addCommunicatorFunctions();
  
  // Add group management functions
  addGroupFunctions();
  
  // Add datatype functions
  addDatatypeFunctions();
  
  // Add request management functions
  addRequestFunctions();
  
  // Add process management functions
  addProcessFunctions();
  
  // Add attribute functions
  addAttributeFunctions();
  
  // Add error handling functions
  addErrorFunctions();
  
  // Add profiling functions
  addProfilingFunctions();
  
  // Add info object functions
  addInfoFunctions();
  
  // Add window (RMA) functions
  addWindowFunctions();
  
  // Add one-sided communication functions
  addOneSidedFunctions();
  
  // Add file I/O functions
  addFileFunctions();
  
  // Add topology functions
  addTopologyFunctions();
}

void MPIFunctionDatabase::initializeFortranFunctions() {
  // Add Fortran-specific MPI function signatures
  // These follow the Fortran MPI binding conventions
  
  // Environment functions - Fortran variants
  addFortranEnvironmentFunctions();
  
  // Point-to-point communication - Fortran variants
  addFortranPointToPointFunctions();
  
  // Collective communication - Fortran variants
  addFortranCollectiveFunctions();
  
  // Communicator management - Fortran variants
  addFortranCommunicatorFunctions();
  
  // Datatype functions - Fortran variants
  addFortranDatatypeFunctions();
  
  // Request management - Fortran variants
  addFortranRequestFunctions();
  
  // Group management - Fortran variants
  addFortranGroupFunctions();
  
  // Window (RMA) functions - Fortran variants
  addFortranWindowFunctions();
  
  // One-sided communication - Fortran variants
  addFortranOneSidedFunctions();
  
  // Info object functions - Fortran variants
  addFortranInfoFunctions();
  
  // Error handling functions - Fortran variants
  addFortranErrorFunctions();
  
  // Topology functions - Fortran variants
  addFortranTopologyFunctions();
  
  // Add MPI-2 Fortran 90 bindings
  addFortran90Bindings();
  
  // Add MPI-3 Fortran 2008 bindings (mpi_f08 module)
  addFortran2008Bindings();
}

void MPIFunctionDatabase::initializeCXXFunctions() {
  // C++ bindings are deprecated in MPI-3.0, but may still be encountered
  // Add C++ MPI namespace and class-based interface support
  
  // Add C++ environment functions
  addCXXEnvironmentFunctions();
  
  // Add C++ point-to-point communication functions
  addCXXPointToPointFunctions();
  
  // Add C++ collective communication functions
  addCXXCollectiveFunctions();
  
  // Add C++ communicator management functions
  addCXXCommunicatorFunctions();
  
  // Add C++ datatype functions
  addCXXDatatypeFunctions();
  
  // Add C++ request management functions
  addCXXRequestFunctions();
  
  // Add C++ group management functions
  addCXXGroupFunctions();
  
  // Add C++ window (RMA) functions
  addCXXWindowFunctions();
  
  // Add C++ info object functions
  addCXXInfoFunctions();
  
  // Add C++ error handling functions
  addCXXErrorFunctions();
  
  // Add C++ topology functions
  addCXXTopologyFunctions();
}

void MPIFunctionDatabase::addEnvironmentFunctions() {
  // Basic environment functions
  addDetailedFunction("MPI_Init", MPIFunctionType::Environment, Language::C, false, false, {
    {"argc", ParameterRole::Count, true, false, false},
    {"argv", ParameterRole::Buffer, true, false, false}
  }, false, "Initialize MPI execution environment", "1.0");
  
  addDetailedFunction("MPI_Init_thread", MPIFunctionType::Environment, Language::C, false, false, {
    {"argc", ParameterRole::Count, true, false, false},
    {"argv", ParameterRole::Buffer, true, false, false},
    {"required", ParameterRole::Flag, true, false, false},
    {"provided", ParameterRole::Flag, false, true, false}
  }, false, "Initialize MPI with thread support", "2.0");
  
  addDetailedFunction("MPI_Finalize", MPIFunctionType::Environment, Language::C, false, false, {
  }, false, "Terminate MPI execution environment", "1.0");
  
  addDetailedFunction("MPI_Initialized", MPIFunctionType::Environment, Language::C, false, false, {
    {"flag", ParameterRole::Flag, false, true, false}
  }, false, "Check if MPI is initialized", "1.0");
  
  addDetailedFunction("MPI_Finalized", MPIFunctionType::Environment, Language::C, false, false, {
    {"flag", ParameterRole::Flag, false, true, false}
  }, false, "Check if MPI is finalized", "2.0");
  
  addDetailedFunction("MPI_Abort", MPIFunctionType::Environment, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"errorcode", ParameterRole::ErrorCode, true, false, false}
  }, false, "Terminate MPI program", "1.0");
  
  addDetailedFunction("MPI_Get_version", MPIFunctionType::Environment, Language::C, false, false, {
    {"version", ParameterRole::Flag, false, true, false},
    {"subversion", ParameterRole::Flag, false, true, false}
  }, false, "Get MPI version", "1.2");
  
  addDetailedFunction("MPI_Get_library_version", MPIFunctionType::Environment, Language::C, false, false, {
    {"version", ParameterRole::Buffer, false, true, false},
    {"resultlen", ParameterRole::Count, false, true, false}
  }, false, "Get MPI library version string", "2.0");
  
  addDetailedFunction("MPI_Query_thread", MPIFunctionType::Environment, Language::C, false, false, {
    {"provided", ParameterRole::Flag, false, true, false}
  }, false, "Query thread support level", "2.0");
  
  addDetailedFunction("MPI_Is_thread_main", MPIFunctionType::Environment, Language::C, false, false, {
    {"flag", ParameterRole::Flag, false, true, false}
  }, false, "Check if calling thread is main thread", "2.0");
}

void MPIFunctionDatabase::addPointToPointFunctions() {
  // Blocking point-to-point
  addDetailedFunction("MPI_Send", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Blocking send", "1.0");
  
  addDetailedFunction("MPI_Recv", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Blocking receive", "1.0");
  
  // Non-blocking point-to-point
  addDetailedFunction("MPI_Isend", MPIFunctionType::PointToPoint, Language::C, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking send", "1.0");
  
  addDetailedFunction("MPI_Irecv", MPIFunctionType::PointToPoint, Language::C, false, true, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking receive", "1.0");
  
  // Combined send-receive
  addDetailedFunction("MPI_Sendrecv", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"sendtag", ParameterRole::Tag, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"recvtag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Send and receive", "1.0");
  
  addDetailedFunction("MPI_Sendrecv_replace", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"buf", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"sendtag", ParameterRole::Tag, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"recvtag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Send and receive with same buffer", "1.0");
  
  // Synchronous and buffered modes
  addDetailedFunction("MPI_Ssend", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Synchronous send", "1.0");
  
  addDetailedFunction("MPI_Bsend", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Buffered send", "1.0");
  
  addDetailedFunction("MPI_Rsend", MPIFunctionType::PointToPoint, Language::C, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Ready send", "1.0");
  
  // Non-blocking variants
  addDetailedFunction("MPI_Issend", MPIFunctionType::PointToPoint, Language::C, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking synchronous send", "1.0");
  
  addDetailedFunction("MPI_Ibsend", MPIFunctionType::PointToPoint, Language::C, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking buffered send", "1.0");
  
  addDetailedFunction("MPI_Irsend", MPIFunctionType::PointToPoint, Language::C, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking ready send", "1.0");
}

void MPIFunctionDatabase::addCollectiveFunctions() {
  // Broadcast
  addDetailedFunction("MPI_Bcast", MPIFunctionType::Collective, Language::C, true, false, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Broadcast data from root to all processes", "1.0");
  
  // Reduction operations
  addDetailedFunction("MPI_Reduce", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Reduce values on all processes to single value", "1.0");
  
  addDetailedFunction("MPI_Allreduce", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Reduce values and distribute result to all processes", "1.0");
  
  addDetailedFunction("MPI_Reduce_scatter", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcounts", ParameterRole::RecvCount, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Reduce and scatter result", "1.0");
  
  addDetailedFunction("MPI_Scan", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Inclusive scan (prefix reduction)", "1.0");
  
  addDetailedFunction("MPI_Exscan", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Exclusive scan", "2.0");
  
  // Gather operations
  addDetailedFunction("MPI_Gather", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Gather data from all processes to root", "1.0");
  
  addDetailedFunction("MPI_Allgather", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Gather data from all processes to all processes", "1.0");
  
  addDetailedFunction("MPI_Gatherv", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcounts", ParameterRole::RecvCount, true, false, false},
    {"displs", ParameterRole::RecvDispl, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Gather varying amounts of data", "1.0");
  
  addDetailedFunction("MPI_Allgatherv", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcounts", ParameterRole::RecvCount, true, false, false},
    {"displs", ParameterRole::RecvDispl, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Gather varying amounts of data to all processes", "1.0");
  
  // Scatter operations
  addDetailedFunction("MPI_Scatter", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Scatter data from root to all processes", "1.0");
  
  addDetailedFunction("MPI_Scatterv", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcounts", ParameterRole::SendCount, true, false, false},
    {"displs", ParameterRole::SendDispl, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Scatter varying amounts of data", "1.0");
  
  // All-to-all operations
  addDetailedFunction("MPI_Alltoall", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "All-to-all communication", "1.0");
  
  addDetailedFunction("MPI_Alltoallv", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcounts", ParameterRole::SendCount, true, false, false},
    {"sdispls", ParameterRole::SendDispl, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcounts", ParameterRole::RecvCount, true, false, false},
    {"rdispls", ParameterRole::RecvDispl, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "All-to-all with varying data amounts", "1.0");
  
  // Synchronization
  addDetailedFunction("MPI_Barrier", MPIFunctionType::Collective, Language::C, true, false, {
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Synchronization barrier", "1.0");
}

void MPIFunctionDatabase::addCommunicatorFunctions() {
  addDetailedFunction("MPI_Comm_create", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"group", ParameterRole::Group, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false}
  }, false, "Create new communicator", "1.0");
  
  addDetailedFunction("MPI_Comm_split", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"color", ParameterRole::Flag, true, false, false},
    {"key", ParameterRole::Flag, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false}
  }, false, "Split communicator", "1.0");
  
  addDetailedFunction("MPI_Comm_dup", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false}
  }, false, "Duplicate communicator", "1.0");
  
  addDetailedFunction("MPI_Comm_free", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, true, false}
  }, false, "Free communicator", "1.0");
  
  addDetailedFunction("MPI_Comm_rank", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"rank", ParameterRole::Rank, false, true, false}
  }, false, "Get process rank", "1.0");
  
  addDetailedFunction("MPI_Comm_size", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"size", ParameterRole::Size, false, true, false}
  }, false, "Get communicator size", "1.0");
  
  addDetailedFunction("MPI_Comm_compare", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm1", ParameterRole::Communicator, true, false, false},
    {"comm2", ParameterRole::Communicator, true, false, false},
    {"result", ParameterRole::Flag, false, true, false}
  }, false, "Compare communicators", "1.0");
  
  addDetailedFunction("MPI_Comm_group", MPIFunctionType::Communicator, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"group", ParameterRole::Group, false, true, false}
  }, false, "Get communicator group", "1.0");
}

void MPIFunctionDatabase::addDatatypeFunctions() {
  addDetailedFunction("MPI_Type_create_struct", MPIFunctionType::Datatype, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_blocklengths", ParameterRole::Count, true, false, false},
    {"array_of_displacements", ParameterRole::Displacement, true, false, false},
    {"array_of_types", ParameterRole::Datatype, true, false, false},
    {"newtype", ParameterRole::Datatype, false, true, false}
  }, false, "Create struct datatype", "2.0");
  
  addDetailedFunction("MPI_Type_commit", MPIFunctionType::Datatype, Language::C, false, false, {
    {"datatype", ParameterRole::Datatype, true, true, false}
  }, false, "Commit datatype", "1.0");
  
  addDetailedFunction("MPI_Type_free", MPIFunctionType::Datatype, Language::C, false, false, {
    {"datatype", ParameterRole::Datatype, true, true, false}
  }, false, "Free datatype", "1.0");
  
  addDetailedFunction("MPI_Type_size", MPIFunctionType::Datatype, Language::C, false, false, {
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"size", ParameterRole::Size, false, true, false}
  }, false, "Get datatype size", "1.0");
  
  addDetailedFunction("MPI_Type_contiguous", MPIFunctionType::Datatype, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"oldtype", ParameterRole::Datatype, true, false, false},
    {"newtype", ParameterRole::Datatype, false, true, false}
  }, false, "Create contiguous datatype", "1.0");
  
  addDetailedFunction("MPI_Type_vector", MPIFunctionType::Datatype, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"blocklength", ParameterRole::Count, true, false, false},
    {"stride", ParameterRole::Count, true, false, false},
    {"oldtype", ParameterRole::Datatype, true, false, false},
    {"newtype", ParameterRole::Datatype, false, true, false}
  }, false, "Create vector datatype", "1.0");
  
  addDetailedFunction("MPI_Type_indexed", MPIFunctionType::Datatype, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_blocklengths", ParameterRole::Count, true, false, false},
    {"array_of_displacements", ParameterRole::Displacement, true, false, false},
    {"oldtype", ParameterRole::Datatype, true, false, false},
    {"newtype", ParameterRole::Datatype, false, true, false}
  }, false, "Create indexed datatype", "1.0");
}

void MPIFunctionDatabase::addRequestFunctions() {
  addDetailedFunction("MPI_Wait", MPIFunctionType::Request, Language::C, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Wait for request completion", "1.0");
  
  addDetailedFunction("MPI_Waitall", MPIFunctionType::Request, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, false}
  }, false, "Wait for all requests", "1.0");
  
  addDetailedFunction("MPI_Waitany", MPIFunctionType::Request, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"index", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Wait for any request", "1.0");
  
  addDetailedFunction("MPI_Test", MPIFunctionType::Request, Language::C, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Test for request completion", "1.0");
  
  addDetailedFunction("MPI_Testall", MPIFunctionType::Request, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, false}
  }, false, "Test all requests", "1.0");
  
  addDetailedFunction("MPI_Testany", MPIFunctionType::Request, Language::C, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"index", ParameterRole::Flag, false, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Test any request", "1.0");
  
  addDetailedFunction("MPI_Request_free", MPIFunctionType::Request, Language::C, false, false, {
    {"request", ParameterRole::Request, true, true, false}
  }, false, "Free request object", "1.0");
  
  addDetailedFunction("MPI_Cancel", MPIFunctionType::Request, Language::C, false, false, {
    {"request", ParameterRole::Request, true, false, false}
  }, false, "Cancel request", "1.0");
}

void MPIFunctionDatabase::addGroupFunctions() {
  addDetailedFunction("MPI_Group_incl", MPIFunctionType::Group, Language::C, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false}
  }, false, "Create group by including ranks", "1.0");
  
  addDetailedFunction("MPI_Group_excl", MPIFunctionType::Group, Language::C, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false}
  }, false, "Create group by excluding ranks", "1.0");
  
  addDetailedFunction("MPI_Group_union", MPIFunctionType::Group, Language::C, false, false, {
    {"group1", ParameterRole::Group, true, false, false},
    {"group2", ParameterRole::Group, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false}
  }, false, "Union of two groups", "1.0");
  
  addDetailedFunction("MPI_Group_intersection", MPIFunctionType::Group, Language::C, false, false, {
    {"group1", ParameterRole::Group, true, false, false},
    {"group2", ParameterRole::Group, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false}
  }, false, "Intersection of two groups", "1.0");
  
  addDetailedFunction("MPI_Group_free", MPIFunctionType::Group, Language::C, false, false, {
    {"group", ParameterRole::Group, true, true, false}
  }, false, "Free group", "1.0");
  
  addDetailedFunction("MPI_Group_size", MPIFunctionType::Group, Language::C, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"size", ParameterRole::Size, false, true, false}
  }, false, "Get group size", "1.0");
  
  addDetailedFunction("MPI_Group_rank", MPIFunctionType::Group, Language::C, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"rank", ParameterRole::Rank, false, true, false}
  }, false, "Get rank in group", "1.0");
}

std::unique_ptr<MPIFunctionSignature> MPIFunctionDatabase::createFunctionSignature(
    const std::string& Name, MPIFunctionType Type, Language Lang,
    bool IsCollective, bool IsNonBlocking, bool IsDeprecated, 
    const std::string& Description, const std::string& Version) {
  auto Signature = std::make_unique<MPIFunctionSignature>(Name, Type, Lang);
  Signature->IsCollective = IsCollective;
  Signature->IsNonBlocking = IsNonBlocking;
  Signature->IsDeprecated = IsDeprecated;
  Signature->Description = Description;
  Signature->Version = Version;
  return Signature;
}

void MPIFunctionDatabase::addDetailedFunction(const std::string& Name, MPIFunctionType Type,
                        Language Lang, bool IsCollective, bool IsNonBlocking,
                        const std::vector<std::tuple<std::string, ParameterRole, bool, bool, bool>>& Params,
                        bool IsDeprecated, const std::string& Description, const std::string& Version) {
  auto Signature = createFunctionSignature(Name, Type, Lang, IsCollective, IsNonBlocking, 
                                          IsDeprecated, Description, Version);
  
  // Add parameters
  for (const auto& [ParamName, Role, IsInput, IsOutput, IsOptional] : Params) {
    Signature->addParameter(ParamName, Role, IsInput, IsOutput, IsOptional);
  }
  
  // Add to lookup map
  FunctionMap[Signature->Name] = Signature.get();
  
  // Store in vector for iteration
  AllFunctions.push_back(std::move(Signature));
}

bool MPIFunctionDatabase::isMPIFunction(StringRef FunctionName) const {
  return FunctionMap.find(FunctionName) != FunctionMap.end();
}

const MPIFunctionSignature* MPIFunctionDatabase::getFunctionSignature(StringRef FunctionName) const {
  auto It = FunctionMap.find(FunctionName);
  return (It != FunctionMap.end()) ? It->second : nullptr;
}

MPIFunctionType MPIFunctionDatabase::classifyFunction(StringRef FunctionName) const {
  const MPIFunctionSignature* Sig = getFunctionSignature(FunctionName);
  return Sig ? Sig->Type : MPIFunctionType::Unknown;
}

std::vector<const MPIFunctionSignature*> 
MPIFunctionDatabase::getFunctionsByType(MPIFunctionType Type) const {
  std::vector<const MPIFunctionSignature*> Result;
  
  for (const auto& Func : AllFunctions) {
    if (Func->Type == Type) {
      Result.push_back(Func.get());
    }
  }
  
  return Result;
}

void MPIFunctionDatabase::addFunctionSignature(const MPIFunctionSignature& Signature) {
  auto NewSig = std::make_unique<MPIFunctionSignature>(Signature);
  FunctionMap[NewSig->Name] = NewSig.get();
  AllFunctions.push_back(std::move(NewSig));
}

void MPIFunctionDatabase::addNonBlockingCollectiveFunctions() {
  // Non-blocking broadcast
  addDetailedFunction("MPI_Ibcast", MPIFunctionType::Collective, Language::C, true, true, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking broadcast", "3.0");
  
  // Non-blocking reductions
  addDetailedFunction("MPI_Ireduce", MPIFunctionType::Collective, Language::C, true, true, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking reduce", "3.0");
  
  addDetailedFunction("MPI_Iallreduce", MPIFunctionType::Collective, Language::C, true, true, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking allreduce", "3.0");
  
  // Non-blocking gather
  addDetailedFunction("MPI_Igather", MPIFunctionType::Collective, Language::C, true, true, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking gather", "3.0");
  
  addDetailedFunction("MPI_Iallgather", MPIFunctionType::Collective, Language::C, true, true, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking allgather", "3.0");
  
  // Non-blocking barrier
  addDetailedFunction("MPI_Ibarrier", MPIFunctionType::Collective, Language::C, true, true, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false}
  }, false, "Non-blocking barrier", "3.0");
}

void MPIFunctionDatabase::addNeighborhoodCollectiveFunctions() {
  addDetailedFunction("MPI_Neighbor_allgather", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Neighborhood allgather", "3.0");
  
  addDetailedFunction("MPI_Neighbor_alltoall", MPIFunctionType::Collective, Language::C, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, false, "Neighborhood alltoall", "3.0");
}

void MPIFunctionDatabase::addProcessFunctions() {
  addDetailedFunction("MPI_Comm_spawn", MPIFunctionType::Process, Language::C, false, false, {
    {"command", ParameterRole::Buffer, true, false, false},
    {"argv", ParameterRole::Buffer, true, false, false},
    {"maxprocs", ParameterRole::Count, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"intercomm", ParameterRole::Communicator, false, true, false},
    {"array_of_errcodes", ParameterRole::ErrorCode, false, true, false}
  }, false, "Spawn new processes", "2.0");
  
  addDetailedFunction("MPI_Comm_connect", MPIFunctionType::Process, Language::C, false, false, {
    {"port_name", ParameterRole::Buffer, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false}
  }, false, "Connect to another MPI process", "2.0");
  
  addDetailedFunction("MPI_Comm_accept", MPIFunctionType::Process, Language::C, false, false, {
    {"port_name", ParameterRole::Buffer, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false}
  }, false, "Accept connection from another MPI process", "2.0");
}

void MPIFunctionDatabase::addAttributeFunctions() {
  addDetailedFunction("MPI_Attr_get", MPIFunctionType::Attribute, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"keyval", ParameterRole::Attribute, true, false, false},
    {"attribute_val", ParameterRole::Attribute, false, true, false},
    {"flag", ParameterRole::Flag, false, true, false}
  }, true, "Get attribute value (deprecated)", "1.0");
  
  addDetailedFunction("MPI_Attr_put", MPIFunctionType::Attribute, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"keyval", ParameterRole::Attribute, true, false, false},
    {"attribute_val", ParameterRole::Attribute, true, false, false}
  }, true, "Put attribute value (deprecated)", "1.0");
  
  addDetailedFunction("MPI_Comm_get_attr", MPIFunctionType::Attribute, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"comm_keyval", ParameterRole::Attribute, true, false, false},
    {"attribute_val", ParameterRole::Attribute, false, true, false},
    {"flag", ParameterRole::Flag, false, true, false}
  }, false, "Get communicator attribute", "2.0");
  
  addDetailedFunction("MPI_Comm_set_attr", MPIFunctionType::Attribute, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"comm_keyval", ParameterRole::Attribute, true, false, false},
    {"attribute_val", ParameterRole::Attribute, true, false, false}
  }, false, "Set communicator attribute", "2.0");
}

void MPIFunctionDatabase::addErrorFunctions() {
  addDetailedFunction("MPI_Error_class", MPIFunctionType::Error, Language::C, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"errorclass", ParameterRole::ErrorClass, false, true, false}
  }, false, "Get error class from error code", "1.0");
  
  addDetailedFunction("MPI_Error_string", MPIFunctionType::Error, Language::C, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"string", ParameterRole::ErrorString, false, true, false},
    {"resultlen", ParameterRole::Count, false, true, false}
  }, false, "Get error string", "1.0");
  
  addDetailedFunction("MPI_Errhandler_create", MPIFunctionType::Error, Language::C, false, false, {
    {"function", ParameterRole::Buffer, true, false, false},
    {"errhandler", ParameterRole::Buffer, false, true, false}
  }, true, "Create error handler (deprecated)", "1.0");
  
  addDetailedFunction("MPI_Comm_create_errhandler", MPIFunctionType::Error, Language::C, false, false, {
    {"comm_errhandler_fn", ParameterRole::Buffer, true, false, false},
    {"errhandler", ParameterRole::Buffer, false, true, false}
  }, false, "Create communicator error handler", "2.0");
}

void MPIFunctionDatabase::addProfilingFunctions() {
  addDetailedFunction("MPI_Pcontrol", MPIFunctionType::Profiling, Language::C, false, false, {
    {"level", ParameterRole::Flag, true, false, false}
  }, false, "Profiling control", "1.0");
}

void MPIFunctionDatabase::addInfoFunctions() {
  addDetailedFunction("MPI_Info_create", MPIFunctionType::Info, Language::C, false, false, {
    {"info", ParameterRole::Info, false, true, false}
  }, false, "Create info object", "2.0");
  
  addDetailedFunction("MPI_Info_set", MPIFunctionType::Info, Language::C, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"key", ParameterRole::Buffer, true, false, false},
    {"value", ParameterRole::Buffer, true, false, false}
  }, false, "Set info key-value pair", "2.0");
  
  addDetailedFunction("MPI_Info_get", MPIFunctionType::Info, Language::C, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"key", ParameterRole::Buffer, true, false, false},
    {"valuelen", ParameterRole::Count, true, false, false},
    {"value", ParameterRole::Buffer, false, true, false},
    {"flag", ParameterRole::Flag, false, true, false}
  }, false, "Get info value", "2.0");
  
  addDetailedFunction("MPI_Info_free", MPIFunctionType::Info, Language::C, false, false, {
    {"info", ParameterRole::Info, true, true, false}
  }, false, "Free info object", "2.0");
}

void MPIFunctionDatabase::addWindowFunctions() {
  addDetailedFunction("MPI_Win_create", MPIFunctionType::Window, Language::C, false, false, {
    {"base", ParameterRole::Buffer, true, false, false},
    {"size", ParameterRole::Size, true, false, false},
    {"disp_unit", ParameterRole::Count, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"win", ParameterRole::Window, false, true, false}
  }, false, "Create RMA window", "2.0");
  
  addDetailedFunction("MPI_Win_free", MPIFunctionType::Window, Language::C, false, false, {
    {"win", ParameterRole::Window, true, true, false}
  }, false, "Free RMA window", "2.0");
  
  addDetailedFunction("MPI_Win_fence", MPIFunctionType::Window, Language::C, false, false, {
    {"assert", ParameterRole::Flag, true, false, false},
    {"win", ParameterRole::Window, true, false, false}
  }, false, "RMA fence synchronization", "2.0");
}

void MPIFunctionDatabase::addOneSidedFunctions() {
  addDetailedFunction("MPI_Get", MPIFunctionType::Window, Language::C, false, false, {
    {"origin_addr", ParameterRole::Buffer, false, true, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"win", ParameterRole::Window, true, false, false}
  }, false, "RMA get operation", "2.0");
  
  addDetailedFunction("MPI_Put", MPIFunctionType::Window, Language::C, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"win", ParameterRole::Window, true, false, false}
  }, false, "RMA put operation", "2.0");
  
  addDetailedFunction("MPI_Accumulate", MPIFunctionType::Window, Language::C, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"win", ParameterRole::Window, true, false, false}
  }, false, "RMA accumulate operation", "2.0");
}

void MPIFunctionDatabase::addFileFunctions() {
  addDetailedFunction("MPI_File_open", MPIFunctionType::File, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"filename", ParameterRole::Buffer, true, false, false},
    {"amode", ParameterRole::Flag, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"fh", ParameterRole::Buffer, false, true, false}
  }, false, "Open MPI file", "2.0");
  
  addDetailedFunction("MPI_File_close", MPIFunctionType::File, Language::C, false, false, {
    {"fh", ParameterRole::Buffer, true, true, false}
  }, false, "Close MPI file", "2.0");
  
  addDetailedFunction("MPI_File_read", MPIFunctionType::File, Language::C, false, false, {
    {"fh", ParameterRole::Buffer, true, false, false},
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Read from MPI file", "2.0");
  
  addDetailedFunction("MPI_File_write", MPIFunctionType::File, Language::C, false, false, {
    {"fh", ParameterRole::Buffer, true, false, false},
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"status", ParameterRole::Status, false, true, false}
  }, false, "Write to MPI file", "2.0");
}

void MPIFunctionDatabase::addTopologyFunctions() {
  addDetailedFunction("MPI_Cart_create", MPIFunctionType::Topology, Language::C, false, false, {
    {"comm_old", ParameterRole::Communicator, true, false, false},
    {"ndims", ParameterRole::Count, true, false, false},
    {"dims", ParameterRole::Count, true, false, false},
    {"periods", ParameterRole::Flag, true, false, false},
    {"reorder", ParameterRole::Flag, true, false, false},
    {"comm_cart", ParameterRole::Communicator, false, true, false}
  }, false, "Create Cartesian topology", "1.0");
  
  addDetailedFunction("MPI_Cart_coords", MPIFunctionType::Topology, Language::C, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"rank", ParameterRole::Rank, true, false, false},
    {"maxdims", ParameterRole::Count, true, false, false},
    {"coords", ParameterRole::Buffer, false, true, false}
  }, false, "Get Cartesian coordinates", "1.0");
  
  addDetailedFunction("MPI_Graph_create", MPIFunctionType::Topology, Language::C, false, false, {
    {"comm_old", ParameterRole::Communicator, true, false, false},
    {"nnodes", ParameterRole::Count, true, false, false},
    {"index", ParameterRole::Buffer, true, false, false},
    {"edges", ParameterRole::Buffer, true, false, false},
    {"reorder", ParameterRole::Flag, true, false, false},
    {"comm_graph", ParameterRole::Communicator, false, true, false}
  }, false, "Create graph topology", "1.0");
}

bool MPIFunctionDatabase::loadFromFile(StringRef FilePath) {
  // TODO: Implement configuration file loading
  return false;
}

//===----------------------------------------------------------------------===//
// NameManglingHandler Implementation
//===----------------------------------------------------------------------===//

std::string NameManglingHandler::mangleFortranName(StringRef CFunctionName, 
                                                   FortranCompiler Compiler) const {
  // Handle special MPI cases first
  std::string SpecialCase = handleMPISpecialCases(CFunctionName, Compiler);
  if (!SpecialCase.empty()) {
    return SpecialCase;
  }
  
  // Apply compiler-specific mangling
  std::string Result;
  switch (Compiler) {
    case FortranCompiler::GFortran:
      Result = applyGfortranMangling(CFunctionName);
      break;
    case FortranCompiler::Intel:
      Result = applyIntelMangling(CFunctionName);
      break;
    case FortranCompiler::PGI:
      Result = applyPGIMangling(CFunctionName);
      break;
    case FortranCompiler::Flang:
      Result = applyFlangMangling(CFunctionName);
      break;
    case FortranCompiler::NAG:
      Result = applyNAGMangling(CFunctionName);
      break;
    case FortranCompiler::Cray:
      Result = applyCrayMangling(CFunctionName);
      break;
    case FortranCompiler::IBM:
      Result = applyIBMMangling(CFunctionName);
      break;
    case FortranCompiler::Auto:
      // Default to gfortran convention
      Result = applyGfortranMangling(CFunctionName);
      break;
  }
  
  // Apply platform-specific adjustments
  return applyPlatformAdjustments(Result, Compiler);
}

std::string NameManglingHandler::demangleFortranName(StringRef MangledName) const {
  std::string Name = MangledName.str();
  
  // Handle module-qualified names first
  if (isModuleQualified(Name)) {
    Name = extractBaseName(Name);
  }
  
  // Remove trailing underscore(s) - handle double underscores
  while (!Name.empty() && Name.back() == '_') {
    Name.pop_back();
  }
  
  // Handle special prefixes (e.g., __wrap_, __real_)
  if (Name.substr(0, 7) == "__wrap_") {
    Name = Name.substr(7);
  } else if (Name.substr(0, 7) == "__real_") {
    Name = Name.substr(7);
  }
  
  // Convert to uppercase (MPI convention)
  std::transform(Name.begin(), Name.end(), Name.begin(), ::toupper);
  
  return Name;
}

bool NameManglingHandler::isFortranMangled(StringRef Name) const {
  // Check various Fortran mangling patterns
  
  // Standard underscore suffix
  std::string lowerName = Name.str();
  std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
  if (Name.ends_with("_") && lowerName == Name) {
    return true;
  }
  
  // Double underscore (for names with underscores)
  if (Name.ends_with("__")) {
    return true;
  }
  
  // Module-qualified names (e.g., __mpi_f08_MOD_mpi_send)
  if (Name.find("_MOD_") != std::string::npos || Name.find("_mod_") != std::string::npos) {
    return true;
  }
  
  // Intel-style mangling
  std::string lowerName2 = Name.str();
  std::transform(lowerName2.begin(), lowerName2.end(), lowerName2.begin(), ::tolower);
  if (lowerName2 == Name && (Name.substr(0, 4) == "mpi_" || Name.substr(0, 5) == "pmpi_")) {
    return true;
  }
  
  return false;
}

std::vector<std::string> NameManglingHandler::getAllMangledVariants(StringRef BaseName) const {
  std::vector<std::string> Variants;
  
  // Original name (C binding)
  Variants.push_back(BaseName.str());
  
  // All compiler variants
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::GFortran));
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::Intel));
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::PGI));
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::Flang));
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::NAG));
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::Cray));
  Variants.push_back(mangleFortranName(BaseName, FortranCompiler::IBM));
  
  // Add PMPI variants (profiling interface)
  std::string PMPIName = "PMPI" + BaseName.substr(3).str(); // Replace MPI with PMPI
  Variants.push_back(PMPIName);
  Variants.push_back(mangleFortranName(PMPIName, FortranCompiler::GFortran));
  Variants.push_back(mangleFortranName(PMPIName, FortranCompiler::Intel));
  
  // Add Fortran 2008 module variants
  std::string baseNameLower = BaseName.str();
  std::transform(baseNameLower.begin(), baseNameLower.end(), baseNameLower.begin(), ::tolower);
  std::string F08Name = "__mpi_f08_MOD_" + baseNameLower;
  Variants.push_back(F08Name);
  
  // Remove duplicates
  std::sort(Variants.begin(), Variants.end());
  Variants.erase(std::unique(Variants.begin(), Variants.end()), Variants.end());
  
  return Variants;
}

FortranCompiler NameManglingHandler::detectCompiler(StringRef MangledName) const {
  // Detect compiler based on mangling patterns
  
  // Intel-specific patterns
  if (MangledName.find("for_") != std::string::npos || MangledName.ends_with("_$") ||
      MangledName.find("_i8") != std::string::npos || MangledName.find("_i4") != std::string::npos) {
    return FortranCompiler::Intel;
  }
  
  // PGI-specific patterns
  if (MangledName.find("pgi_") != std::string::npos || (!MangledName.empty() && MangledName[0] == '_') ||
      MangledName.find("_cuda") != std::string::npos || MangledName.find("_gpu") != std::string::npos) {
    return FortranCompiler::PGI;
  }
  
  // NAG-specific patterns (often uppercase)
  std::string upperName = MangledName.str();
  std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
  if (upperName == MangledName && !MangledName.ends_with("_")) {
    return FortranCompiler::NAG;
  }
  
  // Cray-specific patterns
  if (MangledName.find("_cray") != std::string::npos || MangledName.find("CCE_") != std::string::npos ||
      MangledName.find("_CRAY") != std::string::npos) {
    return FortranCompiler::Cray;
  }
  
  // IBM XL Fortran patterns
  if (MangledName.find("_xl") != std::string::npos || MangledName.find("_XL") != std::string::npos ||
      MangledName.find("_ibm") != std::string::npos || MangledName.find("_aix") != std::string::npos) {
    return FortranCompiler::IBM;
  }
  
  // Flang-specific patterns
  if (MangledName.find("_flang") != std::string::npos || MangledName.find("_llvm") != std::string::npos) {
    return FortranCompiler::Flang;
  }
  
  // Fortran 2008 module patterns
  if (MangledName.find("_MOD_") != std::string::npos || MangledName.find("_f08_") != std::string::npos) {
    return FortranCompiler::GFortran; // Most common for F2008 support
  }
  
  // Default to gfortran (most common)
  return FortranCompiler::GFortran;
}

ManglingConvention NameManglingHandler::getManglingConvention(FortranCompiler Compiler) const {
  switch (Compiler) {
    case FortranCompiler::GFortran:
      return ManglingConvention::Underscore;
    case FortranCompiler::Intel:
      return ManglingConvention::Underscore;
    case FortranCompiler::PGI:
      return ManglingConvention::Underscore;
    case FortranCompiler::Flang:
      return ManglingConvention::Underscore;
    case FortranCompiler::NAG:
      return ManglingConvention::UpperCase;
    case FortranCompiler::Cray:
      return ManglingConvention::CrayStyle;
    case FortranCompiler::IBM:
      return ManglingConvention::IBMStyle;
    case FortranCompiler::Auto:
      return ManglingConvention::Underscore;
  }
}

bool NameManglingHandler::matchesManglingPattern(StringRef Name, ManglingConvention Convention) const {
  switch (Convention) {
    case ManglingConvention::Underscore: {
      std::string lowerName = Name.str();
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
      return Name.ends_with("_") && lowerName == Name;
    }
    case ManglingConvention::DoubleUnderscore:
      return Name.ends_with("__");
    case ManglingConvention::NoUnderscore: {
      std::string lowerName = Name.str();
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
      return !Name.ends_with("_") && lowerName == Name;
    }
    case ManglingConvention::UpperCase: {
      std::string upperName = Name.str();
      std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
      return upperName == Name && !Name.ends_with("_");
    }
    case ManglingConvention::UpperCaseUnderscore: {
      std::string upperName = Name.str();
      std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
      return upperName == Name && Name.ends_with("_");
    }
    case ManglingConvention::Mixed:
      return true; // Accept any mixed case
    case ManglingConvention::CrayStyle: {
      // Cray often uses uppercase without underscores or specific prefixes
      std::string upperName = Name.str();
      std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
      return upperName == Name || Name.find("_cray") != std::string::npos;
    }
    case ManglingConvention::IBMStyle: {
      // IBM XL Fortran uses various conventions depending on platform
      std::string lowerName = Name.str();
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
      return lowerName == Name || Name.find("_xl") != std::string::npos;
    }
  }
  return false;
}

std::string NameManglingHandler::applyGfortranMangling(StringRef Name) const {
  std::string Result = Name.str();
  std::transform(Result.begin(), Result.end(), Result.begin(), ::tolower);
  
  // gfortran uses single underscore for most names
  Result += "_";
  
  // Special case: names containing underscores get double underscore
  if (Name.contains("_")) {
    Result += "_";
  }
  
  // Handle gfortran-specific MPI extensions
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // gfortran may add specific prefixes for MPI functions
    if (Name.find("_f90") != std::string::npos || Name.find("_f08") != std::string::npos) {
      // Already module-qualified, don't add extra underscores
      return Result.substr(0, Result.length() - 1); // Remove one underscore
    }
  }
  
  return Result;
}

std::string NameManglingHandler::applyIntelMangling(StringRef Name) const {
  std::string Result = Name.str();
  std::transform(Result.begin(), Result.end(), Result.begin(), ::tolower);
  
  // Intel Fortran typically uses single underscore
  Result += "_";
  
  // Intel-specific adjustments for MPI functions
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // Intel may use different conventions for MPI
    if (Name.find("_i8") != std::string::npos || Name.find("_8") != std::string::npos) {
      // 64-bit integer variants
      Result += "i8";
    } else if (Name.find("_i4") != std::string::npos || Name.find("_4") != std::string::npos) {
      // 32-bit integer variants  
      Result += "i4";
    }
  }
  
  // Handle Intel's FOR_* runtime functions
  if (Name.find("_array") != std::string::npos || Name.find("_alloc") != std::string::npos) {
    Result = "for_" + Result;
  }
  
  return Result;
}

std::string NameManglingHandler::applyPGIMangling(StringRef Name) const {
  std::string Result = Name.str();
  std::transform(Result.begin(), Result.end(), Result.begin(), ::tolower);
  
  // PGI/NVIDIA HPC SDK uses single underscore
  Result += "_";
  
  // PGI-specific adjustments
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // PGI may use different prefixes
    if (Name.find("_cuda") != std::string::npos || Name.find("_gpu") != std::string::npos) {
      // GPU-aware MPI extensions
      Result = "pgi_" + Result;
    }
  }
  
  // Handle PGI's runtime library functions
  if (Name.find("_acc") != std::string::npos || Name.find("_openacc") != std::string::npos) {
    Result = "acc_" + Result;
  }
  
  return Result;
}

std::string NameManglingHandler::applyFlangMangling(StringRef Name) const {
  std::string Result = Name.str();
  std::transform(Result.begin(), Result.end(), Result.begin(), ::tolower);
  
  // Flang follows gfortran conventions mostly
  Result += "_";
  
  // Handle names with underscores (double underscore like gfortran)
  if (Name.contains("_")) {
    Result += "_";
  }
  
  // Flang-specific adjustments for LLVM integration
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // Flang may have LLVM-specific optimizations
    if (Name.find("_llvm") != std::string::npos || Name.find("_opt") != std::string::npos) {
      Result = "flang_" + Result;
    }
  }
  
  return Result;
}

std::string NameManglingHandler::applyNAGMangling(StringRef Name) const {
  // NAG Fortran often uses uppercase without underscores
  std::string Result = Name.str();
  std::transform(Result.begin(), Result.end(), Result.begin(), ::toupper);
  
  // NAG-specific conventions
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // NAG may keep MPI functions in uppercase
    return Result;
  }
  
  // Some NAG versions may still use underscores
  if (Name.find("_module") != std::string::npos || Name.find("_interface") != std::string::npos) {
    Result += "_";
  }
  
  return Result;
}

std::string NameManglingHandler::handleMPISpecialCases(StringRef Name, FortranCompiler Compiler) const {
  // Handle special MPI function name cases
  
  // MPI constants and handles are often treated differently
  std::string upperName = Name.str();
  std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
  if (Name.substr(0, 4) == "MPI_" && upperName == Name) {
    // This might be a constant, not a function
    return "";
  }
  
  // Handle Fortran array descriptor functions (for assumed-shape arrays)
  if (Name.find("_array") != std::string::npos || Name.find("_descriptor") != std::string::npos) {
    switch (Compiler) {
      case FortranCompiler::GFortran:
        return applyGfortranArrayDescriptorMangling(Name);
      case FortranCompiler::Intel:
        return applyIntelArrayDescriptorMangling(Name);
      case FortranCompiler::PGI:
        return applyPGIArrayDescriptorMangling(Name);
      default:
        break;
    }
  }
  
  // Handle Fortran derived type procedures
  if (Name.find("_dt") != std::string::npos || Name.find("_type") != std::string::npos) {
    return applyDerivedTypeMangling(Name, Compiler);
  }
  
  // Handle Fortran 2008 module procedures
  if (Compiler == FortranCompiler::GFortran || Compiler == FortranCompiler::Auto) {
    // Check for mpi_f08 module procedures
    std::string LowerName = stringToLower(Name);
    if (LowerName.substr(0, 4) == "mpi_") {
      return "__mpi_f08_MOD_" + LowerName;
    }
  }
  
  // Handle Intel Fortran specific MPI bindings
  if (Compiler == FortranCompiler::Intel) {
    std::string LowerName = stringToLower(Name);
    if (LowerName.substr(0, 4) == "mpi_") {
      // Intel may use different module naming
      return "__intel_mpi_MOD_" + LowerName;
    }
  }
  
  // Handle PGI/NVIDIA HPC SDK specific bindings
  if (Compiler == FortranCompiler::PGI) {
    std::string LowerName = stringToLower(Name);
    if (LowerName.substr(0, 4) == "mpi_") {
      // PGI may use different conventions
      return "_" + LowerName + "_";
    }
  }
  
  return ""; // No special case
}

std::string NameManglingHandler::applyPlatformAdjustments(StringRef Name, FortranCompiler Compiler) const {
  std::string Result = Name.str();
  
  // Platform-specific adjustments
  
  // Windows-specific adjustments
  #ifdef _WIN32
  if (Compiler == FortranCompiler::Intel) {
    // Intel Fortran on Windows may use different conventions
    if (Result.ends_with("_")) {
      Result.pop_back(); // Remove trailing underscore on Windows
    }
  }
  #endif
  
  // macOS-specific adjustments
  #ifdef __APPLE__
  if (Compiler == FortranCompiler::GFortran) {
    // macOS gfortran may have different symbol visibility
    if (!Result.empty() && Result[0] != '_') {
      Result = "_" + Result; // Add leading underscore on macOS
    }
  }
  #endif
  
  // Linux-specific adjustments (most common, default behavior)
  #ifdef __linux__
  // Most Linux distributions use standard conventions
  // No special adjustments needed for most cases
  #endif
  
  // Handle 32-bit vs 64-bit differences
  #ifdef __LP64__
  // 64-bit specific adjustments
  if (Name.find("_8") != std::string::npos || Name.find("_i8") != std::string::npos) {
    // Handle 64-bit integer variants
    if (Compiler == FortranCompiler::Intel) {
      Result += "_i8";
    }
  }
  #else
  // 32-bit specific adjustments
  if (Compiler == FortranCompiler::Intel) {
    // Intel Fortran 32-bit may use different suffixes
    if (Name.find("_int") != std::string::npos) {
      Result += "_i4";
    }
  }
  #endif
  
  return Result;
}

std::string NameManglingHandler::applyGfortranArrayDescriptorMangling(StringRef Name) const {
  std::string Result = stringToLower(Name);
  
  // gfortran array descriptor mangling for assumed-shape arrays
  if (Name.find("_array") != std::string::npos) {
    Result += "_array_descriptor_";
  }
  
  // Handle multi-dimensional arrays
  if (Name.find("_2d") != std::string::npos) {
    Result += "2d_";
  } else if (Name.find("_3d") != std::string::npos) {
    Result += "3d_";
  }
  
  Result += "_";
  return Result;
}

std::string NameManglingHandler::applyIntelArrayDescriptorMangling(StringRef Name) const {
  std::string Result = stringToLower(Name);
  
  // Intel Fortran array descriptor conventions
  if (Name.find("_array") != std::string::npos) {
    Result += "_for_array_";
  }
  
  Result += "_";
  return Result;
}

std::string NameManglingHandler::applyPGIArrayDescriptorMangling(StringRef Name) const {
  std::string Result = stringToLower(Name);
  
  // PGI/NVIDIA HPC SDK array descriptor conventions
  if (Name.find("_array") != std::string::npos) {
    Result += "_pgi_array_";
  }
  
  Result += "_";
  return Result;
}

std::string NameManglingHandler::applyDerivedTypeMangling(StringRef Name, FortranCompiler Compiler) const {
  std::string Result = stringToLower(Name);
  
  switch (Compiler) {
    case FortranCompiler::GFortran:
      if (Name.find("_dt") != std::string::npos) {
        Result += "_dt_";
      }
      break;
    case FortranCompiler::Intel:
      if (Name.find("_type") != std::string::npos) {
        Result += "_for_type_";
      }
      break;
    case FortranCompiler::PGI:
      if (Name.find("_type") != std::string::npos) {
        Result += "_pgi_type_";
      }
      break;
    default:
      break;
  }
  
  Result += "_";
  return Result;
}

bool NameManglingHandler::isModuleQualified(StringRef Name) const {
  return Name.find("_MOD_") != std::string::npos || Name.find("_mod_") != std::string::npos || 
         Name.find("_f08_") != std::string::npos || Name.find("_f90_") != std::string::npos;
}

std::string NameManglingHandler::extractBaseName(StringRef ModuleQualifiedName) const {
  // Extract base name from module-qualified name
  // Example: __mpi_f08_MOD_mpi_send -> mpi_send
  
  size_t ModPos = ModuleQualifiedName.find("_MOD_");
  if (ModPos != StringRef::npos) {
    return ModuleQualifiedName.substr(ModPos + 5).str();
  }
  
  ModPos = ModuleQualifiedName.find("_mod_");
  if (ModPos != StringRef::npos) {
    return ModuleQualifiedName.substr(ModPos + 5).str();
  }
  
  // If no module qualifier found, return original
  return ModuleQualifiedName.str();
}

//===----------------------------------------------------------------------===//
// Fortran-specific MPI Function Signatures
//===----------------------------------------------------------------------===//

void MPIFunctionDatabase::addFortranFunction(const std::string& BaseName, MPIFunctionType Type,
                       bool IsCollective, bool IsNonBlocking,
                       const std::vector<std::tuple<std::string, ParameterRole, bool, bool, bool>>& Params,
                       bool IsDeprecated, const std::string& Description, const std::string& Version) {
  
  NameManglingHandler ManglingHandler;
  
  // Get all mangled variants for this function
  std::vector<std::string> Variants = ManglingHandler.getAllMangledVariants(BaseName);
  
  // Add each variant as a separate function signature
  for (const std::string& MangledName : Variants) {
    addDetailedFunction(MangledName, Type, Language::Fortran, IsCollective, IsNonBlocking,
                       Params, IsDeprecated, Description, Version);
  }
  
  // Also add the original C name for mixed-language programs
  addDetailedFunction(BaseName, Type, Language::C, IsCollective, IsNonBlocking,
                     Params, IsDeprecated, Description + " (C binding)", Version);
}

void MPIFunctionDatabase::addFortranEnvironmentFunctions() {
  // MPI_Init - Fortran version has different signature (no argc/argv)
  addFortranFunction("MPI_Init", MPIFunctionType::Environment, false, false, {
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Initialize MPI execution environment (Fortran)", "1.0");
  
  addFortranFunction("MPI_Init_thread", MPIFunctionType::Environment, false, false, {
    {"required", ParameterRole::Flag, true, false, false},
    {"provided", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Initialize MPI with thread support (Fortran)", "2.0");
  
  addFortranFunction("MPI_Finalize", MPIFunctionType::Environment, false, false, {
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Terminate MPI execution environment (Fortran)", "1.0");
  
  addFortranFunction("MPI_Initialized", MPIFunctionType::Environment, false, false, {
    {"flag", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Check if MPI is initialized (Fortran)", "1.0");
  
  addFortranFunction("MPI_Finalized", MPIFunctionType::Environment, false, false, {
    {"flag", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Check if MPI is finalized (Fortran)", "2.0");
  
  addFortranFunction("MPI_Abort", MPIFunctionType::Environment, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Terminate MPI program (Fortran)", "1.0");
  
  addFortranFunction("MPI_Get_version", MPIFunctionType::Environment, false, false, {
    {"version", ParameterRole::Flag, false, true, false},
    {"subversion", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get MPI version (Fortran)", "1.2");
  
  addFortranFunction("MPI_Get_library_version", MPIFunctionType::Environment, false, false, {
    {"version", ParameterRole::Buffer, false, true, false},
    {"resultlen", ParameterRole::Count, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get MPI library version string (Fortran)", "2.0");
}

void MPIFunctionDatabase::addFortranPointToPointFunctions() {
  // Blocking point-to-point - Fortran versions
  addFortranFunction("MPI_Send", MPIFunctionType::PointToPoint, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Blocking send (Fortran)", "1.0");
  
  addFortranFunction("MPI_Recv", MPIFunctionType::PointToPoint, false, false, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"status", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Blocking receive (Fortran)", "1.0");
  
  // Non-blocking point-to-point - Fortran versions
  addFortranFunction("MPI_Isend", MPIFunctionType::PointToPoint, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Non-blocking send (Fortran)", "1.0");
  
  addFortranFunction("MPI_Irecv", MPIFunctionType::PointToPoint, false, true, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Non-blocking receive (Fortran)", "1.0");
  
  // Synchronous and buffered modes - Fortran versions
  addFortranFunction("MPI_Ssend", MPIFunctionType::PointToPoint, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Synchronous send (Fortran)", "1.0");
  
  addFortranFunction("MPI_Bsend", MPIFunctionType::PointToPoint, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Buffered send (Fortran)", "1.0");
  
  addFortranFunction("MPI_Rsend", MPIFunctionType::PointToPoint, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Ready send (Fortran)", "1.0");
}

void MPIFunctionDatabase::addFortranCollectiveFunctions() {
  // Broadcast - Fortran version
  addFortranFunction("MPI_Bcast", MPIFunctionType::Collective, true, false, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Broadcast data from root to all processes (Fortran)", "1.0");
  
  // Reduction operations - Fortran versions
  addFortranFunction("MPI_Reduce", MPIFunctionType::Collective, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Reduce values on all processes to single value (Fortran)", "1.0");
  
  addFortranFunction("MPI_Allreduce", MPIFunctionType::Collective, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Reduce values and distribute result to all processes (Fortran)", "1.0");
  
  // Gather operations - Fortran versions
  addFortranFunction("MPI_Gather", MPIFunctionType::Collective, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Gather data from all processes to root (Fortran)", "1.0");
  
  addFortranFunction("MPI_Allgather", MPIFunctionType::Collective, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Gather data from all processes to all processes (Fortran)", "1.0");
  
  // Synchronization - Fortran version
  addFortranFunction("MPI_Barrier", MPIFunctionType::Collective, true, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Synchronization barrier (Fortran)", "1.0");
}

void MPIFunctionDatabase::addFortranCommunicatorFunctions() {
  addFortranFunction("MPI_Comm_create", MPIFunctionType::Communicator, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"group", ParameterRole::Group, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create new communicator (Fortran)", "1.0");
  
  addFortranFunction("MPI_Comm_split", MPIFunctionType::Communicator, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"color", ParameterRole::Flag, true, false, false},
    {"key", ParameterRole::Flag, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Split communicator (Fortran)", "1.0");
  
  addFortranFunction("MPI_Comm_dup", MPIFunctionType::Communicator, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Duplicate communicator (Fortran)", "1.0");
  
  addFortranFunction("MPI_Comm_free", MPIFunctionType::Communicator, false, false, {
    {"comm", ParameterRole::Communicator, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free communicator (Fortran)", "1.0");
  
  addFortranFunction("MPI_Comm_rank", MPIFunctionType::Communicator, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"rank", ParameterRole::Rank, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get process rank (Fortran)", "1.0");
  
  addFortranFunction("MPI_Comm_size", MPIFunctionType::Communicator, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"size", ParameterRole::Size, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get communicator size (Fortran)", "1.0");
}

void MPIFunctionDatabase::addFortranDatatypeFunctions() {
  addFortranFunction("MPI_Type_create_struct", MPIFunctionType::Datatype, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_blocklengths", ParameterRole::Count, true, false, false},
    {"array_of_displacements", ParameterRole::Displacement, true, false, false},
    {"array_of_types", ParameterRole::Datatype, true, false, false},
    {"newtype", ParameterRole::Datatype, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create struct datatype (Fortran)", "2.0");
  
  addFortranFunction("MPI_Type_commit", MPIFunctionType::Datatype, false, false, {
    {"datatype", ParameterRole::Datatype, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Commit datatype (Fortran)", "1.0");
  
  addFortranFunction("MPI_Type_free", MPIFunctionType::Datatype, false, false, {
    {"datatype", ParameterRole::Datatype, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free datatype (Fortran)", "1.0");
  
  addFortranFunction("MPI_Type_size", MPIFunctionType::Datatype, false, false, {
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"size", ParameterRole::Size, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get datatype size (Fortran)", "1.0");
}

void MPIFunctionDatabase::addFortranRequestFunctions() {
  addFortranFunction("MPI_Wait", MPIFunctionType::Request, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"status", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Wait for request completion (Fortran)", "1.0");
  
  addFortranFunction("MPI_Waitall", MPIFunctionType::Request, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Wait for all requests (Fortran)", "1.0");
  
  addFortranFunction("MPI_Waitany", MPIFunctionType::Request, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"index", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Wait for any request (Fortran)", "1.0");
  
  addFortranFunction("MPI_Waitsome", MPIFunctionType::Request, false, false, {
    {"incount", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"outcount", ParameterRole::Count, false, true, false},
    {"array_of_indices", ParameterRole::Flag, false, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Wait for some requests (Fortran)", "1.0");
  
  addFortranFunction("MPI_Test", MPIFunctionType::Request, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Test for request completion (Fortran)", "1.0");
  
  addFortranFunction("MPI_Testall", MPIFunctionType::Request, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Test all requests (Fortran)", "1.0");
  
  addFortranFunction("MPI_Testany", MPIFunctionType::Request, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"index", ParameterRole::Flag, false, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Test any request (Fortran)", "1.0");
  
  addFortranFunction("MPI_Testsome", MPIFunctionType::Request, false, false, {
    {"incount", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"outcount", ParameterRole::Count, false, true, false},
    {"array_of_indices", ParameterRole::Flag, false, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Test some requests (Fortran)", "1.0");
  
  addFortranFunction("MPI_Request_free", MPIFunctionType::Request, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free request object (Fortran)", "1.0");
  
  addFortranFunction("MPI_Cancel", MPIFunctionType::Request, false, false, {
    {"request", ParameterRole::Request, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Cancel request (Fortran)", "1.0");
  
  addFortranFunction("MPI_Request_get_status", MPIFunctionType::Request, false, false, {
    {"request", ParameterRole::Request, true, false, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get status of request (Fortran)", "2.0");
}

void MPIFunctionDatabase::addFortran90Bindings() {
  // MPI-2 Fortran 90 bindings - these often have different interfaces
  // for array handling and optional parameters
  
  // Add Fortran 90 specific array handling functions
  addFortranFunction("MPI_Allgatherv", MPIFunctionType::Collective, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcounts", ParameterRole::RecvCount, true, false, false},
    {"displs", ParameterRole::RecvDispl, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Gather varying amounts of data to all processes (Fortran 90)", "2.0");
  
  // Add process management functions
  addFortranFunction("MPI_Comm_spawn", MPIFunctionType::Process, false, false, {
    {"command", ParameterRole::Buffer, true, false, false},
    {"argv", ParameterRole::Buffer, true, false, false},
    {"maxprocs", ParameterRole::Count, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"intercomm", ParameterRole::Communicator, false, true, false},
    {"array_of_errcodes", ParameterRole::ErrorCode, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Spawn new processes (Fortran 90)", "2.0");
}

void MPIFunctionDatabase::addFortran2008Bindings() {
  // MPI-3 Fortran 2008 bindings (mpi_f08 module)
  // These have different name mangling and type safety
  
  // Add comprehensive mpi_f08 module procedures with detailed signatures
  
  // Environment functions
  addDetailedFunction("__mpi_f08_MOD_mpi_init", MPIFunctionType::Environment, Language::Fortran, false, false, {
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Initialize MPI execution environment (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_init_thread", MPIFunctionType::Environment, Language::Fortran, false, false, {
    {"required", ParameterRole::Flag, true, false, false},
    {"provided", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Initialize MPI with thread support (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_finalize", MPIFunctionType::Environment, Language::Fortran, false, false, {
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Terminate MPI execution environment (Fortran 2008)", "3.0");
  
  // Point-to-point communication
  addDetailedFunction("__mpi_f08_MOD_mpi_send", MPIFunctionType::PointToPoint, Language::Fortran, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Blocking send (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_recv", MPIFunctionType::PointToPoint, Language::Fortran, false, false, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"status", ParameterRole::Status, false, true, true},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Blocking receive (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_isend", MPIFunctionType::PointToPoint, Language::Fortran, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Non-blocking send (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_irecv", MPIFunctionType::PointToPoint, Language::Fortran, false, true, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Non-blocking receive (Fortran 2008)", "3.0");
  
  // Collective communication
  addDetailedFunction("__mpi_f08_MOD_mpi_bcast", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Broadcast data from root to all processes (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_reduce", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Reduce values on all processes to single value (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_allreduce", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Reduce values and distribute result to all processes (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_gather", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Gather data from all processes to root (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_allgather", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Gather data from all processes to all processes (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_barrier", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Synchronization barrier (Fortran 2008)", "3.0");
  
  // Request management
  addDetailedFunction("__mpi_f08_MOD_mpi_wait", MPIFunctionType::Request, Language::Fortran, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"status", ParameterRole::Status, false, true, true},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Wait for request completion (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_waitall", MPIFunctionType::Request, Language::Fortran, false, false, {
    {"count", ParameterRole::Count, true, false, false},
    {"array_of_requests", ParameterRole::Request, true, true, false},
    {"array_of_statuses", ParameterRole::Status, false, true, true},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Wait for all requests (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_test", MPIFunctionType::Request, Language::Fortran, false, false, {
    {"request", ParameterRole::Request, true, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"status", ParameterRole::Status, false, true, true},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Test for request completion (Fortran 2008)", "3.0");
  
  // Communicator management
  addDetailedFunction("__mpi_f08_MOD_mpi_comm_rank", MPIFunctionType::Communicator, Language::Fortran, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"rank", ParameterRole::Rank, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Get process rank (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_comm_size", MPIFunctionType::Communicator, Language::Fortran, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"size", ParameterRole::Size, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Get communicator size (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_comm_create", MPIFunctionType::Communicator, Language::Fortran, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"group", ParameterRole::Group, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Create new communicator (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_comm_split", MPIFunctionType::Communicator, Language::Fortran, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"color", ParameterRole::Flag, true, false, false},
    {"key", ParameterRole::Flag, true, false, false},
    {"newcomm", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Split communicator (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_comm_free", MPIFunctionType::Communicator, Language::Fortran, false, false, {
    {"comm", ParameterRole::Communicator, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Free communicator (Fortran 2008)", "3.0");
  
  // Add additional Fortran 2008 specific functions
  addDetailedFunction("__mpi_f08_MOD_mpi_scatter", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Scatter data from root to all processes (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_alltoall", MPIFunctionType::Collective, Language::Fortran, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"sendcount", ParameterRole::SendCount, true, false, false},
    {"sendtype", ParameterRole::SendType, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"recvcount", ParameterRole::RecvCount, true, false, false},
    {"recvtype", ParameterRole::RecvType, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "All-to-all communication (Fortran 2008)", "3.0");
  
  // Add non-blocking collective functions (MPI-3.0)
  addDetailedFunction("__mpi_f08_MOD_mpi_ibcast", MPIFunctionType::Collective, Language::Fortran, true, true, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Non-blocking broadcast (Fortran 2008)", "3.0");
  
  addDetailedFunction("__mpi_f08_MOD_mpi_iallreduce", MPIFunctionType::Collective, Language::Fortran, true, true, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"request", ParameterRole::Request, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, true}
  }, false, "Non-blocking allreduce (Fortran 2008)", "3.0");
}

//===----------------------------------------------------------------------===//
// Additional Fortran MPI Function Categories
//===----------------------------------------------------------------------===//

void MPIFunctionDatabase::addFortranGroupFunctions() {
  addFortranFunction("MPI_Group_incl", MPIFunctionType::Group, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create group by including ranks (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_excl", MPIFunctionType::Group, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create group by excluding ranks (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_union", MPIFunctionType::Group, false, false, {
    {"group1", ParameterRole::Group, true, false, false},
    {"group2", ParameterRole::Group, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Union of two groups (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_intersection", MPIFunctionType::Group, false, false, {
    {"group1", ParameterRole::Group, true, false, false},
    {"group2", ParameterRole::Group, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Intersection of two groups (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_difference", MPIFunctionType::Group, false, false, {
    {"group1", ParameterRole::Group, true, false, false},
    {"group2", ParameterRole::Group, true, false, false},
    {"newgroup", ParameterRole::Group, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Difference of two groups (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_free", MPIFunctionType::Group, false, false, {
    {"group", ParameterRole::Group, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free group (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_size", MPIFunctionType::Group, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"size", ParameterRole::Size, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get group size (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_rank", MPIFunctionType::Group, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"rank", ParameterRole::Rank, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get rank in group (Fortran)", "1.0");
  
  addFortranFunction("MPI_Group_compare", MPIFunctionType::Group, false, false, {
    {"group1", ParameterRole::Group, true, false, false},
    {"group2", ParameterRole::Group, true, false, false},
    {"result", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Compare groups (Fortran)", "1.0");
}

void MPIFunctionDatabase::addFortranWindowFunctions() {
  addFortranFunction("MPI_Win_create", MPIFunctionType::Window, false, false, {
    {"base", ParameterRole::Buffer, true, false, false},
    {"size", ParameterRole::Size, true, false, false},
    {"disp_unit", ParameterRole::Count, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false},
    {"win", ParameterRole::Window, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create RMA window (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_free", MPIFunctionType::Window, false, false, {
    {"win", ParameterRole::Window, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free RMA window (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_fence", MPIFunctionType::Window, false, false, {
    {"assert", ParameterRole::Flag, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA fence synchronization (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_start", MPIFunctionType::Window, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"assert", ParameterRole::Flag, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Start RMA access epoch (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_complete", MPIFunctionType::Window, false, false, {
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Complete RMA access epoch (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_post", MPIFunctionType::Window, false, false, {
    {"group", ParameterRole::Group, true, false, false},
    {"assert", ParameterRole::Flag, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Post RMA exposure epoch (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_wait", MPIFunctionType::Window, false, false, {
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Wait for RMA exposure epoch (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_lock", MPIFunctionType::Window, false, false, {
    {"lock_type", ParameterRole::Flag, true, false, false},
    {"rank", ParameterRole::Rank, true, false, false},
    {"assert", ParameterRole::Flag, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Lock RMA window (Fortran)", "2.0");
  
  addFortranFunction("MPI_Win_unlock", MPIFunctionType::Window, false, false, {
    {"rank", ParameterRole::Rank, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Unlock RMA window (Fortran)", "2.0");
}

void MPIFunctionDatabase::addFortranOneSidedFunctions() {
  addFortranFunction("MPI_Get", MPIFunctionType::Window, false, false, {
    {"origin_addr", ParameterRole::Buffer, false, true, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA get operation (Fortran)", "2.0");
  
  addFortranFunction("MPI_Put", MPIFunctionType::Window, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA put operation (Fortran)", "2.0");
  
  addFortranFunction("MPI_Accumulate", MPIFunctionType::Window, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA accumulate operation (Fortran)", "2.0");
  
  addFortranFunction("MPI_Get_accumulate", MPIFunctionType::Window, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"origin_count", ParameterRole::Count, true, false, false},
    {"origin_datatype", ParameterRole::Datatype, true, false, false},
    {"result_addr", ParameterRole::Buffer, false, true, false},
    {"result_count", ParameterRole::Count, true, false, false},
    {"result_datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"target_count", ParameterRole::Count, true, false, false},
    {"target_datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA get-accumulate operation (Fortran)", "3.0");
  
  addFortranFunction("MPI_Fetch_and_op", MPIFunctionType::Window, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"result_addr", ParameterRole::Buffer, false, true, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA fetch-and-op operation (Fortran)", "3.0");
  
  addFortranFunction("MPI_Compare_and_swap", MPIFunctionType::Window, false, false, {
    {"origin_addr", ParameterRole::Buffer, true, false, false},
    {"compare_addr", ParameterRole::Buffer, true, false, false},
    {"result_addr", ParameterRole::Buffer, false, true, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"target_rank", ParameterRole::Rank, true, false, false},
    {"target_disp", ParameterRole::Displacement, true, false, false},
    {"win", ParameterRole::Window, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "RMA compare-and-swap operation (Fortran)", "3.0");
}

void MPIFunctionDatabase::addFortranInfoFunctions() {
  addFortranFunction("MPI_Info_create", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create info object (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_set", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"key", ParameterRole::Buffer, true, false, false},
    {"value", ParameterRole::Buffer, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Set info key-value pair (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_get", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"key", ParameterRole::Buffer, true, false, false},
    {"valuelen", ParameterRole::Count, true, false, false},
    {"value", ParameterRole::Buffer, false, true, false},
    {"flag", ParameterRole::Flag, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get info value (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_delete", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"key", ParameterRole::Buffer, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Delete info key (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_get_nkeys", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"nkeys", ParameterRole::Count, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get number of info keys (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_get_nthkey", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"n", ParameterRole::Count, true, false, false},
    {"key", ParameterRole::Buffer, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get nth info key (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_dup", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, false, false},
    {"newinfo", ParameterRole::Info, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Duplicate info object (Fortran)", "2.0");
  
  addFortranFunction("MPI_Info_free", MPIFunctionType::Info, false, false, {
    {"info", ParameterRole::Info, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free info object (Fortran)", "2.0");
}

void MPIFunctionDatabase::addFortranErrorFunctions() {
  addFortranFunction("MPI_Error_class", MPIFunctionType::Error, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"errorclass", ParameterRole::ErrorClass, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get error class from error code (Fortran)", "1.0");
  
  addFortranFunction("MPI_Error_string", MPIFunctionType::Error, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"string", ParameterRole::ErrorString, false, true, false},
    {"resultlen", ParameterRole::Count, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get error string (Fortran)", "1.0");
  
  addFortranFunction("MPI_Comm_create_errhandler", MPIFunctionType::Error, false, false, {
    {"comm_errhandler_fn", ParameterRole::Buffer, true, false, false},
    {"errhandler", ParameterRole::Buffer, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create communicator error handler (Fortran)", "2.0");
  
  addFortranFunction("MPI_Comm_set_errhandler", MPIFunctionType::Error, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"errhandler", ParameterRole::Buffer, true, false, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Set communicator error handler (Fortran)", "2.0");
  
  addFortranFunction("MPI_Comm_get_errhandler", MPIFunctionType::Error, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"errhandler", ParameterRole::Buffer, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get communicator error handler (Fortran)", "2.0");
  
  addFortranFunction("MPI_Errhandler_free", MPIFunctionType::Error, false, false, {
    {"errhandler", ParameterRole::Buffer, true, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Free error handler (Fortran)", "2.0");
}

void MPIFunctionDatabase::addFortranTopologyFunctions() {
  addFortranFunction("MPI_Cart_create", MPIFunctionType::Topology, false, false, {
    {"comm_old", ParameterRole::Communicator, true, false, false},
    {"ndims", ParameterRole::Count, true, false, false},
    {"dims", ParameterRole::Count, true, false, false},
    {"periods", ParameterRole::Flag, true, false, false},
    {"reorder", ParameterRole::Flag, true, false, false},
    {"comm_cart", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create Cartesian topology (Fortran)", "1.0");
  
  addFortranFunction("MPI_Cart_coords", MPIFunctionType::Topology, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"rank", ParameterRole::Rank, true, false, false},
    {"maxdims", ParameterRole::Count, true, false, false},
    {"coords", ParameterRole::Buffer, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get Cartesian coordinates (Fortran)", "1.0");
  
  addFortranFunction("MPI_Cart_rank", MPIFunctionType::Topology, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"coords", ParameterRole::Buffer, true, false, false},
    {"rank", ParameterRole::Rank, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get rank from Cartesian coordinates (Fortran)", "1.0");
  
  addFortranFunction("MPI_Cart_shift", MPIFunctionType::Topology, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"direction", ParameterRole::Flag, true, false, false},
    {"disp", ParameterRole::Displacement, true, false, false},
    {"rank_source", ParameterRole::Source, false, true, false},
    {"rank_dest", ParameterRole::Destination, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Shift Cartesian coordinates (Fortran)", "1.0");
  
  addFortranFunction("MPI_Graph_create", MPIFunctionType::Topology, false, false, {
    {"comm_old", ParameterRole::Communicator, true, false, false},
    {"nnodes", ParameterRole::Count, true, false, false},
    {"index", ParameterRole::Buffer, true, false, false},
    {"edges", ParameterRole::Buffer, true, false, false},
    {"reorder", ParameterRole::Flag, true, false, false},
    {"comm_graph", ParameterRole::Communicator, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Create graph topology (Fortran)", "1.0");
  
  addFortranFunction("MPI_Graph_neighbors", MPIFunctionType::Topology, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"rank", ParameterRole::Rank, true, false, false},
    {"maxneighbors", ParameterRole::Count, true, false, false},
    {"neighbors", ParameterRole::Buffer, false, true, false},
    {"ierror", ParameterRole::ErrorCode, false, true, false}
  }, false, "Get graph neighbors (Fortran)", "1.0");
}

std::string NameManglingHandler::applyCrayMangling(StringRef Name) const {
  // Cray Fortran Compiler mangling conventions
  std::string Result = stringToUpper(Name);
  
  // Cray typically uses uppercase without underscores for most functions
  // But may add specific prefixes for certain runtime functions
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // Cray MPI implementation may use specific prefixes
    if (Name.find("_cce") != std::string::npos || Name.find("_cray") != std::string::npos) {
      // Already has Cray-specific prefix
      return Result;
    }
    
    // For some Cray systems, MPI functions may have CCE_ prefix
    if (Name.find("_mpi") != std::string::npos || Name.find("_parallel") != std::string::npos) {
      Result = "CCE_" + Result;
    }
  }
  
  // Handle Cray-specific array descriptor functions
  if (Name.find("_array") != std::string::npos || Name.find("_descriptor") != std::string::npos) {
    Result = "_cray_" + stringToLower(Result);
  }
  
  // Handle Cray OpenMP and parallel runtime integration
  if (Name.find("_omp") != std::string::npos || Name.find("_parallel") != std::string::npos) {
    Result = "_CRAY_" + Result;
  }
  
  return Result;
}

std::string NameManglingHandler::applyIBMMangling(StringRef Name) const {
  // IBM XL Fortran mangling conventions
  std::string Result = stringToLower(Name);
  
  // IBM XL Fortran typically uses lowercase with underscore
  Result += "_";
  
  // IBM-specific adjustments for different platforms
  if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
    // IBM MPI implementation (POE/PE) may use specific conventions
    if (Name.find("_pe") != std::string::npos || Name.find("_poe") != std::string::npos) {
      // Already has IBM-specific prefix
      return Result;
    }
    
    // For AIX systems, may need specific prefixes
    if (Name.find("_aix") != std::string::npos || Name.find("_power") != std::string::npos) {
      Result = "_xl_" + Result;
    }
    
    // Handle IBM's 64-bit integer variants
    if (Name.find("_i8") != std::string::npos || Name.find("_64") != std::string::npos) {
      Result += "64";
    }
  }
  
  // Handle IBM XL Fortran runtime functions
  if (Name.find("_xlf") != std::string::npos || Name.find("_xl90") != std::string::npos) {
    Result = "_xlf_" + Result;
  }
  
  // Handle IBM POWER-specific optimizations
  if (Name.find("_power") != std::string::npos || Name.find("_vsx") != std::string::npos) {
    Result = "_power_" + Result;
  }
  
  return Result;
}

std::string NameManglingHandler::applyCrayArrayDescriptorMangling(StringRef Name) const {
  // Cray Fortran array descriptor mangling
  std::string Result = stringToUpper(Name);
  
  // Cray uses specific conventions for array descriptors
  if (Name.find("_array") != std::string::npos) {
    Result = "_CRAY_ARRAY_" + Result;
  } else if (Name.find("_descriptor") != std::string::npos) {
    Result = "_CRAY_DESC_" + Result;
  } else {
    // Standard array parameter - may need descriptor handling
    Result = "_cray_arr_" + stringToLower(Result);
  }
  
  return Result;
}

std::string NameManglingHandler::applyIBMArrayDescriptorMangling(StringRef Name) const {
  // IBM XL Fortran array descriptor mangling
  std::string Result = stringToLower(Name);
  
  // IBM uses specific conventions for array descriptors
  if (Name.find("_array") != std::string::npos) {
    Result = "_xl_array_" + Result + "_";
  } else if (Name.find("_descriptor") != std::string::npos) {
    Result = "_xl_desc_" + Result + "_";
  } else {
    // Standard array parameter - may need descriptor handling
    Result = "_xl_arr_" + Result + "_";
  }
  
  return Result;
}

std::string NameManglingHandler::applyParameterPassingMangling(StringRef Name, FortranCompiler Compiler) const {
  // Handle Fortran parameter passing conventions
  std::string Result = Name.str();
  
  switch (Compiler) {
    case FortranCompiler::GFortran:
      // gfortran passes all parameters by reference
      // Character strings get hidden length parameters
      if (Name.find("_char") != std::string::npos || Name.find("_string") != std::string::npos) {
        Result += "_len";
      }
      break;
      
    case FortranCompiler::Intel:
      // Intel Fortran has similar conventions but with different suffixes
      if (Name.find("_char") != std::string::npos) {
        Result += "_$LEN";
      }
      break;
      
    case FortranCompiler::PGI:
      // PGI uses different conventions for character lengths
      if (Name.find("_char") != std::string::npos) {
        Result += "_len_";
      }
      break;
      
    case FortranCompiler::Cray:
      // Cray uses uppercase conventions
      if (Name.find("_char") != std::string::npos) {
        Result = stringToUpper(Result) + "_LEN";
      }
      break;
      
    case FortranCompiler::IBM:
      // IBM XL Fortran uses specific conventions
      if (Name.find("_char") != std::string::npos) {
        Result += "_xl_len_";
      }
      break;
      
    default:
      // Default behavior
      break;
  }
  
  return Result;
}

std::string NameManglingHandler::applyCharacterLengthMangling(StringRef Name, FortranCompiler Compiler) const {
  // Handle Fortran character string length parameters
  std::string Result = Name.str();
  
  // Different compilers handle character length parameters differently
  switch (Compiler) {
    case FortranCompiler::GFortran:
      // gfortran appends hidden length parameters at the end
      Result += "_hidden_len";
      break;
      
    case FortranCompiler::Intel:
      // Intel Fortran uses specific naming for length parameters
      Result += "_$LEN";
      break;
      
    case FortranCompiler::PGI:
      // PGI uses different conventions
      Result += "_pgi_len";
      break;
      
    case FortranCompiler::Cray:
      // Cray uses uppercase
      Result = stringToUpper(Result) + "_CRAY_LEN";
      break;
      
    case FortranCompiler::IBM:
      // IBM XL Fortran
      Result += "_xl_strlen";
      break;
      
    default:
      Result += "_len";
      break;
  }
  
  return Result;
}

std::string NameManglingHandler::applyOptionalParameterMangling(StringRef Name, FortranCompiler Compiler) const {
  // Handle Fortran optional parameter mangling
  std::string Result = Name.str();
  
  // Optional parameters may need special handling
  switch (Compiler) {
    case FortranCompiler::GFortran:
      // gfortran may add presence flags for optional parameters
      Result += "_present";
      break;
      
    case FortranCompiler::Intel:
      // Intel Fortran uses different conventions
      Result += "_$PRESENT";
      break;
      
    case FortranCompiler::PGI:
      // PGI conventions
      Result += "_opt_present";
      break;
      
    case FortranCompiler::Cray:
      // Cray conventions
      Result = stringToUpper(Result) + "_PRESENT";
      break;
      
    case FortranCompiler::IBM:
      // IBM XL Fortran
      Result += "_xl_present";
      break;
      
    default:
      Result += "_opt";
      break;
  }
  
  return Result;
}

std::string NameManglingHandler::handleMPIFortranInterface(StringRef Name, FortranCompiler Compiler) const {
  // Handle different Fortran MPI binding interfaces
  
  // Check which interface this function belongs to
  if (Name.find("_f08") != std::string::npos || Name.find("_F08") != std::string::npos) {
    return handleMPIF08Module(Name, Compiler);
  } else if (Name.find("_f90") != std::string::npos || Name.find("_F90") != std::string::npos) {
    return handleMPIModule(Name, Compiler);
  } else {
    // Assume traditional mpif.h interface
    return handleMPIFH(Name, Compiler);
  }
}

std::string NameManglingHandler::handleMPIFH(StringRef Name, FortranCompiler Compiler) const {
  // Handle traditional mpif.h interface
  // This is the oldest interface, typically uses simple mangling
  return mangleFortranName(Name, Compiler);
}

std::string NameManglingHandler::handleMPIModule(StringRef Name, FortranCompiler Compiler) const {
  // Handle Fortran 90 mpi module interface
  std::string Result = Name.str();
  
  // Module-qualified names typically have module prefix
  switch (Compiler) {
    case FortranCompiler::GFortran:
      Result = "__mpi_MOD_" + stringToLower(Name) + "_";
      break;
      
    case FortranCompiler::Intel:
      Result = "mpi_mp_" + stringToLower(Name) + "_";
      break;
      
    case FortranCompiler::PGI:
      Result = "_mpi_" + stringToLower(Name) + "_";
      break;
      
    case FortranCompiler::Cray:
      Result = "MPI_" + stringToUpper(Name);
      break;
      
    case FortranCompiler::IBM:
      Result = "_xl_mpi_" + stringToLower(Name) + "_";
      break;
      
    default:
      Result = "__mpi_MOD_" + stringToLower(Name) + "_";
      break;
  }
  
  return Result;
}

std::string NameManglingHandler::handleMPIF08Module(StringRef Name, FortranCompiler Compiler) const {
  // Handle Fortran 2008 mpi_f08 module interface
  std::string Result = Name.str();
  
  // F2008 interface uses more sophisticated mangling
  switch (Compiler) {
    case FortranCompiler::GFortran:
      Result = "__mpi_f08_MOD_" + stringToLower(Name) + "_";
      break;
      
    case FortranCompiler::Intel:
      Result = "mpi_f08_mp_" + stringToLower(Name) + "_";
      break;
      
    case FortranCompiler::PGI:
      Result = "_mpi_f08_" + stringToLower(Name) + "_";
      break;
      
    case FortranCompiler::Cray:
      Result = "MPI_F08_" + stringToUpper(Name);
      break;
      
    case FortranCompiler::IBM:
      Result = "_xl_mpi_f08_" + stringToLower(Name) + "_";
      break;
      
    default:
      Result = "__mpi_f08_MOD_" + stringToLower(Name) + "_";
      break;
  }
  
  return Result;
}

std::string NameManglingHandler::handleCInteropMangling(StringRef Name, FortranCompiler Compiler) const {
  // Handle Fortran C interoperability (ISO_C_BINDING)
  std::string Result = Name.str();
  
  // C interoperable functions typically use C naming conventions
  // but may have compiler-specific prefixes
  switch (Compiler) {
    case FortranCompiler::GFortran:
      // gfortran C interop functions often keep C names
      Result = Name.str(); // No mangling for C interop
      break;
      
    case FortranCompiler::Intel:
      // Intel may add specific prefixes
      if (Name.find("_c_") == std::string::npos) {
        Result = "c_" + Name.str();
      }
      break;
      
    case FortranCompiler::PGI:
      // PGI C interop conventions
      Result = "pgi_c_" + Name.str();
      break;
      
    case FortranCompiler::Cray:
      // Cray C interop
      Result = "CRAY_C_" + stringToUpper(Name);
      break;
      
    case FortranCompiler::IBM:
      // IBM C interop
      Result = "xl_c_" + Name.str();
      break;
      
    default:
      // Default: no mangling for C interop
      Result = Name.str();
      break;
  }
  
  return Result;
}

//===----------------------------------------------------------------------===//
// C++ MPI Binding Support Implementation
//===----------------------------------------------------------------------===//

void MPIFunctionDatabase::addCXXEnvironmentFunctions() {
  // C++ MPI environment functions (deprecated in MPI-3.0 but may still be encountered)
  // These use the MPI:: namespace and class-based interfaces
  
  // MPI::Init - C++ version
  addDetailedFunction("MPI::Init", MPIFunctionType::Environment, Language::CXX, false, false, {
    {"argc", ParameterRole::Count, true, false, false},
    {"argv", ParameterRole::Buffer, true, false, false}
  }, true, "C++ MPI initialization (deprecated)", "2.0");
  
  // Alternative mangled names for C++ MPI::Init
  addDetailedFunction("_ZN3MPI4InitERiRPPc", MPIFunctionType::Environment, Language::CXX, false, false, {
    {"argc", ParameterRole::Count, true, false, false},
    {"argv", ParameterRole::Buffer, true, false, false}
  }, true, "C++ MPI initialization (mangled)", "2.0");
  
  // MPI::Finalize - C++ version
  addDetailedFunction("MPI::Finalize", MPIFunctionType::Environment, Language::CXX, false, false, {
  }, true, "C++ MPI finalization (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI8FinalizeEv", MPIFunctionType::Environment, Language::CXX, false, false, {
  }, true, "C++ MPI finalization (mangled)", "2.0");
  
  // MPI::Is_initialized - C++ version
  addDetailedFunction("MPI::Is_initialized", MPIFunctionType::Environment, Language::CXX, false, false, {
  }, true, "C++ MPI initialization check (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI14Is_initializedEv", MPIFunctionType::Environment, Language::CXX, false, false, {
  }, true, "C++ MPI initialization check (mangled)", "2.0");
  
  // MPI::Is_finalized - C++ version
  addDetailedFunction("MPI::Is_finalized", MPIFunctionType::Environment, Language::CXX, false, false, {
  }, true, "C++ MPI finalization check (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI12Is_finalizedEv", MPIFunctionType::Environment, Language::CXX, false, false, {
  }, true, "C++ MPI finalization check (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXPointToPointFunctions() {
  // C++ MPI point-to-point communication functions
  // These are typically member functions of MPI::Comm class
  
  // MPI::Comm::Send - C++ version
  addDetailedFunction("MPI::Comm::Send", MPIFunctionType::PointToPoint, Language::CXX, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI send (deprecated)", "2.0");
  
  // Mangled version of MPI::Comm::Send
  addDetailedFunction("_ZNK3MPI4Comm4SendEPKviRKNS_8DatatypeEii", MPIFunctionType::PointToPoint, Language::CXX, false, false, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI send (mangled)", "2.0");
  
  // MPI::Comm::Recv - C++ version
  addDetailedFunction("MPI::Comm::Recv", MPIFunctionType::PointToPoint, Language::CXX, false, false, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI receive (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm4RecvEPviRKNS_8DatatypeEii", MPIFunctionType::PointToPoint, Language::CXX, false, false, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI receive (mangled)", "2.0");
  
  // MPI::Comm::Isend - C++ non-blocking send
  addDetailedFunction("MPI::Comm::Isend", MPIFunctionType::PointToPoint, Language::CXX, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI non-blocking send (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm5IsendEPKviRKNS_8DatatypeEii", MPIFunctionType::PointToPoint, Language::CXX, false, true, {
    {"buf", ParameterRole::Buffer, true, false, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"dest", ParameterRole::Destination, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI non-blocking send (mangled)", "2.0");
  
  // MPI::Comm::Irecv - C++ non-blocking receive
  addDetailedFunction("MPI::Comm::Irecv", MPIFunctionType::PointToPoint, Language::CXX, false, true, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI non-blocking receive (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm5IrecvEPviRKNS_8DatatypeEii", MPIFunctionType::PointToPoint, Language::CXX, false, true, {
    {"buf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"source", ParameterRole::Source, true, false, false},
    {"tag", ParameterRole::Tag, true, false, false}
  }, true, "C++ MPI non-blocking receive (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXCollectiveFunctions() {
  // C++ MPI collective communication functions
  
  // MPI::Comm::Bcast - C++ broadcast
  addDetailedFunction("MPI::Comm::Bcast", MPIFunctionType::Collective, Language::CXX, true, false, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false}
  }, true, "C++ MPI broadcast (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm5BcastEPviRKNS_8DatatypeEi", MPIFunctionType::Collective, Language::CXX, true, false, {
    {"buffer", ParameterRole::Buffer, true, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"root", ParameterRole::Root, true, false, false}
  }, true, "C++ MPI broadcast (mangled)", "2.0");
  
  // MPI::Comm::Reduce - C++ reduce
  addDetailedFunction("MPI::Comm::Reduce", MPIFunctionType::Collective, Language::CXX, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"root", ParameterRole::Root, true, false, false}
  }, true, "C++ MPI reduce (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm6ReduceEPKvPviRKNS_8DatatypeERKNS_2OpEi", MPIFunctionType::Collective, Language::CXX, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false},
    {"root", ParameterRole::Root, true, false, false}
  }, true, "C++ MPI reduce (mangled)", "2.0");
  
  // MPI::Comm::Allreduce - C++ allreduce
  addDetailedFunction("MPI::Comm::Allreduce", MPIFunctionType::Collective, Language::CXX, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false}
  }, true, "C++ MPI allreduce (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm9AllreduceEPKvPviRKNS_8DatatypeERKNS_2OpE", MPIFunctionType::Collective, Language::CXX, true, false, {
    {"sendbuf", ParameterRole::Buffer, true, false, false},
    {"recvbuf", ParameterRole::Buffer, false, true, false},
    {"count", ParameterRole::Count, true, false, false},
    {"datatype", ParameterRole::Datatype, true, false, false},
    {"op", ParameterRole::Operation, true, false, false}
  }, true, "C++ MPI allreduce (mangled)", "2.0");
  
  // MPI::Comm::Barrier - C++ barrier
  addDetailedFunction("MPI::Comm::Barrier", MPIFunctionType::Collective, Language::CXX, true, false, {
  }, true, "C++ MPI barrier (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm7BarrierEv", MPIFunctionType::Collective, Language::CXX, true, false, {
  }, true, "C++ MPI barrier (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXCommunicatorFunctions() {
  // C++ MPI communicator management functions
  
  // MPI::Comm::Dup - C++ communicator duplication
  addDetailedFunction("MPI::Comm::Dup", MPIFunctionType::Communicator, Language::CXX, false, false, {
  }, true, "C++ MPI communicator duplication (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm3DupEv", MPIFunctionType::Communicator, Language::CXX, false, false, {
  }, true, "C++ MPI communicator duplication (mangled)", "2.0");
  
  // MPI::Comm::Split - C++ communicator split
  addDetailedFunction("MPI::Comm::Split", MPIFunctionType::Communicator, Language::CXX, false, false, {
    {"color", ParameterRole::Flag, true, false, false},
    {"key", ParameterRole::Flag, true, false, false}
  }, true, "C++ MPI communicator split (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm5SplitEii", MPIFunctionType::Communicator, Language::CXX, false, false, {
    {"color", ParameterRole::Flag, true, false, false},
    {"key", ParameterRole::Flag, true, false, false}
  }, true, "C++ MPI communicator split (mangled)", "2.0");
  
  // MPI::Comm::Get_rank - C++ get rank
  addDetailedFunction("MPI::Comm::Get_rank", MPIFunctionType::Communicator, Language::CXX, false, false, {
  }, true, "C++ MPI get rank (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm8Get_rankEv", MPIFunctionType::Communicator, Language::CXX, false, false, {
  }, true, "C++ MPI get rank (mangled)", "2.0");
  
  // MPI::Comm::Get_size - C++ get size
  addDetailedFunction("MPI::Comm::Get_size", MPIFunctionType::Communicator, Language::CXX, false, false, {
  }, true, "C++ MPI get size (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI4Comm8Get_sizeEv", MPIFunctionType::Communicator, Language::CXX, false, false, {
  }, true, "C++ MPI get size (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXDatatypeFunctions() {
  // C++ MPI datatype functions
  
  // MPI::Datatype::Commit - C++ datatype commit
  addDetailedFunction("MPI::Datatype::Commit", MPIFunctionType::Datatype, Language::CXX, false, false, {
  }, true, "C++ MPI datatype commit (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI8Datatype6CommitEv", MPIFunctionType::Datatype, Language::CXX, false, false, {
  }, true, "C++ MPI datatype commit (mangled)", "2.0");
  
  // MPI::Datatype::Free - C++ datatype free
  addDetailedFunction("MPI::Datatype::Free", MPIFunctionType::Datatype, Language::CXX, false, false, {
  }, true, "C++ MPI datatype free (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI8Datatype4FreeEv", MPIFunctionType::Datatype, Language::CXX, false, false, {
  }, true, "C++ MPI datatype free (mangled)", "2.0");
  
  // MPI::Datatype::Get_size - C++ get datatype size
  addDetailedFunction("MPI::Datatype::Get_size", MPIFunctionType::Datatype, Language::CXX, false, false, {
  }, true, "C++ MPI get datatype size (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI8Datatype8Get_sizeEv", MPIFunctionType::Datatype, Language::CXX, false, false, {
  }, true, "C++ MPI get datatype size (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXRequestFunctions() {
  // C++ MPI request management functions
  
  // MPI::Request::Wait - C++ request wait
  addDetailedFunction("MPI::Request::Wait", MPIFunctionType::Request, Language::CXX, false, false, {
  }, true, "C++ MPI request wait (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI7Request4WaitEv", MPIFunctionType::Request, Language::CXX, false, false, {
  }, true, "C++ MPI request wait (mangled)", "2.0");
  
  // MPI::Request::Test - C++ request test
  addDetailedFunction("MPI::Request::Test", MPIFunctionType::Request, Language::CXX, false, false, {
  }, true, "C++ MPI request test (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI7Request4TestEv", MPIFunctionType::Request, Language::CXX, false, false, {
  }, true, "C++ MPI request test (mangled)", "2.0");
  
  // MPI::Request::Free - C++ request free
  addDetailedFunction("MPI::Request::Free", MPIFunctionType::Request, Language::CXX, false, false, {
  }, true, "C++ MPI request free (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI7Request4FreeEv", MPIFunctionType::Request, Language::CXX, false, false, {
  }, true, "C++ MPI request free (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXGroupFunctions() {
  // C++ MPI group management functions
  
  // MPI::Group::Incl - C++ group include
  addDetailedFunction("MPI::Group::Incl", MPIFunctionType::Group, Language::CXX, false, false, {
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false}
  }, true, "C++ MPI group include (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI5Group4InclEiPKi", MPIFunctionType::Group, Language::CXX, false, false, {
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false}
  }, true, "C++ MPI group include (mangled)", "2.0");
  
  // MPI::Group::Excl - C++ group exclude
  addDetailedFunction("MPI::Group::Excl", MPIFunctionType::Group, Language::CXX, false, false, {
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false}
  }, true, "C++ MPI group exclude (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI5Group4ExclEiPKi", MPIFunctionType::Group, Language::CXX, false, false, {
    {"n", ParameterRole::Count, true, false, false},
    {"ranks", ParameterRole::Rank, true, false, false}
  }, true, "C++ MPI group exclude (mangled)", "2.0");
  
  // MPI::Group::Free - C++ group free
  addDetailedFunction("MPI::Group::Free", MPIFunctionType::Group, Language::CXX, false, false, {
  }, true, "C++ MPI group free (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI5Group4FreeEv", MPIFunctionType::Group, Language::CXX, false, false, {
  }, true, "C++ MPI group free (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXWindowFunctions() {
  // C++ MPI window (RMA) functions
  
  // MPI::Win::Create - C++ window create
  addDetailedFunction("MPI::Win::Create", MPIFunctionType::Window, Language::CXX, false, false, {
    {"base", ParameterRole::Buffer, true, false, false},
    {"size", ParameterRole::Size, true, false, false},
    {"disp_unit", ParameterRole::Count, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, true, "C++ MPI window create (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI3Win6CreateEPvliRKNS_4InfoERKNS_4CommE", MPIFunctionType::Window, Language::CXX, false, false, {
    {"base", ParameterRole::Buffer, true, false, false},
    {"size", ParameterRole::Size, true, false, false},
    {"disp_unit", ParameterRole::Count, true, false, false},
    {"info", ParameterRole::Info, true, false, false},
    {"comm", ParameterRole::Communicator, true, false, false}
  }, true, "C++ MPI window create (mangled)", "2.0");
  
  // MPI::Win::Free - C++ window free
  addDetailedFunction("MPI::Win::Free", MPIFunctionType::Window, Language::CXX, false, false, {
  }, true, "C++ MPI window free (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI3Win4FreeEv", MPIFunctionType::Window, Language::CXX, false, false, {
  }, true, "C++ MPI window free (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXInfoFunctions() {
  // C++ MPI info object functions
  
  // MPI::Info::Create - C++ info create
  addDetailedFunction("MPI::Info::Create", MPIFunctionType::Info, Language::CXX, false, false, {
  }, true, "C++ MPI info create (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI4Info6CreateEv", MPIFunctionType::Info, Language::CXX, false, false, {
  }, true, "C++ MPI info create (mangled)", "2.0");
  
  // MPI::Info::Set - C++ info set
  addDetailedFunction("MPI::Info::Set", MPIFunctionType::Info, Language::CXX, false, false, {
    {"key", ParameterRole::Buffer, true, false, false},
    {"value", ParameterRole::Buffer, true, false, false}
  }, true, "C++ MPI info set (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI4Info3SetEPKcS2_", MPIFunctionType::Info, Language::CXX, false, false, {
    {"key", ParameterRole::Buffer, true, false, false},
    {"value", ParameterRole::Buffer, true, false, false}
  }, true, "C++ MPI info set (mangled)", "2.0");
  
  // MPI::Info::Free - C++ info free
  addDetailedFunction("MPI::Info::Free", MPIFunctionType::Info, Language::CXX, false, false, {
  }, true, "C++ MPI info free (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI4Info4FreeEv", MPIFunctionType::Info, Language::CXX, false, false, {
  }, true, "C++ MPI info free (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXErrorFunctions() {
  // C++ MPI error handling functions
  
  // MPI::Get_error_class - C++ get error class
  addDetailedFunction("MPI::Get_error_class", MPIFunctionType::Error, Language::CXX, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false}
  }, true, "C++ MPI get error class (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI15Get_error_classEi", MPIFunctionType::Error, Language::CXX, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false}
  }, true, "C++ MPI get error class (mangled)", "2.0");
  
  // MPI::Get_error_string - C++ get error string
  addDetailedFunction("MPI::Get_error_string", MPIFunctionType::Error, Language::CXX, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"string", ParameterRole::ErrorString, false, true, false}
  }, true, "C++ MPI get error string (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI16Get_error_stringEiPc", MPIFunctionType::Error, Language::CXX, false, false, {
    {"errorcode", ParameterRole::ErrorCode, true, false, false},
    {"string", ParameterRole::ErrorString, false, true, false}
  }, true, "C++ MPI get error string (mangled)", "2.0");
}

void MPIFunctionDatabase::addCXXTopologyFunctions() {
  // C++ MPI topology functions
  
  // MPI::Cartcomm::Create - C++ Cartesian topology create
  addDetailedFunction("MPI::Cartcomm::Create", MPIFunctionType::Topology, Language::CXX, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ndims", ParameterRole::Count, true, false, false},
    {"dims", ParameterRole::Count, true, false, false},
    {"periods", ParameterRole::Flag, true, false, false},
    {"reorder", ParameterRole::Flag, true, false, false}
  }, true, "C++ MPI Cartesian create (deprecated)", "2.0");
  
  addDetailedFunction("_ZN3MPI8Cartcomm6CreateERKNS_4CommEiPKiPKbb", MPIFunctionType::Topology, Language::CXX, false, false, {
    {"comm", ParameterRole::Communicator, true, false, false},
    {"ndims", ParameterRole::Count, true, false, false},
    {"dims", ParameterRole::Count, true, false, false},
    {"periods", ParameterRole::Flag, true, false, false},
    {"reorder", ParameterRole::Flag, true, false, false}
  }, true, "C++ MPI Cartesian create (mangled)", "2.0");
  
  // MPI::Cartcomm::Get_coords - C++ get Cartesian coordinates
  addDetailedFunction("MPI::Cartcomm::Get_coords", MPIFunctionType::Topology, Language::CXX, false, false, {
    {"rank", ParameterRole::Rank, true, false, false},
    {"maxdims", ParameterRole::Count, true, false, false},
    {"coords", ParameterRole::Buffer, false, true, false}
  }, true, "C++ MPI get Cartesian coordinates (deprecated)", "2.0");
  
  addDetailedFunction("_ZNK3MPI8Cartcomm10Get_coordsEiiPi", MPIFunctionType::Topology, Language::CXX, false, false, {
    {"rank", ParameterRole::Rank, true, false, false},
    {"maxdims", ParameterRole::Count, true, false, false},
    {"coords", ParameterRole::Buffer, false, true, false}
  }, true, "C++ MPI get Cartesian coordinates (mangled)", "2.0");
}
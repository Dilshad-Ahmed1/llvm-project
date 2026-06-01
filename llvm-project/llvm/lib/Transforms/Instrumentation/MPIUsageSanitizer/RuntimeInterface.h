//===- RuntimeInterface.h - MPI Sanitizer Runtime Interface ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the RuntimeInterface class which defines the interface
// between the instrumentation pass and the runtime library.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_RUNTIMEINTERFACE_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_RUNTIMEINTERFACE_H

#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

namespace llvm {

/// Runtime Interface for MPI Sanitizer
///
/// Defines the interface between the instrumentation pass and the runtime
/// library that performs actual error detection and monitoring.
class RuntimeInterface {
public:
  RuntimeInterface() = default;
  ~RuntimeInterface() = default;
  
  /// Get function type for pre-call hook
  /// void __mpi_sanitizer_pre_call(const char* name, void** params, int count, const char* loc)
  static FunctionType* getPreHookType(LLVMContext& Ctx);
  
  /// Get function type for post-call hook
  /// void __mpi_sanitizer_post_call(const char* name, void* ret, int error, const char* loc)
  static FunctionType* getPostHookType(LLVMContext& Ctx);
  
  /// Get function type for performance begin hook
  /// void __mpi_sanitizer_performance_begin(const char* name, const char* type)
  static FunctionType* getPerformanceBeginHookType(LLVMContext& Ctx);
  
  /// Get function type for performance end hook
  /// void __mpi_sanitizer_performance_end(const char* name, const char* type)
  static FunctionType* getPerformanceEndHookType(LLVMContext& Ctx);
  
  /// Get function type for communication volume monitoring hook
  /// void __mpi_sanitizer_comm_volume(const char* name, size_t volume, const char* pattern)
  static FunctionType* getCommunicationVolumeHookType(LLVMContext& Ctx);
  
  /// Get function type for communication pattern monitoring hook
  /// void __mpi_sanitizer_comm_pattern(const char* name, int src, int dest, int tag, const char* pattern_type)
  static FunctionType* getCommunicationPatternHookType(LLVMContext& Ctx);
  
  /// Get function type for collective timing hook
  /// void __mpi_sanitizer_collective_timing(const char* name, int comm_size, double* timing_data)
  static FunctionType* getCollectiveTimingHookType(LLVMContext& Ctx);
  
  /// Get function type for synchronization monitoring hook
  /// void __mpi_sanitizer_sync_point(const char* name, int sync_type, const char* location)
  static FunctionType* getSynchronizationHookType(LLVMContext& Ctx);
  
  /// Validate that hook function signatures match expected interface
  static bool validateHookSignature(Function* HookFunc, FunctionType* ExpectedType);
  
  /// Get the standard hook function names
  static constexpr const char* getPreHookName() { return "__mpi_sanitizer_pre_call"; }
  static constexpr const char* getPostHookName() { return "__mpi_sanitizer_post_call"; }
  static constexpr const char* getPerformanceBeginHookName() { return "__mpi_sanitizer_performance_begin"; }
  static constexpr const char* getPerformanceEndHookName() { return "__mpi_sanitizer_performance_end"; }
  static constexpr const char* getCommunicationVolumeHookName() { return "__mpi_sanitizer_comm_volume"; }
  static constexpr const char* getCommunicationPatternHookName() { return "__mpi_sanitizer_comm_pattern"; }
  static constexpr const char* getCollectiveTimingHookName() { return "__mpi_sanitizer_collective_timing"; }
  static constexpr const char* getSynchronizationHookName() { return "__mpi_sanitizer_sync_point"; }
};

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_RUNTIMEINTERFACE_H
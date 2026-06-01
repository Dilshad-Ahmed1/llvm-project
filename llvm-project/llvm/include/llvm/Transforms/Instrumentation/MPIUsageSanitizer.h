//===- MPIUsageSanitizer.h - MPI Usage Sanitizer instrumentation -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the MPIUsageSanitizer class which instruments MPI programs
// to enable runtime error detection and performance monitoring.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_H

#include "llvm/IR/PassManager.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Analysis/CFGPrinter.h"

namespace llvm {
class Module;
class Function;
class ModulePass;
class FunctionPass;
class raw_ostream;

/// Configuration options for MPI Usage Sanitizer instrumentation
struct MPIUsageSanitizerOptions {
  /// Instrumentation level: Full, Lightweight, or Performance-only
  enum class InstrumentationLevel {
    Full,        // Instrument all MPI operations
    Lightweight, // Instrument only error-prone operations
    Performance  // Only performance monitoring hooks
  };
  
  InstrumentationLevel Level = InstrumentationLevel::Full;
  bool EnableOptimizations = true;
  bool EnablePerformanceMonitoring = false;
  bool EnableDeadlockDetection = true;
  bool EnableDataRaceDetection = true;
  std::string ConfigFile;
};

/// Public interface to the MPI Usage Sanitizer module pass for instrumenting
/// MPI programs to detect runtime errors and performance issues.
///
/// This pass identifies MPI function calls in LLVM IR and inserts runtime
/// hooks that allow the MPI Usage Sanitizer runtime library to monitor
/// and validate MPI operations.
class MPIUsageSanitizerPass : public PassInfoMixin<MPIUsageSanitizerPass> {
public:
  LLVM_ABI
  MPIUsageSanitizerPass(const MPIUsageSanitizerOptions &Options = {});
  
  LLVM_ABI PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  
  LLVM_ABI void
  printPipeline(raw_ostream &OS,
                function_ref<StringRef(StringRef)> MapClassName2PassName);
  
  static bool isRequired() { return true; }

private:
  MPIUsageSanitizerOptions Options;
};

/// Function-level MPI Usage Sanitizer pass for function-specific analysis
class MPIUsageSanitizerFunctionPass : public PassInfoMixin<MPIUsageSanitizerFunctionPass> {
public:
  LLVM_ABI
  MPIUsageSanitizerFunctionPass(const MPIUsageSanitizerOptions &Options = {});
  
  LLVM_ABI PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  
  LLVM_ABI void
  printPipeline(raw_ostream &OS,
                function_ref<StringRef(StringRef)> MapClassName2PassName);

private:
  MPIUsageSanitizerOptions Options;
};

//===----------------------------------------------------------------------===//
// Legacy Pass Manager Support
//===----------------------------------------------------------------------===//

/// Create a legacy module pass for MPI Usage Sanitizer
ModulePass *createMPIUsageSanitizerLegacyPass();

/// Create a legacy module pass for MPI Usage Sanitizer with options
ModulePass *createMPIUsageSanitizerLegacyPass(const MPIUsageSanitizerOptions &Options);

/// Create a legacy function pass for MPI Usage Sanitizer
FunctionPass *createMPIUsageSanitizerFunctionLegacyPass();

/// Create a legacy function pass for MPI Usage Sanitizer with options
FunctionPass *createMPIUsageSanitizerFunctionLegacyPass(const MPIUsageSanitizerOptions &Options);

} // namespace llvm

#endif // LLVM_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_H
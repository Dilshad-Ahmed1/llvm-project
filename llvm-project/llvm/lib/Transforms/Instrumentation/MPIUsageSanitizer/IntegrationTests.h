//===- IntegrationTests.h - MPI Sanitizer Integration Tests ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares integration tests for the MPI Usage Sanitizer LLVM Pass,
// including pass manager integration, compiler flag processing, and end-to-end
// workflow validation.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_INTEGRATIONTESTS_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_INTEGRATIONTESTS_H

#include "MPISanitizerPass.h"
#include "ConfigurationManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/AsmParser/Parser.h"
#include "gtest/gtest.h"
#include <memory>
#include <string>
#include <vector>

namespace llvm {

/// Test result for pass manager integration
struct PassManagerTestResult {
  bool PassRegistered = false;
  bool PassExecuted = false;
  bool ModuleModified = false;
  std::string ErrorMessage;
  uint64_t ExecutionTimeUs = 0;
  uint64_t MemoryUsageBytes = 0;
  
  bool isSuccessful() const {
    return PassRegistered && PassExecuted && ErrorMessage.empty();
  }
};

/// Test result for compiler flag processing
struct CompilerFlagTestResult {
  bool FlagsProcessed = false;
  bool ConfigurationValid = false;
  std::string ParsedConfiguration;
  std::vector<std::string> Errors;
  std::vector<std::string> Warnings;
  
  bool isSuccessful() const {
    return FlagsProcessed && ConfigurationValid && Errors.empty();
  }
};

/// Test result for end-to-end workflow
struct WorkflowTestResult {
  bool ModuleParsed = false;
  bool PassExecuted = false;
  bool InstrumentationApplied = false;
  bool OutputGenerated = false;
  uint32_t MPICallsDetected = 0;
  uint32_t MPICallsInstrumented = 0;
  std::string ErrorMessage;
  
  bool isSuccessful() const {
    return ModuleParsed && PassExecuted && OutputGenerated && ErrorMessage.empty();
  }
};

/// Pass Manager Integration Tests
///
/// Tests that the MPI Sanitizer Pass integrates correctly with LLVM's
/// pass manager infrastructure, including both new and legacy pass managers.
class PassManagerIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test pass registration with new pass manager
  PassManagerTestResult testNewPassManagerRegistration();
  
  /// Test pass execution with new pass manager
  PassManagerTestResult testNewPassManagerExecution(Module& M);
  
  /// Test pass registration with legacy pass manager
  PassManagerTestResult testLegacyPassManagerRegistration();
  
  /// Test pass execution with legacy pass manager
  PassManagerTestResult testLegacyPassManagerExecution(Module& M);
  
  /// Test pass pipeline integration
  PassManagerTestResult testPassPipelineIntegration(Module& M);
  
  /// Test pass dependencies and ordering
  PassManagerTestResult testPassDependencies(Module& M);
  
  /// Create test module with MPI calls
  std::unique_ptr<Module> createTestModule();
  
  /// Verify pass execution results
  bool verifyPassResults(Module& Original, Module& Instrumented);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<PassBuilder> PB;
  std::unique_ptr<ModulePassManager> MPM;
  std::unique_ptr<ModuleAnalysisManager> MAM;
  std::unique_ptr<FunctionAnalysisManager> FAM;
  std::unique_ptr<CGSCCAnalysisManager> CGAM;
  std::unique_ptr<LoopAnalysisManager> LAM;
};

/// Compiler Flag Processing Tests
///
/// Tests command line option parsing, configuration file processing,
/// and flag validation for the MPI Sanitizer Pass.
class CompilerFlagProcessingTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test basic command line option parsing
  CompilerFlagTestResult testBasicOptionParsing(const std::vector<std::string>& Args);
  
  /// Test configuration file processing
  CompilerFlagTestResult testConfigurationFileProcessing(const std::string& ConfigFile);
  
  /// Test invalid flag handling
  CompilerFlagTestResult testInvalidFlagHandling(const std::vector<std::string>& Args);
  
  /// Test flag combination validation
  CompilerFlagTestResult testFlagCombinations(const std::vector<std::string>& Args);
  
  /// Test environment variable processing
  CompilerFlagTestResult testEnvironmentVariables(const std::map<std::string, std::string>& EnvVars);
  
  /// Test default configuration
  CompilerFlagTestResult testDefaultConfiguration();
  
  /// Create temporary configuration file
  std::string createTempConfigFile(const std::string& Content);
  
  /// Clean up temporary files
  void cleanupTempFiles();

protected:
  std::unique_ptr<ConfigurationManager> ConfigManager;
  std::vector<std::string> TempFiles;
  std::map<std::string, std::string> OriginalEnvVars;
};

/// End-to-End Workflow Tests
///
/// Tests complete workflows from source code to instrumented output,
/// validating the entire MPI Sanitizer Pass pipeline.
class EndToEndWorkflowTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test simple MPI program instrumentation
  WorkflowTestResult testSimpleMPIProgram();
  
  /// Test complex MPI program instrumentation
  WorkflowTestResult testComplexMPIProgram();
  
  /// Test multi-language MPI program instrumentation
  WorkflowTestResult testMultiLanguageMPIProgram();
  
  /// Test error handling in workflow
  WorkflowTestResult testErrorHandlingWorkflow();
  
  /// Test optimization workflow
  WorkflowTestResult testOptimizationWorkflow();
  
  /// Test configuration-driven workflow
  WorkflowTestResult testConfigurationDrivenWorkflow();
  
  /// Parse LLVM IR from string
  std::unique_ptr<Module> parseIR(const std::string& IR);
  
  /// Run complete instrumentation pipeline
  WorkflowTestResult runInstrumentationPipeline(Module& M, const PassConfiguration& Config);
  
  /// Verify instrumentation results
  bool verifyInstrumentationResults(Module& Original, Module& Instrumented, 
                                   uint32_t ExpectedCalls, uint32_t ExpectedInstrumented);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<MPISanitizerPass> Pass;
  std::unique_ptr<ConfigurationManager> ConfigManager;
};

/// Pass Integration with LLVM Infrastructure Tests
///
/// Tests integration with various LLVM infrastructure components
/// including analysis passes, optimization passes, and debugging tools.
class LLVMInfrastructureIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test integration with analysis passes
  bool testAnalysisPassIntegration();
  
  /// Test integration with optimization passes
  bool testOptimizationPassIntegration();
  
  /// Test integration with debugging information
  bool testDebugInfoIntegration();
  
  /// Test integration with metadata preservation
  bool testMetadataPreservation();
  
  /// Test integration with profile-guided optimization
  bool testPGOIntegration();
  
  /// Test integration with link-time optimization
  bool testLTOIntegration();

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<Module> TestModule;
};

/// Utility functions for integration testing

/// Create sample MPI programs for testing
namespace TestPrograms {
  /// Simple MPI program with basic point-to-point communication
  std::string getSimpleMPIProgram();
  
  /// Complex MPI program with collective operations and error handling
  std::string getComplexMPIProgram();
  
  /// Multi-language MPI program with C, C++, and Fortran bindings
  std::string getMultiLanguageMPIProgram();
  
  /// MPI program with performance-critical sections
  std::string getPerformanceCriticalMPIProgram();
  
  /// MPI program with error conditions
  std::string getErrorProneMP IProgram();
}

/// Test configuration generators
namespace TestConfigurations {
  /// Default configuration for testing
  PassConfiguration getDefaultConfig();
  
  /// Lightweight instrumentation configuration
  PassConfiguration getLightweightConfig();
  
  /// Full instrumentation configuration
  PassConfiguration getFullInstrumentationConfig();
  
  /// Performance-focused configuration
  PassConfiguration getPerformanceConfig();
  
  /// Debug-focused configuration
  PassConfiguration getDebugConfig();
}

/// Test utilities
namespace TestUtils {
  /// Compare two modules for semantic equivalence
  bool compareModules(Module& M1, Module& M2);
  
  /// Count MPI calls in a module
  uint32_t countMPICalls(Module& M);
  
  /// Count instrumentation hooks in a module
  uint32_t countInstrumentationHooks(Module& M);
  
  /// Measure module size and complexity
  struct ModuleMetrics {
    uint32_t FunctionCount = 0;
    uint32_t InstructionCount = 0;
    uint32_t BasicBlockCount = 0;
    uint32_t MPICallCount = 0;
    uint32_t HookCount = 0;
  };
  
  ModuleMetrics analyzeModule(Module& M);
  
  /// Validate module correctness
  bool validateModule(Module& M, std::string& ErrorMessage);
}

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_INTEGRATIONTESTS_H
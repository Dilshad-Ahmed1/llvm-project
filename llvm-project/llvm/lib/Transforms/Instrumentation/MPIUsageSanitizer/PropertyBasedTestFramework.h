//===- PropertyBasedTestFramework.h - Property-Based Testing ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the property-based testing framework for the MPI Usage
// Sanitizer LLVM Pass, including random MPI module generation and correctness
// property verification.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PROPERTYBASEDTESTFRAMEWORK_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PROPERTYBASEDTESTFRAMEWORK_H

#include "MPICallDetector.h"
#include "MetadataExtractor.h"
#include "HookInserter.h"
#include "StaticAnalyzer.h"
#include "OptimizationEngine.h"
#include "ConfigurationManager.h"
#include "ErrorHandler.h"
#include "RuntimeInterfaceValidator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <memory>
#include <random>
#include <vector>
#include <functional>

namespace llvm {

/// Property test result for a single iteration
struct PropertyTestResult {
  /// Whether the property held for this iteration
  bool PropertyHeld = false;
  
  /// Test iteration number
  uint32_t Iteration = 0;
  
  /// Generated test input description
  std::string InputDescription;
  
  /// Property violation description (if any)
  std::string ViolationDescription;
  
  /// Generated module (for debugging)
  std::unique_ptr<Module> TestModule;
  
  /// Execution time in microseconds
  uint64_t ExecutionTimeUs = 0;
  
  /// Memory usage in bytes
  uint64_t MemoryUsageBytes = 0;
};

/// Complete property test execution result
struct PropertyTestExecution {
  /// Property name being tested
  std::string PropertyName;
  
  /// Total number of iterations run
  uint32_t TotalIterations = 0;
  
  /// Number of iterations where property held
  uint32_t SuccessfulIterations = 0;
  
  /// Individual iteration results
  std::vector<PropertyTestResult> IterationResults;
  
  /// Overall test success
  bool TestPassed = false;
  
  /// First failing iteration (if any)
  uint32_t FirstFailureIteration = 0;
  
  /// Total execution time
  uint64_t TotalExecutionTimeUs = 0;
  
  /// Peak memory usage
  uint64_t PeakMemoryUsageBytes = 0;
  
  /// Success rate (0.0 to 1.0)
  double getSuccessRate() const {
    return TotalIterations > 0 ? static_cast<double>(SuccessfulIterations) / TotalIterations : 0.0;
  }
};

/// Random MPI module generation configuration
struct ModuleGenerationConfig {
  /// Number of functions to generate
  uint32_t NumFunctions = 5;
  
  /// Number of MPI calls per function
  uint32_t MPICallsPerFunction = 3;
  
  /// Probability of generating each MPI function type
  double PointToPointProbability = 0.3;
  double CollectiveProbability = 0.2;
  double RequestProbability = 0.2;
  double CommunicatorProbability = 0.1;
  double DatatypeProbability = 0.1;
  double EnvironmentProbability = 0.1;
  
  /// Probability of complex patterns
  double IndirectCallProbability = 0.1;
  double ConditionalCallProbability = 0.2;
  double LoopCallProbability = 0.15;
  
  /// Parameter generation settings
  uint32_t MaxBufferSize = 1024;
  uint32_t MaxArrayDimensions = 3;
  bool GenerateComplexTypes = true;
  bool GenerateVariadicCalls = false;
  
  /// Language binding probabilities
  double CProbability = 0.6;
  double CppProbability = 0.3;
  double FortranProbability = 0.1;
};

/// Random MPI module generator
class RandomMPIModuleGenerator {
public:
  RandomMPIModuleGenerator(LLVMContext& Context, uint32_t Seed = 0);
  
  /// Generate a random MPI module
  std::unique_ptr<Module> generateModule(const ModuleGenerationConfig& Config);
  
  /// Generate a specific MPI function pattern
  Function* generateMPIFunction(Module& M, StringRef Name, MPIFunctionType Type);
  
  /// Generate random MPI call sites within a function
  std::vector<CallInst*> generateMPICalls(Function& F, const ModuleGenerationConfig& Config);
  
  /// Generate complex MPI usage patterns
  void generateComplexPatterns(Function& F, const ModuleGenerationConfig& Config);
  
  /// Generate different language binding patterns
  void generateLanguageBindings(Module& M, const ModuleGenerationConfig& Config);
  
  /// Set random seed for reproducible generation
  void setSeed(uint32_t Seed) { RandomEngine.seed(Seed); }
  
  /// Get current random seed
  uint32_t getSeed() const { return CurrentSeed; }

private:
  LLVMContext& Context;
  std::mt19937 RandomEngine;
  uint32_t CurrentSeed;
  
  /// Generate random MPI function call
  CallInst* generateRandomMPICall(IRBuilder<>& Builder, MPIFunctionType Type);
  
  /// Generate random function parameters
  std::vector<Value*> generateRandomParameters(IRBuilder<>& Builder, 
                                               const std::vector<Type*>& ParamTypes);
  
  /// Generate random values of specific types
  Value* generateRandomValue(IRBuilder<>& Builder, Type* Ty);
  
  /// Generate random buffer parameters
  Value* generateRandomBuffer(IRBuilder<>& Builder, Type* ElementType, uint32_t Size);
  
  /// Generate random control flow patterns
  void generateRandomControlFlow(Function& F, const ModuleGenerationConfig& Config);
  
  /// Generate random indirect call patterns
  void generateIndirectCallPattern(Function& F);
  
  /// Generate random conditional call patterns
  void generateConditionalCallPattern(Function& F);
  
  /// Generate random loop call patterns
  void generateLoopCallPattern(Function& F);
};

/// Property verification helper functions
class PropertyVerificationHelpers {
public:
  PropertyVerificationHelpers(LLVMContext& Context);
  
  /// Verify complete MPI call detection property
  bool verifyCompleteMPICallDetection(Module& M, MPICallDetector& Detector);
  
  /// Verify accurate metadata extraction property
  bool verifyAccurateMetadataExtraction(Module& M, MetadataExtractor& Extractor);
  
  /// Verify semantic-preserving hook insertion property
  bool verifySemanticPreservingHookInsertion(Module& M, HookInserter& Inserter);
  
  /// Verify optimization correctness property
  bool verifyOptimizationCorrectness(Module& M, OptimizationEngine& Engine);
  
  /// Verify multi-language consistency property
  bool verifyMultiLanguageConsistency(Module& M, MPICallDetector& Detector);
  
  /// Verify error recovery and diagnostics property
  bool verifyErrorRecoveryAndDiagnostics(Module& M, ErrorHandler& Handler);
  
  /// Verify configuration-driven instrumentation property
  bool verifyConfigurationDrivenInstrumentation(Module& M, ConfigurationManager& Config);
  
  /// Verify performance monitoring integration property
  bool verifyPerformanceMonitoringIntegration(Module& M, HookInserter& Inserter);
  
  /// Helper: Count MPI calls in module
  uint32_t countMPICalls(Module& M);
  
  /// Helper: Verify module semantic equivalence
  bool verifySemanticEquivalence(Module& Original, Module& Instrumented);
  
  /// Helper: Check for memory leaks or corruption
  bool checkMemoryIntegrity(Module& M);
  
  /// Helper: Verify performance overhead is acceptable
  bool verifyPerformanceOverhead(Module& Original, Module& Instrumented, double MaxOverhead = 0.1);

private:
  LLVMContext& Context;
  
  /// Compare function semantics
  bool compareFunctionSemantics(Function& F1, Function& F2);
  
  /// Verify instruction preservation
  bool verifyInstructionPreservation(Function& Original, Function& Instrumented);
  
  /// Check for introduced side effects
  bool checkForSideEffects(Function& Original, Function& Instrumented);
};

/// Base class for MPI Pass property-based tests
class MPIPassPropertyTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Run a property test with specified iterations
  PropertyTestExecution runPropertyTest(const std::string& PropertyName,
                                        std::function<bool(Module&)> PropertyChecker,
                                        uint32_t Iterations = 100,
                                        const ModuleGenerationConfig& Config = ModuleGenerationConfig());
  
  /// Run property test with custom module generator
  PropertyTestExecution runPropertyTestWithGenerator(const std::string& PropertyName,
                                                     std::function<bool(Module&)> PropertyChecker,
                                                     std::function<std::unique_ptr<Module>()> ModuleGenerator,
                                                     uint32_t Iterations = 100);
  
  /// Generate test report
  void generateTestReport(const PropertyTestExecution& Execution, raw_ostream& OS);
  
  /// Verify property with detailed logging
  bool verifyPropertyWithLogging(const std::string& PropertyName, Module& M,
                                 std::function<bool(Module&)> PropertyChecker,
                                 PropertyTestResult& Result);
  
  /// Create minimal test module
  std::unique_ptr<Module> createMinimalTestModule();
  
  /// Create complex test module
  std::unique_ptr<Module> createComplexTestModule();
  
  /// Initialize all pass components
  void initializePassComponents();
  
  /// Reset pass components for next test
  void resetPassComponents();
  
  /// Get current memory usage
  uint64_t getCurrentMemoryUsage();
  
  /// Measure execution time
  uint64_t measureExecutionTime(std::function<void()> Operation);

protected:
  // Test infrastructure
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<RandomMPIModuleGenerator> ModuleGenerator;
  std::unique_ptr<PropertyVerificationHelpers> VerificationHelpers;
  
  // Pass components
  std::unique_ptr<MPICallDetector> CallDetector;
  std::unique_ptr<MetadataExtractor> MetadataExtractor;
  std::unique_ptr<HookInserter> HookInserter;
  std::unique_ptr<StaticAnalyzer> StaticAnalyzer;
  std::unique_ptr<OptimizationEngine> OptimizationEngine;
  std::unique_ptr<ConfigurationManager> ConfigurationManager;
  std::unique_ptr<ErrorHandler> ErrorHandler;
  std::unique_ptr<RuntimeInterfaceValidator> RuntimeValidator;
  std::unique_ptr<RuntimeInterface> RuntimeInterface;
  
  // Test configuration
  uint32_t DefaultIterations = 100;
  uint32_t RandomSeed = 42;
  bool VerboseLogging = false;
  bool SaveFailingModules = true;
  std::string TestOutputDirectory = "/tmp/mpi_sanitizer_tests";
};

/// Specific property test implementations

/// Property 1: Complete MPI Call Detection
class CompleteMPICallDetectionTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 2: Accurate Metadata Extraction
class AccurateMetadataExtractionTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 3: Semantic-Preserving Hook Insertion
class SemanticPreservingHookInsertionTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 4: Optimization Correctness
class OptimizationCorrectnessTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 5: Multi-Language Consistency
class MultiLanguageConsistencyTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 6: Error Recovery and Diagnostics
class ErrorRecoveryAndDiagnosticsTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 7: Configuration-Driven Instrumentation
class ConfigurationDrivenInstrumentationTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Property 8: Performance Monitoring Integration
class PerformanceMonitoringIntegrationTest : public MPIPassPropertyTest {
protected:
  bool checkProperty(Module& M);
};

/// Utility macros for property-based testing

#define DEFINE_PROPERTY_TEST(TestClass, PropertyName, Iterations) \
  TEST_F(TestClass, PropertyName) { \
    PropertyTestExecution Result = runPropertyTest( \
        #PropertyName, \
        [this](Module& M) { return checkProperty(M); }, \
        Iterations \
    ); \
    EXPECT_TRUE(Result.TestPassed) << "Property " #PropertyName " failed after " \
                                   << Result.FirstFailureIteration << " iterations"; \
    if (VerboseLogging) { \
      generateTestReport(Result, llvm::outs()); \
    } \
  }

#define DEFINE_CUSTOM_PROPERTY_TEST(TestClass, PropertyName, Generator, Iterations) \
  TEST_F(TestClass, PropertyName) { \
    PropertyTestExecution Result = runPropertyTestWithGenerator( \
        #PropertyName, \
        [this](Module& M) { return checkProperty(M); }, \
        Generator, \
        Iterations \
    ); \
    EXPECT_TRUE(Result.TestPassed) << "Property " #PropertyName " failed"; \
    if (VerboseLogging) { \
      generateTestReport(Result, llvm::outs()); \
    } \
  }

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PROPERTYBASEDTESTFRAMEWORK_H
//===- PerformanceTests.h - MPI Sanitizer Performance Tests ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares performance tests for the MPI Usage Sanitizer LLVM Pass,
// including compilation overhead measurement, runtime overhead analysis, and
// scalability validation.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PERFORMANCETESTS_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PERFORMANCETESTS_H

#include "MPISanitizerPass.h"
#include "ConfigurationManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Timer.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>
#include <chrono>

namespace llvm {

/// Performance measurement result
struct PerformanceMeasurement {
  /// Compilation time in microseconds
  uint64_t CompilationTimeUs = 0;
  
  /// Memory usage in bytes
  uint64_t MemoryUsageBytes = 0;
  
  /// Number of instructions before instrumentation
  uint32_t OriginalInstructions = 0;
  
  /// Number of instructions after instrumentation
  uint32_t InstrumentedInstructions = 0;
  
  /// Number of MPI calls detected
  uint32_t MPICallsDetected = 0;
  
  /// Number of hooks inserted
  uint32_t HooksInserted = 0;
  
  /// Module size before instrumentation (in bytes)
  uint64_t OriginalModuleSize = 0;
  
  /// Module size after instrumentation (in bytes)
  uint64_t InstrumentedModuleSize = 0;
  
  /// Calculate instruction overhead percentage
  double getInstructionOverhead() const {
    if (OriginalInstructions == 0) return 0.0;
    return (static_cast<double>(InstrumentedInstructions - OriginalInstructions) / OriginalInstructions) * 100.0;
  }
  
  /// Calculate size overhead percentage
  double getSizeOverhead() const {
    if (OriginalModuleSize == 0) return 0.0;
    return (static_cast<double>(InstrumentedModuleSize - OriginalModuleSize) / OriginalModuleSize) * 100.0;
  }
  
  /// Calculate hooks per MPI call ratio
  double getHooksPerCall() const {
    if (MPICallsDetected == 0) return 0.0;
    return static_cast<double>(HooksInserted) / MPICallsDetected;
  }
};

/// Performance benchmark configuration
struct BenchmarkConfiguration {
  /// Number of functions to generate
  uint32_t FunctionCount = 100;
  
  /// Number of MPI calls per function
  uint32_t MPICallsPerFunction = 5;
  
  /// Number of iterations for timing
  uint32_t TimingIterations = 10;
  
  /// Instrumentation configuration to test
  PassConfiguration InstrumentationConfig;
  
  /// Whether to enable detailed profiling
  bool EnableDetailedProfiling = false;
  
  /// Maximum acceptable compilation overhead (percentage)
  double MaxCompilationOverhead = 50.0;
  
  /// Maximum acceptable size overhead (percentage)
  double MaxSizeOverhead = 100.0;
  
  /// Maximum acceptable instruction overhead (percentage)
  double MaxInstructionOverhead = 200.0;
};

/// Scalability test result
struct ScalabilityTestResult {
  /// Test configuration
  BenchmarkConfiguration Config;
  
  /// Performance measurements for different scales
  std::vector<PerformanceMeasurement> Measurements;
  
  /// Whether the test passed scalability requirements
  bool PassedScalabilityTest = false;
  
  /// Error message if test failed
  std::string ErrorMessage;
  
  /// Calculate average compilation time
  double getAverageCompilationTime() const {
    if (Measurements.empty()) return 0.0;
    uint64_t Total = 0;
    for (const auto& M : Measurements) {
      Total += M.CompilationTimeUs;
    }
    return static_cast<double>(Total) / Measurements.size();
  }
  
  /// Calculate average memory usage
  double getAverageMemoryUsage() const {
    if (Measurements.empty()) return 0.0;
    uint64_t Total = 0;
    for (const auto& M : Measurements) {
      Total += M.MemoryUsageBytes;
    }
    return static_cast<double>(Total) / Measurements.size();
  }
};

/// Compilation Overhead Tests
///
/// Measures the compilation time overhead introduced by the MPI Sanitizer Pass
/// across different module sizes and complexity levels.
class CompilationOverheadTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Measure compilation overhead for a single module
  PerformanceMeasurement measureCompilationOverhead(Module& M, const PassConfiguration& Config);
  
  /// Test compilation overhead with small modules
  void testSmallModuleOverhead();
  
  /// Test compilation overhead with medium modules
  void testMediumModuleOverhead();
  
  /// Test compilation overhead with large modules
  void testLargeModuleOverhead();
  
  /// Test compilation overhead with different instrumentation levels
  void testInstrumentationLevelOverhead();
  
  /// Test compilation overhead with optimization enabled/disabled
  void testOptimizationOverhead();
  
  /// Generate test module of specified size
  std::unique_ptr<Module> generateTestModule(uint32_t FunctionCount, uint32_t MPICallsPerFunction);
  
  /// Measure baseline compilation time (without MPI sanitizer)
  uint64_t measureBaselineCompilation(Module& M);
  
  /// Measure instrumented compilation time (with MPI sanitizer)
  uint64_t measureInstrumentedCompilation(Module& M, const PassConfiguration& Config);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
  std::vector<BenchmarkConfiguration> TestConfigurations;
};

/// Memory Usage Tests
///
/// Measures memory consumption during pass execution and validates
/// that memory usage scales appropriately with module size.
class MemoryUsageTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Measure memory usage during pass execution
  PerformanceMeasurement measureMemoryUsage(Module& M, const PassConfiguration& Config);
  
  /// Test memory usage with different module sizes
  void testMemoryScaling();
  
  /// Test memory usage with different instrumentation configurations
  void testConfigurationMemoryImpact();
  
  /// Test for memory leaks during pass execution
  void testMemoryLeaks();
  
  /// Test peak memory usage
  void testPeakMemoryUsage();
  
  /// Get current memory usage
  uint64_t getCurrentMemoryUsage();
  
  /// Monitor memory usage during operation
  std::vector<uint64_t> monitorMemoryUsage(std::function<void()> Operation, uint32_t SampleIntervalMs = 10);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
  uint64_t BaselineMemoryUsage = 0;
};

/// Scalability Tests
///
/// Tests how the pass performance scales with increasing module size,
/// number of MPI calls, and complexity.
class ScalabilityTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test scalability with increasing number of functions
  ScalabilityTestResult testFunctionCountScalability();
  
  /// Test scalability with increasing number of MPI calls
  ScalabilityTestResult testMPICallCountScalability();
  
  /// Test scalability with increasing module complexity
  ScalabilityTestResult testComplexityScalability();
  
  /// Test scalability across different instrumentation levels
  ScalabilityTestResult testInstrumentationLevelScalability();
  
  /// Run scalability test with given parameters
  ScalabilityTestResult runScalabilityTest(const std::vector<BenchmarkConfiguration>& Configs);
  
  /// Validate scalability requirements
  bool validateScalabilityRequirements(const ScalabilityTestResult& Result);
  
  /// Generate performance report
  std::string generatePerformanceReport(const ScalabilityTestResult& Result);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
  
  /// Scalability thresholds
  static constexpr double MaxLinearGrowthFactor = 2.0;  // Max acceptable linear growth
  static constexpr double MaxQuadraticGrowthFactor = 1.5; // Max acceptable quadratic growth
};

/// Instrumentation Overhead Tests
///
/// Measures the overhead introduced by different types of instrumentation
/// and validates that it stays within acceptable limits.
class InstrumentationOverheadTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test overhead of pre-call hooks
  PerformanceMeasurement testPreCallHookOverhead();
  
  /// Test overhead of post-call hooks
  PerformanceMeasurement testPostCallHookOverhead();
  
  /// Test overhead of performance monitoring hooks
  PerformanceMeasurement testPerformanceMonitoringOverhead();
  
  /// Test overhead of error checking hooks
  PerformanceMeasurement testErrorCheckingOverhead();
  
  /// Test overhead of deadlock detection hooks
  PerformanceMeasurement testDeadlockDetectionOverhead();
  
  /// Test overhead of data race detection hooks
  PerformanceMeasurement testDataRaceDetectionOverhead();
  
  /// Test combined overhead of all instrumentation types
  PerformanceMeasurement testCombinedInstrumentationOverhead();
  
  /// Compare overhead across different MPI function types
  void testMPIFunctionTypeOverhead();
  
  /// Measure selective instrumentation benefits
  void testSelectiveInstrumentationBenefits();

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
  
  /// Create test module for specific instrumentation type
  std::unique_ptr<Module> createInstrumentationTestModule(const std::string& InstrumentationType);
  
  /// Measure overhead for specific configuration
  PerformanceMeasurement measureInstrumentationOverhead(Module& M, const PassConfiguration& Config);
};

/// Optimization Effectiveness Tests
///
/// Tests the effectiveness of optimization strategies in reducing
/// instrumentation overhead while maintaining correctness.
class OptimizationEffectivenessTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test static analysis optimization effectiveness
  void testStaticAnalysisOptimizations();
  
  /// Test selective instrumentation effectiveness
  void testSelectiveInstrumentationEffectiveness();
  
  /// Test optimization level impact
  void testOptimizationLevelImpact();
  
  /// Test configuration-driven optimization effectiveness
  void testConfigurationOptimizationEffectiveness();
  
  /// Compare optimized vs unoptimized performance
  void compareOptimizedVsUnoptimized();
  
  /// Measure optimization benefit ratio
  double measureOptimizationBenefit(Module& M, const PassConfiguration& OptimizedConfig, 
                                   const PassConfiguration& UnoptimizedConfig);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
};

/// Performance Regression Tests
///
/// Tests to detect performance regressions and ensure that performance
/// improvements are maintained over time.
class PerformanceRegressionTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test against baseline performance metrics
  void testBaselinePerformance();
  
  /// Test performance with standard benchmark suite
  void testBenchmarkSuite();
  
  /// Test performance regression detection
  void testRegressionDetection();
  
  /// Load baseline performance data
  bool loadBaselineData(const std::string& BaselineFile);
  
  /// Save current performance data as baseline
  void saveBaselineData(const std::string& BaselineFile, const std::vector<PerformanceMeasurement>& Data);
  
  /// Compare current performance against baseline
  bool compareAgainstBaseline(const std::vector<PerformanceMeasurement>& Current, 
                             const std::vector<PerformanceMeasurement>& Baseline,
                             double TolerancePercentage = 10.0);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
  std::vector<PerformanceMeasurement> BaselineData;
  
  /// Performance regression thresholds
  static constexpr double MaxRegressionPercentage = 15.0;
  static constexpr double MaxMemoryRegressionPercentage = 20.0;
};

/// Utility functions for performance testing

namespace PerformanceTestUtils {
  /// Create benchmark module with specified characteristics
  std::unique_ptr<Module> createBenchmarkModule(LLVMContext& Context, 
                                               uint32_t FunctionCount,
                                               uint32_t MPICallsPerFunction,
                                               bool IncludeComplexPatterns = false);
  
  /// Measure module size in bytes
  uint64_t measureModuleSize(Module& M);
  
  /// Count instructions in module
  uint32_t countInstructions(Module& M);
  
  /// Count MPI calls in module
  uint32_t countMPICalls(Module& M);
  
  /// Count instrumentation hooks in module
  uint32_t countInstrumentationHooks(Module& M);
  
  /// Generate performance report
  std::string generatePerformanceReport(const std::vector<PerformanceMeasurement>& Measurements,
                                       const std::string& TestName);
  
  /// Validate performance requirements
  bool validatePerformanceRequirements(const PerformanceMeasurement& Measurement,
                                      const BenchmarkConfiguration& Config);
  
  /// Calculate statistical metrics
  struct StatisticalMetrics {
    double Mean = 0.0;
    double StandardDeviation = 0.0;
    double Min = 0.0;
    double Max = 0.0;
    double Median = 0.0;
  };
  
  StatisticalMetrics calculateStatistics(const std::vector<double>& Values);
  
  /// Timer utility for high-precision timing
  class HighPrecisionTimer {
  public:
    HighPrecisionTimer();
    void start();
    void stop();
    uint64_t getElapsedMicroseconds() const;
    void reset();
    
  private:
    std::chrono::high_resolution_clock::time_point StartTime;
    std::chrono::high_resolution_clock::time_point EndTime;
    bool IsRunning = false;
  };
  
  /// Memory monitor for tracking memory usage over time
  class MemoryMonitor {
  public:
    MemoryMonitor();
    void startMonitoring(uint32_t SampleIntervalMs = 10);
    void stopMonitoring();
    std::vector<uint64_t> getMemoryTrace() const;
    uint64_t getPeakMemoryUsage() const;
    
  private:
    std::vector<uint64_t> MemoryTrace;
    uint32_t SampleInterval = 10;
    bool IsMonitoring = false;
    std::thread MonitoringThread;
  };
}

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PERFORMANCETESTS_H
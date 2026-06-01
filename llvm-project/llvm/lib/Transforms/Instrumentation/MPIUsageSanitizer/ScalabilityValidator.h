//===- ScalabilityValidator.h - MPI Sanitizer Scalability Validator ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the ScalabilityValidator class which validates that the
// MPI Usage Sanitizer Pass scales well with large codebases and maintains
// acceptable performance characteristics.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_SCALABILITYVALIDATOR_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_SCALABILITYVALIDATOR_H

#include "PerformanceProfiler.h"
#include "PassOptimizer.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <vector>

namespace llvm {

class Function;
class BasicBlock;

/// Scalability test configuration
struct ScalabilityTestConfig {
  /// Test module sizes (number of functions)
  std::vector<uint32_t> ModuleSizes = {100, 500, 1000, 2500, 5000, 10000};
  
  /// MPI calls per function for each test
  uint32_t MPICallsPerFunction = 8;
  
  /// Maximum acceptable compilation time per function (microseconds)
  uint64_t MaxCompilationTimePerFunctionUs = 1000;
  
  /// Maximum acceptable memory usage per function (bytes)
  uint64_t MaxMemoryPerFunctionBytes = 10240; // 10KB per function
  
  /// Maximum acceptable scalability factor (time complexity)
  double MaxScalabilityFactor = 2.0; // O(n^2) worst case
  
  /// Enable detailed profiling during tests
  bool EnableDetailedProfiling = true;
  
  /// Test timeout in seconds
  uint32_t TestTimeoutSeconds = 300; // 5 minutes per test
};

/// Scalability test results for a specific module size
struct ScalabilityTestResult {
  uint32_t ModuleSize = 0;
  uint32_t FunctionCount = 0;
  uint32_t InstructionCount = 0;
  uint32_t MPICallCount = 0;
  
  /// Performance metrics
  uint64_t CompilationTimeUs = 0;
  uint64_t MemoryUsageBytes = 0;
  uint32_t HooksInserted = 0;
  
  /// Derived metrics
  double CompilationTimePerFunctionUs = 0.0;
  double MemoryPerFunctionBytes = 0.0;
  double MPICallProcessingRateHz = 0.0;
  
  /// Test status
  bool Passed = false;
  std::string FailureReason;
  
  /// Detailed profiling data (if enabled)
  std::unique_ptr<PassPerformanceProfile> DetailedProfile;
};

/// Comprehensive scalability analysis results
struct ScalabilityAnalysis {
  std::vector<ScalabilityTestResult> TestResults;
  
  /// Overall scalability metrics
  double TimeComplexityFactor = 0.0;      // Estimated O(n^x) factor
  double MemoryComplexityFactor = 0.0;    // Estimated memory scaling
  double ProcessingRateStability = 0.0;   // Stability of processing rate
  
  /// Performance thresholds validation
  bool MeetsPerformanceThresholds = false;
  bool MeetsMemoryThresholds = false;
  bool MeetsScalabilityThresholds = false;
  
  /// Overall assessment
  bool PassesScalabilityValidation = false;
  std::vector<std::string> Recommendations;
  
  /// Regression analysis
  struct RegressionAnalysis {
    double TimeRSquared = 0.0;
    double MemoryRSquared = 0.0;
    std::string TimeComplexityClass;
    std::string MemoryComplexityClass;
  } Regression;
};

/// Large codebase simulation for realistic testing
class LargeCodebaseSimulator {
public:
  LargeCodebaseSimulator() = default;
  ~LargeCodebaseSimulator() = default;
  
  /// Create a realistic large MPI application module
  std::unique_ptr<Module> createLargeMPIApplication(LLVMContext& Context,
                                                   uint32_t FunctionCount,
                                                   uint32_t MPICallsPerFunction);
  
  /// Create module with realistic MPI usage patterns
  std::unique_ptr<Module> createRealisticMPIModule(LLVMContext& Context,
                                                   const std::string& ApplicationType,
                                                   uint32_t Scale);
  
  /// Simulate different MPI application patterns
  enum class MPIApplicationPattern {
    ScientificComputing,    ///< Dense computation with periodic communication
    DataParallel,          ///< High communication volume, regular patterns
    TaskParallel,          ///< Irregular communication, dynamic load balancing
    PipelineParallel,      ///< Sequential stages with point-to-point communication
    HybridMPIOpenMP        ///< Mixed MPI/OpenMP patterns
  };
  
  std::unique_ptr<Module> createPatternBasedModule(LLVMContext& Context,
                                                  MPIApplicationPattern Pattern,
                                                  uint32_t Scale);

private:
  /// Helper methods for creating realistic code patterns
  void addScientificComputingPattern(Module& M, uint32_t Scale);
  void addDataParallelPattern(Module& M, uint32_t Scale);
  void addTaskParallelPattern(Module& M, uint32_t Scale);
  void addPipelinePattern(Module& M, uint32_t Scale);
  void addHybridPattern(Module& M, uint32_t Scale);
  
  /// Create realistic function call graphs
  void createCallGraph(Module& M, uint32_t Depth, uint32_t Branching);
  
  /// Add realistic control flow patterns
  void addControlFlowComplexity(Function& F, uint32_t Complexity);
};

/// Main scalability validator
class ScalabilityValidator {
public:
  ScalabilityValidator(const ScalabilityTestConfig& Config);
  ~ScalabilityValidator() = default;
  
  /// Run comprehensive scalability validation
  ScalabilityAnalysis validateScalability(LLVMContext& Context);
  
  /// Test specific module size
  ScalabilityTestResult testModuleSize(LLVMContext& Context, uint32_t ModuleSize);
  
  /// Test with realistic MPI application patterns
  ScalabilityAnalysis validateWithRealisticPatterns(LLVMContext& Context);
  
  /// Analyze scalability trends from test results
  ScalabilityAnalysis analyzeScalabilityTrends(const std::vector<ScalabilityTestResult>& Results);
  
  /// Generate comprehensive scalability report
  std::string generateScalabilityReport(const ScalabilityAnalysis& Analysis) const;
  
  /// Validate against production requirements
  bool validateProductionReadiness(const ScalabilityAnalysis& Analysis) const;
  
  /// Get performance recommendations
  std::vector<std::string> generatePerformanceRecommendations(const ScalabilityAnalysis& Analysis) const;

private:
  ScalabilityTestConfig Config;
  std::unique_ptr<LargeCodebaseSimulator> Simulator;
  std::unique_ptr<PerformanceProfiler> Profiler;
  
  /// Helper methods
  ScalabilityTestResult runSingleTest(Module& M, uint32_t ExpectedSize);
  bool validateTestResult(const ScalabilityTestResult& Result) const;
  double calculateTimeComplexity(const std::vector<ScalabilityTestResult>& Results) const;
  double calculateMemoryComplexity(const std::vector<ScalabilityTestResult>& Results) const;
  std::string classifyComplexity(double Factor) const;
  
  /// Statistical analysis
  struct LinearRegression {
    double Slope = 0.0;
    double Intercept = 0.0;
    double RSquared = 0.0;
  };
  
  LinearRegression performLinearRegression(const std::vector<double>& X, 
                                          const std::vector<double>& Y) const;
  LinearRegression performLogRegression(const std::vector<double>& X, 
                                       const std::vector<double>& Y) const;
};

/// Scalability benchmark suite for automated testing
class ScalabilityBenchmarkSuite {
public:
  ScalabilityBenchmarkSuite() = default;
  ~ScalabilityBenchmarkSuite() = default;
  
  /// Run standard scalability benchmark suite
  bool runStandardBenchmarks(const std::string& OutputDir);
  
  /// Run extended benchmarks with realistic applications
  bool runExtendedBenchmarks(const std::string& OutputDir);
  
  /// Run regression benchmarks against baseline
  bool runRegressionBenchmarks(const std::string& BaselineFile, 
                              const std::string& OutputDir);
  
  /// Generate benchmark report
  void generateBenchmarkReport(const std::string& OutputFile) const;

private:
  std::vector<ScalabilityAnalysis> BenchmarkResults;
  
  /// Benchmark configurations
  ScalabilityTestConfig createStandardConfig() const;
  ScalabilityTestConfig createExtendedConfig() const;
  ScalabilityTestConfig createRegressionConfig() const;
};

/// Performance regression detector
class PerformanceRegressionDetector {
public:
  PerformanceRegressionDetector() = default;
  ~PerformanceRegressionDetector() = default;
  
  /// Compare current results against baseline
  struct RegressionAnalysis {
    bool HasTimeRegression = false;
    bool HasMemoryRegression = false;
    bool HasScalabilityRegression = false;
    
    double TimeRegressionPercent = 0.0;
    double MemoryRegressionPercent = 0.0;
    double ScalabilityRegressionPercent = 0.0;
    
    std::vector<std::string> RegressionDetails;
  };
  
  RegressionAnalysis detectRegression(const ScalabilityAnalysis& Current,
                                     const ScalabilityAnalysis& Baseline) const;
  
  /// Set regression thresholds
  void setRegressionThresholds(double TimeThreshold, double MemoryThreshold, 
                              double ScalabilityThreshold);

private:
  double TimeRegressionThreshold = 0.1;      // 10% time regression threshold
  double MemoryRegressionThreshold = 0.15;   // 15% memory regression threshold
  double ScalabilityRegressionThreshold = 0.2; // 20% scalability regression threshold
};

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_SCALABILITYVALIDATOR_H
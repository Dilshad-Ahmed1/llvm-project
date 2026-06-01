//===- PerformanceProfiler.h - MPI Sanitizer Performance Profiler ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the PerformanceProfiler class which provides detailed
// profiling capabilities for the MPI Usage Sanitizer Pass to identify
// performance bottlenecks and optimization opportunities.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PERFORMANCEPROFILER_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PERFORMANCEPROFILER_H

#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace llvm {

class Module;
class Function;
class Instruction;

/// Detailed performance metrics for a specific operation
struct PerformanceMetrics {
  /// Total execution time in microseconds
  uint64_t ExecutionTimeUs = 0;
  
  /// Peak memory usage in bytes
  uint64_t PeakMemoryBytes = 0;
  
  /// Number of operations performed
  uint32_t OperationCount = 0;
  
  /// Average time per operation in microseconds
  double AverageTimePerOperation() const {
    return OperationCount > 0 ? static_cast<double>(ExecutionTimeUs) / OperationCount : 0.0;
  }
  
  /// Memory efficiency (operations per MB)
  double MemoryEfficiency() const {
    return PeakMemoryBytes > 0 ? static_cast<double>(OperationCount) / (PeakMemoryBytes / 1024.0 / 1024.0) : 0.0;
  }
};

/// Performance profile for a specific pass phase
struct PhaseProfile {
  std::string PhaseName;
  PerformanceMetrics Metrics;
  
  /// Sub-phase profiles for detailed analysis
  DenseMap<StringRef, PerformanceMetrics> SubPhases;
  
  /// Hot spots identified in this phase
  std::vector<std::string> HotSpots;
  
  /// Optimization opportunities
  std::vector<std::string> OptimizationOpportunities;
};

/// Comprehensive performance profile for the entire pass
struct PassPerformanceProfile {
  /// Overall pass metrics
  PerformanceMetrics OverallMetrics;
  
  /// Per-phase profiles
  std::vector<PhaseProfile> PhaseProfiles;
  
  /// Module characteristics
  uint32_t ModuleFunctionCount = 0;
  uint32_t ModuleInstructionCount = 0;
  uint32_t MPICallCount = 0;
  uint32_t HookInsertionCount = 0;
  
  /// Performance ratios and efficiency metrics
  double InstructionProcessingRate() const {
    return OverallMetrics.ExecutionTimeUs > 0 ? 
           static_cast<double>(ModuleInstructionCount) / OverallMetrics.ExecutionTimeUs * 1000000.0 : 0.0;
  }
  
  double MPICallProcessingRate() const {
    return OverallMetrics.ExecutionTimeUs > 0 ? 
           static_cast<double>(MPICallCount) / OverallMetrics.ExecutionTimeUs * 1000000.0 : 0.0;
  }
  
  double HookInsertionRate() const {
    return OverallMetrics.ExecutionTimeUs > 0 ? 
           static_cast<double>(HookInsertionCount) / OverallMetrics.ExecutionTimeUs * 1000000.0 : 0.0;
  }
};

/// High-precision timer for performance measurement
class HighPrecisionProfileTimer {
public:
  HighPrecisionProfileTimer();
  ~HighPrecisionProfileTimer() = default;
  
  /// Start timing
  void start();
  
  /// Stop timing and return elapsed microseconds
  uint64_t stop();
  
  /// Get elapsed time without stopping
  uint64_t getElapsed() const;
  
  /// Reset timer
  void reset();
  
  /// Check if timer is running
  bool isRunning() const { return IsRunning; }

private:
  std::chrono::high_resolution_clock::time_point StartTime;
  std::chrono::high_resolution_clock::time_point EndTime;
  bool IsRunning = false;
};

/// Memory usage monitor for tracking memory consumption
class MemoryUsageMonitor {
public:
  MemoryUsageMonitor();
  ~MemoryUsageMonitor() = default;
  
  /// Start monitoring memory usage
  void startMonitoring();
  
  /// Stop monitoring and return peak usage
  uint64_t stopMonitoring();
  
  /// Get current memory usage
  uint64_t getCurrentUsage() const;
  
  /// Get peak memory usage since monitoring started
  uint64_t getPeakUsage() const { return PeakUsage; }

private:
  uint64_t BaselineUsage = 0;
  uint64_t PeakUsage = 0;
  bool IsMonitoring = false;
};

/// Performance profiler for the MPI Sanitizer Pass
class PerformanceProfiler {
public:
  PerformanceProfiler();
  ~PerformanceProfiler() = default;
  
  /// Start profiling a pass execution
  void startPassProfiling(Module& M);
  
  /// End pass profiling and generate profile
  PassPerformanceProfile endPassProfiling();
  
  /// Start profiling a specific phase
  void startPhase(StringRef PhaseName);
  
  /// End current phase profiling
  void endPhase();
  
  /// Start profiling a sub-phase within the current phase
  void startSubPhase(StringRef SubPhaseName);
  
  /// End current sub-phase profiling
  void endSubPhase();
  
  /// Record an operation for performance tracking
  void recordOperation(StringRef OperationType, uint32_t Count = 1);
  
  /// Identify and record a hot spot
  void recordHotSpot(StringRef Description, uint64_t ExecutionTimeUs);
  
  /// Record an optimization opportunity
  void recordOptimizationOpportunity(StringRef Description, double PotentialSavings);
  
  /// Generate detailed performance report
  std::string generateDetailedReport(const PassPerformanceProfile& Profile) const;
  
  /// Generate optimization recommendations
  std::vector<std::string> generateOptimizationRecommendations(const PassPerformanceProfile& Profile) const;
  
  /// Enable/disable detailed profiling
  void setDetailedProfiling(bool Enable) { DetailedProfiling = Enable; }
  
  /// Enable/disable memory profiling
  void setMemoryProfiling(bool Enable) { MemoryProfiling = Enable; }

private:
  /// Current profiling state
  struct ProfilingState {
    std::unique_ptr<HighPrecisionProfileTimer> PassTimer;
    std::unique_ptr<HighPrecisionProfileTimer> PhaseTimer;
    std::unique_ptr<HighPrecisionProfileTimer> SubPhaseTimer;
    std::unique_ptr<MemoryUsageMonitor> MemoryMonitor;
    
    std::string CurrentPhase;
    std::string CurrentSubPhase;
    
    PassPerformanceProfile Profile;
    PhaseProfile* CurrentPhaseProfile = nullptr;
  };
  
  std::unique_ptr<ProfilingState> State;
  
  /// Configuration
  bool DetailedProfiling = true;
  bool MemoryProfiling = true;
  
  /// Helper methods
  void initializeModuleCharacteristics(Module& M);
  void analyzeHotSpots(PhaseProfile& Phase) const;
  void identifyOptimizationOpportunities(PhaseProfile& Phase) const;
  std::string formatMetrics(const PerformanceMetrics& Metrics) const;
  std::string formatPhaseReport(const PhaseProfile& Phase) const;
};

/// Scoped profiler for automatic phase/sub-phase timing
class ScopedProfiler {
public:
  /// Constructor for phase profiling
  ScopedProfiler(PerformanceProfiler& Profiler, StringRef PhaseName);
  
  /// Constructor for sub-phase profiling
  ScopedProfiler(PerformanceProfiler& Profiler, StringRef PhaseName, StringRef SubPhaseName);
  
  /// Destructor automatically ends profiling
  ~ScopedProfiler();
  
  /// Record an operation within this scope
  void recordOperation(StringRef OperationType, uint32_t Count = 1);
  
  /// Record a hot spot within this scope
  void recordHotSpot(StringRef Description, uint64_t ExecutionTimeUs);

private:
  PerformanceProfiler& Profiler;
  bool IsSubPhase;
};

/// Macro for convenient scoped profiling
#define PROFILE_PHASE(profiler, phase_name) \
  ScopedProfiler _scoped_profiler(profiler, phase_name)

#define PROFILE_SUBPHASE(profiler, phase_name, subphase_name) \
  ScopedProfiler _scoped_profiler(profiler, phase_name, subphase_name)

/// Performance optimization analyzer
class PerformanceOptimizationAnalyzer {
public:
  PerformanceOptimizationAnalyzer() = default;
  ~PerformanceOptimizationAnalyzer() = default;
  
  /// Analyze performance profile and identify bottlenecks
  struct BottleneckAnalysis {
    std::vector<std::string> CriticalBottlenecks;
    std::vector<std::string> MinorBottlenecks;
    std::vector<std::string> OptimizationRecommendations;
    double EstimatedSpeedupPotential = 0.0;
  };
  
  BottleneckAnalysis analyzeBottlenecks(const PassPerformanceProfile& Profile) const;
  
  /// Analyze memory usage patterns
  struct MemoryAnalysis {
    bool HasMemoryLeaks = false;
    bool HasExcessiveMemoryUsage = false;
    std::vector<std::string> MemoryOptimizationRecommendations;
    double EstimatedMemorySavings = 0.0;
  };
  
  MemoryAnalysis analyzeMemoryUsage(const PassPerformanceProfile& Profile) const;
  
  /// Analyze scalability characteristics
  struct ScalabilityAnalysis {
    enum class ScalabilityClass {
      Linear,
      Logarithmic,
      Quadratic,
      Exponential,
      Unknown
    };
    
    ScalabilityClass TimeComplexity = ScalabilityClass::Unknown;
    ScalabilityClass SpaceComplexity = ScalabilityClass::Unknown;
    std::vector<std::string> ScalabilityRecommendations;
  };
  
  ScalabilityAnalysis analyzeScalability(const std::vector<PassPerformanceProfile>& Profiles) const;
  
  /// Generate comprehensive optimization plan
  struct OptimizationPlan {
    std::vector<std::string> ImmediateOptimizations;
    std::vector<std::string> MediumTermOptimizations;
    std::vector<std::string> LongTermOptimizations;
    double EstimatedOverallSpeedup = 0.0;
  };
  
  OptimizationPlan generateOptimizationPlan(const PassPerformanceProfile& Profile) const;

private:
  /// Threshold constants for analysis
  static constexpr double CRITICAL_BOTTLENECK_THRESHOLD = 0.3; // 30% of total time
  static constexpr double MINOR_BOTTLENECK_THRESHOLD = 0.1;    // 10% of total time
  static constexpr double EXCESSIVE_MEMORY_THRESHOLD = 100.0;  // 100MB
  static constexpr double MEMORY_LEAK_THRESHOLD = 0.05;       // 5% growth
};

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_PERFORMANCEPROFILER_H
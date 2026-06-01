//===- PerformanceProfiler.cpp - MPI Sanitizer Performance Profiler ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the PerformanceProfiler class which provides detailed
// profiling capabilities for the MPI Usage Sanitizer Pass to identify
// performance bottlenecks and optimization opportunities.
//
//===----------------------------------------------------------------------===//

#include "PerformanceProfiler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace llvm;

#define DEBUG_TYPE "mpi-performance-profiler"

//===----------------------------------------------------------------------===//
// HighPrecisionProfileTimer Implementation
//===----------------------------------------------------------------------===//

HighPrecisionProfileTimer::HighPrecisionProfileTimer() {
  reset();
}

void HighPrecisionProfileTimer::start() {
  StartTime = std::chrono::high_resolution_clock::now();
  IsRunning = true;
}

uint64_t HighPrecisionProfileTimer::stop() {
  if (!IsRunning) return 0;
  
  EndTime = std::chrono::high_resolution_clock::now();
  IsRunning = false;
  
  auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(EndTime - StartTime);
  return Duration.count();
}

uint64_t HighPrecisionProfileTimer::getElapsed() const {
  if (!IsRunning) return 0;
  
  auto CurrentTime = std::chrono::high_resolution_clock::now();
  auto Duration = std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - StartTime);
  return Duration.count();
}

void HighPrecisionProfileTimer::reset() {
  StartTime = std::chrono::high_resolution_clock::time_point();
  EndTime = std::chrono::high_resolution_clock::time_point();
  IsRunning = false;
}

//===----------------------------------------------------------------------===//
// MemoryUsageMonitor Implementation
//===----------------------------------------------------------------------===//

MemoryUsageMonitor::MemoryUsageMonitor() {
  BaselineUsage = getCurrentUsage();
  PeakUsage = BaselineUsage;
}

void MemoryUsageMonitor::startMonitoring() {
  BaselineUsage = getCurrentUsage();
  PeakUsage = BaselineUsage;
  IsMonitoring = true;
}

uint64_t MemoryUsageMonitor::stopMonitoring() {
  IsMonitoring = false;
  uint64_t CurrentUsage = getCurrentUsage();
  if (CurrentUsage > PeakUsage) {
    PeakUsage = CurrentUsage;
  }
  return PeakUsage - BaselineUsage;
}

uint64_t MemoryUsageMonitor::getCurrentUsage() const {
  return sys::Process::GetMallocUsage();
}

//===----------------------------------------------------------------------===//
// PerformanceProfiler Implementation
//===----------------------------------------------------------------------===//

PerformanceProfiler::PerformanceProfiler() {
  State = std::make_unique<ProfilingState>();
}

void PerformanceProfiler::startPassProfiling(Module& M) {
  // Initialize profiling state
  State->PassTimer = std::make_unique<HighPrecisionProfileTimer>();
  State->MemoryMonitor = std::make_unique<MemoryUsageMonitor>();
  
  // Start timing and memory monitoring
  State->PassTimer->start();
  if (MemoryProfiling) {
    State->MemoryMonitor->startMonitoring();
  }
  
  // Initialize module characteristics
  initializeModuleCharacteristics(M);
  
  LLVM_DEBUG(dbgs() << "Started performance profiling for module: " << M.getName() << "\n");
}

PassPerformanceProfile PerformanceProfiler::endPassProfiling() {
  // Stop overall timing
  if (State->PassTimer) {
    State->Profile.OverallMetrics.ExecutionTimeUs = State->PassTimer->stop();
  }
  
  // Stop memory monitoring
  if (MemoryProfiling && State->MemoryMonitor) {
    State->Profile.OverallMetrics.PeakMemoryBytes = State->MemoryMonitor->stopMonitoring();
  }
  
  // Analyze hot spots and optimization opportunities for all phases
  for (auto& Phase : State->Profile.PhaseProfiles) {
    analyzeHotSpots(Phase);
    identifyOptimizationOpportunities(Phase);
  }
  
  LLVM_DEBUG(dbgs() << "Completed performance profiling - Total time: " 
                    << State->Profile.OverallMetrics.ExecutionTimeUs << " μs\n");
  
  return std::move(State->Profile);
}

void PerformanceProfiler::startPhase(StringRef PhaseName) {
  // End previous phase if active
  if (State->CurrentPhaseProfile) {
    endPhase();
  }
  
  State->CurrentPhase = PhaseName.str();
  State->PhaseTimer = std::make_unique<HighPrecisionProfileTimer>();
  State->PhaseTimer->start();
  
  // Create new phase profile
  PhaseProfile NewPhase;
  NewPhase.PhaseName = State->CurrentPhase;
  State->Profile.PhaseProfiles.push_back(std::move(NewPhase));
  State->CurrentPhaseProfile = &State->Profile.PhaseProfiles.back();
  
  LLVM_DEBUG(dbgs() << "Started profiling phase: " << PhaseName << "\n");
}

void PerformanceProfiler::endPhase() {
  if (!State->CurrentPhaseProfile || !State->PhaseTimer) {
    return;
  }
  
  // End sub-phase if active
  if (!State->CurrentSubPhase.empty()) {
    endSubPhase();
  }
  
  // Record phase timing
  State->CurrentPhaseProfile->Metrics.ExecutionTimeUs = State->PhaseTimer->stop();
  
  // Record memory usage if monitoring
  if (MemoryProfiling && State->MemoryMonitor) {
    State->CurrentPhaseProfile->Metrics.PeakMemoryBytes = State->MemoryMonitor->getCurrentUsage();
  }
  
  LLVM_DEBUG(dbgs() << "Completed profiling phase: " << State->CurrentPhase 
                    << " - Time: " << State->CurrentPhaseProfile->Metrics.ExecutionTimeUs << " μs\n");
  
  State->CurrentPhase.clear();
  State->CurrentPhaseProfile = nullptr;
  State->PhaseTimer.reset();
}

void PerformanceProfiler::startSubPhase(StringRef SubPhaseName) {
  if (!State->CurrentPhaseProfile) {
    LLVM_DEBUG(dbgs() << "Warning: Starting sub-phase without active phase\n");
    return;
  }
  
  // End previous sub-phase if active
  if (!State->CurrentSubPhase.empty()) {
    endSubPhase();
  }
  
  State->CurrentSubPhase = SubPhaseName.str();
  State->SubPhaseTimer = std::make_unique<HighPrecisionProfileTimer>();
  State->SubPhaseTimer->start();
  
  LLVM_DEBUG(dbgs() << "Started profiling sub-phase: " << SubPhaseName << "\n");
}

void PerformanceProfiler::endSubPhase() {
  if (State->CurrentSubPhase.empty() || !State->SubPhaseTimer || !State->CurrentPhaseProfile) {
    return;
  }
  
  // Record sub-phase timing
  PerformanceMetrics SubPhaseMetrics;
  SubPhaseMetrics.ExecutionTimeUs = State->SubPhaseTimer->stop();
  
  if (MemoryProfiling && State->MemoryMonitor) {
    SubPhaseMetrics.PeakMemoryBytes = State->MemoryMonitor->getCurrentUsage();
  }
  
  State->CurrentPhaseProfile->SubPhases[StringRef(State->CurrentSubPhase)] = SubPhaseMetrics;
  
  LLVM_DEBUG(dbgs() << "Completed profiling sub-phase: " << State->CurrentSubPhase 
                    << " - Time: " << SubPhaseMetrics.ExecutionTimeUs << " μs\n");
  
  State->CurrentSubPhase.clear();
  State->SubPhaseTimer.reset();
}

void PerformanceProfiler::recordOperation(StringRef OperationType, uint32_t Count) {
  if (State->CurrentPhaseProfile) {
    State->CurrentPhaseProfile->Metrics.OperationCount += Count;
  }
  
  State->Profile.OverallMetrics.OperationCount += Count;
  
  LLVM_DEBUG(dbgs() << "Recorded " << Count << " operations of type: " << OperationType << "\n");
}

void PerformanceProfiler::recordHotSpot(StringRef Description, uint64_t ExecutionTimeUs) {
  if (!State->CurrentPhaseProfile) return;
  
  std::string HotSpotDesc = Description.str() + " (" + std::to_string(ExecutionTimeUs) + " μs)";
  State->CurrentPhaseProfile->HotSpots.push_back(HotSpotDesc);
  
  LLVM_DEBUG(dbgs() << "Recorded hot spot: " << Description << " - " << ExecutionTimeUs << " μs\n");
}

void PerformanceProfiler::recordOptimizationOpportunity(StringRef Description, double PotentialSavings) {
  if (!State->CurrentPhaseProfile) return;
  
  std::string OpportunityDesc = Description.str() + " (potential savings: " + 
                               std::to_string(PotentialSavings * 100.0) + "%)";
  State->CurrentPhaseProfile->OptimizationOpportunities.push_back(OpportunityDesc);
  
  LLVM_DEBUG(dbgs() << "Recorded optimization opportunity: " << Description 
                    << " - Potential savings: " << (PotentialSavings * 100.0) << "%\n");
}

std::string PerformanceProfiler::generateDetailedReport(const PassPerformanceProfile& Profile) const {
  std::ostringstream Report;
  
  Report << "=== MPI Sanitizer Pass Performance Profile ===\n\n";
  
  // Overall metrics
  Report << "Overall Performance:\n";
  Report << formatMetrics(Profile.OverallMetrics) << "\n";
  
  // Module characteristics
  Report << "Module Characteristics:\n";
  Report << "  Functions: " << Profile.ModuleFunctionCount << "\n";
  Report << "  Instructions: " << Profile.ModuleInstructionCount << "\n";
  Report << "  MPI Calls: " << Profile.MPICallCount << "\n";
  Report << "  Hooks Inserted: " << Profile.HookInsertionCount << "\n\n";
  
  // Performance rates
  Report << "Performance Rates:\n";
  Report << "  Instructions/sec: " << std::fixed << std::setprecision(2) 
         << Profile.InstructionProcessingRate() << "\n";
  Report << "  MPI Calls/sec: " << std::fixed << std::setprecision(2) 
         << Profile.MPICallProcessingRate() << "\n";
  Report << "  Hook Insertions/sec: " << std::fixed << std::setprecision(2) 
         << Profile.HookInsertionRate() << "\n\n";
  
  // Phase-by-phase analysis
  Report << "Phase Analysis:\n";
  for (const auto& Phase : Profile.PhaseProfiles) {
    Report << formatPhaseReport(Phase) << "\n";
  }
  
  return Report.str();
}

std::vector<std::string> PerformanceProfiler::generateOptimizationRecommendations(const PassPerformanceProfile& Profile) const {
  std::vector<std::string> Recommendations;
  
  // Analyze overall performance
  if (Profile.OverallMetrics.ExecutionTimeUs > 100000) { // > 100ms
    Recommendations.push_back("Consider enabling optimization flags to reduce overall execution time");
  }
  
  if (Profile.OverallMetrics.PeakMemoryBytes > 100 * 1024 * 1024) { // > 100MB
    Recommendations.push_back("High memory usage detected - consider memory optimization strategies");
  }
  
  // Analyze processing rates
  if (Profile.InstructionProcessingRate() < 10000) { // < 10K instructions/sec
    Recommendations.push_back("Low instruction processing rate - optimize IR traversal algorithms");
  }
  
  if (Profile.MPICallProcessingRate() < 1000) { // < 1K MPI calls/sec
    Recommendations.push_back("Low MPI call processing rate - optimize call detection algorithms");
  }
  
  // Analyze phase-specific issues
  for (const auto& Phase : Profile.PhaseProfiles) {
    double PhasePercentage = static_cast<double>(Phase.Metrics.ExecutionTimeUs) / 
                            Profile.OverallMetrics.ExecutionTimeUs;
    
    if (PhasePercentage > 0.4) { // Phase takes > 40% of total time
      Recommendations.push_back("Phase '" + Phase.PhaseName + "' is a major bottleneck (" + 
                               std::to_string(PhasePercentage * 100.0) + "% of total time)");
    }
    
    // Add phase-specific optimization opportunities
    for (const auto& Opportunity : Phase.OptimizationOpportunities) {
      Recommendations.push_back("In " + Phase.PhaseName + ": " + Opportunity);
    }
  }
  
  return Recommendations;
}

void PerformanceProfiler::initializeModuleCharacteristics(Module& M) {
  State->Profile.ModuleFunctionCount = 0;
  State->Profile.ModuleInstructionCount = 0;
  State->Profile.MPICallCount = 0;
  
  for (Function& F : M) {
    if (!F.isDeclaration()) {
      State->Profile.ModuleFunctionCount++;
      
      for (BasicBlock& BB : F) {
        State->Profile.ModuleInstructionCount += BB.size();
        
        // Count MPI calls
        for (Instruction& I : BB) {
          if (CallInst* Call = dyn_cast<CallInst>(&I)) {
            if (Function* Callee = Call->getCalledFunction()) {
              StringRef Name = Callee->getName();
              if (Name.startswith("MPI_") || Name.startswith("mpi_") || Name.contains("MPI")) {
                State->Profile.MPICallCount++;
              }
            }
          }
        }
      }
    }
  }
  
  LLVM_DEBUG(dbgs() << "Module characteristics - Functions: " << State->Profile.ModuleFunctionCount
                    << ", Instructions: " << State->Profile.ModuleInstructionCount
                    << ", MPI Calls: " << State->Profile.MPICallCount << "\n");
}

void PerformanceProfiler::analyzeHotSpots(PhaseProfile& Phase) const {
  // Analyze sub-phases to identify hot spots
  for (const auto& SubPhase : Phase.SubPhases) {
    double SubPhasePercentage = static_cast<double>(SubPhase.second.ExecutionTimeUs) / 
                               Phase.Metrics.ExecutionTimeUs;
    
    if (SubPhasePercentage > 0.3) { // Sub-phase takes > 30% of phase time
      std::string HotSpot = "Sub-phase '" + SubPhase.first.str() + "' consumes " + 
                           std::to_string(SubPhasePercentage * 100.0) + "% of phase time";
      const_cast<PhaseProfile&>(Phase).HotSpots.push_back(HotSpot);
    }
  }
  
  // Analyze operation efficiency
  if (Phase.Metrics.OperationCount > 0) {
    double AvgTimePerOp = Phase.Metrics.AverageTimePerOperation();
    if (AvgTimePerOp > 100) { // > 100μs per operation
      std::string HotSpot = "High average time per operation: " + std::to_string(AvgTimePerOp) + " μs";
      const_cast<PhaseProfile&>(Phase).HotSpots.push_back(HotSpot);
    }
  }
}

void PerformanceProfiler::identifyOptimizationOpportunities(PhaseProfile& Phase) const {
  // Memory efficiency analysis
  if (Phase.Metrics.PeakMemoryBytes > 0) {
    double MemoryEfficiency = Phase.Metrics.MemoryEfficiency();
    if (MemoryEfficiency < 100) { // < 100 operations per MB
      std::string Opportunity = "Low memory efficiency (" + std::to_string(MemoryEfficiency) + 
                               " ops/MB) - consider memory optimization";
      const_cast<PhaseProfile&>(Phase).OptimizationOpportunities.push_back(Opportunity);
    }
  }
  
  // Time distribution analysis
  if (Phase.SubPhases.size() > 1) {
    // Find the most time-consuming sub-phase
    auto MaxSubPhase = std::max_element(Phase.SubPhases.begin(), Phase.SubPhases.end(),
      [](const auto& A, const auto& B) {
        return A.second.ExecutionTimeUs < B.second.ExecutionTimeUs;
      });
    
    if (MaxSubPhase != Phase.SubPhases.end()) {
      double MaxPercentage = static_cast<double>(MaxSubPhase->second.ExecutionTimeUs) / 
                            Phase.Metrics.ExecutionTimeUs;
      
      if (MaxPercentage > 0.6) { // > 60% of phase time
        std::string Opportunity = "Sub-phase '" + MaxSubPhase->first.str() + 
                                 "' dominates phase time - focus optimization here";
        const_cast<PhaseProfile&>(Phase).OptimizationOpportunities.push_back(Opportunity);
      }
    }
  }
}

std::string PerformanceProfiler::formatMetrics(const PerformanceMetrics& Metrics) const {
  std::ostringstream Formatted;
  
  Formatted << "  Execution Time: " << Metrics.ExecutionTimeUs << " μs\n";
  Formatted << "  Peak Memory: " << (Metrics.PeakMemoryBytes / 1024.0 / 1024.0) << " MB\n";
  Formatted << "  Operations: " << Metrics.OperationCount << "\n";
  
  if (Metrics.OperationCount > 0) {
    Formatted << "  Avg Time/Op: " << std::fixed << std::setprecision(2) 
              << Metrics.AverageTimePerOperation() << " μs\n";
    Formatted << "  Memory Efficiency: " << std::fixed << std::setprecision(2) 
              << Metrics.MemoryEfficiency() << " ops/MB\n";
  }
  
  return Formatted.str();
}

std::string PerformanceProfiler::formatPhaseReport(const PhaseProfile& Phase) const {
  std::ostringstream Report;
  
  Report << "Phase: " << Phase.PhaseName << "\n";
  Report << formatMetrics(Phase.Metrics);
  
  // Sub-phases
  if (!Phase.SubPhases.empty()) {
    Report << "  Sub-phases:\n";
    for (const auto& SubPhase : Phase.SubPhases) {
      double Percentage = static_cast<double>(SubPhase.second.ExecutionTimeUs) / 
                         Phase.Metrics.ExecutionTimeUs * 100.0;
      Report << "    " << SubPhase.first.str() << ": " << SubPhase.second.ExecutionTimeUs 
             << " μs (" << std::fixed << std::setprecision(1) << Percentage << "%)\n";
    }
  }
  
  // Hot spots
  if (!Phase.HotSpots.empty()) {
    Report << "  Hot Spots:\n";
    for (const auto& HotSpot : Phase.HotSpots) {
      Report << "    - " << HotSpot << "\n";
    }
  }
  
  // Optimization opportunities
  if (!Phase.OptimizationOpportunities.empty()) {
    Report << "  Optimization Opportunities:\n";
    for (const auto& Opportunity : Phase.OptimizationOpportunities) {
      Report << "    - " << Opportunity << "\n";
    }
  }
  
  return Report.str();
}

//===----------------------------------------------------------------------===//
// ScopedProfiler Implementation
//===----------------------------------------------------------------------===//

ScopedProfiler::ScopedProfiler(PerformanceProfiler& Profiler, StringRef PhaseName)
    : Profiler(Profiler), IsSubPhase(false) {
  Profiler.startPhase(PhaseName);
}

ScopedProfiler::ScopedProfiler(PerformanceProfiler& Profiler, StringRef PhaseName, StringRef SubPhaseName)
    : Profiler(Profiler), IsSubPhase(true) {
  Profiler.startPhase(PhaseName);
  Profiler.startSubPhase(SubPhaseName);
}

ScopedProfiler::~ScopedProfiler() {
  if (IsSubPhase) {
    Profiler.endSubPhase();
  }
  Profiler.endPhase();
}

void ScopedProfiler::recordOperation(StringRef OperationType, uint32_t Count) {
  Profiler.recordOperation(OperationType, Count);
}

void ScopedProfiler::recordHotSpot(StringRef Description, uint64_t ExecutionTimeUs) {
  Profiler.recordHotSpot(Description, ExecutionTimeUs);
}

//===----------------------------------------------------------------------===//
// PerformanceOptimizationAnalyzer Implementation
//===----------------------------------------------------------------------===//

PerformanceOptimizationAnalyzer::BottleneckAnalysis 
PerformanceOptimizationAnalyzer::analyzeBottlenecks(const PassPerformanceProfile& Profile) const {
  BottleneckAnalysis Analysis;
  
  // Analyze phase bottlenecks
  for (const auto& Phase : Profile.PhaseProfiles) {
    double PhasePercentage = static_cast<double>(Phase.Metrics.ExecutionTimeUs) / 
                            Profile.OverallMetrics.ExecutionTimeUs;
    
    if (PhasePercentage >= CRITICAL_BOTTLENECK_THRESHOLD) {
      Analysis.CriticalBottlenecks.push_back(
        "Phase '" + Phase.PhaseName + "' consumes " + 
        std::to_string(PhasePercentage * 100.0) + "% of total execution time"
      );
    } else if (PhasePercentage >= MINOR_BOTTLENECK_THRESHOLD) {
      Analysis.MinorBottlenecks.push_back(
        "Phase '" + Phase.PhaseName + "' consumes " + 
        std::to_string(PhasePercentage * 100.0) + "% of total execution time"
      );
    }
  }
  
  // Generate optimization recommendations based on bottlenecks
  if (!Analysis.CriticalBottlenecks.empty()) {
    Analysis.OptimizationRecommendations.push_back(
      "Focus optimization efforts on critical bottleneck phases"
    );
    Analysis.EstimatedSpeedupPotential = 0.3; // 30% potential speedup
  }
  
  if (!Analysis.MinorBottlenecks.empty()) {
    Analysis.OptimizationRecommendations.push_back(
      "Consider optimizing minor bottleneck phases for additional performance gains"
    );
    Analysis.EstimatedSpeedupPotential += 0.1; // Additional 10% potential speedup
  }
  
  return Analysis;
}

PerformanceOptimizationAnalyzer::MemoryAnalysis 
PerformanceOptimizationAnalyzer::analyzeMemoryUsage(const PassPerformanceProfile& Profile) const {
  MemoryAnalysis Analysis;
  
  // Check for excessive memory usage
  double MemoryUsageMB = Profile.OverallMetrics.PeakMemoryBytes / 1024.0 / 1024.0;
  if (MemoryUsageMB > EXCESSIVE_MEMORY_THRESHOLD) {
    Analysis.HasExcessiveMemoryUsage = true;
    Analysis.MemoryOptimizationRecommendations.push_back(
      "High memory usage detected (" + std::to_string(MemoryUsageMB) + " MB) - implement memory optimization strategies"
    );
    Analysis.EstimatedMemorySavings = 0.2; // 20% potential memory savings
  }
  
  // Analyze memory efficiency across phases
  for (const auto& Phase : Profile.PhaseProfiles) {
    if (Phase.Metrics.PeakMemoryBytes > 0) {
      double Efficiency = Phase.Metrics.MemoryEfficiency();
      if (Efficiency < 50) { // < 50 operations per MB
        Analysis.MemoryOptimizationRecommendations.push_back(
          "Phase '" + Phase.PhaseName + "' has low memory efficiency (" + 
          std::to_string(Efficiency) + " ops/MB)"
        );
      }
    }
  }
  
  return Analysis;
}

PerformanceOptimizationAnalyzer::OptimizationPlan 
PerformanceOptimizationAnalyzer::generateOptimizationPlan(const PassPerformanceProfile& Profile) const {
  OptimizationPlan Plan;
  
  auto BottleneckAnalysis = analyzeBottlenecks(Profile);
  auto MemoryAnalysis = analyzeMemoryUsage(Profile);
  
  // Immediate optimizations (can be implemented quickly)
  if (!BottleneckAnalysis.CriticalBottlenecks.empty()) {
    Plan.ImmediateOptimizations.push_back("Optimize critical bottleneck phases");
    Plan.ImmediateOptimizations.push_back("Enable compiler optimizations (-O2/-O3)");
    Plan.ImmediateOptimizations.push_back("Use more efficient data structures in hot paths");
  }
  
  if (MemoryAnalysis.HasExcessiveMemoryUsage) {
    Plan.ImmediateOptimizations.push_back("Implement memory pooling for frequent allocations");
    Plan.ImmediateOptimizations.push_back("Reduce temporary object creation");
  }
  
  // Medium-term optimizations (require more significant changes)
  Plan.MediumTermOptimizations.push_back("Implement caching for expensive computations");
  Plan.MediumTermOptimizations.push_back("Parallelize independent operations");
  Plan.MediumTermOptimizations.push_back("Optimize IR traversal algorithms");
  
  // Long-term optimizations (architectural changes)
  Plan.LongTermOptimizations.push_back("Redesign algorithms for better time complexity");
  Plan.LongTermOptimizations.push_back("Implement lazy evaluation strategies");
  Plan.LongTermOptimizations.push_back("Consider alternative data structures and algorithms");
  
  // Estimate overall speedup potential
  Plan.EstimatedOverallSpeedup = BottleneckAnalysis.EstimatedSpeedupPotential;
  if (MemoryAnalysis.EstimatedMemorySavings > 0) {
    Plan.EstimatedOverallSpeedup += 0.1; // Memory optimizations can provide additional speedup
  }
  
  return Plan;
}
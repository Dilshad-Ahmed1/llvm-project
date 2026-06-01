//===- PerformanceTests.cpp - MPI Sanitizer Performance Tests --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements performance tests for the MPI Usage Sanitizer LLVM Pass,
// including compilation overhead measurement, runtime overhead analysis, and
// scalability validation.
//
//===----------------------------------------------------------------------===//

#include "PerformanceTests.h"
#include "MPISanitizerPass.h"
#include "MPICallDetector.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <thread>

namespace llvm {

//===----------------------------------------------------------------------===//
// CompilationOverheadTest Implementation
//===----------------------------------------------------------------------===//

void CompilationOverheadTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
  
  // Set up test configurations
  BenchmarkConfiguration SmallConfig;
  SmallConfig.FunctionCount = 10;
  SmallConfig.MPICallsPerFunction = 2;
  SmallConfig.MaxCompilationOverhead = 30.0;
  TestConfigurations.push_back(SmallConfig);
  
  BenchmarkConfiguration MediumConfig;
  MediumConfig.FunctionCount = 100;
  MediumConfig.MPICallsPerFunction = 5;
  MediumConfig.MaxCompilationOverhead = 50.0;
  TestConfigurations.push_back(MediumConfig);
  
  BenchmarkConfiguration LargeConfig;
  LargeConfig.FunctionCount = 1000;
  LargeConfig.MPICallsPerFunction = 10;
  LargeConfig.MaxCompilationOverhead = 100.0;
  TestConfigurations.push_back(LargeConfig);
}

void CompilationOverheadTest::TearDown() {
  TestConfigurations.clear();
  ConfigManager.reset();
  Context.reset();
}

PerformanceMeasurement CompilationOverheadTest::measureCompilationOverhead(Module& M, const PassConfiguration& Config) {
  PerformanceMeasurement Measurement;
  
  // Measure original module characteristics
  Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(M);
  Measurement.OriginalModuleSize = PerformanceTestUtils::measureModuleSize(M);
  Measurement.MPICallsDetected = PerformanceTestUtils::countMPICalls(M);
  
  // Clone module for instrumentation
  auto InstrumentedModule = CloneModule(M);
  
  // Measure baseline compilation time
  uint64_t BaselineTime = measureBaselineCompilation(M);
  
  // Measure instrumented compilation time
  uint64_t InstrumentedTime = measureInstrumentedCompilation(*InstrumentedModule, Config);
  
  // Calculate compilation overhead
  Measurement.CompilationTimeUs = InstrumentedTime - BaselineTime;
  
  // Measure instrumented module characteristics
  Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(*InstrumentedModule);
  Measurement.InstrumentedModuleSize = PerformanceTestUtils::measureModuleSize(*InstrumentedModule);
  Measurement.HooksInserted = PerformanceTestUtils::countInstrumentationHooks(*InstrumentedModule);
  
  // Measure memory usage
  Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage();
  
  return Measurement;
}

void CompilationOverheadTest::testSmallModuleOverhead() {
  const auto& Config = TestConfigurations[0]; // Small config
  
  auto TestModule = generateTestModule(Config.FunctionCount, Config.MPICallsPerFunction);
  ASSERT_TRUE(TestModule) << "Failed to generate small test module";
  
  PerformanceMeasurement Measurement = measureCompilationOverhead(*TestModule, Config.InstrumentationConfig);
  
  // Validate overhead is within acceptable limits
  EXPECT_LT(Measurement.getInstructionOverhead(), Config.MaxInstructionOverhead) 
    << "Instruction overhead too high for small module: " << Measurement.getInstructionOverhead() << "%";
  
  EXPECT_LT(Measurement.getSizeOverhead(), Config.MaxSizeOverhead)
    << "Size overhead too high for small module: " << Measurement.getSizeOverhead() << "%";
  
  EXPECT_GT(Measurement.MPICallsDetected, 0u) << "No MPI calls detected in small module";
  EXPECT_GT(Measurement.HooksInserted, 0u) << "No hooks inserted in small module";
}

void CompilationOverheadTest::testMediumModuleOverhead() {
  const auto& Config = TestConfigurations[1]; // Medium config
  
  auto TestModule = generateTestModule(Config.FunctionCount, Config.MPICallsPerFunction);
  ASSERT_TRUE(TestModule) << "Failed to generate medium test module";
  
  PerformanceMeasurement Measurement = measureCompilationOverhead(*TestModule, Config.InstrumentationConfig);
  
  EXPECT_LT(Measurement.getInstructionOverhead(), Config.MaxInstructionOverhead)
    << "Instruction overhead too high for medium module: " << Measurement.getInstructionOverhead() << "%";
  
  EXPECT_LT(Measurement.getSizeOverhead(), Config.MaxSizeOverhead)
    << "Size overhead too high for medium module: " << Measurement.getSizeOverhead() << "%";
  
  // Medium modules should have more MPI calls
  EXPECT_GT(Measurement.MPICallsDetected, TestConfigurations[0].FunctionCount * TestConfigurations[0].MPICallsPerFunction)
    << "Expected more MPI calls in medium module";
}

void CompilationOverheadTest::testLargeModuleOverhead() {
  const auto& Config = TestConfigurations[2]; // Large config
  
  auto TestModule = generateTestModule(Config.FunctionCount, Config.MPICallsPerFunction);
  ASSERT_TRUE(TestModule) << "Failed to generate large test module";
  
  PerformanceMeasurement Measurement = measureCompilationOverhead(*TestModule, Config.InstrumentationConfig);
  
  EXPECT_LT(Measurement.getInstructionOverhead(), Config.MaxInstructionOverhead)
    << "Instruction overhead too high for large module: " << Measurement.getInstructionOverhead() << "%";
  
  EXPECT_LT(Measurement.getSizeOverhead(), Config.MaxSizeOverhead)
    << "Size overhead too high for large module: " << Measurement.getSizeOverhead() << "%";
  
  // Large modules should have significantly more MPI calls
  EXPECT_GT(Measurement.MPICallsDetected, TestConfigurations[1].FunctionCount * TestConfigurations[1].MPICallsPerFunction)
    << "Expected many more MPI calls in large module";
}

void CompilationOverheadTest::testInstrumentationLevelOverhead() {
  auto TestModule = generateTestModule(100, 5);
  ASSERT_TRUE(TestModule) << "Failed to generate test module for instrumentation level test";
  
  // Test different instrumentation levels
  std::vector<InstrumentationMode> Levels = {
    InstrumentationMode::Lightweight,
    InstrumentationMode::Standard,
    InstrumentationMode::Full
  };
  
  std::vector<PerformanceMeasurement> Measurements;
  
  for (auto Level : Levels) {
    PassConfiguration Config;
    Config.InstrumentationMode = Level;
    
    auto ModuleCopy = CloneModule(*TestModule);
    PerformanceMeasurement Measurement = measureCompilationOverhead(*ModuleCopy, Config);
    Measurements.push_back(Measurement);
  }
  
  // Verify that overhead increases with instrumentation level
  EXPECT_LE(Measurements[0].getInstructionOverhead(), Measurements[1].getInstructionOverhead())
    << "Lightweight should have less overhead than Standard";
  
  EXPECT_LE(Measurements[1].getInstructionOverhead(), Measurements[2].getInstructionOverhead())
    << "Standard should have less overhead than Full";
}

void CompilationOverheadTest::testOptimizationOverhead() {
  auto TestModule = generateTestModule(100, 5);
  ASSERT_TRUE(TestModule) << "Failed to generate test module for optimization test";
  
  // Test with optimizations enabled vs disabled
  PassConfiguration OptimizedConfig;
  OptimizedConfig.EnableOptimizations = true;
  
  PassConfiguration UnoptimizedConfig;
  UnoptimizedConfig.EnableOptimizations = false;
  
  auto OptimizedModule = CloneModule(*TestModule);
  auto UnoptimizedModule = CloneModule(*TestModule);
  
  PerformanceMeasurement OptimizedMeasurement = measureCompilationOverhead(*OptimizedModule, OptimizedConfig);
  PerformanceMeasurement UnoptimizedMeasurement = measureCompilationOverhead(*UnoptimizedModule, UnoptimizedConfig);
  
  // Optimized version should have lower overhead
  EXPECT_LE(OptimizedMeasurement.getInstructionOverhead(), UnoptimizedMeasurement.getInstructionOverhead())
    << "Optimized version should have lower instruction overhead";
}

std::unique_ptr<Module> CompilationOverheadTest::generateTestModule(uint32_t FunctionCount, uint32_t MPICallsPerFunction) {
  return PerformanceTestUtils::createBenchmarkModule(*Context, FunctionCount, MPICallsPerFunction, false);
}

uint64_t CompilationOverheadTest::measureBaselineCompilation(Module& M) {
  PerformanceTestUtils::HighPrecisionTimer Timer;
  
  Timer.start();
  
  // Simulate compilation without MPI sanitizer
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  
  // Add some standard passes to simulate realistic compilation
  MPM.addPass(VerifierPass());
  
  MPM.run(M, MAM);
  
  Timer.stop();
  
  return Timer.getElapsedMicroseconds();
}

uint64_t CompilationOverheadTest::measureInstrumentedCompilation(Module& M, const PassConfiguration& Config) {
  PerformanceTestUtils::HighPrecisionTimer Timer;
  
  Timer.start();
  
  // Compile with MPI sanitizer
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  
  // Initialize configuration
  ConfigManager->initialize(Config);
  
  // Add MPI sanitizer pass
  MPM.addPass(MPISanitizerPass());
  MPM.addPass(VerifierPass());
  
  MPM.run(M, MAM);
  
  Timer.stop();
  
  return Timer.getElapsedMicroseconds();
}

//===----------------------------------------------------------------------===//
// MemoryUsageTest Implementation
//===----------------------------------------------------------------------===//

void MemoryUsageTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
  BaselineMemoryUsage = getCurrentMemoryUsage();
}

void MemoryUsageTest::TearDown() {
  ConfigManager.reset();
  Context.reset();
}

PerformanceMeasurement MemoryUsageTest::measureMemoryUsage(Module& M, const PassConfiguration& Config) {
  PerformanceMeasurement Measurement;
  
  uint64_t InitialMemory = getCurrentMemoryUsage();
  
  // Run pass and monitor memory usage
  std::vector<uint64_t> MemoryTrace = monitorMemoryUsage([&]() {
    ModulePassManager MPM;
    ModuleAnalysisManager MAM;
    
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    
    ConfigManager->initialize(Config);
    MPM.addPass(MPISanitizerPass());
    MPM.run(M, MAM);
  });
  
  uint64_t FinalMemory = getCurrentMemoryUsage();
  
  Measurement.MemoryUsageBytes = FinalMemory - InitialMemory;
  Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(M);
  Measurement.MPICallsDetected = PerformanceTestUtils::countMPICalls(M);
  
  return Measurement;
}

void MemoryUsageTest::testMemoryScaling() {
  std::vector<uint32_t> ModuleSizes = {10, 50, 100, 500, 1000};
  std::vector<PerformanceMeasurement> Measurements;
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  
  for (uint32_t Size : ModuleSizes) {
    auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, Size, 5);
    ASSERT_TRUE(TestModule) << "Failed to create test module of size " << Size;
    
    PerformanceMeasurement Measurement = measureMemoryUsage(*TestModule, Config);
    Measurements.push_back(Measurement);
  }
  
  // Verify that memory usage scales reasonably
  for (size_t i = 1; i < Measurements.size(); ++i) {
    double GrowthRatio = static_cast<double>(Measurements[i].MemoryUsageBytes) / Measurements[i-1].MemoryUsageBytes;
    double SizeRatio = static_cast<double>(ModuleSizes[i]) / ModuleSizes[i-1];
    
    // Memory growth should not be significantly worse than linear
    EXPECT_LT(GrowthRatio, SizeRatio * 2.0) 
      << "Memory usage growing too fast: " << GrowthRatio << "x vs size ratio " << SizeRatio << "x";
  }
}

void MemoryUsageTest::testConfigurationMemoryImpact() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 100, 5);
  ASSERT_TRUE(TestModule) << "Failed to create test module";
  
  // Test different configurations
  std::vector<PassConfiguration> Configs = {
    []() { PassConfiguration c; c.InstrumentationMode = InstrumentationMode::Lightweight; return c; }(),
    []() { PassConfiguration c; c.InstrumentationMode = InstrumentationMode::Standard; return c; }(),
    []() { PassConfiguration c; c.InstrumentationMode = InstrumentationMode::Full; return c; }()
  };
  
  std::vector<PerformanceMeasurement> Measurements;
  
  for (const auto& Config : Configs) {
    auto ModuleCopy = CloneModule(*TestModule);
    PerformanceMeasurement Measurement = measureMemoryUsage(*ModuleCopy, Config);
    Measurements.push_back(Measurement);
  }
  
  // Verify memory usage increases with instrumentation level
  EXPECT_LE(Measurements[0].MemoryUsageBytes, Measurements[1].MemoryUsageBytes)
    << "Lightweight should use less memory than Standard";
  
  EXPECT_LE(Measurements[1].MemoryUsageBytes, Measurements[2].MemoryUsageBytes)
    << "Standard should use less memory than Full";
}

void MemoryUsageTest::testMemoryLeaks() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 100, 5);
  ASSERT_TRUE(TestModule) << "Failed to create test module";
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  
  uint64_t InitialMemory = getCurrentMemoryUsage();
  
  // Run pass multiple times
  for (int i = 0; i < 10; ++i) {
    auto ModuleCopy = CloneModule(*TestModule);
    measureMemoryUsage(*ModuleCopy, Config);
  }
  
  uint64_t FinalMemory = getCurrentMemoryUsage();
  
  // Memory should not grow significantly after multiple runs
  double MemoryGrowth = static_cast<double>(FinalMemory - InitialMemory) / InitialMemory;
  EXPECT_LT(MemoryGrowth, 0.1) << "Potential memory leak detected: " << (MemoryGrowth * 100.0) << "% growth";
}

void MemoryUsageTest::testPeakMemoryUsage() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 1000, 10);
  ASSERT_TRUE(TestModule) << "Failed to create large test module";
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Full;
  
  PerformanceTestUtils::MemoryMonitor Monitor;
  Monitor.startMonitoring(5); // Sample every 5ms
  
  measureMemoryUsage(*TestModule, Config);
  
  Monitor.stopMonitoring();
  
  uint64_t PeakMemory = Monitor.getPeakMemoryUsage();
  uint64_t BaselineMemory = BaselineMemoryUsage;
  
  // Peak memory should be reasonable
  double PeakRatio = static_cast<double>(PeakMemory) / BaselineMemory;
  EXPECT_LT(PeakRatio, 10.0) << "Peak memory usage too high: " << PeakRatio << "x baseline";
}

uint64_t MemoryUsageTest::getCurrentMemoryUsage() {
  return sys::Process::GetMallocUsage();
}

std::vector<uint64_t> MemoryUsageTest::monitorMemoryUsage(std::function<void()> Operation, uint32_t SampleIntervalMs) {
  std::vector<uint64_t> MemoryTrace;
  std::atomic<bool> ShouldStop(false);
  
  // Start monitoring thread
  std::thread MonitorThread([&]() {
    while (!ShouldStop.load()) {
      MemoryTrace.push_back(getCurrentMemoryUsage());
      std::this_thread::sleep_for(std::chrono::milliseconds(SampleIntervalMs));
    }
  });
  
  // Run operation
  Operation();
  
  // Stop monitoring
  ShouldStop.store(true);
  MonitorThread.join();
  
  return MemoryTrace;
}

//===----------------------------------------------------------------------===//
// ScalabilityTest Implementation
//===----------------------------------------------------------------------===//

void ScalabilityTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
}

void ScalabilityTest::TearDown() {
  ConfigManager.reset();
  Context.reset();
}

ScalabilityTestResult ScalabilityTest::testFunctionCountScalability() {
  std::vector<BenchmarkConfiguration> Configs;
  
  // Test with increasing function counts
  std::vector<uint32_t> FunctionCounts = {10, 50, 100, 500, 1000};
  
  for (uint32_t Count : FunctionCounts) {
    BenchmarkConfiguration Config;
    Config.FunctionCount = Count;
    Config.MPICallsPerFunction = 5;
    Config.InstrumentationConfig.InstrumentationMode = InstrumentationMode::Standard;
    Configs.push_back(Config);
  }
  
  return runScalabilityTest(Configs);
}

ScalabilityTestResult ScalabilityTest::testMPICallCountScalability() {
  std::vector<BenchmarkConfiguration> Configs;
  
  // Test with increasing MPI call counts
  std::vector<uint32_t> CallCounts = {1, 5, 10, 25, 50};
  
  for (uint32_t Count : CallCounts) {
    BenchmarkConfiguration Config;
    Config.FunctionCount = 100;
    Config.MPICallsPerFunction = Count;
    Config.InstrumentationConfig.InstrumentationMode = InstrumentationMode::Standard;
    Configs.push_back(Config);
  }
  
  return runScalabilityTest(Configs);
}

ScalabilityTestResult ScalabilityTest::testComplexityScalability() {
  std::vector<BenchmarkConfiguration> Configs;
  
  // Test with increasing complexity (functions × calls)
  std::vector<std::pair<uint32_t, uint32_t>> ComplexityLevels = {
    {10, 2}, {25, 4}, {50, 8}, {100, 16}, {200, 32}
  };
  
  for (const auto& Level : ComplexityLevels) {
    BenchmarkConfiguration Config;
    Config.FunctionCount = Level.first;
    Config.MPICallsPerFunction = Level.second;
    Config.InstrumentationConfig.InstrumentationMode = InstrumentationMode::Standard;
    Configs.push_back(Config);
  }
  
  return runScalabilityTest(Configs);
}

ScalabilityTestResult ScalabilityTest::testInstrumentationLevelScalability() {
  std::vector<BenchmarkConfiguration> Configs;
  
  std::vector<InstrumentationMode> Levels = {
    InstrumentationMode::Lightweight,
    InstrumentationMode::Standard,
    InstrumentationMode::Full
  };
  
  for (auto Level : Levels) {
    BenchmarkConfiguration Config;
    Config.FunctionCount = 100;
    Config.MPICallsPerFunction = 10;
    Config.InstrumentationConfig.InstrumentationMode = Level;
    Configs.push_back(Config);
  }
  
  return runScalabilityTest(Configs);
}

ScalabilityTestResult ScalabilityTest::runScalabilityTest(const std::vector<BenchmarkConfiguration>& Configs) {
  ScalabilityTestResult Result;
  
  try {
    for (const auto& Config : Configs) {
      auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 
                                                                   Config.FunctionCount, 
                                                                   Config.MPICallsPerFunction, 
                                                                   true);
      if (!TestModule) {
        Result.ErrorMessage = "Failed to create test module";
        return Result;
      }
      
      // Measure performance
      PerformanceMeasurement Measurement;
      
      PerformanceTestUtils::HighPrecisionTimer Timer;
      Timer.start();
      
      ModulePassManager MPM;
      ModuleAnalysisManager MAM;
      
      PassBuilder PB;
      PB.registerModuleAnalyses(MAM);
      
      ConfigManager->initialize(Config.InstrumentationConfig);
      MPM.addPass(MPISanitizerPass());
      MPM.run(*TestModule, MAM);
      
      Timer.stop();
      
      Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
      Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage();
      Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(*TestModule);
      Measurement.MPICallsDetected = PerformanceTestUtils::countMPICalls(*TestModule);
      Measurement.HooksInserted = PerformanceTestUtils::countInstrumentationHooks(*TestModule);
      
      Result.Measurements.push_back(Measurement);
    }
    
    Result.PassedScalabilityTest = validateScalabilityRequirements(Result);
    
  } catch (const std::exception& E) {
    Result.ErrorMessage = "Exception during scalability test: " + std::string(E.what());
  }
  
  return Result;
}

bool ScalabilityTest::validateScalabilityRequirements(const ScalabilityTestResult& Result) {
  if (Result.Measurements.size() < 2) return false;
  
  // Check that compilation time growth is reasonable
  for (size_t i = 1; i < Result.Measurements.size(); ++i) {
    double TimeRatio = static_cast<double>(Result.Measurements[i].CompilationTimeUs) / 
                      Result.Measurements[i-1].CompilationTimeUs;
    
    double ComplexityRatio = static_cast<double>(Result.Measurements[i].OriginalInstructions) /
                            Result.Measurements[i-1].OriginalInstructions;
    
    // Time growth should not be significantly worse than linear with complexity
    if (TimeRatio > ComplexityRatio * MaxLinearGrowthFactor) {
      return false;
    }
  }
  
  return true;
}

std::string ScalabilityTest::generatePerformanceReport(const ScalabilityTestResult& Result) {
  std::ostringstream Report;
  
  Report << "=== Scalability Test Report ===\n";
  Report << "Test Passed: " << (Result.PassedScalabilityTest ? "YES" : "NO") << "\n";
  Report << "Number of Measurements: " << Result.Measurements.size() << "\n";
  
  if (!Result.ErrorMessage.empty()) {
    Report << "Error: " << Result.ErrorMessage << "\n";
  }
  
  Report << "\nMeasurements:\n";
  for (size_t i = 0; i < Result.Measurements.size(); ++i) {
    const auto& M = Result.Measurements[i];
    Report << "  " << i << ": " << M.CompilationTimeUs << "μs, "
           << M.MemoryUsageBytes << " bytes, "
           << M.OriginalInstructions << " instructions, "
           << M.MPICallsDetected << " MPI calls\n";
  }
  
  Report << "\nAverage Compilation Time: " << Result.getAverageCompilationTime() << "μs\n";
  Report << "Average Memory Usage: " << Result.getAverageMemoryUsage() << " bytes\n";
  
  return Report.str();
}

} // namespace llvm
//===----------------------------------------------------------------------===//
// InstrumentationOverheadTest Implementation
//===----------------------------------------------------------------------===//

void InstrumentationOverheadTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
}

void InstrumentationOverheadTest::TearDown() {
  ConfigManager.reset();
  Context.reset();
}

PerformanceMeasurement InstrumentationOverheadTest::testPreCallHookOverhead() {
  auto TestModule = createInstrumentationTestModule("pre_call_hooks");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  Config.EnablePreHooks = true;
  Config.EnablePostHooks = false;
  Config.EnablePerformanceMonitoring = false;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

PerformanceMeasurement InstrumentationOverheadTest::testPostCallHookOverhead() {
  auto TestModule = createInstrumentationTestModule("post_call_hooks");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  Config.EnablePreHooks = false;
  Config.EnablePostHooks = true;
  Config.EnablePerformanceMonitoring = false;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

PerformanceMeasurement InstrumentationOverheadTest::testPerformanceMonitoringOverhead() {
  auto TestModule = createInstrumentationTestModule("performance_monitoring");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  Config.EnablePerformanceMonitoring = true;
  Config.EnableErrorChecking = false;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

PerformanceMeasurement InstrumentationOverheadTest::testErrorCheckingOverhead() {
  auto TestModule = createInstrumentationTestModule("error_checking");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  Config.EnableErrorChecking = true;
  Config.EnablePerformanceMonitoring = false;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

PerformanceMeasurement InstrumentationOverheadTest::testDeadlockDetectionOverhead() {
  auto TestModule = createInstrumentationTestModule("deadlock_detection");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  Config.EnableDeadlockDetection = true;
  Config.EnableDataRaceDetection = false;
  Config.EnablePerformanceMonitoring = false;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

PerformanceMeasurement InstrumentationOverheadTest::testDataRaceDetectionOverhead() {
  auto TestModule = createInstrumentationTestModule("data_race_detection");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  Config.EnableDataRaceDetection = true;
  Config.EnableDeadlockDetection = false;
  Config.EnablePerformanceMonitoring = false;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

PerformanceMeasurement InstrumentationOverheadTest::testCombinedInstrumentationOverhead() {
  auto TestModule = createInstrumentationTestModule("combined_instrumentation");
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Full;
  Config.EnablePreHooks = true;
  Config.EnablePostHooks = true;
  Config.EnablePerformanceMonitoring = true;
  Config.EnableErrorChecking = true;
  Config.EnableDeadlockDetection = true;
  Config.EnableDataRaceDetection = true;
  
  return measureInstrumentationOverhead(*TestModule, Config);
}

void InstrumentationOverheadTest::testMPIFunctionTypeOverhead() {
  // Test overhead for different MPI function types
  std::vector<std::string> FunctionTypes = {
    "point_to_point", "collective", "communicator", "datatype", "request"
  };
  
  std::vector<PerformanceMeasurement> Measurements;
  
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  
  for (const auto& Type : FunctionTypes) {
    auto TestModule = createInstrumentationTestModule(Type);
    PerformanceMeasurement Measurement = measureInstrumentationOverhead(*TestModule, Config);
    Measurements.push_back(Measurement);
  }
  
  // Verify that overhead is reasonable for all function types
  for (size_t i = 0; i < Measurements.size(); ++i) {
    EXPECT_LT(Measurements[i].getInstructionOverhead(), 300.0)
      << "Overhead too high for " << FunctionTypes[i] << ": " << Measurements[i].getInstructionOverhead() << "%";
  }
}

void InstrumentationOverheadTest::testSelectiveInstrumentationBenefits() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 100, 10, true);
  
  // Test full instrumentation
  PassConfiguration FullConfig;
  FullConfig.InstrumentationMode = InstrumentationMode::Full;
  
  // Test selective instrumentation
  PassConfiguration SelectiveConfig;
  SelectiveConfig.InstrumentationMode = InstrumentationMode::Selective;
  SelectiveConfig.EnableOptimizations = true;
  
  auto FullModule = CloneModule(*TestModule);
  auto SelectiveModule = CloneModule(*TestModule);
  
  PerformanceMeasurement FullMeasurement = measureInstrumentationOverhead(*FullModule, FullConfig);
  PerformanceMeasurement SelectiveMeasurement = measureInstrumentationOverhead(*SelectiveModule, SelectiveConfig);
  
  // Selective instrumentation should have lower overhead
  EXPECT_LT(SelectiveMeasurement.getInstructionOverhead(), FullMeasurement.getInstructionOverhead())
    << "Selective instrumentation should have lower overhead than full instrumentation";
  
  EXPECT_LT(SelectiveMeasurement.CompilationTimeUs, FullMeasurement.CompilationTimeUs)
    << "Selective instrumentation should compile faster than full instrumentation";
}

std::unique_ptr<Module> InstrumentationOverheadTest::createInstrumentationTestModule(const std::string& InstrumentationType) {
  auto M = std::make_unique<Module>("instrumentation_test_" + InstrumentationType, *Context);
  
  // Create function with specific MPI call patterns based on instrumentation type
  FunctionType* FT = FunctionType::get(Type::getInt32Ty(*Context), false);
  Function* F = Function::Create(FT, Function::ExternalLinkage, "test_function", M.get());
  
  BasicBlock* BB = BasicBlock::Create(*Context, "entry", F);
  IRBuilder<> Builder(BB);
  
  if (InstrumentationType == "point_to_point") {
    // Create MPI_Send/Recv calls
    std::vector<Type*> SendArgs = {Type::getInt8PtrTy(*Context), Type::getInt32Ty(*Context),
                                  Type::getInt32Ty(*Context), Type::getInt32Ty(*Context),
                                  Type::getInt32Ty(*Context), Type::getInt32Ty(*Context)};
    FunctionType* SendFT = FunctionType::get(Type::getInt32Ty(*Context), SendArgs, false);
    Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
    
    // Create multiple calls
    for (int i = 0; i < 10; ++i) {
      std::vector<Value*> Args(6, ConstantInt::get(Type::getInt32Ty(*Context), i));
      Args[0] = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
      Builder.CreateCall(MPISend, Args);
    }
  } else if (InstrumentationType == "collective") {
    // Create MPI_Bcast calls
    std::vector<Type*> BcastArgs = {Type::getInt8PtrTy(*Context), Type::getInt32Ty(*Context),
                                   Type::getInt32Ty(*Context), Type::getInt32Ty(*Context),
                                   Type::getInt32Ty(*Context)};
    FunctionType* BcastFT = FunctionType::get(Type::getInt32Ty(*Context), BcastArgs, false);
    Function* MPIBcast = Function::Create(BcastFT, Function::ExternalLinkage, "MPI_Bcast", M.get());
    
    for (int i = 0; i < 5; ++i) {
      std::vector<Value*> Args(5, ConstantInt::get(Type::getInt32Ty(*Context), i));
      Args[0] = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
      Builder.CreateCall(MPIBcast, Args);
    }
  } else {
    // Default: create generic MPI calls
    std::vector<Type*> GenericArgs = {Type::getInt32Ty(*Context)};
    FunctionType* GenericFT = FunctionType::get(Type::getInt32Ty(*Context), GenericArgs, false);
    Function* MPIGeneric = Function::Create(GenericFT, Function::ExternalLinkage, "MPI_Generic", M.get());
    
    for (int i = 0; i < 8; ++i) {
      Value* Arg = ConstantInt::get(Type::getInt32Ty(*Context), i);
      Builder.CreateCall(MPIGeneric, {Arg});
    }
  }
  
  Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(*Context), 0));
  
  return M;
}

PerformanceMeasurement InstrumentationOverheadTest::measureInstrumentationOverhead(Module& M, const PassConfiguration& Config) {
  PerformanceMeasurement Measurement;
  
  // Measure original characteristics
  Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(M);
  Measurement.OriginalModuleSize = PerformanceTestUtils::measureModuleSize(M);
  Measurement.MPICallsDetected = PerformanceTestUtils::countMPICalls(M);
  
  // Run instrumentation
  PerformanceTestUtils::HighPrecisionTimer Timer;
  Timer.start();
  
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  
  ConfigManager->initialize(Config);
  MPM.addPass(MPISanitizerPass());
  MPM.run(M, MAM);
  
  Timer.stop();
  
  // Measure results
  Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
  Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(M);
  Measurement.InstrumentedModuleSize = PerformanceTestUtils::measureModuleSize(M);
  Measurement.HooksInserted = PerformanceTestUtils::countInstrumentationHooks(M);
  Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage();
  
  return Measurement;
}

//===----------------------------------------------------------------------===//
// OptimizationEffectivenessTest Implementation
//===----------------------------------------------------------------------===//

void OptimizationEffectivenessTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
}

void OptimizationEffectivenessTest::TearDown() {
  ConfigManager.reset();
  Context.reset();
}

void OptimizationEffectivenessTest::testStaticAnalysisOptimizations() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 50, 8, true);
  
  PassConfiguration OptimizedConfig;
  OptimizedConfig.EnableOptimizations = true;
  OptimizedConfig.EnableStaticAnalysis = true;
  
  PassConfiguration UnoptimizedConfig;
  UnoptimizedConfig.EnableOptimizations = false;
  UnoptimizedConfig.EnableStaticAnalysis = false;
  
  double Benefit = measureOptimizationBenefit(*TestModule, OptimizedConfig, UnoptimizedConfig);
  
  EXPECT_GT(Benefit, 0.1) << "Static analysis optimizations should provide at least 10% benefit";
  EXPECT_LT(Benefit, 0.8) << "Optimization benefit seems unrealistically high: " << (Benefit * 100.0) << "%";
}

void OptimizationEffectivenessTest::testSelectiveInstrumentationEffectiveness() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 100, 10, true);
  
  PassConfiguration SelectiveConfig;
  SelectiveConfig.InstrumentationMode = InstrumentationMode::Selective;
  SelectiveConfig.EnableOptimizations = true;
  
  PassConfiguration FullConfig;
  FullConfig.InstrumentationMode = InstrumentationMode::Full;
  FullConfig.EnableOptimizations = false;
  
  double Benefit = measureOptimizationBenefit(*TestModule, SelectiveConfig, FullConfig);
  
  EXPECT_GT(Benefit, 0.2) << "Selective instrumentation should provide at least 20% benefit over full instrumentation";
}

void OptimizationEffectivenessTest::testOptimizationLevelImpact() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 75, 6, true);
  
  std::vector<OptimizationLevel> Levels = {
    OptimizationLevel::None,
    OptimizationLevel::Standard,
    OptimizationLevel::Aggressive
  };
  
  std::vector<PerformanceMeasurement> Measurements;
  
  for (auto Level : Levels) {
    PassConfiguration Config;
    Config.OptimizationLevel = Level;
    Config.EnableOptimizations = (Level != OptimizationLevel::None);
    
    auto ModuleCopy = CloneModule(*TestModule);
    
    PerformanceTestUtils::HighPrecisionTimer Timer;
    Timer.start();
    
    ModulePassManager MPM;
    ModuleAnalysisManager MAM;
    
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    
    ConfigManager->initialize(Config);
    MPM.addPass(MPISanitizerPass());
    MPM.run(*ModuleCopy, MAM);
    
    Timer.stop();
    
    PerformanceMeasurement Measurement;
    Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
    Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(*ModuleCopy);
    Measurement.HooksInserted = PerformanceTestUtils::countInstrumentationHooks(*ModuleCopy);
    
    Measurements.push_back(Measurement);
  }
  
  // Higher optimization levels should generally produce better results
  EXPECT_GE(Measurements[2].HooksInserted, Measurements[1].HooksInserted)
    << "Aggressive optimization should not reduce hook count below standard";
  
  // But instruction overhead should be better with optimization
  double NoneOverhead = static_cast<double>(Measurements[0].InstrumentedInstructions) / 
                       PerformanceTestUtils::countInstructions(*TestModule);
  double AggressiveOverhead = static_cast<double>(Measurements[2].InstrumentedInstructions) / 
                             PerformanceTestUtils::countInstructions(*TestModule);
  
  EXPECT_LE(AggressiveOverhead, NoneOverhead * 1.1)
    << "Aggressive optimization should not significantly increase instruction overhead";
}

void OptimizationEffectivenessTest::testConfigurationOptimizationEffectiveness() {
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 60, 7, true);
  
  // Test configuration-driven optimizations
  PassConfiguration OptimizedConfig;
  OptimizedConfig.EnableOptimizations = true;
  OptimizedConfig.InstrumentationMode = InstrumentationMode::Selective;
  OptimizedConfig.EnableConfigurationOptimization = true;
  
  PassConfiguration BasicConfig;
  BasicConfig.EnableOptimizations = false;
  BasicConfig.InstrumentationMode = InstrumentationMode::Standard;
  BasicConfig.EnableConfigurationOptimization = false;
  
  double Benefit = measureOptimizationBenefit(*TestModule, OptimizedConfig, BasicConfig);
  
  EXPECT_GT(Benefit, 0.15) << "Configuration optimizations should provide at least 15% benefit";
}

void OptimizationEffectivenessTest::compareOptimizedVsUnoptimized() {
  std::vector<uint32_t> ModuleSizes = {25, 50, 100, 200};
  
  for (uint32_t Size : ModuleSizes) {
    auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, Size, 5, true);
    
    PassConfiguration OptimizedConfig;
    OptimizedConfig.EnableOptimizations = true;
    OptimizedConfig.InstrumentationMode = InstrumentationMode::Selective;
    
    PassConfiguration UnoptimizedConfig;
    UnoptimizedConfig.EnableOptimizations = false;
    UnoptimizedConfig.InstrumentationMode = InstrumentationMode::Full;
    
    double Benefit = measureOptimizationBenefit(*TestModule, OptimizedConfig, UnoptimizedConfig);
    
    EXPECT_GT(Benefit, 0.05) << "Optimizations should provide benefit for module size " << Size;
    EXPECT_LT(Benefit, 0.9) << "Optimization benefit seems too high for module size " << Size;
  }
}

double OptimizationEffectivenessTest::measureOptimizationBenefit(Module& M, 
                                                               const PassConfiguration& OptimizedConfig, 
                                                               const PassConfiguration& UnoptimizedConfig) {
  auto OptimizedModule = CloneModule(M);
  auto UnoptimizedModule = CloneModule(M);
  
  // Measure optimized version
  PerformanceTestUtils::HighPrecisionTimer OptimizedTimer;
  OptimizedTimer.start();
  
  ModulePassManager OptimizedMPM;
  ModuleAnalysisManager OptimizedMAM;
  PassBuilder OptimizedPB;
  OptimizedPB.registerModuleAnalyses(OptimizedMAM);
  
  ConfigManager->initialize(OptimizedConfig);
  OptimizedMPM.addPass(MPISanitizerPass());
  OptimizedMPM.run(*OptimizedModule, OptimizedMAM);
  
  OptimizedTimer.stop();
  
  // Measure unoptimized version
  PerformanceTestUtils::HighPrecisionTimer UnoptimizedTimer;
  UnoptimizedTimer.start();
  
  ModulePassManager UnoptimizedMPM;
  ModuleAnalysisManager UnoptimizedMAM;
  PassBuilder UnoptimizedPB;
  UnoptimizedPB.registerModuleAnalyses(UnoptimizedMAM);
  
  ConfigManager->initialize(UnoptimizedConfig);
  UnoptimizedMPM.addPass(MPISanitizerPass());
  UnoptimizedMPM.run(*UnoptimizedModule, UnoptimizedMAM);
  
  UnoptimizedTimer.stop();
  
  // Calculate benefit (reduction in compilation time)
  uint64_t OptimizedTime = OptimizedTimer.getElapsedMicroseconds();
  uint64_t UnoptimizedTime = UnoptimizedTimer.getElapsedMicroseconds();
  
  if (UnoptimizedTime == 0) return 0.0;
  
  return static_cast<double>(UnoptimizedTime - OptimizedTime) / UnoptimizedTime;
}

//===----------------------------------------------------------------------===//
// PerformanceRegressionTest Implementation
//===----------------------------------------------------------------------===//

void PerformanceRegressionTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
}

void PerformanceRegressionTest::TearDown() {
  ConfigManager.reset();
  Context.reset();
}

void PerformanceRegressionTest::testBaselinePerformance() {
  // Load baseline data if available
  bool HasBaseline = loadBaselineData("performance_baseline.json");
  
  if (!HasBaseline) {
    // Generate baseline data
    std::vector<PerformanceMeasurement> CurrentData;
    
    std::vector<uint32_t> TestSizes = {10, 50, 100, 500};
    PassConfiguration Config;
    Config.InstrumentationMode = InstrumentationMode::Standard;
    
    for (uint32_t Size : TestSizes) {
      auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, Size, 5, false);
      
      PerformanceTestUtils::HighPrecisionTimer Timer;
      Timer.start();
      
      ModulePassManager MPM;
      ModuleAnalysisManager MAM;
      PassBuilder PB;
      PB.registerModuleAnalyses(MAM);
      
      ConfigManager->initialize(Config);
      MPM.addPass(MPISanitizerPass());
      MPM.run(*TestModule, MAM);
      
      Timer.stop();
      
      PerformanceMeasurement Measurement;
      Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
      Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage();
      Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(*TestModule);
      Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(*TestModule);
      
      CurrentData.push_back(Measurement);
    }
    
    // Save as new baseline
    saveBaselineData("performance_baseline.json", CurrentData);
    BaselineData = CurrentData;
  }
  
  // Test current performance against baseline
  std::vector<PerformanceMeasurement> CurrentData;
  
  std::vector<uint32_t> TestSizes = {10, 50, 100, 500};
  PassConfiguration Config;
  Config.InstrumentationMode = InstrumentationMode::Standard;
  
  for (uint32_t Size : TestSizes) {
    auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, Size, 5, false);
    
    PerformanceTestUtils::HighPrecisionTimer Timer;
    Timer.start();
    
    ModulePassManager MPM;
    ModuleAnalysisManager MAM;
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    
    ConfigManager->initialize(Config);
    MPM.addPass(MPISanitizerPass());
    MPM.run(*TestModule, MAM);
    
    Timer.stop();
    
    PerformanceMeasurement Measurement;
    Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
    Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage();
    Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(*TestModule);
    Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(*TestModule);
    
    CurrentData.push_back(Measurement);
  }
  
  // Compare against baseline
  bool NoRegression = compareAgainstBaseline(CurrentData, BaselineData, MaxRegressionPercentage);
  EXPECT_TRUE(NoRegression) << "Performance regression detected against baseline";
}

void PerformanceRegressionTest::testBenchmarkSuite() {
  // Standard benchmark configurations
  std::vector<BenchmarkConfiguration> Benchmarks = {
    {50, 3, 5, PassConfiguration(), false, 40.0, 80.0, 150.0},   // Small benchmark
    {100, 5, 5, PassConfiguration(), false, 50.0, 100.0, 200.0}, // Medium benchmark
    {200, 8, 3, PassConfiguration(), false, 80.0, 150.0, 300.0}  // Large benchmark
  };
  
  for (size_t i = 0; i < Benchmarks.size(); ++i) {
    const auto& Benchmark = Benchmarks[i];
    
    auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 
                                                                 Benchmark.FunctionCount, 
                                                                 Benchmark.MPICallsPerFunction, 
                                                                 true);
    
    PerformanceTestUtils::HighPrecisionTimer Timer;
    Timer.start();
    
    ModulePassManager MPM;
    ModuleAnalysisManager MAM;
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    
    ConfigManager->initialize(Benchmark.InstrumentationConfig);
    MPM.addPass(MPISanitizerPass());
    MPM.run(*TestModule, MAM);
    
    Timer.stop();
    
    PerformanceMeasurement Measurement;
    Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
    Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage();
    Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(*TestModule);
    Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(*TestModule);
    Measurement.MPICallsDetected = PerformanceTestUtils::countMPICalls(*TestModule);
    Measurement.HooksInserted = PerformanceTestUtils::countInstrumentationHooks(*TestModule);
    
    // Validate against benchmark requirements
    bool PassedBenchmark = PerformanceTestUtils::validatePerformanceRequirements(Measurement, Benchmark);
    EXPECT_TRUE(PassedBenchmark) << "Failed benchmark " << i << " - overhead too high";
  }
}

void PerformanceRegressionTest::testRegressionDetection() {
  // This test simulates regression detection by comparing different configurations
  auto TestModule = PerformanceTestUtils::createBenchmarkModule(*Context, 100, 5, true);
  
  // "Good" configuration (baseline)
  PassConfiguration GoodConfig;
  GoodConfig.InstrumentationMode = InstrumentationMode::Selective;
  GoodConfig.EnableOptimizations = true;
  
  // "Bad" configuration (simulating regression)
  PassConfiguration BadConfig;
  BadConfig.InstrumentationMode = InstrumentationMode::Full;
  BadConfig.EnableOptimizations = false;
  
  auto GoodModule = CloneModule(*TestModule);
  auto BadModule = CloneModule(*TestModule);
  
  // Measure "good" performance
  PerformanceTestUtils::HighPrecisionTimer GoodTimer;
  GoodTimer.start();
  
  ModulePassManager GoodMPM;
  ModuleAnalysisManager GoodMAM;
  PassBuilder GoodPB;
  GoodPB.registerModuleAnalyses(GoodMAM);
  
  ConfigManager->initialize(GoodConfig);
  GoodMPM.addPass(MPISanitizerPass());
  GoodMPM.run(*GoodModule, GoodMAM);
  
  GoodTimer.stop();
  
  // Measure "bad" performance
  PerformanceTestUtils::HighPrecisionTimer BadTimer;
  BadTimer.start();
  
  ModulePassManager BadMPM;
  ModuleAnalysisManager BadMAM;
  PassBuilder BadPB;
  BadPB.registerModuleAnalyses(BadMAM);
  
  ConfigManager->initialize(BadConfig);
  BadMPM.addPass(MPISanitizerPass());
  BadMPM.run(*BadModule, BadMAM);
  
  BadTimer.stop();
  
  // Calculate regression
  uint64_t GoodTime = GoodTimer.getElapsedMicroseconds();
  uint64_t BadTime = BadTimer.getElapsedMicroseconds();
  
  double Regression = static_cast<double>(BadTime - GoodTime) / GoodTime;
  
  // Bad configuration should show significant regression
  EXPECT_GT(Regression, 0.2) << "Expected significant regression with bad configuration";
  EXPECT_LT(Regression, 5.0) << "Regression seems unrealistically high: " << (Regression * 100.0) << "%";
}

bool PerformanceRegressionTest::loadBaselineData(const std::string& BaselineFile) {
  // Simplified baseline loading - in a real implementation, this would parse JSON
  // For now, just return false to indicate no baseline available
  return false;
}

void PerformanceRegressionTest::saveBaselineData(const std::string& BaselineFile, 
                                                const std::vector<PerformanceMeasurement>& Data) {
  // Simplified baseline saving - in a real implementation, this would write JSON
  BaselineData = Data;
}

bool PerformanceRegressionTest::compareAgainstBaseline(const std::vector<PerformanceMeasurement>& Current, 
                                                      const std::vector<PerformanceMeasurement>& Baseline,
                                                      double TolerancePercentage) {
  if (Current.size() != Baseline.size()) return false;
  
  for (size_t i = 0; i < Current.size(); ++i) {
    // Check compilation time regression
    double TimeRegression = static_cast<double>(Current[i].CompilationTimeUs - Baseline[i].CompilationTimeUs) / 
                           Baseline[i].CompilationTimeUs;
    
    if (TimeRegression > TolerancePercentage / 100.0) {
      return false;
    }
    
    // Check memory regression
    double MemoryRegression = static_cast<double>(Current[i].MemoryUsageBytes - Baseline[i].MemoryUsageBytes) / 
                             Baseline[i].MemoryUsageBytes;
    
    if (MemoryRegression > MaxMemoryRegressionPercentage / 100.0) {
      return false;
    }
  }
  
  return true;
}

} // namespace llvm
//===----------------------------------------------------------------------===//
// PerformanceTestUtils Implementation
//===----------------------------------------------------------------------===//

namespace PerformanceTestUtils {

std::unique_ptr<Module> createBenchmarkModule(LLVMContext& Context, 
                                             uint32_t FunctionCount,
                                             uint32_t MPICallsPerFunction,
                                             bool IncludeComplexPatterns) {
  auto M = std::make_unique<Module>("benchmark_module", Context);
  
  // Create MPI function declarations
  std::vector<Type*> SendArgs = {Type::getInt8PtrTy(Context), Type::getInt32Ty(Context),
                                Type::getInt32Ty(Context), Type::getInt32Ty(Context),
                                Type::getInt32Ty(Context), Type::getInt32Ty(Context)};
  FunctionType* SendFT = FunctionType::get(Type::getInt32Ty(Context), SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  std::vector<Type*> RecvArgs = SendArgs; // Same signature
  FunctionType* RecvFT = FunctionType::get(Type::getInt32Ty(Context), RecvArgs, false);
  Function* MPIRecv = Function::Create(RecvFT, Function::ExternalLinkage, "MPI_Recv", M.get());
  
  std::vector<Type*> BcastArgs = {Type::getInt8PtrTy(Context), Type::getInt32Ty(Context),
                                 Type::getInt32Ty(Context), Type::getInt32Ty(Context),
                                 Type::getInt32Ty(Context)};
  FunctionType* BcastFT = FunctionType::get(Type::getInt32Ty(Context), BcastArgs, false);
  Function* MPIBcast = Function::Create(BcastFT, Function::ExternalLinkage, "MPI_Bcast", M.get());
  
  // Create benchmark functions
  for (uint32_t i = 0; i < FunctionCount; ++i) {
    std::string FuncName = "benchmark_function_" + std::to_string(i);
    
    FunctionType* FT = FunctionType::get(Type::getInt32Ty(Context), false);
    Function* F = Function::Create(FT, Function::ExternalLinkage, FuncName, M.get());
    
    BasicBlock* EntryBB = BasicBlock::Create(Context, "entry", F);
    IRBuilder<> Builder(EntryBB);
    
    // Add MPI calls
    for (uint32_t j = 0; j < MPICallsPerFunction; ++j) {
      // Alternate between different MPI functions
      Function* MPIFunc = nullptr;
      std::vector<Value*> Args;
      
      switch (j % 3) {
        case 0: // MPI_Send
          MPIFunc = MPISend;
          Args = {
            ConstantPointerNull::get(Type::getInt8PtrTy(Context)),
            ConstantInt::get(Type::getInt32Ty(Context), 100),
            ConstantInt::get(Type::getInt32Ty(Context), 0), // MPI_INT
            ConstantInt::get(Type::getInt32Ty(Context), (j + 1) % 4), // dest
            ConstantInt::get(Type::getInt32Ty(Context), j), // tag
            ConstantInt::get(Type::getInt32Ty(Context), 0)  // MPI_COMM_WORLD
          };
          break;
          
        case 1: // MPI_Recv
          MPIFunc = MPIRecv;
          Args = {
            ConstantPointerNull::get(Type::getInt8PtrTy(Context)),
            ConstantInt::get(Type::getInt32Ty(Context), 100),
            ConstantInt::get(Type::getInt32Ty(Context), 0), // MPI_INT
            ConstantInt::get(Type::getInt32Ty(Context), (j + 2) % 4), // source
            ConstantInt::get(Type::getInt32Ty(Context), j), // tag
            ConstantInt::get(Type::getInt32Ty(Context), 0)  // MPI_COMM_WORLD
          };
          break;
          
        case 2: // MPI_Bcast
          MPIFunc = MPIBcast;
          Args = {
            ConstantPointerNull::get(Type::getInt8PtrTy(Context)),
            ConstantInt::get(Type::getInt32Ty(Context), 50),
            ConstantInt::get(Type::getInt32Ty(Context), 0), // MPI_INT
            ConstantInt::get(Type::getInt32Ty(Context), 0), // root
            ConstantInt::get(Type::getInt32Ty(Context), 0)  // MPI_COMM_WORLD
          };
          break;
      }
      
      if (MPIFunc) {
        Builder.CreateCall(MPIFunc, Args);
      }
    }
    
    // Add complex patterns if requested
    if (IncludeComplexPatterns && i % 3 == 0) {
      // Add conditional MPI calls
      Value* Condition = Builder.CreateICmpEQ(
        ConstantInt::get(Type::getInt32Ty(Context), i % 2),
        ConstantInt::get(Type::getInt32Ty(Context), 0)
      );
      
      BasicBlock* ThenBB = BasicBlock::Create(Context, "then", F);
      BasicBlock* ElseBB = BasicBlock::Create(Context, "else", F);
      BasicBlock* MergeBB = BasicBlock::Create(Context, "merge", F);
      
      Builder.CreateCondBr(Condition, ThenBB, ElseBB);
      
      // Then block
      Builder.SetInsertPoint(ThenBB);
      std::vector<Value*> ThenArgs = {
        ConstantPointerNull::get(Type::getInt8PtrTy(Context)),
        ConstantInt::get(Type::getInt32Ty(Context), 25),
        ConstantInt::get(Type::getInt32Ty(Context), 0),
        ConstantInt::get(Type::getInt32Ty(Context), 1),
        ConstantInt::get(Type::getInt32Ty(Context), 100),
        ConstantInt::get(Type::getInt32Ty(Context), 0)
      };
      Builder.CreateCall(MPISend, ThenArgs);
      Builder.CreateBr(MergeBB);
      
      // Else block
      Builder.SetInsertPoint(ElseBB);
      std::vector<Value*> ElseArgs = {
        ConstantPointerNull::get(Type::getInt8PtrTy(Context)),
        ConstantInt::get(Type::getInt32Ty(Context), 25),
        ConstantInt::get(Type::getInt32Ty(Context), 0),
        ConstantInt::get(Type::getInt32Ty(Context), 2),
        ConstantInt::get(Type::getInt32Ty(Context), 200),
        ConstantInt::get(Type::getInt32Ty(Context), 0)
      };
      Builder.CreateCall(MPISend, ElseArgs);
      Builder.CreateBr(MergeBB);
      
      // Merge block
      Builder.SetInsertPoint(MergeBB);
    }
    
    Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(Context), 0));
  }
  
  return M;
}

uint64_t measureModuleSize(Module& M) {
  std::string ModuleStr;
  raw_string_ostream Stream(ModuleStr);
  M.print(Stream, nullptr);
  Stream.flush();
  return ModuleStr.size();
}

uint32_t countInstructions(Module& M) {
  uint32_t Count = 0;
  for (Function& F : M) {
    for (BasicBlock& BB : F) {
      Count += BB.size();
    }
  }
  return Count;
}

uint32_t countMPICalls(Module& M) {
  uint32_t Count = 0;
  for (Function& F : M) {
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if (CallInst* Call = dyn_cast<CallInst>(&I)) {
          if (Function* Callee = Call->getCalledFunction()) {
            StringRef Name = Callee->getName();
            if (Name.startswith("MPI_") || Name.startswith("mpi_") || 
                Name.contains("MPI")) {
              Count++;
            }
          }
        }
      }
    }
  }
  return Count;
}

uint32_t countInstrumentationHooks(Module& M) {
  uint32_t Count = 0;
  for (Function& F : M) {
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if (CallInst* Call = dyn_cast<CallInst>(&I)) {
          if (Function* Callee = Call->getCalledFunction()) {
            StringRef Name = Callee->getName();
            if (Name.startswith("__mpi_sanitizer_") || 
                Name.startswith("__mpi_hook_") ||
                Name.startswith("__mpi_pre_") ||
                Name.startswith("__mpi_post_")) {
              Count++;
            }
          }
        }
      }
    }
  }
  return Count;
}

std::string generatePerformanceReport(const std::vector<PerformanceMeasurement>& Measurements,
                                     const std::string& TestName) {
  std::ostringstream Report;
  
  Report << "=== Performance Report: " << TestName << " ===\n";
  Report << "Number of Measurements: " << Measurements.size() << "\n\n";
  
  if (Measurements.empty()) {
    Report << "No measurements available.\n";
    return Report.str();
  }
  
  // Calculate statistics
  std::vector<double> CompilationTimes, MemoryUsages, InstructionOverheads, SizeOverheads;
  
  for (const auto& M : Measurements) {
    CompilationTimes.push_back(static_cast<double>(M.CompilationTimeUs));
    MemoryUsages.push_back(static_cast<double>(M.MemoryUsageBytes));
    InstructionOverheads.push_back(M.getInstructionOverhead());
    SizeOverheads.push_back(M.getSizeOverhead());
  }
  
  auto TimeStats = calculateStatistics(CompilationTimes);
  auto MemoryStats = calculateStatistics(MemoryUsages);
  auto InstrStats = calculateStatistics(InstructionOverheads);
  auto SizeStats = calculateStatistics(SizeOverheads);
  
  Report << "Compilation Time (μs):\n";
  Report << "  Mean: " << TimeStats.Mean << ", StdDev: " << TimeStats.StandardDeviation << "\n";
  Report << "  Min: " << TimeStats.Min << ", Max: " << TimeStats.Max << ", Median: " << TimeStats.Median << "\n\n";
  
  Report << "Memory Usage (bytes):\n";
  Report << "  Mean: " << MemoryStats.Mean << ", StdDev: " << MemoryStats.StandardDeviation << "\n";
  Report << "  Min: " << MemoryStats.Min << ", Max: " << MemoryStats.Max << ", Median: " << MemoryStats.Median << "\n\n";
  
  Report << "Instruction Overhead (%):\n";
  Report << "  Mean: " << InstrStats.Mean << ", StdDev: " << InstrStats.StandardDeviation << "\n";
  Report << "  Min: " << InstrStats.Min << ", Max: " << InstrStats.Max << ", Median: " << InstrStats.Median << "\n\n";
  
  Report << "Size Overhead (%):\n";
  Report << "  Mean: " << SizeStats.Mean << ", StdDev: " << SizeStats.StandardDeviation << "\n";
  Report << "  Min: " << SizeStats.Min << ", Max: " << SizeStats.Max << ", Median: " << SizeStats.Median << "\n\n";
  
  // Individual measurements
  Report << "Individual Measurements:\n";
  for (size_t i = 0; i < Measurements.size(); ++i) {
    const auto& M = Measurements[i];
    Report << "  " << i << ": " << M.CompilationTimeUs << "μs, "
           << M.MemoryUsageBytes << " bytes, "
           << M.getInstructionOverhead() << "% instr overhead, "
           << M.MPICallsDetected << " MPI calls, "
           << M.HooksInserted << " hooks\n";
  }
  
  Report << "================================\n";
  
  return Report.str();
}

bool validatePerformanceRequirements(const PerformanceMeasurement& Measurement,
                                    const BenchmarkConfiguration& Config) {
  // Check compilation overhead
  if (Measurement.CompilationTimeUs > Config.MaxCompilationOverhead * 1000) { // Convert to μs
    return false;
  }
  
  // Check instruction overhead
  if (Measurement.getInstructionOverhead() > Config.MaxInstructionOverhead) {
    return false;
  }
  
  // Check size overhead
  if (Measurement.getSizeOverhead() > Config.MaxSizeOverhead) {
    return false;
  }
  
  // Check that MPI calls were detected
  if (Measurement.MPICallsDetected == 0) {
    return false;
  }
  
  // Check that some hooks were inserted (unless in no-instrumentation mode)
  if (Config.InstrumentationConfig.InstrumentationMode != InstrumentationMode::None &&
      Measurement.HooksInserted == 0) {
    return false;
  }
  
  return true;
}

StatisticalMetrics calculateStatistics(const std::vector<double>& Values) {
  StatisticalMetrics Metrics;
  
  if (Values.empty()) return Metrics;
  
  // Calculate mean
  double Sum = std::accumulate(Values.begin(), Values.end(), 0.0);
  Metrics.Mean = Sum / Values.size();
  
  // Calculate standard deviation
  double SumSquaredDiffs = 0.0;
  for (double Value : Values) {
    double Diff = Value - Metrics.Mean;
    SumSquaredDiffs += Diff * Diff;
  }
  Metrics.StandardDeviation = std::sqrt(SumSquaredDiffs / Values.size());
  
  // Calculate min and max
  auto MinMax = std::minmax_element(Values.begin(), Values.end());
  Metrics.Min = *MinMax.first;
  Metrics.Max = *MinMax.second;
  
  // Calculate median
  std::vector<double> SortedValues = Values;
  std::sort(SortedValues.begin(), SortedValues.end());
  
  size_t Size = SortedValues.size();
  if (Size % 2 == 0) {
    Metrics.Median = (SortedValues[Size/2 - 1] + SortedValues[Size/2]) / 2.0;
  } else {
    Metrics.Median = SortedValues[Size/2];
  }
  
  return Metrics;
}

// HighPrecisionTimer Implementation
HighPrecisionTimer::HighPrecisionTimer() {
  reset();
}

void HighPrecisionTimer::start() {
  StartTime = std::chrono::high_resolution_clock::now();
  IsRunning = true;
}

void HighPrecisionTimer::stop() {
  if (IsRunning) {
    EndTime = std::chrono::high_resolution_clock::now();
    IsRunning = false;
  }
}

uint64_t HighPrecisionTimer::getElapsedMicroseconds() const {
  if (IsRunning) {
    auto CurrentTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(CurrentTime - StartTime).count();
  } else {
    return std::chrono::duration_cast<std::chrono::microseconds>(EndTime - StartTime).count();
  }
}

void HighPrecisionTimer::reset() {
  StartTime = std::chrono::high_resolution_clock::time_point();
  EndTime = std::chrono::high_resolution_clock::time_point();
  IsRunning = false;
}

// MemoryMonitor Implementation
MemoryMonitor::MemoryMonitor() {
  MemoryTrace.clear();
}

void MemoryMonitor::startMonitoring(uint32_t SampleIntervalMs) {
  SampleInterval = SampleIntervalMs;
  IsMonitoring = true;
  MemoryTrace.clear();
  
  MonitoringThread = std::thread([this]() {
    while (IsMonitoring) {
      uint64_t CurrentMemory = sys::Process::GetMallocUsage();
      MemoryTrace.push_back(CurrentMemory);
      std::this_thread::sleep_for(std::chrono::milliseconds(SampleInterval));
    }
  });
}

void MemoryMonitor::stopMonitoring() {
  IsMonitoring = false;
  if (MonitoringThread.joinable()) {
    MonitoringThread.join();
  }
}

std::vector<uint64_t> MemoryMonitor::getMemoryTrace() const {
  return MemoryTrace;
}

uint64_t MemoryMonitor::getPeakMemoryUsage() const {
  if (MemoryTrace.empty()) return 0;
  return *std::max_element(MemoryTrace.begin(), MemoryTrace.end());
}

} // namespace PerformanceTestUtils

//===----------------------------------------------------------------------===//
// Test Instantiations
//===----------------------------------------------------------------------===//

// Compilation Overhead Tests
TEST_F(CompilationOverheadTest, SmallModuleOverhead) {
  testSmallModuleOverhead();
}

TEST_F(CompilationOverheadTest, MediumModuleOverhead) {
  testMediumModuleOverhead();
}

TEST_F(CompilationOverheadTest, LargeModuleOverhead) {
  testLargeModuleOverhead();
}

TEST_F(CompilationOverheadTest, InstrumentationLevelOverhead) {
  testInstrumentationLevelOverhead();
}

TEST_F(CompilationOverheadTest, OptimizationOverhead) {
  testOptimizationOverhead();
}

// Memory Usage Tests
TEST_F(MemoryUsageTest, MemoryScaling) {
  testMemoryScaling();
}

TEST_F(MemoryUsageTest, ConfigurationMemoryImpact) {
  testConfigurationMemoryImpact();
}

TEST_F(MemoryUsageTest, MemoryLeaks) {
  testMemoryLeaks();
}

TEST_F(MemoryUsageTest, PeakMemoryUsage) {
  testPeakMemoryUsage();
}

// Scalability Tests
TEST_F(ScalabilityTest, FunctionCountScalability) {
  ScalabilityTestResult Result = testFunctionCountScalability();
  EXPECT_TRUE(Result.PassedScalabilityTest) << "Function count scalability test failed: " << Result.ErrorMessage;
  
  if (!Result.PassedScalabilityTest) {
    std::string Report = generatePerformanceReport(Result);
    std::cout << Report << std::endl;
  }
}

TEST_F(ScalabilityTest, MPICallCountScalability) {
  ScalabilityTestResult Result = testMPICallCountScalability();
  EXPECT_TRUE(Result.PassedScalabilityTest) << "MPI call count scalability test failed: " << Result.ErrorMessage;
}

TEST_F(ScalabilityTest, ComplexityScalability) {
  ScalabilityTestResult Result = testComplexityScalability();
  EXPECT_TRUE(Result.PassedScalabilityTest) << "Complexity scalability test failed: " << Result.ErrorMessage;
}

TEST_F(ScalabilityTest, InstrumentationLevelScalability) {
  ScalabilityTestResult Result = testInstrumentationLevelScalability();
  EXPECT_TRUE(Result.PassedScalabilityTest) << "Instrumentation level scalability test failed: " << Result.ErrorMessage;
}

// Instrumentation Overhead Tests
TEST_F(InstrumentationOverheadTest, PreCallHookOverhead) {
  PerformanceMeasurement Result = testPreCallHookOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 200.0) << "Pre-call hook overhead too high: " << Result.getInstructionOverhead() << "%";
  EXPECT_GT(Result.HooksInserted, 0u) << "No pre-call hooks were inserted";
}

TEST_F(InstrumentationOverheadTest, PostCallHookOverhead) {
  PerformanceMeasurement Result = testPostCallHookOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 200.0) << "Post-call hook overhead too high: " << Result.getInstructionOverhead() << "%";
  EXPECT_GT(Result.HooksInserted, 0u) << "No post-call hooks were inserted";
}

TEST_F(InstrumentationOverheadTest, PerformanceMonitoringOverhead) {
  PerformanceMeasurement Result = testPerformanceMonitoringOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 300.0) << "Performance monitoring overhead too high: " << Result.getInstructionOverhead() << "%";
}

TEST_F(InstrumentationOverheadTest, ErrorCheckingOverhead) {
  PerformanceMeasurement Result = testErrorCheckingOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 250.0) << "Error checking overhead too high: " << Result.getInstructionOverhead() << "%";
}

TEST_F(InstrumentationOverheadTest, DeadlockDetectionOverhead) {
  PerformanceMeasurement Result = testDeadlockDetectionOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 400.0) << "Deadlock detection overhead too high: " << Result.getInstructionOverhead() << "%";
}

TEST_F(InstrumentationOverheadTest, DataRaceDetectionOverhead) {
  PerformanceMeasurement Result = testDataRaceDetectionOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 400.0) << "Data race detection overhead too high: " << Result.getInstructionOverhead() << "%";
}

TEST_F(InstrumentationOverheadTest, CombinedInstrumentationOverhead) {
  PerformanceMeasurement Result = testCombinedInstrumentationOverhead();
  EXPECT_LT(Result.getInstructionOverhead(), 800.0) << "Combined instrumentation overhead too high: " << Result.getInstructionOverhead() << "%";
  EXPECT_GT(Result.HooksInserted, 0u) << "No hooks were inserted with combined instrumentation";
}

TEST_F(InstrumentationOverheadTest, MPIFunctionTypeOverhead) {
  testMPIFunctionTypeOverhead();
}

TEST_F(InstrumentationOverheadTest, SelectiveInstrumentationBenefits) {
  testSelectiveInstrumentationBenefits();
}

// Optimization Effectiveness Tests
TEST_F(OptimizationEffectivenessTest, StaticAnalysisOptimizations) {
  testStaticAnalysisOptimizations();
}

TEST_F(OptimizationEffectivenessTest, SelectiveInstrumentationEffectiveness) {
  testSelectiveInstrumentationEffectiveness();
}

TEST_F(OptimizationEffectivenessTest, OptimizationLevelImpact) {
  testOptimizationLevelImpact();
}

TEST_F(OptimizationEffectivenessTest, ConfigurationOptimizationEffectiveness) {
  testConfigurationOptimizationEffectiveness();
}

TEST_F(OptimizationEffectivenessTest, OptimizedVsUnoptimized) {
  compareOptimizedVsUnoptimized();
}

// Performance Regression Tests
TEST_F(PerformanceRegressionTest, BaselinePerformance) {
  testBaselinePerformance();
}

TEST_F(PerformanceRegressionTest, BenchmarkSuite) {
  testBenchmarkSuite();
}

TEST_F(PerformanceRegressionTest, RegressionDetection) {
  testRegressionDetection();
}

} // namespace llvm
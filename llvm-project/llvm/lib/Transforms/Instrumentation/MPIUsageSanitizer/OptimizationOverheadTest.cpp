//===- OptimizationOverheadTest.cpp - Optimization Overhead Tests ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements comprehensive tests for Task 18.1: Optimize 
// instrumentation overhead. Tests profiling, hot path optimization, and
// performance improvements.
//
//===----------------------------------------------------------------------===//

#include "OptimizationOverheadTest.h"
#include "PerformanceProfiler.h"
#include "PassOptimizer.h"
#include "MPISanitizerPass.h"
#include "PerformanceTests.h"
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

namespace llvm {

//===----------------------------------------------------------------------===//
// OptimizationOverheadTest Implementation
//===----------------------------------------------------------------------===//

void OptimizationOverheadTest::SetUp() {
  Context = std::make_unique<LLVMContext>();
  ConfigManager = std::make_unique<ConfigurationManager>();
  
  // Set up baseline configuration
  BaselineConfig.EnableProfiling = false;
  BaselineConfig.EnableOptimization = false;
  BaselineConfig.InstrumentationMode = InstrumentationMode::Standard;
  
  // Set up optimized configuration
  OptimizedConfig.EnableProfiling = true;
  OptimizedConfig.EnableOptimization = true;
  OptimizedConfig.InstrumentationMode = InstrumentationMode::Selective;
}

void OptimizationOverheadTest::TearDown() {
  ConfigManager.reset();
  Context.reset();
}

void OptimizationOverheadTest::testCallDetectionOptimization() {
  auto TestModule = createOptimizationTestModule("call_detection", 100, 10);
  ASSERT_TRUE(TestModule) << "Failed to create test module for call detection optimization";
  
  // Measure baseline performance
  auto BaselineModule = CloneModule(*TestModule);
  PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
  
  // Measure optimized performance
  auto OptimizedModule = CloneModule(*TestModule);
  PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, OptimizedConfig);
  
  // Validate optimization effectiveness
  EXPECT_LT(OptimizedMeasurement.CompilationTimeUs, BaselineMeasurement.CompilationTimeUs)
    << "Optimized call detection should be faster than baseline";
  
  double SpeedupRatio = static_cast<double>(BaselineMeasurement.CompilationTimeUs) / 
                       OptimizedMeasurement.CompilationTimeUs;
  
  EXPECT_GT(SpeedupRatio, 1.1) << "Expected at least 10% speedup, got " << ((SpeedupRatio - 1.0) * 100.0) << "%";
  
  // Validate correctness is maintained
  EXPECT_EQ(OptimizedMeasurement.MPICallsDetected, BaselineMeasurement.MPICallsDetected)
    << "Optimization should not change the number of MPI calls detected";
  
  LLVM_DEBUG(dbgs() << "Call detection optimization speedup: " << ((SpeedupRatio - 1.0) * 100.0) << "%\n");
}

void OptimizationOverheadTest::testMetadataExtractionOptimization() {
  auto TestModule = createOptimizationTestModule("metadata_extraction", 150, 8);
  ASSERT_TRUE(TestModule) << "Failed to create test module for metadata extraction optimization";
  
  // Measure baseline performance
  auto BaselineModule = CloneModule(*TestModule);
  PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
  
  // Measure optimized performance with caching enabled
  PassConfiguration CachedConfig = OptimizedConfig;
  CachedConfig.EnableCaching = true;
  
  auto OptimizedModule = CloneModule(*TestModule);
  PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, CachedConfig);
  
  // Validate optimization effectiveness
  double SpeedupRatio = static_cast<double>(BaselineMeasurement.CompilationTimeUs) / 
                       OptimizedMeasurement.CompilationTimeUs;
  
  EXPECT_GT(SpeedupRatio, 1.05) << "Expected at least 5% speedup from metadata caching";
  
  // Memory usage should not increase significantly
  double MemoryRatio = static_cast<double>(OptimizedMeasurement.MemoryUsageBytes) / 
                      BaselineMeasurement.MemoryUsageBytes;
  
  EXPECT_LT(MemoryRatio, 1.5) << "Memory usage should not increase by more than 50%";
  
  LLVM_DEBUG(dbgs() << "Metadata extraction optimization speedup: " << ((SpeedupRatio - 1.0) * 100.0) << "%\n");
}

void OptimizationOverheadTest::testHookInsertionOptimization() {
  auto TestModule = createOptimizationTestModule("hook_insertion", 80, 15);
  ASSERT_TRUE(TestModule) << "Failed to create test module for hook insertion optimization";
  
  // Measure baseline performance
  auto BaselineModule = CloneModule(*TestModule);
  PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
  
  // Measure optimized performance with minimal transformation
  PassConfiguration MinimalConfig = OptimizedConfig;
  MinimalConfig.EnableMinimalTransformation = true;
  
  auto OptimizedModule = CloneModule(*TestModule);
  PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, MinimalConfig);
  
  // Validate optimization effectiveness
  double SpeedupRatio = static_cast<double>(BaselineMeasurement.CompilationTimeUs) / 
                       OptimizedMeasurement.CompilationTimeUs;
  
  EXPECT_GT(SpeedupRatio, 1.15) << "Expected at least 15% speedup from hook insertion optimization";
  
  // Instruction overhead should be reduced
  double BaselineOverhead = BaselineMeasurement.getInstructionOverhead();
  double OptimizedOverhead = OptimizedMeasurement.getInstructionOverhead();
  
  EXPECT_LT(OptimizedOverhead, BaselineOverhead * 0.9) 
    << "Optimized version should have at least 10% less instruction overhead";
  
  LLVM_DEBUG(dbgs() << "Hook insertion optimization speedup: " << ((SpeedupRatio - 1.0) * 100.0) << "%\n");
}

void OptimizationOverheadTest::testHotPathOptimization() {
  // Create a module with clear hot paths (functions with many MPI calls)
  auto TestModule = createHotPathTestModule();
  ASSERT_TRUE(TestModule) << "Failed to create hot path test module";
  
  // Measure baseline performance
  auto BaselineModule = CloneModule(*TestModule);
  PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
  
  // Measure hot path optimized performance
  PassConfiguration HotPathConfig = OptimizedConfig;
  HotPathConfig.EnableHotPathOptimization = true;
  HotPathConfig.HotPathThreshold = 0.05; // 5% threshold for aggressive optimization
  
  auto OptimizedModule = CloneModule(*TestModule);
  PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, HotPathConfig);
  
  // Validate hot path optimization effectiveness
  double SpeedupRatio = static_cast<double>(BaselineMeasurement.CompilationTimeUs) / 
                       OptimizedMeasurement.CompilationTimeUs;
  
  EXPECT_GT(SpeedupRatio, 1.2) << "Expected at least 20% speedup from hot path optimization";
  
  // Validate that correctness is maintained
  EXPECT_EQ(OptimizedMeasurement.MPICallsDetected, BaselineMeasurement.MPICallsDetected)
    << "Hot path optimization should not change MPI call detection";
  
  LLVM_DEBUG(dbgs() << "Hot path optimization speedup: " << ((SpeedupRatio - 1.0) * 100.0) << "%\n");
}

void OptimizationOverheadTest::testMemoryOptimization() {
  auto TestModule = createOptimizationTestModule("memory_optimization", 200, 12);
  ASSERT_TRUE(TestModule) << "Failed to create test module for memory optimization";
  
  // Measure baseline memory usage
  auto BaselineModule = CloneModule(*TestModule);
  PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
  
  // Measure optimized memory usage
  PassConfiguration MemoryOptConfig = OptimizedConfig;
  MemoryOptConfig.EnableMemoryOptimization = true;
  MemoryOptConfig.MaxCacheSize = 500; // Smaller cache to save memory
  
  auto OptimizedModule = CloneModule(*TestModule);
  PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, MemoryOptConfig);
  
  // Validate memory optimization effectiveness
  double MemoryRatio = static_cast<double>(OptimizedMeasurement.MemoryUsageBytes) / 
                      BaselineMeasurement.MemoryUsageBytes;
  
  EXPECT_LT(MemoryRatio, 0.9) << "Expected at least 10% memory reduction from memory optimization";
  
  // Performance should not degrade significantly
  double SpeedupRatio = static_cast<double>(BaselineMeasurement.CompilationTimeUs) / 
                       OptimizedMeasurement.CompilationTimeUs;
  
  EXPECT_GT(SpeedupRatio, 0.8) << "Memory optimization should not cause more than 20% performance degradation";
  
  LLVM_DEBUG(dbgs() << "Memory optimization savings: " << ((1.0 - MemoryRatio) * 100.0) << "%\n");
}

void OptimizationOverheadTest::testScalabilityOptimization() {
  std::vector<uint32_t> ModuleSizes = {50, 100, 200, 400, 800};
  std::vector<PerformanceMeasurement> BaselineMeasurements;
  std::vector<PerformanceMeasurement> OptimizedMeasurements;
  
  for (uint32_t Size : ModuleSizes) {
    auto TestModule = createOptimizationTestModule("scalability", Size, 8);
    ASSERT_TRUE(TestModule) << "Failed to create test module of size " << Size;
    
    // Measure baseline
    auto BaselineModule = CloneModule(*TestModule);
    PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
    BaselineMeasurements.push_back(BaselineMeasurement);
    
    // Measure optimized
    auto OptimizedModule = CloneModule(*TestModule);
    PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, OptimizedConfig);
    OptimizedMeasurements.push_back(OptimizedMeasurement);
  }
  
  // Analyze scalability improvements
  for (size_t i = 1; i < ModuleSizes.size(); ++i) {
    double BaselineGrowth = static_cast<double>(BaselineMeasurements[i].CompilationTimeUs) / 
                           BaselineMeasurements[i-1].CompilationTimeUs;
    double OptimizedGrowth = static_cast<double>(OptimizedMeasurements[i].CompilationTimeUs) / 
                            OptimizedMeasurements[i-1].CompilationTimeUs;
    
    // Optimized version should scale better
    EXPECT_LT(OptimizedGrowth, BaselineGrowth * 1.1) 
      << "Optimized version should not scale significantly worse than baseline";
    
    // Overall speedup should be maintained or improved with scale
    double SpeedupRatio = static_cast<double>(BaselineMeasurements[i].CompilationTimeUs) / 
                         OptimizedMeasurements[i].CompilationTimeUs;
    
    EXPECT_GT(SpeedupRatio, 1.0) << "Optimization should provide speedup at all scales";
  }
}

void OptimizationOverheadTest::testProfilingOverhead() {
  auto TestModule = createOptimizationTestModule("profiling_overhead", 100, 10);
  ASSERT_TRUE(TestModule) << "Failed to create test module for profiling overhead test";
  
  // Measure without profiling
  PassConfiguration NoProfiling = BaselineConfig;
  NoProfiling.EnableProfiling = false;
  
  auto NoProfilingModule = CloneModule(*TestModule);
  PerformanceMeasurement NoProfilingMeasurement = measurePassPerformance(*NoProfilingModule, NoProfiling);
  
  // Measure with profiling
  PassConfiguration WithProfiling = BaselineConfig;
  WithProfiling.EnableProfiling = true;
  
  auto ProfilingModule = CloneModule(*TestModule);
  PerformanceMeasurement ProfilingMeasurement = measurePassPerformance(*ProfilingModule, WithProfiling);
  
  // Profiling overhead should be minimal
  double OverheadRatio = static_cast<double>(ProfilingMeasurement.CompilationTimeUs) / 
                        NoProfilingMeasurement.CompilationTimeUs;
  
  EXPECT_LT(OverheadRatio, 1.2) << "Profiling overhead should be less than 20%";
  
  // Memory overhead should also be reasonable
  double MemoryOverheadRatio = static_cast<double>(ProfilingMeasurement.MemoryUsageBytes) / 
                              NoProfilingMeasurement.MemoryUsageBytes;
  
  EXPECT_LT(MemoryOverheadRatio, 1.3) << "Profiling memory overhead should be less than 30%";
  
  LLVM_DEBUG(dbgs() << "Profiling overhead: " << ((OverheadRatio - 1.0) * 100.0) << "%\n");
}

void OptimizationOverheadTest::testCombinedOptimizationEffectiveness() {
  auto TestModule = createOptimizationTestModule("combined_optimization", 150, 12);
  ASSERT_TRUE(TestModule) << "Failed to create test module for combined optimization test";
  
  // Measure baseline performance
  auto BaselineModule = CloneModule(*TestModule);
  PerformanceMeasurement BaselineMeasurement = measurePassPerformance(*BaselineModule, BaselineConfig);
  
  // Measure with all optimizations enabled
  PassConfiguration AllOptimizations = OptimizedConfig;
  AllOptimizations.EnableCallDetectionOptimization = true;
  AllOptimizations.EnableMetadataExtractionOptimization = true;
  AllOptimizations.EnableHookInsertionOptimization = true;
  AllOptimizations.EnableMemoryOptimization = true;
  AllOptimizations.EnableCaching = true;
  AllOptimizations.EnableHotPathOptimization = true;
  
  auto OptimizedModule = CloneModule(*TestModule);
  PerformanceMeasurement OptimizedMeasurement = measurePassPerformance(*OptimizedModule, AllOptimizations);
  
  // Validate combined optimization effectiveness
  double SpeedupRatio = static_cast<double>(BaselineMeasurement.CompilationTimeUs) / 
                       OptimizedMeasurement.CompilationTimeUs;
  
  EXPECT_GT(SpeedupRatio, 1.3) << "Combined optimizations should provide at least 30% speedup";
  
  // Validate instruction overhead reduction
  double BaselineOverhead = BaselineMeasurement.getInstructionOverhead();
  double OptimizedOverhead = OptimizedMeasurement.getInstructionOverhead();
  
  EXPECT_LT(OptimizedOverhead, BaselineOverhead * 0.8) 
    << "Combined optimizations should reduce instruction overhead by at least 20%";
  
  // Validate memory efficiency
  double MemoryRatio = static_cast<double>(OptimizedMeasurement.MemoryUsageBytes) / 
                      BaselineMeasurement.MemoryUsageBytes;
  
  EXPECT_LT(MemoryRatio, 1.2) << "Combined optimizations should not increase memory usage by more than 20%";
  
  LLVM_DEBUG(dbgs() << "Combined optimization speedup: " << ((SpeedupRatio - 1.0) * 100.0) << "%\n");
  LLVM_DEBUG(dbgs() << "Instruction overhead reduction: " << ((1.0 - OptimizedOverhead/BaselineOverhead) * 100.0) << "%\n");
}

std::unique_ptr<Module> OptimizationOverheadTest::createOptimizationTestModule(const std::string& TestType, 
                                                                              uint32_t FunctionCount, 
                                                                              uint32_t MPICallsPerFunction) {
  auto M = std::make_unique<Module>("optimization_test_" + TestType, *Context);
  
  // Create MPI function declarations
  std::vector<Type*> SendArgs = {Type::getInt8PtrTy(*Context), Type::getInt32Ty(*Context),
                                Type::getInt32Ty(*Context), Type::getInt32Ty(*Context),
                                Type::getInt32Ty(*Context), Type::getInt32Ty(*Context)};
  FunctionType* SendFT = FunctionType::get(Type::getInt32Ty(*Context), SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  std::vector<Type*> BcastArgs = {Type::getInt8PtrTy(*Context), Type::getInt32Ty(*Context),
                                 Type::getInt32Ty(*Context), Type::getInt32Ty(*Context),
                                 Type::getInt32Ty(*Context)};
  FunctionType* BcastFT = FunctionType::get(Type::getInt32Ty(*Context), BcastArgs, false);
  Function* MPIBcast = Function::Create(BcastFT, Function::ExternalLinkage, "MPI_Bcast", M.get());
  
  // Create test functions with varying MPI call patterns
  for (uint32_t i = 0; i < FunctionCount; ++i) {
    std::string FuncName = "test_function_" + std::to_string(i);
    
    FunctionType* FT = FunctionType::get(Type::getInt32Ty(*Context), false);
    Function* F = Function::Create(FT, Function::ExternalLinkage, FuncName, M.get());
    
    BasicBlock* EntryBB = BasicBlock::Create(*Context, "entry", F);
    IRBuilder<> Builder(EntryBB);
    
    // Add MPI calls with different patterns based on test type
    if (TestType == "call_detection") {
      // Mix of direct and indirect calls for call detection testing
      for (uint32_t j = 0; j < MPICallsPerFunction; ++j) {
        if (j % 2 == 0) {
          // Direct call
          std::vector<Value*> Args = {
            ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
            ConstantInt::get(Type::getInt32Ty(*Context), 100),
            ConstantInt::get(Type::getInt32Ty(*Context), 0),
            ConstantInt::get(Type::getInt32Ty(*Context), j % 4),
            ConstantInt::get(Type::getInt32Ty(*Context), j),
            ConstantInt::get(Type::getInt32Ty(*Context), 0)
          };
          Builder.CreateCall(MPISend, Args);
        } else {
          // Collective call
          std::vector<Value*> Args = {
            ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
            ConstantInt::get(Type::getInt32Ty(*Context), 50),
            ConstantInt::get(Type::getInt32Ty(*Context), 0),
            ConstantInt::get(Type::getInt32Ty(*Context), 0),
            ConstantInt::get(Type::getInt32Ty(*Context), 0)
          };
          Builder.CreateCall(MPIBcast, Args);
        }
      }
    } else if (TestType == "metadata_extraction") {
      // Complex parameter patterns for metadata extraction testing
      for (uint32_t j = 0; j < MPICallsPerFunction; ++j) {
        // Create varying parameter patterns
        Value* Count = ConstantInt::get(Type::getInt32Ty(*Context), 100 + j * 10);
        Value* Dest = ConstantInt::get(Type::getInt32Ty(*Context), j % 4);
        Value* Tag = ConstantInt::get(Type::getInt32Ty(*Context), j * 2);
        
        std::vector<Value*> Args = {
          ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
          Count, ConstantInt::get(Type::getInt32Ty(*Context), 0),
          Dest, Tag, ConstantInt::get(Type::getInt32Ty(*Context), 0)
        };
        Builder.CreateCall(MPISend, Args);
      }
    } else {
      // Standard MPI call pattern
      for (uint32_t j = 0; j < MPICallsPerFunction; ++j) {
        std::vector<Value*> Args = {
          ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
          ConstantInt::get(Type::getInt32Ty(*Context), 100),
          ConstantInt::get(Type::getInt32Ty(*Context), 0),
          ConstantInt::get(Type::getInt32Ty(*Context), j % 4),
          ConstantInt::get(Type::getInt32Ty(*Context), j),
          ConstantInt::get(Type::getInt32Ty(*Context), 0)
        };
        Builder.CreateCall(MPISend, Args);
      }
    }
    
    Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(*Context), 0));
  }
  
  return M;
}

std::unique_ptr<Module> OptimizationOverheadTest::createHotPathTestModule() {
  auto M = std::make_unique<Module>("hot_path_test", *Context);
  
  // Create MPI function declarations
  std::vector<Type*> SendArgs = {Type::getInt8PtrTy(*Context), Type::getInt32Ty(*Context),
                                Type::getInt32Ty(*Context), Type::getInt32Ty(*Context),
                                Type::getInt32Ty(*Context), Type::getInt32Ty(*Context)};
  FunctionType* SendFT = FunctionType::get(Type::getInt32Ty(*Context), SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  // Create hot path function (many MPI calls)
  FunctionType* HotFT = FunctionType::get(Type::getInt32Ty(*Context), false);
  Function* HotFunc = Function::Create(HotFT, Function::ExternalLinkage, "hot_path_function", M.get());
  
  BasicBlock* HotBB = BasicBlock::Create(*Context, "entry", HotFunc);
  IRBuilder<> HotBuilder(HotBB);
  
  // Add many MPI calls to create a hot path
  for (int i = 0; i < 50; ++i) {
    std::vector<Value*> Args = {
      ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
      ConstantInt::get(Type::getInt32Ty(*Context), 100),
      ConstantInt::get(Type::getInt32Ty(*Context), 0),
      ConstantInt::get(Type::getInt32Ty(*Context), i % 4),
      ConstantInt::get(Type::getInt32Ty(*Context), i),
      ConstantInt::get(Type::getInt32Ty(*Context), 0)
    };
    HotBuilder.CreateCall(MPISend, Args);
  }
  HotBuilder.CreateRet(ConstantInt::get(Type::getInt32Ty(*Context), 0));
  
  // Create cold path functions (few MPI calls)
  for (int f = 0; f < 10; ++f) {
    std::string FuncName = "cold_path_function_" + std::to_string(f);
    Function* ColdFunc = Function::Create(HotFT, Function::ExternalLinkage, FuncName, M.get());
    
    BasicBlock* ColdBB = BasicBlock::Create(*Context, "entry", ColdFunc);
    IRBuilder<> ColdBuilder(ColdBB);
    
    // Add few MPI calls
    for (int i = 0; i < 2; ++i) {
      std::vector<Value*> Args = {
        ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
        ConstantInt::get(Type::getInt32Ty(*Context), 50),
        ConstantInt::get(Type::getInt32Ty(*Context), 0),
        ConstantInt::get(Type::getInt32Ty(*Context), i),
        ConstantInt::get(Type::getInt32Ty(*Context), f * 10 + i),
        ConstantInt::get(Type::getInt32Ty(*Context), 0)
      };
      ColdBuilder.CreateCall(MPISend, Args);
    }
    ColdBuilder.CreateRet(ConstantInt::get(Type::getInt32Ty(*Context), 0));
  }
  
  return M;
}

PerformanceMeasurement OptimizationOverheadTest::measurePassPerformance(Module& M, const PassConfiguration& Config) {
  PerformanceMeasurement Measurement;
  
  // Measure original characteristics
  Measurement.OriginalInstructions = PerformanceTestUtils::countInstructions(M);
  Measurement.OriginalModuleSize = PerformanceTestUtils::measureModuleSize(M);
  Measurement.MPICallsDetected = PerformanceTestUtils::countMPICalls(M);
  
  // Measure memory before pass execution
  uint64_t InitialMemory = sys::Process::GetMallocUsage();
  
  // Run pass and measure performance
  PerformanceTestUtils::HighPrecisionTimer Timer;
  Timer.start();
  
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  
  ConfigManager->initialize(Config);
  MPM.addPass(MPISanitizerPass(Config));
  MPM.run(M, MAM);
  
  Timer.stop();
  
  // Measure results
  Measurement.CompilationTimeUs = Timer.getElapsedMicroseconds();
  Measurement.InstrumentedInstructions = PerformanceTestUtils::countInstructions(M);
  Measurement.InstrumentedModuleSize = PerformanceTestUtils::measureModuleSize(M);
  Measurement.HooksInserted = PerformanceTestUtils::countInstrumentationHooks(M);
  Measurement.MemoryUsageBytes = sys::Process::GetMallocUsage() - InitialMemory;
  
  return Measurement;
}

} // namespace llvm
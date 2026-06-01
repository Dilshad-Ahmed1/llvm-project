//===- OptimizationOverheadTest.h - Optimization Overhead Tests -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares comprehensive tests for Task 18.1: Optimize 
// instrumentation overhead. Tests profiling, hot path optimization, and
// performance improvements.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_OPTIMIZATIONOVERHEADTEST_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_OPTIMIZATIONOVERHEADTEST_H

#include "ConfigurationManager.h"
#include "PerformanceTests.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

namespace llvm {

/// Comprehensive tests for Task 18.1: Optimize instrumentation overhead
///
/// This test suite validates the effectiveness of performance optimizations
/// including profiling, hot path optimization, caching, and memory optimization.
class OptimizationOverheadTest : public ::testing::Test {
protected:
  void SetUp() override;
  void TearDown() override;
  
  /// Test call detection optimization effectiveness
  void testCallDetectionOptimization();
  
  /// Test metadata extraction optimization with caching
  void testMetadataExtractionOptimization();
  
  /// Test hook insertion optimization with minimal transformation
  void testHookInsertionOptimization();
  
  /// Test hot path identification and optimization
  void testHotPathOptimization();
  
  /// Test memory usage optimization
  void testMemoryOptimization();
  
  /// Test scalability improvements from optimization
  void testScalabilityOptimization();
  
  /// Test profiling overhead is minimal
  void testProfilingOverhead();
  
  /// Test combined optimization effectiveness
  void testCombinedOptimizationEffectiveness();

private:
  /// Create test module for optimization testing
  std::unique_ptr<Module> createOptimizationTestModule(const std::string& TestType, 
                                                       uint32_t FunctionCount, 
                                                       uint32_t MPICallsPerFunction);
  
  /// Create module with clear hot paths for hot path optimization testing
  std::unique_ptr<Module> createHotPathTestModule();
  
  /// Measure pass performance with given configuration
  PerformanceMeasurement measurePassPerformance(Module& M, const PassConfiguration& Config);

protected:
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ConfigurationManager> ConfigManager;
  
  /// Test configurations
  PassConfiguration BaselineConfig;
  PassConfiguration OptimizedConfig;
};

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_OPTIMIZATIONOVERHEADTEST_H
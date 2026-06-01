//===- OptimizationEngineTest.cpp - Unit tests for OptimizationEngine ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/OptimizationEngine.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/StaticAnalyzer.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class OptimizationEngineTest : public ::testing::Test {
protected:
  void SetUp() override {
    Context = std::make_unique<LLVMContext>();
    M = std::make_unique<Module>("test", *Context);
    Engine = std::make_unique<OptimizationEngine>();
    Analyzer = std::make_shared<StaticAnalyzer>();
    
    // Initialize with default configuration
    OptimizationConfiguration Config;
    Engine->initialize(Config);
    Engine->setStaticAnalyzer(Analyzer);
  }

  void TearDown() override {
    Engine.reset();
    Analyzer.reset();
    M.reset();
    Context.reset();
  }

  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<Module> M;
  std::unique_ptr<OptimizationEngine> Engine;
  std::shared_ptr<StaticAnalyzer> Analyzer;
};

TEST_F(OptimizationEngineTest, BasicOptimizationDecision) {
  // Create a mock MPI call site for MPI_Init
  CallSite Site(nullptr, "MPI_Init", MPIFunctionType::Environment, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Init";
  
  AnalysisResult Analysis;
  Analysis.IsSafe = true;
  Analysis.HasConstantParameters = true;
  Analysis.CouldCauseDeadlock = false;
  Analysis.CouldCauseDataRace = false;
  Analysis.RecommendedLevel = OptimizationLevel::Maximum;

  OptimizationDecision Decision = Engine->makeDecision(Site, Metadata, Analysis);
  
  EXPECT_TRUE(Decision.ShouldInstrument) << "MPI_Init should be instrumented";
  EXPECT_EQ(Decision.Level, InstrumentationLevel::Minimal) 
    << "Environment functions should get minimal instrumentation";
  EXPECT_TRUE(Decision.EnablePreHooks) << "Pre-hooks should be enabled";
  EXPECT_FALSE(Decision.EnablePostHooks) << "Post-hooks should be disabled for environment functions";
}

TEST_F(OptimizationEngineTest, HighRiskOperationDecision) {
  // Create a mock MPI call site for MPI_Isend (high-risk operation)
  CallSite Site(nullptr, "MPI_Isend", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Isend";
  
  AnalysisResult Analysis;
  Analysis.IsSafe = false;
  Analysis.HasConstantParameters = false;
  Analysis.CouldCauseDeadlock = true;
  Analysis.CouldCauseDataRace = true;
  Analysis.RecommendedLevel = OptimizationLevel::None;

  OptimizationDecision Decision = Engine->makeDecision(Site, Metadata, Analysis);
  
  EXPECT_TRUE(Decision.ShouldInstrument) << "High-risk operations should be instrumented";
  EXPECT_EQ(Decision.Level, InstrumentationLevel::Full) 
    << "High-risk operations should get full instrumentation";
  EXPECT_TRUE(Decision.EnableDeadlockDetection) << "Deadlock detection should be enabled";
  EXPECT_TRUE(Decision.EnableDataRaceDetection) << "Data race detection should be enabled";
}

TEST_F(OptimizationEngineTest, SafeOperationOptimization) {
  // Create a mock MPI call site for a safe operation
  CallSite Site(nullptr, "MPI_Comm_rank", MPIFunctionType::Communicator, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Comm_rank";
  
  AnalysisResult Analysis;
  Analysis.IsSafe = true;
  Analysis.HasConstantParameters = true;
  Analysis.CouldCauseDeadlock = false;
  Analysis.CouldCauseDataRace = false;
  Analysis.RecommendedLevel = OptimizationLevel::Aggressive;

  OptimizationDecision Decision = Engine->makeDecision(Site, Metadata, Analysis);
  
  EXPECT_TRUE(Decision.ShouldInstrument) << "Safe operations should still be instrumented";
  EXPECT_LE(Decision.Level, InstrumentationLevel::Selective) 
    << "Safe operations should get reduced instrumentation";
  EXPECT_FALSE(Decision.EnableErrorChecking) << "Error checking should be disabled for safe operations";
}

TEST_F(OptimizationEngineTest, PerformanceImpactEstimation) {
  // Test performance impact estimation for different function types
  CallSite CollectiveSite(nullptr, "MPI_Barrier", MPIFunctionType::Collective, false);
  CallSite EnvironmentSite(nullptr, "MPI_Init", MPIFunctionType::Environment, false);
  
  MPICallMetadata Metadata;
  AnalysisResult Analysis;
  Analysis.IsInLoop = false;
  Analysis.HasComplexControlFlow = false;
  Analysis.HasConstantParameters = true;

  double CollectiveImpact = Engine->estimatePerformanceImpact(CollectiveSite, Metadata, Analysis);
  double EnvironmentImpact = Engine->estimatePerformanceImpact(EnvironmentSite, Metadata, Analysis);
  
  EXPECT_GT(CollectiveImpact, EnvironmentImpact) 
    << "Collective operations should have higher performance impact than environment operations";
  EXPECT_GE(CollectiveImpact, 0.0) << "Performance impact should be non-negative";
  EXPECT_LE(CollectiveImpact, 1.0) << "Performance impact should not exceed 1.0";
}

TEST_F(OptimizationEngineTest, SafetyBenefitEstimation) {
  // Test safety benefit estimation for different function types
  CallSite WindowSite(nullptr, "MPI_Put", MPIFunctionType::Window, false);
  CallSite InfoSite(nullptr, "MPI_Get_version", MPIFunctionType::Info, false);
  
  MPICallMetadata Metadata;
  AnalysisResult Analysis;
  Analysis.IsSafe = false;
  Analysis.CouldCauseDeadlock = false;
  Analysis.CouldCauseDataRace = true;

  double WindowBenefit = Engine->estimateSafetyBenefit(WindowSite, Metadata, Analysis);
  double InfoBenefit = Engine->estimateSafetyBenefit(InfoSite, Metadata, Analysis);
  
  EXPECT_GT(WindowBenefit, InfoBenefit) 
    << "Window operations should have higher safety benefit than info operations";
  EXPECT_GE(WindowBenefit, 0.0) << "Safety benefit should be non-negative";
  EXPECT_LE(WindowBenefit, 1.0) << "Safety benefit should not exceed 1.0";
}

TEST_F(OptimizationEngineTest, BatchOptimization) {
  // Test batch optimization of multiple call sites
  std::vector<CallSite> Sites = {
    CallSite(nullptr, "MPI_Init", MPIFunctionType::Environment, false),
    CallSite(nullptr, "MPI_Send", MPIFunctionType::PointToPoint, false),
    CallSite(nullptr, "MPI_Barrier", MPIFunctionType::Collective, false)
  };
  
  std::vector<MPICallMetadata> Metadata(3);
  std::vector<AnalysisResult> Analyses(3);
  
  // Set up different analysis results
  Analyses[0].IsSafe = true;  // MPI_Init is safe
  Analyses[1].CouldCauseDeadlock = true;  // MPI_Send can deadlock
  Analyses[2].CouldCauseDeadlock = true;  // MPI_Barrier can deadlock
  
  std::vector<OptimizationDecision> Decisions = Engine->optimizeCallSites(Sites, Metadata, Analyses);
  
  EXPECT_EQ(Decisions.size(), 3) << "Should return decisions for all call sites";
  EXPECT_EQ(Decisions[0].Level, InstrumentationLevel::Minimal) 
    << "Environment function should get minimal instrumentation";
  EXPECT_TRUE(Decisions[1].EnableDeadlockDetection) 
    << "Point-to-point function should enable deadlock detection";
  EXPECT_TRUE(Decisions[2].EnableDeadlockDetection) 
    << "Collective function should enable deadlock detection";
}

TEST_F(OptimizationEngineTest, ConfigurationUpdate) {
  // Test configuration updates
  OptimizationConfiguration NewConfig;
  NewConfig.GlobalLevel = InstrumentationLevel::Full;
  NewConfig.EnableOptimizations = false;
  NewConfig.AggressiveOptimization = false;
  
  Engine->updateConfiguration(NewConfig);
  
  CallSite Site(nullptr, "MPI_Send", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata;
  AnalysisResult Analysis;
  Analysis.IsSafe = true;  // Even though it's safe, optimizations are disabled
  
  OptimizationDecision Decision = Engine->makeDecision(Site, Metadata, Analysis);
  
  EXPECT_EQ(Decision.Level, InstrumentationLevel::Full) 
    << "Should use full instrumentation when optimizations are disabled";
}

TEST_F(OptimizationEngineTest, StatisticsCollection) {
  // Test statistics collection
  Engine->resetStatistics();
  
  CallSite Site(nullptr, "MPI_Send", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata;
  AnalysisResult Analysis;
  
  // Make several decisions
  for (int i = 0; i < 5; ++i) {
    Engine->makeDecision(Site, Metadata, Analysis);
  }
  
  const OptimizationStatistics& Stats = Engine->getStatistics();
  EXPECT_EQ(Stats.TotalCallSites, 5) << "Should track total call sites";
  EXPECT_GT(Stats.InstrumentedCallSites, 0) << "Should track instrumented call sites";
}

TEST_F(OptimizationEngineTest, OptimizationEngineFactory) {
  // Test factory methods
  auto DevEngine = OptimizationEngineFactory::createDevelopmentEngine();
  auto ProdEngine = OptimizationEngineFactory::createProductionEngine();
  auto DebugEngine = OptimizationEngineFactory::createDebuggingEngine();
  
  EXPECT_NE(DevEngine, nullptr) << "Development engine should be created";
  EXPECT_NE(ProdEngine, nullptr) << "Production engine should be created";
  EXPECT_NE(DebugEngine, nullptr) << "Debug engine should be created";
  
  // Test that different engines make different decisions for the same input
  CallSite Site(nullptr, "MPI_Send", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata;
  AnalysisResult Analysis;
  Analysis.IsSafe = true;
  
  InstrumentationLevel DevLevel = DevEngine->getInstrumentationLevel(Site, Metadata, Analysis);
  InstrumentationLevel ProdLevel = ProdEngine->getInstrumentationLevel(Site, Metadata, Analysis);
  
  // Production engine should be more aggressive in optimization
  EXPECT_LE(ProdLevel, DevLevel) 
    << "Production engine should use lower instrumentation level for safe operations";
}

TEST_F(OptimizationEngineTest, OptimizationReport) {
  // Test optimization report generation
  CallSite Site(nullptr, "MPI_Send", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata;
  AnalysisResult Analysis;
  
  // Make some decisions to populate statistics
  Engine->makeDecision(Site, Metadata, Analysis);
  
  std::string Report = Engine->generateOptimizationReport();
  
  EXPECT_FALSE(Report.empty()) << "Report should not be empty";
  EXPECT_NE(Report.find("Total Call Sites"), std::string::npos) 
    << "Report should contain total call sites";
  EXPECT_NE(Report.find("Instrumented"), std::string::npos) 
    << "Report should contain instrumentation statistics";
}

} // namespace
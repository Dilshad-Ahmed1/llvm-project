//===- ConfigurationManagerTest.cpp - ConfigurationManager Unit Tests ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation/MPIUsageSanitizer/ConfigurationManager.h"
#include "llvm/Transforms/Instrumentation/MPIUsageSanitizer/MPICallDetector.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class ConfigurationManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    Options.Level = MPIUsageSanitizerOptions::InstrumentationLevel::Full;
    Options.EnableOptimizations = true;
    Options.EnablePerformanceMonitoring = false;
    Options.EnableDeadlockDetection = true;
    Options.EnableDataRaceDetection = true;
    
    ConfigMgr = std::make_unique<ConfigurationManager>(Options);
    ConfigMgr->initialize();
  }
  
  CallSite createTestCallSite(StringRef FunctionName, MPIFunctionType Type) {
    CallSite Site;
    Site.FunctionName = FunctionName;
    Site.Type = Type;
    Site.CallInst = nullptr; // Not needed for policy tests
    Site.IsIndirect = false;
    return Site;
  }
  
  MPIUsageSanitizerOptions Options;
  std::unique_ptr<ConfigurationManager> ConfigMgr;
};

TEST_F(ConfigurationManagerTest, BasicInstrumentationDecision) {
  // Test basic instrumentation decisions
  CallSite SendSite = createTestCallSite("MPI_Send", MPIFunctionType::PointToPoint);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(SendSite));
  
  CallSite InitSite = createTestCallSite("MPI_Init", MPIFunctionType::Environment);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(InitSite));
}

TEST_F(ConfigurationManagerTest, LightweightModeFiltering) {
  // Set lightweight mode
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Lightweight);
  
  // Point-to-point operations should be instrumented
  CallSite SendSite = createTestCallSite("MPI_Send", MPIFunctionType::PointToPoint);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(SendSite));
  
  // Request operations should be instrumented
  CallSite WaitSite = createTestCallSite("MPI_Wait", MPIFunctionType::Request);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(WaitSite));
  
  // Critical collective operations should be instrumented
  CallSite BcastSite = createTestCallSite("MPI_Bcast", MPIFunctionType::Collective);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(BcastSite));
  
  CallSite BarrierSite = createTestCallSite("MPI_Barrier", MPIFunctionType::Collective);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(BarrierSite));
  
  // Non-critical collective operations should be skipped
  CallSite GatherSite = createTestCallSite("MPI_Gather", MPIFunctionType::Collective);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(GatherSite));
  
  // Environment operations should be limited to Init/Finalize
  CallSite InitSite = createTestCallSite("MPI_Init", MPIFunctionType::Environment);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(InitSite));
  
  CallSite FinalizeSite = createTestCallSite("MPI_Finalize", MPIFunctionType::Environment);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(FinalizeSite));
  
  CallSite InitializedSite = createTestCallSite("MPI_Initialized", MPIFunctionType::Environment);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(InitializedSite));
  
  // Datatype operations should be skipped in lightweight mode
  CallSite TypeSite = createTestCallSite("MPI_Type_create_struct", MPIFunctionType::Datatype);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(TypeSite));
}

TEST_F(ConfigurationManagerTest, FullModeInstrumentation) {
  // Set full mode
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Full);
  
  // All enabled operation types should be instrumented
  CallSite SendSite = createTestCallSite("MPI_Send", MPIFunctionType::PointToPoint);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(SendSite));
  
  CallSite TypeSite = createTestCallSite("MPI_Type_create_struct", MPIFunctionType::Datatype);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(TypeSite));
  
  CallSite InfoSite = createTestCallSite("MPI_Info_create", MPIFunctionType::Info);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(InfoSite));
  
  CallSite WinSite = createTestCallSite("MPI_Win_create", MPIFunctionType::Window);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(WinSite));
}

TEST_F(ConfigurationManagerTest, PerformanceModeFiltering) {
  // Set performance mode
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Performance);
  
  // Enable performance monitoring
  PassConfiguration Config = ConfigMgr->getConfiguration();
  Config.EnablePerformanceMonitoring = true;
  
  // Only collective and blocking point-to-point operations should be instrumented
  CallSite BcastSite = createTestCallSite("MPI_Bcast", MPIFunctionType::Collective);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(BcastSite));
  
  CallSite SendSite = createTestCallSite("MPI_Send", MPIFunctionType::PointToPoint);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(SendSite));
  
  // Non-blocking operations should be skipped
  CallSite IsendSite = createTestCallSite("MPI_Isend", MPIFunctionType::PointToPoint);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(IsendSite));
  
  // Other operation types should be skipped
  CallSite TypeSite = createTestCallSite("MPI_Type_create_struct", MPIFunctionType::Datatype);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(TypeSite));
}

TEST_F(ConfigurationManagerTest, CategoryEnableDisable) {
  // Test enabling/disabling specific MPI operation categories
  
  // Disable environment operations
  ConfigMgr->disableMPIOperationCategory(MPIFunctionType::Environment);
  
  CallSite InitSite = createTestCallSite("MPI_Init", MPIFunctionType::Environment);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(InitSite));
  
  // Point-to-point should still be enabled
  CallSite SendSite = createTestCallSite("MPI_Send", MPIFunctionType::PointToPoint);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(SendSite));
  
  // Re-enable environment operations
  ConfigMgr->enableMPIOperationCategory(MPIFunctionType::Environment);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(InitSite));
}

TEST_F(ConfigurationManagerTest, FunctionSpecificPolicies) {
  // Test function-specific instrumentation policies
  
  InstrumentationPolicy DisabledPolicy;
  DisabledPolicy.EnablePreHooks = false;
  DisabledPolicy.EnablePostHooks = false;
  DisabledPolicy.EnableErrorChecking = false;
  
  // Disable instrumentation for specific function
  ConfigMgr->setFunctionPolicy("MPI_Send", DisabledPolicy);
  
  CallSite SendSite = createTestCallSite("MPI_Send", MPIFunctionType::PointToPoint);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(SendSite));
  
  // Other point-to-point functions should still be enabled
  CallSite RecvSite = createTestCallSite("MPI_Recv", MPIFunctionType::PointToPoint);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(RecvSite));
}

TEST_F(ConfigurationManagerTest, TypeSpecificPolicies) {
  // Test type-specific instrumentation policies
  
  InstrumentationPolicy LimitedPolicy;
  LimitedPolicy.EnablePreHooks = true;
  LimitedPolicy.EnablePostHooks = false;
  LimitedPolicy.EnableErrorChecking = true;
  LimitedPolicy.EnablePerformanceHooks = false;
  
  // Set limited policy for datatype operations
  ConfigMgr->setFunctionTypePolicy(MPIFunctionType::Datatype, LimitedPolicy);
  
  CallSite TypeSite = createTestCallSite("MPI_Type_create_struct", MPIFunctionType::Datatype);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(TypeSite));
  
  // Check that the policy is correctly applied
  InstrumentationPolicy RetrievedPolicy = ConfigMgr->getInstrumentationPolicy(MPIFunctionType::Datatype);
  EXPECT_TRUE(RetrievedPolicy.EnablePreHooks);
  EXPECT_FALSE(RetrievedPolicy.EnablePostHooks);
  EXPECT_TRUE(RetrievedPolicy.EnableErrorChecking);
  EXPECT_FALSE(RetrievedPolicy.EnablePerformanceHooks);
}

TEST_F(ConfigurationManagerTest, ModeQueries) {
  // Test mode query methods
  
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Lightweight);
  EXPECT_TRUE(ConfigMgr->isLightweightMode());
  EXPECT_FALSE(ConfigMgr->isFullMode());
  EXPECT_FALSE(ConfigMgr->isPerformanceMode());
  
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Full);
  EXPECT_FALSE(ConfigMgr->isLightweightMode());
  EXPECT_TRUE(ConfigMgr->isFullMode());
  EXPECT_FALSE(ConfigMgr->isPerformanceMode());
  
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Performance);
  EXPECT_FALSE(ConfigMgr->isLightweightMode());
  EXPECT_FALSE(ConfigMgr->isFullMode());
  EXPECT_TRUE(ConfigMgr->isPerformanceMode());
}

TEST_F(ConfigurationManagerTest, CategoryListing) {
  // Test category listing methods
  
  std::set<MPIFunctionType> EnabledCategories = ConfigMgr->getEnabledCategories();
  EXPECT_GT(EnabledCategories.size(), 0);
  EXPECT_TRUE(EnabledCategories.count(MPIFunctionType::PointToPoint));
  EXPECT_TRUE(EnabledCategories.count(MPIFunctionType::Collective));
  
  // Disable a category and check disabled list
  ConfigMgr->disableMPIOperationCategory(MPIFunctionType::Info);
  std::set<MPIFunctionType> DisabledCategories = ConfigMgr->getDisabledCategories();
  EXPECT_TRUE(DisabledCategories.count(MPIFunctionType::Info));
}

} // anonymous namespace
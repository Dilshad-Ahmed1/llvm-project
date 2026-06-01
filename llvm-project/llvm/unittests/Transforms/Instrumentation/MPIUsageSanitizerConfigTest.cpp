//===- MPIUsageSanitizerConfigTest.cpp - ConfigurationManager Tests -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for the ConfigurationManager class
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation/MPIUsageSanitizer.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/ConfigurationManager.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPICallDetector.h"
#include "gtest/gtest.h"
#include <memory>

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
  }

  MPIUsageSanitizerOptions Options;
  std::unique_ptr<ConfigurationManager> ConfigMgr;
};

TEST_F(ConfigurationManagerTest, BasicInitialization) {
  EXPECT_TRUE(ConfigMgr->initialize());
  EXPECT_TRUE(ConfigMgr->isFullMode());
  EXPECT_FALSE(ConfigMgr->isLightweightMode());
  EXPECT_FALSE(ConfigMgr->isPerformanceMode());
}

TEST_F(ConfigurationManagerTest, InstrumentationLevelSettings) {
  EXPECT_TRUE(ConfigMgr->initialize());
  
  // Test full mode
  EXPECT_TRUE(ConfigMgr->shouldInstrument(MPIFunctionType::PointToPoint));
  EXPECT_TRUE(ConfigMgr->shouldInstrument(MPIFunctionType::Collective));
  EXPECT_TRUE(ConfigMgr->shouldInstrument(MPIFunctionType::Environment));
  
  // Test lightweight mode
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Lightweight);
  EXPECT_TRUE(ConfigMgr->isLightweightMode());
  EXPECT_TRUE(ConfigMgr->shouldInstrument(MPIFunctionType::PointToPoint));
  EXPECT_TRUE(ConfigMgr->shouldInstrument(MPIFunctionType::Request));
  
  // Test performance mode
  ConfigMgr->setInstrumentationMode(MPIUsageSanitizerOptions::InstrumentationLevel::Performance);
  EXPECT_TRUE(ConfigMgr->isPerformanceMode());
}

TEST_F(ConfigurationManagerTest, MPIOperationCategoryControl) {
  EXPECT_TRUE(ConfigMgr->initialize());
  
  // Initially all categories should be enabled
  auto EnabledCategories = ConfigMgr->getEnabledCategories();
  EXPECT_TRUE(EnabledCategories.find(MPIFunctionType::PointToPoint) != EnabledCategories.end());
  EXPECT_TRUE(EnabledCategories.find(MPIFunctionType::Collective) != EnabledCategories.end());
  
  // Disable a category
  ConfigMgr->disableMPIOperationCategory(MPIFunctionType::Info);
  EXPECT_FALSE(ConfigMgr->shouldInstrument(MPIFunctionType::Info));
  
  // Re-enable it
  ConfigMgr->enableMPIOperationCategory(MPIFunctionType::Info);
  EXPECT_TRUE(ConfigMgr->shouldInstrument(MPIFunctionType::Info));
}

TEST_F(ConfigurationManagerTest, FunctionSpecificPolicies) {
  EXPECT_TRUE(ConfigMgr->initialize());
  
  // Test function-specific instrumentation
  EXPECT_TRUE(ConfigMgr->shouldInstrument("MPI_Send"));
  EXPECT_TRUE(ConfigMgr->shouldInstrument("MPI_Recv"));
  EXPECT_TRUE(ConfigMgr->shouldInstrument("MPI_Bcast"));
  
  // Set a custom policy for a specific function
  InstrumentationPolicy CustomPolicy;
  CustomPolicy.EnablePreHooks = false;
  CustomPolicy.EnablePostHooks = true;
  CustomPolicy.EnableErrorChecking = true;
  
  ConfigMgr->setFunctionPolicy("MPI_Send", CustomPolicy);
  
  InstrumentationPolicy RetrievedPolicy = ConfigMgr->getInstrumentationPolicy("MPI_Send");
  EXPECT_FALSE(RetrievedPolicy.EnablePreHooks);
  EXPECT_TRUE(RetrievedPolicy.EnablePostHooks);
  EXPECT_TRUE(RetrievedPolicy.EnableErrorChecking);
}

TEST_F(ConfigurationManagerTest, InstrumentationPolicyByType) {
  EXPECT_TRUE(ConfigMgr->initialize());
  
  // Test type-based policies
  InstrumentationPolicy CollectivePolicy;
  CollectivePolicy.EnablePerformanceHooks = true;
  CollectivePolicy.OptimizationLevel = 2;
  
  ConfigMgr->setFunctionTypePolicy(MPIFunctionType::Collective, CollectivePolicy);
  
  InstrumentationPolicy RetrievedPolicy = ConfigMgr->getInstrumentationPolicy(MPIFunctionType::Collective);
  EXPECT_TRUE(RetrievedPolicy.EnablePerformanceHooks);
  EXPECT_EQ(2, RetrievedPolicy.OptimizationLevel);
}

TEST_F(ConfigurationManagerTest, ConfigurationFileSupport) {
  // Create a temporary config file
  std::string ConfigContent = R"(
[global]
instrumentation_level=lightweight
enable_optimizations=true
enable_performance_monitoring=true

[policy.type.collective]
enable_performance_hooks=true
optimization_level=1

[policy.function.MPI_Send]
enable_pre_hooks=false
enable_post_hooks=true
)";
  
  // Write to temporary file
  std::string TempFile = "/tmp/mpi_sanitizer_test.conf";
  std::ofstream File(TempFile);
  File << ConfigContent;
  File.close();
  
  // Test loading configuration
  EXPECT_TRUE(ConfigMgr->loadFromFile(TempFile));
  
  // Verify configuration was loaded
  const PassConfiguration& Config = ConfigMgr->getConfiguration();
  EXPECT_EQ(MPIUsageSanitizerOptions::InstrumentationLevel::Lightweight, Config.Level);
  EXPECT_TRUE(Config.EnableOptimizations);
  EXPECT_TRUE(Config.EnablePerformanceMonitoring);
  
  // Clean up
  std::remove(TempFile.c_str());
}

} // namespace
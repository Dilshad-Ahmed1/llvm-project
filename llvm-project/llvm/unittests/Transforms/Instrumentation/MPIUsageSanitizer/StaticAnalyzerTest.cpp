//===- StaticAnalyzerTest.cpp - Unit tests for MPI Static Analyzer -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/StaticAnalyzer.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPICallDetector.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MetadataExtractor.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class StaticAnalyzerTest : public ::testing::Test {
protected:
  void SetUp() override {
    Context = std::make_unique<LLVMContext>();
    M = std::make_unique<Module>("test", *Context);
    Analyzer = std::make_unique<StaticAnalyzer>();
  }

  void TearDown() override {
    Analyzer.reset();
    M.reset();
    Context.reset();
  }

  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<Module> M;
  std::unique_ptr<StaticAnalyzer> Analyzer;
};

TEST_F(StaticAnalyzerTest, BasicSafetyAnalysis) {
  // Create a simple function
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  IRBuilder<> Builder(BB);
  Builder.CreateRetVoid();

  // Initialize analyzer
  Analyzer->initialize(*F);

  // Create a mock MPI call site for MPI_Init
  CallSite Site(nullptr, "MPI_Init", MPIFunctionType::Environment, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Init";
  Metadata.FunctionType = MPIFunctionType::Environment;

  // Test safety analysis
  bool isSafe = Analyzer->isProvablySafe(Site, Metadata);
  EXPECT_TRUE(isSafe) << "MPI_Init should be considered safe";

  bool hasDeadlockRisk = Analyzer->couldCauseDeadlock(Site, Metadata);
  EXPECT_FALSE(hasDeadlockRisk) << "MPI_Init should not have deadlock risk";

  bool hasDataRaceRisk = Analyzer->hasDataRaceRisk(Site, Metadata);
  EXPECT_FALSE(hasDataRaceRisk) << "MPI_Init should not have data race risk";
}

TEST_F(StaticAnalyzerTest, CollectiveDeadlockAnalysis) {
  // Create a simple function
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  IRBuilder<> Builder(BB);
  Builder.CreateRetVoid();

  // Initialize analyzer
  Analyzer->initialize(*F);

  // Create a mock MPI call site for MPI_Barrier
  CallSite Site(nullptr, "MPI_Barrier", MPIFunctionType::Collective, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Barrier";
  Metadata.FunctionType = MPIFunctionType::Collective;

  // Test deadlock analysis
  bool hasDeadlockRisk = Analyzer->couldCauseDeadlock(Site, Metadata);
  // MPI_Barrier can have deadlock risk if not called by all processes
  EXPECT_TRUE(hasDeadlockRisk) << "MPI_Barrier should have potential deadlock risk";
}

TEST_F(StaticAnalyzerTest, NonBlockingDataRaceAnalysis) {
  // Create a simple function
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  IRBuilder<> Builder(BB);
  Builder.CreateRetVoid();

  // Initialize analyzer
  Analyzer->initialize(*F);

  // Create a mock MPI call site for MPI_Isend
  CallSite Site(nullptr, "MPI_Isend", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Isend";
  Metadata.FunctionType = MPIFunctionType::PointToPoint;

  // Test data race analysis
  bool hasDataRaceRisk = Analyzer->hasDataRaceRisk(Site, Metadata);
  EXPECT_TRUE(hasDataRaceRisk) << "MPI_Isend should have potential data race risk";
}

TEST_F(StaticAnalyzerTest, OneSidedDataRaceAnalysis) {
  // Create a simple function
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  IRBuilder<> Builder(BB);
  Builder.CreateRetVoid();

  // Initialize analyzer
  Analyzer->initialize(*F);

  // Create a mock MPI call site for MPI_Put
  CallSite Site(nullptr, "MPI_Put", MPIFunctionType::Window, false);
  MPICallMetadata Metadata;
  Metadata.FunctionName = "MPI_Put";
  Metadata.FunctionType = MPIFunctionType::Window;

  // Test data race analysis
  bool hasDataRaceRisk = Analyzer->hasDataRaceRisk(Site, Metadata);
  EXPECT_TRUE(hasDataRaceRisk) << "MPI_Put should have potential data race risk";
}

TEST_F(StaticAnalyzerTest, OptimizationLevelRecommendation) {
  // Create a simple function
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  IRBuilder<> Builder(BB);
  Builder.CreateRetVoid();

  // Initialize analyzer
  Analyzer->initialize(*F);

  // Test environment function optimization
  CallSite EnvSite(nullptr, "MPI_Init", MPIFunctionType::Environment, false);
  MPICallMetadata EnvMetadata;
  EnvMetadata.FunctionName = "MPI_Init";
  EnvMetadata.FunctionType = MPIFunctionType::Environment;

  OptimizationLevel EnvLevel = Analyzer->getRecommendedOptimizationLevel(EnvSite, EnvMetadata);
  EXPECT_EQ(EnvLevel, OptimizationLevel::Maximum) << "Environment functions should get maximum optimization";

  // Test risky function optimization
  CallSite RiskySite(nullptr, "MPI_Isend", MPIFunctionType::PointToPoint, false);
  MPICallMetadata RiskyMetadata;
  RiskyMetadata.FunctionName = "MPI_Isend";
  RiskyMetadata.FunctionType = MPIFunctionType::PointToPoint;

  OptimizationLevel RiskyLevel = Analyzer->getRecommendedOptimizationLevel(RiskySite, RiskyMetadata);
  EXPECT_EQ(RiskyLevel, OptimizationLevel::Minimal) << "Risky functions should get minimal optimization";
}

} // namespace
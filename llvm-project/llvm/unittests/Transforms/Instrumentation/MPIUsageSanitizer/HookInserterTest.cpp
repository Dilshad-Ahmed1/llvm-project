//===- HookInserterTest.cpp - Unit tests for HookInserter ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/HookInserter.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/RuntimeInterface.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class HookInserterTest : public ::testing::Test {
protected:
  void SetUp() override {
    M = std::make_unique<Module>("test", Context);
  }

  LLVMContext Context;
  std::unique_ptr<Module> M;
};

TEST_F(HookInserterTest, BasicConfiguration) {
  HookConfiguration Config;
  Config.EnablePreHooks = true;
  Config.EnablePostHooks = true;
  Config.EnablePerformanceHooks = false;
  Config.Level = InstrumentationLevel::Full;
  
  HookInserter Inserter(Config);
  
  EXPECT_EQ(Config.EnablePreHooks, Inserter.getConfiguration().EnablePreHooks);
  EXPECT_EQ(Config.EnablePostHooks, Inserter.getConfiguration().EnablePostHooks);
  EXPECT_EQ(Config.EnablePerformanceHooks, Inserter.getConfiguration().EnablePerformanceHooks);
  EXPECT_EQ(Config.Level, Inserter.getConfiguration().Level);
}

TEST_F(HookInserterTest, HookDeclarationCreation) {
  HookConfiguration Config;
  Config.EnablePreHooks = true;
  Config.EnablePostHooks = true;
  Config.EnablePerformanceHooks = true;
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  
  // Create hook declarations
  Inserter.createHookDeclarations(*M);
  
  // Verify pre-call hook declaration
  Function* PreHook = M->getFunction(RuntimeInterface::getPreHookName());
  ASSERT_NE(PreHook, nullptr);
  EXPECT_EQ(PreHook->getName(), RuntimeInterface::getPreHookName());
  
  // Verify post-call hook declaration
  Function* PostHook = M->getFunction(RuntimeInterface::getPostHookName());
  ASSERT_NE(PostHook, nullptr);
  EXPECT_EQ(PostHook->getName(), RuntimeInterface::getPostHookName());
  
  // Verify performance hook declarations
  Function* PerfBegin = M->getFunction(RuntimeInterface::getPerformanceBeginHookName());
  ASSERT_NE(PerfBegin, nullptr);
  EXPECT_EQ(PerfBegin->getName(), RuntimeInterface::getPerformanceBeginHookName());
  
  Function* PerfEnd = M->getFunction(RuntimeInterface::getPerformanceEndHookName());
  ASSERT_NE(PerfEnd, nullptr);
  EXPECT_EQ(PerfEnd->getName(), RuntimeInterface::getPerformanceEndHookName());
}

TEST_F(HookInserterTest, RuntimeInterfaceTypes) {
  // Test pre-hook type
  FunctionType* PreHookType = RuntimeInterface::getPreHookType(Context);
  ASSERT_NE(PreHookType, nullptr);
  EXPECT_TRUE(PreHookType->getReturnType()->isVoidTy());
  EXPECT_EQ(PreHookType->getNumParams(), 4U);
  
  // Test post-hook type
  FunctionType* PostHookType = RuntimeInterface::getPostHookType(Context);
  ASSERT_NE(PostHookType, nullptr);
  EXPECT_TRUE(PostHookType->getReturnType()->isVoidTy());
  EXPECT_EQ(PostHookType->getNumParams(), 4U);
  
  // Test performance hook types
  FunctionType* PerfBeginType = RuntimeInterface::getPerformanceBeginHookType(Context);
  ASSERT_NE(PerfBeginType, nullptr);
  EXPECT_TRUE(PerfBeginType->getReturnType()->isVoidTy());
  EXPECT_EQ(PerfBeginType->getNumParams(), 2U);
  
  FunctionType* PerfEndType = RuntimeInterface::getPerformanceEndHookType(Context);
  ASSERT_NE(PerfEndType, nullptr);
  EXPECT_TRUE(PerfEndType->getReturnType()->isVoidTy());
  EXPECT_EQ(PerfEndType->getNumParams(), 2U);
}

TEST_F(HookInserterTest, HookSignatureValidation) {
  // Create a function with correct signature
  FunctionType* CorrectType = RuntimeInterface::getPreHookType(Context);
  Function* CorrectFunc = Function::Create(CorrectType, Function::ExternalLinkage, "test_func", *M);
  
  // Validate correct signature
  EXPECT_TRUE(RuntimeInterface::validateHookSignature(CorrectFunc, CorrectType));
  
  // Create a function with incorrect signature
  FunctionType* IncorrectType = FunctionType::get(Type::getVoidTy(Context), {Type::getInt32Ty(Context)}, false);
  Function* IncorrectFunc = Function::Create(IncorrectType, Function::ExternalLinkage, "test_func2", *M);
  
  // Validate incorrect signature
  EXPECT_FALSE(RuntimeInterface::validateHookSignature(IncorrectFunc, CorrectType));
}

} // namespace
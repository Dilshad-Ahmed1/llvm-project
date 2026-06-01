//===- RuntimeInterfacePerformanceTest.cpp - Performance Interface Tests -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for performance monitoring runtime interface extensions.
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/RuntimeInterface.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class RuntimeInterfacePerformanceTest : public ::testing::Test {
protected:
  void SetUp() override {
    Context = std::make_unique<LLVMContext>();
  }
  
  std::unique_ptr<LLVMContext> Context;
};

TEST_F(RuntimeInterfacePerformanceTest, CommunicationVolumeHookType) {
  FunctionType* HookType = RuntimeInterface::getCommunicationVolumeHookType(*Context);
  
  ASSERT_NE(HookType, nullptr);
  
  // Verify return type is void
  EXPECT_TRUE(HookType->getReturnType()->isVoidTy());
  
  // Verify parameter count: name, volume, pattern
  EXPECT_EQ(HookType->getNumParams(), 3);
  
  // Verify parameter types
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
  Type* SizeTy = Type::getInt64Ty(*Context);
  
  EXPECT_EQ(HookType->getParamType(0), CharPtrTy); // name
  EXPECT_EQ(HookType->getParamType(1), SizeTy);    // volume
  EXPECT_EQ(HookType->getParamType(2), CharPtrTy); // pattern
  
  // Verify not variadic
  EXPECT_FALSE(HookType->isVarArg());
}

TEST_F(RuntimeInterfacePerformanceTest, CommunicationPatternHookType) {
  FunctionType* HookType = RuntimeInterface::getCommunicationPatternHookType(*Context);
  
  ASSERT_NE(HookType, nullptr);
  
  // Verify return type is void
  EXPECT_TRUE(HookType->getReturnType()->isVoidTy());
  
  // Verify parameter count: name, src, dest, tag, pattern_type
  EXPECT_EQ(HookType->getNumParams(), 5);
  
  // Verify parameter types
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
  Type* IntTy = Type::getInt32Ty(*Context);
  
  EXPECT_EQ(HookType->getParamType(0), CharPtrTy); // name
  EXPECT_EQ(HookType->getParamType(1), IntTy);     // src
  EXPECT_EQ(HookType->getParamType(2), IntTy);     // dest
  EXPECT_EQ(HookType->getParamType(3), IntTy);     // tag
  EXPECT_EQ(HookType->getParamType(4), CharPtrTy); // pattern_type
  
  // Verify not variadic
  EXPECT_FALSE(HookType->isVarArg());
}

TEST_F(RuntimeInterfacePerformanceTest, CollectiveTimingHookType) {
  FunctionType* HookType = RuntimeInterface::getCollectiveTimingHookType(*Context);
  
  ASSERT_NE(HookType, nullptr);
  
  // Verify return type is void
  EXPECT_TRUE(HookType->getReturnType()->isVoidTy());
  
  // Verify parameter count: name, comm_size, timing_data
  EXPECT_EQ(HookType->getNumParams(), 3);
  
  // Verify parameter types
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
  Type* IntTy = Type::getInt32Ty(*Context);
  Type* DoublePtrTy = PointerType::get(Type::getDoubleTy(*Context), 0);
  
  EXPECT_EQ(HookType->getParamType(0), CharPtrTy);    // name
  EXPECT_EQ(HookType->getParamType(1), IntTy);        // comm_size
  EXPECT_EQ(HookType->getParamType(2), DoublePtrTy);  // timing_data
  
  // Verify not variadic
  EXPECT_FALSE(HookType->isVarArg());
}

TEST_F(RuntimeInterfacePerformanceTest, SynchronizationHookType) {
  FunctionType* HookType = RuntimeInterface::getSynchronizationHookType(*Context);
  
  ASSERT_NE(HookType, nullptr);
  
  // Verify return type is void
  EXPECT_TRUE(HookType->getReturnType()->isVoidTy());
  
  // Verify parameter count: name, sync_type, location
  EXPECT_EQ(HookType->getNumParams(), 3);
  
  // Verify parameter types
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
  Type* IntTy = Type::getInt32Ty(*Context);
  
  EXPECT_EQ(HookType->getParamType(0), CharPtrTy); // name
  EXPECT_EQ(HookType->getParamType(1), IntTy);     // sync_type
  EXPECT_EQ(HookType->getParamType(2), CharPtrTy); // location
  
  // Verify not variadic
  EXPECT_FALSE(HookType->isVarArg());
}

TEST_F(RuntimeInterfacePerformanceTest, HookFunctionNames) {
  // Verify hook function names are correct
  EXPECT_STREQ(RuntimeInterface::getCommunicationVolumeHookName(), 
               "__mpi_sanitizer_comm_volume");
  EXPECT_STREQ(RuntimeInterface::getCommunicationPatternHookName(), 
               "__mpi_sanitizer_comm_pattern");
  EXPECT_STREQ(RuntimeInterface::getCollectiveTimingHookName(), 
               "__mpi_sanitizer_collective_timing");
  EXPECT_STREQ(RuntimeInterface::getSynchronizationHookName(), 
               "__mpi_sanitizer_sync_point");
}

TEST_F(RuntimeInterfacePerformanceTest, HookSignatureValidation) {
  // Create a module and function with correct signature
  Module M("test", *Context);
  
  // Test communication volume hook validation
  FunctionType* CommVolumeType = RuntimeInterface::getCommunicationVolumeHookType(*Context);
  Function* CommVolumeFunc = Function::Create(CommVolumeType, Function::ExternalLinkage, 
                                             RuntimeInterface::getCommunicationVolumeHookName(), M);
  
  EXPECT_TRUE(RuntimeInterface::validateHookSignature(CommVolumeFunc, CommVolumeType));
  
  // Test communication pattern hook validation
  FunctionType* CommPatternType = RuntimeInterface::getCommunicationPatternHookType(*Context);
  Function* CommPatternFunc = Function::Create(CommPatternType, Function::ExternalLinkage, 
                                              RuntimeInterface::getCommunicationPatternHookName(), M);
  
  EXPECT_TRUE(RuntimeInterface::validateHookSignature(CommPatternFunc, CommPatternType));
  
  // Test collective timing hook validation
  FunctionType* CollectiveTimingType = RuntimeInterface::getCollectiveTimingHookType(*Context);
  Function* CollectiveTimingFunc = Function::Create(CollectiveTimingType, Function::ExternalLinkage, 
                                                   RuntimeInterface::getCollectiveTimingHookName(), M);
  
  EXPECT_TRUE(RuntimeInterface::validateHookSignature(CollectiveTimingFunc, CollectiveTimingType));
  
  // Test synchronization hook validation
  FunctionType* SyncType = RuntimeInterface::getSynchronizationHookType(*Context);
  Function* SyncFunc = Function::Create(SyncType, Function::ExternalLinkage, 
                                       RuntimeInterface::getSynchronizationHookName(), M);
  
  EXPECT_TRUE(RuntimeInterface::validateHookSignature(SyncFunc, SyncType));
}

TEST_F(RuntimeInterfacePerformanceTest, InvalidHookSignatureValidation) {
  // Create a module and function with incorrect signature
  Module M("test", *Context);
  
  // Create function with wrong signature (wrong parameter count)
  Type* VoidTy = Type::getVoidTy(*Context);
  Type* IntTy = Type::getInt32Ty(*Context);
  FunctionType* WrongType = FunctionType::get(VoidTy, {IntTy}, false);
  
  Function* WrongFunc = Function::Create(WrongType, Function::ExternalLinkage, "wrong_func", M);
  
  // Test against correct signature - should fail
  FunctionType* CorrectType = RuntimeInterface::getCommunicationVolumeHookType(*Context);
  EXPECT_FALSE(RuntimeInterface::validateHookSignature(WrongFunc, CorrectType));
}

TEST_F(RuntimeInterfacePerformanceTest, NullPointerValidation) {
  // Test validation with null pointers
  FunctionType* ValidType = RuntimeInterface::getCommunicationVolumeHookType(*Context);
  
  EXPECT_FALSE(RuntimeInterface::validateHookSignature(nullptr, ValidType));
  EXPECT_FALSE(RuntimeInterface::validateHookSignature(nullptr, nullptr));
}

TEST_F(RuntimeInterfacePerformanceTest, ExistingHookTypesStillWork) {
  // Verify that existing hook types still work correctly
  FunctionType* PreHookType = RuntimeInterface::getPreHookType(*Context);
  FunctionType* PostHookType = RuntimeInterface::getPostHookType(*Context);
  FunctionType* PerfBeginType = RuntimeInterface::getPerformanceBeginHookType(*Context);
  FunctionType* PerfEndType = RuntimeInterface::getPerformanceEndHookType(*Context);
  
  ASSERT_NE(PreHookType, nullptr);
  ASSERT_NE(PostHookType, nullptr);
  ASSERT_NE(PerfBeginType, nullptr);
  ASSERT_NE(PerfEndType, nullptr);
  
  // Verify existing hook names
  EXPECT_STREQ(RuntimeInterface::getPreHookName(), "__mpi_sanitizer_pre_call");
  EXPECT_STREQ(RuntimeInterface::getPostHookName(), "__mpi_sanitizer_post_call");
  EXPECT_STREQ(RuntimeInterface::getPerformanceBeginHookName(), "__mpi_sanitizer_performance_begin");
  EXPECT_STREQ(RuntimeInterface::getPerformanceEndHookName(), "__mpi_sanitizer_performance_end");
}

} // anonymous namespace
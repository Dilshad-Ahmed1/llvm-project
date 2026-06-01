//===- HookInserterPerformanceTest.cpp - Performance Hook Tests ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Unit tests for performance monitoring hook insertion functionality.
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/HookInserter.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/RuntimeInterface.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MetadataExtractor.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class HookInserterPerformanceTest : public ::testing::Test {
protected:
  void SetUp() override {
    Context = std::make_unique<LLVMContext>();
    M = std::make_unique<Module>("test", *Context);
    
    // Create a test function
    FunctionType* FuncTy = FunctionType::get(Type::getVoidTy(*Context), false);
    TestFunc = Function::Create(FuncTy, Function::ExternalLinkage, "test_func", *M);
    BasicBlock* BB = BasicBlock::Create(*Context, "entry", TestFunc);
    Builder = std::make_unique<IRBuilder<>>(BB);
    
    // Create MPI function declaration for testing
    createMPIFunctionDeclarations();
  }
  
  void TearDown() override {
    // Verify the module is valid after modifications
    std::string ErrorStr;
    raw_string_ostream ErrorOS(ErrorStr);
    if (verifyModule(*M, &ErrorOS)) {
      FAIL() << "Module verification failed: " << ErrorStr;
    }
  }
  
  void createMPIFunctionDeclarations() {
    // Create MPI_Send declaration
    Type* VoidPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
    Type* IntTy = Type::getInt32Ty(*Context);
    
    FunctionType* SendTy = FunctionType::get(IntTy, {
      VoidPtrTy,  // buf
      IntTy,      // count
      IntTy,      // datatype (simplified as int)
      IntTy,      // dest
      IntTy,      // tag
      VoidPtrTy   // comm (simplified as void*)
    }, false);
    
    MPISend = Function::Create(SendTy, Function::ExternalLinkage, "MPI_Send", *M);
    
    // Create MPI_Bcast declaration
    FunctionType* BcastTy = FunctionType::get(IntTy, {
      VoidPtrTy,  // buffer
      IntTy,      // count
      IntTy,      // datatype
      IntTy,      // root
      VoidPtrTy   // comm
    }, false);
    
    MPIBcast = Function::Create(BcastTy, Function::ExternalLinkage, "MPI_Bcast", *M);
  }
  
  CallSite createMPISendCall() {
    Type* VoidPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
    Type* IntTy = Type::getInt32Ty(*Context);
    
    // Create arguments
    Value* Buffer = ConstantPointerNull::get(cast<PointerType>(VoidPtrTy));
    Value* Count = ConstantInt::get(IntTy, 100);
    Value* Datatype = ConstantInt::get(IntTy, 1);
    Value* Dest = ConstantInt::get(IntTy, 1);
    Value* Tag = ConstantInt::get(IntTy, 0);
    Value* Comm = ConstantPointerNull::get(cast<PointerType>(VoidPtrTy));
    
    CallInst* Call = Builder->CreateCall(MPISend, {Buffer, Count, Datatype, Dest, Tag, Comm});
    
    CallSite Site;
    Site.CallInst = Call;
    Site.FunctionName = "MPI_Send";
    Site.Type = MPIFunctionType::PointToPoint;
    Site.IsIndirect = false;
    
    return Site;
  }
  
  CallSite createMPIBcastCall() {
    Type* VoidPtrTy = PointerType::get(Type::getInt8Ty(*Context), 0);
    Type* IntTy = Type::getInt32Ty(*Context);
    
    // Create arguments
    Value* Buffer = ConstantPointerNull::get(cast<PointerType>(VoidPtrTy));
    Value* Count = ConstantInt::get(IntTy, 1000);
    Value* Datatype = ConstantInt::get(IntTy, 1);
    Value* Root = ConstantInt::get(IntTy, 0);
    Value* Comm = ConstantPointerNull::get(cast<PointerType>(VoidPtrTy));
    
    CallInst* Call = Builder->CreateCall(MPIBcast, {Buffer, Count, Datatype, Root, Comm});
    
    CallSite Site;
    Site.CallInst = Call;
    Site.FunctionName = "MPI_Bcast";
    Site.Type = MPIFunctionType::Collective;
    Site.IsIndirect = false;
    
    return Site;
  }
  
  MPICallMetadata createMetadata(const CallSite& Site) {
    MPICallMetadata Metadata;
    Metadata.FunctionName = Site.FunctionName;
    Metadata.FunctionType = Site.Type;
    
    // Extract parameters from the call
    if (Site.CallInst) {
      for (unsigned i = 0; i < Site.CallInst->getNumOperands() - 1; ++i) {
        Metadata.Parameters.push_back(Site.CallInst->getOperand(i));
      }
    }
    
    return Metadata;
  }
  
  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<Module> M;
  std::unique_ptr<IRBuilder<>> Builder;
  Function* TestFunc;
  Function* MPISend;
  Function* MPIBcast;
};

TEST_F(HookInserterPerformanceTest, BasicPerformanceHookInsertion) {
  // Configure hook inserter for performance monitoring
  HookConfiguration Config;
  Config.EnablePerformanceHooks = true;
  Config.MonitorPointToPointOps = true;
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  Inserter.createHookDeclarations(*M);
  
  // Create MPI call site
  CallSite Site = createMPISendCall();
  MPICallMetadata Metadata = createMetadata(Site);
  
  // Insert performance hooks
  bool Result = Inserter.insertPerformanceHooks(Site, Metadata);
  EXPECT_TRUE(Result);
  
  // Verify performance hook functions were created
  Function* PerfBegin = M->getFunction(RuntimeInterface::getPerformanceBeginHookName());
  Function* PerfEnd = M->getFunction(RuntimeInterface::getPerformanceEndHookName());
  
  ASSERT_NE(PerfBegin, nullptr);
  ASSERT_NE(PerfEnd, nullptr);
  
  // Verify hook calls were inserted
  bool FoundBeginCall = false;
  bool FoundEndCall = false;
  
  for (auto& BB : *TestFunc) {
    for (auto& I : BB) {
      if (auto* Call = dyn_cast<CallInst>(&I)) {
        if (Call->getCalledFunction() == PerfBegin) {
          FoundBeginCall = true;
        } else if (Call->getCalledFunction() == PerfEnd) {
          FoundEndCall = true;
        }
      }
    }
  }
  
  EXPECT_TRUE(FoundBeginCall);
  EXPECT_TRUE(FoundEndCall);
}

TEST_F(HookInserterPerformanceTest, CommunicationVolumeHookInsertion) {
  // Configure hook inserter for communication volume monitoring
  HookConfiguration Config;
  Config.EnableCommunicationVolumeHooks = true;
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  Inserter.createHookDeclarations(*M);
  
  // Create MPI call site with buffer
  CallSite Site = createMPISendCall();
  MPICallMetadata Metadata = createMetadata(Site);
  
  // Insert communication volume hooks
  bool Result = Inserter.insertCommunicationVolumeHooks(Site, Metadata);
  EXPECT_TRUE(Result);
  
  // Verify communication volume hook function was created
  Function* CommVolumeHook = M->getFunction(RuntimeInterface::getCommunicationVolumeHookName());
  ASSERT_NE(CommVolumeHook, nullptr);
  
  // Verify hook call was inserted
  bool FoundCommVolumeCall = false;
  
  for (auto& BB : *TestFunc) {
    for (auto& I : BB) {
      if (auto* Call = dyn_cast<CallInst>(&I)) {
        if (Call->getCalledFunction() == CommVolumeHook) {
          FoundCommVolumeCall = true;
          
          // Verify hook has correct number of arguments
          EXPECT_EQ(Call->getNumOperands() - 1, 3); // function_name, volume, pattern
        }
      }
    }
  }
  
  EXPECT_TRUE(FoundCommVolumeCall);
}

TEST_F(HookInserterPerformanceTest, CommunicationPatternHookInsertion) {
  // Configure hook inserter for communication pattern monitoring
  HookConfiguration Config;
  Config.EnableCommunicationPatternHooks = true;
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  Inserter.createHookDeclarations(*M);
  
  // Create MPI point-to-point call site
  CallSite Site = createMPISendCall();
  MPICallMetadata Metadata = createMetadata(Site);
  
  // Insert communication pattern hooks
  bool Result = Inserter.insertCommunicationPatternHooks(Site, Metadata);
  EXPECT_TRUE(Result);
  
  // Verify communication pattern hook function was created
  Function* CommPatternHook = M->getFunction(RuntimeInterface::getCommunicationPatternHookName());
  ASSERT_NE(CommPatternHook, nullptr);
  
  // Verify hook call was inserted
  bool FoundCommPatternCall = false;
  
  for (auto& BB : *TestFunc) {
    for (auto& I : BB) {
      if (auto* Call = dyn_cast<CallInst>(&I)) {
        if (Call->getCalledFunction() == CommPatternHook) {
          FoundCommPatternCall = true;
          
          // Verify hook has correct number of arguments
          EXPECT_EQ(Call->getNumOperands() - 1, 5); // name, src, dest, tag, pattern_type
        }
      }
    }
  }
  
  EXPECT_TRUE(FoundCommPatternCall);
}

TEST_F(HookInserterPerformanceTest, CollectiveTimingHookInsertion) {
  // Configure hook inserter for collective timing monitoring
  HookConfiguration Config;
  Config.EnableCollectiveTimingHooks = true;
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  Inserter.createHookDeclarations(*M);
  
  // Create MPI collective call site
  CallSite Site = createMPIBcastCall();
  MPICallMetadata Metadata = createMetadata(Site);
  
  // Insert collective timing hooks
  bool Result = Inserter.insertCollectiveTimingHooks(Site, Metadata);
  EXPECT_TRUE(Result);
  
  // Verify collective timing hook function was created
  Function* CollectiveTimingHook = M->getFunction(RuntimeInterface::getCollectiveTimingHookName());
  ASSERT_NE(CollectiveTimingHook, nullptr);
  
  // Verify hook call was inserted
  bool FoundCollectiveTimingCall = false;
  
  for (auto& BB : *TestFunc) {
    for (auto& I : BB) {
      if (auto* Call = dyn_cast<CallInst>(&I)) {
        if (Call->getCalledFunction() == CollectiveTimingHook) {
          FoundCollectiveTimingCall = true;
          
          // Verify hook has correct number of arguments
          EXPECT_EQ(Call->getNumOperands() - 1, 3); // name, comm_size, timing_data
        }
      }
    }
  }
  
  EXPECT_TRUE(FoundCollectiveTimingCall);
}

TEST_F(HookInserterPerformanceTest, SynchronizationHookInsertion) {
  // Configure hook inserter for synchronization monitoring
  HookConfiguration Config;
  Config.EnableSynchronizationHooks = true;
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  Inserter.createHookDeclarations(*M);
  
  // Create MPI collective call site (synchronization point)
  CallSite Site = createMPIBcastCall();
  MPICallMetadata Metadata = createMetadata(Site);
  
  // Insert synchronization hooks
  bool Result = Inserter.insertSynchronizationHooks(Site, Metadata);
  EXPECT_TRUE(Result);
  
  // Verify synchronization hook function was created
  Function* SyncHook = M->getFunction(RuntimeInterface::getSynchronizationHookName());
  ASSERT_NE(SyncHook, nullptr);
  
  // Verify hook call was inserted
  bool FoundSyncCall = false;
  
  for (auto& BB : *TestFunc) {
    for (auto& I : BB) {
      if (auto* Call = dyn_cast<CallInst>(&I)) {
        if (Call->getCalledFunction() == SyncHook) {
          FoundSyncCall = true;
          
          // Verify hook has correct number of arguments
          EXPECT_EQ(Call->getNumOperands() - 1, 3); // name, sync_type, location
        }
      }
    }
  }
  
  EXPECT_TRUE(FoundSyncCall);
}

TEST_F(HookInserterPerformanceTest, SelectivePerformanceInstrumentation) {
  // Configure hook inserter with selective instrumentation
  HookConfiguration Config;
  Config.EnableSelectiveInstrumentation = true;
  Config.EnablePerformanceHooks = true;
  Config.EnableCommunicationVolumeHooks = true;
  Config.MonitorPointToPointOps = true;
  Config.MonitorCollectiveOps = false; // Disable collective monitoring
  
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  Inserter.createHookDeclarations(*M);
  
  // Test point-to-point operation (should be instrumented)
  CallSite P2PSite = createMPISendCall();
  MPICallMetadata P2PMetadata = createMetadata(P2PSite);
  
  bool P2PResult = Inserter.shouldApplyPerformanceMonitoring(P2PSite, P2PMetadata);
  EXPECT_TRUE(P2PResult);
  
  // Test collective operation (should not be instrumented due to config)
  CallSite CollectiveSite = createMPIBcastCall();
  MPICallMetadata CollectiveMetadata = createMetadata(CollectiveSite);
  
  bool CollectiveResult = Inserter.shouldApplyPerformanceMonitoring(CollectiveSite, CollectiveMetadata);
  EXPECT_FALSE(CollectiveResult);
}

TEST_F(HookInserterPerformanceTest, CommunicationVolumeCalculation) {
  HookConfiguration Config;
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  
  // Create MPI call site with known count
  CallSite Site = createMPISendCall();
  MPICallMetadata Metadata = createMetadata(Site);
  
  // Calculate communication volume
  IRBuilder<> TestBuilder(TestFunc->getEntryBlock().getTerminator());
  Inserter.Builder = std::make_unique<IRBuilder<>>(TestBuilder.GetInsertBlock());
  
  Value* Volume = Inserter.calculateCommunicationVolume(Site, Metadata);
  ASSERT_NE(Volume, nullptr);
  
  // Verify volume is calculated correctly (count * element_size)
  // For our test: 100 elements * 4 bytes = 400 bytes
  if (auto* ConstVolume = dyn_cast<ConstantInt>(Volume)) {
    EXPECT_EQ(ConstVolume->getZExtValue(), 400);
  }
}

TEST_F(HookInserterPerformanceTest, CommunicationPatternExtraction) {
  HookConfiguration Config;
  HookInserter Inserter(Config);
  Inserter.setModule(M.get());
  
  // Test point-to-point pattern
  CallSite P2PSite = createMPISendCall();
  MPICallMetadata P2PMetadata = createMetadata(P2PSite);
  
  IRBuilder<> TestBuilder(TestFunc->getEntryBlock().getTerminator());
  Inserter.Builder = std::make_unique<IRBuilder<>>(TestBuilder.GetInsertBlock());
  
  Value* P2PPattern = Inserter.extractCommunicationPattern(P2PSite, P2PMetadata);
  ASSERT_NE(P2PPattern, nullptr);
  
  // Test collective pattern
  CallSite CollectiveSite = createMPIBcastCall();
  MPICallMetadata CollectiveMetadata = createMetadata(CollectiveSite);
  
  Value* CollectivePattern = Inserter.extractCommunicationPattern(CollectiveSite, CollectiveMetadata);
  ASSERT_NE(CollectivePattern, nullptr);
}

TEST_F(HookInserterPerformanceTest, PerformanceOverheadThreshold) {
  // Configure with low overhead threshold
  HookConfiguration Config;
  Config.EnablePerformanceHooks = true;
  Config.PerformanceOverheadThreshold = 0.01; // 1% threshold
  
  HookInserter Inserter(Config);
  
  // Verify threshold is respected in configuration
  EXPECT_EQ(Inserter.getConfiguration().PerformanceOverheadThreshold, 0.01);
}

} // anonymous namespace
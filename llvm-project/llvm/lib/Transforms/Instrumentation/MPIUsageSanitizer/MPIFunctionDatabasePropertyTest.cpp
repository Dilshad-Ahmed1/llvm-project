//===- MPIFunctionDatabasePropertyTest.cpp - Property Tests for MPI DB ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements property-based tests for the MPI Function Database,
// specifically testing Property 1: Complete MPI Call Detection.
//
// **Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5, 7.1, 7.2, 7.4**
//
//===----------------------------------------------------------------------===//

#include "MPIFunctionDatabase.h"
#include "MPICallDetector.h"
#include "PropertyBasedTestFramework.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <memory>
#include <random>
#include <vector>
#include <set>
#include <algorithm>

using namespace llvm;

namespace {

/// Property test for MPI Function Database - Complete MPI Call Detection
/// **Feature: mpi-usage-sanitizer-llvm-pass, Property 1: Complete MPI Call Detection**
class MPIFunctionDatabasePropertyTest : public MPIPassPropertyTest {
protected:
  void SetUp() override {
    MPIPassPropertyTest::SetUp();
    
    // Initialize MPI function database
    FunctionDB = std::make_unique<MPIFunctionDatabase>();
    FunctionDB->initialize();
    
    // Initialize name mangling handler
    ManglingHandler = std::make_unique<NameManglingHandler>();
    
    // Set up test configuration
    TestConfig.NumFunctions = 3;
    TestConfig.MPICallsPerFunction = 5;
    TestConfig.CProbability = 0.5;
    TestConfig.CppProbability = 0.3;
    TestConfig.FortranProbability = 0.2;
    TestConfig.IndirectCallProbability = 0.2;
    TestConfig.ConditionalCallProbability = 0.3;
    TestConfig.LoopCallProbability = 0.2;
  }
  
  void TearDown() override {
    ManglingHandler.reset();
    FunctionDB.reset();
    MPIPassPropertyTest::TearDown();
  }

  /// **Property 1: Complete MPI Call Detection**
  /// For any LLVM IR module containing MPI function calls, the MPI Function Database
  /// SHALL correctly identify all direct and indirect MPI function calls across all
  /// supported language bindings (C, Fortran, C++).
  bool checkCompleteMPICallDetectionProperty(Module& M) {
    // Count all actual MPI calls in the module manually
    std::set<std::string> ActualMPICalls = countActualMPICalls(M);
    
    // Use the MPI function database to detect calls
    std::set<std::string> DetectedMPICalls = detectMPICallsUsingDatabase(M);
    
    // Verify completeness: all actual calls should be detected
    bool AllCallsDetected = true;
    std::vector<std::string> MissedCalls;
    
    for (const auto& ActualCall : ActualMPICalls) {
      if (DetectedMPICalls.find(ActualCall) == DetectedMPICalls.end()) {
        AllCallsDetected = false;
        MissedCalls.push_back(ActualCall);
      }
    }
    
    // Log any missed calls for debugging
    if (!AllCallsDetected && VerboseLogging) {
      llvm::outs() << "Missed MPI calls: ";
      for (const auto& Call : MissedCalls) {
        llvm::outs() << Call << " ";
      }
      llvm::outs() << "\n";
    }
    
    // Verify no false positives: detected calls should be actual MPI calls
    bool NoFalsePositives = true;
    std::vector<std::string> FalsePositives;
    
    for (const auto& DetectedCall : DetectedMPICalls) {
      if (ActualMPICalls.find(DetectedCall) == ActualMPICalls.end()) {
        NoFalsePositives = false;
        FalsePositives.push_back(DetectedCall);
      }
    }
    
    if (!NoFalsePositives && VerboseLogging) {
      llvm::outs() << "False positive MPI calls: ";
      for (const auto& Call : FalsePositives) {
        llvm::outs() << Call << " ";
      }
      llvm::outs() << "\n";
    }
    
    return AllCallsDetected && NoFalsePositives;
  }

  /// Generate a comprehensive test module with various MPI call patterns
  std::unique_ptr<Module> generateComprehensiveMPIModule() {
    auto M = std::make_unique<Module>("comprehensive_mpi_test", *Context);
    
    // Generate C MPI calls
    generateCMPICalls(*M);
    
    // Generate Fortran MPI calls (with name mangling)
    generateFortranMPICalls(*M);
    
    // Generate C++ MPI calls
    generateCppMPICalls(*M);
    
    // Generate indirect MPI calls
    generateIndirectMPICalls(*M);
    
    // Generate conditional MPI calls
    generateConditionalMPICalls(*M);
    
    // Generate MPI calls in loops
    generateLoopMPICalls(*M);
    
    return M;
  }

private:
  std::unique_ptr<MPIFunctionDatabase> FunctionDB;
  std::unique_ptr<NameManglingHandler> ManglingHandler;
  ModuleGenerationConfig TestConfig;
  
  /// Count actual MPI calls in the module by examining all call instructions
  std::set<std::string> countActualMPICalls(Module& M) {
    std::set<std::string> MPICalls;
    
    for (Function& F : M) {
      for (BasicBlock& BB : F) {
        for (Instruction& I : BB) {
          if (CallInst* Call = dyn_cast<CallInst>(&I)) {
            if (Function* Callee = Call->getCalledFunction()) {
              StringRef FuncName = Callee->getName();
              
              // Check if this is an MPI function call
              if (isMPIFunctionName(FuncName)) {
                MPICalls.insert(FuncName.str());
              }
            } else {
              // Handle indirect calls through function pointers
              Value* CalledValue = Call->getCalledOperand();
              if (auto* Cast = dyn_cast<BitCastInst>(CalledValue)) {
                if (auto* GV = dyn_cast<GlobalVariable>(Cast->getOperand(0))) {
                  StringRef FuncName = GV->getName();
                  if (isMPIFunctionName(FuncName)) {
                    MPICalls.insert(FuncName.str());
                  }
                }
              }
            }
          }
        }
      }
    }
    
    return MPICalls;
  }
  
  /// Use the MPI function database to detect MPI calls
  std::set<std::string> detectMPICallsUsingDatabase(Module& M) {
    std::set<std::string> DetectedCalls;
    
    for (Function& F : M) {
      for (BasicBlock& BB : F) {
        for (Instruction& I : BB) {
          if (CallInst* Call = dyn_cast<CallInst>(&I)) {
            if (Function* Callee = Call->getCalledFunction()) {
              StringRef FuncName = Callee->getName();
              
              // Use database to check if this is an MPI function
              if (FunctionDB->isMPIFunction(FuncName)) {
                DetectedCalls.insert(FuncName.str());
              }
            } else {
              // Handle indirect calls
              Value* CalledValue = Call->getCalledOperand();
              if (auto* Cast = dyn_cast<BitCastInst>(CalledValue)) {
                if (auto* GV = dyn_cast<GlobalVariable>(Cast->getOperand(0))) {
                  StringRef FuncName = GV->getName();
                  if (FunctionDB->isMPIFunction(FuncName)) {
                    DetectedCalls.insert(FuncName.str());
                  }
                }
              }
            }
          }
        }
      }
    }
    
    return DetectedCalls;
  }
  
  /// Check if a function name is an MPI function (ground truth)
  bool isMPIFunctionName(StringRef Name) {
    // Standard C MPI functions
    if (Name.startswith("MPI_") || Name.startswith("PMPI_")) {
      return true;
    }
    
    // Fortran mangled MPI functions
    if (ManglingHandler->isFortranMangled(Name)) {
      std::string Demangled = ManglingHandler->demangleFortranName(Name);
      if (StringRef(Demangled).startswith("MPI_") || StringRef(Demangled).startswith("PMPI_")) {
        return true;
      }
    }
    
    // C++ MPI namespace functions
    if (Name.contains("MPI::") || Name.contains("_ZN3MPI")) {
      return true;
    }
    
    return false;
  }
  
  /// Generate C MPI function calls
  void generateCMPICalls(Module& M) {
    // Create a test function
    FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
    Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_c_mpi", &M);
    BasicBlock* BB = BasicBlock::Create(*Context, "entry", TestFunc);
    IRBuilder<> Builder(BB);
    
    // Generate various C MPI function calls
    std::vector<std::string> CMPIFunctions = {
      "MPI_Init", "MPI_Finalize", "MPI_Send", "MPI_Recv", "MPI_Bcast",
      "MPI_Reduce", "MPI_Allreduce", "MPI_Gather", "MPI_Scatter",
      "MPI_Comm_rank", "MPI_Comm_size", "MPI_Barrier", "MPI_Wait",
      "MPI_Isend", "MPI_Irecv", "MPI_Waitall"
    };
    
    for (const auto& FuncName : CMPIFunctions) {
      // Create function declaration
      FunctionType* MPIFuncType = FunctionType::get(Type::getInt32Ty(*Context), 
                                                    {Type::getInt8PtrTy(*Context)}, true);
      Function* MPIFunc = Function::Create(MPIFuncType, Function::ExternalLinkage, FuncName, &M);
      
      // Create call to MPI function
      Value* NullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
      Builder.CreateCall(MPIFunc, {NullPtr});
    }
    
    Builder.CreateRetVoid();
  }
  
  /// Generate Fortran MPI function calls with name mangling
  void generateFortranMPICalls(Module& M) {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
    Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_fortran_mpi", &M);
    BasicBlock* BB = BasicBlock::Create(*Context, "entry", TestFunc);
    IRBuilder<> Builder(BB);
    
    // Generate Fortran MPI function calls with different mangling conventions
    std::vector<std::string> FortranMPIFunctions = {
      "MPI_Init", "MPI_Finalize", "MPI_Send", "MPI_Recv", "MPI_Bcast"
    };
    
    std::vector<FortranCompiler> Compilers = {
      FortranCompiler::GFortran, FortranCompiler::Intel, FortranCompiler::PGI
    };
    
    for (const auto& BaseName : FortranMPIFunctions) {
      for (auto Compiler : Compilers) {
        std::string MangledName = ManglingHandler->mangleFortranName(BaseName, Compiler);
        
        // Create function declaration with mangled name
        FunctionType* MPIFuncType = FunctionType::get(Type::getVoidTy(*Context), 
                                                      {Type::getInt32PtrTy(*Context)}, true);
        Function* MPIFunc = Function::Create(MPIFuncType, Function::ExternalLinkage, MangledName, &M);
        
        // Create call to mangled MPI function
        Value* NullPtr = ConstantPointerNull::get(Type::getInt32PtrTy(*Context));
        Builder.CreateCall(MPIFunc, {NullPtr});
      }
    }
    
    Builder.CreateRetVoid();
  }
  
  /// Generate C++ MPI function calls
  void generateCppMPICalls(Module& M) {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
    Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_cpp_mpi", &M);
    BasicBlock* BB = BasicBlock::Create(*Context, "entry", TestFunc);
    IRBuilder<> Builder(BB);
    
    // Generate C++ MPI namespace function calls (mangled names)
    std::vector<std::string> CppMPIFunctions = {
      "_ZN3MPI4InitEiPPc",           // MPI::Init(int&, char**&)
      "_ZN3MPI8FinalizeEv",         // MPI::Finalize()
      "_ZN3MPI4Comm4SendEPKviiiRKNS_8DatatypeE", // MPI::Comm::Send
      "_ZN3MPI4Comm4RecvEPviiiRKNS_8DatatypeERNS_6StatusE", // MPI::Comm::Recv
      "_ZN3MPI4Comm5BcastEPviRKNS_8DatatypeEi" // MPI::Comm::Bcast
    };
    
    for (const auto& FuncName : CppMPIFunctions) {
      // Create function declaration
      FunctionType* MPIFuncType = FunctionType::get(Type::getVoidTy(*Context), 
                                                    {Type::getInt8PtrTy(*Context)}, true);
      Function* MPIFunc = Function::Create(MPIFuncType, Function::ExternalLinkage, FuncName, &M);
      
      // Create call to C++ MPI function
      Value* NullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
      Builder.CreateCall(MPIFunc, {NullPtr});
    }
    
    Builder.CreateRetVoid();
  }
  
  /// Generate indirect MPI function calls through function pointers
  void generateIndirectMPICalls(Module& M) {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
    Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_indirect_mpi", &M);
    BasicBlock* BB = BasicBlock::Create(*Context, "entry", TestFunc);
    IRBuilder<> Builder(BB);
    
    // Create function pointer type for MPI functions
    FunctionType* MPIFuncType = FunctionType::get(Type::getInt32Ty(*Context), 
                                                  {Type::getInt8PtrTy(*Context)}, true);
    
    // Create global variable to hold function pointer
    GlobalVariable* FuncPtr = new GlobalVariable(M, MPIFuncType->getPointerTo(),
                                                false, GlobalValue::ExternalLinkage,
                                                nullptr, "mpi_send_ptr");
    
    // Create MPI_Send function
    Function* MPISend = Function::Create(MPIFuncType, Function::ExternalLinkage, "MPI_Send", &M);
    
    // Load function pointer and call indirectly
    Value* LoadedPtr = Builder.CreateLoad(MPIFuncType->getPointerTo(), FuncPtr);
    Value* NullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
    Builder.CreateCall(MPIFuncType, LoadedPtr, {NullPtr});
    
    Builder.CreateRetVoid();
  }
  
  /// Generate conditional MPI function calls
  void generateConditionalMPICalls(Module& M) {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
    Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_conditional_mpi", &M);
    BasicBlock* EntryBB = BasicBlock::Create(*Context, "entry", TestFunc);
    BasicBlock* ThenBB = BasicBlock::Create(*Context, "then", TestFunc);
    BasicBlock* ElseBB = BasicBlock::Create(*Context, "else", TestFunc);
    BasicBlock* MergeBB = BasicBlock::Create(*Context, "merge", TestFunc);
    
    IRBuilder<> Builder(EntryBB);
    
    // Create condition
    Value* Condition = Builder.getTrue();
    Builder.CreateCondBr(Condition, ThenBB, ElseBB);
    
    // Then branch - call MPI_Send
    Builder.SetInsertPoint(ThenBB);
    FunctionType* MPIFuncType = FunctionType::get(Type::getInt32Ty(*Context), 
                                                  {Type::getInt8PtrTy(*Context)}, true);
    Function* MPISend = Function::Create(MPIFuncType, Function::ExternalLinkage, "MPI_Send", &M);
    Value* NullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
    Builder.CreateCall(MPISend, {NullPtr});
    Builder.CreateBr(MergeBB);
    
    // Else branch - call MPI_Recv
    Builder.SetInsertPoint(ElseBB);
    Function* MPIRecv = Function::Create(MPIFuncType, Function::ExternalLinkage, "MPI_Recv", &M);
    Builder.CreateCall(MPIRecv, {NullPtr});
    Builder.CreateBr(MergeBB);
    
    // Merge block
    Builder.SetInsertPoint(MergeBB);
    Builder.CreateRetVoid();
  }
  
  /// Generate MPI function calls in loops
  void generateLoopMPICalls(Module& M) {
    FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
    Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_loop_mpi", &M);
    BasicBlock* EntryBB = BasicBlock::Create(*Context, "entry", TestFunc);
    BasicBlock* LoopBB = BasicBlock::Create(*Context, "loop", TestFunc);
    BasicBlock* ExitBB = BasicBlock::Create(*Context, "exit", TestFunc);
    
    IRBuilder<> Builder(EntryBB);
    
    // Initialize loop counter
    Value* Counter = Builder.CreateAlloca(Type::getInt32Ty(*Context));
    Builder.CreateStore(Builder.getInt32(0), Counter);
    Builder.CreateBr(LoopBB);
    
    // Loop body
    Builder.SetInsertPoint(LoopBB);
    Value* CurrentCount = Builder.CreateLoad(Type::getInt32Ty(*Context), Counter);
    
    // Call MPI function in loop
    FunctionType* MPIFuncType = FunctionType::get(Type::getInt32Ty(*Context), 
                                                  {Type::getInt8PtrTy(*Context)}, true);
    Function* MPIBcast = Function::Create(MPIFuncType, Function::ExternalLinkage, "MPI_Bcast", &M);
    Value* NullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
    Builder.CreateCall(MPIBcast, {NullPtr});
    
    // Update counter and check condition
    Value* NextCount = Builder.CreateAdd(CurrentCount, Builder.getInt32(1));
    Builder.CreateStore(NextCount, Counter);
    Value* Condition = Builder.CreateICmpSLT(NextCount, Builder.getInt32(10));
    Builder.CreateCondBr(Condition, LoopBB, ExitBB);
    
    // Exit block
    Builder.SetInsertPoint(ExitBB);
    Builder.CreateRetVoid();
  }
};

/// Test instantiation for Property 1: Complete MPI Call Detection
TEST_F(MPIFunctionDatabasePropertyTest, CompleteMPICallDetection) {
  // **Property 1: Complete MPI Call Detection**
  // **Validates: Requirements 1.1, 1.2, 1.3, 1.4, 1.5, 7.1, 7.2, 7.4**
  
  PropertyTestExecution Result = runPropertyTest(
    "CompleteMPICallDetection",
    [this](Module& M) { return checkCompleteMPICallDetectionProperty(M); },
    100, // Run 100 iterations
    TestConfig
  );
  
  EXPECT_TRUE(Result.TestPassed) 
    << "Property 1: Complete MPI Call Detection failed after " 
    << Result.FirstFailureIteration << " iterations. "
    << "Success rate: " << (Result.getSuccessRate() * 100.0) << "%";
  
  // Ensure high success rate (at least 95%)
  EXPECT_GE(Result.getSuccessRate(), 0.95)
    << "Success rate too low: " << (Result.getSuccessRate() * 100.0) << "%";
  
  if (VerboseLogging) {
    generateTestReport(Result, llvm::outs());
  }
}

/// Test with comprehensive manually-generated module
TEST_F(MPIFunctionDatabasePropertyTest, ComprehensiveMPICallDetection) {
  // Test with a carefully crafted module containing all MPI call patterns
  auto TestModule = generateComprehensiveMPIModule();
  
  bool PropertyHolds = checkCompleteMPICallDetectionProperty(*TestModule);
  
  EXPECT_TRUE(PropertyHolds)
    << "Complete MPI call detection failed on comprehensive test module";
  
  // Additional verification: check specific requirements
  
  // Requirement 1.1: Direct calls to MPI functions
  std::set<std::string> ActualCalls = countActualMPICalls(*TestModule);
  std::set<std::string> DetectedCalls = detectMPICallsUsingDatabase(*TestModule);
  
  // Should detect standard C MPI functions
  EXPECT_TRUE(DetectedCalls.count("MPI_Init") > 0) << "Failed to detect MPI_Init";
  EXPECT_TRUE(DetectedCalls.count("MPI_Send") > 0) << "Failed to detect MPI_Send";
  EXPECT_TRUE(DetectedCalls.count("MPI_Recv") > 0) << "Failed to detect MPI_Recv";
  
  // Requirement 1.2: Indirect calls through function pointers
  // (Tested in generateIndirectMPICalls)
  
  // Requirement 7.1, 7.2: C and Fortran bindings
  // Should detect Fortran mangled functions
  bool HasFortranCalls = false;
  for (const auto& Call : DetectedCalls) {
    if (ManglingHandler->isFortranMangled(Call)) {
      HasFortranCalls = true;
      break;
    }
  }
  EXPECT_TRUE(HasFortranCalls) << "Failed to detect Fortran MPI calls";
  
  // Requirement 7.4: C++ bindings
  bool HasCppCalls = false;
  for (const auto& Call : DetectedCalls) {
    if (Call.find("_ZN3MPI") != std::string::npos) {
      HasCppCalls = true;
      break;
    }
  }
  EXPECT_TRUE(HasCppCalls) << "Failed to detect C++ MPI calls";
}

/// Test edge cases and boundary conditions
TEST_F(MPIFunctionDatabasePropertyTest, EdgeCaseMPICallDetection) {
  auto M = std::make_unique<Module>("edge_case_test", *Context);
  
  // Test empty module
  bool EmptyModuleResult = checkCompleteMPICallDetectionProperty(*M);
  EXPECT_TRUE(EmptyModuleResult) << "Failed on empty module";
  
  // Test module with non-MPI functions only
  FunctionType* FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function* NonMPIFunc = Function::Create(FT, Function::ExternalLinkage, "regular_function", M.get());
  BasicBlock* BB = BasicBlock::Create(*Context, "entry", NonMPIFunc);
  IRBuilder<> Builder(BB);
  
  // Call a non-MPI function
  Function* PrintfFunc = Function::Create(
    FunctionType::get(Type::getInt32Ty(*Context), {Type::getInt8PtrTy(*Context)}, true),
    Function::ExternalLinkage, "printf", M.get());
  Value* FormatStr = Builder.CreateGlobalStringPtr("Hello World\n");
  Builder.CreateCall(PrintfFunc, {FormatStr});
  Builder.CreateRetVoid();
  
  bool NonMPIResult = checkCompleteMPICallDetectionProperty(*M);
  EXPECT_TRUE(NonMPIResult) << "Failed on module with non-MPI functions";
  
  // Test with mixed MPI and non-MPI calls
  Function* MPIInit = Function::Create(
    FunctionType::get(Type::getInt32Ty(*Context), {Type::getInt8PtrTy(*Context)}, true),
    Function::ExternalLinkage, "MPI_Init", M.get());
  Value* NullPtr = ConstantPointerNull::get(Type::getInt8PtrTy(*Context));
  Builder.CreateCall(MPIInit, {NullPtr});
  
  bool MixedResult = checkCompleteMPICallDetectionProperty(*M);
  EXPECT_TRUE(MixedResult) << "Failed on module with mixed MPI and non-MPI calls";
}

} // anonymous namespace
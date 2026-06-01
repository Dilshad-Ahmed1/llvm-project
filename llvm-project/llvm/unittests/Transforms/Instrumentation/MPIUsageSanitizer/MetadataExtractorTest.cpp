//===- MetadataExtractorTest.cpp - Unit tests for MetadataExtractor -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MetadataExtractor.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPICallDetector.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPIFunctionDatabase.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class MetadataExtractorTest : public ::testing::Test {
protected:
  void SetUp() override {
    M = std::make_unique<Module>("test", Context);
    DB = std::make_unique<MPIFunctionDatabase>();
    Extractor = std::make_unique<MetadataExtractor>();
    Extractor->setFunctionDatabase(DB.get());
  }

  LLVMContext Context;
  std::unique_ptr<Module> M;
  std::unique_ptr<MPIFunctionDatabase> DB;
  std::unique_ptr<MetadataExtractor> Extractor;
};

TEST_F(MetadataExtractorTest, ExtractCommunicatorFromMPISend) {
  // Create MPI_Send function signature
  Type* IntTy = Type::getInt32Ty(Context);
  Type* PtrTy = PointerType::get(Context, 0);
  
  std::vector<Type*> SendArgs = {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy};
  FunctionType* SendFT = FunctionType::get(IntTy, SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  // Create test function
  FunctionType* FT = FunctionType::get(Type::getVoidTy(Context), false);
  Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
  IRBuilder<> Builder(BB);
  
  // Create test values
  Value* Buffer = Builder.CreateAlloca(IntTy, ConstantInt::get(IntTy, 100));
  Value* Count = ConstantInt::get(IntTy, 10);
  Value* Datatype = ConstantInt::get(IntTy, 1);
  Value* Dest = ConstantInt::get(IntTy, 1);
  Value* Tag = ConstantInt::get(IntTy, 0);
  Value* Comm = ConstantInt::get(IntTy, 0); // MPI_COMM_WORLD
  
  // Create MPI_Send call
  std::vector<Value*> Args = {Buffer, Count, Datatype, Dest, Tag, Comm};
  CallInst* SendCall = Builder.CreateCall(MPISend, Args);
  Builder.CreateRetVoid();
  
  // Test communicator extraction
  CallSite Site(SendCall, "MPI_Send", MPIFunctionType::PointToPoint, false);
  Value* ExtractedComm = Extractor->extractCommunicator(Site);
  
  ASSERT_NE(ExtractedComm, nullptr);
  EXPECT_EQ(ExtractedComm, Comm);
}

TEST_F(MetadataExtractorTest, ExtractBufferInfoFromMPISend) {
  // Create MPI_Send function signature
  Type* IntTy = Type::getInt32Ty(Context);
  Type* PtrTy = PointerType::get(Context, 0);
  
  std::vector<Type*> SendArgs = {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy};
  FunctionType* SendFT = FunctionType::get(IntTy, SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  // Create test function
  FunctionType* FT = FunctionType::get(Type::getVoidTy(Context), false);
  Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
  IRBuilder<> Builder(BB);
  
  // Create test values
  Value* Buffer = Builder.CreateAlloca(IntTy, ConstantInt::get(IntTy, 100));
  Value* Count = ConstantInt::get(IntTy, 10);
  Value* Datatype = ConstantInt::get(IntTy, 1);
  Value* Dest = ConstantInt::get(IntTy, 1);
  Value* Tag = ConstantInt::get(IntTy, 0);
  Value* Comm = ConstantInt::get(IntTy, 0);
  
  // Create MPI_Send call
  std::vector<Value*> Args = {Buffer, Count, Datatype, Dest, Tag, Comm};
  CallInst* SendCall = Builder.CreateCall(MPISend, Args);
  Builder.CreateRetVoid();
  
  // Test buffer info extraction
  CallSite Site(SendCall, "MPI_Send", MPIFunctionType::PointToPoint, false);
  std::vector<Value*> BufferInfo = Extractor->extractBufferInfo(Site);
  
  ASSERT_GE(BufferInfo.size(), 3u);
  EXPECT_EQ(BufferInfo[0], Buffer);  // Buffer
  EXPECT_EQ(BufferInfo[1], Count);   // Count
  EXPECT_EQ(BufferInfo[2], Datatype); // Datatype
}

TEST_F(MetadataExtractorTest, ExtractRequestHandleFromMPIIsend) {
  // Create MPI_Isend function signature
  Type* IntTy = Type::getInt32Ty(Context);
  Type* PtrTy = PointerType::get(Context, 0);
  
  std::vector<Type*> IsendArgs = {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy, PtrTy};
  FunctionType* IsendFT = FunctionType::get(IntTy, IsendArgs, false);
  Function* MPIIsend = Function::Create(IsendFT, Function::ExternalLinkage, "MPI_Isend", M.get());
  
  // Create test function
  FunctionType* FT = FunctionType::get(Type::getVoidTy(Context), false);
  Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
  IRBuilder<> Builder(BB);
  
  // Create test values
  Value* Buffer = Builder.CreateAlloca(IntTy, ConstantInt::get(IntTy, 100));
  Value* Count = ConstantInt::get(IntTy, 10);
  Value* Datatype = ConstantInt::get(IntTy, 1);
  Value* Dest = ConstantInt::get(IntTy, 1);
  Value* Tag = ConstantInt::get(IntTy, 0);
  Value* Comm = ConstantInt::get(IntTy, 0);
  Value* Request = Builder.CreateAlloca(PtrTy);
  
  // Create MPI_Isend call
  std::vector<Value*> Args = {Buffer, Count, Datatype, Dest, Tag, Comm, Request};
  CallInst* IsendCall = Builder.CreateCall(MPIIsend, Args);
  Builder.CreateRetVoid();
  
  // Test request handle extraction
  CallSite Site(IsendCall, "MPI_Isend", MPIFunctionType::PointToPoint, false);
  Value* ExtractedRequest = Extractor->extractRequestHandle(Site);
  
  ASSERT_NE(ExtractedRequest, nullptr);
  EXPECT_EQ(ExtractedRequest, Request);
}

TEST_F(MetadataExtractorTest, NoRequestHandleForBlockingOperation) {
  // Create MPI_Send function signature (blocking operation)
  Type* IntTy = Type::getInt32Ty(Context);
  Type* PtrTy = PointerType::get(Context, 0);
  
  std::vector<Type*> SendArgs = {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy};
  FunctionType* SendFT = FunctionType::get(IntTy, SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  // Create test function
  FunctionType* FT = FunctionType::get(Type::getVoidTy(Context), false);
  Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
  IRBuilder<> Builder(BB);
  
  // Create test values
  Value* Buffer = Builder.CreateAlloca(IntTy, ConstantInt::get(IntTy, 100));
  Value* Count = ConstantInt::get(IntTy, 10);
  Value* Datatype = ConstantInt::get(IntTy, 1);
  Value* Dest = ConstantInt::get(IntTy, 1);
  Value* Tag = ConstantInt::get(IntTy, 0);
  Value* Comm = ConstantInt::get(IntTy, 0);
  
  // Create MPI_Send call
  std::vector<Value*> Args = {Buffer, Count, Datatype, Dest, Tag, Comm};
  CallInst* SendCall = Builder.CreateCall(MPISend, Args);
  Builder.CreateRetVoid();
  
  // Test that no request handle is extracted for blocking operation
  CallSite Site(SendCall, "MPI_Send", MPIFunctionType::PointToPoint, false);
  Value* ExtractedRequest = Extractor->extractRequestHandle(Site);
  
  // Should be null for blocking operations
  EXPECT_EQ(ExtractedRequest, nullptr);
}

TEST_F(MetadataExtractorTest, ExtractMetadataCompleteness) {
  // Create MPI_Send function signature
  Type* IntTy = Type::getInt32Ty(Context);
  Type* PtrTy = PointerType::get(Context, 0);
  
  std::vector<Type*> SendArgs = {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy};
  FunctionType* SendFT = FunctionType::get(IntTy, SendArgs, false);
  Function* MPISend = Function::Create(SendFT, Function::ExternalLinkage, "MPI_Send", M.get());
  
  // Create test function
  FunctionType* FT = FunctionType::get(Type::getVoidTy(Context), false);
  Function* TestFunc = Function::Create(FT, Function::ExternalLinkage, "test_func", M.get());
  BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
  IRBuilder<> Builder(BB);
  
  // Create test values
  Value* Buffer = Builder.CreateAlloca(IntTy, ConstantInt::get(IntTy, 100));
  Value* Count = ConstantInt::get(IntTy, 10);
  Value* Datatype = ConstantInt::get(IntTy, 1);
  Value* Dest = ConstantInt::get(IntTy, 1);
  Value* Tag = ConstantInt::get(IntTy, 0);
  Value* Comm = ConstantInt::get(IntTy, 0);
  
  // Create MPI_Send call
  std::vector<Value*> Args = {Buffer, Count, Datatype, Dest, Tag, Comm};
  CallInst* SendCall = Builder.CreateCall(MPISend, Args);
  Builder.CreateRetVoid();
  
  // Test complete metadata extraction
  CallSite Site(SendCall, "MPI_Send", MPIFunctionType::PointToPoint, false);
  MPICallMetadata Metadata = Extractor->extractMetadata(Site);
  
  EXPECT_EQ(Metadata.FunctionName, "MPI_Send");
  EXPECT_EQ(Metadata.Parameters.size(), 6u);
  EXPECT_EQ(Metadata.FunctionType, MPIFunctionType::PointToPoint);
  EXPECT_NE(Metadata.ReturnType, nullptr);
}

} // anonymous namespace
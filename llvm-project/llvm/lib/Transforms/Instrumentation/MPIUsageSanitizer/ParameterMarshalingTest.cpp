/**
 * @file ParameterMarshalingTest.cpp
 * @brief Tests for parameter marshaling between LLVM IR and runtime hooks
 */

#include "MetadataExtractor.h"
#include "HookInserter.h"
#include "gtest/gtest.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

namespace llvm {
namespace mpi_sanitizer {

class ParameterMarshalingTest : public ::testing::Test {
protected:
  void SetUp() override {
    Context = std::make_unique<LLVMContext>();
    M = std::make_unique<Module>("test", *Context);
    Builder = std::make_unique<IRBuilder<>>(*Context);
  }

  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<Module> M;
  std::unique_ptr<IRBuilder<>> Builder;
};

TEST_F(ParameterMarshalingTest, BasicParameterExtraction) {
  MetadataExtractor extractor;
  
  // Create a simple function with MPI call
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  Builder->SetInsertPoint(BB);
  
  // Create mock MPI_Send call
  std::vector<Type*> ArgTypes = {
    Type::getInt8PtrTy(*Context),  // buffer
    Type::getInt32Ty(*Context),    // count
    Type::getInt32Ty(*Context),    // datatype
    Type::getInt32Ty(*Context),    // dest
    Type::getInt32Ty(*Context),    // tag
    Type::getInt32Ty(*Context)     // comm
  };
  
  FunctionType *MPISendType = FunctionType::get(Type::getInt32Ty(*Context), ArgTypes, false);
  Function *MPISend = Function::Create(MPISendType, Function::ExternalLinkage, "MPI_Send", M.get());
  
  // Create call instruction
  std::vector<Value*> Args = {
    ConstantPointerNull::get(Type::getInt8PtrTy(*Context)),
    ConstantInt::get(Type::getInt32Ty(*Context), 100),
    ConstantInt::get(Type::getInt32Ty(*Context), 1), // MPI_INT
    ConstantInt::get(Type::getInt32Ty(*Context), 1), // dest rank
    ConstantInt::get(Type::getInt32Ty(*Context), 0), // tag
    ConstantInt::get(Type::getInt32Ty(*Context), 0)  // MPI_COMM_WORLD
  };
  
  CallInst *Call = Builder->CreateCall(MPISend, Args);
  
  // Test metadata extraction
  MPICallMetadata metadata = extractor.extractMetadata(Call);
  
  EXPECT_EQ(metadata.functionName, "MPI_Send");
  EXPECT_EQ(metadata.count, 100);
  EXPECT_EQ(metadata.destinationRank, 1);
  EXPECT_EQ(metadata.tag, 0);
}

TEST_F(ParameterMarshalingTest, BufferSizeCalculation) {
  MetadataExtractor extractor;
  
  // Test buffer size calculation for different data types
  EXPECT_EQ(extractor.calculateBufferSize(100, MPI_INT), 100 * sizeof(int));
  EXPECT_EQ(extractor.calculateBufferSize(50, MPI_DOUBLE), 50 * sizeof(double));
  EXPECT_EQ(extractor.calculateBufferSize(200, MPI_CHAR), 200 * sizeof(char));
}

TEST_F(ParameterMarshalingTest, HookParameterMarshaling) {
  HookInserter inserter;
  
  // Create function for testing
  FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, "test", M.get());
  BasicBlock *BB = BasicBlock::Create(*Context, "entry", F);
  Builder->SetInsertPoint(BB);
  
  // Test hook parameter creation
  MPICallMetadata metadata;
  metadata.functionName = "MPI_Send";
  metadata.bufferAddress = reinterpret_cast<void*>(0x1000);
  metadata.bufferSize = 1024;
  metadata.count = 256;
  
  // This should not crash
  EXPECT_NO_THROW({
    std::vector<Value*> hookArgs = inserter.createHookArguments(*Builder, metadata);
    EXPECT_FALSE(hookArgs.empty());
  });
}

} // namespace mpi_sanitizer
} // namespace llvm
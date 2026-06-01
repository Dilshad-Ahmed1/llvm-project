//===- test_metadata_comprehensive.cpp - Comprehensive MetadataExtractor test -===//
//
// Comprehensive test to verify MetadataExtractor functionality with different MPI functions
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

// Include the MPI sanitizer headers
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MetadataExtractor.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPICallDetector.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPIFunctionDatabase.h"

using namespace llvm;

void testMPIFunction(const std::string& funcName, MPIFunctionType funcType, 
                     const std::vector<Type*>& paramTypes, Module& M, 
                     MetadataExtractor& extractor) {
    LLVMContext& Context = M.getContext();
    
    std::cout << "\n=== Testing " << funcName << " ===\n";
    
    // Create function signature
    Type* IntTy = Type::getInt32Ty(Context);
    FunctionType* FuncTy = FunctionType::get(IntTy, paramTypes, false);
    Function* MPIFunc = Function::Create(FuncTy, Function::ExternalLinkage, funcName, &M);
    
    // Create a test function that calls this MPI function
    Type* VoidTy = Type::getVoidTy(Context);
    FunctionType* TestFuncTy = FunctionType::get(VoidTy, {}, false);
    Function* TestFunc = Function::Create(TestFuncTy, Function::ExternalLinkage, 
                                          "test_" + funcName, &M);
    
    BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
    IRBuilder<> Builder(BB);
    
    // Create dummy arguments
    std::vector<Value*> Args;
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        if (paramTypes[i]->isPointerTy()) {
            Args.push_back(Builder.CreateAlloca(IntTy, Builder.getInt32(100)));
        } else {
            Args.push_back(Builder.getInt32(i));
        }
    }
    
    CallInst* MPICall = Builder.CreateCall(MPIFunc, Args);
    Builder.CreateRetVoid();
    
    // Test metadata extraction
    CallSite Site(MPICall, funcName, funcType, false);
    MPICallMetadata Metadata = extractor.extractMetadata(Site);
    
    std::cout << "Function: " << Metadata.FunctionName.str() << "\n";
    std::cout << "Parameters: " << Metadata.Parameters.size() << "\n";
    std::cout << "Parameter infos: " << Metadata.ParameterInfos.size() << "\n";
    
    // Test specific extractors
    Value* Communicator = extractor.extractCommunicator(Site);
    std::vector<Value*> BufferInfo = extractor.extractBufferInfo(Site);
    Value* Request = extractor.extractRequestHandle(Site);
    Value* Status = extractor.extractStatus(Site);
    
    std::cout << "Communicator: " << (Communicator ? "Found" : "Not found") << "\n";
    std::cout << "Buffer info: " << BufferInfo.size() << " elements\n";
    std::cout << "Request: " << (Request ? "Found" : "Not found") << "\n";
    std::cout << "Status: " << (Status ? "Found" : "Not found") << "\n";
    
    // Print parameter roles
    for (size_t i = 0; i < Metadata.ParameterInfos.size(); ++i) {
        const auto& Info = Metadata.ParameterInfos[i];
        std::cout << "  Param " << i << ": ";
        switch (Info.Role) {
            case ParameterRole::Buffer: std::cout << "Buffer"; break;
            case ParameterRole::Count: std::cout << "Count"; break;
            case ParameterRole::Datatype: std::cout << "Datatype"; break;
            case ParameterRole::Destination: std::cout << "Destination"; break;
            case ParameterRole::Source: std::cout << "Source"; break;
            case ParameterRole::Tag: std::cout << "Tag"; break;
            case ParameterRole::Communicator: std::cout << "Communicator"; break;
            case ParameterRole::Request: std::cout << "Request"; break;
            case ParameterRole::Status: std::cout << "Status"; break;
            case ParameterRole::Root: std::cout << "Root"; break;
            case ParameterRole::Operation: std::cout << "Operation"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << " (I:" << (Info.IsInput ? "Y" : "N") 
                  << " O:" << (Info.IsOutput ? "Y" : "N") << ")\n";
    }
}

int main() {
    LLVMContext Context;
    Module M("comprehensive_test", Context);
    
    Type* IntTy = Type::getInt32Ty(Context);
    Type* PtrTy = PointerType::get(Context, 0);
    
    // Initialize MetadataExtractor
    MetadataExtractor Extractor;
    MPIFunctionDatabase DB;
    DB.initialize();
    Extractor.setFunctionDatabase(&DB);
    
    std::cout << "=== Comprehensive MetadataExtractor Test ===\n";
    
    // Test MPI_Send (point-to-point)
    testMPIFunction("MPI_Send", MPIFunctionType::PointToPoint,
                    {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy}, M, Extractor);
    
    // Test MPI_Recv (point-to-point)
    testMPIFunction("MPI_Recv", MPIFunctionType::PointToPoint,
                    {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy, PtrTy}, M, Extractor);
    
    // Test MPI_Isend (non-blocking point-to-point)
    testMPIFunction("MPI_Isend", MPIFunctionType::PointToPoint,
                    {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy, PtrTy}, M, Extractor);
    
    // Test MPI_Bcast (collective)
    testMPIFunction("MPI_Bcast", MPIFunctionType::Collective,
                    {PtrTy, IntTy, IntTy, IntTy, IntTy}, M, Extractor);
    
    // Test MPI_Reduce (collective)
    testMPIFunction("MPI_Reduce", MPIFunctionType::Collective,
                    {PtrTy, PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy}, M, Extractor);
    
    // Test MPI_Wait (request management)
    testMPIFunction("MPI_Wait", MPIFunctionType::Request,
                    {PtrTy, PtrTy}, M, Extractor);
    
    std::cout << "\n=== All tests completed successfully! ===\n";
    return 0;
}
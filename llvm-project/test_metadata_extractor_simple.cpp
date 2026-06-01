//===- test_metadata_extractor_simple.cpp - Simple MetadataExtractor test -===//
//
// Simple test to verify MetadataExtractor functionality
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

int main() {
    LLVMContext Context;
    Module M("test_module", Context);
    
    // Create a simple MPI_Send function signature
    Type* VoidTy = Type::getVoidTy(Context);
    Type* IntTy = Type::getInt32Ty(Context);
    Type* PtrTy = PointerType::get(Context, 0); // Use new opaque pointer API
    
    std::vector<Type*> ParamTypes = {
        PtrTy,  // buffer
        IntTy,  // count
        IntTy,  // datatype
        IntTy,  // dest
        IntTy,  // tag
        IntTy   // communicator
    };
    
    FunctionType* MPISendTy = FunctionType::get(IntTy, ParamTypes, false);
    Function* MPISendFunc = Function::Create(MPISendTy, Function::ExternalLinkage, "MPI_Send", &M);
    
    // Create a test function that calls MPI_Send
    FunctionType* TestFuncTy = FunctionType::get(VoidTy, {}, false);
    Function* TestFunc = Function::Create(TestFuncTy, Function::ExternalLinkage, "test_func", &M);
    
    BasicBlock* BB = BasicBlock::Create(Context, "entry", TestFunc);
    IRBuilder<> Builder(BB);
    
    // Create some dummy values for MPI_Send call
    Value* Buffer = Builder.CreateAlloca(IntTy, Builder.getInt32(100));
    Value* Count = Builder.getInt32(100);
    Value* Datatype = Builder.getInt32(1); // MPI_INT
    Value* Dest = Builder.getInt32(1);
    Value* Tag = Builder.getInt32(0);
    Value* Comm = Builder.getInt32(0); // MPI_COMM_WORLD
    
    std::vector<Value*> Args = {Buffer, Count, Datatype, Dest, Tag, Comm};
    CallInst* MPICall = Builder.CreateCall(MPISendFunc, Args);
    
    Builder.CreateRetVoid();
    
    // Test MetadataExtractor
    std::cout << "Testing MetadataExtractor...\n";
    
    MetadataExtractor Extractor;
    MPIFunctionDatabase DB;
    DB.initialize();
    Extractor.setFunctionDatabase(&DB);
    
    // Create a CallSite for testing
    CallSite Site(MPICall, "MPI_Send", MPIFunctionType::PointToPoint, false);
    
    // Extract metadata
    MPICallMetadata Metadata = Extractor.extractMetadata(Site);
    
    std::cout << "Function: " << Metadata.FunctionName.str() << "\n";
    std::cout << "Parameters: " << Metadata.Parameters.size() << "\n";
    std::cout << "Parameter infos: " << Metadata.ParameterInfos.size() << "\n";
    
    // Test specific extractors
    Value* Communicator = Extractor.extractCommunicator(Site);
    std::vector<Value*> BufferInfo = Extractor.extractBufferInfo(Site);
    Value* Request = Extractor.extractRequestHandle(Site);
    
    std::cout << "Communicator extracted: " << (Communicator ? "Yes" : "No") << "\n";
    std::cout << "Buffer info count: " << BufferInfo.size() << "\n";
    std::cout << "Request extracted: " << (Request ? "Yes" : "No") << "\n";
    
    // Print parameter roles
    for (size_t i = 0; i < Metadata.ParameterInfos.size(); ++i) {
        const auto& Info = Metadata.ParameterInfos[i];
        std::cout << "Parameter " << i << " role: ";
        switch (Info.Role) {
            case ParameterRole::Buffer: std::cout << "Buffer"; break;
            case ParameterRole::Count: std::cout << "Count"; break;
            case ParameterRole::Datatype: std::cout << "Datatype"; break;
            case ParameterRole::Destination: std::cout << "Destination"; break;
            case ParameterRole::Tag: std::cout << "Tag"; break;
            case ParameterRole::Communicator: std::cout << "Communicator"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << " (Input: " << (Info.IsInput ? "Yes" : "No") 
                  << ", Output: " << (Info.IsOutput ? "Yes" : "No") << ")\n";
    }
    
    std::cout << "MetadataExtractor test completed successfully!\n";
    return 0;
}
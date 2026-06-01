// Simple test to verify MPI function database functionality
// This is a conceptual test - actual unit tests would be in unittests/

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPIFunctionDatabase.h"
#include <iostream>
#include <cassert>

using namespace llvm;

int main() {
    MPIFunctionDatabase db;
    db.initialize();
    
    // Test basic MPI function recognition
    assert(db.isMPIFunction("MPI_Init"));
    assert(db.isMPIFunction("MPI_Finalize"));
    assert(db.isMPIFunction("MPI_Send"));
    assert(db.isMPIFunction("MPI_Recv"));
    assert(db.isMPIFunction("MPI_Bcast"));
    assert(db.isMPIFunction("MPI_Barrier"));
    
    // Test non-MPI functions are not recognized
    assert(!db.isMPIFunction("printf"));
    assert(!db.isMPIFunction("malloc"));
    assert(!db.isMPIFunction("free"));
    
    // Test function classification
    assert(db.classifyFunction("MPI_Init") == MPIFunctionType::Environment);
    assert(db.classifyFunction("MPI_Send") == MPIFunctionType::PointToPoint);
    assert(db.classifyFunction("MPI_Bcast") == MPIFunctionType::Collective);
    assert(db.classifyFunction("MPI_Comm_create") == MPIFunctionType::Communicator);
    assert(db.classifyFunction("MPI_Type_commit") == MPIFunctionType::Datatype);
    assert(db.classifyFunction("MPI_Wait") == MPIFunctionType::Request);
    
    // Test function signature retrieval
    const MPIFunctionSignature* sig = db.getFunctionSignature("MPI_Send");
    assert(sig != nullptr);
    assert(sig->Name == "MPI_Send");
    assert(sig->Type == MPIFunctionType::PointToPoint);
    assert(sig->SourceLanguage == Language::C);
    assert(!sig->IsCollective);
    assert(!sig->IsNonBlocking);
    
    // Test collective function
    sig = db.getFunctionSignature("MPI_Bcast");
    assert(sig != nullptr);
    assert(sig->IsCollective);
    assert(!sig->IsNonBlocking);
    
    // Test non-blocking function
    sig = db.getFunctionSignature("MPI_Isend");
    assert(sig != nullptr);
    assert(!sig->IsCollective);
    assert(sig->IsNonBlocking);
    
    // Test parameter information
    assert(sig->Parameters.size() > 0);
    
    // Test functions by type
    auto pointToPointFuncs = db.getFunctionsByType(MPIFunctionType::PointToPoint);
    assert(pointToPointFuncs.size() > 0);
    
    auto collectiveFuncs = db.getFunctionsByType(MPIFunctionType::Collective);
    assert(collectiveFuncs.size() > 0);
    
    std::cout << "All MPI Function Database tests passed!" << std::endl;
    std::cout << "Total functions loaded: " << pointToPointFuncs.size() + collectiveFuncs.size() << std::endl;
    
    return 0;
}
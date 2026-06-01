#include "llvm-project/llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPIFunctionDatabase.h"
#include <iostream>
#include <cassert>

using namespace llvm;

int main() {
    MPIFunctionDatabase db;
    db.initialize();
    
    // Test C MPI functions
    assert(db.isMPIFunction("MPI_Init"));
    assert(db.isMPIFunction("MPI_Send"));
    assert(db.isMPIFunction("MPI_Recv"));
    assert(db.isMPIFunction("MPI_Bcast"));
    
    // Test Fortran mangled functions
    NameManglingHandler handler;
    
    // Test gfortran mangling
    std::string gfortran_send = handler.mangleFortranName("MPI_Send", FortranCompiler::GFortran);
    std::cout << "GFortran MPI_Send: " << gfortran_send << std::endl;
    
    // Test Intel mangling
    std::string intel_send = handler.mangleFortranName("MPI_Send", FortranCompiler::Intel);
    std::cout << "Intel MPI_Send: " << intel_send << std::endl;
    
    // Test PGI mangling
    std::string pgi_send = handler.mangleFortranName("MPI_Send", FortranCompiler::PGI);
    std::cout << "PGI MPI_Send: " << pgi_send << std::endl;
    
    // Test Fortran 2008 module functions
    assert(db.isMPIFunction("__mpi_f08_MOD_mpi_init"));
    assert(db.isMPIFunction("__mpi_f08_MOD_mpi_send"));
    assert(db.isMPIFunction("__mpi_f08_MOD_mpi_recv"));
    
    // Test all mangled variants
    std::vector<std::string> variants = handler.getAllMangledVariants("MPI_Send");
    std::cout << "All MPI_Send variants:" << std::endl;
    for (const auto& variant : variants) {
        std::cout << "  " << variant << std::endl;
    }
    
    // Test function classification
    assert(db.classifyFunction("MPI_Send") == MPIFunctionType::PointToPoint);
    assert(db.classifyFunction("MPI_Bcast") == MPIFunctionType::Collective);
    assert(db.classifyFunction("MPI_Wait") == MPIFunctionType::Request);
    
    // Test Fortran-specific functions
    const MPIFunctionSignature* sig = db.getFunctionSignature("__mpi_f08_MOD_mpi_send");
    if (sig) {
        std::cout << "Found Fortran 2008 MPI_Send signature with " << sig->Parameters.size() << " parameters" << std::endl;
        assert(sig->SourceLanguage == Language::Fortran);
        assert(sig->Type == MPIFunctionType::PointToPoint);
    }
    
    std::cout << "All tests passed! Enhanced Fortran MPI function database is working correctly." << std::endl;
    return 0;
}
//===- test_fortran_enhanced_support.cpp - Test Enhanced Fortran Support ===//
//
// Test program to validate the enhanced Fortran binding support in the
// MPI Usage Sanitizer LLVM Pass, including extended name mangling,
// array descriptors, derived types, and parameter passing conventions.
//
//===----------------------------------------------------------------------===//

#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MPIFunctionDatabase.h"
#include "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/MetadataExtractor.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include <iostream>
#include <vector>
#include <string>

using namespace llvm;

void testExtendedNameMangling() {
    std::cout << "=== Testing Extended Name Mangling Support ===\n";
    
    NameManglingHandler handler;
    std::string baseName = "MPI_Send";
    
    // Test all supported compilers
    std::vector<std::pair<FortranCompiler, std::string>> compilers = {
        {FortranCompiler::GFortran, "GFortran"},
        {FortranCompiler::Intel, "Intel"},
        {FortranCompiler::PGI, "PGI/NVIDIA"},
        {FortranCompiler::Flang, "Flang"},
        {FortranCompiler::NAG, "NAG"},
        {FortranCompiler::Cray, "Cray"},
        {FortranCompiler::IBM, "IBM XL"}
    };
    
    for (const auto& [compiler, name] : compilers) {
        std::string mangled = handler.mangleFortranName(baseName, compiler);
        std::cout << name << " mangling: " << baseName << " -> " << mangled << "\n";
        
        // Test demangling
        std::string demangled = handler.demangleFortranName(mangled);
        std::cout << "  Demangled: " << mangled << " -> " << demangled << "\n";
        
        // Test compiler detection
        FortranCompiler detected = handler.detectCompiler(mangled);
        std::cout << "  Detected compiler: " << (detected == compiler ? "CORRECT" : "INCORRECT") << "\n";
    }
    
    // Test all mangled variants
    std::vector<std::string> variants = handler.getAllMangledVariants(baseName);
    std::cout << "\nAll mangled variants for " << baseName << ":\n";
    for (const auto& variant : variants) {
        std::cout << "  " << variant << "\n";
    }
    
    std::cout << "Total variants: " << variants.size() << "\n\n";
}

void testFortranMPIInterfaces() {
    std::cout << "=== Testing Fortran MPI Interface Support ===\n";
    
    NameManglingHandler handler;
    std::string baseName = "MPI_Bcast";
    
    // Test different Fortran MPI interfaces
    std::vector<FortranCompiler> compilers = {
        FortranCompiler::GFortran,
        FortranCompiler::Intel,
        FortranCompiler::PGI,
        FortranCompiler::Cray,
        FortranCompiler::IBM
    };
    
    for (auto compiler : compilers) {
        std::cout << "Compiler: " << static_cast<int>(compiler) << "\n";
        
        // Test traditional mpif.h interface
        std::string mpifh = handler.handleMPIFH(baseName, compiler);
        std::cout << "  mpif.h: " << mpifh << "\n";
        
        // Test Fortran 90 module interface
        std::string f90mod = handler.handleMPIModule(baseName, compiler);
        std::cout << "  F90 module: " << f90mod << "\n";
        
        // Test Fortran 2008 mpi_f08 interface
        std::string f08mod = handler.handleMPIF08Module(baseName, compiler);
        std::cout << "  F2008 module: " << f08mod << "\n";
        
        // Test C interoperability
        std::string cinterop = handler.handleCInteropMangling(baseName, compiler);
        std::cout << "  C interop: " << cinterop << "\n";
    }
    std::cout << "\n";
}

void testFortranParameterConventions() {
    std::cout << "=== Testing Fortran Parameter Passing Conventions ===\n";
    
    NameManglingHandler handler;
    std::string baseName = "MPI_Send_char";
    
    std::vector<FortranCompiler> compilers = {
        FortranCompiler::GFortran,
        FortranCompiler::Intel,
        FortranCompiler::PGI,
        FortranCompiler::Cray,
        FortranCompiler::IBM
    };
    
    for (auto compiler : compilers) {
        std::cout << "Compiler: " << static_cast<int>(compiler) << "\n";
        
        // Test parameter passing mangling
        std::string paramPassing = handler.applyParameterPassingMangling(baseName, compiler);
        std::cout << "  Parameter passing: " << paramPassing << "\n";
        
        // Test character length mangling
        std::string charLength = handler.applyCharacterLengthMangling(baseName, compiler);
        std::cout << "  Character length: " << charLength << "\n";
        
        // Test optional parameter mangling
        std::string optional = handler.applyOptionalParameterMangling(baseName, compiler);
        std::cout << "  Optional parameter: " << optional << "\n";
    }
    std::cout << "\n";
}

void testFortranArrayDescriptors() {
    std::cout << "=== Testing Fortran Array Descriptor Support ===\n";
    
    NameManglingHandler handler;
    std::string baseName = "MPI_Send_array";
    
    std::vector<FortranCompiler> compilers = {
        FortranCompiler::GFortran,
        FortranCompiler::Intel,
        FortranCompiler::PGI,
        FortranCompiler::Cray,
        FortranCompiler::IBM
    };
    
    for (auto compiler : compilers) {
        std::cout << "Compiler: " << static_cast<int>(compiler) << "\n";
        
        // Test array descriptor mangling
        switch (compiler) {
            case FortranCompiler::GFortran: {
                std::string gfortranDesc = handler.applyGfortranArrayDescriptorMangling(baseName);
                std::cout << "  GFortran array descriptor: " << gfortranDesc << "\n";
                break;
            }
            case FortranCompiler::Intel: {
                std::string intelDesc = handler.applyIntelArrayDescriptorMangling(baseName);
                std::cout << "  Intel array descriptor: " << intelDesc << "\n";
                break;
            }
            case FortranCompiler::PGI: {
                std::string pgiDesc = handler.applyPGIArrayDescriptorMangling(baseName);
                std::cout << "  PGI array descriptor: " << pgiDesc << "\n";
                break;
            }
            case FortranCompiler::Cray: {
                std::string crayDesc = handler.applyCrayArrayDescriptorMangling(baseName);
                std::cout << "  Cray array descriptor: " << crayDesc << "\n";
                break;
            }
            case FortranCompiler::IBM: {
                std::string ibmDesc = handler.applyIBMArrayDescriptorMangling(baseName);
                std::cout << "  IBM array descriptor: " << ibmDesc << "\n";
                break;
            }
            default:
                break;
        }
    }
    std::cout << "\n";
}

void testFortranFunctionDatabase() {
    std::cout << "=== Testing Enhanced Fortran Function Database ===\n";
    
    MPIFunctionDatabase db;
    db.initialize();
    
    // Test various Fortran function variants
    std::vector<std::string> testFunctions = {
        "MPI_Init",
        "mpi_init_",
        "mpi_init__",
        "MPI_INIT",
        "__mpi_MOD_mpi_init_",
        "__mpi_f08_MOD_mpi_init_",
        "mpi_mp_mpi_init_",
        "CCE_MPI_INIT",
        "_xl_mpi_init_"
    };
    
    for (const auto& funcName : testFunctions) {
        bool isMPI = db.isMPIFunction(funcName);
        std::cout << funcName << ": " << (isMPI ? "RECOGNIZED" : "NOT RECOGNIZED") << "\n";
        
        if (isMPI) {
            const MPIFunctionSignature* sig = db.getFunctionSignature(funcName);
            if (sig) {
                std::cout << "  Type: " << static_cast<int>(sig->Type) << "\n";
                std::cout << "  Language: " << static_cast<int>(sig->SourceLanguage) << "\n";
                std::cout << "  Parameters: " << sig->Parameters.size() << "\n";
            }
        }
    }
    std::cout << "\n";
}

void testMetadataExtractorFortranSupport() {
    std::cout << "=== Testing MetadataExtractor Fortran Support ===\n";
    
    LLVMContext Context;
    Module M("test", Context);
    IRBuilder<> Builder(Context);
    
    // Create a simple function to test with
    FunctionType* FT = FunctionType::get(Builder.getVoidTy(), {}, false);
    Function* F = Function::Create(FT, Function::ExternalLinkage, "test_func", &M);
    BasicBlock* BB = BasicBlock::Create(Context, "entry", F);
    Builder.SetInsertPoint(BB);
    
    // Create some test types
    Type* IntTy = Builder.getInt32Ty();
    Type* PtrTy = Builder.getPtrTy();
    Type* BoolTy = Builder.getInt1Ty();
    
    // Create a mock call instruction for testing
    std::vector<Type*> ParamTypes = {PtrTy, IntTy, IntTy, IntTy, IntTy, IntTy, PtrTy};
    FunctionType* MPIFuncTy = FunctionType::get(IntTy, ParamTypes, false);
    Function* MPIFunc = Function::Create(MPIFuncTy, Function::ExternalLinkage, "mpi_send_", &M);
    
    std::vector<Value*> Args;
    for (Type* T : ParamTypes) {
        Args.push_back(UndefValue::get(T));
    }
    
    CallInst* CI = Builder.CreateCall(MPIFunc, Args);
    
    // Create CallSite for testing
    CallSite Site;
    Site.CallInst = CI;
    Site.FunctionName = "mpi_send_";
    Site.Type = MPIFunctionType::PointToPoint;
    
    // Test MetadataExtractor
    MetadataExtractor extractor;
    TypeAnalyzer analyzer;
    
    // Test Fortran-specific type analysis
    std::cout << "Testing Fortran type analysis:\n";
    std::cout << "  Array descriptor detection: " << analyzer.isFortranArrayDescriptor(PtrTy) << "\n";
    std::cout << "  Derived type detection: " << analyzer.isFortranDerivedType(PtrTy) << "\n";
    std::cout << "  Character length detection: " << analyzer.isFortranCharacterLength(IntTy, 1) << "\n";
    std::cout << "  Optional present detection: " << analyzer.isFortranOptionalPresent(BoolTy, 2) << "\n";
    
    // Test Fortran parameter passing detection
    bool isFortranPassing = extractor.isFortranParameterPassing(Site);
    std::cout << "  Fortran parameter passing detected: " << isFortranPassing << "\n";
    
    // Test Fortran-specific extraction methods
    std::vector<Value*> arrayDescriptors = extractor.extractFortranArrayDescriptors(Site);
    std::cout << "  Array descriptors found: " << arrayDescriptors.size() << "\n";
    
    std::vector<Value*> charLengths = extractor.extractCharacterLengths(Site);
    std::cout << "  Character lengths found: " << charLengths.size() << "\n";
    
    std::vector<Value*> optionalFlags = extractor.extractOptionalPresenceFlags(Site);
    std::cout << "  Optional flags found: " << optionalFlags.size() << "\n";
    
    std::vector<Value*> derivedTypes = extractor.extractDerivedTypeInfo(Site);
    std::cout << "  Derived types found: " << derivedTypes.size() << "\n";
    
    std::vector<Value*> references = extractor.handleFortranPassByReference(Site);
    std::cout << "  Pass-by-reference parameters: " << references.size() << "\n";
    
    Builder.CreateRetVoid();
    std::cout << "\n";
}

void testManglingConventions() {
    std::cout << "=== Testing Mangling Convention Support ===\n";
    
    NameManglingHandler handler;
    
    std::vector<std::pair<ManglingConvention, std::string>> conventions = {
        {ManglingConvention::Underscore, "name_"},
        {ManglingConvention::DoubleUnderscore, "name__"},
        {ManglingConvention::NoUnderscore, "name"},
        {ManglingConvention::UpperCase, "NAME"},
        {ManglingConvention::UpperCaseUnderscore, "NAME_"},
        {ManglingConvention::CrayStyle, "CRAY_NAME"},
        {ManglingConvention::IBMStyle, "_xl_name_"}
    };
    
    for (const auto& [convention, example] : conventions) {
        bool matches = handler.matchesManglingPattern(example, convention);
        std::cout << "Convention " << static_cast<int>(convention) 
                  << " matches '" << example << "': " << (matches ? "YES" : "NO") << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "Enhanced Fortran Binding Support Test\n";
    std::cout << "=====================================\n\n";
    
    try {
        testExtendedNameMangling();
        testFortranMPIInterfaces();
        testFortranParameterConventions();
        testFortranArrayDescriptors();
        testFortranFunctionDatabase();
        testMetadataExtractorFortranSupport();
        testManglingConventions();
        
        std::cout << "All tests completed successfully!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
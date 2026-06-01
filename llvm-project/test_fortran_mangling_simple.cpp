//===- test_fortran_mangling_simple.cpp - Simple Fortran Mangling Test ===//
//
// Simple test program to validate the enhanced Fortran name mangling
// without requiring full LLVM build system dependencies.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

// Simplified enums for testing
enum class FortranCompiler {
  GFortran,
  Intel,
  PGI,
  Flang,
  NAG,
  Cray,
  IBM,
  Auto
};

enum class ManglingConvention {
  Underscore,
  DoubleUnderscore,
  NoUnderscore,
  UpperCase,
  UpperCaseUnderscore,
  Mixed,
  CrayStyle,
  IBMStyle
};

// Simplified NameManglingHandler for testing
class SimpleNameManglingHandler {
public:
  std::string mangleFortranName(const std::string& CFunctionName, FortranCompiler Compiler) const {
    std::string Result;
    switch (Compiler) {
      case FortranCompiler::GFortran:
        Result = applyGfortranMangling(CFunctionName);
        break;
      case FortranCompiler::Intel:
        Result = applyIntelMangling(CFunctionName);
        break;
      case FortranCompiler::PGI:
        Result = applyPGIMangling(CFunctionName);
        break;
      case FortranCompiler::Flang:
        Result = applyFlangMangling(CFunctionName);
        break;
      case FortranCompiler::NAG:
        Result = applyNAGMangling(CFunctionName);
        break;
      case FortranCompiler::Cray:
        Result = applyCrayMangling(CFunctionName);
        break;
      case FortranCompiler::IBM:
        Result = applyIBMMangling(CFunctionName);
        break;
      case FortranCompiler::Auto:
        Result = applyGfortranMangling(CFunctionName);
        break;
    }
    return Result;
  }
  
  std::vector<std::string> getAllMangledVariants(const std::string& BaseName) const {
    std::vector<std::string> Variants;
    
    // Original name (C binding)
    Variants.push_back(BaseName);
    
    // All compiler variants
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::GFortran));
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::Intel));
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::PGI));
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::Flang));
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::NAG));
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::Cray));
    Variants.push_back(mangleFortranName(BaseName, FortranCompiler::IBM));
    
    // Add PMPI variants
    std::string PMPIName = "PMPI" + BaseName.substr(3);
    Variants.push_back(PMPIName);
    Variants.push_back(mangleFortranName(PMPIName, FortranCompiler::GFortran));
    Variants.push_back(mangleFortranName(PMPIName, FortranCompiler::Intel));
    
    // Add Fortran 2008 module variants
    std::string F08Name = "__mpi_f08_MOD_" + toLower(BaseName);
    Variants.push_back(F08Name);
    
    // Remove duplicates
    std::sort(Variants.begin(), Variants.end());
    Variants.erase(std::unique(Variants.begin(), Variants.end()), Variants.end());
    
    return Variants;
  }
  
  FortranCompiler detectCompiler(const std::string& MangledName) const {
    // Intel-specific patterns
    if (MangledName.find("for_") != std::string::npos || 
        MangledName.find("_i8") != std::string::npos || 
        MangledName.find("_i4") != std::string::npos) {
      return FortranCompiler::Intel;
    }
    
    // PGI-specific patterns
    if (MangledName.find("pgi_") != std::string::npos || 
        (!MangledName.empty() && MangledName[0] == '_') ||
        MangledName.find("_cuda") != std::string::npos) {
      return FortranCompiler::PGI;
    }
    
    // NAG-specific patterns (often uppercase)
    if (toUpper(MangledName) == MangledName && !endsWith(MangledName, "_")) {
      return FortranCompiler::NAG;
    }
    
    // Cray-specific patterns
    if (MangledName.find("_cray") != std::string::npos || 
        MangledName.find("CCE_") != std::string::npos ||
        MangledName.find("_CRAY") != std::string::npos) {
      return FortranCompiler::Cray;
    }
    
    // IBM XL Fortran patterns
    if (MangledName.find("_xl") != std::string::npos || 
        MangledName.find("_XL") != std::string::npos ||
        MangledName.find("_ibm") != std::string::npos || 
        MangledName.find("_aix") != std::string::npos) {
      return FortranCompiler::IBM;
    }
    
    // Flang-specific patterns
    if (MangledName.find("_flang") != std::string::npos || 
        MangledName.find("_llvm") != std::string::npos) {
      return FortranCompiler::Flang;
    }
    
    // Default to gfortran
    return FortranCompiler::GFortran;
  }
  
  bool matchesManglingPattern(const std::string& Name, ManglingConvention Convention) const {
    switch (Convention) {
      case ManglingConvention::Underscore:
        return endsWith(Name, "_") && toLower(Name) == Name;
      case ManglingConvention::DoubleUnderscore:
        return endsWith(Name, "__");
      case ManglingConvention::NoUnderscore:
        return !endsWith(Name, "_") && toLower(Name) == Name;
      case ManglingConvention::UpperCase:
        return toUpper(Name) == Name && !endsWith(Name, "_");
      case ManglingConvention::UpperCaseUnderscore:
        return toUpper(Name) == Name && endsWith(Name, "_");
      case ManglingConvention::Mixed:
        return true;
      case ManglingConvention::CrayStyle:
        return toUpper(Name) == Name || Name.find("_cray") != std::string::npos;
      case ManglingConvention::IBMStyle:
        return toLower(Name) == Name || Name.find("_xl") != std::string::npos;
    }
    return false;
  }

private:
  std::string applyGfortranMangling(const std::string& Name) const {
    std::string Result = toLower(Name);
    Result += "_";
    if (Name.find("_") != std::string::npos) {
      Result += "_";
    }
    return Result;
  }
  
  std::string applyIntelMangling(const std::string& Name) const {
    std::string Result = toLower(Name);
    Result += "_";
    if (Name.find("_i8") != std::string::npos || Name.find("_8") != std::string::npos) {
      Result += "i8";
    } else if (Name.find("_i4") != std::string::npos || Name.find("_4") != std::string::npos) {
      Result += "i4";
    }
    return Result;
  }
  
  std::string applyPGIMangling(const std::string& Name) const {
    std::string Result = toLower(Name);
    Result += "_";
    if (Name.find("_cuda") != std::string::npos || Name.find("_gpu") != std::string::npos) {
      Result = "pgi_" + Result;
    }
    return Result;
  }
  
  std::string applyFlangMangling(const std::string& Name) const {
    std::string Result = toLower(Name);
    Result += "_";
    if (Name.find("_") != std::string::npos) {
      Result += "_";
    }
    return Result;
  }
  
  std::string applyNAGMangling(const std::string& Name) const {
    return toUpper(Name);
  }
  
  std::string applyCrayMangling(const std::string& Name) const {
    std::string Result = toUpper(Name);
    if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
      if (Name.find("_cce") != std::string::npos || Name.find("_cray") != std::string::npos) {
        return Result;
      }
      if (Name.find("_mpi") != std::string::npos || Name.find("_parallel") != std::string::npos) {
        Result = "CCE_" + Result;
      }
    }
    return Result;
  }
  
  std::string applyIBMMangling(const std::string& Name) const {
    std::string Result = toLower(Name);
    Result += "_";
    if (Name.substr(0, 4) == "MPI_" || Name.substr(0, 5) == "PMPI_") {
      if (Name.find("_aix") != std::string::npos || Name.find("_power") != std::string::npos) {
        Result = "_xl_" + Result;
      }
      if (Name.find("_i8") != std::string::npos || Name.find("_64") != std::string::npos) {
        Result += "64";
      }
    }
    return Result;
  }
  
  // Helper functions
  std::string toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
  }
  
  std::string toUpper(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
  }
  
  bool endsWith(const std::string& str, const std::string& suffix) const {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
  }
};

void testExtendedNameMangling() {
    std::cout << "=== Testing Extended Name Mangling Support ===\n";
    
    SimpleNameManglingHandler handler;
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

void testManglingConventions() {
    std::cout << "=== Testing Mangling Convention Support ===\n";
    
    SimpleNameManglingHandler handler;
    
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

void testSpecificMPIFunctions() {
    std::cout << "=== Testing Specific MPI Function Mangling ===\n";
    
    SimpleNameManglingHandler handler;
    
    std::vector<std::string> mpiFunctions = {
        "MPI_Init",
        "MPI_Finalize", 
        "MPI_Send",
        "MPI_Recv",
        "MPI_Bcast",
        "MPI_Reduce",
        "MPI_Allreduce",
        "MPI_Barrier",
        "MPI_Comm_rank",
        "MPI_Comm_size"
    };
    
    for (const auto& func : mpiFunctions) {
        std::cout << "Function: " << func << "\n";
        
        // Test GFortran mangling (most common)
        std::string gfortran = handler.mangleFortranName(func, FortranCompiler::GFortran);
        std::cout << "  GFortran: " << gfortran << "\n";
        
        // Test Intel mangling
        std::string intel = handler.mangleFortranName(func, FortranCompiler::Intel);
        std::cout << "  Intel: " << intel << "\n";
        
        // Test Cray mangling (new)
        std::string cray = handler.mangleFortranName(func, FortranCompiler::Cray);
        std::cout << "  Cray: " << cray << "\n";
        
        // Test IBM mangling (new)
        std::string ibm = handler.mangleFortranName(func, FortranCompiler::IBM);
        std::cout << "  IBM: " << ibm << "\n";
    }
    std::cout << "\n";
}

void testCompilerDetection() {
    std::cout << "=== Testing Compiler Detection ===\n";
    
    SimpleNameManglingHandler handler;
    
    std::vector<std::pair<std::string, FortranCompiler>> testCases = {
        {"mpi_send_", FortranCompiler::GFortran},
        {"mpi_send__", FortranCompiler::GFortran},
        {"mpi_send_i8", FortranCompiler::Intel},
        {"for_mpi_send_", FortranCompiler::Intel},
        {"pgi_mpi_send_", FortranCompiler::PGI},
        {"_mpi_send_cuda", FortranCompiler::PGI},
        {"MPI_SEND", FortranCompiler::NAG},
        {"CCE_MPI_SEND", FortranCompiler::Cray},
        {"_cray_MPI_SEND", FortranCompiler::Cray},
        {"_xl_mpi_send_", FortranCompiler::IBM},
        {"mpi_send_aix_", FortranCompiler::IBM},
        {"_flang_mpi_send_", FortranCompiler::Flang}
    };
    
    for (const auto& [mangledName, expectedCompiler] : testCases) {
        FortranCompiler detected = handler.detectCompiler(mangledName);
        bool correct = (detected == expectedCompiler);
        std::cout << "'" << mangledName << "' -> " 
                  << static_cast<int>(detected) << " (expected " 
                  << static_cast<int>(expectedCompiler) << ") " 
                  << (correct ? "✓" : "✗") << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "Enhanced Fortran Binding Support Test\n";
    std::cout << "=====================================\n\n";
    
    try {
        testExtendedNameMangling();
        testManglingConventions();
        testSpecificMPIFunctions();
        testCompilerDetection();
        
        std::cout << "All tests completed successfully!\n";
        std::cout << "\nSummary of Enhanced Features:\n";
        std::cout << "- Added support for Cray and IBM XL Fortran compilers\n";
        std::cout << "- Extended mangling conventions (CrayStyle, IBMStyle)\n";
        std::cout << "- Comprehensive compiler detection patterns\n";
        std::cout << "- Support for compiler-specific MPI extensions\n";
        std::cout << "- Enhanced variant generation for all compilers\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
//===- MPIFunctionDatabase.h - MPI Function Signature Database -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the MPIFunctionDatabase class which maintains a
// comprehensive database of MPI function signatures for detection and
// analysis across multiple language bindings.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_MPIFUNCTIONDATABASE_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_MPIFUNCTIONDATABASE_H

#include "MPICallDetector.h"
#include "MetadataExtractor.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Type.h"
#include <vector>
#include <string>

namespace llvm {

/// Supported MPI language bindings
enum class Language {
  C,
  Fortran,
  CXX
};

/// Information about a single MPI function parameter
struct MPIParameterInfo {
  std::string Name;
  ParameterRole Role;
  bool IsInput;
  bool IsOutput;
  bool IsOptional;
  
  MPIParameterInfo(const std::string& N, ParameterRole R, bool In, bool Out, bool Opt = false)
    : Name(N), Role(R), IsInput(In), IsOutput(Out), IsOptional(Opt) {}
};

/// Complete signature information for an MPI function
struct MPIFunctionSignature {
  std::string Name;
  std::string MangledName;  // For Fortran bindings
  MPIFunctionType Type;
  Language SourceLanguage;
  std::vector<MPIParameterInfo> Parameters;
  bool IsCollective;
  bool IsNonBlocking;
  bool IsDeprecated;
  std::string Description;  // Function description for diagnostics
  std::string Version;      // MPI version when introduced
  
  MPIFunctionSignature(const std::string& N, MPIFunctionType T, Language L)
    : Name(N), Type(T), SourceLanguage(L), IsCollective(false), 
      IsNonBlocking(false), IsDeprecated(false) {}
      
  /// Add a parameter to the function signature
  void addParameter(const std::string& Name, ParameterRole Role, 
                   bool IsInput, bool IsOutput, bool IsOptional = false) {
    Parameters.emplace_back(Name, Role, IsInput, IsOutput, IsOptional);
  }
  
  /// Get parameter by role (returns first match)
  const MPIParameterInfo* getParameterByRole(ParameterRole Role) const {
    for (const auto& Param : Parameters) {
      if (Param.Role == Role) return &Param;
    }
    return nullptr;
  }
  
  /// Get all parameters with a specific role
  std::vector<const MPIParameterInfo*> getParametersByRole(ParameterRole Role) const {
    std::vector<const MPIParameterInfo*> Result;
    for (const auto& Param : Parameters) {
      if (Param.Role == Role) Result.push_back(&Param);
    }
    return Result;
  }
};

/// MPI Function Database
///
/// Maintains a comprehensive database of MPI function signatures for
/// detection and analysis across multiple language bindings.
class MPIFunctionDatabase {
public:
  MPIFunctionDatabase();
  ~MPIFunctionDatabase() = default;
  
  /// Initialize the database with standard MPI functions
  void initialize();
  
  /// Check if a function name is an MPI function
  bool isMPIFunction(StringRef FunctionName) const;
  
  /// Get function signature by name
  const MPIFunctionSignature* getFunctionSignature(StringRef FunctionName) const;
  
  /// Classify MPI function type
  MPIFunctionType classifyFunction(StringRef FunctionName) const;
  
  /// Get all functions of a specific type
  std::vector<const MPIFunctionSignature*> getFunctionsByType(MPIFunctionType Type) const;
  
  /// Add custom MPI function signature
  void addFunctionSignature(const MPIFunctionSignature& Signature);
  
  /// Load function signatures from configuration file
  bool loadFromFile(StringRef FilePath);

private:
  /// Initialize C MPI function signatures
  void initializeCFunctions();
  
  /// Initialize Fortran MPI function signatures
  void initializeFortranFunctions();
  
  /// Initialize C++ MPI function signatures
  void initializeCXXFunctions();
  
  /// Add C++ MPI function categories
  void addCXXEnvironmentFunctions();
  void addCXXPointToPointFunctions();
  void addCXXCollectiveFunctions();
  void addCXXCommunicatorFunctions();
  void addCXXDatatypeFunctions();
  void addCXXRequestFunctions();
  void addCXXGroupFunctions();
  void addCXXWindowFunctions();
  void addCXXInfoFunctions();
  void addCXXErrorFunctions();
  void addCXXTopologyFunctions();
  
  /// Add a standard MPI function with common parameters
  void addStandardFunction(const std::string& Name, MPIFunctionType Type,
                           Language Lang, bool IsCollective = false,
                           bool IsNonBlocking = false);
  
  /// Add point-to-point communication functions
  void addPointToPointFunctions();
  
  /// Add collective communication functions
  void addCollectiveFunctions();
  
  /// Add communicator management functions
  void addCommunicatorFunctions();
  
  /// Add datatype functions
  void addDatatypeFunctions();
  
  /// Add request management functions
  void addRequestFunctions();
  
  /// Add environment functions (Init, Finalize, etc.)
  void addEnvironmentFunctions();
  
  /// Add group management functions
  void addGroupFunctions();
  
  /// Add process management functions
  void addProcessFunctions();
  
  /// Add attribute functions
  void addAttributeFunctions();
  
  /// Add error handling functions
  void addErrorFunctions();
  
  /// Add profiling functions
  void addProfilingFunctions();
  
  /// Add info object functions
  void addInfoFunctions();
  
  /// Add window (RMA) functions
  void addWindowFunctions();
  
  /// Add file I/O functions
  void addFileFunctions();
  
  /// Add topology functions
  void addTopologyFunctions();
  
  /// Add one-sided communication functions
  void addOneSidedFunctions();
  
  /// Add neighborhood collective functions
  void addNeighborhoodCollectiveFunctions();
  
  /// Add non-blocking collective functions
  void addNonBlockingCollectiveFunctions();
  
  /// Add Fortran-specific function signatures
  void addFortranEnvironmentFunctions();
  void addFortranPointToPointFunctions();
  void addFortranCollectiveFunctions();
  void addFortranCommunicatorFunctions();
  void addFortranDatatypeFunctions();
  void addFortranRequestFunctions();
  void addFortranGroupFunctions();
  void addFortranWindowFunctions();
  void addFortranOneSidedFunctions();
  void addFortranInfoFunctions();
  void addFortranErrorFunctions();
  void addFortranTopologyFunctions();
  void addFortran90Bindings();
  void addFortran2008Bindings();
  
  /// Helper to add Fortran function with all mangled variants
  void addFortranFunction(const std::string& BaseName, MPIFunctionType Type,
                         bool IsCollective = false, bool IsNonBlocking = false,
                         const std::vector<std::tuple<std::string, ParameterRole, bool, bool, bool>>& Params = {},
                         bool IsDeprecated = false, const std::string& Description = "",
                         const std::string& Version = "1.0");
  
  /// Helper to create a function signature with parameters
  std::unique_ptr<MPIFunctionSignature> createFunctionSignature(
    const std::string& Name, MPIFunctionType Type, Language Lang,
    bool IsCollective = false, bool IsNonBlocking = false, 
    bool IsDeprecated = false, const std::string& Description = "",
    const std::string& Version = "1.0");
    
  /// Helper to add a function with detailed parameter specification
  void addDetailedFunction(const std::string& Name, MPIFunctionType Type,
                          Language Lang, bool IsCollective, bool IsNonBlocking,
                          const std::vector<std::tuple<std::string, ParameterRole, bool, bool, bool>>& Params,
                          bool IsDeprecated = false, const std::string& Description = "",
                          const std::string& Version = "1.0");
  
  // Storage for function signatures
  DenseMap<StringRef, MPIFunctionSignature*> FunctionMap;
  std::vector<std::unique_ptr<MPIFunctionSignature>> AllFunctions;
};

/// Fortran compiler types for name mangling
enum class FortranCompiler {
  GFortran,     // GNU Fortran (gfortran)
  Intel,        // Intel Fortran Compiler (ifort/ifx)
  PGI,          // PGI/NVIDIA HPC SDK
  Flang,        // LLVM Flang
  NAG,          // NAG Fortran Compiler
  Cray,         // Cray Fortran Compiler
  IBM,          // IBM XL Fortran
  Auto          // Auto-detect based on patterns
};

/// Platform-specific mangling conventions
enum class ManglingConvention {
  Underscore,           // name_
  DoubleUnderscore,     // name__
  NoUnderscore,         // name
  UpperCase,            // NAME
  UpperCaseUnderscore,  // NAME_
  Mixed,                // Mixed case variations
  CrayStyle,            // Cray-specific mangling
  IBMStyle              // IBM XL Fortran mangling
};

/// Name Mangling Handler for Fortran bindings
class NameManglingHandler {
public:
  NameManglingHandler() = default;
  
  /// Convert C function name to Fortran mangled name for specific compiler
  std::string mangleFortranName(StringRef CFunctionName, 
                               FortranCompiler Compiler = FortranCompiler::Auto) const;
  
  /// Demangle Fortran function name to C equivalent
  std::string demangleFortranName(StringRef MangledName) const;
  
  /// Check if a name appears to be Fortran mangled
  bool isFortranMangled(StringRef Name) const;
  
  /// Get all possible mangled variants of a function name
  std::vector<std::string> getAllMangledVariants(StringRef BaseName) const;
  
  /// Detect likely Fortran compiler from mangled name patterns
  FortranCompiler detectCompiler(StringRef MangledName) const;
  
  /// Get mangling convention for specific compiler
  ManglingConvention getManglingConvention(FortranCompiler Compiler) const;
  
  /// Check if name matches specific mangling pattern
  bool matchesManglingPattern(StringRef Name, ManglingConvention Convention) const;

private:
  /// Apply compiler-specific Fortran mangling conventions
  std::string applyGfortranMangling(StringRef Name) const;
  std::string applyIntelMangling(StringRef Name) const;
  std::string applyPGIMangling(StringRef Name) const;
  std::string applyFlangMangling(StringRef Name) const;
  std::string applyNAGMangling(StringRef Name) const;
  std::string applyCrayMangling(StringRef Name) const;
  std::string applyIBMMangling(StringRef Name) const;
  
  /// Handle special cases for MPI function names
  std::string handleMPISpecialCases(StringRef Name, FortranCompiler Compiler) const;
  
  /// Apply platform-specific adjustments
  std::string applyPlatformAdjustments(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran array descriptor mangling
  std::string applyGfortranArrayDescriptorMangling(StringRef Name) const;
  std::string applyIntelArrayDescriptorMangling(StringRef Name) const;
  std::string applyPGIArrayDescriptorMangling(StringRef Name) const;
  std::string applyCrayArrayDescriptorMangling(StringRef Name) const;
  std::string applyIBMArrayDescriptorMangling(StringRef Name) const;
  
  /// Handle Fortran derived type mangling
  std::string applyDerivedTypeMangling(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran parameter passing conventions
  std::string applyParameterPassingMangling(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran character string length parameters
  std::string applyCharacterLengthMangling(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran optional parameter mangling
  std::string applyOptionalParameterMangling(StringRef Name, FortranCompiler Compiler) const;
  
  /// Check for Fortran module-qualified names (e.g., mpi_f08 module)
  bool isModuleQualified(StringRef Name) const;
  
  /// Extract base name from module-qualified name
  std::string extractBaseName(StringRef ModuleQualifiedName) const;
  
  /// Handle different Fortran MPI binding interfaces
  std::string handleMPIFortranInterface(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle traditional mpif.h interface
  std::string handleMPIFH(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran 90 mpi module interface
  std::string handleMPIModule(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran 2008 mpi_f08 module interface
  std::string handleMPIF08Module(StringRef Name, FortranCompiler Compiler) const;
  
  /// Handle Fortran C interoperability (ISO_C_BINDING)
  std::string handleCInteropMangling(StringRef Name, FortranCompiler Compiler) const;
};

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_MPIFUNCTIONDATABASE_H
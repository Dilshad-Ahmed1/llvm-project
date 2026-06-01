//===- RuntimeInterfaceValidator.h - Runtime Interface Validation -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the RuntimeInterfaceValidator class which provides
// comprehensive validation of hook function signatures and runtime library
// interface compatibility for the MPI Usage Sanitizer.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_RUNTIMEINTERFACEVALIDATOR_H
#define LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_RUNTIMEINTERFACEVALIDATOR_H

#include "RuntimeInterface.h"
#include "ErrorHandler.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/VersionTuple.h"
#include <string>
#include <memory>

namespace llvm {

class LLVMContext;

/// Runtime library version information
struct RuntimeVersion {
  /// Major version number
  uint32_t Major = 0;
  
  /// Minor version number
  uint32_t Minor = 0;
  
  /// Patch version number
  uint32_t Patch = 0;
  
  /// Version string representation
  std::string VersionString;
  
  /// ABI version identifier
  uint32_t ABIVersion = 0;
  
  /// Default constructor
  RuntimeVersion() = default;
  
  /// Constructor from version components
  RuntimeVersion(uint32_t Major, uint32_t Minor, uint32_t Patch, uint32_t ABI = 0)
      : Major(Major), Minor(Minor), Patch(Patch), ABIVersion(ABI) {
    VersionString = std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Patch);
  }
  
  /// Parse version from string
  static RuntimeVersion parseFromString(StringRef VersionStr);
  
  /// Check compatibility with another version
  bool isCompatibleWith(const RuntimeVersion& Other) const;
  
  /// Compare versions
  bool operator<(const RuntimeVersion& Other) const;
  bool operator==(const RuntimeVersion& Other) const;
  bool operator!=(const RuntimeVersion& Other) const { return !(*this == Other); }
};

/// Hook function signature specification
struct HookSignature {
  /// Function name
  std::string Name;
  
  /// Return type
  Type* ReturnType = nullptr;
  
  /// Parameter types
  SmallVector<Type*, 8> ParameterTypes;
  
  /// Calling convention
  CallingConv::ID CallingConv = CallingConv::C;
  
  /// Whether function is variadic
  bool IsVariadic = false;
  
  /// Minimum required runtime version
  RuntimeVersion MinVersion;
  
  /// Maximum supported runtime version (empty means no limit)
  RuntimeVersion MaxVersion;
  
  /// Whether this hook is required or optional
  bool IsRequired = true;
  
  /// Hook category for grouping
  std::string Category;
  
  /// Constructor
  HookSignature(StringRef Name, Type* RetType, ArrayRef<Type*> ParamTypes,
                bool Required = true, StringRef Cat = "")
      : Name(Name.str()), ReturnType(RetType), ParameterTypes(ParamTypes.begin(), ParamTypes.end()),
        IsRequired(Required), Category(Cat.str()) {}
};

/// Validation result for a single hook function
struct HookValidationResult {
  /// Hook name
  std::string HookName;
  
  /// Whether validation passed
  bool IsValid = false;
  
  /// Whether hook function was found
  bool Found = false;
  
  /// Validation error messages
  SmallVector<std::string, 4> Errors;
  
  /// Validation warnings
  SmallVector<std::string, 4> Warnings;
  
  /// Expected signature
  const HookSignature* ExpectedSignature = nullptr;
  
  /// Actual function (if found)
  const Function* ActualFunction = nullptr;
  
  /// Constructor
  HookValidationResult(StringRef Name) : HookName(Name.str()) {}
  
  /// Add error message
  void addError(StringRef Message) {
    Errors.push_back(Message.str());
    IsValid = false;
  }
  
  /// Add warning message
  void addWarning(StringRef Message) {
    Warnings.push_back(Message.str());
  }
  
  /// Check if has any issues
  bool hasIssues() const {
    return !Errors.empty() || !Warnings.empty();
  }
};

/// Complete validation result for all hooks
struct InterfaceValidationResult {
  /// Individual hook validation results
  StringMap<HookValidationResult> HookResults;
  
  /// Runtime version information
  RuntimeVersion DetectedVersion;
  
  /// Whether runtime library was found
  bool RuntimeFound = false;
  
  /// Overall validation status
  bool IsValid = false;
  
  /// Global validation errors
  SmallVector<std::string, 4> GlobalErrors;
  
  /// Global validation warnings
  SmallVector<std::string, 4> GlobalWarnings;
  
  /// Statistics
  uint32_t TotalHooks = 0;
  uint32_t ValidHooks = 0;
  uint32_t RequiredHooks = 0;
  uint32_t ValidRequiredHooks = 0;
  
  /// Add global error
  void addGlobalError(StringRef Message) {
    GlobalErrors.push_back(Message.str());
    IsValid = false;
  }
  
  /// Add global warning
  void addGlobalWarning(StringRef Message) {
    GlobalWarnings.push_back(Message.str());
  }
  
  /// Update statistics
  void updateStatistics();
  
  /// Check if validation passed
  bool passed() const {
    return IsValid && RuntimeFound && (ValidRequiredHooks == RequiredHooks);
  }
};

/// Runtime Interface Validator
///
/// Provides comprehensive validation of hook function signatures and runtime
/// library interface compatibility. Validates against expected signatures,
/// checks version compatibility, and ensures ABI compliance.
class RuntimeInterfaceValidator {
public:
  RuntimeInterfaceValidator(LLVMContext& Context, ErrorHandler& ErrorHandler);
  ~RuntimeInterfaceValidator();
  
  /// Initialize validator with runtime interface
  bool initialize(const RuntimeInterface& Interface);
  
  /// Validate all hook functions in a module
  InterfaceValidationResult validateModule(Module& M);
  
  /// Validate specific hook function
  HookValidationResult validateHookFunction(const Function* F, const HookSignature& Expected);
  
  /// Validate hook function signature
  bool validateSignature(const Function* F, const HookSignature& Expected, 
                         HookValidationResult& Result);
  
  /// Detect runtime library version from module
  RuntimeVersion detectRuntimeVersion(Module& M);
  
  /// Check version compatibility
  bool checkVersionCompatibility(const RuntimeVersion& Detected, const RuntimeVersion& Required);
  
  /// Validate ABI compatibility
  bool validateABICompatibility(Module& M, const RuntimeVersion& Version);
  
  /// Add expected hook signature
  void addExpectedHook(const HookSignature& Signature);
  
  /// Remove expected hook signature
  void removeExpectedHook(StringRef HookName);
  
  /// Get expected hook signature
  const HookSignature* getExpectedHook(StringRef HookName) const;
  
  /// Get all expected hooks
  const StringMap<HookSignature>& getExpectedHooks() const { return ExpectedHooks; }
  
  /// Set minimum required runtime version
  void setMinimumRuntimeVersion(const RuntimeVersion& Version) { MinRuntimeVersion = Version; }
  
  /// Set maximum supported runtime version
  void setMaximumRuntimeVersion(const RuntimeVersion& Version) { MaxRuntimeVersion = Version; }
  
  /// Enable/disable strict validation mode
  void setStrictValidation(bool Strict) { StrictValidation = Strict; }
  
  /// Enable/disable ABI validation
  void setABIValidation(bool Enable) { ABIValidation = Enable; }
  
  /// Set runtime library name pattern
  void setRuntimeLibraryPattern(StringRef Pattern) { RuntimeLibraryPattern = Pattern.str(); }
  
  /// Check if hook is required
  bool isHookRequired(StringRef HookName) const;
  
  /// Check if hook is optional
  bool isHookOptional(StringRef HookName) const { return !isHookRequired(HookName); }
  
  /// Get validation statistics
  void getValidationStatistics(const InterfaceValidationResult& Result, raw_ostream& OS) const;
  
  /// Generate validation report
  void generateValidationReport(const InterfaceValidationResult& Result, raw_ostream& OS) const;
  
  /// Create default hook signatures for MPI sanitizer
  void createDefaultHookSignatures();
  
  /// Validate calling convention compatibility
  bool validateCallingConvention(const Function* F, CallingConv::ID Expected) const;
  
  /// Validate parameter types compatibility
  bool validateParameterTypes(const Function* F, ArrayRef<Type*> Expected, 
                              HookValidationResult& Result) const;
  
  /// Validate return type compatibility
  bool validateReturnType(const Function* F, Type* Expected, HookValidationResult& Result) const;
  
  /// Check type compatibility with coercion rules
  bool areTypesCompatible(Type* Actual, Type* Expected) const;
  
  /// Get type compatibility error message
  std::string getTypeCompatibilityError(Type* Actual, Type* Expected) const;

private:
  /// LLVM context
  LLVMContext& Context;
  
  /// Error handler for reporting issues
  ErrorHandler& ErrHandler;
  
  /// Expected hook signatures
  StringMap<HookSignature> ExpectedHooks;
  
  /// Minimum required runtime version
  RuntimeVersion MinRuntimeVersion;
  
  /// Maximum supported runtime version
  RuntimeVersion MaxRuntimeVersion;
  
  /// Validation options
  bool StrictValidation = true;
  bool ABIValidation = true;
  
  /// Runtime library pattern for detection
  std::string RuntimeLibraryPattern = "mpi_sanitizer_runtime";
  
  /// Initialization state
  bool Initialized = false;
  
  /// Find runtime library functions in module
  SmallVector<const Function*, 16> findRuntimeFunctions(Module& M) const;
  
  /// Extract version information from function names or metadata
  RuntimeVersion extractVersionFromModule(Module& M) const;
  
  /// Validate function linkage and visibility
  bool validateFunctionLinkage(const Function* F, HookValidationResult& Result) const;
  
  /// Check for ABI-breaking changes
  bool checkABIBreakingChanges(const Function* F, const HookSignature& Expected) const;
  
  /// Generate type mismatch diagnostic
  void generateTypeMismatchDiagnostic(const Function* F, const HookSignature& Expected,
                                      HookValidationResult& Result) const;
  
  /// Validate function attributes
  bool validateFunctionAttributes(const Function* F, const HookSignature& Expected,
                                  HookValidationResult& Result) const;
  
  /// Check for deprecated hook functions
  bool checkDeprecatedHooks(const Function* F, HookValidationResult& Result) const;
  
  /// Validate hook function naming conventions
  bool validateNamingConventions(const Function* F, HookValidationResult& Result) const;
};

/// Utility functions for runtime interface validation

/// Create standard MPI sanitizer hook signatures
StringMap<HookSignature> createStandardHookSignatures(LLVMContext& Context);

/// Parse runtime version from string representation
RuntimeVersion parseRuntimeVersion(StringRef VersionString);

/// Format runtime version as string
std::string formatRuntimeVersion(const RuntimeVersion& Version);

/// Check if two function types are ABI compatible
bool areABICompatible(FunctionType* Actual, FunctionType* Expected);

/// Get ABI compatibility error description
std::string getABICompatibilityError(FunctionType* Actual, FunctionType* Expected);

/// Validate hook function naming pattern
bool validateHookNaming(StringRef FunctionName, StringRef ExpectedPattern);

/// Extract ABI version from module metadata
uint32_t extractABIVersion(Module& M);

} // namespace llvm

#endif // LLVM_LIB_TRANSFORMS_INSTRUMENTATION_MPIUSAGESANITIZER_RUNTIMEINTERFACEVALIDATOR_H
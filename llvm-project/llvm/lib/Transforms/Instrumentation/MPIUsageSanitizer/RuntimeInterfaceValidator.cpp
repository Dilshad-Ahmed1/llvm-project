//===- RuntimeInterfaceValidator.cpp - Runtime Interface Validation -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the RuntimeInterfaceValidator class which provides
// comprehensive validation of hook function signatures and runtime library
// interface compatibility for the MPI Usage Sanitizer.
//
//===----------------------------------------------------------------------===//

#include "RuntimeInterfaceValidator.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringExtras.h"
#include <regex>
#include <sstream>

using namespace llvm;

#define DEBUG_TYPE "mpi-runtime-validator"

//===----------------------------------------------------------------------===//
// RuntimeVersion Implementation
//===----------------------------------------------------------------------===//

RuntimeVersion RuntimeVersion::parseFromString(StringRef VersionStr) {
  RuntimeVersion Version;
  
  // Parse version string in format "major.minor.patch" or "major.minor.patch-abi"
  SmallVector<StringRef, 4> Parts;
  VersionStr.split(Parts, '.');
  
  if (Parts.size() >= 1) {
    Parts[0].getAsInteger(10, Version.Major);
  }
  if (Parts.size() >= 2) {
    Parts[1].getAsInteger(10, Version.Minor);
  }
  if (Parts.size() >= 3) {
    // Handle patch version with optional ABI suffix
    StringRef PatchPart = Parts[2];
    size_t DashPos = PatchPart.find('-');
    if (DashPos != StringRef::npos) {
      PatchPart.substr(0, DashPos).getAsInteger(10, Version.Patch);
      PatchPart.substr(DashPos + 1).getAsInteger(10, Version.ABIVersion);
    } else {
      PatchPart.getAsInteger(10, Version.Patch);
    }
  }
  
  Version.VersionString = VersionStr.str();
  return Version;
}

bool RuntimeVersion::isCompatibleWith(const RuntimeVersion& Other) const {
  // Major version must match for ABI compatibility
  if (Major != Other.Major) {
    return false;
  }
  
  // Minor version must be >= for backward compatibility
  if (Minor < Other.Minor) {
    return false;
  }
  
  // ABI version must match if specified
  if (ABIVersion != 0 && Other.ABIVersion != 0 && ABIVersion != Other.ABIVersion) {
    return false;
  }
  
  return true;
}

bool RuntimeVersion::operator<(const RuntimeVersion& Other) const {
  if (Major != Other.Major) return Major < Other.Major;
  if (Minor != Other.Minor) return Minor < Other.Minor;
  if (Patch != Other.Patch) return Patch < Other.Patch;
  return ABIVersion < Other.ABIVersion;
}

bool RuntimeVersion::operator==(const RuntimeVersion& Other) const {
  return Major == Other.Major && Minor == Other.Minor && 
         Patch == Other.Patch && ABIVersion == Other.ABIVersion;
}

//===----------------------------------------------------------------------===//
// InterfaceValidationResult Implementation
//===----------------------------------------------------------------------===//

void InterfaceValidationResult::updateStatistics() {
  TotalHooks = HookResults.size();
  ValidHooks = 0;
  RequiredHooks = 0;
  ValidRequiredHooks = 0;
  
  for (const auto& Result : HookResults) {
    const HookValidationResult& HookResult = Result.second;
    
    if (HookResult.IsValid) {
      ValidHooks++;
    }
    
    if (HookResult.ExpectedSignature && HookResult.ExpectedSignature->IsRequired) {
      RequiredHooks++;
      if (HookResult.IsValid) {
        ValidRequiredHooks++;
      }
    }
  }
  
  // Overall validation passes if all required hooks are valid and no global errors
  IsValid = (ValidRequiredHooks == RequiredHooks) && GlobalErrors.empty();
}

//===----------------------------------------------------------------------===//
// RuntimeInterfaceValidator Implementation
//===----------------------------------------------------------------------===//

RuntimeInterfaceValidator::RuntimeInterfaceValidator(LLVMContext& Context, ErrorHandler& ErrorHandler)
    : Context(Context), ErrHandler(ErrorHandler) {
  LLVM_DEBUG(dbgs() << "Initializing Runtime Interface Validator\n");
  
  // Set default version requirements
  MinRuntimeVersion = RuntimeVersion(1, 0, 0, 1);
  MaxRuntimeVersion = RuntimeVersion(2, 0, 0, 0);
}

RuntimeInterfaceValidator::~RuntimeInterfaceValidator() = default;

bool RuntimeInterfaceValidator::initialize(const RuntimeInterface& Interface) {
  LLVM_DEBUG(dbgs() << "Initializing validator with runtime interface\n");
  
  // Create default hook signatures
  createDefaultHookSignatures();
  
  Initialized = true;
  return true;
}

InterfaceValidationResult RuntimeInterfaceValidator::validateModule(Module& M) {
  InterfaceValidationResult Result;
  
  if (!Initialized) {
    Result.addGlobalError("Validator not initialized");
    return Result;
  }
  
  LLVM_DEBUG(dbgs() << "Validating module: " << M.getName() << "\n");
  
  // Detect runtime version
  Result.DetectedVersion = detectRuntimeVersion(M);
  Result.RuntimeFound = (Result.DetectedVersion.Major > 0);
  
  if (!Result.RuntimeFound) {
    Result.addGlobalWarning("Runtime library not detected in module");
  } else {
    LLVM_DEBUG(dbgs() << "Detected runtime version: " << Result.DetectedVersion.VersionString << "\n");
    
    // Check version compatibility
    if (!checkVersionCompatibility(Result.DetectedVersion, MinRuntimeVersion)) {
      Result.addGlobalError("Runtime version " + Result.DetectedVersion.VersionString + 
                           " is below minimum required version " + MinRuntimeVersion.VersionString);
    }
    
    if (MaxRuntimeVersion.Major > 0 && 
        !checkVersionCompatibility(MaxRuntimeVersion, Result.DetectedVersion)) {
      Result.addGlobalWarning("Runtime version " + Result.DetectedVersion.VersionString + 
                             " is above maximum tested version " + MaxRuntimeVersion.VersionString);
    }
  }
  
  // Validate ABI compatibility if enabled
  if (ABIValidation && Result.RuntimeFound) {
    if (!validateABICompatibility(M, Result.DetectedVersion)) {
      Result.addGlobalError("ABI compatibility validation failed");
    }
  }
  
  // Validate each expected hook function
  for (const auto& ExpectedHook : ExpectedHooks) {
    const std::string& HookName = ExpectedHook.first();
    const HookSignature& Signature = ExpectedHook.second;
    
    HookValidationResult HookResult(HookName);
    HookResult.ExpectedSignature = &Signature;
    
    // Find the function in the module
    const Function* F = M.getFunction(HookName);
    if (F) {
      HookResult.Found = true;
      HookResult.ActualFunction = F;
      
      // Validate the signature
      HookResult.IsValid = validateSignature(F, Signature, HookResult);
    } else {
      HookResult.Found = false;
      if (Signature.IsRequired) {
        HookResult.addError("Required hook function not found");
      } else {
        HookResult.addWarning("Optional hook function not found");
        HookResult.IsValid = true; // Optional hooks are valid if missing
      }
    }
    
    Result.HookResults[HookName] = std::move(HookResult);
  }
  
  // Update statistics
  Result.updateStatistics();
  
  LLVM_DEBUG(dbgs() << "Validation complete: " << Result.ValidHooks << "/" << Result.TotalHooks 
                    << " hooks valid\n");
  
  return Result;
}

HookValidationResult RuntimeInterfaceValidator::validateHookFunction(const Function* F, 
                                                                     const HookSignature& Expected) {
  HookValidationResult Result(F->getName());
  Result.ExpectedSignature = &Expected;
  Result.ActualFunction = F;
  Result.Found = true;
  
  Result.IsValid = validateSignature(F, Expected, Result);
  
  return Result;
}

bool RuntimeInterfaceValidator::validateSignature(const Function* F, const HookSignature& Expected,
                                                  HookValidationResult& Result) {
  bool IsValid = true;
  
  // Validate return type
  if (!validateReturnType(F, Expected.ReturnType, Result)) {
    IsValid = false;
  }
  
  // Validate parameter types
  if (!validateParameterTypes(F, Expected.ParameterTypes, Result)) {
    IsValid = false;
  }
  
  // Validate calling convention
  if (!validateCallingConvention(F, Expected.CallingConv)) {
    Result.addError("Calling convention mismatch: expected " + 
                   std::to_string(Expected.CallingConv) + ", got " + 
                   std::to_string(F->getCallingConv()));
    IsValid = false;
  }
  
  // Validate variadic nature
  if (F->isVarArg() != Expected.IsVariadic) {
    Result.addError("Variadic mismatch: expected " + 
                   (Expected.IsVariadic ? "variadic" : "non-variadic") + 
                   ", got " + (F->isVarArg() ? "variadic" : "non-variadic"));
    IsValid = false;
  }
  
  // Validate function linkage and visibility
  if (!validateFunctionLinkage(F, Result)) {
    IsValid = false;
  }
  
  // Validate function attributes
  if (!validateFunctionAttributes(F, Expected, Result)) {
    IsValid = false;
  }
  
  // Check for deprecated hooks
  if (!checkDeprecatedHooks(F, Result)) {
    // Deprecated hooks generate warnings but don't invalidate
  }
  
  // Validate naming conventions
  if (!validateNamingConventions(F, Result)) {
    // Naming issues generate warnings but don't invalidate
  }
  
  return IsValid;
}

RuntimeVersion RuntimeInterfaceValidator::detectRuntimeVersion(Module& M) {
  // Try to extract version from module metadata
  RuntimeVersion Version = extractVersionFromModule(M);
  if (Version.Major > 0) {
    return Version;
  }
  
  // Try to detect from function names
  SmallVector<const Function*, 16> RuntimeFunctions = findRuntimeFunctions(M);
  if (!RuntimeFunctions.empty()) {
    // If we found runtime functions but no version info, assume minimum version
    return MinRuntimeVersion;
  }
  
  // No runtime library detected
  return RuntimeVersion();
}

bool RuntimeInterfaceValidator::checkVersionCompatibility(const RuntimeVersion& Detected, 
                                                          const RuntimeVersion& Required) {
  return Detected.isCompatibleWith(Required);
}

bool RuntimeInterfaceValidator::validateABICompatibility(Module& M, const RuntimeVersion& Version) {
  // Extract ABI version from module
  uint32_t ModuleABI = extractABIVersion(M);
  
  if (ModuleABI == 0) {
    LLVM_DEBUG(dbgs() << "No ABI version found in module\n");
    return !StrictValidation; // Allow if not in strict mode
  }
  
  if (Version.ABIVersion != 0 && ModuleABI != Version.ABIVersion) {
    LLVM_DEBUG(dbgs() << "ABI version mismatch: module=" << ModuleABI 
                      << ", runtime=" << Version.ABIVersion << "\n");
    return false;
  }
  
  return true;
}

void RuntimeInterfaceValidator::addExpectedHook(const HookSignature& Signature) {
  ExpectedHooks[Signature.Name] = Signature;
  LLVM_DEBUG(dbgs() << "Added expected hook: " << Signature.Name << "\n");
}

void RuntimeInterfaceValidator::removeExpectedHook(StringRef HookName) {
  ExpectedHooks.erase(HookName);
  LLVM_DEBUG(dbgs() << "Removed expected hook: " << HookName << "\n");
}

const HookSignature* RuntimeInterfaceValidator::getExpectedHook(StringRef HookName) const {
  auto It = ExpectedHooks.find(HookName);
  return (It != ExpectedHooks.end()) ? &It->second : nullptr;
}

bool RuntimeInterfaceValidator::isHookRequired(StringRef HookName) const {
  const HookSignature* Sig = getExpectedHook(HookName);
  return Sig ? Sig->IsRequired : false;
}

void RuntimeInterfaceValidator::getValidationStatistics(const InterfaceValidationResult& Result, 
                                                        raw_ostream& OS) const {
  OS << "Runtime Interface Validation Statistics:\n";
  OS << "  Runtime Found: " << (Result.RuntimeFound ? "Yes" : "No") << "\n";
  if (Result.RuntimeFound) {
    OS << "  Runtime Version: " << Result.DetectedVersion.VersionString << "\n";
  }
  OS << "  Total Hooks: " << Result.TotalHooks << "\n";
  OS << "  Valid Hooks: " << Result.ValidHooks << "\n";
  OS << "  Required Hooks: " << Result.RequiredHooks << "\n";
  OS << "  Valid Required Hooks: " << Result.ValidRequiredHooks << "\n";
  OS << "  Overall Status: " << (Result.passed() ? "PASS" : "FAIL") << "\n";
}

void RuntimeInterfaceValidator::generateValidationReport(const InterfaceValidationResult& Result, 
                                                         raw_ostream& OS) const {
  OS << "=== Runtime Interface Validation Report ===\n\n";
  
  // Global status
  getValidationStatistics(Result, OS);
  OS << "\n";
  
  // Global errors and warnings
  if (!Result.GlobalErrors.empty()) {
    OS << "Global Errors:\n";
    for (const auto& Error : Result.GlobalErrors) {
      OS << "  - " << Error << "\n";
    }
    OS << "\n";
  }
  
  if (!Result.GlobalWarnings.empty()) {
    OS << "Global Warnings:\n";
    for (const auto& Warning : Result.GlobalWarnings) {
      OS << "  - " << Warning << "\n";
    }
    OS << "\n";
  }
  
  // Hook-specific results
  OS << "Hook Validation Results:\n";
  for (const auto& HookResult : Result.HookResults) {
    const std::string& HookName = HookResult.first();
    const HookValidationResult& Hook = HookResult.second;
    
    OS << "  " << HookName << ": ";
    if (Hook.IsValid) {
      OS << "PASS";
    } else {
      OS << "FAIL";
    }
    
    if (!Hook.Found) {
      OS << " (NOT FOUND)";
    }
    
    if (Hook.ExpectedSignature && Hook.ExpectedSignature->IsRequired) {
      OS << " [REQUIRED]";
    } else {
      OS << " [OPTIONAL]";
    }
    
    OS << "\n";
    
    // Show errors and warnings
    for (const auto& Error : Hook.Errors) {
      OS << "    ERROR: " << Error << "\n";
    }
    for (const auto& Warning : Hook.Warnings) {
      OS << "    WARNING: " << Warning << "\n";
    }
  }
}

void RuntimeInterfaceValidator::createDefaultHookSignatures() {
  // Create standard hook signatures for MPI sanitizer
  Type* VoidTy = Type::getVoidTy(Context);
  Type* IntTy = Type::getInt32Ty(Context);
  Type* PtrTy = Type::getInt8PtrTy(Context);
  Type* SizeTy = Type::getInt64Ty(Context);
  
  // Pre-call hook: void mpi_sanitizer_pre_call(const char* func_name, void* args)
  {
    SmallVector<Type*, 2> ParamTypes = {PtrTy, PtrTy};
    HookSignature PreCallHook("mpi_sanitizer_pre_call", VoidTy, ParamTypes, true, "core");
    PreCallHook.MinVersion = RuntimeVersion(1, 0, 0);
    addExpectedHook(PreCallHook);
  }
  
  // Post-call hook: void mpi_sanitizer_post_call(const char* func_name, int result, void* args)
  {
    SmallVector<Type*, 3> ParamTypes = {PtrTy, IntTy, PtrTy};
    HookSignature PostCallHook("mpi_sanitizer_post_call", VoidTy, ParamTypes, true, "core");
    PostCallHook.MinVersion = RuntimeVersion(1, 0, 0);
    addExpectedHook(PostCallHook);
  }
  
  // Performance timing hook: void mpi_sanitizer_timing_start(const char* func_name)
  {
    SmallVector<Type*, 1> ParamTypes = {PtrTy};
    HookSignature TimingStartHook("mpi_sanitizer_timing_start", VoidTy, ParamTypes, false, "performance");
    TimingStartHook.MinVersion = RuntimeVersion(1, 1, 0);
    addExpectedHook(TimingStartHook);
  }
  
  // Performance timing hook: void mpi_sanitizer_timing_end(const char* func_name, uint64_t duration)
  {
    SmallVector<Type*, 2> ParamTypes = {PtrTy, SizeTy};
    HookSignature TimingEndHook("mpi_sanitizer_timing_end", VoidTy, ParamTypes, false, "performance");
    TimingEndHook.MinVersion = RuntimeVersion(1, 1, 0);
    addExpectedHook(TimingEndHook);
  }
  
  // Communication volume hook: void mpi_sanitizer_comm_volume(const char* func_name, size_t bytes)
  {
    SmallVector<Type*, 2> ParamTypes = {PtrTy, SizeTy};
    HookSignature CommVolumeHook("mpi_sanitizer_comm_volume", VoidTy, ParamTypes, false, "performance");
    CommVolumeHook.MinVersion = RuntimeVersion(1, 2, 0);
    addExpectedHook(CommVolumeHook);
  }
  
  // Error reporting hook: void mpi_sanitizer_report_error(const char* message, const char* location)
  {
    SmallVector<Type*, 2> ParamTypes = {PtrTy, PtrTy};
    HookSignature ErrorHook("mpi_sanitizer_report_error", VoidTy, ParamTypes, true, "error");
    ErrorHook.MinVersion = RuntimeVersion(1, 0, 0);
    addExpectedHook(ErrorHook);
  }
  
  // Initialization hook: int mpi_sanitizer_init(void)
  {
    SmallVector<Type*, 0> ParamTypes;
    HookSignature InitHook("mpi_sanitizer_init", IntTy, ParamTypes, true, "lifecycle");
    InitHook.MinVersion = RuntimeVersion(1, 0, 0);
    addExpectedHook(InitHook);
  }
  
  // Finalization hook: void mpi_sanitizer_finalize(void)
  {
    SmallVector<Type*, 0> ParamTypes;
    HookSignature FinalizeHook("mpi_sanitizer_finalize", VoidTy, ParamTypes, true, "lifecycle");
    FinalizeHook.MinVersion = RuntimeVersion(1, 0, 0);
    addExpectedHook(FinalizeHook);
  }
  
  LLVM_DEBUG(dbgs() << "Created " << ExpectedHooks.size() << " default hook signatures\n");
}

bool RuntimeInterfaceValidator::validateCallingConvention(const Function* F, 
                                                          CallingConv::ID Expected) const {
  return F->getCallingConv() == Expected;
}

bool RuntimeInterfaceValidator::validateParameterTypes(const Function* F, ArrayRef<Type*> Expected,
                                                       HookValidationResult& Result) const {
  FunctionType* FT = F->getFunctionType();
  
  if (FT->getNumParams() != Expected.size()) {
    Result.addError("Parameter count mismatch: expected " + std::to_string(Expected.size()) + 
                   ", got " + std::to_string(FT->getNumParams()));
    return false;
  }
  
  bool IsValid = true;
  for (unsigned i = 0; i < Expected.size(); ++i) {
    Type* ActualType = FT->getParamType(i);
    Type* ExpectedType = Expected[i];
    
    if (!areTypesCompatible(ActualType, ExpectedType)) {
      Result.addError("Parameter " + std::to_string(i) + " type mismatch: " + 
                     getTypeCompatibilityError(ActualType, ExpectedType));
      IsValid = false;
    }
  }
  
  return IsValid;
}

bool RuntimeInterfaceValidator::validateReturnType(const Function* F, Type* Expected,
                                                   HookValidationResult& Result) const {
  Type* ActualType = F->getReturnType();
  
  if (!areTypesCompatible(ActualType, Expected)) {
    Result.addError("Return type mismatch: " + getTypeCompatibilityError(ActualType, Expected));
    return false;
  }
  
  return true;
}

bool RuntimeInterfaceValidator::areTypesCompatible(Type* Actual, Type* Expected) const {
  // Exact match
  if (Actual == Expected) {
    return true;
  }
  
  // Pointer type compatibility
  if (Actual->isPointerTy() && Expected->isPointerTy()) {
    // Allow i8* as generic pointer type
    if (Expected->getPointerElementType()->isIntegerTy(8) ||
        Actual->getPointerElementType()->isIntegerTy(8)) {
      return true;
    }
    
    // Check element type compatibility
    return areTypesCompatible(Actual->getPointerElementType(), 
                             Expected->getPointerElementType());
  }
  
  // Integer type compatibility (allow size differences in non-strict mode)
  if (Actual->isIntegerTy() && Expected->isIntegerTy()) {
    if (!StrictValidation) {
      return true; // Allow any integer types in non-strict mode
    }
    return Actual->getIntegerBitWidth() == Expected->getIntegerBitWidth();
  }
  
  // Function type compatibility
  if (Actual->isFunctionTy() && Expected->isFunctionTy()) {
    return areABICompatible(cast<FunctionType>(Actual), cast<FunctionType>(Expected));
  }
  
  return false;
}

std::string RuntimeInterfaceValidator::getTypeCompatibilityError(Type* Actual, Type* Expected) const {
  std::string ActualStr, ExpectedStr;
  raw_string_ostream ActualOS(ActualStr), ExpectedOS(ExpectedStr);
  
  Actual->print(ActualOS);
  Expected->print(ExpectedOS);
  
  return "expected " + ExpectedOS.str() + ", got " + ActualOS.str();
}

SmallVector<const Function*, 16> RuntimeInterfaceValidator::findRuntimeFunctions(Module& M) const {
  SmallVector<const Function*, 16> RuntimeFunctions;
  
  for (const Function& F : M) {
    if (F.getName().startswith(RuntimeLibraryPattern)) {
      RuntimeFunctions.push_back(&F);
    }
  }
  
  return RuntimeFunctions;
}

RuntimeVersion RuntimeInterfaceValidator::extractVersionFromModule(Module& M) const {
  // Try to find version in module metadata
  if (NamedMDNode* VersionMD = M.getNamedMetadata("mpi.sanitizer.version")) {
    if (MDNode* Node = VersionMD->getOperand(0)) {
      if (MDString* VersionStr = dyn_cast<MDString>(Node->getOperand(0))) {
        return RuntimeVersion::parseFromString(VersionStr->getString());
      }
    }
  }
  
  // Try to find version in global variables
  if (GlobalVariable* VersionGV = M.getGlobalVariable("mpi_sanitizer_version")) {
    if (ConstantDataArray* CDA = dyn_cast<ConstantDataArray>(VersionGV->getInitializer())) {
      if (CDA->isString()) {
        return RuntimeVersion::parseFromString(CDA->getAsString());
      }
    }
  }
  
  return RuntimeVersion();
}

bool RuntimeInterfaceValidator::validateFunctionLinkage(const Function* F, 
                                                        HookValidationResult& Result) const {
  // Runtime hooks should have external linkage
  if (F->getLinkage() != GlobalValue::ExternalLinkage) {
    Result.addWarning("Hook function should have external linkage");
  }
  
  // Check visibility
  if (F->getVisibility() != GlobalValue::DefaultVisibility) {
    Result.addWarning("Hook function should have default visibility");
  }
  
  return true; // Linkage issues are warnings, not errors
}

bool RuntimeInterfaceValidator::validateFunctionAttributes(const Function* F, 
                                                           const HookSignature& Expected,
                                                           HookValidationResult& Result) const {
  // Check for problematic attributes
  if (F->hasFnAttribute(Attribute::AlwaysInline)) {
    Result.addWarning("Hook function should not be always inlined");
  }
  
  if (F->hasFnAttribute(Attribute::NoReturn)) {
    Result.addError("Hook function should not have noreturn attribute");
    return false;
  }
  
  return true;
}

bool RuntimeInterfaceValidator::checkDeprecatedHooks(const Function* F, 
                                                     HookValidationResult& Result) const {
  StringRef FuncName = F->getName();
  
  // Check for deprecated hook names
  if (FuncName.contains("_deprecated_") || FuncName.endswith("_old")) {
    Result.addWarning("Hook function appears to be deprecated");
    return false;
  }
  
  return true;
}

bool RuntimeInterfaceValidator::validateNamingConventions(const Function* F, 
                                                          HookValidationResult& Result) const {
  StringRef FuncName = F->getName();
  
  // Check naming pattern
  if (!FuncName.startswith("mpi_sanitizer_")) {
    Result.addWarning("Hook function should follow naming convention: mpi_sanitizer_*");
    return false;
  }
  
  return true;
}

//===----------------------------------------------------------------------===//
// Utility Functions Implementation
//===----------------------------------------------------------------------===//

StringMap<HookSignature> createStandardHookSignatures(LLVMContext& Context) {
  StringMap<HookSignature> Signatures;
  
  RuntimeInterfaceValidator Validator(Context, *static_cast<ErrorHandler*>(nullptr));
  Validator.createDefaultHookSignatures();
  
  return Validator.getExpectedHooks();
}

RuntimeVersion parseRuntimeVersion(StringRef VersionString) {
  return RuntimeVersion::parseFromString(VersionString);
}

std::string formatRuntimeVersion(const RuntimeVersion& Version) {
  return Version.VersionString;
}

bool areABICompatible(FunctionType* Actual, FunctionType* Expected) {
  // Check return type
  if (Actual->getReturnType() != Expected->getReturnType()) {
    return false;
  }
  
  // Check parameter count
  if (Actual->getNumParams() != Expected->getNumParams()) {
    return false;
  }
  
  // Check parameter types
  for (unsigned i = 0; i < Actual->getNumParams(); ++i) {
    if (Actual->getParamType(i) != Expected->getParamType(i)) {
      return false;
    }
  }
  
  // Check variadic nature
  if (Actual->isVarArg() != Expected->isVarArg()) {
    return false;
  }
  
  return true;
}

std::string getABICompatibilityError(FunctionType* Actual, FunctionType* Expected) {
  std::string ActualStr, ExpectedStr;
  raw_string_ostream ActualOS(ActualStr), ExpectedOS(ExpectedStr);
  
  Actual->print(ActualOS);
  Expected->print(ExpectedOS);
  
  return "ABI mismatch: expected " + ExpectedOS.str() + ", got " + ActualOS.str();
}

bool validateHookNaming(StringRef FunctionName, StringRef ExpectedPattern) {
  std::regex Pattern(ExpectedPattern.str());
  return std::regex_match(FunctionName.str(), Pattern);
}

uint32_t extractABIVersion(Module& M) {
  // Try to find ABI version in module metadata
  if (NamedMDNode* ABIMD = M.getNamedMetadata("mpi.sanitizer.abi")) {
    if (MDNode* Node = ABIMD->getOperand(0)) {
      if (ConstantInt* CI = mdconst::dyn_extract<ConstantInt>(Node->getOperand(0))) {
        return CI->getZExtValue();
      }
    }
  }
  
  // Try to find ABI version in global variables
  if (GlobalVariable* ABIGV = M.getGlobalVariable("mpi_sanitizer_abi_version")) {
    if (ConstantInt* CI = dyn_cast<ConstantInt>(ABIGV->getInitializer())) {
      return CI->getZExtValue();
    }
  }
  
  return 0; // No ABI version found
}
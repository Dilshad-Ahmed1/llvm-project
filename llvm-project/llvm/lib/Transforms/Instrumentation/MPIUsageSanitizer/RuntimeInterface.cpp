//===- RuntimeInterface.cpp - MPI Sanitizer Runtime Interface -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the RuntimeInterface class which defines the interface
// between the instrumentation pass and the runtime library.
//
//===----------------------------------------------------------------------===//

#include "RuntimeInterface.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"

using namespace llvm;

FunctionType* RuntimeInterface::getPreHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_pre_call(const char* name, void** params, int count, const char* loc)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* VoidPtrPtrTy = PointerType::get(PointerType::get(Type::getInt8Ty(Ctx), 0), 0);
  Type* IntTy = Type::getInt32Ty(Ctx);
  
  return FunctionType::get(VoidTy, {CharPtrTy, VoidPtrPtrTy, IntTy, CharPtrTy}, false);
}

FunctionType* RuntimeInterface::getPostHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_post_call(const char* name, void* ret, int error, const char* loc)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* VoidPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* IntTy = Type::getInt32Ty(Ctx);
  
  return FunctionType::get(VoidTy, {CharPtrTy, VoidPtrTy, IntTy, CharPtrTy}, false);
}

FunctionType* RuntimeInterface::getPerformanceBeginHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_performance_begin(const char* name, const char* type)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  
  return FunctionType::get(VoidTy, {CharPtrTy, CharPtrTy}, false);
}

FunctionType* RuntimeInterface::getPerformanceEndHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_performance_end(const char* name, const char* type)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  
  return FunctionType::get(VoidTy, {CharPtrTy, CharPtrTy}, false);
}

FunctionType* RuntimeInterface::getCommunicationVolumeHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_comm_volume(const char* name, size_t volume, const char* pattern)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* SizeTy = Type::getInt64Ty(Ctx); // size_t as 64-bit
  
  return FunctionType::get(VoidTy, {CharPtrTy, SizeTy, CharPtrTy}, false);
}

FunctionType* RuntimeInterface::getCommunicationPatternHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_comm_pattern(const char* name, int src, int dest, int tag, const char* pattern_type)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* IntTy = Type::getInt32Ty(Ctx);
  
  return FunctionType::get(VoidTy, {CharPtrTy, IntTy, IntTy, IntTy, CharPtrTy}, false);
}

FunctionType* RuntimeInterface::getCollectiveTimingHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_collective_timing(const char* name, int comm_size, double* timing_data)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* IntTy = Type::getInt32Ty(Ctx);
  Type* DoublePtrTy = PointerType::get(Type::getDoubleTy(Ctx), 0);
  
  return FunctionType::get(VoidTy, {CharPtrTy, IntTy, DoublePtrTy}, false);
}

FunctionType* RuntimeInterface::getSynchronizationHookType(LLVMContext& Ctx) {
  // void __mpi_sanitizer_sync_point(const char* name, int sync_type, const char* location)
  Type* VoidTy = Type::getVoidTy(Ctx);
  Type* CharPtrTy = PointerType::get(Type::getInt8Ty(Ctx), 0);
  Type* IntTy = Type::getInt32Ty(Ctx);
  
  return FunctionType::get(VoidTy, {CharPtrTy, IntTy, CharPtrTy}, false);
}

bool RuntimeInterface::validateHookSignature(Function* HookFunc, FunctionType* ExpectedType) {
  if (!HookFunc || !ExpectedType)
    return false;
    
  FunctionType* ActualType = HookFunc->getFunctionType();
  
  // Check return type
  if (ActualType->getReturnType() != ExpectedType->getReturnType())
    return false;
    
  // Check parameter count
  if (ActualType->getNumParams() != ExpectedType->getNumParams())
    return false;
    
  // Check parameter types
  for (unsigned i = 0; i < ActualType->getNumParams(); ++i) {
    if (ActualType->getParamType(i) != ExpectedType->getParamType(i))
      return false;
  }
  
  // Check variadic flag
  if (ActualType->isVarArg() != ExpectedType->isVarArg())
    return false;
    
  return true;
}
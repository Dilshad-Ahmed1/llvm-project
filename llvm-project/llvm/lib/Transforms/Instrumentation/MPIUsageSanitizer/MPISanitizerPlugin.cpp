//===- MPISanitizerPlugin.cpp - MPI Usage Sanitizer Plugin ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the plugin entry point for the MPI Usage Sanitizer.
//
//===----------------------------------------------------------------------===//

#include "MPISanitizerPass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Plugin Registration
//===----------------------------------------------------------------------===//

llvm::PassPluginLibraryInfo getMPISanitizerPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, 
    "MPIUsageSanitizer", 
    LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      // Register the pass with the pass builder
      PB.registerPipelineParsingCallback(
        [](StringRef Name, ModulePassManager &MPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "mpi-sanitizer") {
            MPM.addPass(MPISanitizerPass());
            return true;
          }
          return false;
        });
      
      // Register the pass for optimization pipeline insertion
      PB.registerOptimizerLastEPCallback(
        [](ModulePassManager &MPM, OptimizationLevel Level) {
          // Only add in debug builds or when explicitly requested
          if (Level == OptimizationLevel::O0) {
            MPM.addPass(MPISanitizerPass());
          }
        });
      
      // Register analysis passes if needed
      PB.registerAnalysisRegistrationCallback(
        [](ModuleAnalysisManager &MAM) {
          // Register any analysis passes here if needed
        });
    }
  };
}

// Plugin entry point for dynamic loading
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMPISanitizerPluginInfo();
}
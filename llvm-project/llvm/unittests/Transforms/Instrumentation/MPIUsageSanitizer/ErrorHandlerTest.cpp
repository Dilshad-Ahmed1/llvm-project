//===- ErrorHandlerTest.cpp - ErrorHandler unit tests -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../../../../lib/Transforms/Instrumentation/MPIUsageSanitizer/ErrorHandler.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class ErrorHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
    Context = std::make_unique<LLVMContext>();
    Handler = std::make_unique<ErrorHandler>(*Context);
    
    // Create a simple module and function for testing
    TestModule = std::make_unique<Module>("test", *Context);
    FunctionType *FT = FunctionType::get(Type::getVoidTy(*Context), false);
    TestFunction = Function::Create(FT, Function::ExternalLinkage, "test_func", TestModule.get());
    BasicBlock *BB = BasicBlock::Create(*Context, "entry", TestFunction);
    IRBuilder<> Builder(BB);
    TestInstruction = Builder.CreateRetVoid();
  }

  void TearDown() override {
    Handler.reset();
    TestModule.reset();
    Context.reset();
  }

  std::unique_ptr<LLVMContext> Context;
  std::unique_ptr<ErrorHandler> Handler;
  std::unique_ptr<Module> TestModule;
  Function *TestFunction = nullptr;
  Instruction *TestInstruction = nullptr;
};

// Test basic error reporting functionality
TEST_F(ErrorHandlerTest, BasicErrorReporting) {
  EXPECT_FALSE(Handler->hasErrors());
  EXPECT_FALSE(Handler->hasFatalErrors());
  EXPECT_FALSE(Handler->hasWarnings());
  
  Handler->reportError("Test error message");
  
  EXPECT_TRUE(Handler->hasErrors());
  EXPECT_FALSE(Handler->hasFatalErrors());
  EXPECT_FALSE(Handler->hasWarnings());
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Error), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 1u);
}

// Test fatal error reporting
TEST_F(ErrorHandlerTest, FatalErrorReporting) {
  Handler->reportFatal("Fatal error message");
  
  EXPECT_TRUE(Handler->hasErrors());
  EXPECT_TRUE(Handler->hasFatalErrors());
  EXPECT_FALSE(Handler->hasWarnings());
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Fatal), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 1u);
}

// Test warning reporting
TEST_F(ErrorHandlerTest, WarningReporting) {
  Handler->reportWarning("Warning message");
  
  EXPECT_FALSE(Handler->hasErrors());
  EXPECT_FALSE(Handler->hasFatalErrors());
  EXPECT_TRUE(Handler->hasWarnings());
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Warning), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 1u);
}

// Test info and debug reporting
TEST_F(ErrorHandlerTest, InfoAndDebugReporting) {
  Handler->reportInfo("Info message");
  Handler->reportDebug("Debug message");
  
  EXPECT_FALSE(Handler->hasErrors());
  EXPECT_FALSE(Handler->hasFatalErrors());
  EXPECT_FALSE(Handler->hasWarnings());
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Info), 1u);
#ifndef NDEBUG
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Debug), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 2u);
#else
  EXPECT_EQ(Stats.getTotalCount(), 1u);
#endif
}

// Test categorized error reporting
TEST_F(ErrorHandlerTest, CategorizedErrorReporting) {
  Handler->report(ErrorLevel::Error, DiagnosticCategory::CallDetection, 
                 "Call detection error");
  Handler->report(ErrorLevel::Warning, DiagnosticCategory::HookInsertion, 
                 "Hook insertion warning");
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByCategory.at(DiagnosticCategory::CallDetection), 1u);
  EXPECT_EQ(Stats.CountsByCategory.at(DiagnosticCategory::HookInsertion), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 2u);
}

// Test error reporting with instruction context
TEST_F(ErrorHandlerTest, ErrorReportingWithInstruction) {
  Handler->reportError("Error with instruction", TestInstruction, "test_func");
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByFunction.at("test_func"), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 1u);
}

// Test continuation logic
TEST_F(ErrorHandlerTest, ContinuationLogic) {
  // Fatal errors should never continue
  EXPECT_FALSE(Handler->shouldContinueAfterError(ErrorLevel::Fatal));
  
  // Non-fatal errors respect ContinueOnError setting
  Handler->setContinueOnError(true);
  EXPECT_TRUE(Handler->shouldContinueAfterError(ErrorLevel::Error));
  
  Handler->setContinueOnError(false);
  EXPECT_FALSE(Handler->shouldContinueAfterError(ErrorLevel::Error));
  
  // Warnings and info should always continue
  EXPECT_TRUE(Handler->shouldContinueAfterError(ErrorLevel::Warning));
  EXPECT_TRUE(Handler->shouldContinueAfterError(ErrorLevel::Info));
}

// Test minimum level filtering
TEST_F(ErrorHandlerTest, MinimumLevelFiltering) {
  Handler->setMinimumLevel(ErrorLevel::Warning);
  
  Handler->reportInfo("Info message");
  Handler->reportDebug("Debug message");
  Handler->reportWarning("Warning message");
  Handler->reportError("Error message");
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByLevel.count(ErrorLevel::Info), 0u);
  EXPECT_EQ(Stats.CountsByLevel.count(ErrorLevel::Debug), 0u);
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Warning), 1u);
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Error), 1u);
}

// Test statistics reset
TEST_F(ErrorHandlerTest, StatisticsReset) {
  Handler->reportError("Error 1");
  Handler->reportWarning("Warning 1");
  Handler->reportInfo("Info 1");
  
  EXPECT_EQ(Handler->getStatistics().getTotalCount(), 3u);
  
  Handler->reset();
  
  EXPECT_EQ(Handler->getStatistics().getTotalCount(), 0u);
  EXPECT_FALSE(Handler->hasErrors());
  EXPECT_FALSE(Handler->hasWarnings());
}

// Test multiple errors of same type
TEST_F(ErrorHandlerTest, MultipleErrorsSameType) {
  Handler->reportError("Error 1");
  Handler->reportError("Error 2");
  Handler->reportError("Error 3");
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByLevel.at(ErrorLevel::Error), 3u);
  EXPECT_EQ(Stats.getTotalCount(), 3u);
}

// Test function-specific statistics
TEST_F(ErrorHandlerTest, FunctionSpecificStatistics) {
  Handler->reportError("Error in func1", nullptr, "func1");
  Handler->reportError("Error in func1 again", nullptr, "func1");
  Handler->reportError("Error in func2", nullptr, "func2");
  Handler->reportWarning("Warning in func1", nullptr, "func1");
  
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.CountsByFunction.at("func1"), 3u);
  EXPECT_EQ(Stats.CountsByFunction.at("func2"), 1u);
  EXPECT_EQ(Stats.getTotalCount(), 4u);
}

// Test summary report generation
TEST_F(ErrorHandlerTest, SummaryReportGeneration) {
  Handler->reportFatal("Fatal error");
  Handler->reportError("Regular error");
  Handler->reportWarning("Warning message");
  Handler->reportInfo("Info message");
  
  std::string Summary = Handler->generateSummaryReport();
  
  // Check that summary contains expected information
  EXPECT_NE(Summary.find("Total diagnostics: 4"), std::string::npos);
  EXPECT_NE(Summary.find("Fatal: 1"), std::string::npos);
  EXPECT_NE(Summary.find("Error: 1"), std::string::npos);
  EXPECT_NE(Summary.find("Warning: 1"), std::string::npos);
  EXPECT_NE(Summary.find("Info: 1"), std::string::npos);
}

// Test empty summary report
TEST_F(ErrorHandlerTest, EmptySummaryReport) {
  std::string Summary = Handler->generateSummaryReport();
  
  EXPECT_NE(Summary.find("Total diagnostics: 0"), std::string::npos);
  EXPECT_NE(Summary.find("No diagnostics reported"), std::string::npos);
}

// Test error level string conversion
TEST_F(ErrorHandlerTest, ErrorLevelStringConversion) {
  // This is testing internal functionality, but we can verify through summary reports
  Handler->reportFatal("Fatal");
  Handler->reportError("Error");
  Handler->reportWarning("Warning");
  Handler->reportInfo("Info");
  
  std::string Summary = Handler->generateSummaryReport();
  
  EXPECT_NE(Summary.find("Fatal:"), std::string::npos);
  EXPECT_NE(Summary.find("Error:"), std::string::npos);
  EXPECT_NE(Summary.find("Warning:"), std::string::npos);
  EXPECT_NE(Summary.find("Info:"), std::string::npos);
}

// Test diagnostic category reporting
TEST_F(ErrorHandlerTest, DiagnosticCategoryReporting) {
  Handler->report(ErrorLevel::Error, DiagnosticCategory::CallDetection, "Call error");
  Handler->report(ErrorLevel::Error, DiagnosticCategory::MetadataExtraction, "Metadata error");
  Handler->report(ErrorLevel::Warning, DiagnosticCategory::HookInsertion, "Hook warning");
  Handler->report(ErrorLevel::Info, DiagnosticCategory::Optimization, "Optimization info");
  
  std::string Summary = Handler->generateSummaryReport();
  
  EXPECT_NE(Summary.find("Call Detection:"), std::string::npos);
  EXPECT_NE(Summary.find("Metadata Extraction:"), std::string::npos);
  EXPECT_NE(Summary.find("Hook Insertion:"), std::string::npos);
  EXPECT_NE(Summary.find("Optimization:"), std::string::npos);
}

// Test source location extraction from instruction
TEST_F(ErrorHandlerTest, SourceLocationExtraction) {
  // Create debug info for the instruction
  DIBuilder DIB(*TestModule);
  DIFile *File = DIB.createFile("test.c", "/tmp");
  DICompileUnit *CU = DIB.createCompileUnit(dwarf::DW_LANG_C, File, "test", false, "", 0);
  DISubprogram *SP = DIB.createFunction(CU, "test_func", "test_func", File, 1, 
                                       DIB.createSubroutineType(DIB.getOrCreateTypeArray({})), 
                                       1, DINode::FlagZero, DISubprogram::SPFlagDefinition);
  TestFunction->setSubprogram(SP);
  
  DebugLoc DL = DILocation::get(*Context, 10, 5, SP);
  TestInstruction->setDebugLoc(DL);
  
  // Report error with instruction that has debug info
  Handler->reportError("Error with debug info", TestInstruction);
  
  // Verify the error was recorded
  EXPECT_TRUE(Handler->hasErrors());
  const auto& Stats = Handler->getStatistics();
  EXPECT_EQ(Stats.getTotalCount(), 1u);
}

} // anonymous namespace
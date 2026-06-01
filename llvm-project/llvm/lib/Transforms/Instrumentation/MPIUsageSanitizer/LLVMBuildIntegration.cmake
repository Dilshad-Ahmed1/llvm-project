# LLVM Build System Integration for MPI Usage Sanitizer Pass
# This file provides comprehensive integration with LLVM's build and test systems

# Add MPI Usage Sanitizer to LLVM's transformation passes
set(LLVM_MPI_SANITIZER_SOURCES
  MPISanitizerPass.cpp
  MPIFunctionDatabase.cpp
  MPICallDetector.cpp
  MetadataExtractor.cpp
  HookInserter.cpp
  StaticAnalyzer.cpp
  OptimizationEngine.cpp
  ConfigurationManager.cpp
  ErrorHandler.cpp
  RuntimeInterfaceValidator.cpp
  PerformanceProfiler.cpp
  PassOptimizer.cpp
)

# Add MPI Usage Sanitizer headers
set(LLVM_MPI_SANITIZER_HEADERS
  MPISanitizerPass.h
  MPIFunctionDatabase.h
  MPICallDetector.h
  MetadataExtractor.h
  HookInserter.h
  StaticAnalyzer.h
  OptimizationEngine.h
  ConfigurationManager.h
  ErrorHandler.h
  RuntimeInterfaceValidator.h
  PerformanceProfiler.h
  PassOptimizer.h
)

# Add test files
set(LLVM_MPI_SANITIZER_TESTS
  MPIFunctionDatabaseTest.cpp
  MPICallDetectorTest.cpp
  MetadataExtractorTest.cpp
  HookInserterTest.cpp
  StaticAnalyzerTest.cpp
  OptimizationEngineTest.cpp
  ConfigurationManagerTest.cpp
  ErrorHandlerTest.cpp
  RuntimeInterfaceValidatorTest.cpp
  RuntimeLibraryIntegrationTest.cpp
  ParameterMarshalingTest.cpp
  PropertyBasedTestFramework.cpp
  IntegrationTests.cpp
  PerformanceTests.cpp
  OptimizationOverheadTest.cpp
)

# Integration with LLVM's CMake system
if(LLVM_ENABLE_MPI_SANITIZER)
  # Add the MPI sanitizer pass to the instrumentation library
  target_sources(LLVMInstrumentation PRIVATE ${LLVM_MPI_SANITIZER_SOURCES})
  
  # Add include directories
  target_include_directories(LLVMInstrumentation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MPIUsageSanitizer
  )
  
  # Link required LLVM libraries
  target_link_libraries(LLVMInstrumentation PRIVATE
    LLVMCore
    LLVMSupport
    LLVMAnalysis
    LLVMTransformUtils
    LLVMScalarOpts
    LLVMInstCombine
  )
  
  # Add MPI dependency if available
  find_package(MPI QUIET)
  if(MPI_FOUND)
    target_compile_definitions(LLVMInstrumentation PRIVATE LLVM_ENABLE_MPI_SUPPORT=1)
    target_include_directories(LLVMInstrumentation PRIVATE ${MPI_INCLUDE_PATH})
  endif()
  
  # Install headers for external use
  install(FILES ${LLVM_MPI_SANITIZER_HEADERS}
    DESTINATION include/llvm/Transforms/Instrumentation/MPIUsageSanitizer
    COMPONENT llvm-headers
  )
endif()

# Test integration
if(LLVM_INCLUDE_TESTS AND LLVM_ENABLE_MPI_SANITIZER)
  # Add unit tests
  add_llvm_unittest(MPIUsageSanitizerTests ${LLVM_MPI_SANITIZER_TESTS})
  
  # Link test dependencies
  target_link_libraries(MPIUsageSanitizerTests PRIVATE
    LLVMInstrumentation
    LLVMCore
    LLVMSupport
    LLVMTestingSupport
    gtest
    gtest_main
  )
  
  # Add test include directories
  target_include_directories(MPIUsageSanitizerTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MPIUsageSanitizer
  )
  
  # Add property-based testing support
  if(LLVM_ENABLE_PROPERTY_BASED_TESTING)
    target_compile_definitions(MPIUsageSanitizerTests PRIVATE LLVM_ENABLE_PBT=1)
  endif()
endif()

# Lit test integration
if(LLVM_INCLUDE_TESTS AND LLVM_ENABLE_MPI_SANITIZER)
  # Configure lit tests
  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/test/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/test/lit.site.cfg.py
    MAIN_CONFIG
    ${CMAKE_CURRENT_SOURCE_DIR}/test/lit.cfg.py
  )
  
  # Add lit test suite
  add_lit_testsuite(check-mpi-sanitizer "Running MPI Usage Sanitizer tests"
    ${CMAKE_CURRENT_BINARY_DIR}/test
    DEPENDS opt FileCheck count not
  )
  
  # Add to main test suite
  add_dependencies(check-llvm check-mpi-sanitizer)
endif()

# Documentation integration
if(LLVM_ENABLE_SPHINX AND LLVM_ENABLE_MPI_SANITIZER)
  # Add documentation files
  set(LLVM_MPI_SANITIZER_DOCS
    docs/MPIUsageSanitizer.rst
    docs/UserGuide.rst
    docs/DeveloperGuide.rst
    docs/PerformanceGuide.rst
  )
  
  # Install documentation
  install(FILES ${LLVM_MPI_SANITIZER_DOCS}
    DESTINATION docs/llvm/Transforms/Instrumentation/MPIUsageSanitizer
    COMPONENT llvm-docs
  )
endif()

# Configuration options
option(LLVM_ENABLE_MPI_SANITIZER "Enable MPI Usage Sanitizer Pass" ON)
option(LLVM_MPI_SANITIZER_ENABLE_PROFILING "Enable performance profiling in MPI sanitizer" OFF)
option(LLVM_MPI_SANITIZER_ENABLE_OPTIMIZATION "Enable optimization in MPI sanitizer" ON)

# Pass configuration to source code
if(LLVM_MPI_SANITIZER_ENABLE_PROFILING)
  target_compile_definitions(LLVMInstrumentation PRIVATE LLVM_MPI_SANITIZER_PROFILING=1)
endif()

if(LLVM_MPI_SANITIZER_ENABLE_OPTIMIZATION)
  target_compile_definitions(LLVMInstrumentation PRIVATE LLVM_MPI_SANITIZER_OPTIMIZATION=1)
endif()

# Version information
set(LLVM_MPI_SANITIZER_VERSION_MAJOR 1)
set(LLVM_MPI_SANITIZER_VERSION_MINOR 0)
set(LLVM_MPI_SANITIZER_VERSION_PATCH 0)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MPIUsageSanitizer/Version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/MPIUsageSanitizer/Version.h
)

# Export configuration for external projects
if(LLVM_ENABLE_MPI_SANITIZER)
  export(TARGETS LLVMInstrumentation
    NAMESPACE LLVM::
    FILE LLVMMPIUsageSanitizerTargets.cmake
  )
endif()
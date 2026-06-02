add_llvm_component_library(LLVMInstrumentation
  AddressSanitizer.cpp
  AllocToken.cpp
  BoundsChecking.cpp
  CGProfile.cpp
  ControlHeightReduction.cpp
  DataFlowSanitizer.cpp
  GCOVProfiling.cpp
  BlockCoverageInference.cpp
  MemProfInstrumentation.cpp
  MemProfUse.cpp
  MemorySanitizer.cpp
  MPIUsageSanitizer.cpp
  NumericalStabilitySanitizer.cpp
  IndirectCallPromotion.cpp
  InstrProfiling.cpp
  KCFI.cpp
  LowerAllowCheckPass.cpp
  PGOCtxProfFlattening.cpp
  PGOCtxProfLowering.cpp
  PGOForceFunctionAttrs.cpp
  PGOInstrumentation.cpp
  PGOMemOPSizeOpt.cpp
  SanitizerCoverage.cpp
  SanitizerBinaryMetadata.cpp
  ValueProfileCollector.cpp
  ThreadSanitizer.cpp
  TypeSanitizer.cpp
  HWAddressSanitizer.cpp
  RealtimeSanitizer.cpp

  PARTIAL_SOURCES_INTENDED

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Analysis
  Core
  Demangle
  MC
  Support
  TargetParser
  TransformUtils
  ProfileData
  )

# Add MPI Sanitizer subdirectory if enabled
if(LLVM_ENABLE_MPI_SANITIZER OR LLVM_ENABLE_PROJECTS MATCHES ".*mpi-sanitizer.*")
  add_subdirectory(MPIUsageSanitizer)
else()
  # Always add subdirectory for now to support CI
  add_subdirectory(MPIUsageSanitizer)
endif()

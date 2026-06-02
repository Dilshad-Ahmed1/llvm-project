set(LLVM_LINK_COMPONENTS
  Core
  Support
  TransformUtils
  )

add_llvm_unittest(MPIUsageSanitizerTests
  ConfigurationManagerTest.cpp
  ErrorHandlerTest.cpp
  MPIUsageSanitizerConfigTest.cpp
  PARTIAL_SOURCES_INTENDED
  )

target_link_libraries(MPIUsageSanitizerTests PRIVATE
  LLVMTransformUtils
  LLVMCore
  LLVMSupport
  )
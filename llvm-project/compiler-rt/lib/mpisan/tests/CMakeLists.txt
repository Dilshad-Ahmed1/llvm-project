# Unit tests for the MPISan runtime library.

include(CompilerRTCompile)

include_directories(..)

set(MPISAN_UNITTEST_CFLAGS
  ${COMPILER_RT_UNITTEST_CFLAGS}
  ${COMPILER_RT_GTEST_CFLAGS}
  -I${COMPILER_RT_SOURCE_DIR}/lib
  -I${COMPILER_RT_SOURCE_DIR}/include
  -I${COMPILER_RT_SOURCE_DIR}/lib/mpisan
  -I${CMAKE_CURRENT_SOURCE_DIR}/..
  -DSANITIZER_COMMON_NO_REDEFINE_BUILTINS
  -O2
)

set(MPISAN_TEST_SOURCES
  mpisan_test_flags.cpp
  mpisan_test_type_compat.cpp
  mpisan_test_pending_ops.cpp
  mpisan_test_buffer_aliasing.cpp
  mpisan_test_collectives.cpp
  mpisan_test_deadlock.cpp
  mpisan_test_nonblocking.cpp
)

set(MPISAN_UNITTEST_LINK_FLAGS
  ${COMPILER_RT_UNITTEST_LINK_FLAGS}
  ${COMPILER_RT_UNWINDER_LINK_LIBS}
  ${SANITIZER_TEST_CXX_LIBRARIES}
)

append_list_if(COMPILER_RT_HAS_LIBDL     -ldl      MPISAN_UNITTEST_LINK_FLAGS)
append_list_if(COMPILER_RT_HAS_LIBRT     -lrt      MPISAN_UNITTEST_LINK_FLAGS)
append_list_if(COMPILER_RT_HAS_LIBM      -lm       MPISAN_UNITTEST_LINK_FLAGS)
append_list_if(COMPILER_RT_HAS_LIBPTHREAD -pthread  MPISAN_UNITTEST_LINK_FLAGS)

# Create the umbrella target that add_compiler_rt_test / generate_compiler_rt_tests
# will attach individual test executables to.
add_custom_target(MpisanUnitTests)
set_target_properties(MpisanUnitTests PROPERTIES FOLDER "Compiler-RT Tests")

set(MPISAN_TEST_ARCH ${MPISAN_SUPPORTED_ARCH})
if(APPLE)
  darwin_filter_host_archs(MPISAN_SUPPORTED_ARCH MPISAN_TEST_ARCH)
endif()

foreach(arch ${MPISAN_TEST_ARCH})
  set(MpisanTestObjects)
  generate_compiler_rt_tests(MpisanTestObjects
    MpisanUnitTests "Mpisan-${arch}-Test" ${arch}
    SOURCES ${MPISAN_TEST_SOURCES} ${COMPILER_RT_GTEST_SOURCE}
    DEPS mpisan
    CFLAGS ${MPISAN_UNITTEST_CFLAGS}
    LINK_FLAGS ${MPISAN_UNITTEST_LINK_FLAGS}
  )
  set_target_properties(MpisanUnitTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

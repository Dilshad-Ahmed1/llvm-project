# Build for the MPIUsageSanitizer runtime support library.
# Track B – Runtime Core

include_directories(..)

set(MPISAN_CXX_SOURCES
  mpisan_flags.cpp
  mpisan_report.cpp
  mpisan_rtl.cpp
  mpisan_suppressions.cpp
)

set(MPISAN_PREINIT_SOURCES
  mpisan_preinit.cpp
)

set(MPISAN_HEADERS
  mpisan_flags.h
  mpisan_flags.inc
  mpisan_internal.h
  mpisan_report.h
  mpisan_rtl.h
  mpisan_suppressions.h
)

set(MPISAN_CFLAGS
  ${SANITIZER_COMMON_CFLAGS}
  -DSANITIZER_COMMON_NO_REDEFINE_BUILTINS
)
set(MPISAN_LINK_FLAGS ${SANITIZER_COMMON_LINK_FLAGS})
set(MPISAN_DYNAMIC_LIBS
  ${COMPILER_RT_UNWINDER_LINK_LIBS}
  ${SANITIZER_CXX_ABI_LIBRARIES}
  ${SANITIZER_COMMON_LINK_LIBS}
)

append_rtti_flag(OFF MPISAN_CFLAGS)

set(MPISAN_COMMON_RUNTIME_OBJECT_LIBS
  RTSanitizerCommon
  RTSanitizerCommonLibc
  RTSanitizerCommonCoverage
  RTSanitizerCommonSymbolizer
)

append_list_if(COMPILER_RT_HAS_LIBDL     dl      MPISAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBRT     rt      MPISAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBM      m       MPISAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBPTHREAD pthread MPISAN_DYNAMIC_LIBS)

add_compiler_rt_component(mpisan)

if(APPLE)
  add_compiler_rt_object_libraries(RTMpisan
    OS ${SANITIZER_COMMON_SUPPORTED_OS}
    ARCHS ${MPISAN_SUPPORTED_ARCH}
    SOURCES ${MPISAN_CXX_SOURCES}
    ADDITIONAL_HEADERS ${MPISAN_HEADERS}
    CFLAGS ${MPISAN_CFLAGS}
  )

  add_weak_symbols("sanitizer_common" WEAK_SYMBOL_LINK_FLAGS)
  set(MPISAN_LINK_FLAGS ${MPISAN_LINK_FLAGS} ${WEAK_SYMBOL_LINK_FLAGS})

  add_compiler_rt_runtime(clang_rt.mpisan
    SHARED
    OS ${SANITIZER_COMMON_SUPPORTED_OS}
    ARCHS ${MPISAN_SUPPORTED_ARCH}
    OBJECT_LIBS RTMpisan
                ${MPISAN_COMMON_RUNTIME_OBJECT_LIBS}
    LINK_FLAGS ${MPISAN_LINK_FLAGS}
    LINK_LIBS ${MPISAN_DYNAMIC_LIBS}
    PARENT_TARGET mpisan
  )
else()
  add_compiler_rt_object_libraries(RTMpisan
    ARCHS ${MPISAN_SUPPORTED_ARCH}
    SOURCES ${MPISAN_CXX_SOURCES}
    ADDITIONAL_HEADERS ${MPISAN_HEADERS}
    CFLAGS ${MPISAN_CFLAGS}
  )
  add_compiler_rt_object_libraries(RTMpisan_preinit
    ARCHS ${MPISAN_SUPPORTED_ARCH}
    SOURCES ${MPISAN_PREINIT_SOURCES}
    ADDITIONAL_HEADERS ${MPISAN_HEADERS}
    CFLAGS ${MPISAN_CFLAGS}
  )

  add_compiler_rt_runtime(clang_rt.mpisan
    STATIC
    ARCHS ${MPISAN_SUPPORTED_ARCH}
    OBJECT_LIBS RTMpisan_preinit
                RTMpisan
                ${MPISAN_COMMON_RUNTIME_OBJECT_LIBS}
    LINK_FLAGS ${MPISAN_LINK_FLAGS}
    CFLAGS ${MPISAN_CFLAGS}
    PARENT_TARGET mpisan
  )
endif()

if(COMPILER_RT_INCLUDE_TESTS)
  add_subdirectory(tests)
endif()

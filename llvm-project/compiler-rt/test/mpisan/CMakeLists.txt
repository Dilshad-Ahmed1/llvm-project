set(MPISAN_LIT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MPISAN_TESTSUITES)

set(MPISAN_TEST_DEPS ${SANITIZER_COMMON_LIT_TEST_DEPS})
set(MPISAN_TEST_ARCH ${MPISAN_SUPPORTED_ARCH})
if(APPLE)
  darwin_filter_host_archs(MPISAN_SUPPORTED_ARCH MPISAN_TEST_ARCH)
endif()

foreach(arch ${MPISAN_TEST_ARCH})
  set(MPISAN_TEST_TARGET_ARCH ${arch})
  string(TOLOWER "-${arch}-${OS_NAME}" MPISAN_TEST_CONFIG_SUFFIX)
  get_test_cc_for_arch(${arch} MPISAN_TEST_TARGET_CC MPISAN_TEST_TARGET_CFLAGS)
  string(TOUPPER ${arch} ARCH_UPPER_CASE)
  set(CONFIG_NAME ${ARCH_UPPER_CASE}${OS_NAME}Config)

  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}/lit.site.cfg.py)
  list(APPEND MPISAN_TESTSUITES ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME})
endforeach()

# Unit test integration via lit
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg.py)
list(APPEND MPISAN_TEST_DEPS MpisanUnitTests)
list(APPEND MPISAN_TESTSUITES ${CMAKE_CURRENT_BINARY_DIR}/Unit)

add_lit_testsuite(check-mpisan "Running the MPISan tests"
  ${MPISAN_TESTSUITES}
  DEPENDS ${MPISAN_TEST_DEPS})
set_target_properties(check-mpisan PROPERTIES FOLDER "Compiler-RT Misc")

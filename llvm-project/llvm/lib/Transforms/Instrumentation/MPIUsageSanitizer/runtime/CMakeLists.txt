# Runtime library for MPI Usage Sanitizer

# Try to find MPI, but don't fail if not found
find_package(MPI QUIET)

if(MPI_FOUND)
  message(STATUS "MPI found, building full runtime library")
  
  add_library(mpi_sanitizer_runtime SHARED
    mpi_sanitizer_runtime.c
  )

  target_include_directories(mpi_sanitizer_runtime PRIVATE
    ${MPI_C_INCLUDE_DIRS}
  )

  target_link_libraries(mpi_sanitizer_runtime
    ${MPI_C_LIBRARIES}
  )

  target_compile_options(mpi_sanitizer_runtime PRIVATE
    ${MPI_C_COMPILE_FLAGS}
  )

  # Install the runtime library
  install(TARGETS mpi_sanitizer_runtime
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
else()
  message(STATUS "MPI not found, building stub runtime library")
  
  # Create a stub runtime library that doesn't depend on MPI
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mpi_sanitizer_runtime_stub.c" "
// Stub runtime library for MPI Usage Sanitizer
// This is used when MPI is not available during build

#include <stdio.h>
#include <stdlib.h>

void __mpi_sanitizer_pre_call(const char* function_name, 
                              const char* source_location,
                              void* buffer_address,
                              size_t buffer_size,
                              int count,
                              int datatype,
                              int rank,
                              int tag,
                              int communicator) {
    // Stub implementation - does nothing
}

void __mpi_sanitizer_post_call(const char* function_name,
                               int return_code,
                               double execution_time) {
    // Stub implementation - does nothing
}

void __mpi_sanitizer_report_error(const char* error_message,
                                  const char* source_location) {
    fprintf(stderr, \"[MPI Sanitizer Stub] ERROR: %s at %s\\n\",
            error_message, source_location);
}
")

  add_library(mpi_sanitizer_runtime SHARED
    "${CMAKE_CURRENT_BINARY_DIR}/mpi_sanitizer_runtime_stub.c"
  )

  # Install the stub runtime library
  install(TARGETS mpi_sanitizer_runtime
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()
/**
 * @file mpi_sanitizer_runtime.c
 * @brief Runtime library stub for MPI Usage Sanitizer
 * 
 * This is a minimal runtime library implementation for testing purposes.
 * The full runtime library would be implemented separately.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

// Runtime hook function prototypes
void __mpi_sanitizer_pre_call(const char* function_name, 
                              const char* source_location,
                              void* buffer_address,
                              size_t buffer_size,
                              int count,
                              int datatype,
                              int rank,
                              int tag,
                              int communicator);

void __mpi_sanitizer_post_call(const char* function_name,
                               int return_code,
                               double execution_time);

void __mpi_sanitizer_report_error(const char* error_message,
                                  const char* source_location);

// Global configuration
static int g_sanitizer_enabled = 1;
static int g_performance_monitoring = 0;
static FILE* g_log_file = NULL;

// Initialize the runtime library
__attribute__((constructor))
void __mpi_sanitizer_init(void) {
    const char* options = getenv("MPI_SANITIZER_OPTIONS");
    if (options) {
        if (strstr(options, "enable_performance=1")) {
            g_performance_monitoring = 1;
        }
        if (strstr(options, "disable=1")) {
            g_sanitizer_enabled = 0;
        }
    }
    
    const char* log_file = getenv("MPI_SANITIZER_LOG_FILE");
    if (log_file) {
        g_log_file = fopen(log_file, "w");
    } else {
        g_log_file = stderr;
    }
}

// Cleanup the runtime library
__attribute__((destructor))
void __mpi_sanitizer_cleanup(void) {
    if (g_log_file && g_log_file != stderr) {
        fclose(g_log_file);
    }
}

// Pre-call hook implementation
void __mpi_sanitizer_pre_call(const char* function_name, 
                              const char* source_location,
                              void* buffer_address,
                              size_t buffer_size,
                              int count,
                              int datatype,
                              int rank,
                              int tag,
                              int communicator) {
    if (!g_sanitizer_enabled) return;
    
    fprintf(g_log_file, "[MPI Sanitizer] Pre-call: %s at %s\n", 
            function_name, source_location);
    
    // Basic parameter validation
    if (strcmp(function_name, "MPI_Send") == 0 || strcmp(function_name, "MPI_Recv") == 0) {
        if (buffer_address) {
            fprintf(g_log_file, "[MPI Sanitizer]   - Buffer validation: %p, size %zu bytes\n",
                    buffer_address, buffer_size);
        }
        if (count > 0) {
            fprintf(g_log_file, "[MPI Sanitizer]   - Count: %d\n", count);
        }
        if (rank >= 0) {
            fprintf(g_log_file, "[MPI Sanitizer]   - Rank: %d\n", rank);
        }
        if (tag >= 0) {
            fprintf(g_log_file, "[MPI Sanitizer]   - Tag: %d\n", tag);
        }
    }
    
    // Deadlock detection (simplified)
    if (strcmp(function_name, "MPI_Send") == 0) {
        static int send_count = 0;
        send_count++;
        if (send_count > 1) {
            fprintf(g_log_file, "[MPI Sanitizer] WARNING: Multiple MPI_Send calls detected - potential deadlock\n");
        }
    }
}

// Post-call hook implementation
void __mpi_sanitizer_post_call(const char* function_name,
                               int return_code,
                               double execution_time) {
    if (!g_sanitizer_enabled) return;
    
    fprintf(g_log_file, "[MPI Sanitizer] Post-call: %s returned %s\n",
            function_name, 
            (return_code == MPI_SUCCESS) ? "MPI_SUCCESS" : "ERROR");
    
    if (g_performance_monitoring && execution_time > 0.0) {
        fprintf(g_log_file, "[MPI Sanitizer]   - Execution time: %.3f ms\n",
                execution_time * 1000.0);
    }
    
    if (return_code != MPI_SUCCESS) {
        fprintf(g_log_file, "[MPI Sanitizer]   - Error code: %d\n", return_code);
    }
}

// Error reporting implementation
void __mpi_sanitizer_report_error(const char* error_message,
                                  const char* source_location) {
    if (!g_sanitizer_enabled) return;
    
    fprintf(g_log_file, "[MPI Sanitizer] ERROR: %s at %s\n",
            error_message, source_location);
    fflush(g_log_file);
}
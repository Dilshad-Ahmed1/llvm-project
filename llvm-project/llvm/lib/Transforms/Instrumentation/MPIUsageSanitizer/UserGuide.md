# MPI Usage Sanitizer User Guide

This comprehensive user guide provides detailed instructions for using the MPI Usage Sanitizer LLVM Pass to detect errors and monitor performance in MPI applications.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Basic Usage Patterns](#basic-usage-patterns)
3. [Configuration Guide](#configuration-guide)
4. [Error Detection](#error-detection)
5. [Performance Monitoring](#performance-monitoring)
6. [Advanced Features](#advanced-features)
7. [Integration Workflows](#integration-workflows)
8. [Best Practices](#best-practices)
9. [Troubleshooting](#troubleshooting)

## Getting Started

### Quick Start

The fastest way to get started with MPI Usage Sanitizer:

1. **Compile your MPI program** with sanitizer instrumentation:
   ```bash
   clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
         -mllvm -passes=mpi-sanitizer \
         your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime
   ```

2. **Run your program** normally:
   ```bash
   mpirun -np 4 ./your_program
   ```

3. **Check the output** for detected issues and performance information.

### Verification

Verify that the sanitizer is working by running a simple test:

```c
// test_mpi.c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("Hello from rank %d\n", rank);
    
    MPI_Finalize();
    return 0;
}
```

Compile and run:
```bash
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -passes=mpi-sanitizer \
      test_mpi.c -o test_mpi -lmpi -lmpi_sanitizer_runtime

mpirun -np 2 ./test_mpi
```

You should see output similar to:
```
Hello from rank 0
Hello from rank 1

=== MPI Sanitizer Report ===
MPI Calls Detected: 4 (MPI_Init, MPI_Comm_rank, MPI_Finalize per process)
Issues Found: 0
Execution Time: 0.001s
============================
```

## Basic Usage Patterns

### Pattern 1: Development and Testing

For active development where you want comprehensive error detection:

```bash
# Compile with standard instrumentation
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=standard \
      -mllvm -mpi-sanitizer-enable-optimizations=true \
      -g -O1 \
      your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime

# Run with detailed reporting
MPI_SANITIZER_OPTIONS="report_file=debug_report.txt:verbose=1" \
mpirun -np 4 ./your_program
```

### Pattern 2: Production Monitoring

For production environments where minimal overhead is critical:

```bash
# Compile with lightweight instrumentation
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=lightweight \
      -mllvm -mpi-sanitizer-enable-optimizations=true \
      -mllvm -mpi-sanitizer-max-overhead=0.05 \
      -O3 \
      your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime

# Run with minimal reporting
MPI_SANITIZER_OPTIONS="report_errors_only=1" \
mpirun -np 16 ./your_program
```

### Pattern 3: Performance Analysis

For detailed performance analysis and optimization:

```bash
# Compile with performance monitoring
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=standard \
      -mllvm -mpi-sanitizer-enable-performance=true \
      -mllvm -mpi-sanitizer-enable-timing=true \
      -O2 \
      your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime

# Run with performance reporting
MPI_SANITIZER_OPTIONS="report_file=perf_report.txt:enable_performance=1:timing_details=1" \
mpirun -np 8 ./your_program
```

### Pattern 4: Debugging Specific Issues

For debugging known or suspected issues:

```bash
# Compile with full instrumentation
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=debug \
      -mllvm -mpi-sanitizer-enable-deadlock-detection=true \
      -mllvm -mpi-sanitizer-enable-data-race-detection=true \
      -g -O0 \
      your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime

# Run with comprehensive debugging
MPI_SANITIZER_OPTIONS="report_file=debug.txt:verbose=1:stack_traces=1" \
mpirun -np 4 ./your_program
```

## Configuration Guide

### Configuration Methods

The MPI Usage Sanitizer supports three configuration methods, listed in order of precedence:

1. **Command Line Options** (highest priority)
2. **Configuration Files**
3. **Environment Variables** (lowest priority)

### Command Line Configuration

Pass options directly to the LLVM pass during compilation:

```bash
opt -passes=mpi-sanitizer \
    -mpi-sanitizer-level=standard \
    -mpi-sanitizer-enable-optimizations=true \
    -mpi-sanitizer-enable-performance=false \
    -mpi-sanitizer-enable-deadlock-detection=true \
    -mpi-sanitizer-enable-data-race-detection=true \
    -mpi-sanitizer-max-overhead=0.1 \
    -mpi-sanitizer-report-file=compile_report.txt \
    input.ll -o output.ll
```

### Configuration File Format

Create a detailed configuration file for complex setups:

```ini
# mpi_sanitizer.conf - Comprehensive Configuration Example

# ============================================================================
# Global Settings
# ============================================================================

# Instrumentation level: none, lightweight, standard, full, debug
instrumentation_mode = "standard"

# Enable compile-time optimizations to reduce overhead
enable_optimizations = true

# Enable static analysis for better optimization decisions
enable_static_analysis = true

# Maximum acceptable performance overhead (0.0 to 1.0)
max_performance_impact = 0.1

# ============================================================================
# Error Detection Settings
# ============================================================================

# Deadlock detection
enable_deadlock_detection = true
deadlock_detection_timeout = 30.0  # seconds
deadlock_detection_sensitivity = "medium"  # low, medium, high

# Data race detection
enable_data_race_detection = true
data_race_detection_mode = "precise"  # fast, precise, comprehensive

# Parameter validation
enable_parameter_validation = true
validate_buffer_bounds = true
validate_datatype_consistency = true
validate_communicator_validity = true

# Resource leak detection
enable_resource_leak_detection = true
track_communicators = true
track_requests = true
track_datatypes = true

# ============================================================================
# Performance Monitoring Settings
# ============================================================================

# Enable performance monitoring
enable_performance_monitoring = false

# Timing analysis
enable_timing_analysis = false
timing_precision = "microsecond"  # nanosecond, microsecond, millisecond

# Communication pattern analysis
enable_pattern_analysis = false
track_message_sizes = false
track_communication_volume = false

# Load balancing analysis
enable_load_balancing_analysis = false

# ============================================================================
# Output and Reporting Settings
# ============================================================================

# Report file (use "stderr" for standard error output)
report_file = "mpi_sanitizer_report.txt"

# Report format: text, json, xml
report_format = "text"

# Verbosity level: 0 (quiet), 1 (normal), 2 (verbose), 3 (debug)
verbosity_level = 1

# Include stack traces in error reports
include_stack_traces = true

# Generate summary statistics
generate_statistics = true

# Report only errors (suppress informational messages)
report_errors_only = false

# ============================================================================
# Function-Specific Policies
# ============================================================================

[function_policies]

# Point-to-point communication
MPI_Send = {
    enabled = true,
    enable_pre_hooks = true,
    enable_post_hooks = true,
    enable_parameter_validation = true,
    enable_timing = false
}

MPI_Recv = {
    enabled = true,
    enable_pre_hooks = true,
    enable_post_hooks = true,
    enable_parameter_validation = true,
    enable_timing = false
}

MPI_Isend = {
    enabled = true,
    enable_pre_hooks = true,
    enable_post_hooks = true,
    enable_request_tracking = true
}

MPI_Irecv = {
    enabled = true,
    enable_pre_hooks = true,
    enable_post_hooks = true,
    enable_request_tracking = true
}

# Collective communication
MPI_Bcast = {
    enabled = true,
    enable_pre_hooks = false,  # Reduced overhead for collectives
    enable_post_hooks = true,
    enable_collective_analysis = true
}

MPI_Reduce = {
    enabled = true,
    enable_pre_hooks = false,
    enable_post_hooks = true,
    enable_collective_analysis = true
}

MPI_Allreduce = {
    enabled = true,
    enable_pre_hooks = false,
    enable_post_hooks = true,
    enable_collective_analysis = true
}

# Synchronization
MPI_Barrier = {
    enabled = true,
    enable_deadlock_detection = true,
    enable_timing = true
}

MPI_Wait = {
    enabled = true,
    enable_request_validation = true,
    enable_deadlock_detection = true
}

# ============================================================================
# Advanced Settings
# ============================================================================

# Optimization settings
[optimization]
enable_selective_instrumentation = true
enable_hot_path_optimization = true
enable_constant_propagation = true
enable_dead_code_elimination = true

# Static analysis settings
[static_analysis]
enable_dataflow_analysis = true
enable_alias_analysis = true
enable_escape_analysis = true
analysis_precision = "medium"  # low, medium, high

# Runtime settings
[runtime]
enable_signal_handlers = true
enable_atexit_handlers = true
buffer_size = 1048576  # 1MB
max_tracked_requests = 10000
```

### Environment Variable Configuration

Configure runtime behavior using environment variables:

```bash
# Basic configuration
export MPI_SANITIZER_LEVEL=standard
export MPI_SANITIZER_REPORT_FILE=/tmp/mpi_report.txt
export MPI_SANITIZER_VERBOSE=1

# Advanced configuration using options string
export MPI_SANITIZER_OPTIONS="report_file=/tmp/report.txt:verbose=1:enable_performance=1:timing_details=1"

# Disable sanitizer at runtime (useful for performance comparison)
export MPI_SANITIZER_DISABLE=1

# Run your program
mpirun -np 4 ./your_program
```

### Configuration Validation

Validate your configuration before running:

```bash
# Check configuration file syntax
opt -passes=mpi-sanitizer \
    -mpi-sanitizer-config=mpi_sanitizer.conf \
    -mpi-sanitizer-validate-config=true \
    /dev/null -o /dev/null
```

## Error Detection

### Types of Errors Detected

#### 1. Deadlock Detection

The sanitizer can detect several types of deadlocks:

**Example: Circular Wait Deadlock**
```c
// Problematic code that can deadlock
if (rank == 0) {
    MPI_Send(data, size, MPI_INT, 1, 0, MPI_COMM_WORLD);
    MPI_Recv(data, size, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
} else if (rank == 1) {
    MPI_Send(data, size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(data, size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
}
```

**Sanitizer Output:**
```
=== MPI Sanitizer Error Report ===
Error Type: Potential Deadlock
Location: example.c:15
Description: Circular dependency detected in MPI_Send/MPI_Recv pattern
Processes Involved: 0, 1
Suggested Fix: Use MPI_Sendrecv or reorder operations
Stack Trace:
  #0 MPI_Send at example.c:15
  #1 main at example.c:12
===================================
```

#### 2. Data Race Detection

Detects concurrent access to MPI resources:

**Example: Concurrent Request Access**
```c
MPI_Request request;
MPI_Isend(data, size, MPI_INT, dest, tag, MPI_COMM_WORLD, &request);

// Problematic: Multiple threads accessing the same request
#pragma omp parallel
{
    int flag;
    MPI_Test(&request, &flag, &status);  // Race condition!
}
```

#### 3. Parameter Validation

Validates MPI function parameters:

**Example: Invalid Parameters**
```c
int *null_buffer = NULL;
MPI_Send(null_buffer, 100, MPI_INT, 1, 0, MPI_COMM_WORLD);  // NULL buffer
MPI_Send(data, -5, MPI_INT, 1, 0, MPI_COMM_WORLD);          // Negative count
MPI_Send(data, 100, MPI_INT, -1, 0, MPI_COMM_WORLD);        // Invalid rank
```

**Sanitizer Output:**
```
=== MPI Sanitizer Error Report ===
Error Type: Invalid Parameter
Function: MPI_Send
Location: example.c:23
Parameter: buffer (argument 1)
Issue: NULL pointer passed for non-zero count
Suggested Fix: Check buffer allocation before MPI call
===================================
```

#### 4. Resource Leak Detection

Tracks unreleased MPI resources:

**Example: Unreleased Communicator**
```c
MPI_Comm new_comm;
MPI_Comm_dup(MPI_COMM_WORLD, &new_comm);
// Missing MPI_Comm_free(&new_comm);
```

### Configuring Error Detection

#### Selective Error Detection

Enable only specific error types:

```ini
# Configuration for specific error types
enable_deadlock_detection = true
enable_data_race_detection = false
enable_parameter_validation = true
enable_resource_leak_detection = true

# Fine-tune detection sensitivity
deadlock_detection_sensitivity = "high"
parameter_validation_level = "strict"
```

#### Error Reporting Options

Customize error reporting:

```bash
# Environment variables for error reporting
export MPI_SANITIZER_OPTIONS="report_errors_only=1:stack_traces=1:abort_on_error=0"
```

## Performance Monitoring

### Enabling Performance Monitoring

```bash
# Compile with performance monitoring
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-enable-performance=true \
      -mllvm -mpi-sanitizer-enable-timing=true \
      your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime
```

### Performance Metrics Collected

#### 1. Timing Analysis

Measures execution time of MPI operations:

```c
// Example program for timing analysis
for (int i = 0; i < 1000; i++) {
    MPI_Send(data, size, MPI_INT, dest, tag, MPI_COMM_WORLD);
    MPI_Recv(data, size, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
}
```

**Performance Report:**
```
=== MPI Performance Report ===
Function: MPI_Send
  Total Calls: 1000
  Total Time: 0.245s
  Average Time: 245μs
  Min Time: 180μs
  Max Time: 1.2ms
  Standard Deviation: 45μs

Function: MPI_Recv
  Total Calls: 1000
  Total Time: 0.892s
  Average Time: 892μs
  Min Time: 200μs
  Max Time: 15.3ms
  Standard Deviation: 234μs
===============================
```

#### 2. Communication Volume Tracking

Monitors data transfer volumes:

```
=== Communication Volume Report ===
Total Data Sent: 1.2 GB
Total Data Received: 1.2 GB
Average Message Size: 1.2 MB
Largest Message: 10 MB
Communication Efficiency: 95.3%
===============================
```

#### 3. Load Balancing Analysis

Analyzes workload distribution:

```
=== Load Balancing Report ===
Process 0: 1.234s (25.1%)
Process 1: 1.189s (24.2%)
Process 2: 1.267s (25.8%)
Process 3: 1.223s (24.9%)

Load Imbalance: 6.5%
Recommendation: Well balanced
===============================
```

### Performance Configuration

```ini
# Detailed performance monitoring configuration
[performance]
enable_timing_analysis = true
timing_precision = "microsecond"
enable_volume_tracking = true
enable_pattern_analysis = true
enable_load_balancing_analysis = true

# Sampling configuration (to reduce overhead)
enable_sampling = true
sampling_rate = 0.1  # Sample 10% of calls
adaptive_sampling = true

# Reporting configuration
performance_report_interval = 10.0  # seconds
generate_performance_summary = true
include_histograms = true
```

## Advanced Features

### Static Analysis Integration

The sanitizer uses static analysis to optimize instrumentation:

```bash
# Enable advanced static analysis
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-enable-static-analysis=true \
      -mllvm -mpi-sanitizer-analysis-precision=high \
      your_program.c -o your_program -lmpi -lmpi_sanitizer_runtime
```

### Selective Instrumentation

Instrument only specific functions or code regions:

```ini
# Selective instrumentation configuration
[selective_instrumentation]
enable = true
mode = "whitelist"  # whitelist, blacklist, automatic

# Whitelist specific functions
whitelist_functions = [
    "MPI_Send",
    "MPI_Recv",
    "MPI_Wait"
]

# Blacklist functions (when mode = "blacklist")
blacklist_functions = [
    "MPI_Wtime",
    "MPI_Wtick"
]

# Automatic selection based on static analysis
automatic_threshold = 0.8  # Confidence threshold
```

### Custom Hook Functions

Define custom instrumentation hooks:

```c
// Custom pre-hook function
void my_mpi_pre_hook(const char* function_name, 
                     void* parameters, 
                     size_t param_size) {
    printf("Calling %s\n", function_name);
    // Custom logic here
}

// Custom post-hook function  
void my_mpi_post_hook(const char* function_name,
                      int return_code,
                      double execution_time) {
    if (return_code != MPI_SUCCESS) {
        printf("Error in %s: %d\n", function_name, return_code);
    }
    // Custom logic here
}
```

Register custom hooks:
```bash
export MPI_SANITIZER_OPTIONS="custom_pre_hook=my_mpi_pre_hook:custom_post_hook=my_mpi_post_hook"
```

## Integration Workflows

### CMake Integration

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyMPIProject)

find_package(MPI REQUIRED)

# Option to enable MPI Sanitizer
option(ENABLE_MPI_SANITIZER "Enable MPI Usage Sanitizer" OFF)

if(ENABLE_MPI_SANITIZER)
    # Find MPI Sanitizer components
    find_library(MPI_SANITIZER_PLUGIN 
                 NAMES LLVMMPIUsageSanitizerComponents
                 PATHS ${LLVM_LIBRARY_DIRS})
    
    find_library(MPI_SANITIZER_RUNTIME
                 NAMES mpi_sanitizer_runtime
                 PATHS ${MPI_SANITIZER_LIBRARY_DIRS})
    
    if(MPI_SANITIZER_PLUGIN AND MPI_SANITIZER_RUNTIME)
        # Add compiler flags
        set(MPI_SANITIZER_FLAGS 
            "-fpass-plugin=${MPI_SANITIZER_PLUGIN}"
            "-mllvm" "-passes=mpi-sanitizer"
            "-mllvm" "-mpi-sanitizer-level=standard")
        
        # Apply to all targets
        add_compile_options(${MPI_SANITIZER_FLAGS})
        
        # Link runtime library
        link_libraries(${MPI_SANITIZER_RUNTIME})
        
        message(STATUS "MPI Sanitizer enabled")
    else()
        message(WARNING "MPI Sanitizer requested but not found")
    endif()
endif()

# Your MPI targets
add_executable(my_mpi_program main.c)
target_link_libraries(my_mpi_program MPI::MPI_C)
```

Build with sanitizer:
```bash
mkdir build && cd build
cmake -DENABLE_MPI_SANITIZER=ON ..
make
```

### Makefile Integration

```makefile
# Makefile
CC = clang
MPICC = mpicc

# MPI Sanitizer configuration
ifdef ENABLE_MPI_SANITIZER
    MPI_SANITIZER_PLUGIN = LLVMMPIUsageSanitizerComponents.so
    MPI_SANITIZER_FLAGS = -fpass-plugin=$(MPI_SANITIZER_PLUGIN) \
                         -mllvm -passes=mpi-sanitizer \
                         -mllvm -mpi-sanitizer-level=standard
    MPI_SANITIZER_LIBS = -lmpi_sanitizer_runtime
    
    CFLAGS += $(MPI_SANITIZER_FLAGS)
    LDFLAGS += $(MPI_SANITIZER_LIBS)
endif

# Standard MPI flags
CFLAGS += $(shell $(MPICC) --showme:compile)
LDFLAGS += $(shell $(MPICC) --showme:link)

# Targets
SOURCES = main.c utils.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = my_mpi_program

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

# Convenience targets
sanitizer:
	$(MAKE) ENABLE_MPI_SANITIZER=1

test: $(TARGET)
	mpirun -np 4 ./$(TARGET)

.PHONY: all clean sanitizer test
```

### Continuous Integration

```yaml
# .github/workflows/mpi-sanitizer.yml
name: MPI Sanitizer CI

on: [push, pull_request]

jobs:
  test-with-sanitizer:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y openmpi-bin openmpi-common libopenmpi-dev
        sudo apt-get install -y llvm-14 clang-14
    
    - name: Build LLVM with MPI Sanitizer
      run: |
        git clone https://github.com/llvm/llvm-project.git
        cd llvm-project
        mkdir build && cd build
        cmake -G Ninja ../llvm \
          -DCMAKE_BUILD_TYPE=Release \
          -DLLVM_ENABLE_PROJECTS="clang" \
          -DLLVM_ENABLE_MPI_SANITIZER=ON
        ninja LLVMMPIUsageSanitizerComponents
    
    - name: Build test program
      run: |
        export LLVM_DIR=llvm-project/build
        clang -fpass-plugin=$LLVM_DIR/lib/LLVMMPIUsageSanitizerComponents.so \
              -mllvm -passes=mpi-sanitizer \
              test_program.c -o test_program -lmpi -lmpi_sanitizer_runtime
    
    - name: Run tests
      run: |
        mpirun -np 4 ./test_program
        # Check for sanitizer output
        if [ -f mpi_sanitizer_report.txt ]; then
          cat mpi_sanitizer_report.txt
        fi
```

## Best Practices

### Development Workflow

1. **Start with Standard Mode**: Begin development with standard instrumentation
2. **Use Configuration Files**: Create project-specific configuration files
3. **Enable Optimizations**: Always enable optimizations for better performance
4. **Regular Testing**: Run sanitizer regularly during development
5. **Performance Baseline**: Establish performance baselines early

### Production Deployment

1. **Lightweight Mode**: Use lightweight instrumentation in production
2. **Selective Monitoring**: Monitor only critical code paths
3. **Performance Budgets**: Set and monitor performance overhead budgets
4. **Gradual Rollout**: Deploy sanitizer gradually across production systems
5. **Monitoring Integration**: Integrate with existing monitoring systems

### Debugging Workflow

1. **Reproduce Issues**: Use debug mode to reproduce and analyze issues
2. **Isolate Problems**: Use selective instrumentation to isolate problems
3. **Stack Traces**: Enable stack traces for detailed error analysis
4. **Verbose Output**: Use verbose output for comprehensive debugging
5. **Iterative Analysis**: Iteratively refine configuration based on findings

### Performance Optimization

1. **Profile First**: Profile without sanitizer to establish baseline
2. **Measure Overhead**: Quantify sanitizer overhead impact
3. **Optimize Configuration**: Tune configuration for optimal performance
4. **Static Analysis**: Leverage static analysis for better optimization
5. **Continuous Monitoring**: Monitor performance impact continuously

## Troubleshooting

### Common Issues and Solutions

#### Issue: High Performance Overhead

**Symptoms:**
- Program runs significantly slower with sanitizer
- Unacceptable performance degradation

**Solutions:**
1. **Use Lightweight Mode:**
   ```bash
   -mllvm -mpi-sanitizer-level=lightweight
   ```

2. **Enable Optimizations:**
   ```bash
   -mllvm -mpi-sanitizer-enable-optimizations=true
   ```

3. **Reduce Instrumentation Scope:**
   ```ini
   [selective_instrumentation]
   enable = true
   mode = "whitelist"
   whitelist_functions = ["MPI_Send", "MPI_Recv"]
   ```

4. **Disable Performance Monitoring:**
   ```bash
   -mllvm -mpi-sanitizer-enable-performance=false
   ```

#### Issue: False Positive Deadlock Detection

**Symptoms:**
- Sanitizer reports deadlocks that don't actually occur
- Program works correctly but sanitizer complains

**Solutions:**
1. **Adjust Detection Sensitivity:**
   ```ini
   deadlock_detection_sensitivity = "low"
   deadlock_detection_timeout = 60.0
   ```

2. **Use Static Analysis:**
   ```bash
   -mllvm -mpi-sanitizer-enable-static-analysis=true
   ```

3. **Exclude Specific Functions:**
   ```ini
   [function_policies]
   MPI_Barrier = { enable_deadlock_detection = false }
   ```

#### Issue: Missing Error Detection

**Symptoms:**
- Known issues not detected by sanitizer
- Incomplete error reporting

**Solutions:**
1. **Increase Instrumentation Level:**
   ```bash
   -mllvm -mpi-sanitizer-level=full
   ```

2. **Enable All Detection Types:**
   ```ini
   enable_deadlock_detection = true
   enable_data_race_detection = true
   enable_parameter_validation = true
   enable_resource_leak_detection = true
   ```

3. **Check Function Coverage:**
   ```bash
   -mllvm -mpi-sanitizer-verbose=true
   ```

### Debugging Tips

#### 1. Enable Comprehensive Logging

```bash
export MPI_SANITIZER_OPTIONS="verbose=1:debug_output=1:log_file=debug.log"
```

#### 2. Use Debug Build

```bash
clang -g -O0 -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=debug \
      your_program.c -o your_program_debug -lmpi -lmpi_sanitizer_runtime
```

#### 3. Analyze Generated IR

```bash
# Generate instrumented LLVM IR
clang -S -emit-llvm your_program.c -o your_program.ll
opt -passes=mpi-sanitizer -S your_program.ll -o your_program_instrumented.ll

# Examine the instrumented IR
less your_program_instrumented.ll
```

#### 4. Runtime Debugging

```bash
# Enable runtime debugging
export MPI_SANITIZER_DEBUG=1
export MPI_SANITIZER_OPTIONS="stack_traces=1:abort_on_error=1"

# Run with debugger
mpirun -np 2 gdb --args ./your_program
```

This comprehensive user guide provides detailed instructions for effectively using the MPI Usage Sanitizer in various scenarios, from development to production deployment.
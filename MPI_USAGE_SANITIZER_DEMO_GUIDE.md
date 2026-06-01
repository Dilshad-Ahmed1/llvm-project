# MPI Usage Sanitizer LLVM Pass - Demo Guide

## Overview

This guide shows how to demonstrate the MPI Usage Sanitizer LLVM Pass, a comprehensive compiler transformation pass that instruments MPI programs for runtime error detection and performance monitoring.

## Demo Prerequisites

### System Requirements
- LLVM 14.0 or later with the MPI Usage Sanitizer pass installed
- MPI implementation (OpenMPI, MPICH, Intel MPI, etc.)
- C/C++ compiler with MPI support
- CMake 3.16 or later

### Verify Installation
```bash
# Check if MPI is available
mpicc --version
mpirun --version

# Check if LLVM with MPI Sanitizer is available
opt -load-pass-plugin=LLVMMPIUsageSanitizerComponents.so -passes=help | grep mpi-sanitizer
```

## Demo Structure

The demo consists of several example programs that showcase different aspects of the MPI Usage Sanitizer:

1. **Basic Examples**: Simple MPI programs showing fundamental instrumentation
2. **Error Detection**: Programs that trigger various error conditions
3. **Performance Monitoring**: Examples showing performance analysis capabilities
4. **Multi-Language Support**: C, C++, and Fortran examples

## Quick Demo Setup

### 1. Navigate to Examples Directory
```bash
cd llvm-project/llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples
```

### 2. Build All Examples
```bash
cd build_scripts
./build_all.sh
```

This creates:
- `build/uninstrumented/`: Programs compiled without sanitizer
- `build/instrumented/`: Programs compiled with sanitizer
- `build/run_uninstrumented.sh`: Script to run normal programs
- `build/run_instrumented.sh`: Script to run instrumented programs

## Demo Scenarios

### Demo 1: Basic MPI Program Instrumentation

**Objective**: Show how the sanitizer instruments a simple MPI program.

**Program**: `basic/hello_world.c`
```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from process %d of %d\n", rank, size);
    
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    
    return 0;
}
```

**Demo Steps**:

1. **Run without instrumentation**:
```bash
cd build
./run_uninstrumented.sh basic/hello_world -np 4
```

**Expected Output**:
```
Hello from process 0 of 4
Hello from process 1 of 4
Hello from process 2 of 4
Hello from process 3 of 4
```

2. **Run with instrumentation**:
```bash
./run_instrumented.sh basic/hello_world -np 4
```

**Expected Output**:
```
[MPI Sanitizer] Pre-call: MPI_Init at hello_world.c:8
[MPI Sanitizer]   - Validating argc/argv parameters
[MPI Sanitizer]   - Checking MPI initialization state
[MPI Sanitizer] Post-call: MPI_Init returned MPI_SUCCESS
[MPI Sanitizer]   - MPI environment successfully initialized

Hello from process 0 of 4
Hello from process 1 of 4
Hello from process 2 of 4
Hello from process 3 of 4

[MPI Sanitizer] Pre-call: MPI_Barrier at hello_world.c:13
[MPI Sanitizer]   - Validating communicator: MPI_COMM_WORLD
[MPI Sanitizer]   - Checking for potential deadlock conditions
[MPI Sanitizer] Post-call: MPI_Barrier returned MPI_SUCCESS
[MPI Sanitizer]   - Synchronization completed successfully

[MPI Sanitizer] Pre-call: MPI_Finalize at hello_world.c:14
[MPI Sanitizer]   - Checking for outstanding requests
[MPI Sanitizer]   - Validating MPI state for finalization
[MPI Sanitizer] Post-call: MPI_Finalize returned MPI_SUCCESS
[MPI Sanitizer]   - MPI environment successfully finalized
```

**Key Points to Highlight**:
- Every MPI call is instrumented with pre-call and post-call hooks
- Parameter validation occurs before each MPI call
- Return codes are checked and reported
- Source location information is provided for debugging

### Demo 2: Point-to-Point Communication Validation

**Objective**: Demonstrate buffer validation and message tracking.

**Program**: `basic/send_recv.c`

**Demo Steps**:

1. **Run the send/receive example**:
```bash
./run_instrumented.sh basic/send_recv -np 2
```

**Expected Output**:
```
[MPI Sanitizer] Pre-call: MPI_Send at send_recv.c:52
[MPI Sanitizer]   - Buffer validation: 0x7fff5fbff8a0, size 100 bytes
[MPI Sanitizer]   - Count: 22, Datatype: MPI_CHAR (1 byte)
[MPI Sanitizer]   - Total message size: 22 bytes
[MPI Sanitizer]   - Destination rank: 1 (valid in communicator)
[MPI Sanitizer]   - Tag: 42 (within valid range)
[MPI Sanitizer]   - Communicator: MPI_COMM_WORLD (valid)
[MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
[MPI Sanitizer]   - Message sent successfully

Process 0 sending: 'Hello from process 0!'
Process 1 received: 'Hello from process 0!'

[MPI Sanitizer] Pre-call: MPI_Recv at send_recv.c:62
[MPI Sanitizer]   - Buffer validation: 0x7fff5fbff7a0, capacity 100 bytes
[MPI Sanitizer]   - Count: 100, Datatype: MPI_CHAR (1 byte)
[MPI Sanitizer]   - Maximum message size: 100 bytes
[MPI Sanitizer]   - Source rank: 1 (valid in communicator)
[MPI Sanitizer] Post-call: MPI_Recv returned MPI_SUCCESS
[MPI Sanitizer]   - Message received: 23 bytes from rank 1, tag 42
[MPI Sanitizer]   - Buffer usage: 23/100 bytes (23%)

Process 0 received: 'Reply from process 1!'
```

**Key Points to Highlight**:
- Buffer addresses and sizes are validated
- Message sizes are calculated and checked
- Rank validation ensures communication partners exist
- Buffer usage statistics help identify efficiency issues

### Demo 3: Error Detection Capabilities

**Objective**: Show how the sanitizer detects common MPI programming errors.

**Program**: `error_cases/deadlock_example.c` (simulated)

Create a simple deadlock example:
```bash
cat > /tmp/deadlock_demo.c << 'EOF'
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int rank, size, data = 42;
    MPI_Status status;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (size != 2) {
        printf("This example requires exactly 2 processes\n");
        MPI_Finalize();
        return 1;
    }
    
    // Potential deadlock: both processes send first
    if (rank == 0) {
        MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
    } else {
        MPI_Send(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Recv(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
    
    MPI_Finalize();
    return 0;
}
EOF
```

**Compile and run**:
```bash
# Compile with sanitizer
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -passes=mpi-sanitizer \
      -mllvm -mpi-sanitizer-level=full \
      /tmp/deadlock_demo.c -o /tmp/deadlock_demo -lmpi -lmpi_sanitizer_runtime

# Run with timeout to prevent hanging
timeout 10s mpirun -np 2 /tmp/deadlock_demo
```

**Expected Output**:
```
[MPI Sanitizer] Pre-call: MPI_Send at deadlock_demo.c:18
[MPI Sanitizer]   - Deadlock analysis: POTENTIAL DEADLOCK DETECTED
[MPI Sanitizer]   - Pattern: Symmetric send operations detected
[MPI Sanitizer]   - Recommendation: Use MPI_Sendrecv or reorder operations
[MPI Sanitizer] WARNING: Potential deadlock in MPI_Send at deadlock_demo.c:18
[MPI Sanitizer]   - Both processes attempting to send simultaneously
[MPI Sanitizer]   - Consider using non-blocking operations or MPI_Sendrecv

[Program hangs or times out, demonstrating the deadlock]
```

**Key Points to Highlight**:
- Static analysis detects potential deadlock patterns
- Warnings are issued before the deadlock occurs
- Specific recommendations are provided for fixing the issue
- Source location helps developers locate the problem

### Demo 4: Performance Monitoring

**Objective**: Show performance monitoring and optimization recommendations.

**Program**: `performance/bandwidth_test.c`

**Demo Steps**:

1. **Run with performance monitoring enabled**:
```bash
# Set environment variable to enable performance monitoring
export MPI_SANITIZER_OPTIONS="enable_performance=1:timing_details=1"
./run_instrumented.sh performance/bandwidth_test -np 2
```

**Expected Output**:
```
[MPI Sanitizer] Performance monitoring enabled
[MPI Sanitizer] Pre-call: MPI_Send at bandwidth_test.c:45
[MPI Sanitizer]   - Performance timer started
[MPI Sanitizer]   - Message size: 1048576 bytes (1.0 MB)
[MPI Sanitizer] Post-call: MPI_Send returned MPI_SUCCESS
[MPI Sanitizer]   - Execution time: 1.234 ms
[MPI Sanitizer]   - Bandwidth: 849.5 MB/s
[MPI Sanitizer]   - Efficiency: 85% of theoretical peak

=== MPI Performance Report ===
Function: MPI_Send
  Total Calls: 100
  Total Time: 123.4 ms
  Average Time: 1.234 ms
  Min Time: 0.987 ms
  Max Time: 2.156 ms
  Total Data: 100 MB
  Average Bandwidth: 810.4 MB/s

Recommendations:
- Consider using non-blocking operations for better overlap
- Message size is optimal for this network configuration
- No significant performance bottlenecks detected
===============================
```

**Key Points to Highlight**:
- Detailed timing information for each MPI operation
- Bandwidth calculations and efficiency metrics
- Performance recommendations based on usage patterns
- Statistical analysis across multiple operations

### Demo 5: Multi-Language Support

**Objective**: Demonstrate consistent instrumentation across different languages.

**Programs**: 
- `multi_language/cpp_bindings/mpi_cpp_example.cpp`
- `multi_language/fortran_bindings/hello_world.f90`

**Demo Steps**:

1. **C++ Example**:
```bash
./run_instrumented.sh multi_language/cpp_bindings/mpi_cpp_example -np 2
```

2. **Fortran Example**:
```bash
./run_instrumented.sh multi_language/fortran_bindings/hello_world -np 2
```

**Key Points to Highlight**:
- Same instrumentation quality across all supported languages
- Proper handling of language-specific features (C++ templates, Fortran name mangling)
- Consistent error detection and performance monitoring

## Advanced Demo Features

### Configuration Demonstration

**Objective**: Show flexible configuration options.

1. **Create a custom configuration file**:
```bash
cat > /tmp/demo_config.conf << 'EOF'
# MPI Sanitizer Demo Configuration
instrumentation_mode = "standard"
enable_optimizations = true
enable_performance_monitoring = true

# Error detection settings
enable_deadlock_detection = true
enable_data_race_detection = true
enable_parameter_validation = true

# Output settings
report_file = "mpi_sanitizer_demo_report.txt"
verbose_output = true

# Function-specific settings
[function_policies]
MPI_Send = { enable_pre_hooks = true, enable_post_hooks = true, enable_timing = true }
MPI_Recv = { enable_pre_hooks = true, enable_post_hooks = true, enable_timing = true }
MPI_Bcast = { enable_pre_hooks = false, enable_post_hooks = true }
EOF
```

2. **Compile with custom configuration**:
```bash
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -passes=mpi-sanitizer \
      -mllvm -mpi-sanitizer-config=/tmp/demo_config.conf \
      basic/hello_world.c -o /tmp/configured_hello -lmpi -lmpi_sanitizer_runtime
```

3. **Run and show customized output**:
```bash
mpirun -np 2 /tmp/configured_hello
cat mpi_sanitizer_demo_report.txt
```

### Performance Comparison

**Objective**: Demonstrate the overhead measurement capabilities.

```bash
# Run performance comparison script
cd build
./performance_comparison.sh basic/hello_world -np 4
```

**Expected Output**:
```
=== MPI Usage Sanitizer Performance Comparison ===

Program: basic/hello_world
Processes: 4
Iterations: 10

Uninstrumented Performance:
  Average execution time: 0.045s
  Standard deviation: 0.003s

Instrumented Performance:
  Average execution time: 0.052s
  Standard deviation: 0.004s

Performance Impact:
  Overhead: 0.007s (15.6%)
  Overhead per MPI call: 0.0018s
  
Assessment: Acceptable overhead for development/testing
Recommendation: Use lightweight mode for production
================================================
```

## Demo Script Template

Here's a complete demo script you can use:

```bash
#!/bin/bash
# MPI Usage Sanitizer Demo Script

echo "=== MPI Usage Sanitizer LLVM Pass Demo ==="
echo ""

# Demo 1: Basic instrumentation
echo "Demo 1: Basic MPI Program Instrumentation"
echo "Running hello_world without sanitizer:"
./run_uninstrumented.sh basic/hello_world -np 2
echo ""
echo "Running hello_world WITH sanitizer:"
./run_instrumented.sh basic/hello_world -np 2
echo ""
read -p "Press Enter to continue to next demo..."

# Demo 2: Point-to-point communication
echo "Demo 2: Point-to-Point Communication Validation"
echo "Running send_recv with detailed buffer validation:"
./run_instrumented.sh basic/send_recv -np 2
echo ""
read -p "Press Enter to continue to next demo..."

# Demo 3: Performance monitoring
echo "Demo 3: Performance Monitoring"
echo "Running with performance monitoring enabled:"
export MPI_SANITIZER_OPTIONS="enable_performance=1"
./run_instrumented.sh performance/bandwidth_test -np 2
echo ""
read -p "Press Enter to continue to next demo..."

# Demo 4: Performance comparison
echo "Demo 4: Performance Impact Assessment"
./performance_comparison.sh basic/hello_world -np 2
echo ""

echo "=== Demo Complete ==="
echo "The MPI Usage Sanitizer provides:"
echo "- Comprehensive error detection"
echo "- Performance monitoring and analysis"
echo "- Multi-language support (C, C++, Fortran)"
echo "- Flexible configuration options"
echo "- Production-ready LLVM integration"
```

## Key Demo Talking Points

### For Technical Audiences
1. **LLVM Integration**: Show how the pass integrates seamlessly with LLVM's pass manager
2. **Static Analysis**: Demonstrate compile-time optimization and error detection
3. **Runtime Efficiency**: Highlight low overhead and performance monitoring capabilities
4. **Extensibility**: Show configuration options and customization possibilities

### For Management/Decision Makers
1. **Error Prevention**: Demonstrate how it catches bugs before they reach production
2. **Performance Optimization**: Show how it identifies bottlenecks and optimization opportunities
3. **Development Efficiency**: Highlight faster debugging and development cycles
4. **Production Readiness**: Emphasize LLVM integration and industry standards compliance

### For HPC/MPI Developers
1. **Comprehensive Coverage**: Show support for all major MPI operations and patterns
2. **Multi-Language Support**: Demonstrate C, C++, and Fortran compatibility
3. **Performance Analysis**: Highlight detailed performance monitoring capabilities
4. **Best Practices**: Show how it helps enforce MPI programming best practices

## Troubleshooting Demo Issues

### Common Issues and Solutions

1. **MPI Sanitizer pass not found**:
   - Verify LLVM installation includes the MPI sanitizer
   - Check library paths and plugin loading

2. **Runtime library missing**:
   - Ensure `libmpi_sanitizer_runtime.so` is available
   - Check `LD_LIBRARY_PATH` settings

3. **MPI not available**:
   - Install MPI development packages
   - Verify `mpicc` and `mpirun` are in PATH

4. **Examples don't compile**:
   - Check compiler compatibility
   - Verify MPI headers and libraries are available

This comprehensive demo guide provides everything needed to showcase the MPI Usage Sanitizer's capabilities effectively to different audiences.
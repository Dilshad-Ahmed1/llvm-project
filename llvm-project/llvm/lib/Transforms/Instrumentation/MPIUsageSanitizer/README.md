# MPI Usage Sanitizer LLVM Pass

The MPI Usage Sanitizer is an LLVM transformation pass that instruments MPI (Message Passing Interface) programs for runtime error detection and performance monitoring. It provides comprehensive analysis and instrumentation capabilities to help developers identify and debug MPI-related issues in parallel applications.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Command Line Options](#command-line-options)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [Performance](#performance)
- [Contributing](#contributing)
- [License](#license)

## Overview

The MPI Usage Sanitizer LLVM Pass is designed to detect common MPI programming errors and performance issues at runtime by inserting instrumentation hooks around MPI function calls. It supports multiple MPI language bindings (C, C++, Fortran) and provides configurable instrumentation levels to balance between detection capability and performance overhead.

### Key Capabilities

- **Error Detection**: Identifies common MPI programming errors such as deadlocks, data races, and invalid parameter usage
- **Performance Monitoring**: Tracks communication patterns, message sizes, and timing information
- **Multi-Language Support**: Works with C, C++, and Fortran MPI programs
- **Configurable Instrumentation**: Supports different instrumentation levels from lightweight to comprehensive
- **Static Analysis Integration**: Uses compile-time analysis to optimize instrumentation placement
- **LLVM Integration**: Seamlessly integrates with LLVM's pass infrastructure and toolchain

## Features

### Error Detection Capabilities

- **Deadlock Detection**: Identifies potential deadlock situations in MPI communication patterns
- **Data Race Detection**: Detects concurrent access to shared MPI resources
- **Parameter Validation**: Validates MPI function parameters for correctness
- **Resource Leak Detection**: Identifies unreleased MPI resources (communicators, requests, etc.)
- **Type Safety**: Ensures MPI datatype consistency across operations
- **Buffer Overflow Detection**: Detects buffer overruns in MPI communication operations

### Performance Monitoring Features

- **Communication Pattern Analysis**: Tracks message passing patterns and identifies bottlenecks
- **Timing Analysis**: Measures MPI operation execution times and identifies slow operations
- **Message Size Tracking**: Monitors communication volume and message size distributions
- **Load Balancing Analysis**: Identifies imbalanced communication patterns
- **Collective Operation Monitoring**: Analyzes collective communication efficiency
- **Memory Usage Tracking**: Monitors MPI-related memory allocation and usage

### Instrumentation Modes

1. **Lightweight Mode**: Minimal overhead instrumentation for production use
2. **Standard Mode**: Balanced instrumentation providing good error detection with reasonable overhead
3. **Full Mode**: Comprehensive instrumentation for thorough debugging and analysis
4. **Selective Mode**: Configurable instrumentation based on static analysis results
5. **Debug Mode**: Maximum instrumentation for detailed debugging information

## Installation

### Prerequisites

- LLVM 14.0 or later
- CMake 3.16 or later
- C++17 compatible compiler
- MPI implementation (OpenMPI, MPICH, Intel MPI, etc.)

### Building from Source

1. **Clone the LLVM repository** (if not already available):
   ```bash
   git clone https://github.com/llvm/llvm-project.git
   cd llvm-project
   ```

2. **Configure the build** with MPI Sanitizer enabled:
   ```bash
   mkdir build && cd build
   cmake -G Ninja ../llvm \
     -DCMAKE_BUILD_TYPE=Release \
     -DLLVM_ENABLE_PROJECTS="clang" \
     -DLLVM_TARGETS_TO_BUILD="X86;AArch64" \
     -DLLVM_ENABLE_MPI_SANITIZER=ON
   ```

3. **Build LLVM with MPI Sanitizer**:
   ```bash
   ninja
   ```

4. **Install** (optional):
   ```bash
   ninja install
   ```

### Verification

Verify the installation by checking if the pass is available:
```bash
opt -load-pass-plugin=LLVMMPIUsageSanitizerComponents.so -passes=help | grep mpi-sanitizer
```

## Usage

### Basic Usage

The MPI Usage Sanitizer can be used in several ways:

#### 1. Using Clang with Pass Plugins

```bash
# Compile with MPI Sanitizer instrumentation
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -Xclang -load -Xclang LLVMMPIUsageSanitizerComponents.so \
      -mllvm -passes=mpi-sanitizer \
      -o myprogram myprogram.c -lmpi
```

#### 2. Using opt (LLVM Optimizer)

```bash
# Generate LLVM IR
clang -S -emit-llvm myprogram.c -o myprogram.ll

# Apply MPI Sanitizer pass
opt -load-pass-plugin=LLVMMPIUsageSanitizerComponents.so \
    -passes=mpi-sanitizer \
    -S myprogram.ll -o myprogram_instrumented.ll

# Compile instrumented IR
clang myprogram_instrumented.ll -o myprogram -lmpi -lmpi_sanitizer_runtime
```

#### 3. Integration with Build Systems

**CMake Integration:**
```cmake
# Add MPI Sanitizer flags to your CMakeLists.txt
if(ENABLE_MPI_SANITIZER)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpass-plugin=LLVMMPIUsageSanitizerComponents.so")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpass-plugin=LLVMMPIUsageSanitizerComponents.so")
    target_link_libraries(your_target mpi_sanitizer_runtime)
endif()
```

**Makefile Integration:**
```makefile
# Add to your Makefile
ifdef ENABLE_MPI_SANITIZER
    CFLAGS += -fpass-plugin=LLVMMPIUsageSanitizerComponents.so
    CXXFLAGS += -fpass-plugin=LLVMMPIUsageSanitizerComponents.so
    LDFLAGS += -lmpi_sanitizer_runtime
endif
```

### Runtime Execution

When running instrumented programs, the MPI Sanitizer runtime will automatically:

1. **Initialize** monitoring systems at MPI_Init
2. **Track** MPI operations during execution
3. **Report** detected issues to stderr or specified output files
4. **Generate** performance reports (if enabled)
5. **Cleanup** resources at MPI_Finalize

```bash
# Run instrumented program
mpirun -np 4 ./myprogram

# Run with specific runtime options
MPI_SANITIZER_OPTIONS="report_file=mpi_report.txt:enable_performance=1" \
mpirun -np 4 ./myprogram
```

## Configuration

The MPI Usage Sanitizer supports extensive configuration through multiple mechanisms:

### 1. Command Line Options

Pass options directly to the LLVM pass:

```bash
opt -passes=mpi-sanitizer \
    -mpi-sanitizer-level=standard \
    -mpi-sanitizer-enable-optimizations=true \
    -mpi-sanitizer-enable-performance=false \
    -mpi-sanitizer-report-file=report.txt \
    input.ll -o output.ll
```

### 2. Configuration Files

Create a configuration file (e.g., `mpi_sanitizer.conf`):

```ini
# MPI Sanitizer Configuration File

# Instrumentation settings
instrumentation_mode = "standard"
enable_optimizations = true
enable_performance_monitoring = false

# Error detection settings
enable_deadlock_detection = true
enable_data_race_detection = true
enable_parameter_validation = true

# Performance settings
max_performance_impact = 0.1
enable_timing_analysis = false

# Output settings
report_file = "mpi_sanitizer_report.txt"
verbose_output = false

# Function-specific settings
[function_policies]
MPI_Send = { enable_pre_hooks = true, enable_post_hooks = true }
MPI_Recv = { enable_pre_hooks = true, enable_post_hooks = true }
MPI_Bcast = { enable_pre_hooks = false, enable_post_hooks = true }
```

Use the configuration file:
```bash
opt -passes=mpi-sanitizer -mpi-sanitizer-config=mpi_sanitizer.conf input.ll -o output.ll
```

### 3. Environment Variables

Configure runtime behavior using environment variables:

```bash
# Set instrumentation level
export MPI_SANITIZER_LEVEL=standard

# Enable performance monitoring
export MPI_SANITIZER_ENABLE_PERFORMANCE=1

# Set output file
export MPI_SANITIZER_REPORT_FILE=mpi_report.txt

# Enable verbose output
export MPI_SANITIZER_VERBOSE=1
```

## Command Line Options

### Pass-Level Options

| Option | Description | Default | Values |
|--------|-------------|---------|---------|
| `-mpi-sanitizer-level` | Instrumentation level | `standard` | `none`, `lightweight`, `standard`, `full`, `debug` |
| `-mpi-sanitizer-enable-optimizations` | Enable static analysis optimizations | `true` | `true`, `false` |
| `-mpi-sanitizer-enable-performance` | Enable performance monitoring | `false` | `true`, `false` |
| `-mpi-sanitizer-enable-deadlock-detection` | Enable deadlock detection | `true` | `true`, `false` |
| `-mpi-sanitizer-enable-data-race-detection` | Enable data race detection | `true` | `true`, `false` |
| `-mpi-sanitizer-report-file` | Output report file | `stderr` | File path |
| `-mpi-sanitizer-config` | Configuration file path | None | File path |
| `-mpi-sanitizer-max-overhead` | Maximum acceptable overhead | `0.5` | Float (0.0-1.0) |

### Runtime Environment Variables

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| `MPI_SANITIZER_OPTIONS` | Runtime options string | None | `report_file=out.txt:verbose=1` |
| `MPI_SANITIZER_LEVEL` | Runtime instrumentation level | Compile-time setting | `standard` |
| `MPI_SANITIZER_REPORT_FILE` | Runtime report file | `stderr` | `/tmp/mpi_report.txt` |
| `MPI_SANITIZER_VERBOSE` | Enable verbose output | `0` | `1` |
| `MPI_SANITIZER_DISABLE` | Disable runtime checking | `0` | `1` |

## Examples

### Example 1: Basic MPI Program

**Source Code (hello_mpi.c):**
```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from rank %d of %d\n", rank, size);
    
    MPI_Finalize();
    return 0;
}
```

**Compilation and Execution:**
```bash
# Compile with MPI Sanitizer
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=standard \
      hello_mpi.c -o hello_mpi -lmpi -lmpi_sanitizer_runtime

# Run the program
mpirun -np 4 ./hello_mpi
```

**Expected Output:**
```
Hello from rank 0 of 4
Hello from rank 1 of 4
Hello from rank 2 of 4
Hello from rank 3 of 4

=== MPI Sanitizer Report ===
Program: hello_mpi
MPI Calls Detected: 4
Issues Found: 0
Performance Summary: No significant bottlenecks detected
============================
```

### Example 2: Point-to-Point Communication

**Source Code (ping_pong.c):**
```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (size != 2) {
        printf("This program requires exactly 2 processes\n");
        MPI_Finalize();
        return 1;
    }
    
    int data = rank;
    if (rank == 0) {
        MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Rank 0 received: %d\n", data);
    } else {
        MPI_Recv(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Rank 1 received: %d\n", data);
        data = 42;
        MPI_Send(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
    return 0;
}
```

**Compilation with Performance Monitoring:**
```bash
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=standard \
      -mllvm -mpi-sanitizer-enable-performance=true \
      ping_pong.c -o ping_pong -lmpi -lmpi_sanitizer_runtime
```

### Example 3: Collective Communication

**Source Code (collective.c):**
```c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Broadcast from root
    int data = (rank == 0) ? 100 : 0;
    MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // All-reduce sum
    int local_sum = rank + 1;
    int global_sum;
    MPI_Allreduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    
    printf("Rank %d: data=%d, global_sum=%d\n", rank, data, global_sum);
    
    MPI_Finalize();
    return 0;
}
```

**Configuration File (collective.conf):**
```ini
instrumentation_mode = "full"
enable_performance_monitoring = true
enable_collective_analysis = true
report_file = "collective_report.txt"

[function_policies]
MPI_Bcast = { enable_timing = true, enable_volume_tracking = true }
MPI_Allreduce = { enable_timing = true, enable_pattern_analysis = true }
```

**Compilation and Execution:**
```bash
# Compile with configuration file
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-config=collective.conf \
      collective.c -o collective -lmpi -lmpi_sanitizer_runtime

# Run with 4 processes
mpirun -np 4 ./collective
```

## Troubleshooting

### Common Issues and Solutions

#### 1. Pass Not Found Error

**Error:**
```
opt: error: unknown pass name 'mpi-sanitizer'
```

**Solution:**
- Ensure the pass plugin is properly loaded:
  ```bash
  opt -load-pass-plugin=LLVMMPIUsageSanitizerComponents.so -passes=mpi-sanitizer
  ```
- Verify the plugin file exists and is in the correct location
- Check LLVM version compatibility

#### 2. Runtime Library Not Found

**Error:**
```
./myprogram: error while loading shared libraries: libmpi_sanitizer_runtime.so: cannot open shared object file
```

**Solution:**
- Ensure the runtime library is installed and in the library path:
  ```bash
  export LD_LIBRARY_PATH=/path/to/mpi/sanitizer/lib:$LD_LIBRARY_PATH
  ```
- Link the runtime library explicitly:
  ```bash
  clang myprogram.c -o myprogram -lmpi -lmpi_sanitizer_runtime
  ```

#### 3. High Performance Overhead

**Issue:** Instrumented program runs significantly slower than expected.

**Solutions:**
- Use lightweight instrumentation mode:
  ```bash
  -mllvm -mpi-sanitizer-level=lightweight
  ```
- Enable optimizations:
  ```bash
  -mllvm -mpi-sanitizer-enable-optimizations=true
  ```
- Disable performance monitoring if not needed:
  ```bash
  -mllvm -mpi-sanitizer-enable-performance=false
  ```
- Use selective instrumentation with configuration files

#### 4. False Positive Deadlock Detection

**Issue:** Sanitizer reports deadlocks that don't actually occur.

**Solutions:**
- Adjust deadlock detection sensitivity in configuration
- Use static analysis to reduce false positives:
  ```ini
  enable_static_analysis = true
  deadlock_detection_sensitivity = "low"
  ```
- Exclude specific functions from deadlock detection if necessary

#### 5. Missing MPI Function Detection

**Issue:** Some MPI calls are not being instrumented.

**Solutions:**
- Verify MPI function signatures are recognized:
  ```bash
  opt -passes=mpi-sanitizer -mpi-sanitizer-verbose=true
  ```
- Check for non-standard MPI implementations or custom wrappers
- Add custom function signatures to configuration file
- Ensure proper linking with MPI libraries

### Debugging Tips

#### 1. Enable Verbose Output

```bash
# Compile-time verbosity
opt -passes=mpi-sanitizer -mpi-sanitizer-verbose=true

# Runtime verbosity
export MPI_SANITIZER_VERBOSE=1
```

#### 2. Generate Detailed Reports

```bash
# Enable comprehensive reporting
export MPI_SANITIZER_OPTIONS="report_file=debug.txt:detailed_reports=1:include_statistics=1"
```

#### 3. Use Debug Instrumentation Mode

```bash
clang -fpass-plugin=LLVMMPIUsageSanitizerComponents.so \
      -mllvm -mpi-sanitizer-level=debug \
      myprogram.c -o myprogram -lmpi -lmpi_sanitizer_runtime
```

#### 4. Analyze LLVM IR

```bash
# Generate and inspect instrumented IR
clang -S -emit-llvm myprogram.c -o myprogram.ll
opt -passes=mpi-sanitizer -S myprogram.ll -o myprogram_instrumented.ll
# Inspect myprogram_instrumented.ll to see inserted hooks
```

### Performance Tuning

#### 1. Optimize for Production Use

```ini
# Production configuration
instrumentation_mode = "lightweight"
enable_optimizations = true
enable_performance_monitoring = false
enable_static_analysis = true
max_performance_impact = 0.05
```

#### 2. Selective Instrumentation

```ini
# Only instrument critical functions
[function_policies]
MPI_Send = { enabled = true }
MPI_Recv = { enabled = true }
MPI_Bcast = { enabled = false }  # Skip collective operations
MPI_Allreduce = { enabled = false }
```

#### 3. Conditional Compilation

```c
#ifdef ENABLE_MPI_SANITIZER
    // MPI Sanitizer specific code
#endif
```

## Performance

### Overhead Characteristics

The MPI Usage Sanitizer is designed to minimize performance impact while providing comprehensive error detection:

| Instrumentation Mode | Typical Overhead | Use Case |
|---------------------|------------------|----------|
| Lightweight | 5-15% | Production monitoring |
| Standard | 15-50% | Development and testing |
| Full | 50-200% | Comprehensive debugging |
| Debug | 200%+ | Detailed analysis |

### Optimization Features

- **Static Analysis**: Reduces instrumentation overhead by analyzing code at compile time
- **Selective Instrumentation**: Only instruments potentially problematic code paths
- **Adaptive Monitoring**: Adjusts monitoring intensity based on detected patterns
- **Efficient Runtime**: Optimized runtime library with minimal overhead data structures

### Scalability

The MPI Usage Sanitizer is designed to scale with application size:

- **Linear Scaling**: Instrumentation overhead scales linearly with the number of MPI calls
- **Memory Efficiency**: Constant memory overhead per MPI operation
- **Parallel Efficiency**: Minimal impact on parallel scaling characteristics

## Contributing

We welcome contributions to the MPI Usage Sanitizer project! Please see our [Contributing Guide](CONTRIBUTING.md) for details on:

- Code style and conventions
- Testing requirements
- Submission process
- Development setup

### Development Setup

1. **Clone the repository**
2. **Set up development environment**
3. **Build and test**
4. **Submit pull requests**

## License

The MPI Usage Sanitizer LLVM Pass is part of the LLVM Project and is licensed under the Apache License v2.0 with LLVM Exceptions. See [LICENSE.txt](LICENSE.txt) for details.

## Support

For support and questions:

- **Documentation**: This README and inline code documentation
- **Issues**: GitHub Issues for bug reports and feature requests
- **Discussions**: GitHub Discussions for questions and community support
- **Mailing Lists**: LLVM development mailing lists for technical discussions

## Acknowledgments

The MPI Usage Sanitizer builds upon the excellent foundation provided by:

- The LLVM Project and community
- Existing sanitizer implementations (AddressSanitizer, ThreadSanitizer)
- MPI standard and implementations
- Research in parallel program analysis and debugging tools
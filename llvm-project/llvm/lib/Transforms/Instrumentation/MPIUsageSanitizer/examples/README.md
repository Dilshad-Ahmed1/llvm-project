# MPI Usage Sanitizer Examples

This directory contains comprehensive example programs that demonstrate the MPI Usage Sanitizer LLVM Pass functionality. These examples show how the pass instruments different MPI usage patterns, provides runtime error detection, and enables performance monitoring.

## Directory Structure

```
examples/
├── README.md                    # This file
├── basic/                       # Basic MPI usage patterns
├── collective/                  # Collective operation examples
├── point_to_point/             # Point-to-point communication examples
├── advanced/                   # Advanced MPI patterns
├── error_cases/                # Examples that trigger sanitizer warnings
├── performance/                # Performance monitoring demonstrations
├── multi_language/             # C, Fortran, and C++ examples
├── build_scripts/              # Build and test scripts
└── instrumentation_output/     # Sample instrumentation output
```

## Building Examples

To build and test the examples:

```bash
# Build all examples
cd examples/build_scripts
./build_all.sh

# Run specific example with instrumentation
./run_instrumented.sh basic/hello_world.c

# Compare performance with/without instrumentation
./performance_comparison.sh collective/allreduce_benchmark.c
```

## Example Categories

### Basic Examples
- **hello_world.c**: Simple MPI initialization and finalization
- **send_recv.c**: Basic point-to-point communication
- **broadcast.c**: Simple collective operation

### Collective Operations
- **allreduce_patterns.c**: Various allreduce usage patterns
- **gather_scatter.c**: Gather and scatter operations
- **barrier_sync.c**: Synchronization patterns

### Point-to-Point Communication
- **nonblocking_comm.c**: Non-blocking send/receive patterns
- **persistent_comm.c**: Persistent communication requests
- **derived_datatypes.c**: Custom MPI datatypes

### Advanced Patterns
- **communicator_management.c**: Communicator creation and management
- **topology_aware.c**: Topology-aware communication
- **one_sided_comm.c**: RMA (Remote Memory Access) operations

### Error Cases
- **deadlock_example.c**: Potential deadlock scenarios
- **buffer_overflow.c**: Buffer size mismatches
- **type_mismatch.c**: Datatype inconsistencies

### Performance Examples
- **bandwidth_test.c**: Communication bandwidth measurement
- **latency_test.c**: Communication latency analysis
- **scalability_test.c**: Scalability analysis patterns

### Multi-Language Examples
- **fortran_bindings/**: Fortran MPI examples
- **cpp_bindings/**: C++ MPI examples
- **mixed_language/**: Mixed C/Fortran programs

## Instrumentation Output

Each example includes sample instrumentation output showing:
- Pre-call and post-call hook execution
- Parameter validation results
- Performance monitoring data
- Error detection reports

## Requirements Validation

These examples validate requirements:
- **9.1**: Comprehensive unit testing framework
- **8.5**: Performance monitoring integration

The examples demonstrate all major MPI usage patterns and show how the sanitizer pass instruments them for runtime analysis.
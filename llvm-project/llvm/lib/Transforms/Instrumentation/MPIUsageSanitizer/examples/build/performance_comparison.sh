#!/bin/bash

# Performance comparison script for MPI Usage Sanitizer
# Compares execution time between instrumented and uninstrumented versions

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLES_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$EXAMPLES_DIR/build"
INSTRUMENTED_DIR="$BUILD_DIR/instrumented"
UNINSTRUMENTED_DIR="$BUILD_DIR/uninstrumented"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[PERF]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to run a program and measure time
measure_execution_time() {
    local program_path="$1"
    shift
    local mpi_args=("$@")
    
    if [ ! -f "$program_path" ]; then
        echo "0.0"
        return 1
    fi
    
    # Run the program 3 times and take the average
    local total_time=0
    local runs=3
    local successful_runs=0
    
    for i in $(seq 1 $runs); do
        local start_time=$(date +%s.%N)
        
        # Run with timeout to prevent hanging (use gtimeout on macOS if available, otherwise skip timeout)
        local timeout_cmd=""
        if command -v timeout >/dev/null 2>&1; then
            timeout_cmd="timeout 30s"
        elif command -v gtimeout >/dev/null 2>&1; then
            timeout_cmd="gtimeout 30s"
        fi
        
        if [ -n "$timeout_cmd" ]; then
            timeout_result=$($timeout_cmd mpirun "${mpi_args[@]}" "$program_path" >/dev/null 2>&1; echo $?)
        else
            timeout_result=$(mpirun "${mpi_args[@]}" "$program_path" >/dev/null 2>&1; echo $?)
        fi
        
        if [ "$timeout_result" -eq 0 ]; then
            local end_time=$(date +%s.%N)
            local run_time=$(echo "$end_time - $start_time" | bc -l)
            total_time=$(echo "$total_time + $run_time" | bc -l)
            successful_runs=$((successful_runs + 1))
        fi
    done
    
    if [ $successful_runs -gt 0 ]; then
        local avg_time=$(echo "scale=3; $total_time / $successful_runs" | bc -l)
        echo "$avg_time"
        return 0
    else
        echo "0.0"
        return 1
    fi
}

# Main performance comparison function
compare_performance() {
    local program="$1"
    shift
    local mpi_args=("$@")
    
    local uninstrumented_path="$UNINSTRUMENTED_DIR/$program"
    local instrumented_path="$INSTRUMENTED_DIR/$program"
    
    print_status "Performance comparison for: $program"
    print_status "MPI arguments: ${mpi_args[*]}"
    echo ""
    
    # Check if both versions exist
    if [ ! -f "$uninstrumented_path" ]; then
        print_error "Uninstrumented version not found: $uninstrumented_path"
        return 1
    fi
    
    if [ ! -f "$instrumented_path" ]; then
        print_error "Instrumented version not found: $instrumented_path"
        return 1
    fi
    
    # Measure uninstrumented version
    print_status "Measuring uninstrumented version..."
    local uninstrumented_time
    uninstrumented_time=$(measure_execution_time "$uninstrumented_path" "${mpi_args[@]}")
    local uninstrumented_status=$?
    
    # Measure instrumented version
    print_status "Measuring instrumented version..."
    local instrumented_time
    instrumented_time=$(measure_execution_time "$instrumented_path" "${mpi_args[@]}")
    local instrumented_status=$?
    
    # Display results
    echo ""
    echo "================================================"
    echo "Performance Comparison Results"
    echo "================================================"
    echo "Program: $program"
    echo "MPI Args: ${mpi_args[*]}"
    echo ""
    
    if [ $uninstrumented_status -eq 0 ]; then
        printf "Uninstrumented: %.3fs\n" "$uninstrumented_time"
    else
        echo "Uninstrumented: FAILED"
    fi
    
    if [ $instrumented_status -eq 0 ]; then
        printf "Instrumented:   %.3fs\n" "$instrumented_time"
    else
        echo "Instrumented:   FAILED"
    fi
    
    # Calculate overhead if both succeeded
    if [ $uninstrumented_status -eq 0 ] && [ $instrumented_status -eq 0 ]; then
        local overhead_time=$(echo "$instrumented_time - $uninstrumented_time" | bc -l)
        local overhead_percent
        
        if [ "$(echo "$uninstrumented_time > 0" | bc -l)" -eq 1 ]; then
            overhead_percent=$(echo "scale=1; ($overhead_time / $uninstrumented_time) * 100" | bc -l)
        else
            overhead_percent="N/A"
        fi
        
        echo ""
        printf "Overhead: %.3fs" "$overhead_time"
        if [ "$overhead_percent" != "N/A" ]; then
            printf " (%.1f%%)" "$overhead_percent"
        fi
        echo ""
        
        # Determine if overhead is acceptable
        if [ "$overhead_percent" != "N/A" ] && [ "$(echo "$overhead_percent < 50.0" | bc -l)" -eq 1 ]; then
            print_success "Performance overhead is acceptable"
        elif [ "$overhead_percent" != "N/A" ]; then
            print_warning "Performance overhead is high (>50%)"
        fi
    else
        echo ""
        print_error "Cannot calculate overhead - one or both versions failed"
    fi
    
    echo "================================================"
    echo ""
}

# Usage function
usage() {
    echo "Usage: $0 <category/program> [mpi_args...]"
    echo ""
    echo "Examples:"
    echo "  $0 basic/hello_world -np 2"
    echo "  $0 performance/bandwidth_test -np 4"
    echo "  $0 collective/allreduce_patterns -np 8"
    echo ""
    echo "Available programs:"
    if [ -d "$UNINSTRUMENTED_DIR" ]; then
        find "$UNINSTRUMENTED_DIR" -type f -executable | sed "s|$UNINSTRUMENTED_DIR/||" | sort
    else
        echo "  (Build directory not found - run build_all.sh first)"
    fi
}

# Check dependencies
check_dependencies() {
    if ! command -v bc &> /dev/null; then
        print_error "bc (calculator) not found. Please install bc package."
        exit 1
    fi
    
    if ! command -v mpirun &> /dev/null; then
        print_error "mpirun not found. Please install MPI."
        exit 1
    fi
    
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found. Please run build_all.sh first."
        exit 1
    fi
}

# Main script
main() {
    if [ $# -eq 0 ]; then
        usage
        exit 1
    fi
    
    check_dependencies
    
    local program="$1"
    shift
    local mpi_args=("$@")
    
    # Default MPI args if none provided
    if [ ${#mpi_args[@]} -eq 0 ]; then
        mpi_args=("-np" "2")
        print_status "No MPI arguments provided, using default: ${mpi_args[*]}"
    fi
    
    compare_performance "$program" "${mpi_args[@]}"
}

# Run main function
main "$@"
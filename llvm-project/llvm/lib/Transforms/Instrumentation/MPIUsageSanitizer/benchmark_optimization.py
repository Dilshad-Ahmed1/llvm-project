#!/usr/bin/env python3
"""
Benchmark script for Task 18.1: Optimize instrumentation overhead

This script runs comprehensive benchmarks to validate the performance
optimizations implemented for the MPI Usage Sanitizer Pass.
"""

import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Tuple, Any

class OptimizationBenchmark:
    """Comprehensive benchmark suite for MPI sanitizer pass optimization."""
    
    def __init__(self, llvm_build_dir: str, test_dir: str):
        self.llvm_build_dir = Path(llvm_build_dir)
        self.test_dir = Path(test_dir)
        self.results = {}
        
        # Benchmark configurations
        self.configurations = {
            'baseline': {
                'enable_profiling': False,
                'enable_optimization': False,
                'instrumentation_mode': 'standard'
            },
            'profiling_only': {
                'enable_profiling': True,
                'enable_optimization': False,
                'instrumentation_mode': 'standard'
            },
            'optimized': {
                'enable_profiling': True,
                'enable_optimization': True,
                'instrumentation_mode': 'selective'
            },
            'aggressive': {
                'enable_profiling': True,
                'enable_optimization': True,
                'instrumentation_mode': 'selective',
                'optimization_level': 'aggressive'
            }
        }
        
        # Test module sizes for scalability testing
        self.test_sizes = [50, 100, 200, 500, 1000]
        
    def run_all_benchmarks(self) -> Dict[str, Any]:
        """Run all optimization benchmarks."""
        print("Starting MPI Sanitizer Pass Optimization Benchmarks...")
        
        # 1. Basic optimization effectiveness
        print("\n1. Testing basic optimization effectiveness...")
        self.results['basic_optimization'] = self.test_basic_optimization()
        
        # 2. Hot path optimization
        print("\n2. Testing hot path optimization...")
        self.results['hot_path_optimization'] = self.test_hot_path_optimization()
        
        # 3. Memory optimization
        print("\n3. Testing memory optimization...")
        self.results['memory_optimization'] = self.test_memory_optimization()
        
        # 4. Scalability improvements
        print("\n4. Testing scalability improvements...")
        self.results['scalability'] = self.test_scalability()
        
        # 5. Profiling overhead
        print("\n5. Testing profiling overhead...")
        self.results['profiling_overhead'] = self.test_profiling_overhead()
        
        # 6. Combined optimization effectiveness
        print("\n6. Testing combined optimization effectiveness...")
        self.results['combined_optimization'] = self.test_combined_optimization()
        
        return self.results
    
    def test_basic_optimization(self) -> Dict[str, Any]:
        """Test basic optimization effectiveness."""
        results = {}
        
        # Create test module
        test_module = self.create_test_module("basic_optimization", 100, 10)
        
        # Run baseline
        baseline_time, baseline_memory = self.run_pass_benchmark(
            test_module, self.configurations['baseline']
        )
        
        # Run optimized
        optimized_time, optimized_memory = self.run_pass_benchmark(
            test_module, self.configurations['optimized']
        )
        
        # Calculate improvements
        speedup = baseline_time / optimized_time if optimized_time > 0 else 0
        memory_ratio = optimized_memory / baseline_memory if baseline_memory > 0 else 0
        
        results = {
            'baseline_time_us': baseline_time,
            'optimized_time_us': optimized_time,
            'speedup_ratio': speedup,
            'baseline_memory_mb': baseline_memory / (1024 * 1024),
            'optimized_memory_mb': optimized_memory / (1024 * 1024),
            'memory_ratio': memory_ratio,
            'passed': speedup > 1.1  # Expect at least 10% speedup
        }
        
        print(f"  Speedup: {speedup:.2f}x ({(speedup-1)*100:.1f}% improvement)")
        print(f"  Memory ratio: {memory_ratio:.2f}")
        print(f"  Test passed: {results['passed']}")
        
        return results
    
    def test_hot_path_optimization(self) -> Dict[str, Any]:
        """Test hot path optimization effectiveness."""
        results = {}
        
        # Create module with clear hot paths
        test_module = self.create_hot_path_test_module()
        
        # Run baseline
        baseline_time, baseline_memory = self.run_pass_benchmark(
            test_module, self.configurations['baseline']
        )
        
        # Run with hot path optimization
        hot_path_config = self.configurations['optimized'].copy()
        hot_path_config['enable_hot_path_optimization'] = True
        hot_path_config['hot_path_threshold'] = 0.05
        
        optimized_time, optimized_memory = self.run_pass_benchmark(
            test_module, hot_path_config
        )
        
        # Calculate improvements
        speedup = baseline_time / optimized_time if optimized_time > 0 else 0
        
        results = {
            'baseline_time_us': baseline_time,
            'optimized_time_us': optimized_time,
            'speedup_ratio': speedup,
            'passed': speedup > 1.2  # Expect at least 20% speedup for hot paths
        }
        
        print(f"  Hot path speedup: {speedup:.2f}x ({(speedup-1)*100:.1f}% improvement)")
        print(f"  Test passed: {results['passed']}")
        
        return results
    
    def test_memory_optimization(self) -> Dict[str, Any]:
        """Test memory optimization effectiveness."""
        results = {}
        
        # Create large test module
        test_module = self.create_test_module("memory_optimization", 200, 15)
        
        # Run baseline
        baseline_time, baseline_memory = self.run_pass_benchmark(
            test_module, self.configurations['baseline']
        )
        
        # Run with memory optimization
        memory_config = self.configurations['optimized'].copy()
        memory_config['enable_memory_optimization'] = True
        memory_config['max_cache_size'] = 500
        
        optimized_time, optimized_memory = self.run_pass_benchmark(
            test_module, memory_config
        )
        
        # Calculate improvements
        memory_reduction = 1.0 - (optimized_memory / baseline_memory) if baseline_memory > 0 else 0
        performance_ratio = optimized_time / baseline_time if baseline_time > 0 else 0
        
        results = {
            'baseline_memory_mb': baseline_memory / (1024 * 1024),
            'optimized_memory_mb': optimized_memory / (1024 * 1024),
            'memory_reduction': memory_reduction,
            'performance_ratio': performance_ratio,
            'passed': memory_reduction > 0.05 and performance_ratio < 1.2  # 5% memory reduction, <20% perf loss
        }
        
        print(f"  Memory reduction: {memory_reduction*100:.1f}%")
        print(f"  Performance ratio: {performance_ratio:.2f}")
        print(f"  Test passed: {results['passed']}")
        
        return results
    
    def test_scalability(self) -> Dict[str, Any]:
        """Test scalability improvements from optimization."""
        results = {
            'baseline_times': [],
            'optimized_times': [],
            'speedup_ratios': [],
            'scalability_improvement': 0.0,
            'passed': False
        }
        
        for size in self.test_sizes:
            print(f"  Testing size: {size} functions")
            
            # Create test module
            test_module = self.create_test_module("scalability", size, 8)
            
            # Run baseline
            baseline_time, _ = self.run_pass_benchmark(
                test_module, self.configurations['baseline']
            )
            
            # Run optimized
            optimized_time, _ = self.run_pass_benchmark(
                test_module, self.configurations['optimized']
            )
            
            speedup = baseline_time / optimized_time if optimized_time > 0 else 0
            
            results['baseline_times'].append(baseline_time)
            results['optimized_times'].append(optimized_time)
            results['speedup_ratios'].append(speedup)
            
            print(f"    Speedup: {speedup:.2f}x")
        
        # Analyze scalability improvement
        if len(results['speedup_ratios']) >= 2:
            # Check if speedup is maintained or improved with scale
            avg_speedup = sum(results['speedup_ratios']) / len(results['speedup_ratios'])
            results['scalability_improvement'] = avg_speedup
            results['passed'] = avg_speedup > 1.1 and min(results['speedup_ratios']) > 1.0
        
        print(f"  Average speedup: {results['scalability_improvement']:.2f}x")
        print(f"  Test passed: {results['passed']}")
        
        return results
    
    def test_profiling_overhead(self) -> Dict[str, Any]:
        """Test that profiling overhead is minimal."""
        results = {}
        
        # Create test module
        test_module = self.create_test_module("profiling_overhead", 100, 10)
        
        # Run without profiling
        no_profiling_time, no_profiling_memory = self.run_pass_benchmark(
            test_module, self.configurations['baseline']
        )
        
        # Run with profiling
        profiling_time, profiling_memory = self.run_pass_benchmark(
            test_module, self.configurations['profiling_only']
        )
        
        # Calculate overhead
        time_overhead = (profiling_time / no_profiling_time) - 1.0 if no_profiling_time > 0 else 0
        memory_overhead = (profiling_memory / no_profiling_memory) - 1.0 if no_profiling_memory > 0 else 0
        
        results = {
            'time_overhead': time_overhead,
            'memory_overhead': memory_overhead,
            'passed': time_overhead < 0.2 and memory_overhead < 0.3  # <20% time, <30% memory overhead
        }
        
        print(f"  Time overhead: {time_overhead*100:.1f}%")
        print(f"  Memory overhead: {memory_overhead*100:.1f}%")
        print(f"  Test passed: {results['passed']}")
        
        return results
    
    def test_combined_optimization(self) -> Dict[str, Any]:
        """Test combined optimization effectiveness."""
        results = {}
        
        # Create comprehensive test module
        test_module = self.create_test_module("combined_optimization", 150, 12)
        
        # Run baseline
        baseline_time, baseline_memory = self.run_pass_benchmark(
            test_module, self.configurations['baseline']
        )
        
        # Run with all optimizations
        aggressive_time, aggressive_memory = self.run_pass_benchmark(
            test_module, self.configurations['aggressive']
        )
        
        # Calculate improvements
        speedup = baseline_time / aggressive_time if aggressive_time > 0 else 0
        memory_ratio = aggressive_memory / baseline_memory if baseline_memory > 0 else 0
        
        results = {
            'baseline_time_us': baseline_time,
            'aggressive_time_us': aggressive_time,
            'speedup_ratio': speedup,
            'memory_ratio': memory_ratio,
            'passed': speedup > 1.3  # Expect at least 30% speedup with all optimizations
        }
        
        print(f"  Combined speedup: {speedup:.2f}x ({(speedup-1)*100:.1f}% improvement)")
        print(f"  Memory ratio: {memory_ratio:.2f}")
        print(f"  Test passed: {results['passed']}")
        
        return results
    
    def create_test_module(self, test_type: str, function_count: int, mpi_calls_per_function: int) -> str:
        """Create a test LLVM IR module for benchmarking."""
        module_name = f"{test_type}_{function_count}_{mpi_calls_per_function}"
        module_path = self.test_dir / f"{module_name}.ll"
        
        # Generate LLVM IR for test module
        ir_content = self.generate_test_ir(test_type, function_count, mpi_calls_per_function)
        
        with open(module_path, 'w') as f:
            f.write(ir_content)
        
        return str(module_path)
    
    def create_hot_path_test_module(self) -> str:
        """Create a test module with clear hot paths."""
        module_path = self.test_dir / "hot_path_test.ll"
        
        # Generate IR with one function having many MPI calls (hot path)
        # and several functions with few MPI calls (cold paths)
        ir_content = '''
; Hot path test module
target triple = "x86_64-unknown-linux-gnu"

declare i32 @MPI_Send(i8*, i32, i32, i32, i32, i32)
declare i32 @MPI_Recv(i8*, i32, i32, i32, i32, i32)

; Hot path function with many MPI calls
define i32 @hot_path_function() {
entry:
'''
        
        # Add many MPI calls to create hot path
        for i in range(50):
            ir_content += f'  call i32 @MPI_Send(i8* null, i32 100, i32 0, i32 {i % 4}, i32 {i}, i32 0)\n'
        
        ir_content += '  ret i32 0\n}\n\n'
        
        # Add cold path functions with few MPI calls
        for f in range(10):
            ir_content += f'''
define i32 @cold_path_function_{f}() {{
entry:
  call i32 @MPI_Send(i8* null, i32 50, i32 0, i32 0, i32 {f * 10}, i32 0)
  call i32 @MPI_Send(i8* null, i32 50, i32 0, i32 1, i32 {f * 10 + 1}, i32 0)
  ret i32 0
}}
'''
        
        with open(module_path, 'w') as f:
            f.write(ir_content)
        
        return str(module_path)
    
    def generate_test_ir(self, test_type: str, function_count: int, mpi_calls_per_function: int) -> str:
        """Generate LLVM IR for test module."""
        ir_content = f'''
; Test module for {test_type}
target triple = "x86_64-unknown-linux-gnu"

declare i32 @MPI_Send(i8*, i32, i32, i32, i32, i32)
declare i32 @MPI_Bcast(i8*, i32, i32, i32, i32)
declare i32 @MPI_Reduce(i8*, i8*, i32, i32, i32, i32, i32)

'''
        
        # Generate functions with MPI calls
        for f in range(function_count):
            ir_content += f'''
define i32 @test_function_{f}() {{
entry:
'''
            
            # Add MPI calls based on test type
            for i in range(mpi_calls_per_function):
                if test_type == "metadata_extraction":
                    # Varying parameter patterns for metadata extraction testing
                    count = 100 + i * 10
                    dest = i % 4
                    tag = i * 2
                    ir_content += f'  call i32 @MPI_Send(i8* null, i32 {count}, i32 0, i32 {dest}, i32 {tag}, i32 0)\n'
                elif i % 3 == 0:
                    # Point-to-point
                    ir_content += f'  call i32 @MPI_Send(i8* null, i32 100, i32 0, i32 {i % 4}, i32 {i}, i32 0)\n'
                elif i % 3 == 1:
                    # Collective
                    ir_content += f'  call i32 @MPI_Bcast(i8* null, i32 50, i32 0, i32 0, i32 0)\n'
                else:
                    # Reduction
                    ir_content += f'  call i32 @MPI_Reduce(i8* null, i8* null, i32 25, i32 0, i32 0, i32 0, i32 0)\n'
            
            ir_content += '  ret i32 0\n}\n\n'
        
        return ir_content
    
    def run_pass_benchmark(self, module_path: str, config: Dict[str, Any]) -> Tuple[int, int]:
        """Run the MPI sanitizer pass with given configuration and measure performance."""
        # Create configuration file
        config_path = self.test_dir / "benchmark_config.json"
        with open(config_path, 'w') as f:
            json.dump(config, f)
        
        # Run opt with MPI sanitizer pass
        opt_path = self.llvm_build_dir / "bin" / "opt"
        
        cmd = [
            str(opt_path),
            "-load-pass-plugin", str(self.llvm_build_dir / "lib" / "LLVMMPIUsageSanitizer.so"),
            "-passes=mpi-sanitizer",
            f"-mpi-sanitizer-config={config_path}",
            "-disable-output",
            module_path
        ]
        
        # Measure execution time and memory usage
        start_time = time.time()
        start_memory = self.get_memory_usage()
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            if result.returncode != 0:
                print(f"Warning: Pass execution failed: {result.stderr}")
                return 0, 0
        except subprocess.TimeoutExpired:
            print("Warning: Pass execution timed out")
            return 0, 0
        
        end_time = time.time()
        end_memory = self.get_memory_usage()
        
        execution_time_us = int((end_time - start_time) * 1000000)
        memory_usage = max(0, end_memory - start_memory)
        
        return execution_time_us, memory_usage
    
    def get_memory_usage(self) -> int:
        """Get current memory usage in bytes."""
        try:
            # Use /proc/self/status on Linux
            with open('/proc/self/status', 'r') as f:
                for line in f:
                    if line.startswith('VmRSS:'):
                        # Extract memory in kB and convert to bytes
                        memory_kb = int(line.split()[1])
                        return memory_kb * 1024
        except:
            pass
        return 0
    
    def generate_report(self, output_file: str = None):
        """Generate comprehensive benchmark report."""
        report = {
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'summary': self.generate_summary(),
            'detailed_results': self.results
        }
        
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(report, f, indent=2)
            print(f"\nDetailed report saved to: {output_file}")
        
        # Print summary
        print("\n" + "="*60)
        print("MPI SANITIZER PASS OPTIMIZATION BENCHMARK SUMMARY")
        print("="*60)
        
        summary = report['summary']
        print(f"Overall Performance Improvement: {summary['overall_speedup']:.2f}x")
        print(f"Memory Optimization: {summary['memory_improvement']:.1f}%")
        print(f"Tests Passed: {summary['tests_passed']}/{summary['total_tests']}")
        print(f"Success Rate: {summary['success_rate']:.1f}%")
        
        if summary['success_rate'] >= 80.0:
            print("\n✅ OPTIMIZATION BENCHMARKS PASSED")
            print("Task 18.1 requirements have been successfully met!")
        else:
            print("\n❌ OPTIMIZATION BENCHMARKS FAILED")
            print("Some optimization targets were not met.")
        
        print("="*60)
        
        return report
    
    def generate_summary(self) -> Dict[str, Any]:
        """Generate benchmark summary statistics."""
        summary = {
            'total_tests': 0,
            'tests_passed': 0,
            'overall_speedup': 1.0,
            'memory_improvement': 0.0,
            'success_rate': 0.0
        }
        
        speedups = []
        memory_improvements = []
        
        for test_name, test_results in self.results.items():
            if isinstance(test_results, dict):
                summary['total_tests'] += 1
                
                if test_results.get('passed', False):
                    summary['tests_passed'] += 1
                
                # Collect speedup data
                if 'speedup_ratio' in test_results:
                    speedups.append(test_results['speedup_ratio'])
                
                # Collect memory improvement data
                if 'memory_reduction' in test_results:
                    memory_improvements.append(test_results['memory_reduction'])
                elif 'memory_ratio' in test_results:
                    memory_improvements.append(1.0 - test_results['memory_ratio'])
        
        # Calculate overall metrics
        if speedups:
            summary['overall_speedup'] = sum(speedups) / len(speedups)
        
        if memory_improvements:
            summary['memory_improvement'] = (sum(memory_improvements) / len(memory_improvements)) * 100
        
        if summary['total_tests'] > 0:
            summary['success_rate'] = (summary['tests_passed'] / summary['total_tests']) * 100
        
        return summary


def main():
    parser = argparse.ArgumentParser(description='MPI Sanitizer Pass Optimization Benchmark')
    parser.add_argument('--llvm-build-dir', required=True,
                       help='Path to LLVM build directory')
    parser.add_argument('--test-dir', default='/tmp/mpi_sanitizer_benchmark',
                       help='Directory for test files')
    parser.add_argument('--output', '-o',
                       help='Output file for detailed results (JSON)')
    
    args = parser.parse_args()
    
    # Create test directory
    test_dir = Path(args.test_dir)
    test_dir.mkdir(parents=True, exist_ok=True)
    
    # Run benchmarks
    benchmark = OptimizationBenchmark(args.llvm_build_dir, args.test_dir)
    
    try:
        results = benchmark.run_all_benchmarks()
        report = benchmark.generate_report(args.output)
        
        # Return appropriate exit code
        if report['summary']['success_rate'] >= 80.0:
            return 0
        else:
            return 1
            
    except Exception as e:
        print(f"Error running benchmarks: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
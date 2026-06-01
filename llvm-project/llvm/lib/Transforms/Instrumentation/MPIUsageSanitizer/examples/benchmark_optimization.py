#!/usr/bin/env python3
"""
Benchmark optimization script for MPI Usage Sanitizer
Validates performance characteristics and generates optimization reports
"""

import os
import sys
import json
import time
import subprocess
import argparse
import statistics
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class PerformanceBenchmark:
    """Performance benchmarking and validation for MPI Usage Sanitizer"""
    
    def __init__(self, examples_dir: str, iterations: int = 5):
        self.examples_dir = Path(examples_dir)
        self.build_dir = self.examples_dir / "build"
        self.instrumented_dir = self.build_dir / "instrumented"
        self.uninstrumented_dir = self.build_dir / "uninstrumented"
        self.iterations = iterations
        self.results = {}
        
    def find_test_programs(self) -> List[str]:
        """Find all available test programs"""
        programs = []
        
        if not self.uninstrumented_dir.exists():
            print("Warning: Uninstrumented build directory not found")
            return programs
            
        for category_dir in self.uninstrumented_dir.iterdir():
            if category_dir.is_dir():
                for program in category_dir.iterdir():
                    if program.is_file() and os.access(program, os.X_OK):
                        relative_path = program.relative_to(self.uninstrumented_dir)
                        programs.append(str(relative_path))
        
        return sorted(programs)
    
    def run_program(self, program_path: Path, mpi_args: List[str], timeout: int = 30) -> Tuple[float, bool]:
        """Run a program and measure execution time"""
        try:
            start_time = time.time()
            result = subprocess.run(
                ["mpirun"] + mpi_args + [str(program_path)],
                capture_output=True,
                timeout=timeout,
                text=True
            )
            end_time = time.time()
            
            execution_time = end_time - start_time
            success = result.returncode == 0
            
            return execution_time, success
            
        except subprocess.TimeoutExpired:
            return 0.0, False
        except Exception as e:
            print(f"Error running {program_path}: {e}")
            return 0.0, False
    
    def benchmark_program(self, program: str, mpi_args: List[str] = None) -> Dict:
        """Benchmark a specific program"""
        if mpi_args is None:
            mpi_args = ["-np", "2"]
            
        uninstrumented_path = self.uninstrumented_dir / program
        instrumented_path = self.instrumented_dir / program
        
        result = {
            "program": program,
            "mpi_args": mpi_args,
            "uninstrumented_times": [],
            "instrumented_times": [],
            "uninstrumented_success": 0,
            "instrumented_success": 0,
            "overhead_percent": None,
            "overhead_seconds": None
        }
        
        print(f"Benchmarking {program} with {mpi_args}...")
        
        # Benchmark uninstrumented version
        if uninstrumented_path.exists():
            for i in range(self.iterations):
                exec_time, success = self.run_program(uninstrumented_path, mpi_args)
                if success:
                    result["uninstrumented_times"].append(exec_time)
                    result["uninstrumented_success"] += 1
        
        # Benchmark instrumented version
        if instrumented_path.exists():
            for i in range(self.iterations):
                exec_time, success = self.run_program(instrumented_path, mpi_args)
                if success:
                    result["instrumented_times"].append(exec_time)
                    result["instrumented_success"] += 1
        
        # Calculate statistics
        if result["uninstrumented_times"] and result["instrumented_times"]:
            uninstrumented_avg = statistics.mean(result["uninstrumented_times"])
            instrumented_avg = statistics.mean(result["instrumented_times"])
            
            result["uninstrumented_avg"] = uninstrumented_avg
            result["instrumented_avg"] = instrumented_avg
            result["overhead_seconds"] = instrumented_avg - uninstrumented_avg
            
            if uninstrumented_avg > 0:
                result["overhead_percent"] = (result["overhead_seconds"] / uninstrumented_avg) * 100
        
        return result
    
    def run_comprehensive_benchmark(self) -> Dict:
        """Run comprehensive performance benchmarks"""
        programs = self.find_test_programs()
        
        if not programs:
            print("No test programs found. Please run build_all.sh first.")
            return {}
        
        print(f"Found {len(programs)} test programs")
        print(f"Running {self.iterations} iterations per program")
        print()
        
        results = {}
        
        # Test different MPI configurations
        mpi_configs = [
            ["-np", "1"],
            ["-np", "2"],
            ["-np", "4"]
        ]
        
        for program in programs:
            program_results = {}
            
            for mpi_args in mpi_configs:
                config_key = f"np_{mpi_args[1]}"
                program_results[config_key] = self.benchmark_program(program, mpi_args)
            
            results[program] = program_results
        
        return results
    
    def validate_performance(self, results: Dict, max_overhead_percent: float = 50.0) -> bool:
        """Validate that performance overhead is acceptable"""
        validation_passed = True
        issues = []
        
        for program, configs in results.items():
            for config, result in configs.items():
                if result.get("overhead_percent") is not None:
                    overhead = result["overhead_percent"]
                    if overhead > max_overhead_percent:
                        validation_passed = False
                        issues.append(f"{program} ({config}): {overhead:.1f}% overhead")
        
        if not validation_passed:
            print("Performance validation FAILED:")
            for issue in issues:
                print(f"  - {issue}")
        else:
            print("Performance validation PASSED")
        
        return validation_passed
    
    def generate_report(self, results: Dict) -> str:
        """Generate a performance report"""
        report = []
        report.append("MPI Usage Sanitizer Performance Report")
        report.append("=" * 50)
        report.append("")
        
        total_programs = len(results)
        successful_programs = 0
        total_overhead = []
        
        for program, configs in results.items():
            report.append(f"Program: {program}")
            report.append("-" * 30)
            
            program_successful = False
            
            for config, result in configs.items():
                report.append(f"  Configuration: {config}")
                
                if result["uninstrumented_success"] > 0 and result["instrumented_success"] > 0:
                    uninstrumented_avg = result.get("uninstrumented_avg", 0)
                    instrumented_avg = result.get("instrumented_avg", 0)
                    overhead_percent = result.get("overhead_percent", 0)
                    
                    report.append(f"    Uninstrumented: {uninstrumented_avg:.3f}s")
                    report.append(f"    Instrumented:   {instrumented_avg:.3f}s")
                    report.append(f"    Overhead:       {overhead_percent:.1f}%")
                    
                    total_overhead.append(overhead_percent)
                    program_successful = True
                else:
                    report.append(f"    Status: FAILED")
                
                    report.append("")
            
            if program_successful:
                successful_programs += 1
        
        # Summary
        report.append("Summary")
        report.append("-" * 20)
        report.append(f"Total programs tested: {total_programs}")
        report.append(f"Successful programs: {successful_programs}")
        
        if total_overhead:
            avg_overhead = statistics.mean(total_overhead)
            max_overhead = max(total_overhead)
            min_overhead = min(total_overhead)
            
            report.append(f"Average overhead: {avg_overhead:.1f}%")
            report.append(f"Maximum overhead: {max_overhead:.1f}%")
            report.append(f"Minimum overhead: {min_overhead:.1f}%")
        
        return "\n".join(report)
    
    def save_results(self, results: Dict, output_file: str = "performance_results.json"):
        """Save results to JSON file"""
        output_path = self.examples_dir / output_file
        
        with open(output_path, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"Results saved to {output_path}")

def main():
    parser = argparse.ArgumentParser(description="MPI Usage Sanitizer Performance Benchmark")
    parser.add_argument("--examples-dir", default=".", help="Examples directory path")
    parser.add_argument("--iterations", type=int, default=5, help="Number of iterations per test")
    parser.add_argument("--validate-performance", action="store_true", help="Validate performance requirements")
    parser.add_argument("--max-overhead", type=float, default=50.0, help="Maximum acceptable overhead percentage")
    parser.add_argument("--output", default="performance_results.json", help="Output file for results")
    
    args = parser.parse_args()
    
    # Find examples directory
    examples_dir = Path(args.examples_dir).resolve()
    if not examples_dir.exists():
        print(f"Examples directory not found: {examples_dir}")
        sys.exit(1)
    
    # Create benchmark instance
    benchmark = PerformanceBenchmark(str(examples_dir), args.iterations)
    
    # Run benchmarks
    print("Starting MPI Usage Sanitizer performance benchmarks...")
    print()
    
    results = benchmark.run_comprehensive_benchmark()
    
    if not results:
        print("No benchmark results generated")
        sys.exit(1)
    
    # Generate and display report
    report = benchmark.generate_report(results)
    print()
    print(report)
    
    # Save results
    benchmark.save_results(results, args.output)
    
    # Validate performance if requested
    if args.validate_performance:
        print()
        validation_passed = benchmark.validate_performance(results, args.max_overhead)
        sys.exit(0 if validation_passed else 1)

if __name__ == "__main__":
    main()
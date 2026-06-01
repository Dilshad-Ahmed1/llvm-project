# -*- Python -*-

# Configuration file for the 'lit' test runner for MPI Usage Sanitizer Pass

import os
import platform
import re
import subprocess
import tempfile

import lit.formats
import lit.util

from lit.llvm import llvm_config
from lit.llvm.subst import ToolSubst
from lit.llvm.subst import FindTool

# name: The name of this test suite.
config.name = 'MPI-Usage-Sanitizer'

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = ['.ll', '.c', '.cpp', '.f90']

# excludes: A list of directories to exclude from the testsuite.
config.excludes = ['Inputs', 'CMakeLists.txt', 'README.txt', 'LICENSE.txt']

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.llvm_obj_root, 'test', 'Transforms', 'Instrumentation', 'MPIUsageSanitizer')

# Tweak the PATH to include the tools dir.
llvm_config.with_environment('PATH', config.llvm_tools_dir, append_path=True)

# Propagate some variables from the host environment.
llvm_config.with_system_environment(['HOME', 'INCLUDE', 'LIB', 'TMP', 'TEMP'])

# Configure tool substitutions
llvm_config.use_default_substitutions()

# Add MPI-specific tool substitutions
tool_dirs = [config.llvm_tools_dir]

tools = [
    'opt',
    'FileCheck',
    'count',
    'not',
    'llvm-dis',
    'llvm-as',
    'llc',
    'clang',
    'clang++',
]

# Add MPI compiler wrappers if available
mpi_tools = ['mpicc', 'mpicxx', 'mpifort']
for tool in mpi_tools:
    if lit.util.which(tool):
        tools.append(tool)

llvm_config.add_tool_substitutions(tools, tool_dirs)

# MPI Usage Sanitizer specific substitutions
config.substitutions.append(('%mpi-sanitizer-pass', 
    '-load-pass-plugin ' + os.path.join(config.llvm_shlib_dir, 'LLVMMPIUsageSanitizer' + config.llvm_shlib_ext) + 
    ' -passes=mpi-sanitizer'))

config.substitutions.append(('%mpi-sanitizer-legacy', 
    '-load ' + os.path.join(config.llvm_shlib_dir, 'LLVMMPIUsageSanitizer' + config.llvm_shlib_ext) + 
    ' -mpi-sanitizer'))

# Configuration-specific substitutions
if config.llvm_mpi_sanitizer_profiling:
    config.substitutions.append(('%mpi-sanitizer-profiling', '-mpi-sanitizer-enable-profiling'))
else:
    config.substitutions.append(('%mpi-sanitizer-profiling', ''))

if config.llvm_mpi_sanitizer_optimization:
    config.substitutions.append(('%mpi-sanitizer-optimization', '-mpi-sanitizer-enable-optimization'))
else:
    config.substitutions.append(('%mpi-sanitizer-optimization', ''))

# Test categories
config.substitutions.append(('%basic-test', 
    '%mpi-sanitizer-pass -mpi-sanitizer-mode=standard'))
config.substitutions.append(('%optimization-test', 
    '%mpi-sanitizer-pass -mpi-sanitizer-mode=selective %mpi-sanitizer-optimization'))
config.substitutions.append(('%profiling-test', 
    '%mpi-sanitizer-pass %mpi-sanitizer-profiling'))

# Platform-specific configurations
if platform.system() == 'Windows':
    config.substitutions.append(('%shared-lib-ext', '.dll'))
elif platform.system() == 'Darwin':
    config.substitutions.append(('%shared-lib-ext', '.dylib'))
else:
    config.substitutions.append(('%shared-lib-ext', '.so'))

# MPI availability check
def check_mpi_available():
    """Check if MPI is available on the system."""
    try:
        subprocess.check_output(['mpicc', '--version'], stderr=subprocess.STDOUT)
        return True
    except (subprocess.CalledProcessError, OSError):
        return False

if check_mpi_available():
    config.available_features.add('mpi')
    config.substitutions.append(('%mpi-compile', 'mpicc'))
    config.substitutions.append(('%mpi-compile-cxx', 'mpicxx'))
    if lit.util.which('mpifort'):
        config.substitutions.append(('%mpi-compile-fortran', 'mpifort'))
        config.available_features.add('mpi-fortran')
else:
    # Fallback to regular compilers with MPI flags
    config.substitutions.append(('%mpi-compile', 'clang -lmpi'))
    config.substitutions.append(('%mpi-compile-cxx', 'clang++ -lmpi'))

# Property-based testing support
if hasattr(config, 'llvm_enable_property_based_testing') and config.llvm_enable_property_based_testing:
    config.available_features.add('property-based-testing')

# Performance testing support
if hasattr(config, 'llvm_mpi_sanitizer_enable_performance_tests') and config.llvm_mpi_sanitizer_enable_performance_tests:
    config.available_features.add('performance-testing')

# Debug build detection
if hasattr(config, 'llvm_build_mode') and config.llvm_build_mode == 'Debug':
    config.available_features.add('debug-build')

# Optimization level detection
if hasattr(config, 'llvm_optimization_level'):
    if config.llvm_optimization_level in ['O2', 'O3']:
        config.available_features.add('optimized-build')

# Thread safety support
if hasattr(config, 'llvm_enable_threads') and config.llvm_enable_threads:
    config.available_features.add('thread-support')

# Memory sanitizer support for testing
if hasattr(config, 'llvm_use_sanitizer'):
    if 'Memory' in config.llvm_use_sanitizer:
        config.available_features.add('msan')
    if 'Address' in config.llvm_use_sanitizer:
        config.available_features.add('asan')

# Test timeout (in seconds)
try:
    config.test_timeout = int(os.environ.get('LIT_TEST_TIMEOUT', '300'))
except ValueError:
    config.test_timeout = 300

# Parallelism
try:
    config.parallelism_group = os.environ.get('LIT_PARALLELISM_GROUP', 'mpi-sanitizer')
except:
    pass

# Custom test result codes
config.pipefail = False  # Don't fail on intermediate pipeline failures

# Add custom lit features based on configuration
def add_feature_if_available(feature_name, check_func):
    """Add a feature if the check function returns True."""
    try:
        if check_func():
            config.available_features.add(feature_name)
    except:
        pass

# Check for specific LLVM features
add_feature_if_available('llvm-new-pass-manager', 
    lambda: hasattr(config, 'llvm_use_new_pass_manager') and config.llvm_use_new_pass_manager)

add_feature_if_available('llvm-legacy-pass-manager',
    lambda: not (hasattr(config, 'llvm_use_new_pass_manager') and config.llvm_use_new_pass_manager))

# Check for specific compiler features
def check_compiler_feature(feature, code):
    """Check if compiler supports a specific feature."""
    try:
        with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
            f.write(code)
            f.flush()
            subprocess.check_output([config.clang, '-fsyntax-only', f.name], 
                                  stderr=subprocess.STDOUT)
            os.unlink(f.name)
            return True
    except:
        try:
            os.unlink(f.name)
        except:
            pass
        return False

# Check for C++17 support
add_feature_if_available('cxx17', 
    lambda: check_compiler_feature('cxx17', 'int main() { auto [a, b] = std::make_pair(1, 2); }'))

# Check for OpenMP support
add_feature_if_available('openmp',
    lambda: check_compiler_feature('openmp', '#include <omp.h>\nint main() { return omp_get_num_threads(); }'))

# Verbose output for debugging
if lit_config.debug:
    config.environment['LIT_DEBUG'] = '1'
    lit_config.note('MPI Usage Sanitizer test configuration:')
    lit_config.note('  Test source root: %s' % config.test_source_root)
    lit_config.note('  Test exec root: %s' % config.test_exec_root)
    lit_config.note('  Available features: %s' % sorted(config.available_features))
    lit_config.note('  Tool substitutions: %s' % [s[0] for s in config.substitutions if s[0].startswith('%')])
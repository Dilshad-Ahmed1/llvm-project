#!/bin/bash

# MPI Usage Sanitizer Development Environment Setup
# This script sets up a local development environment for the MPI Usage Sanitizer

set -e

# Configuration
LLVM_VERSION="17"
BUILD_TYPE="Release"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[SETUP]${NC} $1"
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

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
        if command -v apt-get &> /dev/null; then
            DISTRO="ubuntu"
        elif command -v yum &> /dev/null; then
            DISTRO="rhel"
        else
            DISTRO="unknown"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        DISTRO="macos"
    else
        OS="unknown"
        DISTRO="unknown"
    fi
    
    print_status "Detected OS: $OS ($DISTRO)"
}

# Install system dependencies
install_dependencies() {
    print_status "Installing system dependencies..."
    
    case "$DISTRO" in
        ubuntu)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                ninja-build \
                git \
                python3 \
                python3-pip \
                libxml2-dev \
                libzstd-dev \
                libedit-dev \
                swig \
                python3-dev \
                libncurses5-dev \
                liblzma-dev \
                libz-dev \
                pkg-config \
                clang \
                clang-tools \
                lldb \
                lld
            
            # Install MPI
            sudo apt-get install -y \
                openmpi-bin \
                openmpi-common \
                libopenmpi-dev \
                mpich \
                libmpich-dev
            
            # Install development tools
            sudo apt-get install -y \
                valgrind \
                gdb \
                strace \
                htop \
                tree
            ;;
            
        rhel)
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                cmake \
                ninja-build \
                git \
                python3 \
                python3-pip \
                libxml2-devel \
                libzstd-devel \
                libedit-devel \
                swig \
                python3-devel \
                ncurses-devel \
                xz-devel \
                zlib-devel \
                pkgconfig \
                clang \
                clang-tools-extra \
                lldb
            
            # Install MPI
            sudo yum install -y \
                openmpi \
                openmpi-devel \
                mpich \
                mpich-devel
            ;;
            
        macos)
            # Check if Homebrew is installed
            if ! command -v brew &> /dev/null; then
                print_error "Homebrew is required but not installed. Please install it first:"
                print_error "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
                exit 1
            fi
            
            brew update
            brew install \
                cmake \
                ninja \
                python@3.11 \
                libxml2 \
                zstd \
                libedit \
                swig \
                pkg-config \
                llvm \
                clang-format
            
            # Install MPI
            brew install \
                open-mpi \
                mpich
            ;;
            
        *)
            print_error "Unsupported distribution: $DISTRO"
            print_error "Please install dependencies manually:"
            print_error "  - CMake 3.16+"
            print_error "  - Ninja build system"
            print_error "  - Python 3.8+"
            print_error "  - MPI implementation (OpenMPI or MPICH)"
            print_error "  - C++17 compatible compiler"
            exit 1
            ;;
    esac
    
    print_success "System dependencies installed"
}

# Setup Python environment
setup_python_env() {
    print_status "Setting up Python environment..."
    
    # Install Python packages for development and testing
    pip3 install --user \
        lit \
        filecheck \
        pytest \
        numpy \
        matplotlib \
        scipy \
        pyyaml \
        jinja2 \
        sphinx \
        breathe \
        doxygen
    
    print_success "Python environment configured"
}

# Configure LLVM build
configure_llvm() {
    print_status "Configuring LLVM build..."
    
    cd "$PROJECT_ROOT"
    
    # Create build directory
    mkdir -p llvm-build
    cd llvm-build
    
    # Configure with CMake
    cmake -G Ninja ../llvm \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX="$PROJECT_ROOT/llvm-install" \
        -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
        -DLLVM_TARGETS_TO_BUILD="X86;AArch64;ARM" \
        -DLLVM_ENABLE_ASSERTIONS=ON \
        -DLLVM_ENABLE_RTTI=ON \
        -DLLVM_ENABLE_EH=ON \
        -DLLVM_INCLUDE_TESTS=ON \
        -DLLVM_INCLUDE_EXAMPLES=ON \
        -DLLVM_ENABLE_MPI_SANITIZER=ON \
        -DLLVM_MPI_SANITIZER_ENABLE_PROFILING=ON \
        -DLLVM_MPI_SANITIZER_ENABLE_OPTIMIZATION=ON \
        -DLLVM_ENABLE_PROPERTY_BASED_TESTING=ON \
        -DLLVM_PARALLEL_LINK_JOBS=2 \
        -DLLVM_PARALLEL_COMPILE_JOBS=$PARALLEL_JOBS \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++
    
    print_success "LLVM build configured"
}

# Build LLVM with MPI Sanitizer
build_llvm() {
    print_status "Building LLVM with MPI Sanitizer..."
    print_status "This may take 30-60 minutes depending on your system..."
    
    cd "$PROJECT_ROOT/llvm-build"
    
    # Build core components first
    print_status "Building core LLVM components..."
    ninja llvm-config opt FileCheck count not
    
    # Build MPI Sanitizer
    print_status "Building MPI Sanitizer components..."
    ninja LLVMMPIUsageSanitizerComponents
    
    # Build Clang
    print_status "Building Clang..."
    ninja clang
    
    # Build testing tools
    print_status "Building testing tools..."
    ninja llvm-lit
    
    print_success "LLVM build completed"
}

# Install LLVM
install_llvm() {
    print_status "Installing LLVM..."
    
    cd "$PROJECT_ROOT/llvm-build"
    ninja install
    
    print_success "LLVM installed to $PROJECT_ROOT/llvm-install"
}

# Verify installation
verify_installation() {
    print_status "Verifying installation..."
    
    cd "$PROJECT_ROOT"
    
    # Add to PATH temporarily
    export PATH="$PROJECT_ROOT/llvm-install/bin:$PATH"
    
    # Check if opt can load the MPI sanitizer pass
    if opt -load-pass-plugin=llvm-install/lib/LLVMMPIUsageSanitizerComponents.so -passes=help | grep -q mpi-sanitizer; then
        print_success "MPI Sanitizer pass is available in opt"
    else
        print_error "MPI Sanitizer pass not found in opt"
        return 1
    fi
    
    # Test basic compilation
    echo 'int main(){return 0;}' > test.c
    if clang -fpass-plugin=llvm-install/lib/LLVMMPIUsageSanitizerComponents.so test.c -o test; then
        print_success "Clang can load MPI Sanitizer plugin"
        rm -f test test.c
    else
        print_error "Clang cannot load MPI Sanitizer plugin"
        rm -f test test.c
        return 1
    fi
    
    # Check MPI availability
    if command -v mpicc &> /dev/null; then
        print_success "MPI compiler (mpicc) is available"
    else
        print_warning "MPI compiler (mpicc) not found in PATH"
    fi
    
    if command -v mpirun &> /dev/null; then
        print_success "MPI runtime (mpirun) is available"
    else
        print_warning "MPI runtime (mpirun) not found in PATH"
    fi
    
    print_success "Installation verification completed"
}

# Run tests
run_tests() {
    print_status "Running tests..."
    
    cd "$PROJECT_ROOT/llvm-build"
    
    # Run unit tests
    if ninja check-mpi-sanitizer; then
        print_success "Unit tests passed"
    else
        print_warning "Some unit tests failed (this may be expected in development)"
    fi
    
    # Test examples if available
    if [ -d "$PROJECT_ROOT/llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples" ]; then
        print_status "Testing example programs..."
        
        cd "$PROJECT_ROOT/llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples"
        
        # Set environment
        export LLVM_DIR="$PROJECT_ROOT/llvm-install"
        export PATH="$LLVM_DIR/bin:$PATH"
        
        # Build examples
        if [ -f "build_scripts/build_all.sh" ]; then
            cd build_scripts
            if ./build_all.sh; then
                print_success "Example programs built successfully"
                
                # Test basic functionality
                cd ../build
                if timeout 30s ./run_instrumented.sh basic/hello_world -np 2 &>/dev/null; then
                    print_success "Example programs run successfully"
                else
                    print_warning "Example programs failed to run (MPI may not be properly configured)"
                fi
            else
                print_warning "Failed to build example programs"
            fi
        fi
    fi
}

# Create development scripts
create_dev_scripts() {
    print_status "Creating development scripts..."
    
    mkdir -p "$PROJECT_ROOT/scripts"
    
    # Create environment setup script
    cat > "$PROJECT_ROOT/scripts/env.sh" << EOF
#!/bin/bash
# MPI Usage Sanitizer development environment

export LLVM_DIR="$PROJECT_ROOT/llvm-install"
export PATH="\$LLVM_DIR/bin:\$PATH"
export LD_LIBRARY_PATH="\$LLVM_DIR/lib:\$LD_LIBRARY_PATH"

# MPI Sanitizer specific
export MPI_SANITIZER_PLUGIN="\$LLVM_DIR/lib/LLVMMPIUsageSanitizerComponents.so"

# Development aliases
alias mpi-opt="opt -load-pass-plugin=\$MPI_SANITIZER_PLUGIN"
alias mpi-clang="clang -fpass-plugin=\$MPI_SANITIZER_PLUGIN"
alias mpi-clang++="clang++ -fpass-plugin=\$MPI_SANITIZER_PLUGIN"

echo "MPI Usage Sanitizer development environment loaded"
echo "LLVM installed at: \$LLVM_DIR"
echo ""
echo "Usage:"
echo "  mpi-clang -mllvm -passes=mpi-sanitizer program.c -o program -lmpi"
echo "  mpi-opt -passes=mpi-sanitizer program.ll -o program_instrumented.ll"
EOF
    
    chmod +x "$PROJECT_ROOT/scripts/env.sh"
    
    # Create quick test script
    cat > "$PROJECT_ROOT/scripts/quick-test.sh" << EOF
#!/bin/bash
# Quick test of MPI Usage Sanitizer

set -e

# Load environment
source "\$(dirname "\$0")/env.sh"

# Create test program
cat > /tmp/mpi_test.c << 'EOC'
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from rank %d of %d\\n", rank, size);
    
    MPI_Finalize();
    return 0;
}
EOC

echo "Compiling test program with MPI Sanitizer..."
mpi-clang -mllvm -passes=mpi-sanitizer \\
          -mllvm -mpi-sanitizer-level=standard \\
          /tmp/mpi_test.c -o /tmp/mpi_test -lmpi

echo "Running test program..."
mpirun -np 2 /tmp/mpi_test

echo "Test completed successfully!"
rm -f /tmp/mpi_test /tmp/mpi_test.c
EOF
    
    chmod +x "$PROJECT_ROOT/scripts/quick-test.sh"
    
    print_success "Development scripts created"
}

# Main setup function
main() {
    print_status "Starting MPI Usage Sanitizer development environment setup..."
    
    detect_os
    
    # Check if we should skip dependency installation
    if [[ "$1" != "--skip-deps" ]]; then
        install_dependencies
        setup_python_env
    else
        print_warning "Skipping dependency installation"
    fi
    
    configure_llvm
    build_llvm
    install_llvm
    verify_installation
    
    if [[ "$1" != "--skip-tests" ]]; then
        run_tests
    else
        print_warning "Skipping tests"
    fi
    
    create_dev_scripts
    
    print_success "Setup completed successfully!"
    print_status ""
    print_status "To use the development environment:"
    print_status "  source scripts/env.sh"
    print_status ""
    print_status "To run a quick test:"
    print_status "  ./scripts/quick-test.sh"
    print_status ""
    print_status "Example usage:"
    print_status "  mpi-clang -mllvm -passes=mpi-sanitizer program.c -o program -lmpi"
    print_status "  mpirun -np 4 ./program"
}

# Handle command line arguments
case "$1" in
    --help|-h)
        echo "MPI Usage Sanitizer Development Environment Setup"
        echo ""
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --skip-deps    Skip system dependency installation"
        echo "  --skip-tests   Skip running tests after build"
        echo "  --help, -h     Show this help message"
        echo ""
        echo "This script will:"
        echo "  1. Install system dependencies (CMake, Ninja, MPI, etc.)"
        echo "  2. Configure and build LLVM with MPI Sanitizer"
        echo "  3. Install LLVM to llvm-install/"
        echo "  4. Verify the installation"
        echo "  5. Run basic tests"
        echo "  6. Create development scripts"
        echo ""
        echo "Requirements:"
        echo "  - Ubuntu 20.04+, RHEL 8+, or macOS 12+"
        echo "  - At least 8GB RAM and 20GB free disk space"
        echo "  - Internet connection for downloading dependencies"
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac
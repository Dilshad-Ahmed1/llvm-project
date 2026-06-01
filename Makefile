# Makefile for MPI Usage Sanitizer LLVM Pass
# Provides convenient targets for building, testing, and development

# Configuration
LLVM_VERSION ?= 17
BUILD_TYPE ?= Release
PARALLEL_JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
PROJECT_ROOT := $(shell pwd)
LLVM_BUILD_DIR := $(PROJECT_ROOT)/llvm-build
LLVM_INSTALL_DIR := $(PROJECT_ROOT)/llvm-install

# Docker configuration
DOCKER_DEV_IMAGE := mpi-sanitizer-dev
DOCKER_CI_IMAGE := mpi-sanitizer-ci
DOCKER_DOCS_IMAGE := mpi-sanitizer-docs

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

# Default target
.PHONY: help
help: ## Show this help message
	@echo "MPI Usage Sanitizer LLVM Pass - Development Makefile"
	@echo "===================================================="
	@echo ""
	@echo "Available targets:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  $(BLUE)%-20s$(NC) %s\n", $$1, $$2}'
	@echo ""
	@echo "Configuration:"
	@echo "  LLVM_VERSION: $(LLVM_VERSION)"
	@echo "  BUILD_TYPE: $(BUILD_TYPE)"
	@echo "  PARALLEL_JOBS: $(PARALLEL_JOBS)"
	@echo "  PROJECT_ROOT: $(PROJECT_ROOT)"

# Build targets
.PHONY: configure
configure: ## Configure LLVM build with MPI Sanitizer
	@echo "$(BLUE)[BUILD]$(NC) Configuring LLVM build..."
	@mkdir -p $(LLVM_BUILD_DIR)
	@cd $(LLVM_BUILD_DIR) && cmake -G Ninja ../llvm \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(LLVM_INSTALL_DIR) \
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
		-DLLVM_PARALLEL_COMPILE_JOBS=$(PARALLEL_JOBS)
	@echo "$(GREEN)[SUCCESS]$(NC) LLVM build configured"

.PHONY: build-core
build-core: configure ## Build core LLVM components
	@echo "$(BLUE)[BUILD]$(NC) Building core LLVM components..."
	@cd $(LLVM_BUILD_DIR) && ninja llvm-config opt FileCheck count not
	@echo "$(GREEN)[SUCCESS]$(NC) Core LLVM components built"

.PHONY: build-sanitizer
build-sanitizer: build-core ## Build MPI Sanitizer components
	@echo "$(BLUE)[BUILD]$(NC) Building MPI Sanitizer components..."
	@cd $(LLVM_BUILD_DIR) && ninja LLVMMPIUsageSanitizerComponents
	@echo "$(GREEN)[SUCCESS]$(NC) MPI Sanitizer components built"

.PHONY: build-clang
build-clang: build-sanitizer ## Build Clang
	@echo "$(BLUE)[BUILD]$(NC) Building Clang..."
	@cd $(LLVM_BUILD_DIR) && ninja clang
	@echo "$(GREEN)[SUCCESS]$(NC) Clang built"

.PHONY: build-tests
build-tests: build-clang ## Build testing tools
	@echo "$(BLUE)[BUILD]$(NC) Building testing tools..."
	@cd $(LLVM_BUILD_DIR) && ninja llvm-lit
	@echo "$(GREEN)[SUCCESS]$(NC) Testing tools built"

.PHONY: build
build: build-tests ## Build everything (core + sanitizer + clang + tests)
	@echo "$(GREEN)[SUCCESS]$(NC) Full build completed"

.PHONY: install
install: build ## Install LLVM with MPI Sanitizer
	@echo "$(BLUE)[INSTALL]$(NC) Installing LLVM..."
	@cd $(LLVM_BUILD_DIR) && ninja install
	@echo "$(GREEN)[SUCCESS]$(NC) LLVM installed to $(LLVM_INSTALL_DIR)"

.PHONY: quick-build
quick-build: ## Quick incremental build (sanitizer only)
	@echo "$(BLUE)[BUILD]$(NC) Quick incremental build..."
	@cd $(LLVM_BUILD_DIR) && ninja LLVMMPIUsageSanitizerComponents
	@echo "$(GREEN)[SUCCESS]$(NC) Quick build completed"

# Testing targets
.PHONY: test
test: install ## Run all tests
	@echo "$(BLUE)[TEST]$(NC) Running all tests..."
	@cd $(LLVM_BUILD_DIR) && ninja check-mpi-sanitizer
	@$(MAKE) test-examples
	@echo "$(GREEN)[SUCCESS]$(NC) All tests passed"

.PHONY: test-unit
test-unit: install ## Run unit tests only
	@echo "$(BLUE)[TEST]$(NC) Running unit tests..."
	@cd $(LLVM_BUILD_DIR) && ninja check-mpi-sanitizer
	@echo "$(GREEN)[SUCCESS]$(NC) Unit tests passed"

.PHONY: test-examples
test-examples: install ## Test example programs
	@echo "$(BLUE)[TEST]$(NC) Testing example programs..."
	@if [ -d "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples" ]; then \
		cd llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples && \
		export LLVM_DIR=$(LLVM_INSTALL_DIR) && \
		export PATH=$(LLVM_INSTALL_DIR)/bin:$$PATH && \
		cd build_scripts && ./build_all.sh && \
		cd ../build && \
		timeout 30s ./run_instrumented.sh basic/hello_world -np 2 && \
		timeout 30s ./run_instrumented.sh basic/send_recv -np 2; \
	else \
		echo "$(YELLOW)[WARNING]$(NC) Example programs not found"; \
	fi
	@echo "$(GREEN)[SUCCESS]$(NC) Example tests completed"

.PHONY: test-performance
test-performance: install ## Run performance tests
	@echo "$(BLUE)[TEST]$(NC) Running performance tests..."
	@if [ -d "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples" ]; then \
		cd llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/examples/build && \
		./performance_comparison.sh basic/hello_world -np 2; \
	fi
	@echo "$(GREEN)[SUCCESS]$(NC) Performance tests completed"

# Development targets
.PHONY: dev-setup
dev-setup: ## Set up development environment
	@echo "$(BLUE)[SETUP]$(NC) Setting up development environment..."
	@chmod +x scripts/setup-dev-environment.sh
	@./scripts/setup-dev-environment.sh
	@echo "$(GREEN)[SUCCESS]$(NC) Development environment ready"

.PHONY: format
format: ## Format code using clang-format
	@echo "$(BLUE)[FORMAT]$(NC) Formatting code..."
	@find llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer \
		-name "*.cpp" -o -name "*.h" | \
		xargs clang-format -i
	@echo "$(GREEN)[SUCCESS]$(NC) Code formatted"

.PHONY: lint
lint: ## Run static analysis with clang-tidy
	@echo "$(BLUE)[LINT]$(NC) Running static analysis..."
	@find llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer \
		-name "*.cpp" | \
		xargs clang-tidy \
			--checks='-*,llvm-*,readability-*,performance-*,modernize-*' || \
		echo "$(YELLOW)[WARNING]$(NC) Static analysis found issues"

.PHONY: verify
verify: install ## Verify installation
	@echo "$(BLUE)[VERIFY]$(NC) Verifying installation..."
	@export PATH=$(LLVM_INSTALL_DIR)/bin:$$PATH && \
		if opt -load-pass-plugin=$(LLVM_INSTALL_DIR)/lib/LLVMMPIUsageSanitizerComponents.so -passes=help | grep -q mpi-sanitizer; then \
			echo "$(GREEN)[SUCCESS]$(NC) MPI Sanitizer pass is available"; \
		else \
			echo "$(RED)[ERROR]$(NC) MPI Sanitizer pass not found"; \
			exit 1; \
		fi
	@echo 'int main(){return 0;}' > /tmp/test.c && \
		export PATH=$(LLVM_INSTALL_DIR)/bin:$$PATH && \
		clang -fpass-plugin=$(LLVM_INSTALL_DIR)/lib/LLVMMPIUsageSanitizerComponents.so /tmp/test.c -o /tmp/test && \
		rm -f /tmp/test /tmp/test.c && \
		echo "$(GREEN)[SUCCESS]$(NC) Clang can load MPI Sanitizer plugin"

# Docker targets
.PHONY: docker-build-dev
docker-build-dev: ## Build development Docker image
	@echo "$(BLUE)[DOCKER]$(NC) Building development Docker image..."
	@docker build -f Dockerfile.dev -t $(DOCKER_DEV_IMAGE) .
	@echo "$(GREEN)[SUCCESS]$(NC) Development Docker image built"

.PHONY: docker-run-dev
docker-run-dev: docker-build-dev ## Run development environment in Docker
	@echo "$(BLUE)[DOCKER]$(NC) Starting development environment..."
	@docker-compose up -d mpi-sanitizer-dev
	@echo "$(GREEN)[SUCCESS]$(NC) Development environment started"
	@echo "$(BLUE)[INFO]$(NC) Connect with: docker exec -it mpi-sanitizer-dev bash"

.PHONY: docker-build-ci
docker-build-ci: ## Build CI Docker images
	@echo "$(BLUE)[DOCKER]$(NC) Building CI Docker images..."
	@docker build -f Dockerfile.ci -t $(DOCKER_CI_IMAGE)-ubuntu20 --build-arg BASE_IMAGE=ubuntu:20.04 .
	@docker build -f Dockerfile.ci -t $(DOCKER_CI_IMAGE)-ubuntu22 --build-arg BASE_IMAGE=ubuntu:22.04 .
	@echo "$(GREEN)[SUCCESS]$(NC) CI Docker images built"

.PHONY: docker-test-ci
docker-test-ci: docker-build-ci ## Run CI tests in Docker
	@echo "$(BLUE)[DOCKER]$(NC) Running CI tests..."
	@docker run --rm -v $(PWD):/workspace $(DOCKER_CI_IMAGE)-ubuntu22 /home/ci-user/ci-build.sh
	@docker run --rm -v $(PWD):/workspace $(DOCKER_CI_IMAGE)-ubuntu22 /home/ci-user/ci-test.sh
	@echo "$(GREEN)[SUCCESS]$(NC) CI tests completed"

.PHONY: docker-docs
docker-docs: ## Build and serve documentation
	@echo "$(BLUE)[DOCKER]$(NC) Building documentation..."
	@docker-compose up -d docs
	@echo "$(GREEN)[SUCCESS]$(NC) Documentation available at http://localhost:8080"

.PHONY: docker-clean
docker-clean: ## Clean up Docker resources
	@echo "$(BLUE)[DOCKER]$(NC) Cleaning up Docker resources..."
	@docker-compose down -v
	@docker system prune -f
	@echo "$(GREEN)[SUCCESS]$(NC) Docker resources cleaned"

# Documentation targets
.PHONY: docs
docs: ## Generate documentation
	@echo "$(BLUE)[DOCS]$(NC) Generating documentation..."
	@mkdir -p docs/output
	@if [ -f "llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer/docs/Doxyfile" ]; then \
		cd llvm/lib/Transforms/Instrumentation/MPIUsageSanitizer && \
		doxygen docs/Doxyfile; \
	fi
	@echo "$(GREEN)[SUCCESS]$(NC) Documentation generated"

.PHONY: serve-docs
serve-docs: docs ## Serve documentation locally
	@echo "$(BLUE)[DOCS]$(NC) Serving documentation at http://localhost:8000"
	@cd docs/output && python3 -m http.server 8000

# Cleanup targets
.PHONY: clean
clean: ## Clean build artifacts
	@echo "$(BLUE)[CLEAN]$(NC) Cleaning build artifacts..."
	@rm -rf $(LLVM_BUILD_DIR)
	@echo "$(GREEN)[SUCCESS]$(NC) Build artifacts cleaned"

.PHONY: clean-install
clean-install: ## Clean installation
	@echo "$(BLUE)[CLEAN]$(NC) Cleaning installation..."
	@rm -rf $(LLVM_INSTALL_DIR)
	@echo "$(GREEN)[SUCCESS]$(NC) Installation cleaned"

.PHONY: clean-all
clean-all: clean clean-install ## Clean everything
	@echo "$(BLUE)[CLEAN]$(NC) Cleaning everything..."
	@rm -rf docs/output
	@echo "$(GREEN)[SUCCESS]$(NC) Everything cleaned"

# Utility targets
.PHONY: env
env: ## Show environment information
	@echo "Environment Information:"
	@echo "======================="
	@echo "LLVM_VERSION: $(LLVM_VERSION)"
	@echo "BUILD_TYPE: $(BUILD_TYPE)"
	@echo "PARALLEL_JOBS: $(PARALLEL_JOBS)"
	@echo "PROJECT_ROOT: $(PROJECT_ROOT)"
	@echo "LLVM_BUILD_DIR: $(LLVM_BUILD_DIR)"
	@echo "LLVM_INSTALL_DIR: $(LLVM_INSTALL_DIR)"
	@echo ""
	@echo "System Information:"
	@echo "=================="
	@echo "OS: $$(uname -s)"
	@echo "Architecture: $$(uname -m)"
	@echo "CPU Cores: $(PARALLEL_JOBS)"
	@if command -v cmake >/dev/null 2>&1; then echo "CMake: $$(cmake --version | head -1)"; fi
	@if command -v ninja >/dev/null 2>&1; then echo "Ninja: $$(ninja --version)"; fi
	@if command -v clang >/dev/null 2>&1; then echo "Clang: $$(clang --version | head -1)"; fi
	@if command -v mpicc >/dev/null 2>&1; then echo "MPI: $$(mpicc --version | head -1)"; fi

.PHONY: deps
deps: ## Check and install dependencies
	@echo "$(BLUE)[DEPS]$(NC) Checking dependencies..."
	@chmod +x scripts/setup-dev-environment.sh
	@./scripts/setup-dev-environment.sh --skip-tests

.PHONY: quick-test
quick-test: install ## Quick functionality test
	@echo "$(BLUE)[TEST]$(NC) Running quick functionality test..."
	@export PATH=$(LLVM_INSTALL_DIR)/bin:$$PATH && \
		echo '#include <mpi.h>' > /tmp/quick_test.c && \
		echo 'int main(int argc, char** argv) {' >> /tmp/quick_test.c && \
		echo '  MPI_Init(&argc, &argv);' >> /tmp/quick_test.c && \
		echo '  MPI_Finalize();' >> /tmp/quick_test.c && \
		echo '  return 0;' >> /tmp/quick_test.c && \
		echo '}' >> /tmp/quick_test.c && \
		clang -fpass-plugin=$(LLVM_INSTALL_DIR)/lib/LLVMMPIUsageSanitizerComponents.so \
			-mllvm -passes=mpi-sanitizer \
			/tmp/quick_test.c -o /tmp/quick_test -lmpi && \
		timeout 10s mpirun -np 1 /tmp/quick_test && \
		rm -f /tmp/quick_test /tmp/quick_test.c
	@echo "$(GREEN)[SUCCESS]$(NC) Quick test passed"

# CI targets
.PHONY: ci
ci: clean build test ## Full CI pipeline (clean + build + test)
	@echo "$(GREEN)[SUCCESS]$(NC) CI pipeline completed successfully"

.PHONY: ci-quick
ci-quick: quick-build test-unit ## Quick CI pipeline (incremental build + unit tests)
	@echo "$(GREEN)[SUCCESS]$(NC) Quick CI pipeline completed successfully"

# Development workflow targets
.PHONY: dev
dev: quick-build verify quick-test ## Development workflow (quick build + verify + test)
	@echo "$(GREEN)[SUCCESS]$(NC) Development workflow completed"

.PHONY: full
full: clean build test docs ## Full build and test everything
	@echo "$(GREEN)[SUCCESS]$(NC) Full build and test completed"

# Make sure intermediate files are not deleted
.PRECIOUS: $(LLVM_BUILD_DIR)/CMakeCache.txt
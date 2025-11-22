# Makefile - Convenience wrapper for CMake builds
.PHONY: all clean build test install docker run-sequential run-parallel benchmark help

# Default target
all: build

# Configuration
BUILD_DIR ?= build
BUILD_TYPE ?= Release
NUM_JOBS ?= $(shell nproc)
INSTALL_PREFIX ?= /usr/local

help:
	@echo "Image Processing Pipeline - Build Targets"
	@echo "=========================================="
	@echo "  make build          - Build the project"
	@echo "  make test           - Run tests"
	@echo "  make clean          - Clean build files"
	@echo "  make install        - Install binaries"
	@echo "  make docker         - Build Docker image"
	@echo "  make benchmark      - Run performance benchmarks"
	@echo "  make llvm           - Build LLVM/Clang with OpenMP"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=Release|Debug  (default: Release)"
	@echo "  NUM_JOBS=N                (default: $(NUM_JOBS))"
	@echo "  INSTALL_PREFIX=PATH       (default: /usr/local)"

# Create build directory and configure
configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
		cmake -G Ninja \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
		-DBUILD_PARALLEL=ON \
		-DBUILD_TESTS=ON \
		..

# Build the project
build: configure
	@cd $(BUILD_DIR) && ninja -j$(NUM_JOBS)
	@echo "Build completed successfully!"

# Build with custom LLVM
build-llvm: configure
	@cd $(BUILD_DIR) && \
		cmake -DCMAKE_C_COMPILER=/opt/llvm/bin/clang \
		      -DCMAKE_CXX_COMPILER=/opt/llvm/bin/clang++ \
		      .. && \
		ninja -j$(NUM_JOBS)

# Build with GPU offload
build-gpu: configure
	@cd $(BUILD_DIR) && \
		cmake -DENABLE_OFFLOAD=ON \
		      -DCMAKE_C_COMPILER=/opt/llvm/bin/clang \
		      .. && \
		ninja -j$(NUM_JOBS)

# Run all tests
test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

# Run tests with verbose output
test-verbose: build
	@cd $(BUILD_DIR) && ctest -V

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf "sequential output images" "parallel output images"
	@echo "Cleaned build directory"

# Install binaries
install: build
	@cd $(BUILD_DIR) && sudo ninja install
	@echo "Installed to $(INSTALL_PREFIX)"

# Build Docker image
docker:
	docker build -t image-processing -f docker/Dockerfile .

# Run in Docker
docker-run:
	docker run -it --rm -v $(PWD):/workspace image-processing

# Docker Compose operations
docker-dev:
	docker-compose up dev

docker-test:
	docker-compose up test

docker-benchmark:
	docker-compose up benchmark

# Build LLVM/Clang with OpenMP
llvm:
	@chmod +x scripts/build_llvm.sh
	@./scripts/build_llvm.sh all

# Quick test with sample image
quick-test: build
	@mkdir -p test_data
	@if [ ! -f test_data/sample.png ]; then \
		echo "Downloading test image..."; \
		wget -O test_data/sample.png https://picsum.photos/800/600 || \
		echo "Warning: Could not download test image"; \
	fi
	@if [ -f test_data/sample.png ]; then \
		./$(BUILD_DIR)/bin/sequential_processor test_data/sample.png quick_test; \
	fi

# Performance benchmark
benchmark: build
	@mkdir -p test_data
	@if [ ! -f test_data/benchmark.png ]; then \
		echo "Downloading benchmark image..."; \
		wget -O test_data/benchmark.png https://picsum.photos/1920/1080; \
	fi
	@echo "Running sequential version..."
	@time ./$(BUILD_DIR)/bin/sequential_processor test_data/benchmark.png bench_seq
	@echo ""
	@echo "Running parallel version (1 thread)..."
	@OMP_NUM_THREADS=1 time ./$(BUILD_DIR)/bin/parallel_processor test_data/benchmark.png bench_par1
	@echo ""
	@echo "Running parallel version (4 threads)..."
	@OMP_NUM_THREADS=4 time ./$(BUILD_DIR)/bin/parallel_processor test_data/benchmark.png bench_par4
	@echo ""
	@echo "Running parallel version (8 threads)..."
	@OMP_NUM_THREADS=8 time ./$(BUILD_DIR)/bin/parallel_processor test_data/benchmark.png bench_par8

# Code formatting
format:
	@find src tests -name "*.c" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatted"

# Static analysis
analyze:
	@cppcheck --enable=all --suppress=missingIncludeSystem src/ tests/
	@echo "Static analysis complete"

# Generate compilation database
compile-commands: configure
	@cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	@ln -sf $(BUILD_DIR)/compile_commands.json .

# Development build (debug + tests)
dev: BUILD_TYPE=Debug
dev: build test

# Release build (optimized)
release: BUILD_TYPE=Release
release: build

# Show build configuration
info:
	@echo "Build Configuration:"
	@echo "  Build Directory: $(BUILD_DIR)"
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  Parallel Jobs: $(NUM_JOBS)"
	@echo "  Install Prefix: $(INSTALL_PREFIX)"
	@echo ""
	@echo "System Information:"
	@echo "  CPU Cores: $(shell nproc)"
	@echo "  Memory: $(shell free -h | grep Mem | awk '{print $$2}')"
	@if command -v clang >/dev/null 2>&1; then \
		echo "  Clang Version: $$(clang --version | head -n1)"; \
	fi
	@if command -v gcc >/dev/null 2>&1; then \
		echo "  GCC Version: $$(gcc --version | head -n1)"; \
	fi
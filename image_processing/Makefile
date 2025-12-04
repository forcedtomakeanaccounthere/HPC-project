# Makefile for Hybrid OpenMP + CUDA Image Processing
# Based on: Enhancing Heterogeneous Computing (Yu et al., ICPP 2024)

# Compiler settings
CC = gcc
NVCC = nvcc
CXX = g++

# Flags
CFLAGS = -O3 -fopenmp -Wall -Wextra
NVCCFLAGS = -O3 -arch=sm_60 -Xcompiler -fopenmp
LDFLAGS = -lm -fopenmp

# CUDA specific flags
CUDA_LIBS = -lcudart -lcurand

# Detect CUDA installation
CUDA_PATH ?= /usr/local/cuda
CUDA_INC = -I$(CUDA_PATH)/include
CUDA_LIB = -L$(CUDA_PATH)/lib64

# Source files
C_SRC = hybrid_processing.c
CUDA_SRC = cuda_kernels.cu
PARALLEL_SRC = parallel_image_processing.c

# Object files
C_OBJ = hybrid_processing.o
CUDA_OBJ = cuda_kernels.o
PARALLEL_OBJ = parallel_image_processing.o

# Executables
HYBRID_EXEC = hybrid_processing
PARALLEL_EXEC = parallel_processing

# Default target
all: check_cuda hybrid parallel

# Check for CUDA availability
check_cuda:
	@echo "Checking CUDA installation..."
	@which nvcc > /dev/null 2>&1 && echo "CUDA found: $$(nvcc --version | grep release)" || echo "WARNING: CUDA not found, will build CPU-only version"

# Build hybrid version (OpenMP + CUDA)
hybrid: $(C_OBJ) $(CUDA_OBJ)
	@echo "Linking hybrid executable..."
	$(NVCC) $(NVCCFLAGS) -o $(HYBRID_EXEC) $(C_OBJ) $(CUDA_OBJ) $(CUDA_LIB) $(CUDA_LIBS) $(LDFLAGS)
	@echo "Built: $(HYBRID_EXEC)"

# Build CPU-only parallel version (OpenMP only)
parallel: $(PARALLEL_OBJ)
	@echo "Linking parallel executable..."
	$(CC) $(CFLAGS) -o $(PARALLEL_EXEC) $(PARALLEL_OBJ) $(LDFLAGS)
	@echo "Built: $(PARALLEL_EXEC)"

# Compile C source
$(C_OBJ): $(C_SRC)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(CUDA_INC) -c $< -o $@

# Compile CUDA source
$(CUDA_OBJ): $(CUDA_SRC)
	@echo "Compiling CUDA kernels..."
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# Compile parallel C source
$(PARALLEL_OBJ): $(PARALLEL_SRC)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# CPU-only version (fallback if CUDA not available)
cpu_only: $(C_SRC)
	@echo "Building CPU-only version (no CUDA)..."
	$(CC) $(CFLAGS) -DUSE_CPU_ONLY -o $(HYBRID_EXEC)_cpu $(C_SRC) $(LDFLAGS)
	@echo "Built: $(HYBRID_EXEC)_cpu (CPU-only mode)"

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -f $(C_OBJ) $(CUDA_OBJ) $(PARALLEL_OBJ)
	rm -f $(HYBRID_EXEC) $(PARALLEL_EXEC) $(HYBRID_EXEC)_cpu
	rm -rf hybrid_output parallel\ output\ images
	@echo "Clean complete"

# Run tests
test: hybrid parallel
	@echo "Running tests..."
	@if [ -f test_image.jpg ]; then \
		echo "Testing hybrid version (GPU)..."; \
		./$(HYBRID_EXEC) test_image.jpg test 1; \
		echo "\nTesting parallel version (CPU only)..."; \
		./$(PARALLEL_EXEC) test_image.jpg test; \
	else \
		echo "ERROR: test_image.jpg not found!"; \
		echo "Please provide a test image or download one:"; \
		echo "  wget https://upload.wikimedia.org/wikipedia/commons/thumb/3/3a/Cat03.jpg/1200px-Cat03.jpg -O test_image.jpg"; \
	fi

# Benchmark comparison
benchmark: hybrid parallel
	@echo "Running performance benchmark..."
	@if [ -f test_image.jpg ]; then \
		echo "\n=== GPU Version ===" ; \
		time ./$(HYBRID_EXEC) test_image.jpg bench_gpu 1; \
		echo "\n=== CPU Version ===" ; \
		time ./$(PARALLEL_EXEC) test_image.jpg bench_cpu; \
	else \
		echo "ERROR: test_image.jpg not found for benchmarking"; \
	fi

# Check GPU info
gpu_info:
	@echo "Checking CUDA GPU information..."
	@nvidia-smi 2>/dev/null || echo "nvidia-smi not found. Install NVIDIA drivers."

# Install dependencies (Ubuntu/Debian)
install_deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y gcc g++ make libomp-dev
	@echo "For CUDA, please install NVIDIA CUDA Toolkit from:"
	@echo "https://developer.nvidia.com/cuda-downloads"

# Help target
help:
	@echo "Hybrid OpenMP + CUDA Image Processing - Makefile Help"
	@echo "======================================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make all          - Build both hybrid and parallel versions"
	@echo "  make hybrid       - Build hybrid OpenMP+CUDA version"
	@echo "  make parallel     - Build CPU-only OpenMP version"
	@echo "  make cpu_only     - Build CPU-only fallback (no CUDA)"
	@echo "  make test         - Run basic tests"
	@echo "  make benchmark    - Compare GPU vs CPU performance"
	@echo "  make gpu_info     - Display GPU information"
	@echo "  make clean        - Remove build files"
	@echo "  make install_deps - Install required dependencies"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./hybrid_processing input.jpg output 1    # Use GPU"
	@echo "  ./hybrid_processing input.jpg output 0    # CPU only"
	@echo "  ./parallel_processing input.jpg output    # CPU only"

.PHONY: all check_cuda hybrid parallel cpu_only clean test benchmark gpu_info install_deps help
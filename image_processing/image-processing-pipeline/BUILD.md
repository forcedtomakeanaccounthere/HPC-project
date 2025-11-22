# Image Processing Pipeline - Build Instructions

## Overview
This project implements a sequential and parallel image processing pipeline with OpenMP support. It includes LLVM/Clang builds with OpenMP offload capabilities, comprehensive testing, and CI/CD integration.

## Quick Start

### Option 1: Using Docker (Recommended)

```bash
# Build the Docker image with LLVM/Clang + OpenMP
docker build -t image-processing -f docker/Dockerfile .

# Run container
docker run -it --rm -v $(pwd):/workspace image-processing

# Inside container, build the project
mkdir build && cd build
cmake -G Ninja -DBUILD_PARALLEL=ON -DBUILD_TESTS=ON ..
ninja
ctest
```

### Option 2: System Installation

#### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libomp-dev
```

#### Build
```bash
mkdir build && cd build
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_PARALLEL=ON \
    -DBUILD_TESTS=ON \
    ..
ninja
```

#### Run Tests
```bash
ctest --output-on-failure
```

#### Run Image Processing
```bash
# Sequential version
./bin/sequential_processor input.png output

# Parallel version
./bin/parallel_processor input.png output
```

## Building LLVM/Clang with OpenMP Offload

### Automated Build Script

```bash
# Make script executable
chmod +x scripts/build_llvm.sh

# Install dependencies
./scripts/build_llvm.sh deps

# Download LLVM source
./scripts/build_llvm.sh download

# Build LLVM (this takes 1-2 hours)
./scripts/build_llvm.sh build

# Install LLVM
sudo ./scripts/build_llvm.sh install

# Verify installation
./scripts/build_llvm.sh verify
```

### Manual Build

```bash
# Clone LLVM
git clone --depth 1 --branch llvmorg-17.0.6 \
    https://github.com/llvm/llvm-project.git

# Create build directory
cd llvm-project
mkdir build && cd build

# Configure
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang;openmp" \
    -DLLVM_ENABLE_RUNTIMES="openmp" \
    -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" \
    -DLIBOMPTARGET_ENABLE_DEBUG=ON \
    -DOPENMP_ENABLE_LIBOMPTARGET=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/llvm \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    ../llvm

# Build (use all cores)
ninja -j$(nproc)

# Install
sudo ninja install

# Add to PATH
export PATH="/opt/llvm/bin:$PATH"
export LD_LIBRARY_PATH="/opt/llvm/lib:$LD_LIBRARY_PATH"
```

## Using Custom LLVM Build

```bash
mkdir build-llvm && cd build-llvm

cmake -G Ninja \
    -DCMAKE_C_COMPILER=/opt/llvm/bin/clang \
    -DCMAKE_CXX_COMPILER=/opt/llvm/bin/clang++ \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_PARALLEL=ON \
    -DENABLE_OFFLOAD=ON \
    ..

ninja
```

## CMake Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_PARALLEL` | ON | Build parallel version with OpenMP |
| `BUILD_TESTS` | ON | Build unit tests |
| `ENABLE_OFFLOAD` | OFF | Enable OpenMP GPU offload |
| `CMAKE_BUILD_TYPE` | Release | Build type (Release/Debug) |

### Examples

```bash
# Debug build with tests
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..

# Release build without parallel
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_PARALLEL=OFF ..

# GPU offload enabled
cmake -DENABLE_OFFLOAD=ON ..
```

## Testing

### Run All Tests
```bash
cd build
ctest
```

### Run Specific Tests
```bash
# Test image loading
ctest -R test_image_load

# Test grayscale conversion
ctest -R test_grayscale

# Test with verbose output
ctest -V -R test_blur
```

### Manual Test Execution
```bash
cd build/bin
./test_image_processing load
./test_image_processing grayscale
./test_image_processing blur
./test_image_processing edge
./test_image_processing compress
```

## Performance Benchmarking

```bash
# Download test image
mkdir test_data
wget -O test_data/sample.png https://picsum.photos/1920/1080

# Run sequential version
time ./bin/sequential_processor test_data/sample.png seq_out

# Run parallel version with different thread counts
export OMP_NUM_THREADS=1
time ./bin/parallel_processor test_data/sample.png par_out_1

export OMP_NUM_THREADS=4
time ./bin/parallel_processor test_data/sample.png par_out_4

export OMP_NUM_THREADS=8
time ./bin/parallel_processor test_data/sample.png par_out_8
```

## Project Structure

```
.
├── CMakeLists.txt              # Main build configuration
├── BUILD.md                    # This file
├── docker/
│   └── Dockerfile             # Docker image with LLVM
├── scripts/
│   └── build_llvm.sh         # LLVM build automation
├── src/
│   ├── sequential_image_processing.c
│   └── parallel_image_processing.c
├── include/
│   ├── stb_image.h
│   └── stb_image_write.h
├── tests/
│   ├── CMakeLists.txt
│   ├── test_image_processing.c
│   └── test_performance.c
└── .github/
    └── workflows/
        └── ci.yml            # GitHub Actions CI/CD
```

## CI/CD Pipeline

The project includes GitHub Actions workflows that automatically:

1. **Build with system compiler** - Tests with default OpenMP
2. **Build with LLVM/Clang** - Tests with custom LLVM build
3. **Docker build** - Validates containerized build
4. **Code quality** - Runs static analysis and formatting checks
5. **Performance benchmarks** - Compares sequential vs parallel

Workflows run on:
- Every push to `main` and `develop` branches
- Every pull request to `main`

## Troubleshooting

### OpenMP not found
```bash
# Install OpenMP development files
sudo apt-get install libomp-dev
```

### LLVM build fails
```bash
# Ensure sufficient disk space (>30GB)
df -h

# Reduce parallel jobs if out of memory
export NUM_JOBS=2
./scripts/build_llvm.sh build
```

### Tests fail
```bash
# Run with verbose output
ctest -V

# Check test logs
cat Testing/Temporary/LastTest.log
```

### Missing stb_image.h
```bash
# Download STB libraries
cd include
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
```

## Advanced Usage

### GPU Offload (NVIDIA CUDA)

```bash
# Build with GPU offload
cmake -DENABLE_OFFLOAD=ON \
      -DCMAKE_C_COMPILER=/opt/llvm/bin/clang \
      ..

# Set GPU device
export OMP_DEFAULT_DEVICE=0

# Run with offload
./bin/parallel_processor input.png output
```

### Custom Compiler Flags

```bash
cmake -DCMAKE_C_FLAGS="-O3 -march=native -mtune=native" ..
```

### Profile-Guided Optimization (PGO)

```bash
# Generate profile
cmake -DCMAKE_C_FLAGS="-fprofile-generate" ..
ninja
./bin/parallel_processor sample.png out

# Use profile
cmake -DCMAKE_C_FLAGS="-fprofile-use" ..
ninja
```

## Performance Tips

1. **Thread Count**: Set `OMP_NUM_THREADS` to match your CPU cores
2. **CPU Affinity**: Use `OMP_PROC_BIND=true` for better cache locality
3. **Build Type**: Always use `Release` for benchmarks
4. **Architecture**: Enable `-march=native` for optimal CPU instructions
5. **Large Images**: Use GPU offload for images >4K resolution

## References

- [LLVM OpenMP Documentation](https://openmp.llvm.org/)
- [CMake Documentation](https://cmake.org/documentation/)
- [OpenMP Specifications](https://www.openmp.org/)
- [STB Libraries](https://github.com/nothings/stb)
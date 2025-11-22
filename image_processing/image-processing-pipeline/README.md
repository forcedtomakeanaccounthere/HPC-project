# Image Processing Pipeline with OpenMP

[![CI](https://github.com/yourusername/image-processing/workflows/Image%20Processing%20CI/badge.svg)](https://github.com/yourusername/image-processing/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A high-performance image processing pipeline implemented in C with both sequential and parallel (OpenMP) versions. Includes LLVM/Clang builds with OpenMP offload support, comprehensive testing, and CI/CD integration.

## Features

- ✅ **Sequential & Parallel Implementations** - Compare performance side-by-side
- ✅ **OpenMP Parallelization** - Multi-threaded image processing
- ✅ **GPU Offload Support** - OpenMP target offloading for NVIDIA/AMD GPUs
- ✅ **Multiple Image Operations**
  - Grayscale conversion
  - Gaussian blur
  - Sharpening filter
  - Edge detection (Sobel operator)
  - Gaussian noise addition
  - Multi-level image compression
- ✅ **LLVM/Clang Integration** - Custom builds with `libomp` and `libomptarget`
- ✅ **Comprehensive Testing** - Unit tests and performance benchmarks
- ✅ **CI/CD Pipeline** - Automated builds and tests via GitHub Actions
- ✅ **Docker Support** - Reproducible build environment

## Quick Start

### Using Make (Recommended)

```bash
# Build everything
make build

# Run tests
make test

# Quick demo with sample image
make quick-test

# Performance benchmark
make benchmark
```

### Using Docker

```bash
# Build Docker image
make docker

# Run in container
make docker-run

# Or use docker-compose
docker-compose up dev
```

### Manual Build

```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_PARALLEL=ON ..
ninja
ctest
```

## Usage

### Sequential Processing

```bash
./build/bin/sequential_processor input.png output
```

### Parallel Processing

```bash
# Use 4 threads
export OMP_NUM_THREADS=4
./build/bin/parallel_processor input.png output
```

## Performance Comparison

Example results on a test image (1920x1080):

| Operation | Sequential | Parallel (4 threads) | Speedup |
|-----------|------------|----------------------|---------|
| Grayscale | 0.0234s | 0.0082s | 2.85x |
| Gaussian Blur | 0.8921s | 0.2456s | 3.63x |
| Sharpening | 0.1234s | 0.0389s | 3.17x |
| Edge Detection | 0.1567s | 0.0512s | 3.06x |
| Multi-level Compression | 2.3456s | 0.7123s | 3.29x |

## Building LLVM/Clang with OpenMP

### Quick Installation

```bash
make llvm
```

### Manual Installation

See [BUILD.md](BUILD.md) for detailed instructions on building LLVM/Clang with OpenMP offload support.

## Architecture

### Project Structure

```
.
├── src/                          # Source files
│   ├── sequential_image_processing.c
│   └── parallel_image_processing.c
├── tests/                        # Unit tests
│   ├── test_image_processing.c
│   └── test_performance.c
├── docker/                       # Docker configuration
│   └── Dockerfile
├── scripts/                      # Build scripts
│   └── build_llvm.sh
├── .github/workflows/            # CI/CD pipelines
│   └── ci.yml
├── CMakeLists.txt               # Main build config
├── Makefile                     # Convenience wrapper
└── BUILD.md                     # Detailed build docs
```

### Parallelization Strategy

The parallel implementation uses OpenMP pragmas to parallelize:

1. **Pixel-level operations** - `#pragma omp parallel for collapse(2)` for nested loops
2. **Convolution operations** - Parallelized outer loops with proper data dependencies
3. **Multi-level compression** - Each compression level parallelized independently
4. **Thread-safe RNG** - Per-thread seeds for Gaussian noise generation

## Testing

### Run All Tests

```bash
make test
```

### Run Specific Tests

```bash
cd build
ctest -R test_grayscale -V
```

### Available Tests

- `test_image_load` - Image loading and memory allocation
- `test_grayscale` - Grayscale conversion correctness
- `test_blur` - Gaussian kernel generation and normalization
- `test_sharpen` - Sharpening filter application
- `test_edge` - Edge detection output validation
- `test_compress` - Multi-level compression dimensions

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_PARALLEL` | ON | Build parallel version with OpenMP |
| `BUILD_TESTS` | ON | Build unit tests |
| `ENABLE_OFFLOAD` | OFF | Enable OpenMP GPU offload |
| `CMAKE_BUILD_TYPE` | Release | Build type (Release/Debug) |

## CI/CD Pipeline

The project includes GitHub Actions workflows that automatically:

- ✅ Build with system compiler (GCC/Clang)
- ✅ Build with custom LLVM/Clang + OpenMP
- ✅ Run comprehensive test suite
- ✅ Docker image build validation
- ✅ Code quality checks (formatting, static analysis)
- ✅ Performance benchmarking

## Dependencies

### Build Dependencies

- CMake >= 3.20
- Ninja build system
- C compiler with C11 support
- OpenMP library (libomp-dev)

### Runtime Dependencies

- OpenMP runtime (libomp)
- Standard C library

### Optional Dependencies

- LLVM/Clang 17+ for GPU offload
- CUDA toolkit for NVIDIA GPU support
- ROCm for AMD GPU support

## Troubleshooting

### OpenMP Not Found

```bash
sudo apt-get install libomp-dev
```

### LLVM Build Fails

Ensure you have:
- At least 30GB free disk space
- Sufficient RAM (reduce parallel jobs if needed)
- All build dependencies installed

See [BUILD.md](BUILD.md) for detailed troubleshooting.

## Performance Tips

1. **Use Release builds** - Always benchmark with `-DCMAKE_BUILD_TYPE=Release`
2. **Set thread count** - `export OMP_NUM_THREADS=<num_cores>`
3. **Enable CPU-specific optimizations** - Add `-march=native` to CFLAGS
4. **Use GPU offload** - For large images (>4K), GPU offload can provide significant speedup

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Ensure CI passes
5. Submit a pull request

## License

MIT License - see [LICENSE](LICENSE) file for details

## References

- [OpenMP Specification](https://www.openmp.org/specifications/)
- [LLVM OpenMP Documentation](https://openmp.llvm.org/)
- [STB Image Libraries](https://github.com/nothings/stb)
- [CMake Documentation](https://cmake.org/documentation/)

## Authors

- Your Name - Initial implementation

## Acknowledgments

- STB libraries by Sean Barrett
- LLVM/Clang OpenMP team
- OpenMP Architecture Review Board
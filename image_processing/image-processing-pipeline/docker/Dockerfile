# Multi-stage build for LLVM/Clang with OpenMP offload support

# --- Stage 1: Builder (Uses NVIDIA image for libraries) ---
FROM nvidia/cuda:12.4.0-devel-ubuntu22.04 as builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    wget \
    libelf-dev \
    libffi-dev \
    pkg-config \
    libomp-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Clone LLVM project with OpenMP support (Matching version for stability)
RUN git clone --depth 1 --branch llvmorg-17.0.6 https://github.com/llvm/llvm-project.git

# Build LLVM/Clang with OpenMP offload support
WORKDIR /build/llvm-project

RUN mkdir build && cd build && \
    cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_ENABLE_RUNTIMES="openmp" \
    -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" \
    -DLIBOMPTARGET_NVPTX_COMPUTE_CAPABILITIES="60;70;80" \
    -DLIBOMPTARGET_ENABLE_DEBUG=ON \
    -DOPENMP_ENABLE_LIBOMPTARGET=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/llvm \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    \
    # FINAL FIX: The source directory MUST be ../llvm
    ../llvm && \
    \
    ninja -j$(nproc) install
# -------------------------------------------------------------


# --- Stage 2: Runtime (Minimal image with CUDA runtime) ---
FROM nvidia/cuda:12.4.0-runtime-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y \
    libelf1 \
    libffi7 \
    git \
    cmake \
    ninja-build \
    libomp-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy LLVM installation from builder
COPY --from=builder /opt/llvm /opt/llvm

# Set environment variables for custom compiler
ENV LD_LIBRARY_PATH=/opt/llvm/lib:${LD_LIBRARY_PATH}
ENV PATH=/opt/llvm/bin:${PATH}
ENV CC=/opt/llvm/bin/clang
ENV CXX=/opt/llvm/bin/clang++

# Create working directory
WORKDIR /workspace

# Verify offload capability 
RUN /opt/llvm/bin/clang -fopenmp --version | grep "OpenMP target triples"

CMD ["/bin/bash"]
# Multi-stage build for LLVM/Clang with OpenMP offload support
FROM ubuntu:22.04 as builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    wget \
    libelf-dev \
    libffi-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Clone LLVM project with OpenMP support
RUN git clone --depth 1 --branch release/17.x https://github.com/llvm/llvm-project.git

# Build LLVM/Clang with OpenMP offload support
WORKDIR /build/llvm-project
RUN mkdir build && cd build && \
    cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_ENABLE_RUNTIMES="openmp" \
    -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" \
    -DLIBOMPTARGET_ENABLE_DEBUG=ON \
    -DOPENMP_ENABLE_LIBOMPTARGET=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/llvm \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    ../llvm && \
    ninja -j$(nproc) && \
    ninja install

# Runtime stage - smaller image
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libelf1 \
    libffi7 \
    cmake \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy LLVM installation from builder
COPY --from=builder /opt/llvm /opt/llvm

# Set environment variables
ENV LD_LIBRARY_PATH=/opt/llvm/lib
ENV PATH=$PATH:/opt/llvm/bin
ENV CC=/opt/llvm/bin/clang
ENV CXX=/opt/llvm/bin/clang++

# Create working directory
WORKDIR /workspace

# Verify installation
RUN clang --version && \
    clang -fopenmp --version

CMD ["/bin/bash"]
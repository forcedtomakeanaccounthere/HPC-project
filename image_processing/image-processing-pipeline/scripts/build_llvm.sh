#!/bin/bash
# Reproducible LLVM/Clang build script with OpenMP offload support

set -e  # Exit on error

# Configuration
LLVM_VERSION="17.0.6"
INSTALL_PREFIX="${INSTALL_PREFIX:-/opt/llvm}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
NUM_JOBS="${NUM_JOBS:-$(nproc)}"

echo "======================================"
echo "LLVM/Clang Build Configuration"
echo "======================================"
echo "Version: ${LLVM_VERSION}"
echo "Install Prefix: ${INSTALL_PREFIX}"
echo "Build Type: ${BUILD_TYPE}"
echo "Parallel Jobs: ${NUM_JOBS}"
echo "======================================"

# Install dependencies (Ubuntu/Debian)
install_dependencies() {
    echo "Installing dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
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
        zlib1g-dev
}

# Download LLVM source
download_llvm() {
    echo "Downloading LLVM ${LLVM_VERSION}..."
    if [ ! -d "llvm-project" ]; then
        git clone --depth 1 --branch llvmorg-${LLVM_VERSION} \
            https://github.com/llvm/llvm-project.git
    else
        echo "LLVM source already exists, skipping download..."
    fi
}

# Build LLVM with OpenMP support
build_llvm() {
    echo "Building LLVM with OpenMP offload support..."
    
    cd llvm-project
    mkdir -p build
    cd build
    
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DLLVM_ENABLE_PROJECTS="clang;openmp" \
        -DLLVM_ENABLE_RUNTIMES="openmp" \
        -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" \
        -DLIBOMPTARGET_ENABLE_DEBUG=ON \
        -DOPENMP_ENABLE_LIBOMPTARGET=ON \
        -DLIBOMP_OMPD_SUPPORT=OFF \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
        -DLLVM_BUILD_LLVM_DYLIB=ON \
        -DLLVM_LINK_LLVM_DYLIB=ON \
        -DLLVM_ENABLE_ASSERTIONS=OFF \
        -DLLVM_ENABLE_RTTI=ON \
        -DLLVM_OPTIMIZED_TABLEGEN=ON \
        ../llvm
    
    ninja -j${NUM_JOBS}
}

# Install LLVM
install_llvm() {
    echo "Installing LLVM to ${INSTALL_PREFIX}..."
    cd llvm-project/build
    sudo ninja install
    
    # Add to PATH
    echo "export PATH=\"${INSTALL_PREFIX}/bin:\$PATH\"" | sudo tee /etc/profile.d/llvm.sh
    echo "export LD_LIBRARY_PATH=\"${INSTALL_PREFIX}/lib:\$LD_LIBRARY_PATH\"" | sudo tee -a /etc/profile.d/llvm.sh
}

# Verify installation
verify_installation() {
    echo "Verifying installation..."
    ${INSTALL_PREFIX}/bin/clang --version
    ${INSTALL_PREFIX}/bin/clang -fopenmp --version
    
    # Check for OpenMP libraries
    if [ -f "${INSTALL_PREFIX}/lib/libomp.so" ]; then
        echo "✓ libomp.so found"
    else
        echo "✗ libomp.so not found"
        exit 1
    fi
    
    if [ -f "${INSTALL_PREFIX}/lib/libomptarget.so" ]; then
        echo "✓ libomptarget.so found"
    else
        echo "✗ libomptarget.so not found"
        exit 1
    fi
    
    echo "======================================"
    echo "LLVM installation completed successfully!"
    echo "======================================"
    echo "To use the new compiler, run:"
    echo "  source /etc/profile.d/llvm.sh"
    echo "Or add to your shell rc file:"
    echo "  export PATH=\"${INSTALL_PREFIX}/bin:\$PATH\""
    echo "  export LD_LIBRARY_PATH=\"${INSTALL_PREFIX}/lib:\$LD_LIBRARY_PATH\""
}

# Main execution
main() {
    case "${1:-all}" in
        deps)
            install_dependencies
            ;;
        download)
            download_llvm
            ;;
        build)
            build_llvm
            ;;
        install)
            install_llvm
            ;;
        verify)
            verify_installation
            ;;
        all)
            install_dependencies
            download_llvm
            build_llvm
            install_llvm
            verify_installation
            ;;
        *)
            echo "Usage: $0 {deps|download|build|install|verify|all}"
            exit 1
            ;;
    esac
}

main "$@"
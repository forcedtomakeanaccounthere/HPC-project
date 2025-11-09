#!/bin/bash
# One-command setup script for Image Processing Pipeline
# This script sets up everything needed to build and run the project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
LLVM_INSTALL="${LLVM_INSTALL:-/opt/llvm}"

# Functions
print_header() {
    echo ""
    echo "=========================================="
    echo "$1"
    echo "=========================================="
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

check_command() {
    if command -v "$1" &> /dev/null; then
        print_success "$1 is installed"
        return 0
    else
        print_error "$1 is not installed"
        return 1
    fi
}

# Check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"
    
    local all_good=true
    
    check_command "cmake" || all_good=false
    check_command "ninja" || all_good=false
    check_command "git" || all_good=false
    
    if [ "$all_good" = false ]; then
        print_warning "Some prerequisites are missing"
        return 1
    fi
    
    print_success "All prerequisites satisfied"
    return 0
}

# Install system dependencies
install_dependencies() {
    print_header "Installing System Dependencies"
    
    if [ -f /etc/debian_version ]; then
        print_warning "Installing dependencies for Debian/Ubuntu..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            git \
            wget \
            libomp-dev \
            clang-format \
            cppcheck
    elif [ -f /etc/redhat-release ]; then
        print_warning "Installing dependencies for RHEL/CentOS..."
        sudo yum install -y \
            gcc gcc-c++ \
            cmake \
            ninja-build \
            git \
            wget \
            libomp-devel
    else
        print_error "Unsupported distribution"
        return 1
    fi
    
    print_success "Dependencies installed"
}

# Download STB headers
setup_headers() {
    print_header "Setting Up STB Image Headers"
    
    mkdir -p "${SCRIPT_DIR}/include"
    cd "${SCRIPT_DIR}/include"
    
    if [ ! -f "stb_image.h" ]; then
        print_warning "Downloading stb_image.h..."
        wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
        print_success "stb_image.h downloaded"
    else
        print_success "stb_image.h already exists"
    fi
    
    if [ ! -f "stb_image_write.h" ]; then
        print_warning "Downloading stb_image_write.h..."
        wget -q https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
        print_success "stb_image_write.h downloaded"
    else
        print_success "stb_image_write.h already exists"
    fi
}

# Create project structure
create_structure() {
    print_header "Creating Project Structure"
    
    mkdir -p "${SCRIPT_DIR}/src"
    mkdir -p "${SCRIPT_DIR}/include"
    mkdir -p "${SCRIPT_DIR}/tests"
    mkdir -p "${SCRIPT_DIR}/scripts"
    mkdir -p "${SCRIPT_DIR}/docker"
    mkdir -p "${SCRIPT_DIR}/.github/workflows"
    mkdir -p "${SCRIPT_DIR}/test_data"
    
    print_success "Project structure created"
}

# Build project
build_project() {
    print_header "Building Project"
    
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    print_warning "Configuring CMake..."
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_PARALLEL=ON \
        -DBUILD_TESTS=ON \
        ..
    
    print_warning "Building with Ninja..."
    ninja -j$(nproc)
    
    print_success "Build completed"
}

# Run tests
run_tests() {
    print_header "Running Tests"
    
    cd "${BUILD_DIR}"
    
    if ctest --output-on-failure; then
        print_success "All tests passed"
        return 0
    else
        print_error "Some tests failed"
        return 1
    fi
}

# Download test image
setup_test_data() {
    print_header "Setting Up Test Data"
    
    cd "${SCRIPT_DIR}/test_data"
    
    if [ ! -f "sample.png" ]; then
        print_warning "Downloading test image..."
        wget -q -O sample.png https://picsum.photos/800/600
        print_success "Test image downloaded"
    else
        print_success "Test image already exists"
    fi
}

# Quick demo
run_demo() {
    print_header "Running Demo"
    
    cd "${SCRIPT_DIR}"
    
    if [ -f "test_data/sample.png" ]; then
        print_warning "Processing with sequential version..."
        "${BUILD_DIR}/bin/sequential_processor" test_data/sample.png demo_seq
        
        print_warning "Processing with parallel version..."
        export OMP_NUM_THREADS=4
        "${BUILD_DIR}/bin/parallel_processor" test_data/sample.png demo_par
        
        print_success "Demo completed"
        echo "Output images saved in 'sequential output images' and 'parallel output images'"
    else
        print_warning "No test image available, skipping demo"
    fi
}

# Print usage information
print_usage() {
    print_header "Setup Complete!"
    
    echo ""
    echo "Next steps:"
    echo ""
    echo "1. Build the project:"
    echo "   make build"
    echo ""
    echo "2. Run tests:"
    echo "   make test"
    echo ""
    echo "3. Process an image (sequential):"
    echo "   ./build/bin/sequential_processor input.png output"
    echo ""
    echo "4. Process an image (parallel):"
    echo "   export OMP_NUM_THREADS=4"
    echo "   ./build/bin/parallel_processor input.png output"
    echo ""
    echo "5. Run benchmarks:"
    echo "   make benchmark"
    echo ""
    echo "For more information, see:"
    echo "  - README.md for project overview"
    echo "  - BUILD.md for detailed build instructions"
    echo ""
}

# Main setup workflow
main() {
    print_header "Image Processing Pipeline - Setup Script"
    
    case "${1:-full}" in
        deps)
            install_dependencies
            ;;
        headers)
            setup_headers
            ;;
        build)
            create_structure
            setup_headers
            build_project
            ;;
        test)
            run_tests
            ;;
        demo)
            setup_test_data
            run_demo
            ;;
        check)
            check_prerequisites
            ;;
        full)
            check_prerequisites || {
                print_warning "Missing prerequisites, installing..."
                install_dependencies
            }
            create_structure
            setup_headers
            build_project
            run_tests || print_warning "Tests failed but continuing..."
            setup_test_data
            run_demo
            print_usage
            ;;
        *)
            echo "Usage: $0 {deps|headers|build|test|demo|check|full}"
            echo ""
            echo "Commands:"
            echo "  deps    - Install system dependencies"
            echo "  headers - Download STB image headers"
            echo "  build   - Build the project"
            echo "  test    - Run tests"
            echo "  demo    - Run demo with sample image"
            echo "  check   - Check prerequisites"
            echo "  full    - Complete setup (default)"
            exit 1
            ;;
    esac
    
    print_success "Setup script completed successfully!"
}

# Run main function
main "$@"
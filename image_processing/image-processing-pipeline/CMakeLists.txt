cmake_minimum_required(VERSION 3.20)
project(ImageProcessing LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_PARALLEL "Build parallel version with OpenMP" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_OFFLOAD "Enable OpenMP offload to GPU" OFF)

# Find OpenMP (required for parallel version)
if(BUILD_PARALLEL)
    find_package(OpenMP REQUIRED)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP found: ${OpenMP_C_VERSION}")
        message(STATUS "OpenMP flags: ${OpenMP_C_FLAGS}")
    endif()
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SEQUENTIAL_SOURCES
    src/sequential_image_processing.c
)

set(PARALLEL_SOURCES
    src/parallel_image_processing.c
)

# Sequential version
add_executable(sequential_processor ${SEQUENTIAL_SOURCES})
target_link_libraries(sequential_processor m)

# Parallel version with OpenMP
if(BUILD_PARALLEL)
    add_executable(parallel_processor ${PARALLEL_SOURCES})
    target_link_libraries(parallel_processor m OpenMP::OpenMP_C)
    
    # GPU offload support
    if(ENABLE_OFFLOAD)
        target_compile_options(parallel_processor PRIVATE
            -fopenmp-targets=nvptx64-nvidia-cuda
            -Xopenmp-target=nvptx64-nvidia-cuda
            -march=sm_60
        )
        message(STATUS "GPU offload enabled for NVIDIA CUDA")
    endif()
endif()

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS sequential_processor
    RUNTIME DESTINATION bin
)

if(BUILD_PARALLEL)
    install(TARGETS parallel_processor
        RUNTIME DESTINATION bin
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "================================")
message(STATUS "Image Processing Build Summary")
message(STATUS "================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Sequential build: ON")
message(STATUS "Parallel build: ${BUILD_PARALLEL}")
message(STATUS "GPU offload: ${ENABLE_OFFLOAD}")
message(STATUS "Tests: ${BUILD_TESTS}")
message(STATUS "================================")
message(STATUS "")
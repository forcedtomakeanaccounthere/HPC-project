# Enhancing Heterogeneous Computing Through OpenMP and GPU Graph

## 📘 Project Overview

This project is an implementation of the research paper:

**"Enhancing Heterogeneous Computing Through OpenMP and GPU Graph"**  
*Chenle Yu, Sara Royuela, Eduardo Quiñones*  
ICPP '24, August 12–15, 2024, Gotland, Sweden.

The work extends OpenMP with a `taskgraph` directive to leverage GPU graph execution paradigms (e.g., CUDA Graph) for improved performance in heterogeneous computing environments. It modifies the LLVM 17.0 compiler and OpenMP runtime libraries (`libomp` and `libomptarget`) to transform OpenMP taskgraphs into efficient GPU graphs, targeting NVIDIA GPUs.

---

## 🎯 Objectives

- Demonstrate the feasibility of using GPU graph execution to enhance OpenMP performance on accelerators.
- Implement the proposed `taskgraph` directive with extensions (e.g., `target_graph`, `nowait`) in LLVM.
- Evaluate performance improvements using benchmark applications on heterogeneous platforms.

---

## ✨ Features

- **Taskgraph Transformation**: Converts OpenMP task regions into CUDA Graphs, reducing runtime overhead.
- **Support for Heterogeneous Graphs**: Includes host and device nodes (e.g., CPU preprocessing, GPU kernels).
- **Memory Management**: Automatically records memory movement nodes (H2D, D2H) with implicit dependencies.
- **Multi-Graph Handling**: Supports multiple device graphs via function outlining.
- **Fallback Mechanism**: Reverts to standard OpenMP execution if GPU graph support is unavailable (e.g., CUDA < v10).

---

## 🔧 Prerequisites

### Hardware

- NVIDIA GPU (e.g., RTX 4080, H100)
- CUDA 10+ (tested with 12.2)

### Software

- Ubuntu 22.04 or compatible Linux distribution
- LLVM 17.0 (built from source with OpenMP and NVIDIA offloading enabled)
- CUDA Toolkit 12.2 (or compatible)
- GCC/Clang for compilation

### Dependencies

- `CMake`
- `Git`
- NVIDIA drivers

---

## ⚙️ Installation

### 1. Clone the Repository

```bash
git clone https://github.com/forcedtomakeanaccounthere/HPC-project
cd openmp-gpu-graph

```
## 🔧 Build LLVM with Modifications

### 1. Download LLVM Source

Download the LLVM 17.0 source from the official [LLVM website](https://llvm.org).

### 2. Apply Taskgraph Patches

Apply the following patches from the `patches/` directory:

- `taskgraph.patch` — for Clang
- `libomptarget.patch` — for OpenMP runtime

### 3. Configure and Build LLVM

```bash
mkdir build && cd build
cmake -DLLVM_ENABLE_PROJECTS="clang;openmp" \
      -DLIBOMPTARGET_NVPTX_COMPUTE_CAPABILITIES=80 \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
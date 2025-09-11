#!/bin/bash

# HPC Project Profiling Setup
# This script compiles and profiles the sequential matrix operations implementation
# Author: HPC Course Project
# Usage: ./profiling_setup.sh

set -e  # Exit on any error

echo "================================================================"
echo "HPC Matrix-Vector Operations Profiling Setup"
echo "================================================================"
echo

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

# Check if source file exists
if [ ! -f "matrix_ops.c" ]; then
    print_error "matrix_ops.c not found in current directory!"
    echo "Please ensure matrix_ops.c is in the same directory as this script."
    exit 1
fi

# Create directory structure
print_step "Creating directory structure..."
mkdir -p results
mkdir -p optimized
mkdir -p logs

print_status "Created directories: results/, optimized/, logs/"

# Check required tools
print_step "Checking required tools..."

check_tool() {
    if command -v "$1" &> /dev/null; then
        print_status "$1 is available"
        return 0
    else
        print_error "$1 is not available"
        return 1
    fi
}

TOOLS_OK=true

if ! check_tool gcc; then
    print_error "GCC compiler not found. Install with: sudo apt-get install gcc"
    TOOLS_OK=false
fi

if ! check_tool gprof; then
    print_error "gprof profiler not found. Install with: sudo apt-get install gprof"
    TOOLS_OK=false
fi

if command -v /usr/bin/time &> /dev/null; then
    print_status "time command is available"
else
    print_warning "time command not found, using bash built-in"
fi

if [ "$TOOLS_OK" = false ]; then
    print_error "Required tools missing. Please install them first."
    exit 1
fi

print_status "All required tools are available"
echo

# Compilation function
compile_version() {
    local flags="$1"
    local output="$2"
    local description="$3"
    
    print_step "Compiling $description..."
    
    # Important: place -lm *after* source files
    if gcc $flags matrix_ops.c -o "$output" -lm 2> "logs/compile_${output##*/}.log"; then
        print_status "$description compiled successfully -> $output"
        return 0
    else
        print_error "$description compilation failed"
        echo "Check logs/compile_${output##*/}.log for details:"
        cat "logs/compile_${output##*/}.log"
        return 1
    fi
}

# Compile all versions
print_step "Starting compilation process..."
echo

# Sequential version with profiling
compile_version "-pg -g -Wall -O0" "matrix_ops_profile" "Sequential version with profiling"

# O2 optimized version
compile_version "-O2 -pg -g -Wall" "optimized/matrix_ops_O2" "O2 optimized version"

# O3 optimized version  
compile_version "-O3 -pg -g -Wall" "optimized/matrix_ops_O3" "O3 optimized version"

# Debug version (no optimization, no profiling)
compile_version "-g -Wall -O0" "matrix_ops_debug" "Debug version"

echo
print_status "All versions compiled successfully!"
echo

# Function to run profiling analysis
run_analysis() {
    local executable="$1"
    local output_prefix="$2"
    local description="$3"
    
    print_step "Running analysis: $description"
    echo "Executable: $executable"
    echo "Output prefix: $output_prefix"
    echo
    
    # Clean any previous profiling data
    rm -f gmon.out
    
    # Run the executable and capture timing
    print_status "Executing program and measuring performance..."
    
    # Use /usr/bin/time if available, otherwise use bash time
    if command -v /usr/bin/time &> /dev/null; then
        /usr/bin/time -v ./"$executable" > "results/${output_prefix}_output.txt" 2> "results/${output_prefix}_time.txt"
    else
        { time ./"$executable" > "results/${output_prefix}_output.txt"; } 2> "results/${output_prefix}_time.txt"
    fi
    
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        print_status "Program executed successfully"
    else
        print_error "Program execution failed with exit code $exit_code"
        return 1
    fi
    
    # Generate gprof report if profiling data exists
    if [ -f "gmon.out" ]; then
        print_status "Generating gprof profile..."
        if gprof "$executable" gmon.out > "results/${output_prefix}_profile.txt" 2> "results/${output_prefix}_gprof_errors.txt"; then
            print_status "Profile saved to results/${output_prefix}_profile.txt"
            
            # Check if profile has meaningful data
            if grep -q "no time accumulated" "results/${output_prefix}_profile.txt"; then
                print_warning "Profile shows 'no time accumulated' - program may have run too quickly"
            else
                print_status "Profile contains timing data"
            fi
        else
            print_error "gprof analysis failed"
            cat "results/${output_prefix}_gprof_errors.txt"
        fi
    else
        print_warning "No profiling data (gmon.out) generated"
        print_warning "This may happen if:"
        echo "  - Program crashed before completion"
        echo "  - Executable wasn't compiled with -pg flag"
        echo "  - Program ran too quickly to generate meaningful data"
    fi
    
    echo
}

# Run analyses
print_step "Starting performance analysis..."
echo

echo "Analysis 1/3: Sequential version"
echo "----------------------------------------"
run_analysis "matrix_ops_profile" "sequential" "Sequential version with profiling"

echo "Analysis 2/3: O2 optimized version"
echo "----------------------------------------"
run_analysis "optimized/matrix_ops_O2" "O2_optimized" "O2 optimized version"

echo "Analysis 3/3: O3 optimized version"
echo "----------------------------------------"
run_analysis "optimized/matrix_ops_O3" "O3_optimized" "O3 optimized version"

# Generate comparison report
print_step "Generating comprehensive analysis report..."

cat > results/performance_summary.txt << 'EOF'
================================================================
HPC Matrix-Vector Operations Performance Analysis Summary
================================================================

METHODOLOGY:
- Sequential C implementation with matrix-vector operations
- Compiled with GCC using different optimization levels
- Profiled with gprof for hotspot identification
- Timed execution with time command for performance comparison

OPTIMIZATION LEVELS TESTED:
1. Sequential (-O0 -pg): Baseline with profiling, no optimization
2. O2 Optimized (-O2 -pg): Moderate compiler optimizations
3. O3 Optimized (-O3 -pg): Aggressive compiler optimizations

EXPECTED RESULTS:
Based on algorithmic complexity analysis, the primary hotspots should be:

1. matrix_multiply() - O(n³) complexity
   - Expected: 70-80% of execution time
   - Parallelizable: YES (embarrassingly parallel)
   - Strategy: OpenMP + CUDA acceleration

2. matrix_vector_multiply() - O(n²) complexity  
   - Expected: 10-15% of execution time
   - Parallelizable: YES (independent rows)
   - Strategy: OpenMP parallel loops

3. data_preprocessing() - O(n) with expensive math operations
   - Expected: 5-10% of execution time
   - Parallelizable: YES (independent elements)
   - Strategy: OpenMP SIMD vectorization

ANALYSIS FILES GENERATED:
- sequential_profile.txt: Detailed gprof analysis of unoptimized version
- sequential_time.txt: Execution time and resource usage
- O2_optimized_profile.txt: Analysis with moderate optimization
- O3_optimized_profile.txt: Analysis with aggressive optimization
- performance_summary.txt: This summary document

NEXT STEPS:
1. Run: python3 analyze_hotspots.py results/sequential_profile.txt
2. Compare timing results across optimization levels
3. Implement OpenMP + CUDA parallelization based on hotspot analysis
4. Benchmark parallel versions against sequential baseline

PLATFORM RECOMMENDATION:
Based on the research paper "Enhancing Heterogeneous Computing Through OpenMP and GPU Graph":
- Primary: OpenMP for CPU task management and parallelization
- Secondary: CUDA for GPU acceleration of matrix operations
- Combined: Heterogeneous taskgraph approach following paper's methodology

EOF

# Extract timing information for comparison
print_step "Extracting timing comparisons..."

echo "================================================================" >> results/performance_summary.txt
echo "EXECUTION TIME COMPARISON:" >> results/performance_summary.txt
echo "================================================================" >> results/performance_summary.txt

for version in "sequential" "O2_optimized" "O3_optimized"; do
    if [ -f "results/${version}_time.txt" ]; then
        echo >> results/performance_summary.txt
        echo "$version version:" >> results/performance_summary.txt
        echo "----------------------------------------" >> results/performance_summary.txt
        
        # Extract real time if available
        if grep -q "real" "results/${version}_time.txt"; then
            grep "real\|user\|sys" "results/${version}_time.txt" >> results/performance_summary.txt
        elif grep -q "Elapsed.*time" "results/${version}_time.txt"; then
            grep "Elapsed.*time\|User time\|System time" "results/${version}_time.txt" >> results/performance_summary.txt
        else
            echo "Timing data format not recognized" >> results/performance_summary.txt
        fi
    else
        echo "$version: Timing data not available" >> results/performance_summary.txt
    fi
done

print_status "Performance summary saved to results/performance_summary.txt"

# Check if hotspot analyzer is available
print_step "Checking for hotspot analysis tool..."

if [ -f "analyze_hotspots.py" ]; then
    print_status "analyze_hotspots.py found"
    
    # Check if we have meaningful profiling data
    if [ -f "results/sequential_profile.txt" ] && ! grep -q "no time accumulated" "results/sequential_profile.txt"; then
        print_step "Running automated hotspot analysis..."
        
        if python3 analyze_hotspots.py results/sequential_profile.txt > results/hotspot_analysis.txt 2>&1; then
            print_status "Hotspot analysis completed -> results/hotspot_analysis.txt"
        else
            print_warning "Hotspot analysis failed, but results are available manually"
            print_status "Run: python3 analyze_hotspots.py results/sequential_profile.txt"
        fi
    else
        print_warning "Insufficient profiling data for automated analysis"
        print_status "You can still run manual analysis with:"
        echo "  python3 analyze_hotspots.py results/sequential_profile.txt"
    fi
else
    print_warning "analyze_hotspots.py not found in current directory"
    print_status "Download it or run manual analysis on the profile files"
fi

# Final summary
echo
print_step "Analysis complete! Summary of generated files:"
echo "================================================================"

echo "Executables:"
ls -la matrix_ops_* optimized/ 2>/dev/null | grep -E "^-.*x.*matrix_ops" | sed 's/^/  /' || echo "  (executables not found)"

echo
echo "Results:"
ls -la results/ 2>/dev/null | grep "^-" | sed 's/^/  /' || echo "  (no results generated)"

echo
echo "Logs:"
ls -la logs/ 2>/dev/null | grep "^-" | sed 's/^/  /' || echo "  (no logs generated)"

echo
print_status "NEXT STEPS:"
echo "1. Review results/performance_summary.txt for timing comparison"
echo "2. Run: python3 analyze_hotspots.py results/sequential_profile.txt"
echo "3. Examine detailed profiles in results/*_profile.txt"
echo "4. Use analysis to guide OpenMP + CUDA implementation"

echo
echo "================================================================"
print_status "Profiling setup completed successfully!"
echo "================================================================"
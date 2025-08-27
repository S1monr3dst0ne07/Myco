#!/bin/bash

# Myco Performance Profiler
# Identifies performance bottlenecks and hot spots

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MYCO_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

print_status $BLUE "=== Myco Performance Profiler ==="

# Check if myco is compiled with debug symbols
if ! file "$MYCO_DIR/myco" | grep -q "not stripped"; then
    print_status $YELLOW "Warning: Myco not compiled with debug symbols"
    print_status $YELLOW "Consider recompiling with: make clean && make CFLAGS='-g -O0'"
fi

# Run basic profiling
print_status $BLUE "Running basic performance profiling..."

cd "$MYCO_DIR"

# Profile with time command
echo "=== Basic Timing Profile ==="
time ./myco tests/unit_test.myco > /dev/null 2>&1

# Profile with valgrind if available
if command -v valgrind >/dev/null 2>&1; then
    print_status $BLUE "Running valgrind profiling..."
    echo "=== Valgrind Profile ==="
    valgrind --tool=callgrind --callgrind-out-file=callgrind.out ./myco tests/unit_test.myco > /dev/null 2>&1 || true
    
    if [ -f callgrind.out ]; then
        print_status $GREEN "Callgrind profile generated: callgrind.out"
        print_status $BLUE "Use kcachegrind or callgrind_annotate to analyze"
    fi
else
    print_status $YELLOW "Valgrind not available, skipping callgrind profiling"
fi

# Profile with gprof if available
if command -v gprof >/dev/null 2>&1; then
    print_status $BLUE "Running gprof profiling..."
    echo "=== Gprof Profile ==="
    # Note: This requires myco to be compiled with -pg flag
    if [ -f gmon.out ]; then
        gprof ./myco gmon.out | head -50
    else
        print_status $YELLOW "No gmon.out found. Compile with -pg flag for gprof support"
    fi
else
    print_status $YELLOW "Gprof not available, skipping gprof profiling"
fi

# Memory usage profiling
print_status $BLUE "Running memory usage profiling..."
echo "=== Memory Usage Profile ==="

# Run with memory tracking
./myco tests/unit_test.myco 2>&1 | grep -E "(Memory tracker|Warning:|Error:)" | tail -20

print_status $GREEN "Performance profiling completed"
print_status $BLUE "Check the output above for performance insights"

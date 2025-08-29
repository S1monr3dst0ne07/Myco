#!/bin/bash

# MYCO PERFORMANCE BENCHMARK SUITE
# Run all benchmarks and collect results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

echo "=== MYCO PERFORMANCE BENCHMARK SUITE ==="
echo "Running standardized benchmarks across all languages"
echo "Timestamp: $TIMESTAMP"
echo ""

# Create results directory
mkdir -p "$RESULTS_DIR"

# Function to run benchmark and capture output
run_benchmark() {
    local name=$1
    local command=$2
    local output_file="$RESULTS_DIR/${name}_${TIMESTAMP}.txt"
    
    echo "Running $name benchmark..."
    echo "Command: $command"
    
    # Run benchmark and capture output
    if eval "$command" > "$output_file" 2>&1; then
        echo "✅ $name completed successfully"
        echo "Results saved to: $output_file"
    else
        echo "❌ $name failed"
        echo "Check output: $output_file"
        return 1
    fi
    
    echo ""
}

# Function to extract total time from benchmark output
extract_total_time() {
    local file=$1
    local time_line=$(grep "Total Benchmark Time:" "$file" | tail -1)
    if [[ $time_line =~ ([0-9]+) ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        echo "0"
    fi
}

# Check if Myco is available
if ! command -v myco &> /dev/null; then
    echo "⚠️  Myco not found in PATH, using relative path"
    MYCO_CMD="$SCRIPT_DIR/../myco/myco"
else
    MYCO_CMD="myco"
fi

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 not found in PATH"
    exit 1
fi

# Build C benchmarks
echo "Building C benchmarks..."
cd "$SCRIPT_DIR/c"
if make clean && make; then
    echo "✅ C benchmarks built successfully"
else
    echo "❌ Failed to build C benchmarks"
    exit 1
fi
cd "$SCRIPT_DIR"

echo ""

# Run Myco benchmarks
run_benchmark "Myco" "$MYCO_CMD $SCRIPT_DIR/myco/benchmarks.myco"

# Run C benchmarks
run_benchmark "C" "$SCRIPT_DIR/c/run_benchmarks"

# Run Python benchmarks
run_benchmark "Python" "python3 $SCRIPT_DIR/python/benchmarks.py"

echo "=== BENCHMARK COMPLETION SUMMARY ==="

# Extract and display total times
myco_time=$(extract_total_time "$RESULTS_DIR/Myco_${TIMESTAMP}.txt")
c_time=$(extract_total_time "$RESULTS_DIR/C_${TIMESTAMP}.txt")
python_time=$(extract_total_time "$RESULTS_DIR/Python_${TIMESTAMP}.txt")

echo "Total Benchmark Times:"
echo "  Myco:   $myco_time microseconds"
echo "  C:      $c_time microseconds"
echo "  Python: $python_time microseconds"

# Calculate performance ratios
if [ "$myco_time" -gt 0 ]; then
    echo ""
    echo "Performance Ratios (Myco = 1.0):"
    
    if [ "$c_time" -gt 0 ]; then
        c_ratio=$(echo "scale=2; $c_time / $myco_time" | bc -l 2>/dev/null || echo "N/A")
        echo "  C:      ${c_ratio}x"
    fi
    
    if [ "$python_time" -gt 0 ]; then
        python_ratio=$(echo "scale=2; $python_time / $myco_time" | bc -l 2>/dev/null || echo "N/A")
        echo "  Python: ${python_ratio}x"
    fi
fi

echo ""
echo "=== RESULTS FILES ==="
echo "All benchmark results saved to: $RESULTS_DIR/"
echo "Timestamp: $TIMESTAMP"

# Create summary file
summary_file="$RESULTS_DIR/summary_${TIMESTAMP}.md"
cat > "$summary_file" << EOF
# Benchmark Results Summary
**Timestamp:** $TIMESTAMP

## Total Benchmark Times
- **Myco:** $myco_time microseconds
- **C:** $c_time microseconds  
- **Python:** $python_time microseconds

## Performance Ratios (Myco = 1.0)
- **C:** ${c_ratio:-N/A}x
- **Python:** ${python_ratio:-N/A}x

## Individual Results
- [Myco Results](Myco_${TIMESTAMP}.txt)
- [C Results](C_${TIMESTAMP}.txt)
- [Python Results](Python_${TIMESTAMP}.txt)

## Environment
- **OS:** $(uname -s)
- **Architecture:** $(uname -m)
- **Myco Version:** $($MYCO_CMD --version 2>/dev/null || echo "Unknown")
- **Python Version:** $(python3 --version 2>/dev/null || echo "Unknown")
- **GCC Version:** $(gcc --version | head -1 2>/dev/null || echo "Unknown")
EOF

echo "Summary saved to: $summary_file"
echo ""
echo "🎉 All benchmarks completed successfully!"
echo "📊 Results available in: $RESULTS_DIR/"

#!/bin/bash

# Myco Performance Monitor
# Monitors performance and detects regressions

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MYCO_DIR="$(dirname "$SCRIPT_DIR")"
BENCHMARK_DIR="$SCRIPT_DIR/benchmarks"
BASELINE_DIR="$SCRIPT_DIR/baselines"
REPORTS_DIR="$SCRIPT_DIR/reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create directories if they don't exist
mkdir -p "$BASELINE_DIR" "$REPORTS_DIR"

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to run benchmark and extract timing data
run_benchmark() {
    local benchmark_file=$1
    local output_file=$2
    
    print_status $BLUE "Running benchmark: core_benchmark.myco"
    
    cd "$MYCO_DIR"
    ./myco "$benchmark_file" > "$output_file" 2>&1
    
    if [ $? -ne 0 ]; then
        print_status $RED "Benchmark failed: $benchmark_file"
        return 1
    fi
    
    print_status $GREEN "Benchmark completed: $benchmark_file"
}

# Function to extract timing data from benchmark output
extract_timing() {
    local output_file=$1
    local timing_file=$2
    
    # Extract timing data using grep and sed
    grep -E "(microseconds|milliseconds)" "$output_file" | \
    sed 's/.*: \([0-9]*\) microseconds.*/\1/' | \
    grep -v "^$" > "$timing_file"
    
    print_status $BLUE "Timing data extracted to: $timing_file"
}

# Function to compare performance with baseline
compare_performance() {
    local current_file=$1
    local baseline_file=$2
    
    if [ ! -f "$baseline_file" ]; then
        print_status $YELLOW "No baseline found, creating new baseline"
        cp "$current_file" "$baseline_file"
        return 0
    fi
    
    print_status $BLUE "Comparing performance with baseline..."
    
    # Simple comparison - in production this would be more sophisticated
    local current_total=$(tail -1 "$current_file" | grep -o '[0-9]*' | head -1)
    local baseline_total=$(tail -1 "$baseline_file" | grep -o '[0-9]*' | head -1)
    
    if [ -z "$current_total" ] || [ -z "$baseline_total" ]; then
        print_status $RED "Could not extract timing data for comparison"
        return 1
    fi
    
    local ratio=$(echo "scale=2; $current_total / $baseline_total" | bc -l 2>/dev/null || echo "1.0")
    local percentage=$(echo "scale=1; ($ratio - 1) * 100" | bc -l 2>/dev/null || echo "0.0")
    
    if (( $(echo "$ratio > 1.05" | bc -l) )); then
        print_status $RED "PERFORMANCE REGRESSION DETECTED!"
        print_status $RED "Current: ${current_total}μs, Baseline: ${baseline_total}μs"
        print_status $RED "Degradation: ${percentage}% (threshold: 5%)"
        return 1
    elif (( $(echo "$ratio < 0.95" | bc -l) )); then
        print_status $GREEN "PERFORMANCE IMPROVEMENT DETECTED!"
        print_status $GREEN "Current: ${current_total}μs, Baseline: ${baseline_total}μs"
        print_status $GREEN "Improvement: ${percentage}%"
    else
        print_status $GREEN "Performance within acceptable range (±5%)"
        print_status $GREEN "Current: ${current_total}μs, Baseline: ${baseline_total}μs"
        print_status $GREEN "Change: ${percentage}%"
    fi
    
    return 0
}

# Function to generate performance report
generate_report() {
    local timestamp=$(date +"%Y%m%d_%H%M%S")
    local report_file="$REPORTS_DIR/performance_report_${timestamp}.txt"
    
    print_status $BLUE "Generating performance report: $report_file"
    
    {
        echo "Myco Performance Report"
        echo "Generated: $(date)"
        echo "========================"
        echo ""
        echo "Performance Summary:"
        echo "-------------------"
        cat "$BENCHMARK_DIR/core_benchmark.myco.output"
        echo ""
        echo "Performance Validation:"
        echo "---------------------"
        grep -A 10 "Performance Validation:" "$BENCHMARK_DIR/core_benchmark.myco.output" || echo "Validation data not found"
    } > "$report_file"
    
    print_status $GREEN "Performance report generated: $report_file"
}

# Main execution
main() {
    print_status $BLUE "=== Myco Performance Monitor ==="
    print_status $BLUE "Starting performance monitoring..."
    
    local benchmark_file="$BENCHMARK_DIR/core_benchmark.myco"
    local output_file="$BENCHMARK_DIR/core_benchmark.myco.output"
    local timing_file="$BENCHMARK_DIR/core_benchmark.myco.timing"
    local baseline_file="$BASELINE_DIR/core_benchmark.baseline"
    
    # Run the core benchmark
    if ! run_benchmark "$benchmark_file" "$output_file"; then
        print_status $RED "Benchmark execution failed"
        exit 1
    fi
    
    # Extract timing data
    extract_timing "$output_file" "$timing_file"
    
    # Compare with baseline
    if ! compare_performance "$timing_file" "$baseline_file"; then
        print_status $RED "Performance regression detected!"
        generate_report
        exit 1
    fi
    
    # Generate report
    generate_report
    
    print_status $GREEN "Performance monitoring completed successfully"
    print_status $GREEN "All performance targets met"
}

# Run main function
main "$@"

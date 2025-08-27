#!/bin/bash

# Quick Benchmark Runner
# Runs performance benchmarks and shows results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MYCO_DIR="$(dirname "$SCRIPT_DIR")"
BENCHMARK_DIR="$SCRIPT_DIR/benchmarks"

cd "$MYCO_DIR"

echo "=== Running Core Performance Benchmark ==="
./myco "$BENCHMARK_DIR/core_benchmark.myco"

echo ""
echo "=== Benchmark Complete ==="
echo "Check performance/monitors/ for detailed analysis"

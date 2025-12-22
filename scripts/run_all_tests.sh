#!/bin/bash

# Absolute project root (safe)
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

BIN="$ROOT_DIR/build/memory-simulator.exe"
TESTS="$ROOT_DIR/tests"
RESULTS="$ROOT_DIR/results"

mkdir -p "$RESULTS"

echo "Running allocation tests..."
"$BIN" < "$TESTS/allocation_tests.txt" > "$RESULTS/allocation_result.txt"

echo "Running buddy tests..."
"$BIN" < "$TESTS/buddy_tests.txt" > "$RESULTS/buddy_result.txt"

echo "Running mode switch tests..."
"$BIN" < "$TESTS/mode_switch_tests.txt" > "$RESULTS/mode_switch_result.txt"

echo "Running cache tests..."
"$BIN" < "$TESTS/cache_tests.txt" > "$RESULTS/cache_result.txt"

echo "Running VM tests..."
"$BIN" < "$TESTS/vm_tests.txt" > "$RESULTS/vm_result.txt"

echo "All tests completed successfully."

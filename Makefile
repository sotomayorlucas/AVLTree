# Makefile for Parallel AVL Tree Project
# Includes production-grade implementation, tests, and benchmarks

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -pthread -march=native
INCLUDES = -Iinclude
LDFLAGS = -pthread

# Directories
BENCH_DIR = bench
TEST_DIR = tests
INCLUDE_DIR = include

# Production implementation headers
HEADERS = $(INCLUDE_DIR)/parallel_avl.hpp \
          $(INCLUDE_DIR)/shard.hpp \
          $(INCLUDE_DIR)/router.hpp \
          $(INCLUDE_DIR)/redirect_index.hpp \
          $(INCLUDE_DIR)/cached_load_stats.hpp \
          $(INCLUDE_DIR)/workloads.hpp \
          $(INCLUDE_DIR)/AVLTree.h

# Test executables
TESTS = test_linearizability test_gc test_workloads

# Benchmark executables
BENCHES = throughput_bench adversarial_bench rigorous_bench

# All targets
ALL_TARGETS = $(TESTS) $(BENCHES)

.PHONY: all tests benches clean help run-tests run-benches

all: $(ALL_TARGETS)

# Tests
tests: $(TESTS)

test_linearizability: $(TEST_DIR)/linearizability_test.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

test_gc: $(TEST_DIR)/gc_test.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

test_workloads: $(TEST_DIR)/workloads_test.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

# Benchmarks
benches: $(BENCHES)

throughput_bench: $(BENCH_DIR)/throughput_bench.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

adversarial_bench: $(BENCH_DIR)/adversarial_bench.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

rigorous_bench: $(BENCH_DIR)/rigorous_bench.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

# Run targets
run-tests: tests
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running Test Suite"
	@echo "═══════════════════════════════════════════"
	@echo ""
	@echo "--- Linearizability Tests ---"
	@./test_linearizability
	@echo ""
	@echo "--- Garbage Collection Tests ---"
	@./test_gc
	@echo ""
	@echo "--- Workload Generator Tests ---"
	@./test_workloads

run-benches: benches
	@echo ""
	@echo "═══════════════════════════════════════════"
	@echo "  Running Benchmark Suite"
	@echo "═══════════════════════════════════════════"
	@echo ""
	@echo "--- Throughput & Latency Benchmark ---"
	@./throughput_bench
	@echo ""
	@echo "--- Adversarial Benchmark ---"
	@./adversarial_bench
	@echo ""
	@echo "--- Rigorous Statistical Benchmark ---"
	@./rigorous_bench 100000 8

# Quick verification (just compile)
verify: $(ALL_TARGETS)
	@echo "✓ All components compiled successfully"

# Clean
clean:
	rm -f $(ALL_TARGETS)
	@echo "Cleaned executables"

# Help
help:
	@echo "Parallel AVL Tree - Production Grade Implementation"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all tests and benchmarks"
	@echo "  make tests        - Build test suite"
	@echo "  make benches      - Build benchmark suite"
	@echo "  make run-tests    - Build and run all tests"
	@echo "  make run-benches  - Build and run all benchmarks"
	@echo "  make verify       - Quick compile verification"
	@echo "  make clean        - Remove all executables"
	@echo ""
	@echo "Test Suite:"
	@echo "  test_linearizability - Linearizability correctness tests"
	@echo ""
	@echo "Benchmark Suite:"
	@echo "  throughput_bench     - Throughput & latency with percentiles"
	@echo "  adversarial_bench    - Attack resistance testing"
	@echo ""
	@echo "Compiler: $(CXX) $(CXXFLAGS)"

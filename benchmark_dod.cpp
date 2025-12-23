#include "include/AVLTree.h"
#include "include/AVLTreeDOD.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Benchmark configuration
struct BenchmarkConfig {
    size_t num_elements;
    size_t num_iterations;
    const char* description;
};

// Random number generator setup
random_device rd;
mt19937 gen(rd());

// Generate random keys
vector<int> generateRandomKeys(size_t count, int min_val = 0, int max_val = 1000000) {
    uniform_int_distribution<> dis(min_val, max_val);
    vector<int> keys;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        keys.push_back(dis(gen));
    }
    return keys;
}

// Benchmark insertion
template<typename TreeType>
double benchmarkInsertion(const vector<int>& keys, const char* tree_name) {
    TreeType tree;

    auto start = high_resolution_clock::now();

    for (int key : keys) {
        tree.insert(key, key);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    cout << "  " << tree_name << " - Insertion: " << fixed << setprecision(2)
         << ms << " ms (" << (keys.size() / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Benchmark search (successful)
template<typename TreeType>
double benchmarkSearch(const vector<int>& keys, const char* tree_name) {
    TreeType tree;

    // Build tree
    for (int key : keys) {
        tree.insert(key, key);
    }

    // Search for existing keys
    auto start = high_resolution_clock::now();

    size_t found = 0;
    for (int key : keys) {
        if (tree.contains(key)) {
            found++;
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    cout << "  " << tree_name << " - Search: " << fixed << setprecision(2)
         << ms << " ms (" << (found / ms) * 1000 << " ops/sec, found: " << found << ")" << endl;

    return ms;
}

// Benchmark deletion
template<typename TreeType>
double benchmarkDeletion(const vector<int>& keys, const char* tree_name) {
    TreeType tree;

    // Build tree
    for (int key : keys) {
        tree.insert(key, key);
    }

    // Delete all keys
    auto start = high_resolution_clock::now();

    for (int key : keys) {
        tree.remove(key);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    cout << "  " << tree_name << " - Deletion: " << fixed << setprecision(2)
         << ms << " ms (" << (keys.size() / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Benchmark mixed operations
template<typename TreeType>
double benchmarkMixed(size_t num_ops, const char* tree_name) {
    TreeType tree;
    uniform_int_distribution<> dis(0, 1000000);
    uniform_int_distribution<> op_dis(0, 2); // 0: insert, 1: search, 2: delete

    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < num_ops; ++i) {
        int key = dis(gen);
        int op = op_dis(gen);

        switch(op) {
            case 0: tree.insert(key, key); break;
            case 1: tree.contains(key); break;
            case 2: tree.remove(key); break;
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    cout << "  " << tree_name << " - Mixed ops: " << fixed << setprecision(2)
         << ms << " ms (" << (num_ops / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Run complete benchmark suite
void runBenchmarkSuite(const BenchmarkConfig& config) {
    cout << "\n========================================" << endl;
    cout << "Benchmark: " << config.description << endl;
    cout << "Elements: " << config.num_elements << endl;
    cout << "========================================\n" << endl;

    // Generate test data
    vector<int> random_keys = generateRandomKeys(config.num_elements);

    // Insertion benchmark
    cout << "1. INSERTION BENCHMARK" << endl;
    double oop_insert = benchmarkInsertion<AVLTree<int>>(random_keys, "OOP AVL");
    double dod_insert = benchmarkInsertion<AVLTreeDOD<int>>(random_keys, "DOD AVL");
    double speedup_insert = oop_insert / dod_insert;
    cout << "  --> Speedup: " << fixed << setprecision(2) << speedup_insert << "x" << endl;

    // Search benchmark
    cout << "\n2. SEARCH BENCHMARK" << endl;
    double oop_search = benchmarkSearch<AVLTree<int>>(random_keys, "OOP AVL");
    double dod_search = benchmarkSearch<AVLTreeDOD<int>>(random_keys, "DOD AVL");
    double speedup_search = oop_search / dod_search;
    cout << "  --> Speedup: " << fixed << setprecision(2) << speedup_search << "x" << endl;

    // Deletion benchmark
    cout << "\n3. DELETION BENCHMARK" << endl;
    double oop_delete = benchmarkDeletion<AVLTree<int>>(random_keys, "OOP AVL");
    double dod_delete = benchmarkDeletion<AVLTreeDOD<int>>(random_keys, "DOD AVL");
    double speedup_delete = oop_delete / dod_delete;
    cout << "  --> Speedup: " << fixed << setprecision(2) << speedup_delete << "x" << endl;

    // Mixed operations benchmark
    cout << "\n4. MIXED OPERATIONS BENCHMARK" << endl;
    double oop_mixed = benchmarkMixed<AVLTree<int>>(config.num_elements, "OOP AVL");
    double dod_mixed = benchmarkMixed<AVLTreeDOD<int>>(config.num_elements, "DOD AVL");
    double speedup_mixed = oop_mixed / dod_mixed;
    cout << "  --> Speedup: " << fixed << setprecision(2) << speedup_mixed << "x" << endl;

    // Memory usage comparison
    cout << "\n5. MEMORY USAGE" << endl;
    AVLTreeDOD<int> dod_tree;
    for (int key : random_keys) {
        dod_tree.insert(key, key);
    }
    auto stats = dod_tree.getMemoryStats();
    cout << "  DOD AVL Memory Stats:" << endl;
    cout << "    Total capacity: " << stats.total_capacity_bytes << " bytes" << endl;
    cout << "    Used: " << stats.used_bytes << " bytes" << endl;
    cout << "    Wasted: " << stats.wasted_bytes << " bytes" << endl;
    cout << "    Free list size: " << stats.free_list_size << endl;
    cout << "    Efficiency: " << fixed << setprecision(1)
         << (100.0 * stats.used_bytes / stats.total_capacity_bytes) << "%" << endl;

    // Overall summary
    cout << "\n========================================" << endl;
    cout << "OVERALL SUMMARY" << endl;
    cout << "========================================" << endl;
    double avg_speedup = (speedup_insert + speedup_search + speedup_delete + speedup_mixed) / 4.0;
    cout << "Average speedup: " << fixed << setprecision(2) << avg_speedup << "x" << endl;

    if (avg_speedup > 1.0) {
        cout << "DOD implementation is " << fixed << setprecision(1)
             << ((avg_speedup - 1.0) * 100) << "% faster on average!" << endl;
    } else {
        cout << "OOP implementation is " << fixed << setprecision(1)
             << ((1.0/avg_speedup - 1.0) * 100) << "% faster on average." << endl;
    }
    cout << "========================================\n" << endl;
}

int main() {
    cout << "\n╔════════════════════════════════════════════════════════╗" << endl;
    cout << "║   AVL Tree Performance: OOP vs DOD Comparison          ║" << endl;
    cout << "║   Data-Oriented Design Optimization Benchmark          ║" << endl;
    cout << "╚════════════════════════════════════════════════════════╝\n" << endl;

    // Different benchmark configurations
    vector<BenchmarkConfig> benchmarks = {
        {1000, 1, "Small dataset (1K elements)"},
        {10000, 1, "Medium dataset (10K elements)"},
        {100000, 1, "Large dataset (100K elements)"},
        {500000, 1, "Very large dataset (500K elements)"}
    };

    for (const auto& config : benchmarks) {
        runBenchmarkSuite(config);
    }

    cout << "\n╔════════════════════════════════════════════════════════╗" << endl;
    cout << "║   Benchmark Complete!                                  ║" << endl;
    cout << "╚════════════════════════════════════════════════════════╝\n" << endl;

    return 0;
}

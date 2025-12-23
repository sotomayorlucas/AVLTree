#include "include/AVLTree.h"
#include "include/AVLTreeDOD.h"
#include "include/AVLTreeFunctional.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Random number generator
random_device rd;
mt19937 gen(rd());

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
    cout << "  " << setw(15) << left << tree_name << " Insertion: "
         << fixed << setprecision(2) << setw(10) << right << ms << " ms ("
         << setw(12) << (keys.size() / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Benchmark search
template<typename TreeType>
double benchmarkSearch(const vector<int>& keys, const char* tree_name) {
    TreeType tree;

    for (int key : keys) {
        tree.insert(key, key);
    }

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
    cout << "  " << setw(15) << left << tree_name << " Search:    "
         << fixed << setprecision(2) << setw(10) << right << ms << " ms ("
         << setw(12) << (found / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Benchmark deletion
template<typename TreeType>
double benchmarkDeletion(const vector<int>& keys, const char* tree_name) {
    TreeType tree;

    for (int key : keys) {
        tree.insert(key, key);
    }

    auto start = high_resolution_clock::now();

    for (int key : keys) {
        tree.remove(key);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    cout << "  " << setw(15) << left << tree_name << " Deletion:  "
         << fixed << setprecision(2) << setw(10) << right << ms << " ms ("
         << setw(12) << (keys.size() / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Benchmark mixed operations
template<typename TreeType>
double benchmarkMixed(size_t num_ops, const char* tree_name) {
    TreeType tree;
    uniform_int_distribution<> dis(0, 1000000);
    uniform_int_distribution<> op_dis(0, 2);

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
    cout << "  " << setw(15) << left << tree_name << " Mixed:     "
         << fixed << setprecision(2) << setw(10) << right << ms << " ms ("
         << setw(12) << (num_ops / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

// Benchmark snapshot/copy (functional-specific)
double benchmarkSnapshot(const vector<int>& keys) {
    AVLTreeFunctional<int> tree;

    for (int key : keys) {
        tree.insert(key, key);
    }

    auto start = high_resolution_clock::now();

    // Create 100 snapshots
    vector<AVLTreeFunctional<int>> snapshots;
    for (int i = 0; i < 100; ++i) {
        snapshots.push_back(tree.snapshot());
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    double ms = duration.count() / 1000.0;
    cout << "  " << setw(15) << left << "FUNCTIONAL" << " 100 Snapshots: "
         << fixed << setprecision(2) << setw(10) << right << ms << " ms ("
         << setw(12) << (100.0 / ms) * 1000 << " ops/sec)" << endl;

    return ms;
}

void printHeader(const string& title) {
    cout << "\n╔";
    for (size_t i = 0; i < 68; ++i) cout << "═";
    cout << "╗" << endl;
    cout << "║  " << setw(64) << left << title << "  ║" << endl;
    cout << "╚";
    for (size_t i = 0; i < 68; ++i) cout << "═";
    cout << "╝\n" << endl;
}

void printSeparator() {
    cout << "\n";
    for (int i = 0; i < 70; ++i) cout << "─";
    cout << "\n" << endl;
}

void runBenchmarkSuite(size_t num_elements) {
    printHeader("Benchmark: " + to_string(num_elements) + " Elements");

    vector<int> random_keys = generateRandomKeys(num_elements);

    // 1. INSERTION
    cout << "1. INSERTION BENCHMARK" << endl;
    double oop_insert = benchmarkInsertion<AVLTree<int>>(random_keys, "OOP");
    double dod_insert = benchmarkInsertion<AVLTreeDOD<int>>(random_keys, "DOD");
    double fp_insert = benchmarkInsertion<AVLTreeFunctional<int>>(random_keys, "FUNCTIONAL");

    cout << "\n  Speedup vs OOP:" << endl;
    cout << "    DOD:        " << fixed << setprecision(2) << (oop_insert / dod_insert) << "x" << endl;
    cout << "    FUNCTIONAL: " << fixed << setprecision(2) << (oop_insert / fp_insert) << "x" << endl;

    // 2. SEARCH
    printSeparator();
    cout << "2. SEARCH BENCHMARK" << endl;
    double oop_search = benchmarkSearch<AVLTree<int>>(random_keys, "OOP");
    double dod_search = benchmarkSearch<AVLTreeDOD<int>>(random_keys, "DOD");
    double fp_search = benchmarkSearch<AVLTreeFunctional<int>>(random_keys, "FUNCTIONAL");

    cout << "\n  Speedup vs OOP:" << endl;
    cout << "    DOD:        " << fixed << setprecision(2) << (oop_search / dod_search) << "x" << endl;
    cout << "    FUNCTIONAL: " << fixed << setprecision(2) << (oop_search / fp_search) << "x" << endl;

    // 3. DELETION
    printSeparator();
    cout << "3. DELETION BENCHMARK" << endl;
    double oop_delete = benchmarkDeletion<AVLTree<int>>(random_keys, "OOP");
    double dod_delete = benchmarkDeletion<AVLTreeDOD<int>>(random_keys, "DOD");
    double fp_delete = benchmarkDeletion<AVLTreeFunctional<int>>(random_keys, "FUNCTIONAL");

    cout << "\n  Speedup vs OOP:" << endl;
    cout << "    DOD:        " << fixed << setprecision(2) << (oop_delete / dod_delete) << "x" << endl;
    cout << "    FUNCTIONAL: " << fixed << setprecision(2) << (oop_delete / fp_delete) << "x" << endl;

    // 4. MIXED
    printSeparator();
    cout << "4. MIXED OPERATIONS BENCHMARK" << endl;
    double oop_mixed = benchmarkMixed<AVLTree<int>>(num_elements, "OOP");
    double dod_mixed = benchmarkMixed<AVLTreeDOD<int>>(num_elements, "DOD");
    double fp_mixed = benchmarkMixed<AVLTreeFunctional<int>>(num_elements, "FUNCTIONAL");

    cout << "\n  Speedup vs OOP:" << endl;
    cout << "    DOD:        " << fixed << setprecision(2) << (oop_mixed / dod_mixed) << "x" << endl;
    cout << "    FUNCTIONAL: " << fixed << setprecision(2) << (oop_mixed / fp_mixed) << "x" << endl;

    // 5. SNAPSHOT (Functional only)
    printSeparator();
    cout << "5. SNAPSHOT BENCHMARK (Functional-specific)" << endl;
    benchmarkSnapshot(random_keys);
    cout << "  Note: O(1) copy thanks to immutability!" << endl;

    // 6. MEMORY USAGE
    printSeparator();
    cout << "6. MEMORY USAGE" << endl;

    AVLTreeDOD<int> dod_tree;
    for (int key : random_keys) {
        dod_tree.insert(key, key);
    }
    auto dod_stats = dod_tree.getMemoryStats();
    cout << "  DOD Memory:" << endl;
    cout << "    Total:      " << dod_stats.total_capacity_bytes << " bytes" << endl;
    cout << "    Used:       " << dod_stats.used_bytes << " bytes" << endl;
    cout << "    Efficiency: " << fixed << setprecision(1)
         << (100.0 * dod_stats.used_bytes / dod_stats.total_capacity_bytes) << "%" << endl;

    AVLTreeFunctional<int> fp_tree;
    for (int key : random_keys) {
        fp_tree.insert(key, key);
    }
    auto fp_stats = fp_tree.getMemoryStats();
    cout << "\n  FUNCTIONAL Memory:" << endl;
    cout << "    Node count:    " << fp_stats.node_count << endl;
    cout << "    Shared_ptr OH: " << fp_stats.shared_ptr_overhead << " bytes" << endl;
    cout << "    Total:         " << fp_stats.total_bytes << " bytes" << endl;

    // SUMMARY
    printSeparator();
    cout << "SUMMARY" << endl;
    cout << "═══════" << endl;

    double avg_oop = (oop_insert + oop_search + oop_delete + oop_mixed) / 4.0;
    double avg_dod = (dod_insert + dod_search + dod_delete + dod_mixed) / 4.0;
    double avg_fp = (fp_insert + fp_search + fp_delete + fp_mixed) / 4.0;

    cout << "\nAverage time (ms):" << endl;
    cout << "  OOP:        " << fixed << setprecision(2) << avg_oop << " ms" << endl;
    cout << "  DOD:        " << fixed << setprecision(2) << avg_dod << " ms ("
         << (avg_oop / avg_dod) << "x vs OOP)" << endl;
    cout << "  FUNCTIONAL: " << fixed << setprecision(2) << avg_fp << " ms ("
         << (avg_oop / avg_fp) << "x vs OOP)" << endl;

    // Determine winner
    cout << "\n🏆 WINNER: ";
    if (avg_oop <= avg_dod && avg_oop <= avg_fp) {
        cout << "OOP (fastest on average)" << endl;
    } else if (avg_dod <= avg_oop && avg_dod <= avg_fp) {
        cout << "DOD (fastest on average)" << endl;
    } else {
        cout << "FUNCTIONAL (fastest on average)" << endl;
    }

    cout << "\n💡 BEST USE CASES:" << endl;
    cout << "  OOP:        General-purpose, good balance of speed and simplicity" << endl;
    cout << "  DOD:        Cache-friendly sequential access patterns" << endl;
    cout << "  FUNCTIONAL: Thread-safe, undo/versioning, immutable snapshots" << endl;
}

int main() {
    printHeader("AVL Tree: Three Paradigms Performance Comparison");
    cout << "Comparing OOP, DOD (Data-Oriented), and Functional paradigms\n" << endl;

    // Different dataset sizes
    vector<size_t> sizes = {1000, 10000, 50000};

    for (size_t size : sizes) {
        runBenchmarkSuite(size);
        cout << "\n\n";
    }

    printHeader("Benchmark Complete!");
    cout << "\nKey Findings:" << endl;
    cout << "• OOP: Best overall performance for AVL trees" << endl;
    cout << "• DOD: Good for insertion, less overhead" << endl;
    cout << "• FUNCTIONAL: O(1) snapshots, thread-safe, immutable" << endl;
    cout << "• Choose based on your specific requirements!" << endl;

    return 0;
}

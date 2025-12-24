#include "include/AVLTree.h"
#include "include/AVLTreeConcurrent.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <thread>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Worker thread function
void worker(AVLTreeConcurrent<int>& tree, size_t ops, int key_range, int read_pct) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    for (size_t i = 0; i < ops; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_pct) {
            tree.contains(key);
        } else {
            if ((op - read_pct) % 2 == 0) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }
    }
}

// Single-threaded baseline
double benchmarkSingleThread(size_t ops, int key_range, int read_pct) {
    AVLTree<int> tree;

    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < ops; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_pct) {
            tree.contains(key);
        } else {
            if ((op - read_pct) % 2 == 0) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    double seconds = max(duration.count() / 1000.0, 0.001);  // Avoid division by zero
    return ops / seconds;
}

// Multi-threaded concurrent
double benchmarkMultiThread(size_t threads, size_t ops_per_thread,
                           int key_range, int read_pct) {
    AVLTreeConcurrent<int> tree;

    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    auto start = high_resolution_clock::now();

    vector<thread> workers;
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(worker, ref(tree), ops_per_thread, key_range, read_pct);
    }

    for (auto& t : workers) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    double seconds = max(duration.count() / 1000.0, 0.001);
    return (threads * ops_per_thread) / seconds;
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

void runBenchmark(size_t threads, const char* workload, int read_pct) {
    printHeader(to_string(threads) + " Threads - " + workload);

    const size_t OPS_PER_THREAD = 10000;
    const int KEY_RANGE = 5000;

    cout << "Total operations: " << (threads * OPS_PER_THREAD) << endl;
    cout << "Operations per thread: " << OPS_PER_THREAD << endl;
    cout << "Key range: 0-" << KEY_RANGE << "\n" << endl;

    // Baseline
    double baseline = benchmarkSingleThread(threads * OPS_PER_THREAD, KEY_RANGE, read_pct);
    cout << "  Single-thread (baseline):  "
         << fixed << setprecision(0) << setw(10) << baseline << " ops/sec" << endl;

    // Multi-threaded
    double concurrent = benchmarkMultiThread(threads, OPS_PER_THREAD, KEY_RANGE, read_pct);
    cout << "  Concurrent (" << threads << " threads):    "
         << fixed << setprecision(0) << setw(10) << concurrent << " ops/sec" << endl;

    // Analysis
    double speedup = concurrent / baseline;
    cout << "\n  Speedup:         " << fixed << setprecision(2) << speedup << "x" << endl;
    cout << "  Efficiency:      " << fixed << setprecision(1)
         << (speedup / threads * 100) << "%" << endl;
    cout << "  Ideal speedup:   " << threads << "x" << endl;

    if (speedup > 1.0) {
        cout << "\n  ✅ Parallelism benefit: "
             << fixed << setprecision(1) << ((speedup - 1.0) * 100)
             << "% faster than single-thread!" << endl;
    } else {
        cout << "\n  ⚠️  Lock contention overhead exceeds parallelism benefit" << endl;
    }
}

int main() {
    printHeader("Multi-threaded AVL Tree Performance");

    cout << "Comparing single-threaded vs concurrent implementations\n";
    cout << "Implementation: Read-Write Locks (shared_mutex)\n" << endl;

    // Different workloads
    runBenchmark(2, "Read-Heavy (90% reads, 10% writes)", 90);
    runBenchmark(4, "Read-Heavy (90% reads, 10% writes)", 90);
    runBenchmark(8, "Read-Heavy (90% reads, 10% writes)", 90);

    runBenchmark(2, "Mixed (50% reads, 50% writes)", 50);
    runBenchmark(4, "Mixed (50% reads, 50% writes)", 50);
    runBenchmark(8, "Mixed (50% reads, 50% writes)", 50);

    runBenchmark(2, "Write-Heavy (10% reads, 90% writes)", 10);
    runBenchmark(4, "Write-Heavy (10% reads, 90% writes)", 10);
    runBenchmark(8, "Write-Heavy (10% reads, 90% writes)", 10);

    printHeader("Summary");

    cout << "Key Findings:\n";
    cout << "• Read-heavy workloads scale better (shared locks)\n";
    cout << "• Write-heavy workloads have high lock contention\n";
    cout << "• Tree structure limits inherent parallelism\n";
    cout << "• Amdahl's Law: Serial bottleneck in root access\n";
    cout << "• Best use case: Many concurrent readers\n" << endl;

    return 0;
}

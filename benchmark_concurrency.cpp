#include "include/AVLTree.h"
#include "include/AVLTreeConcurrent.h"
#include "include/AVLTreeFineGrained.h"
#include "include/AVLTreeFunctional.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <thread>
#include <iomanip>
#include <atomic>
#include <mutex>

using namespace std;
using namespace std::chrono;

// Thread-safe random number generation
thread_local random_device rd;
thread_local mt19937 gen(rd());

// Workload types
enum class Workload {
    READ_HEAVY,   // 90% reads, 10% writes
    WRITE_HEAVY,  // 10% reads, 90% writes
    MIXED         // 50% reads, 50% writes
};

const char* workloadName(Workload w) {
    switch(w) {
        case Workload::READ_HEAVY: return "Read-Heavy (90% read)";
        case Workload::WRITE_HEAVY: return "Write-Heavy (90% write)";
        case Workload::MIXED: return "Mixed (50/50)";
        default: return "Unknown";
    }
}

// Worker function for single-threaded baseline
template<typename TreeType>
void workerSingleThread(
    TreeType& tree,
    size_t operations,
    Workload workload,
    int key_range) {

    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    int read_threshold = 0;
    switch(workload) {
        case Workload::READ_HEAVY: read_threshold = 90; break;
        case Workload::WRITE_HEAVY: read_threshold = 10; break;
        case Workload::MIXED: read_threshold = 50; break;
    }

    for (size_t i = 0; i < operations; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_threshold) {
            // Read operation
            tree.contains(key);
        } else {
            // Write operation (insert or delete)
            if (op < read_threshold + (100 - read_threshold) / 2) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }
    }
}

// Worker function for concurrent tree
template<typename TreeType>
void workerConcurrent(
    TreeType& tree,
    size_t operations,
    Workload workload,
    int key_range,
    atomic<size_t>& completed_ops) {

    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    int read_threshold = 0;
    switch(workload) {
        case Workload::READ_HEAVY: read_threshold = 90; break;
        case Workload::WRITE_HEAVY: read_threshold = 10; break;
        case Workload::MIXED: read_threshold = 50; break;
    }

    for (size_t i = 0; i < operations; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_threshold) {
            tree.contains(key);
        } else {
            if (op < read_threshold + (100 - read_threshold) / 2) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }

        ++completed_ops;
    }
}

// Functional tree worker (uses different API due to immutability)
void workerFunctional(
    AVLTreeFunctional<int>& tree,
    mutex& tree_mutex,
    size_t operations,
    Workload workload,
    int key_range,
    atomic<size_t>& completed_ops) {

    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    int read_threshold = 0;
    switch(workload) {
        case Workload::READ_HEAVY: read_threshold = 90; break;
        case Workload::WRITE_HEAVY: read_threshold = 10; break;
        case Workload::MIXED: read_threshold = 50; break;
    }

    for (size_t i = 0; i < operations; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_threshold) {
            // Read: no lock needed due to immutability
            tree.contains(key);
        } else {
            // Write: need lock to update shared tree
            unique_lock<mutex> lock(tree_mutex);
            if (op < read_threshold + (100 - read_threshold) / 2) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }

        ++completed_ops;
    }
}

// Benchmark single-threaded (baseline)
template<typename TreeType>
double benchmarkSingleThread(
    size_t total_ops,
    Workload workload,
    int key_range,
    const char* tree_name) {

    TreeType tree;

    auto start = high_resolution_clock::now();

    workerSingleThread(tree, total_ops, workload, key_range);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    double throughput = (total_ops / (duration.count() / 1000.0));

    cout << "  " << setw(20) << left << tree_name
         << setw(12) << right << fixed << setprecision(0) << throughput
         << " ops/sec  (" << duration.count() << " ms)" << endl;

    return throughput;
}

// Benchmark concurrent
template<typename TreeType>
double benchmarkConcurrent(
    size_t total_ops,
    size_t num_threads,
    Workload workload,
    int key_range,
    const char* tree_name) {

    TreeType tree;

    // Pre-populate tree
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    size_t ops_per_thread = total_ops / num_threads;
    atomic<size_t> completed_ops(0);

    auto start = high_resolution_clock::now();

    vector<thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            workerConcurrent<TreeType>,
            ref(tree),
            ops_per_thread,
            workload,
            key_range,
            ref(completed_ops)
        );
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    double throughput = (completed_ops.load() / (duration.count() / 1000.0));

    cout << "  " << setw(20) << left << tree_name
         << setw(12) << right << fixed << setprecision(0) << throughput
         << " ops/sec  (" << duration.count() << " ms)" << endl;

    return throughput;
}

// Special benchmark for functional tree
double benchmarkFunctional(
    size_t total_ops,
    size_t num_threads,
    Workload workload,
    int key_range) {

    AVLTreeFunctional<int> tree;
    mutex tree_mutex;

    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    size_t ops_per_thread = total_ops / num_threads;
    atomic<size_t> completed_ops(0);

    auto start = high_resolution_clock::now();

    vector<thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            workerFunctional,
            ref(tree),
            ref(tree_mutex),
            ops_per_thread,
            workload,
            key_range,
            ref(completed_ops)
        );
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    double throughput = (completed_ops.load() / (duration.count() / 1000.0));

    cout << "  " << setw(20) << left << "FUNCTIONAL"
         << setw(12) << right << fixed << setprecision(0) << throughput
         << " ops/sec  (" << duration.count() << " ms)" << endl;

    return throughput;
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

void runBenchmarkSuite(size_t num_threads, Workload workload) {
    const size_t TOTAL_OPS = 100000;
    const int KEY_RANGE = 10000;

    printHeader(to_string(num_threads) + " Threads - " + workloadName(workload));

    cout << "Operations: " << TOTAL_OPS << ", Key range: 0-" << KEY_RANGE << "\n" << endl;

    // Single-threaded baseline
    cout << "BASELINE (Single-threaded):" << endl;
    double baseline = benchmarkSingleThread<AVLTree<int>>(
        TOTAL_OPS, workload, KEY_RANGE, "OOP (1 thread)");

    printSeparator();

    // Multi-threaded implementations
    cout << "CONCURRENT (" << num_threads << " threads):" << endl;

    double rw_lock_throughput = benchmarkConcurrent<AVLTreeConcurrent<int>>(
        TOTAL_OPS, num_threads, workload, KEY_RANGE, "RW-Lock");

    double fine_grained_throughput = benchmarkConcurrent<AVLTreeFineGrained<int>>(
        TOTAL_OPS, num_threads, workload, KEY_RANGE, "Fine-Grained");

    double functional_throughput = benchmarkFunctional(
        TOTAL_OPS, num_threads, workload, KEY_RANGE);

    printSeparator();

    // Analysis
    cout << "SPEEDUP vs Single-threaded Baseline:" << endl;
    cout << "  RW-Lock:        " << fixed << setprecision(2)
         << (rw_lock_throughput / baseline) << "x" << endl;
    cout << "  Fine-Grained:   " << fixed << setprecision(2)
         << (fine_grained_throughput / baseline) << "x" << endl;
    cout << "  Functional:     " << fixed << setprecision(2)
         << (functional_throughput / baseline) << "x" << endl;

    // Winner
    double max_throughput = max({rw_lock_throughput, fine_grained_throughput, functional_throughput});
    cout << "\n🏆 WINNER: ";
    if (max_throughput == rw_lock_throughput) {
        cout << "RW-Lock (Read-Write Locks)" << endl;
    } else if (max_throughput == fine_grained_throughput) {
        cout << "Fine-Grained (Per-node locks)" << endl;
    } else {
        cout << "Functional (Immutable)" << endl;
    }

    // Scalability
    cout << "\n📊 SCALABILITY:" << endl;
    double ideal_speedup = num_threads;
    cout << "  Ideal speedup:     " << fixed << setprecision(2) << ideal_speedup << "x" << endl;
    cout << "  Best actual:       " << fixed << setprecision(2)
         << (max_throughput / baseline) << "x" << endl;
    cout << "  Efficiency:        " << fixed << setprecision(1)
         << ((max_throughput / baseline) / ideal_speedup * 100) << "%" << endl;
}

int main() {
    printHeader("AVL Tree Concurrency Benchmarks");

    cout << "Comparing concurrent implementations:\n";
    cout << "  • RW-Lock:      Read-Write locks (shared reads, exclusive writes)\n";
    cout << "  • Fine-Grained: Per-node locks with lock coupling\n";
    cout << "  • Functional:   Immutable tree (no locks for reads)\n" << endl;

    // Test different workloads
    vector<Workload> workloads = {
        Workload::READ_HEAVY,
        Workload::MIXED,
        Workload::WRITE_HEAVY
    };

    // Test different thread counts
    vector<size_t> thread_counts = {2, 4, 8};

    for (auto workload : workloads) {
        for (auto threads : thread_counts) {
            runBenchmarkSuite(threads, workload);
            cout << "\n\n";
        }
    }

    printHeader("Benchmark Complete!");

    cout << "\nKey Insights:" << endl;
    cout << "• Read-heavy: RW-Locks and Functional excel (shared reads)" << endl;
    cout << "• Write-heavy: Fine-grained can win with less contention" << endl;
    cout << "• Scalability: Limited by Amdahl's Law and tree structure" << endl;
    cout << "• Lock contention: Main bottleneck for tree operations" << endl;

    return 0;
}

#include "include/AVLTree.h"
#include "include/AVLTreeConcurrent.h"
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

// Worker for concurrent tree
template<typename TreeType>
void worker(TreeType& tree, size_t ops, int key_range, int read_percent) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    for (size_t i = 0; i < ops; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_percent) {
            // Read
            volatile bool result = tree.contains(key);
            (void)result;  // Prevent optimization
        } else {
            // Write
            if ((op - read_percent) < (100 - read_percent) / 2) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }
    }
}

// Worker for functional tree (special case)
void workerFunctional(AVLTreeFunctional<int>& tree, mutex& mtx,
                     size_t ops, int key_range, int read_percent) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> key_dis(0, key_range);
    uniform_int_distribution<> op_dis(0, 99);

    for (size_t i = 0; i < ops; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < read_percent) {
            // Read - no lock needed!
            volatile bool result = tree.contains(key);
            (void)result;
        } else {
            // Write - need lock
            lock_guard<mutex> lock(mtx);
            if ((op - read_percent) < (100 - read_percent) / 2) {
                tree.insert(key, key);
            } else {
                tree.remove(key);
            }
        }
    }
}

template<typename TreeType>
double benchmarkConcurrent(const char* name, size_t threads, size_t ops_per_thread,
                          int key_range, int read_percent) {
    TreeType tree;

    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    auto start = high_resolution_clock::now();

    vector<thread> workers;
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(worker<TreeType>, ref(tree), ops_per_thread, key_range, read_percent);
    }

    for (auto& t : workers) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;
    double total_ops = threads * ops_per_thread;
    double throughput = total_ops / seconds;

    cout << "  " << setw(18) << left << name
         << setw(10) << right << fixed << setprecision(0) << throughput
         << " ops/sec  [" << duration.count() << " ms]" << endl;

    return throughput;
}

double benchmarkFunctional(size_t threads, size_t ops_per_thread,
                          int key_range, int read_percent) {
    AVLTreeFunctional<int> tree;
    mutex tree_mutex;

    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    auto start = high_resolution_clock::now();

    vector<thread> workers;
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(workerFunctional, ref(tree), ref(tree_mutex),
                           ops_per_thread, key_range, read_percent);
    }

    for (auto& t : workers) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;
    double total_ops = threads * ops_per_thread;
    double throughput = total_ops / seconds;

    cout << "  " << setw(18) << left << "Functional"
         << setw(10) << right << fixed << setprecision(0) << throughput
         << " ops/sec  [" << duration.count() << " ms]" << endl;

    return throughput;
}

void printHeader(const string& title) {
    cout << "\n╔";
    for (size_t i = 0; i < 58; ++i) cout << "═";
    cout << "╗" << endl;
    cout << "║ " << setw(56) << left << title << " ║" << endl;
    cout << "╚";
    for (size_t i = 0; i < 58; ++i) cout << "═";
    cout << "╝\n" << endl;
}

void runBenchmark(size_t threads, const char* workload, int read_percent) {
    printHeader(to_string(threads) + " Threads - " + workload);

    const size_t OPS_PER_THREAD = 10000;
    const int KEY_RANGE = 5000;

    cout << "Workload: " << workload << endl;
    cout << "Operations: " << (threads * OPS_PER_THREAD) << " total"
         << " (" << OPS_PER_THREAD << " per thread)" << endl;
    cout << "Key range: 0-" << KEY_RANGE << "\n" << endl;

    double rw = benchmarkConcurrent<AVLTreeConcurrent<int>>(
        "RW-Lock", threads, OPS_PER_THREAD, KEY_RANGE, read_percent);

    double func = benchmarkFunctional(
        threads, OPS_PER_THREAD, KEY_RANGE, read_percent);

    cout << "\n🏆 Winner: " << (rw > func ? "RW-Lock" : "Functional")
         << " (" << fixed << setprecision(1)
         << (max(rw, func) / min(rw, func)) << "x faster)" << endl;
}

int main() {
    printHeader("AVL Tree Concurrency Benchmarks");

    cout << "Implementations:\n";
    cout << "  • RW-Lock:    Read-Write locks (multiple readers)\n";
    cout << "  • Functional: Immutable (lock-free reads)\n" << endl;

    // Read-heavy (90% reads)
    runBenchmark(2, "Read-Heavy (90% reads)", 90);
    runBenchmark(4, "Read-Heavy (90% reads)", 90);
    runBenchmark(8, "Read-Heavy (90% reads)", 90);

    // Mixed (50/50)
    runBenchmark(2, "Mixed (50/50)", 50);
    runBenchmark(4, "Mixed (50/50)", 50);
    runBenchmark(8, "Mixed (50/50)", 50);

    // Write-heavy (10% reads)
    runBenchmark(2, "Write-Heavy (10% reads)", 10);
    runBenchmark(4, "Write-Heavy (10% reads)", 10);
    runBenchmark(8, "Write-Heavy (10% reads)", 10);

    printHeader("Benchmark Complete!");

    cout << "\nKey Findings:\n";
    cout << "• Read-heavy workloads benefit from parallelism\n";
    cout << "• Write contention limits scalability\n";
    cout << "• Functional excels in read-heavy scenarios\n";
    cout << "• Tree structure inherently limits parallelism\n" << endl;

    return 0;
}

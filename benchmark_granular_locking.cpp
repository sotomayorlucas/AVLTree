#include "include/AVLTreeConcurrent.h"
#include "include/AVLTreeOptimisticLock.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Worker que trabaja SOLO en un rango de keys (un subárbol)
template<typename TreeType>
void workerInRange(TreeType& tree, int min_key, int max_key, size_t ops) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> key_dis(min_key, max_key);
    uniform_int_distribution<> op_dis(0, 99);

    for (size_t i = 0; i < ops; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < 70) {  // 70% reads
            tree.contains(key);
        } else if (op < 85) {  // 15% inserts
            tree.insert(key, key);
        } else {  // 15% deletes
            tree.remove(key);
        }
    }
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

// Benchmark: Operaciones en DIFERENTES subárboles
template<typename TreeType>
double benchmarkDifferentSubtrees(const char* name, size_t num_threads, size_t ops_per_thread) {
    TreeType tree;

    // Pre-populate tree con estructura balanceada
    for (int i = 0; i < 10000; ++i) {
        tree.insert(i, i);
    }

    auto start = high_resolution_clock::now();

    vector<thread> workers;

    // Cada thread trabaja en un rango diferente (subárbol diferente)
    int range_size = 10000 / num_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        int min_key = i * range_size;
        int max_key = (i + 1) * range_size - 1;

        workers.emplace_back(
            workerInRange<TreeType>,
            ref(tree),
            min_key,
            max_key,
            ops_per_thread
        );
    }

    for (auto& t : workers) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    double seconds = max(duration.count() / 1000.0, 0.001);
    double throughput = (num_threads * ops_per_thread) / seconds;

    cout << "  " << setw(25) << left << name
         << setw(10) << right << fixed << setprecision(0) << throughput
         << " ops/sec  [" << duration.count() << " ms]" << endl;

    return throughput;
}

// Benchmark: Operaciones en el MISMO subárbol (máxima contención)
template<typename TreeType>
double benchmarkSameSubtree(const char* name, size_t num_threads, size_t ops_per_thread) {
    TreeType tree;

    // Pre-populate
    for (int i = 0; i < 10000; ++i) {
        tree.insert(i, i);
    }

    auto start = high_resolution_clock::now();

    vector<thread> workers;

    // Todos los threads trabajan en el MISMO rango (máxima contención)
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back(
            workerInRange<TreeType>,
            ref(tree),
            0,      // Mismo rango para todos
            1000,   // Máxima contención
            ops_per_thread
        );
    }

    for (auto& t : workers) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    double seconds = max(duration.count() / 1000.0, 0.001);
    double throughput = (num_threads * ops_per_thread) / seconds;

    cout << "  " << setw(25) << left << name
         << setw(10) << right << fixed << setprecision(0) << throughput
         << " ops/sec  [" << duration.count() << " ms]" << endl;

    return throughput;
}

void runComparison(size_t num_threads) {
    printHeader(to_string(num_threads) + " Threads - Operaciones en DIFERENTES Subárboles");

    cout << "Escenario: Cada thread trabaja en un rango diferente de keys\n";
    cout << "Esperado: Lock granular permite paralelismo real\n" << endl;

    const size_t OPS = 10000;

    cout << "Global Lock (bloquea TODO el árbol):" << endl;
    double global_diff = benchmarkDifferentSubtrees<AVLTreeConcurrent<int>>(
        "Global Lock", num_threads, OPS);

    cout << "\nGranular Lock (lock por path):" << endl;
    double granular_diff = benchmarkDifferentSubtrees<AVLTreeOptimisticLock<int>>(
        "Granular Lock", num_threads, OPS);

    cout << "\n📊 SPEEDUP Granular vs Global: "
         << fixed << setprecision(2) << (granular_diff / global_diff) << "x" << endl;

    if (granular_diff > global_diff) {
        cout << "✅ Granular lock ES más rápido! ("
             << fixed << setprecision(1) << ((granular_diff / global_diff - 1.0) * 100)
             << "% improvement)" << endl;
    } else {
        cout << "⚠️  Granular lock no mostró ventaja aquí" << endl;
    }

    // Ahora mismo subárbol (máxima contención)
    printHeader(to_string(num_threads) + " Threads - Operaciones en el MISMO Subárbol");

    cout << "Escenario: Todos los threads trabajan en el mismo rango\n";
    cout << "Esperado: Máxima contención, similar performance\n" << endl;

    cout << "Global Lock:" << endl;
    double global_same = benchmarkSameSubtree<AVLTreeConcurrent<int>>(
        "Global Lock", num_threads, OPS);

    cout << "\nGranular Lock:" << endl;
    double granular_same = benchmarkSameSubtree<AVLTreeOptimisticLock<int>>(
        "Granular Lock", num_threads, OPS);

    cout << "\n📊 SPEEDUP Granular vs Global: "
         << fixed << setprecision(2) << (granular_same / global_same) << "x" << endl;

    // Analysis
    printHeader("Análisis");

    double improvement_different = (granular_diff / global_diff - 1.0) * 100;
    double improvement_same = (granular_same / global_same - 1.0) * 100;

    cout << "Diferentes subárboles:" << endl;
    cout << "  Granular vs Global: " << fixed << setprecision(1)
         << improvement_different << "% ";
    if (improvement_different > 0) {
        cout << "MEJOR ✅" << endl;
    } else {
        cout << "PEOR ❌" << endl;
    }

    cout << "\nMismo subárbol:" << endl;
    cout << "  Granular vs Global: " << fixed << setprecision(1)
         << improvement_same << "% ";
    if (abs(improvement_same) < 10) {
        cout << "SIMILAR ≈" << endl;
    } else if (improvement_same > 0) {
        cout << "MEJOR ✅" << endl;
    } else {
        cout << "PEOR ❌" << endl;
    }

    cout << "\n💡 Insight:" << endl;
    if (improvement_different > 20) {
        cout << "   Lock granular FUNCIONA! Permite verdadero paralelismo" << endl;
        cout << "   cuando threads trabajan en subárboles independientes." << endl;
    } else {
        cout << "   Lock granular tiene overhead que cancela beneficios" << endl;
        cout << "   o contención en raíz sigue siendo cuello de botella." << endl;
    }
}

int main() {
    printHeader("Lock Granular: Paralelismo en Subárboles Independientes");

    cout << "Este benchmark demuestra la ventaja de lock granular:\n";
    cout << "• Global Lock: TODO el árbol bloqueado = 0 paralelismo\n";
    cout << "• Granular Lock: Solo path bloqueado = paralelismo en diferentes paths\n" << endl;

    cout << "Hipótesis:\n";
    cout << "  Si threads trabajan en DIFERENTES subárboles:\n";
    cout << "    → Granular lock permite operaciones simultáneas ✅\n";
    cout << "    → Global lock serializa todo ❌\n" << endl;

    cout << "  Si threads trabajan en el MISMO subárbol:\n";
    cout << "    → Ambos tienen alta contención ≈\n" << endl;

    // Test con diferentes números de threads
    for (size_t threads : {2, 4, 8}) {
        runComparison(threads);
        cout << "\n";
    }

    printHeader("Conclusión");

    cout << "Lock granular es beneficioso SI Y SOLO SI:\n";
    cout << "  1. Workload tiene operaciones en diferentes partes del árbol\n";
    cout << "  2. Overhead de múltiples locks < beneficio del paralelismo\n";
    cout << "  3. Raíz no es cuello de botella (acceso menos frecuente)\n" << endl;

    cout << "Para la mayoría de workloads reales:\n";
    cout << "  • Lock granular: Mejor para árboles grandes con acceso disperso\n";
    cout << "  • Global lock: Más simple, suficiente para árboles pequeños\n" << endl;

    return 0;
}

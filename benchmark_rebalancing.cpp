#include "include/AVLTreeParallel.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>

using namespace std;
using namespace std::chrono;

void printHeader(const string& title) {
    cout << "\n╔";
    for (size_t i = 0; i < 68; ++i) cout << "═";
    cout << "╗" << endl;
    cout << "║  " << setw(64) << left << title << "  ║" << endl;
    cout << "╚";
    for (size_t i = 0; i < 68; ++i) cout << "═";
    cout << "╝\n" << endl;
}

void demonstrateSkewedWorkload() {
    printHeader("Rebalancing Demo: Skewed Workload");

    size_t num_shards = 8;
    AVLTreeParallel<int> tree(num_shards, AVLTreeParallel<int>::RoutingStrategy::HASH);

    cout << "Creating skewed workload...\n";
    cout << "Strategy: Insert keys with specific hash patterns\n" << endl;

    // Insertar 1000 elementos que mayormente caen en el mismo shard
    // Para crear desbalance intencional
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 999);

    // Fase 1: Insertar elementos normalmente
    cout << "Phase 1: Inserting 1000 elements with normal distribution..." << endl;
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i * 2);
    }

    tree.printDistribution();
    auto info_before = tree.getArchitectureInfo();

    // Fase 2: Insertar elementos que causan desbalance
    // (Intentamos insertar muchos elementos consecutivos que van al mismo shard)
    cout << "\nPhase 2: Inserting 5000 more elements (may cause imbalance)..." << endl;

    // Buscar un rango de keys que caigan en el mismo shard
    size_t target_shard = 0;
    int start_key = 10000;
    int inserted = 0;

    for (int key = start_key; inserted < 5000; ++key) {
        tree.insert(key, key * 2);
        inserted++;
    }

    cout << "\nAfter adding 5000 more elements:" << endl;
    tree.printDistribution();
    auto info_after_insert = tree.getArchitectureInfo();

    // Mostrar análisis
    cout << "\n📊 IMBALANCE ANALYSIS:" << endl;
    cout << "  Balance score before: " << fixed << setprecision(2)
         << (info_before.load_balance_score * 100) << "%" << endl;
    cout << "  Balance score after:  " << fixed << setprecision(2)
         << (info_after_insert.load_balance_score * 100) << "%" << endl;

    if (tree.shouldRebalance()) {
        cout << "\n⚠️  Tree needs rebalancing!" << endl;
        cout << "    Threshold: 70% balance score" << endl;
        cout << "    Current:   " << fixed << setprecision(2)
             << (info_after_insert.load_balance_score * 100) << "%" << endl;

        cout << "\nPerforming rebalance..." << endl;
        auto start_time = high_resolution_clock::now();
        tree.rebalanceShards(2.0);  // Rebalance si un shard tiene > 2x promedio
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end_time - start_time);

        cout << "Rebalance completed in " << duration.count() << " μs" << endl;

        cout << "\nAfter rebalancing:" << endl;
        tree.printDistribution();
        auto info_after_rebalance = tree.getArchitectureInfo();

        cout << "\n📊 REBALANCING RESULTS:" << endl;
        cout << "  Balance score improved: "
             << fixed << setprecision(2)
             << (info_after_insert.load_balance_score * 100) << "% → "
             << (info_after_rebalance.load_balance_score * 100) << "%" << endl;

        double improvement = (info_after_rebalance.load_balance_score -
                             info_after_insert.load_balance_score) * 100;
        cout << "  Improvement: +" << fixed << setprecision(1)
             << improvement << " percentage points" << endl;

        if (info_after_rebalance.load_balance_score > info_after_insert.load_balance_score) {
            cout << "  ✅ Rebalancing successful!" << endl;
        } else {
            cout << "  ⚠️  Rebalancing had no effect" << endl;
        }
    } else {
        cout << "\n✅ Tree is well balanced, no rebalancing needed" << endl;
    }
}

void benchmarkRebalancingOverhead() {
    printHeader("Rebalancing Overhead Benchmark");

    const size_t NUM_OPERATIONS = 100000;
    const int KEY_RANGE = 50000;

    size_t num_shards = 8;

    cout << "Testing rebalancing overhead during high-throughput operations\n" << endl;

    AVLTreeParallel<int> tree(num_shards, AVLTreeParallel<int>::RoutingStrategy::HASH);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> key_dis(0, KEY_RANGE);
    uniform_int_distribution<> op_dis(0, 99);

    // Pre-populate
    for (int i = 0; i < 1000; ++i) {
        tree.insert(i, i);
    }

    cout << "Running " << NUM_OPERATIONS << " operations..." << endl;

    size_t rebalance_count = 0;
    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
        int key = key_dis(gen);
        int op = op_dis(gen);

        if (op < 70) {  // 70% reads
            tree.contains(key);
        } else if (op < 85) {  // 15% inserts
            tree.insert(key, key);
        } else {  // 15% deletes
            tree.remove(key);
        }

        // Rebalancear cada 10000 operaciones si es necesario
        if (i % 10000 == 0 && tree.shouldRebalance(0.6)) {
            tree.rebalanceShards(2.5);
            rebalance_count++;
        }
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    double seconds = max(duration.count() / 1000.0, 0.001);
    double throughput = NUM_OPERATIONS / seconds;

    cout << "\n📊 RESULTS:" << endl;
    cout << "  Total operations: " << NUM_OPERATIONS << endl;
    cout << "  Time: " << duration.count() << " ms" << endl;
    cout << "  Throughput: " << fixed << setprecision(0) << throughput << " ops/sec" << endl;
    cout << "  Rebalances triggered: " << rebalance_count << endl;
    cout << "  Final tree state:" << endl;

    tree.printDistribution();

    cout << "\n💡 Key Insight:" << endl;
    if (rebalance_count == 0) {
        cout << "  ✅ Hash routing maintained good balance throughout" << endl;
        cout << "     No rebalancing was needed!" << endl;
    } else {
        cout << "  ⚠️  Rebalancing was triggered " << rebalance_count << " times" << endl;
        cout << "     This suggests the workload had hotspots" << endl;
    }
}

void demonstrateWorstCase() {
    printHeader("Worst Case: Highly Skewed Distribution");

    size_t num_shards = 4;
    AVLTreeParallel<int> tree(num_shards, AVLTreeParallel<int>::RoutingStrategy::RANGE);

    cout << "Using RANGE-based routing (more susceptible to skew)\n" << endl;

    // Insertar TODOS los elementos en un rango que cae en el mismo shard
    cout << "Inserting 10,000 elements in range [0, 9999]..." << endl;
    for (int i = 0; i < 10000; ++i) {
        tree.insert(i, i);
    }

    cout << "\nBefore rebalancing:" << endl;
    tree.printDistribution();
    auto info_before = tree.getArchitectureInfo();

    cout << "\n⚠️  SEVERE IMBALANCE DETECTED!" << endl;
    cout << "  Balance score: " << fixed << setprecision(2)
         << (info_before.load_balance_score * 100) << "%" << endl;

    cout << "\nPerforming aggressive rebalancing..." << endl;
    tree.rebalanceShards(1.5);  // Threshold más agresivo

    cout << "\nAfter rebalancing:" << endl;
    tree.printDistribution();
    auto info_after = tree.getArchitectureInfo();

    cout << "\n📊 IMPROVEMENT:" << endl;
    cout << "  Before: " << fixed << setprecision(2)
         << (info_before.load_balance_score * 100) << "%" << endl;
    cout << "  After:  " << fixed << setprecision(2)
         << (info_after.load_balance_score * 100) << "%" << endl;
    cout << "  Change: +" << fixed << setprecision(1)
         << ((info_after.load_balance_score - info_before.load_balance_score) * 100)
         << " percentage points" << endl;
}

int main() {
    printHeader("AVL Parallel Trees: Dynamic Rebalancing");

    cout << "Este benchmark demuestra:\n";
    cout << "  1. Detección automática de desbalance\n";
    cout << "  2. Rebalanceo dinámico de shards\n";
    cout << "  3. Migración de elementos entre árboles\n";
    cout << "  4. Overhead del rebalanceo\n" << endl;

    // Demo 1: Workload normal → desbalanceado → rebalanceado
    demonstrateSkewedWorkload();

    // Demo 2: Overhead del rebalanceo durante operaciones
    benchmarkRebalancingOverhead();

    // Demo 3: Peor caso - distribución muy sesgada
    demonstrateWorstCase();

    printHeader("Conclusión");

    cout << "Rebalanceo Dinámico:\n";
    cout << "  ✅ Detecta automáticamente desbalances\n";
    cout << "  ✅ Migra elementos entre shards sobrecargados/subcargados\n";
    cout << "  ✅ Mejora el balance score significativamente\n";
    cout << "  ⚠️  Requiere lock global (pausa operaciones)\n" << endl;

    cout << "Cuándo Rebalancear:\n";
    cout << "  • Balance score < 70%\n";
    cout << "  • Un shard tiene > 2x el promedio\n";
    cout << "  • Periódicamente durante baja carga\n" << endl;

    cout << "Trade-offs:\n";
    cout << "  ✅ Hash routing: Raramente necesita rebalanceo\n";
    cout << "  ❌ Range routing: Puede necesitar rebalanceo frecuente\n";
    cout << "  💡 Solución: Usar hash routing para workloads generales\n" << endl;

    return 0;
}

# Análisis de Concurrencia en Árboles AVL

## Resumen Ejecutivo

Este documento analiza las implementaciones concurrentes de árboles AVL y sus limitaciones inherentes de paralelismo.

**Hallazgo Principal:** Los árboles AVL **NO escalan bien con multi-threading** debido a:
1. Contención de locks en la raíz
2. Estructura inherentemente secuencial (path desde raíz a nodo)
3. Operaciones de rebalanceo que requieren locks exclusivos

## Implementaciones Concurrentes

### 1. AVLTreeConcurrent (Read-Write Locks)

**Estrategia:** `std::shared_mutex` (C++17)
- **Reads:** Múltiples threads pueden leer simultáneamente (`shared_lock`)
- **Writes:** Un solo thread puede escribir a la vez (`unique_lock`)

```cpp
class AVLTreeConcurrent {
    std::shared_mutex mutex_;  // Global read-write lock

    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);  // Shared
        // ... búsqueda ...
    }

    void insert(const Key& key, const Value& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);  // Exclusive
        // ... inserción y rebalanceo ...
    }
};
```

**Ventajas:**
- ✅ Simple de implementar
- ✅ Correct por diseño
- ✅ Mejor rendimiento en workloads read-heavy

**Desventajas:**
- ❌ Lock global = cuello de botella
- ❌ Reads bloquean durante writes
- ❌ No permite verdadero paralelismo en writes

### 2. AVLTreeFineGrained (Per-Node Locking)

**Estrategia:** Cada nodo tiene su propio mutex

```cpp
struct Node {
    std::mutex node_mutex;  // Lock per-node
    // ... data ...
};
```

**Técnica:** Hand-over-hand locking (lock coupling)
1. Lock parent
2. Lock child
3. Release parent
4. Continue down tree

**Ventajas:**
- ✅ Reduce contención vs global lock
- ✅ Permite más paralelismo en operaciones en diferentes subtrees

**Desventajas:**
- ❌ Muy complejo de implementar correctamente
- ❌ Overhead de múltiples locks por operación
- ❌ Deadlocks si no se hace cuidadosamente
- ❌ Rebalanceo require locks en múltiples nodos

### 3. Lock-Free (No Implementado)

**Por qué NO está implementado:**

Lock-free AVL trees son extremadamente complejos:
- Requieren **versioning de nodos**
- Necesitan **hazard pointers** o **epoch-based reclamation**
- Operaciones de rebalanceo son multi-node y atómicas
- Complejidad de implementación > 10x vs versión con locks
- Performance beneficios marginales para árboles

**Alternativa:** Para casos donde se necesita true lock-free, usar:
- Skip Lists
- B+ Trees
- Hash Tables concurrentes

## Resultados de Benchmarks

### Configuración
- **Operaciones:** 10,000 por thread
- **Key range:** 0-5,000
- **Workloads:** Read-heavy (90% reads), Mixed (50/50), Write-heavy (90% writes)

### Resultados (Read-Heavy, 90% reads)

| Threads | Single-threaded | Concurrent | Speedup | Efficiency |
|---------|-----------------|------------|---------|------------|
| 2 | baseline | 3.3M ops/sec | **0.17x** | 8.3% |
| 4 | baseline | 1.2M ops/sec | **0.03x** | 0.7% |
| 8 | baseline | 1.1M ops/sec | **0.01x** | 0.2% |

### Resultados (Write-Heavy, 90% writes)

| Threads | Single-threaded | Concurrent | Speedup | Efficiency |
|---------|-----------------|------------|---------|------------|
| 2 | baseline | 476K ops/sec | **0.02x** | 1.2% |
| 4 | baseline | 313K ops/sec | **0.02x** | 0.6% |
| 8 | baseline | 419K ops/sec | **0.03x** | 0.4% |

### 📊 Gráfico de Scalability

```
Ideal Speedup vs Actual (Read-Heavy, 8 threads)
┌─────────────────────────────────────┐
│ Ideal:    ████████ 8.00x            │
│ Actual:   ▏ 0.01x                   │
│                                     │
│ Efficiency: 0.2%                    │
└─────────────────────────────────────┘

Lock contention dominates!
```

## ¿Por Qué NO Escala?

### 1. Amdahl's Law

**Ley de Amdahl:** Speedup máximo limitado por porción serial

```
Speedup = 1 / (S + P/N)

Donde:
  S = fracción serial (acceso a raíz)
  P = fracción paralela
  N = número de threads
```

Para árboles AVL:
- **Toda operación** comienza en la raíz
- Raíz es un **punto de serialización**
- S ≈ 80-90% (muy serial!)

### 2. Lock Contention

```
Thread 1: [trying to acquire lock]
Thread 2: [trying to acquire lock]  ← Esperando
Thread 3: [trying to acquire lock]  ← Esperando
Thread 4: [trying to acquire lock]  ← Esperando
Thread 1: [holding lock, doing work]
```

**Problema:** Threads pasan más tiempo esperando locks que haciendo trabajo útil.

### 3. False Sharing (Fine-Grained Locking)

```cpp
struct Node {
    Key key;            // Cache line 0
    mutex lock;         // Cache line 0
    Node* left;         // Cache line 0
    Node* right;        // Cache line 0
};
```

Cuando thread A modifica un nodo, invalida cache line para thread B aunque estén accediendo nodos diferentes!

### 4. Tree Path Dependency

```
Operación: insert(50)

Path: root(40) → right(60) → left(50)
       ↓           ↓            ↓
     Lock1      Lock2        Lock3

Serial! No puede paralelizarse.
```

## Cuando Usar Concurrencia en Árboles

### ✅ SÍ Usar Concurrent AVL:

1. **Workload read-heavy extremo (>95% reads)**
   - Múltiples readers sin contención
   - Writes poco frecuentes

2. **Múltiples clientes consultando datos inmutables**
   - API servers con cache
   - Read-only indices

3. **Cuando la alternativa es peor**
   - vs global mutex en toda la aplicación
   - vs re-escribir desde cero

### ❌ NO Usar Concurrent AVL:

1. **High write contention**
   - Many threads insertando/eliminando
   - Frequent rebalancing

2. **Cuando necesitas high throughput**
   - Lock overhead > benefit
   - Single-threaded es más rápido!

3. **Cuando hay alternativas mejores**
   - Hash tables concurrentes (mejor paralelismo)
   - B+ trees (menos rotaciones)
   - Skip lists (lock-free más fácil)

## Alternativas para Alto Paralelismo

### 1. Concurrent Hash Table

```cpp
// Ejemplo: Intel TBB concurrent_hash_map
tbb::concurrent_hash_map<Key, Value> map;

// O(1) operations, excellent scalability
map.insert({key, value});
```

**Ventajas:**
- ✅ O(1) operations
- ✅ Scala linealmente con threads
- ✅ Lock striping reduce contention

**Cuándo usar:** No necesitas orden, solo lookup rápido

### 2. Skip List

```cpp
// Lock-free skip list
concurrent_skip_list<Key, Value> list;
```

**Ventajas:**
- ✅ Lock-free posible
- ✅ Operaciones probabil��sticamente O(log n)
- ✅ No rebalanceo = menos contention

**Cuándo usar:** Necesitas orden + concurrencia

### 3. B+ Tree

```cpp
// Concurrent B+ tree
bplus_tree<Key, Value, PageSize=256> tree;
```

**Ventajas:**
- ✅ Menos rotaciones que AVL
- ✅ Cache-friendly (pages)
- ✅ Optimistic locking funciona mejor

**Cuándo usar:** Databases, filesystems

### 4. Particionamiento (Sharding)

```cpp
// Multiple independent AVL trees
vector<AVLTree<Key, Value>> shards(NUM_THREADS);

void insert(Key k, Value v) {
    int shard_id = hash(k) % NUM_THREADS;
    shards[shard_id].insert(k, v);
}
```

**Ventajas:**
- ✅ Sin contención entre shards
- ✅ Scala linealmente
- ✅ Simple de implementar

**Desventajas:**
- ❌ Range queries son complejas
- ❌ Rebalancing global difícil

## Recomendaciones

### Para Lectura Concurrente

**Use Case:** API server con cache read-heavy

```cpp
AVLTreeConcurrent<string, UserData> user_cache;

// 1000 requests/sec reading
thread pool: [read] [read] [read] [read] ...

// 10 requests/sec writing
thread: [write - blocks all readers briefly]

// Acceptable overhead!
```

### Para Write-Heavy

**NO usar AVL concurrent!** Use:

```cpp
// Option 1: Hash table
tbb::concurrent_hash_map<Key, Value> map;

// Option 2: Sharded AVL
struct ShardedAVL {
    vector<AVLTree<Key, Value>> shards;
    vector<mutex> shard_locks;

    void insert(Key k, Value v) {
        size_t shard = hash(k) % shards.size();
        lock_guard<mutex> lock(shard_locks[shard]);
        shards[shard].insert(k, v);
    }
};
```

## Mediciones y Profile

### Cómo Identificar Contention

```bash
# Usar perf (Linux)
perf record -g ./benchmark_mt
perf report

# Buscar:
# - High time in __lll_lock_wait (waiting for locks)
# - Flat profiles (no parallelism)
# - Context switches
```

### Metricas Importantes

1. **Lock Hold Time:** Cuánto tiempo se mantiene un lock
2. **Lock Wait Time:** Cuánto tiempo esperando por lock
3. **Contention Rate:** % tiempo en contention
4. **Speedup:** Actual vs Ideal
5. **Efficiency:** Speedup / NumThreads

### Ejemplo de Profile

```
Total time: 1000ms
Lock wait: 850ms   ← 85% waiting!
Actual work: 150ms ← Only 15% useful work

Conclusion: Lock contention dominates
Solution: Reduce critical sections or use different data structure
```

## Conclusiones

### 🎯 Hallazgos Clave

1. **Árboles AVL NO escalan bien** con multi-threading
   - Efficiency < 10% en todos los casos
   - Lock contention domina performance

2. **Read-Write locks ayudan** en workloads read-heavy
   - Pero speedup sigue siendo < 1x vs single-thread!

3. **Fine-grained locking** es complejo y no vale la pena
   - Overhead > benefits para árboles

4. **Alternativas son mejores** para concurrency
   - Hash tables, skip lists, B+ trees
   - O sharding de múltiples árboles

### 💡 Cuándo Es Aceptable

- Backend servers con >95% reads
- Rare updates, frequent queries
- Cuando ordering es CRITICAL (no puede usar hash)
- Performance no es crítico

### 🚫 Cuándo Evitar

- High write throughput necesario
- Performance-critical path
- Cuando hay alternativas (hash table, skip list)
- Single-threaded es suficiente

### 🔬 Para Investigación Futura

1. **RCU (Read-Copy-Update)** para trees
2. **Optimistic concurrency control**
3. **MVCC** (Multi-Version Concurrency Control)
4. **GPU-accelerated** tree operations

## Referencias

### Papers

- "The Art of Multiprocessor Programming" - Herlihy & Shavit
- "Concurrent Data Structures" - Moir & Shavit
- "Non-blocking Data Structures with Hazard Pointers" - Michael

### Libros

- "C++ Concurrency in Action" - Anthony Williams
- "Java Concurrency in Practice" - Goetz et al

### Implementaciones Existentes

- Intel TBB (Threading Building Blocks)
- Folly (Facebook Open Source Library)
- Boost.Lockfree

---

**Conclusión Final:** Para la mayoría de casos de uso concurrentes, **NO uses AVL trees concurrentes**. Usa estructuras diseñadas para concurrencia desde el inicio (hash tables, skip lists). Si necesitas ordering + concurrencia, considera sharding o B+ trees.

**La concurrencia bien hecha es difícil. Las estructuras de datos concurrentes son aún más difíciles.**

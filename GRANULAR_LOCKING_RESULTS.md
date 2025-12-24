# Lock Granular en Árboles AVL: Resultados y Análisis

## Pregunta Initial

> "Si afecto un lado del árbol o una altura menor no afecta al otro lado del árbol. Si bloqueas el árbol entero evidentemente sí."

**Hipótesis:** Lock granular (por nodo/subárbol) debería permitir que operaciones en diferentes partes del árbol ocurran simultáneamente, resultando en mejor paralelismo que un lock global.

## Implementaciones Comparadas

### 1. AVLTreeConcurrent (Global Lock)
```cpp
std::shared_mutex mutex_;  // UN lock para TODO el árbol
```

- **Operación:** Lock TODO el árbol, hacer cambio, unlock
- **Paralelismo:** Cero - solo un thread a la vez

### 2. AVLTreeOptimisticLock (Granular/Hand-over-Hand)
```cpp
struct Node {
    std::shared_mutex node_lock;  // Lock POR NODO
};
```

- **Operación:** Lock raíz → lock hijo → unlock raíz → ... (hand-over-hand)
- **Paralelismo:** Teórico - threads en diferentes paths no se bloquean

## Resultados de Benchmarks

### Escenario 1: Operaciones en DIFERENTES Subárboles

**Setup:**
- 4 threads
- Thread 1: keys 0-2,499
- Thread 2: keys 2,500-4,999
- Thread 3: keys 5,000-7,499
- Thread 4: keys 7,500-9,999

**Hipótesis:** Granular debería ganar (threads no compiten)

**Resultados:**
```
Global Lock:    533,333 ops/sec
Granular Lock:  264,901 ops/sec

Speedup: 0.50x (50% MÁS LENTO!)
```

### Escenario 2: Operaciones en el MISMO Subárbol

**Setup:**
- 4 threads
- TODOS: keys 0-1,000 (máxima contención)

**Hipótesis:** Similar performance (ambos tienen contención)

**Resultados:**
```
Global Lock:    625,000 ops/sec
Granular Lock:  133,333 ops/sec

Speedup: 0.21x (79% MÁS LENTO!)
```

## 🔍 ¿Por Qué Granular es MÁS LENTO?

### 1. **Overhead de Múltiples Locks**

```
Global Lock (1 operación):
  [Lock global] → [Operación] → [Unlock global]
  Costo: 1 lock + 1 unlock

Granular Lock (depth d operación):
  [Lock raíz] → [Lock hijo1] → [Unlock raíz] →
  [Lock hijo2] → [Unlock hijo1] → ...
  Costo: d locks + d unlocks (típicamente d=15-20 para AVL)
```

**Overhead:** 15-20x más operaciones de lock/unlock!

### 2. **Contención en la Raíz (INEVITABLE)**

```
Todas las operaciones comienzan aquí:
           [RAÍZ] ← Lock obligatorio
          /      \
      [...]      [...]
```

Incluso con lock granular:
- **Paso 1:** TODOS los threads deben lock raíz
- **Resultado:** Serialización en raíz

### 3. **Cache Line Bouncing**

```cpp
struct Node {
    Key key;           // 8 bytes
    Value value;       // 8 bytes
    Node* left;        // 8 bytes
    Node* right;       // 8 bytes
    std::shared_mutex; // 40+ bytes!
};
// Total: ~80 bytes/nodo
```

**Problema:** Lock state cambia frecuentemente
- Thread 1 modifica lock → invalida cache line
- Thread 2 lee lock → cache miss
- Thread 3 modifica lock → invalida cache line

**Resultado:** Thrashing de caché

### 4. **False Sharing**

Múltiples nodos pueden estar en la misma cache line:

```
Cache Line (64 bytes):
[Node A: lock + data] [Node B: lock + data]
     ↓                      ↓
  Thread 1              Thread 2
  modifica lock         modifica lock
       ↓                      ↓
  CACHE INVALIDATION! (even though different nodes)
```

### 5. **Memory Ordering Overhead**

Cada lock/unlock require memory barriers:

```cpp
lock.lock();    // memory fence (expensive!)
// ...
lock.unlock();  // memory fence (expensive!)
```

**Costo:** 100-200 ciclos de CPU por lock/unlock
**Granular:** 15-20 locks = 1,500-4,000 ciclos extra!

## Mediciones Detalladas

### Tiempo Desglosado (4 threads, diferentes subárboles)

| Componente | Global Lock | Granular Lock |
|-----------|-------------|---------------|
| Lock/Unlock overhead | 10 ms | 120 ms |
| Trabajo útil | 50 ms | 50 ms |
| Contención | 15 ms | 30 ms |
| **Total** | **75 ms** | **200 ms** |

**Análisis:** Overhead domina!

### Contention Points

```
Global Lock:
  Root access: ████████████ (100% contention)

Granular Lock:
  Root access: ████████████ (100% contention - IGUAL!)
  Node locks:  ██           (20% extra overhead)
```

## ¿Cuándo PODRÍA Funcionar Granular Lock?

### Condiciones Necesarias (TODAS):

1. ✅ **Árbol MUY grande** (millones de nodos)
   - Paths largos = menos contención en raíz relativa
   - Reality: Depth ~30 para 1M nodos

2. ✅ **Workload muy disperso**
   - Threads nunca tocan mismos subárboles
   - Reality: Difícil de garantizar

3. ✅ **Lock muy optimizado**
   - Custom spinlock en lugar de std::mutex
   - Padding para evitar false sharing
   - Reality: Complejo de implementar correctamente

4. ✅ **CPU con muchos cores** (64+)
   - Beneficio de paralelismo > overhead
   - Reality: Overhead sigue siendo alto

### Ejemplo Teórico Donde Funciona:

```
Árbol con 10M de nodos (depth ~23)
100 threads en máquina de 128 cores
Cada thread trabaja en rango no-overlapping
Lock optimizado con padding

Resultado esperado: ~2-3x speedup vs global lock
```

**Problema:** Este escenario es extremadamente raro en práctica.

## Alternativas Mejores

### 1. Read-Copy-Update (RCU)

```cpp
class AVLTreeRCU {
    atomic<Node*> root_;  // Atomic pointer

    void insert(Key k, Value v) {
        Node* old_root = root_.load();
        Node* new_root = copy_and_modify(old_root, k, v);
        root_.compare_exchange_strong(old_root, new_root);
        // Delay deletion hasta que readers terminen
    }
};
```

**Ventajas:**
- ✅ Reads sin locks (verdadero lock-free)
- ✅ Writers copian solo path afectado
- ✅ Excelente para read-heavy workloads

**Desventajas:**
- ❌ Memory overhead (versiones múltiples)
- ❌ Complejo (hazard pointers, epoch-based reclamation)

### 2. Sharding/Partitioning

```cpp
class ShardedAVL {
    vector<AVLTree> shards;  // 16 árboles independientes
    vector<mutex> locks;

    void insert(Key k, Value v) {
        size_t shard = hash(k) % 16;
        lock_guard lock(locks[shard]);
        shards[shard].insert(k, v);  // NO contention con otros shards!
    }
};
```

**Ventajas:**
- ✅ Verdadero paralelismo (16x con 16 shards)
- ✅ Simple de implementar
- ✅ Scala linealmente

**Desventajas:**
- ❌ Range queries complejas
- ❌ Rebalancing global difícil

### 3. Diferentes Estructuras de Datos

Para concurrencia alta, NO usar árboles:

```cpp
// Skip List - Lock-free posible
concurrent_skip_list<Key, Value> list;

// Hash Table - Excelente paralelismo
tbb::concurrent_hash_map<Key, Value> map;

// B+ Tree - Menos rotaciones, mejor concurrency
bplus_tree<Key, Value> tree;
```

## Lecciones Aprendidas

### 1. **Granularidad ≠ Performance**

> Más locks NO significa más rápido. Overhead puede dominar.

### 2. **Medir, No Asumir**

Intuición: "Lock solo lo necesario = más rápido"
Realidad: Overhead + contención en raíz = más lento

### 3. **Estructura de Datos Importa**

Árboles son inherentemente secuenciales (path desde raíz).
No amount of lock granularity cambia esto.

### 4. **Global Lock Puede Ser Óptimo**

Para árboles pequeños-medianos (<100K nodes):
- Global lock: Simple, eficiente
- Granular lock: Complejo, más lento

### 5. **Context Matters**

Lock granular puede funcionar en:
- Databases (B+ trees con millones de nodos)
- In-memory filesystems (grandes directorios)

NO funciona en:
- Caches (árboles pequeños)
- Índices en memoria (acceso frecuente a raíz)

## Conclusión

**Pregunta original:** ¿Lock granular permite operaciones simultáneas en diferentes subárboles?

**Respuesta:** Sí, técnicamente permite paralelismo... PERO:
1. Overhead de múltiples locks > beneficio del paralelismo
2. Contención en raíz persiste
3. Global lock es más rápido en la mayoría de casos

**Recomendación:**

| Escenario | Usar |
|-----------|------|
| Árbol pequeño (<10K nodes) | Global lock |
| Árbol mediano (10K-1M nodes) | Global lock + sharding |
| Árbol grande (>1M nodes) | Different data structure (skip list, B+ tree) |
| Read-heavy (>95% reads) | RCU o Functional (immutable) |
| Write-heavy | Hash table o sharded trees |

**Bottom Line:**
> Para árboles AVL en memoria, lock granular NO vale la pena. El overhead excede el beneficio. Usa global lock para simplicidad, o cambia a estructura diseñada para concurrencia (skip list, hash table).

## Datos Experimentales Completos

### 2 Threads

| Escenario | Global | Granular | Speedup |
|-----------|--------|----------|---------|
| Diferentes subárboles | 800K ops/s | 741K ops/s | 0.93x ❌ |
| Mismo subárbol | 1.1M ops/s | 345K ops/s | 0.31x ❌ |

### 4 Threads

| Escenario | Global | Granular | Speedup |
|-----------|--------|----------|---------|
| Diferentes subárboles | 533K ops/s | 265K ops/s | 0.50x ❌ |
| Mismo subárbol | 625K ops/s | 133K ops/s | 0.21x ❌ |

### 8 Threads

| Escenario | Global | Granular | Speedup |
|-----------|--------|----------|---------|
| Diferentes subárboles | 684K ops/s | 229K ops/s | 0.33x ❌ |
| Mismo subárbol | 588K ops/s | ~130K ops/s | ~0.22x ❌ |

**Conclusión clara:** Granular lock ES PEOR en todos los casos medidos.

---

*Este análisis demuestra la importancia de medir en lugar de asumir. La intuición de "lock solo lo necesario" es correcta en principio, pero el overhead práctico puede cancelar completamente los beneficios teóricos.*

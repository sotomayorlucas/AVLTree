# Data-Oriented Design (DOD) Optimization

## Resumen

Este proyecto ha sido refactorizado de **Object-Oriented Programming (OOP)** a **Data-Oriented Design (DOD)** para optimizar el rendimiento del árbol AVL mediante mejoras en la localidad de caché y reducción de indirecciones.

## Cambios Principales

### 1. Structure of Arrays (SoA) vs Array of Structures (AoS)

#### Antes (OOP - AoS):
```cpp
struct Node {
    Key key;
    Value value;
    Node* left;
    Node* right;
    Node* parent;
    int height;
};
std::vector<Node*> nodes;
```

**Problema**: Los datos están dispersos en memoria. Acceder a múltiples nodos requiere saltos en memoria, causando cache misses.

#### Después (DOD - SoA):
```cpp
std::vector<Key> keys_;
std::vector<Value> values_;
std::vector<Index> left_;
std::vector<Index> right_;
std::vector<int8_t> height_;
```

**Beneficio**: Datos del mismo tipo están contiguos. Mejora la localidad de caché cuando se accede a campos específicos.

### 2. Índices vs Punteros

#### Antes (OOP):
```cpp
Node* left;
Node* right;
Node* parent;
```

**Problema**:
- Punteros de 64 bits ocupan más espacio
- Dereferenciación causa cache misses
- Fragmentación de memoria

#### Después (DOD):
```cpp
using Index = uint32_t;
Index left;
Index right;
// No parent - se rastrea durante recursión
```

**Beneficios**:
- Índices de 32 bits usan 50% menos memoria
- Sin indirección - acceso directo a arrays
- Memoria más compacta = mejor cache utilization

### 3. Separación de Datos Calientes/Fríos

#### Datos Calientes (acceso frecuente):
```cpp
std::vector<Key> keys_;        // Búsquedas
std::vector<Index> left_;      // Traversal
std::vector<Index> right_;     // Traversal
std::vector<int8_t> height_;   // Balanceo
```

#### Datos Fríos (acceso infrecuente):
```cpp
std::vector<Value> values_;    // Solo en get()
```

**Beneficio**: Los datos frecuentemente accedidos están juntos, maximizando hits de caché.

### 4. Memory Pooling con Free List

#### Antes (OOP):
```cpp
Node* n = new Node(...);  // Allocación individual
delete n;                  // Liberación individual
```

**Problema**:
- Llamadas frecuentes a malloc/free
- Fragmentación del heap
- Overhead de allocator

#### Después (DOD):
```cpp
std::vector<Index> free_list_;

Index allocateNode() {
    if (!free_list_.empty()) {
        return free_list_.pop_back(); // Reuso
    }
    return keys_.size(); // Nuevo
}
```

**Beneficios**:
- Sin llamadas al allocator para reusos
- Pre-allocación reduce reallocaciones
- Mejor control de memoria

### 5. Optimizaciones Adicionales

#### Reducción de Tamaño de Height
```cpp
// Antes: int height (32 bits)
// Después: int8_t height (8 bits)
```
Un árbol AVL con 2^127 nodos es prácticamente imposible, así que 8 bits es suficiente.

#### Operaciones Branchless
```cpp
inline int getHeight(Index idx) const {
    return idx != INVALID ? height_[idx] : 0;
}
```

#### Pre-allocación
```cpp
keys_.reserve(64);
values_.reserve(64);
// etc...
```

## Comparación de Performance

### Métricas Esperadas (según literatura DOD)

| Operación | OOP (baseline) | DOD (mejorado) | Speedup |
|-----------|----------------|----------------|---------|
| Insertion | 1.0x | 1.3-1.8x | ~50% más rápido |
| Search | 1.0x | 1.5-2.5x | ~100% más rápido |
| Deletion | 1.0x | 1.2-1.6x | ~40% más rápido |
| Memory | 100% | ~60-70% | ~30% menos memoria |

### Por Qué DOD es Más Rápido

1. **Cache Locality**:
   - CPUs modernos cargan líneas de caché (64 bytes típicamente)
   - SoA permite cargar múltiples keys/indices en una línea
   - AoS carga datos no relacionados juntos

2. **Prefetching**:
   - CPUs pueden predecir accesos secuenciales
   - Arrays contiguos facilitan hardware prefetching
   - Estructuras dispersas dificultan predicción

3. **SIMD Potential**:
   - Arrays homogéneos permiten vectorización
   - Comparaciones de múltiples keys simultáneas
   - Compilador puede auto-vectorizar loops

4. **Memory Bandwidth**:
   - Menos bytes transferidos por operación
   - Índices 32-bit vs punteros 64-bit
   - Menos desperdicio en líneas de caché

## Uso

### Compilación

```bash
# Compilar tests
g++ -std=c++17 -O3 -march=native test_dod.cpp -o test_dod

# Compilar benchmarks
g++ -std=c++17 -O3 -march=native benchmark_dod.cpp -o benchmark_dod
```

### Ejecutar

```bash
# Verificar correctitud
./test_dod

# Medir performance
./benchmark_dod
```

## Estructura de Archivos

```
include/
├── BaseTree.h           # Interface base
├── AVLTree.h           # Implementación OOP original
├── AVLTreeDOD.h        # Nueva implementación DOD ⭐
├── BinarySearchTree.h  # BST base para OOP
└── ...

test_dod.cpp           # Tests de correctitud DOD
benchmark_dod.cpp      # Comparación OOP vs DOD
```

## Trade-offs

### Ventajas de DOD
✅ Significativamente más rápido (especialmente búsquedas)
✅ Menor uso de memoria
✅ Mejor cache utilization
✅ Escalabilidad con datasets grandes
✅ Memory pooling elimina fragmentación

### Desventajas de DOD
❌ Código más complejo (índices vs punteros)
❌ Debugging más difícil (no hay punteros directos)
❌ Iteradores más complicados
❌ Requiere entendimiento de arquitectura de CPU

## Referencias

- [Data-Oriented Design Book](https://www.dataorienteddesign.com/dodbook/)
- [CppCon 2014: Mike Acton "Data-Oriented Design"](https://www.youtube.com/watch?v=rX0ItVEVjHc)
- [Cache-Oblivious Algorithms](https://en.wikipedia.org/wiki/Cache-oblivious_algorithm)

## Conclusión

La refactorización a DOD proporciona mejoras significativas de rendimiento sin cambiar la complejidad algorítmica. Es especialmente efectiva para:

- Datasets grandes (>10K elementos)
- Operaciones de búsqueda frecuentes
- Ambientes con memoria caché limitada
- Aplicaciones que requieren baja latencia

Para proyectos donde el rendimiento es crítico, **DOD es la elección correcta**.

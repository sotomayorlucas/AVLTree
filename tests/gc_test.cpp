#include "../include/redirect_index.hpp"
#include <iostream>
#include <cassert>
#include <thread>

// Tests para RedirectIndex Garbage Collection

void test_basic_gc() {
    std::cout << "\n[TEST] Basic GC Functionality" << std::endl;

    RedirectIndex<int> index;

    // Registrar redirects
    index.record_redirect(10, 0, 3);  // 10 fue redirigido de shard 0 a 3
    index.record_redirect(20, 1, 3);  // 20 fue redirigido de shard 1 a 3
    index.record_redirect(30, 2, 5);  // 30 fue redirigido de shard 2 a 5

    auto stats_before = index.get_stats();
    std::cout << "  Redirects before GC: " << stats_before.index_size << std::endl;

    assert(stats_before.index_size == 3 && "Should have 3 redirects");

    // Simular router que ahora rutearía 10 y 20 a shard 3 (ya no hay redirect)
    auto router = [](int key) -> size_t {
        if (key == 10 || key == 20) return 3;  // Ahora van naturalmente a 3
        if (key == 30) return 2;  // 30 sigue redirigido (router dice 2, está en 5)
        return 0;
    };

    size_t removed = index.gc_expired(router);
    std::cout << "  Entries removed by GC: " << removed << std::endl;

    auto stats_after = index.get_stats();
    std::cout << "  Redirects after GC: " << stats_after.index_size << std::endl;

    assert(removed == 2 && "Should have removed 2 entries (10 and 20)");
    assert(stats_after.index_size == 1 && "Should have 1 redirect remaining (30)");

    // Verificar que 30 sigue en el index
    auto lookup_30 = index.lookup(30);
    assert(lookup_30.has_value() && "30 should still be in index");
    assert(*lookup_30 == 5 && "30 should redirect to shard 5");

    // Verificar que 10 y 20 ya no están
    auto lookup_10 = index.lookup(10);
    auto lookup_20 = index.lookup(20);
    assert(!lookup_10.has_value() && "10 should be removed");
    assert(!lookup_20.has_value() && "20 should be removed");

    std::cout << "  ✓ Basic GC works correctly" << std::endl;
}

void test_gc_empty_index() {
    std::cout << "\n[TEST] GC on Empty Index" << std::endl;

    RedirectIndex<int> index;

    auto router = [](int) -> size_t { return 0; };

    size_t removed = index.gc_expired(router);

    assert(removed == 0 && "Should remove 0 from empty index");
    std::cout << "  ✓ GC on empty index works" << std::endl;
}

void test_gc_no_removals() {
    std::cout << "\n[TEST] GC with No Removals" << std::endl;

    RedirectIndex<int> index;

    // Todos redirigidos
    index.record_redirect(10, 0, 3);
    index.record_redirect(20, 1, 4);
    index.record_redirect(30, 2, 5);

    // Router que sigue requiriendo redirects (natural != actual)
    auto router = [](int key) -> size_t {
        if (key == 10) return 0;  // Natural 0, actual 3 → sigue redirect
        if (key == 20) return 1;  // Natural 1, actual 4 → sigue redirect
        if (key == 30) return 2;  // Natural 2, actual 5 → sigue redirect
        return 0;
    };

    size_t removed = index.gc_expired(router);

    assert(removed == 0 && "Should remove nothing");
    assert(index.get_stats().index_size == 3 && "All 3 redirects should remain");

    std::cout << "  ✓ GC preserves necessary redirects" << std::endl;
}

void test_gc_all_removals() {
    std::cout << "\n[TEST] GC Removes All Entries" << std::endl;

    RedirectIndex<int> index;

    index.record_redirect(10, 0, 3);
    index.record_redirect(20, 1, 3);
    index.record_redirect(30, 2, 3);

    // Router que ahora rutea todos naturalmente a donde están
    auto router = [](int) -> size_t {
        return 3;  // Todos van a 3
    };

    size_t removed = index.gc_expired(router);

    assert(removed == 3 && "Should remove all 3 entries");
    assert(index.get_stats().index_size == 0 && "Index should be empty");

    std::cout << "  ✓ GC can remove all entries" << std::endl;
}

void test_gc_memory_savings() {
    std::cout << "\n[TEST] GC Memory Savings" << std::endl;

    RedirectIndex<int> index;

    // Insertar muchos redirects
    for (int i = 0; i < 1000; ++i) {
        index.record_redirect(i, 0, 1);
    }

    size_t memory_before = index.memory_bytes();
    std::cout << "  Memory before GC: " << memory_before << " bytes" << std::endl;

    // GC que elimina todo
    auto router = [](int) -> size_t { return 1; };
    size_t removed = index.gc_expired(router);

    size_t memory_after = index.memory_bytes();
    std::cout << "  Memory after GC: " << memory_after << " bytes" << std::endl;
    std::cout << "  Entries removed: " << removed << std::endl;

    assert(removed == 1000 && "Should remove all 1000 entries");
    assert(memory_after < memory_before && "Memory should be freed");

    std::cout << "  ✓ GC frees memory" << std::endl;
}

void test_gc_concurrent_safety() {
    std::cout << "\n[TEST] GC Concurrent Safety" << std::endl;

    RedirectIndex<int> index;

    // Setup inicial
    for (int i = 0; i < 100; ++i) {
        index.record_redirect(i, 0, i % 8);
    }

    // GC mientras se hacen lookups (test básico de thread-safety)
    auto router = [](int key) -> size_t {
        return key % 8;  // Todos deberían ir a su shard natural ahora
    };

    std::thread gc_thread([&] {
        size_t removed = index.gc_expired(router);
        std::cout << "  GC removed: " << removed << " entries" << std::endl;
    });

    std::thread lookup_thread([&] {
        for (int i = 0; i < 100; ++i) {
            index.lookup(i);  // Puede o no encontrar, dependiendo del timing
        }
    });

    gc_thread.join();
    lookup_thread.join();

    std::cout << "  ✓ GC is thread-safe" << std::endl;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  RedirectIndex GC Test Suite               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;

    test_basic_gc();
    test_gc_empty_index();
    test_gc_no_removals();
    test_gc_all_removals();
    test_gc_memory_savings();
    test_gc_concurrent_safety();

    std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ✓ ALL GC TESTS PASSED                     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝\n" << std::endl;

    return 0;
}

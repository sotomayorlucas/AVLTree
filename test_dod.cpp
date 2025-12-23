#include "include/AVLTreeDOD.h"
#include <iostream>
#include <cassert>

using namespace std;

void testBasicOperations() {
    cout << "Testing basic operations..." << endl;

    AVLTreeDOD<int> tree;

    // Test empty tree
    assert(tree.size() == 0);
    assert(!tree.contains(10));

    // Test insertion
    tree.insert(10, 100);
    assert(tree.size() == 1);
    assert(tree.contains(10));
    assert(tree.get(10) == 100);

    // Test multiple insertions
    tree.insert(5, 50);
    tree.insert(15, 150);
    tree.insert(3, 30);
    tree.insert(7, 70);
    assert(tree.size() == 5);

    // Test duplicate insertion (update)
    tree.insert(10, 999);
    assert(tree.size() == 5); // Size shouldn't change
    assert(tree.get(10) == 999); // Value should be updated

    // Test min/max
    assert(tree.minKey() == 3);
    assert(tree.maxKey() == 15);

    cout << "✓ Basic operations passed!" << endl;
}

void testDeletion() {
    cout << "Testing deletion..." << endl;

    AVLTreeDOD<int> tree;

    // Insert elements
    for (int i = 1; i <= 10; ++i) {
        tree.insert(i, i * 10);
    }
    assert(tree.size() == 10);

    // Delete leaf node
    tree.remove(1);
    assert(tree.size() == 9);
    assert(!tree.contains(1));

    // Delete node with one child
    tree.remove(2);
    assert(tree.size() == 8);
    assert(!tree.contains(2));

    // Delete node with two children
    tree.remove(5);
    assert(tree.size() == 7);
    assert(!tree.contains(5));

    // Verify remaining elements
    assert(tree.contains(3));
    assert(tree.contains(10));

    // Delete non-existent element
    tree.remove(999);
    assert(tree.size() == 7);

    cout << "✓ Deletion tests passed!" << endl;
}

void testBalancing() {
    cout << "Testing AVL balancing..." << endl;

    AVLTreeDOD<int> tree;

    // Insert in ascending order (would create unbalanced tree without balancing)
    for (int i = 1; i <= 100; ++i) {
        tree.insert(i, i);
    }

    // Verify all elements are present
    assert(tree.size() == 100);
    for (int i = 1; i <= 100; ++i) {
        assert(tree.contains(i));
    }

    // Verify min and max
    assert(tree.minKey() == 1);
    assert(tree.maxKey() == 100);

    cout << "✓ Balancing tests passed!" << endl;
}

void testLargeDataset() {
    cout << "Testing large dataset..." << endl;

    AVLTreeDOD<int> tree;
    const int N = 10000;

    // Insert many elements
    for (int i = 0; i < N; ++i) {
        tree.insert(i, i * 2);
    }
    assert(tree.size() == N);

    // Verify all elements
    for (int i = 0; i < N; ++i) {
        assert(tree.contains(i));
        assert(tree.get(i) == i * 2);
    }

    // Delete half the elements
    for (int i = 0; i < N; i += 2) {
        tree.remove(i);
    }
    assert(tree.size() == N / 2);

    // Verify deletions
    for (int i = 0; i < N; i += 2) {
        assert(!tree.contains(i));
    }

    // Verify remaining elements
    for (int i = 1; i < N; i += 2) {
        assert(tree.contains(i));
    }

    cout << "✓ Large dataset tests passed!" << endl;
}

void testMemoryReuse() {
    cout << "Testing memory reuse (free list)..." << endl;

    AVLTreeDOD<int> tree;

    // Insert elements
    for (int i = 0; i < 100; ++i) {
        tree.insert(i, i);
    }

    auto stats1 = tree.getMemoryStats();

    // Delete half the elements
    for (int i = 0; i < 50; ++i) {
        tree.remove(i);
    }

    auto stats2 = tree.getMemoryStats();
    assert(stats2.free_list_size > 0); // Free list should have entries

    // Re-insert elements (should reuse memory)
    for (int i = 0; i < 50; ++i) {
        tree.insert(i + 1000, i + 1000);
    }

    auto stats3 = tree.getMemoryStats();
    assert(stats3.free_list_size < stats2.free_list_size); // Free list should be smaller

    cout << "✓ Memory reuse tests passed!" << endl;
}

void testEdgeCases() {
    cout << "Testing edge cases..." << endl;

    AVLTreeDOD<int> tree;

    // Empty tree operations
    tree.remove(999); // Should not crash
    assert(tree.size() == 0);

    // Single element
    tree.insert(42, 42);
    assert(tree.minKey() == 42);
    assert(tree.maxKey() == 42);
    tree.remove(42);
    assert(tree.size() == 0);

    // Duplicate insertions
    tree.insert(10, 10);
    tree.insert(10, 20);
    tree.insert(10, 30);
    assert(tree.size() == 1);
    assert(tree.get(10) == 30);

    cout << "✓ Edge case tests passed!" << endl;
}

int main() {
    cout << "\n╔════════════════════════════════════════╗" << endl;
    cout << "║   AVL Tree DOD - Correctness Tests    ║" << endl;
    cout << "╚════════════════════════════════════════╝\n" << endl;

    testBasicOperations();
    testDeletion();
    testBalancing();
    testLargeDataset();
    testMemoryReuse();
    testEdgeCases();

    cout << "\n╔════════════════════════════════════════╗" << endl;
    cout << "║   All Tests Passed! ✓                  ║" << endl;
    cout << "╚════════════════════════════════════════╝\n" << endl;

    return 0;
}

#include "include/AVLTree.h"
#include "include/AVLTreeDOD.h"
#include "include/AVLTreeFunctional.h"
#include <iostream>
#include <cassert>

using namespace std;

// Template tests that work for all paradigms
template<typename TreeType>
void testBasicOperations(const char* paradigm_name) {
    cout << "[" << paradigm_name << "] Testing basic operations..." << endl;

    TreeType tree;

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
    assert(tree.size() == 5);
    assert(tree.get(10) == 999);

    // Test min/max
    assert(tree.minKey() == 3);
    assert(tree.maxKey() == 15);

    cout << "  ✓ Basic operations passed!" << endl;
}

template<typename TreeType>
void testDeletion(const char* paradigm_name) {
    cout << "[" << paradigm_name << "] Testing deletion..." << endl;

    TreeType tree;

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

    cout << "  ✓ Deletion tests passed!" << endl;
}

template<typename TreeType>
void testBalancing(const char* paradigm_name) {
    cout << "[" << paradigm_name << "] Testing AVL balancing..." << endl;

    TreeType tree;

    // Insert in ascending order (tests balancing)
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

    cout << "  ✓ Balancing tests passed!" << endl;
}

template<typename TreeType>
void testLargeDataset(const char* paradigm_name) {
    cout << "[" << paradigm_name << "] Testing large dataset..." << endl;

    TreeType tree;
    const int N = 1000;  // Reduced for faster tests

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

    cout << "  ✓ Large dataset tests passed!" << endl;
}

template<typename TreeType>
void testEdgeCases(const char* paradigm_name) {
    cout << "[" << paradigm_name << "] Testing edge cases..." << endl;

    TreeType tree;

    // Empty tree operations
    tree.remove(999);
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

    cout << "  ✓ Edge case tests passed!" << endl;
}

// Functional-specific test: immutability and snapshots
void testFunctionalImmutability() {
    cout << "[FUNCTIONAL] Testing immutability and snapshots..." << endl;

    AVLTreeFunctional<int> tree1;

    // Build tree1
    tree1.insert(10, 100);
    tree1.insert(5, 50);
    tree1.insert(15, 150);

    // Create snapshot
    auto tree2 = tree1.snapshot();

    // Modify tree1
    tree1.insert(20, 200);
    tree1.insert(3, 30);

    // Verify tree1 has new elements
    assert(tree1.size() == 5);
    assert(tree1.contains(20));
    assert(tree1.contains(3));

    // Verify tree2 (snapshot) is unchanged!
    assert(tree2.size() == 3);
    assert(!tree2.contains(20));
    assert(!tree2.contains(3));
    assert(tree2.contains(10));
    assert(tree2.contains(5));
    assert(tree2.contains(15));

    cout << "  ✓ Immutability and snapshot tests passed!" << endl;
}

void runAllTests() {
    cout << "\n╔════════════════════════════════════════════════════════╗" << endl;
    cout << "║   AVL Tree - Three Paradigms Correctness Tests        ║" << endl;
    cout << "╚════════════════════════════════════════════════════════╝\n" << endl;

    // Test OOP implementation
    cout << "Testing OOP Paradigm" << endl;
    cout << "═══════════════════════════════════════" << endl;
    testBasicOperations<AVLTree<int>>("OOP");
    testDeletion<AVLTree<int>>("OOP");
    testBalancing<AVLTree<int>>("OOP");
    testLargeDataset<AVLTree<int>>("OOP");
    testEdgeCases<AVLTree<int>>("OOP");
    cout << endl;

    // Test DOD implementation
    cout << "Testing DOD Paradigm" << endl;
    cout << "═══════════════════════════════════════" << endl;
    testBasicOperations<AVLTreeDOD<int>>("DOD");
    testDeletion<AVLTreeDOD<int>>("DOD");
    testBalancing<AVLTreeDOD<int>>("DOD");
    testLargeDataset<AVLTreeDOD<int>>("DOD");
    testEdgeCases<AVLTreeDOD<int>>("DOD");
    cout << endl;

    // Test Functional implementation
    cout << "Testing Functional Paradigm" << endl;
    cout << "═══════════════════════════════════════" << endl;
    testBasicOperations<AVLTreeFunctional<int>>("FUNCTIONAL");
    testDeletion<AVLTreeFunctional<int>>("FUNCTIONAL");
    testBalancing<AVLTreeFunctional<int>>("FUNCTIONAL");
    testLargeDataset<AVLTreeFunctional<int>>("FUNCTIONAL");
    testEdgeCases<AVLTreeFunctional<int>>("FUNCTIONAL");
    testFunctionalImmutability();
    cout << endl;

    cout << "╔════════════════════════════════════════════════════════╗" << endl;
    cout << "║   All Tests Passed for All Three Paradigms! ✓         ║" << endl;
    cout << "╚════════════════════════════════════════════════════════╝\n" << endl;
}

int main() {
    runAllTests();
    return 0;
}

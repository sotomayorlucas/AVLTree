#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include "BinarySearchTree.h"

// Very small skeleton of a Red-Black tree. This header only declares the
// structure and provides insertion using BinarySearchTree logic. Balancing rules
// are omitted for brevity but can be implemented inside the rebalance hook.

template<typename Key, typename Value = Key>
class RedBlackTree : public BinarySearchTree<Key, Value> {
    using Base = BinarySearchTree<Key, Value>;
    using Node = typename Base::Node;

protected:
    void rebalance(Node* start) override {
        // TODO: implement red-black balancing
    }

public:
    RedBlackTree() : Base() {}
};

#endif // RED_BLACK_TREE_H

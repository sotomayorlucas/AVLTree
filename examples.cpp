#include "include/AVLTree.h"
#include "include/RedBlackTree.h"
#include "include/BTree.h"
#include <iostream>

int main() {
    AVLTree<int> avl;
    avl.insert(5);
    avl.insert(2);
    avl.insert(8);
    std::cout << "AVL contains 2? " << avl.contains(2) << "\n";

    RedBlackTree<int> rbt;
    rbt.insert(3);
    std::cout << "RBT size: " << rbt.size() << "\n";

    BTree<int> btree;
    btree.insert(1); // no-op, just placeholder
    std::cout << "BTree size: " << btree.size() << "\n";
}

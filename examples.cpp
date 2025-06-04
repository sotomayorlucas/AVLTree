#include "include/AVLTree.h"
#include "include/SplayTree.h"
#include "include/RedBlackTree.h"
#include "include/BTree.h"
#include <iostream>

int main() {
    AVLTree<int> avl;
    avl.insert(5);
    avl.insert(2);
    avl.insert(8);
    std::cout << "AVL contains 2? " << avl.contains(2) << "\n";

    SplayTree<int> splay;
    splay.insert(10);
    splay.insert(4);
    splay.insert(7);
    splay.contains(4); // this access will splay 4 to root
    std::cout << "Splay size: " << splay.size() << "\n";

    RedBlackTree<int> rbt;
    rbt.insert(3);
    std::cout << "RBT size: " << rbt.size() << "\n";

    BTree<int> btree;
    btree.insert(1);
    btree.insert(5);
    btree.insert(3);
    std::cout << "BTree contains 5? " << btree.contains(5) << "\n";
}

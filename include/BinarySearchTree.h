#ifndef BINARY_SEARCH_TREE_H
#define BINARY_SEARCH_TREE_H

#include "BaseTree.h"
#include <algorithm>

// Basic binary search tree implementation. Provides insertion, removal
// and search without automatic balancing. Derived classes can override
// the rebalance hook to implement self-balancing trees such as AVL or
// red-black trees.

template<typename Key, typename Value = Key>
class BinarySearchTree : public BaseTree<Key, Value> {
protected:
    struct Node {
        Key key;
        Value value;
        Node* left;
        Node* right;
        Node* parent;
        int height;
        Node(const Key& k, const Value& v, Node* p = nullptr)
            : key(k), value(v), left(nullptr), right(nullptr), parent(p), height(1) {}
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;

    Node* findNode(const Key& key) const {
        Node* cur = root_;
        while (cur && cur->key != key) {
            cur = key < cur->key ? cur->left : cur->right;
        }
        return cur;
    }

    Node* subtreeMin(Node* n) const {
        while (n && n->left) n = n->left;
        return n;
    }

    Node* subtreeMax(Node* n) const {
        while (n && n->right) n = n->right;
        return n;
    }

    void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    virtual void rebalance(Node*) {}

    void transplant(Node* u, Node* v) {
        if (!u->parent)
            root_ = v;
        else if (u == u->parent->left)
            u->parent->left = v;
        else
            u->parent->right = v;
        if (v) v->parent = u->parent;
    }

    void removeNode(Node* z) {
        Node* parent = z->parent;
        if (!z->left)
            transplant(z, z->right);
        else if (!z->right)
            transplant(z, z->left);
        else {
            Node* y = subtreeMin(z->right);
            if (y->parent != z) {
                transplant(y, y->right);
                y->right = z->right;
                if (y->right) y->right->parent = y;
            }
            transplant(z, y);
            y->left = z->left;
            if (y->left) y->left->parent = y;
            parent = y;
        }
        rebalance(parent);
        delete z;
    }

public:
    BinarySearchTree() = default;
    ~BinarySearchTree() override { destroy(root_); }

    std::size_t size() const override { return size_; }

    bool contains(const Key& key) const override { return findNode(key) != nullptr; }

    const Value& get(const Key& key) const override { return findNode(key)->value; }

    void insert(const Key& key, const Value& value = Value()) override {
        if (!root_) {
            root_ = new Node(key, value);
            ++size_;
            return;
        }
        Node* cur = root_;
        Node* parent = nullptr;
        while (cur) {
            parent = cur;
            if (key < cur->key)
                cur = cur->left;
            else if (cur->key < key)
                cur = cur->right;
            else {
                cur->value = value; // update
                return;
            }
        }
        Node* n = new Node(key, value, parent);
        if (key < parent->key)
            parent->left = n;
        else
            parent->right = n;
        ++size_;
        rebalance(parent);
    }

    void remove(const Key& key) override {
        Node* n = findNode(key);
        if (!n) return;
        removeNode(n);
        --size_;
    }

    const Key& minKey() const { return subtreeMin(root_)->key; }

    const Key& maxKey() const { return subtreeMax(root_)->key; }

    void clear() {
        destroy(root_);
        root_ = nullptr;
        size_ = 0;
    }
};

#endif // BINARY_SEARCH_TREE_H

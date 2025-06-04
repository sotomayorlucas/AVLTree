#ifndef SPLAY_TREE_H
#define SPLAY_TREE_H

#include "BinarySearchTree.h"
#include <stdexcept>

// Simple Splay tree derived from BinarySearchTree. Every access splays the
// accessed node to the root using rotations.

template<typename Key, typename Value = Key>
class SplayTree : public BinarySearchTree<Key, Value> {
    using Base = BinarySearchTree<Key, Value>;
    using Node = typename Base::Node;

    int height(Node* n) const { return n ? n->height : 0; }

    void updateHeight(Node* n) {
        if (n) n->height = 1 + std::max(height(n->left), height(n->right));
    }

    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* B = y->left;
        y->left = x;
        x->right = B;
        if (B) B->parent = x;
        y->parent = x->parent;
        if (!x->parent)
            this->root_ = y;
        else if (x->parent->left == x)
            x->parent->left = y;
        else
            x->parent->right = y;
        x->parent = y;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    Node* rotateRight(Node* x) {
        Node* y = x->left;
        Node* B = y->right;
        y->right = x;
        x->left = B;
        if (B) B->parent = x;
        y->parent = x->parent;
        if (!x->parent)
            this->root_ = y;
        else if (x->parent->left == x)
            x->parent->left = y;
        else
            x->parent->right = y;
        x->parent = y;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    void splay(Node* x) {
        while (x && x->parent) {
            Node* p = x->parent;
            Node* g = p->parent;
            if (!g) {
                if (x == p->left)
                    rotateRight(p);
                else
                    rotateLeft(p);
            } else if (x == p->left && p == g->left) {
                rotateRight(g);
                rotateRight(p);
            } else if (x == p->right && p == g->right) {
                rotateLeft(g);
                rotateLeft(p);
            } else if (x == p->right && p == g->left) {
                rotateLeft(p);
                rotateRight(g);
            } else {
                rotateRight(p);
                rotateLeft(g);
            }
        }
        this->root_ = x;
    }

protected:
    void rebalance(Node* start) override {
        if (start) splay(start);
    }

    typename Base::Node* insertNode(const Key& key, const Value& value) {
        if (!this->root_) {
            this->root_ = new Node(key, value);
            ++this->size_;
            return this->root_;
        }
        Node* cur = this->root_;
        Node* parent = nullptr;
        while (cur) {
            parent = cur;
            if (key < cur->key)
                cur = cur->left;
            else if (cur->key < key)
                cur = cur->right;
            else {
                cur->value = value;
                return cur;
            }
        }
        Node* n = new Node(key, value, parent);
        if (key < parent->key)
            parent->left = n;
        else
            parent->right = n;
        ++this->size_;
        return n;
    }

public:
    SplayTree() : Base() {}

    void insert(const Key& key, const Value& value = Value()) override {
        Node* n = insertNode(key, value);
        splay(n);
    }

    bool contains(const Key& key) const override {
        Node* n = const_cast<SplayTree*>(this)->findNode(key);
        if (n) const_cast<SplayTree*>(this)->splay(n);
        return n != nullptr;
    }

    const Value& get(const Key& key) const override {
        Node* n = const_cast<SplayTree*>(this)->findNode(key);
        if (!n) throw std::runtime_error("key not found");
        const_cast<SplayTree*>(this)->splay(n);
        return n->value;
    }

    void remove(const Key& key) override {
        Node* n = this->findNode(key);
        if (!n) return;
        this->removeNode(n);
        --this->size_;
        if (this->root_)
            splay(this->root_);
    }
};

#endif // SPLAY_TREE_H

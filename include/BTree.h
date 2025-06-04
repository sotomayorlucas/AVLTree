#ifndef B_TREE_H
#define B_TREE_H

#include "BaseTree.h"
#include <vector>
#include <utility>
#include <stdexcept>

// Simple B-Tree implementation with configurable order. This is a classic
// multi-way search tree where each node can contain up to `Order-1` keys and
// `Order` children. Only insertion and search are provided for brevity.

template<typename Key, typename Value = Key, unsigned Order = 4>
class BTree : public BaseTree<Key, Value> {
    struct Node {
        bool leaf = true;
        std::vector<Key> keys;
        std::vector<Value> values;
        std::vector<Node*> children;
        explicit Node(bool l = true) : leaf(l) {}
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;

    static constexpr unsigned MAX_KEYS = Order - 1;
    static constexpr unsigned MIN_DEGREE = (Order + 1) / 2; // ceil(Order/2)

    void destroy(Node* n) {
        if (!n) return;
        for (Node* c : n->children) destroy(c);
        delete n;
    }

    Node* search(Node* x, const Key& key) const {
        if (!x) return nullptr;
        unsigned i = 0;
        while (i < x->keys.size() && key > x->keys[i]) ++i;
        if (i < x->keys.size() && key == x->keys[i]) return x;
        if (x->leaf) return nullptr;
        return search(x->children[i], key);
    }

    unsigned findKeyIndex(Node* x, const Key& key) const {
        unsigned i = 0;
        while (i < x->keys.size() && key > x->keys[i]) ++i;
        return i;
    }

    void splitChild(Node* parent, unsigned i, Node* y) {
        Node* z = new Node(y->leaf);
        unsigned t = MIN_DEGREE;
        // Move last t-1 keys from y to z
        for (unsigned j = 0; j < t - 1; ++j) {
            z->keys.push_back(y->keys[j + t]);
            z->values.push_back(y->values[j + t]);
        }
        if (!y->leaf) {
            for (unsigned j = 0; j < t; ++j) {
                z->children.push_back(y->children[j + t]);
            }
            y->children.resize(t);
        }
        y->keys.resize(t - 1);
        y->values.resize(t - 1);

        parent->children.insert(parent->children.begin() + i + 1, z);
        parent->keys.insert(parent->keys.begin() + i, y->keys[t - 1]);
        parent->values.insert(parent->values.begin() + i, y->values[t - 1]);
    }

    void insertNonFull(Node* x, const Key& key, const Value& value) {
        unsigned i = x->keys.size();
        if (x->leaf) {
            // Insert key in sorted order
            x->keys.push_back(key);
            x->values.push_back(value);
            while (i > 0 && x->keys[i - 1] > key) {
                x->keys[i] = x->keys[i - 1];
                x->values[i] = x->values[i - 1];
                --i;
            }
            x->keys[i] = key;
            x->values[i] = value;
            ++size_;
        } else {
            while (i > 0 && key < x->keys[i - 1]) --i;
            if (x->children[i]->keys.size() == MAX_KEYS) {
                splitChild(x, i, x->children[i]);
                if (key > x->keys[i]) ++i;
            }
            insertNonFull(x->children[i], key, value);
        }
    }

public:
    BTree() = default;
    ~BTree() override { destroy(root_); }

    std::size_t size() const override { return size_; }

    bool contains(const Key& key) const override { return search(root_, key) != nullptr; }

    const Value& get(const Key& key) const override {
        Node* n = search(root_, key);
        if (!n) throw std::runtime_error("key not found");
        unsigned i = findKeyIndex(n, key);
        return n->values[i];
    }

    void insert(const Key& key, const Value& value = Value()) override {
        if (!root_) {
            root_ = new Node(true);
            root_->keys.push_back(key);
            root_->values.push_back(value);
            ++size_;
            return;
        }
        if (root_->keys.size() == MAX_KEYS) {
            Node* s = new Node(false);
            s->children.push_back(root_);
            splitChild(s, 0, root_);
            root_ = s;
        }
        insertNonFull(root_, key, value);
    }

    void remove(const Key& key) override { (void)key; /* removal not implemented */ }
};

#endif // B_TREE_H

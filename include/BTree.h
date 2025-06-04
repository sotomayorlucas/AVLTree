#ifndef B_TREE_H
#define B_TREE_H

#include "BaseTree.h"
#include <vector>
#include <utility>
#include <stdexcept>

// Simple B-Tree skeleton with configurable order. Only insertion of keys is
// sketched; full implementation is outside the scope of this example.

template<typename Key, typename Value = Key, unsigned Order = 4>
class BTree : public BaseTree<Key, Value> {
    struct Node {
        bool leaf = true;
        std::vector<Key> keys;
        std::vector<Value> values;
        std::vector<Node*> children;
    };

    Node* root_ = nullptr;
    std::size_t size_ = 0;

    // Placeholder split and insert algorithms would go here

public:
    BTree() = default;
    ~BTree() override { /* TODO: destroy tree */ }

    std::size_t size() const override { return size_; }
    bool contains(const Key&) const override { return false; }
    const Value& get(const Key& key) const override { throw std::runtime_error("not implemented"); }
    void insert(const Key& key, const Value& value = Value()) override {
        (void)key; (void)value; // not implemented
    }
    void remove(const Key& key) override { (void)key; /* not implemented */ }
};

#endif // B_TREE_H

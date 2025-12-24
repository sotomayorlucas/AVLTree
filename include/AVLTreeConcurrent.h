#ifndef AVL_TREE_CONCURRENT_H
#define AVL_TREE_CONCURRENT_H

#include "BaseTree.h"
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <algorithm>
#include <stdexcept>

// Concurrent AVL Tree using Read-Write Locks
//
// Strategy: Multiple readers, single writer
// - Readers acquire shared_lock (multiple allowed)
// - Writers acquire unique_lock (exclusive)
// - Good for read-heavy workloads
//
// Concurrency model: Readers-Writer Lock
// Performance: O(log n) with potential lock contention

template<typename Key, typename Value = Key>
class AVLTreeConcurrent : public BaseTree<Key, Value> {
private:
    struct Node {
        Key key;
        Value value;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        int height;

        Node(const Key& k, const Value& v)
            : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

    std::shared_ptr<Node> root_;
    std::size_t size_;
    mutable std::shared_mutex mutex_;  // Read-Write lock

    static int height(const std::shared_ptr<Node>& n) {
        return n ? n->height : 0;
    }

    static int balanceFactor(const std::shared_ptr<Node>& n) {
        return n ? height(n->right) - height(n->left) : 0;
    }

    static void updateHeight(std::shared_ptr<Node>& n) {
        if (n) {
            n->height = 1 + std::max(height(n->left), height(n->right));
        }
    }

    static std::shared_ptr<Node> rotateLeft(std::shared_ptr<Node> x) {
        auto y = x->right;
        auto B = y->left;

        y->left = x;
        x->right = B;

        updateHeight(x);
        updateHeight(y);

        return y;
    }

    static std::shared_ptr<Node> rotateRight(std::shared_ptr<Node> x) {
        auto y = x->left;
        auto B = y->right;

        y->right = x;
        x->left = B;

        updateHeight(x);
        updateHeight(y);

        return y;
    }

    static std::shared_ptr<Node> rebalance(std::shared_ptr<Node> node) {
        if (!node) return nullptr;

        updateHeight(node);
        int bf = balanceFactor(node);

        if (bf < -1) {
            if (balanceFactor(node->left) > 0) {
                node->left = rotateLeft(node->left);
            }
            return rotateRight(node);
        }

        if (bf > 1) {
            if (balanceFactor(node->right) < 0) {
                node->right = rotateRight(node->right);
            }
            return rotateLeft(node);
        }

        return node;
    }

    static std::shared_ptr<Node> insertRec(
        std::shared_ptr<Node> node,
        const Key& key,
        const Value& value,
        bool& inserted) {

        if (!node) {
            inserted = true;
            return std::make_shared<Node>(key, value);
        }

        if (key < node->key) {
            node->left = insertRec(node->left, key, value, inserted);
        } else if (node->key < key) {
            node->right = insertRec(node->right, key, value, inserted);
        } else {
            node->value = value;
            inserted = false;
            return node;
        }

        return rebalance(node);
    }

    static std::shared_ptr<Node> findMin(std::shared_ptr<Node> node) {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }

    static std::shared_ptr<Node> removeRec(
        std::shared_ptr<Node> node,
        const Key& key,
        bool& removed) {

        if (!node) {
            removed = false;
            return nullptr;
        }

        if (key < node->key) {
            node->left = removeRec(node->left, key, removed);
        } else if (node->key < key) {
            node->right = removeRec(node->right, key, removed);
        } else {
            removed = true;

            if (!node->left) return node->right;
            if (!node->right) return node->left;

            auto successor = findMin(node->right);
            node->key = successor->key;
            node->value = successor->value;
            node->right = removeRec(node->right, successor->key, removed);
        }

        return rebalance(node);
    }

    static bool containsRec(const std::shared_ptr<Node>& node, const Key& key) {
        if (!node) return false;
        if (key == node->key) return true;
        return key < node->key ? containsRec(node->left, key)
                                : containsRec(node->right, key);
    }

    static const Value& getRec(const std::shared_ptr<Node>& node, const Key& key) {
        if (!node) throw std::runtime_error("Key not found");
        if (key == node->key) return node->value;
        return key < node->key ? getRec(node->left, key)
                                : getRec(node->right, key);
    }

public:
    AVLTreeConcurrent() : root_(nullptr), size_(0) {}

    ~AVLTreeConcurrent() override = default;

    // Thread-safe insert (exclusive lock)
    void insert(const Key& key, const Value& value = Value()) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        bool inserted = false;
        root_ = insertRec(root_, key, value, inserted);
        if (inserted) ++size_;
    }

    // Thread-safe remove (exclusive lock)
    void remove(const Key& key) override {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        bool removed = false;
        root_ = removeRec(root_, key, removed);
        if (removed) --size_;
    }

    // Thread-safe contains (shared lock - multiple readers allowed)
    bool contains(const Key& key) const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return containsRec(root_, key);
    }

    // Thread-safe get (shared lock)
    const Value& get(const Key& key) const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return getRec(root_, key);
    }

    // Thread-safe size (shared lock)
    std::size_t size() const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return size_;
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        root_ = nullptr;
        size_ = 0;
    }

    const Key& minKey() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto node = findMin(root_);
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }

    const Key& maxKey() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto node = root_;
        while (node && node->right) {
            node = node->right;
        }
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }
};

#endif // AVL_TREE_CONCURRENT_H

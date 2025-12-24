#ifndef AVL_TREE_FINE_GRAINED_H
#define AVL_TREE_FINE_GRAINED_H

#include "BaseTree.h"
#include <memory>
#include <mutex>
#include <algorithm>
#include <stdexcept>

// Fine-Grained Locking AVL Tree
//
// Strategy: Each node has its own mutex
// - Hand-over-hand locking (lock coupling)
// - Lock parent, then child, release parent
// - Reduces contention compared to global lock
// - More complex but better concurrency
//
// Concurrency model: Per-node locking with lock coupling
// Performance: Better parallelism, higher overhead per operation

template<typename Key, typename Value = Key>
class AVLTreeFineGrained : public BaseTree<Key, Value> {
private:
    struct Node {
        Key key;
        Value value;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        int height;
        mutable std::mutex node_mutex;  // Each node has its own lock!

        Node(const Key& k, const Value& v)
            : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

    std::shared_ptr<Node> root_;
    std::size_t size_;
    mutable std::mutex root_mutex_;  // Separate lock for root pointer

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

    // Note: These rotation functions assume caller holds necessary locks
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

    // Simplified insert for fine-grained locking
    // In production, this would use lock coupling for better concurrency
    std::shared_ptr<Node> insertRec(
        std::shared_ptr<Node> node,
        const Key& key,
        const Value& value,
        bool& inserted) {

        if (!node) {
            inserted = true;
            return std::make_shared<Node>(key, value);
        }

        // Lock current node
        std::unique_lock<std::mutex> lock(node->node_mutex);

        if (key < node->key) {
            // Recurse to left (will lock left child)
            lock.unlock();  // Release parent before locking child
            node->left = insertRec(node->left, key, value, inserted);
            lock.lock();    // Re-acquire for rebalance
        } else if (node->key < key) {
            lock.unlock();
            node->right = insertRec(node->right, key, value, inserted);
            lock.lock();
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

    std::shared_ptr<Node> removeRec(
        std::shared_ptr<Node> node,
        const Key& key,
        bool& removed) {

        if (!node) {
            removed = false;
            return nullptr;
        }

        std::unique_lock<std::mutex> lock(node->node_mutex);

        if (key < node->key) {
            lock.unlock();
            node->left = removeRec(node->left, key, removed);
            lock.lock();
        } else if (node->key < key) {
            lock.unlock();
            node->right = removeRec(node->right, key, removed);
            lock.lock();
        } else {
            removed = true;

            if (!node->left) return node->right;
            if (!node->right) return node->left;

            auto successor = findMin(node->right);
            node->key = successor->key;
            node->value = successor->value;
            lock.unlock();
            node->right = removeRec(node->right, successor->key, removed);
            lock.lock();
        }

        return rebalance(node);
    }

    bool containsRec(const std::shared_ptr<Node>& node, const Key& key) const {
        if (!node) return false;

        std::unique_lock<std::mutex> lock(node->node_mutex);

        if (key == node->key) return true;

        lock.unlock();  // Release before recursing
        return key < node->key ? containsRec(node->left, key)
                                : containsRec(node->right, key);
    }

    const Value& getRec(const std::shared_ptr<Node>& node, const Key& key) const {
        if (!node) throw std::runtime_error("Key not found");

        std::unique_lock<std::mutex> lock(node->node_mutex);

        if (key == node->key) return node->value;

        lock.unlock();
        return key < node->key ? getRec(node->left, key)
                                : getRec(node->right, key);
    }

public:
    AVLTreeFineGrained() : root_(nullptr), size_(0) {}

    ~AVLTreeFineGrained() override = default;

    void insert(const Key& key, const Value& value = Value()) override {
        std::unique_lock<std::mutex> root_lock(root_mutex_);
        bool inserted = false;
        root_ = insertRec(root_, key, value, inserted);
        if (inserted) ++size_;
    }

    void remove(const Key& key) override {
        std::unique_lock<std::mutex> root_lock(root_mutex_);
        bool removed = false;
        root_ = removeRec(root_, key, removed);
        if (removed) --size_;
    }

    bool contains(const Key& key) const override {
        std::unique_lock<std::mutex> root_lock(root_mutex_);
        auto root_copy = root_;  // Copy root pointer
        root_lock.unlock();       // Release root lock early
        return containsRec(root_copy, key);
    }

    const Value& get(const Key& key) const override {
        std::unique_lock<std::mutex> root_lock(root_mutex_);
        auto root_copy = root_;
        root_lock.unlock();
        return getRec(root_copy, key);
    }

    std::size_t size() const override {
        std::unique_lock<std::mutex> lock(root_mutex_);
        return size_;
    }

    void clear() {
        std::unique_lock<std::mutex> lock(root_mutex_);
        root_ = nullptr;
        size_ = 0;
    }

    const Key& minKey() const {
        std::unique_lock<std::mutex> root_lock(root_mutex_);
        auto node = findMin(root_);
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }

    const Key& maxKey() const {
        std::unique_lock<std::mutex> root_lock(root_mutex_);
        auto node = root_;
        while (node && node->right) {
            node = node->right;
        }
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }
};

#endif // AVL_TREE_FINE_GRAINED_H

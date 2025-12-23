#ifndef AVL_TREE_FUNCTIONAL_H
#define AVL_TREE_FUNCTIONAL_H

#include "BaseTree.h"
#include <memory>
#include <algorithm>
#include <stdexcept>

// Functional Programming Paradigm AVL Tree Implementation
//
// Key principles:
// 1. IMMUTABILITY: Nodes are never modified after creation
// 2. PERSISTENT: Operations return new trees, old versions remain valid
// 3. STRUCTURAL SHARING: Reuse unchanged subtrees (copy-on-write)
// 4. PURE FUNCTIONS: No side effects, referential transparency
// 5. THREAD-SAFE: Immutability means inherent thread safety
//
// Trade-offs:
// + Thread-safe by design
// + Undo/versioning for free
// + No mutation bugs
// - More memory usage (shared_ptr overhead + structural sharing)
// - Slower single operations (copying nodes)
// + Better for concurrent reads from multiple threads

template<typename Key, typename Value = Key>
class AVLTreeFunctional : public BaseTree<Key, Value> {
private:
    // Immutable Node - once created, never modified
    struct Node {
        const Key key;
        const Value value;
        const std::shared_ptr<const Node> left;
        const std::shared_ptr<const Node> right;
        const int height;

        // Constructor for new nodes
        Node(const Key& k, const Value& v,
             std::shared_ptr<const Node> l = nullptr,
             std::shared_ptr<const Node> r = nullptr)
            : key(k), value(v), left(l), right(r),
              height(1 + std::max(getHeight(l), getHeight(r))) {}

        // Copy constructor with new children (for rotations)
        Node(const Node& other,
             std::shared_ptr<const Node> new_left,
             std::shared_ptr<const Node> new_right)
            : key(other.key), value(other.value),
              left(new_left), right(new_right),
              height(1 + std::max(getHeight(new_left), getHeight(new_right))) {}

        static int getHeight(const std::shared_ptr<const Node>& n) {
            return n ? n->height : 0;
        }

        int balanceFactor() const {
            return getHeight(right) - getHeight(left);
        }
    };

    std::shared_ptr<const Node> root_;
    std::size_t size_;

    // Pure function: get height
    static int height(const std::shared_ptr<const Node>& node) {
        return Node::getHeight(node);
    }

    // Pure function: get balance factor
    static int balanceFactor(const std::shared_ptr<const Node>& node) {
        return node ? node->balanceFactor() : 0;
    }

    // Pure function: create new node
    static std::shared_ptr<const Node> makeNode(
        const Key& key, const Value& value,
        std::shared_ptr<const Node> left = nullptr,
        std::shared_ptr<const Node> right = nullptr) {
        return std::make_shared<const Node>(key, value, left, right);
    }

    // Pure function: left rotation (returns NEW tree, original unchanged)
    static std::shared_ptr<const Node> rotateLeft(
        const std::shared_ptr<const Node>& x) {
        if (!x || !x->right) return x;

        auto y = x->right;
        auto B = y->left;

        // Create new nodes (immutable - no modification of originals)
        auto new_x = std::make_shared<const Node>(*x, x->left, B);
        auto new_y = std::make_shared<const Node>(*y, new_x, y->right);

        return new_y;
    }

    // Pure function: right rotation
    static std::shared_ptr<const Node> rotateRight(
        const std::shared_ptr<const Node>& x) {
        if (!x || !x->left) return x;

        auto y = x->left;
        auto B = y->right;

        auto new_x = std::make_shared<const Node>(*x, B, x->right);
        auto new_y = std::make_shared<const Node>(*y, y->left, new_x);

        return new_y;
    }

    // Pure function: rebalance (returns NEW balanced tree)
    static std::shared_ptr<const Node> rebalance(
        const std::shared_ptr<const Node>& node) {
        if (!node) return nullptr;

        int bf = balanceFactor(node);

        // Left-Left or Left-Right case
        if (bf < -1) {
            auto left = node->left;
            if (balanceFactor(left) > 0) {
                // Left-Right: rotate left child left first
                auto new_left = rotateLeft(left);
                auto new_node = std::make_shared<const Node>(*node, new_left, node->right);
                return rotateRight(new_node);
            } else {
                // Left-Left: just rotate right
                return rotateRight(node);
            }
        }

        // Right-Right or Right-Left case
        if (bf > 1) {
            auto right = node->right;
            if (balanceFactor(right) < 0) {
                // Right-Left: rotate right child right first
                auto new_right = rotateRight(right);
                auto new_node = std::make_shared<const Node>(*node, node->left, new_right);
                return rotateLeft(new_node);
            } else {
                // Right-Right: just rotate left
                return rotateLeft(node);
            }
        }

        return node;
    }

    // Pure function: insert (returns NEW tree)
    static std::shared_ptr<const Node> insertRec(
        const std::shared_ptr<const Node>& node,
        const Key& key, const Value& value,
        bool& inserted) {

        // Base case: create new node
        if (!node) {
            inserted = true;
            return makeNode(key, value);
        }

        // Recursive cases
        if (key < node->key) {
            // Insert in left subtree, create new node with new left child
            auto new_left = insertRec(node->left, key, value, inserted);
            auto new_node = std::make_shared<const Node>(*node, new_left, node->right);
            return rebalance(new_node);
        } else if (node->key < key) {
            // Insert in right subtree
            auto new_right = insertRec(node->right, key, value, inserted);
            auto new_node = std::make_shared<const Node>(*node, node->left, new_right);
            return rebalance(new_node);
        } else {
            // Key exists, update value (create new node with new value)
            inserted = false;
            return makeNode(key, value, node->left, node->right);
        }
    }

    // Pure function: find minimum
    static std::shared_ptr<const Node> findMin(
        const std::shared_ptr<const Node>& node) {
        if (!node) return nullptr;
        return node->left ? findMin(node->left) : node;
    }

    // Pure function: find maximum
    static std::shared_ptr<const Node> findMax(
        const std::shared_ptr<const Node>& node) {
        if (!node) return nullptr;
        return node->right ? findMax(node->right) : node;
    }

    // Pure function: remove (returns NEW tree)
    static std::shared_ptr<const Node> removeRec(
        const std::shared_ptr<const Node>& node,
        const Key& key,
        bool& removed) {

        if (!node) {
            removed = false;
            return nullptr;
        }

        if (key < node->key) {
            // Remove from left subtree
            auto new_left = removeRec(node->left, key, removed);
            if (!removed) return node; // Key not found, return unchanged
            auto new_node = std::make_shared<const Node>(*node, new_left, node->right);
            return rebalance(new_node);
        } else if (node->key < key) {
            // Remove from right subtree
            auto new_right = removeRec(node->right, key, removed);
            if (!removed) return node;
            auto new_node = std::make_shared<const Node>(*node, node->left, new_right);
            return rebalance(new_node);
        } else {
            // Found node to remove
            removed = true;

            // Case 1: Leaf or one child
            if (!node->left) return node->right;
            if (!node->right) return node->left;

            // Case 2: Two children - find inorder successor
            auto successor = findMin(node->right);
            // Create new node with successor's data and updated right subtree
            bool dummy;
            auto new_right = removeRec(node->right, successor->key, dummy);
            auto new_node = makeNode(successor->key, successor->value,
                                    node->left, new_right);
            return rebalance(new_node);
        }
    }

    // Pure function: find node
    static std::shared_ptr<const Node> findNode(
        const std::shared_ptr<const Node>& node,
        const Key& key) {
        if (!node) return nullptr;
        if (key == node->key) return node;
        return key < node->key ? findNode(node->left, key)
                                : findNode(node->right, key);
    }

    // Pure function: count nodes (for size)
    static std::size_t countNodes(const std::shared_ptr<const Node>& node) {
        if (!node) return 0;
        return 1 + countNodes(node->left) + countNodes(node->right);
    }

public:
    // Constructor: empty tree
    AVLTreeFunctional() : root_(nullptr), size_(0) {}

    // Copy constructor: shallow copy (structural sharing!)
    AVLTreeFunctional(const AVLTreeFunctional& other)
        : root_(other.root_), size_(other.size_) {
        // This is O(1) thanks to shared_ptr!
        // Both trees share the same nodes
    }

    // Assignment operator
    AVLTreeFunctional& operator=(const AVLTreeFunctional& other) {
        root_ = other.root_;
        size_ = other.size_;
        return *this;
    }

    ~AVLTreeFunctional() override = default;

    // Insert: returns nothing, but modifies THIS tree
    // (We maintain BaseTree interface but use functional internally)
    void insert(const Key& key, const Value& value = Value()) override {
        bool inserted = false;
        root_ = insertRec(root_, key, value, inserted);
        if (inserted) ++size_;
    }

    // Remove: returns nothing, modifies THIS tree
    void remove(const Key& key) override {
        bool removed = false;
        root_ = removeRec(root_, key, removed);
        if (removed) --size_;
    }

    bool contains(const Key& key) const override {
        return findNode(root_, key) != nullptr;
    }

    const Value& get(const Key& key) const override {
        auto node = findNode(root_, key);
        if (!node) throw std::runtime_error("Key not found");
        return node->value;
    }

    std::size_t size() const override {
        return size_;
    }

    const Key& minKey() const {
        auto node = findMin(root_);
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }

    const Key& maxKey() const {
        auto node = findMax(root_);
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }

    void clear() {
        root_ = nullptr;
        size_ = 0;
    }

    // Functional-specific: create a copy/snapshot (O(1) thanks to immutability!)
    AVLTreeFunctional snapshot() const {
        return AVLTreeFunctional(*this);
    }

    // Functional-specific: get memory usage stats
    struct MemoryStats {
        size_t node_count;
        size_t shared_ptr_overhead;
        size_t total_bytes;
    };

    MemoryStats getMemoryStats() const {
        MemoryStats stats;
        stats.node_count = size_;
        // Each node has 2 shared_ptrs (left, right)
        stats.shared_ptr_overhead = size_ * 2 * sizeof(std::shared_ptr<const Node>);
        // Total: node size * count + overhead
        stats.total_bytes = size_ * sizeof(Node) + stats.shared_ptr_overhead;
        return stats;
    }
};

#endif // AVL_TREE_FUNCTIONAL_H

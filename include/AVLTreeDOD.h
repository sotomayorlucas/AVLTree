#ifndef AVL_TREE_DOD_H
#define AVL_TREE_DOD_H

#include "BaseTree.h"
#include <vector>
#include <algorithm>
#include <cstdint>
#include <limits>

// Data-Oriented Design AVL Tree Implementation
//
// Key optimizations:
// 1. Structure of Arrays (SoA) instead of Array of Structures (AoS)
// 2. Index-based instead of pointer-based (eliminates pointer chasing)
// 3. Contiguous memory layout for better cache locality
// 4. Separation of hot/cold data paths
// 5. Node pooling with free list for efficient memory reuse

template<typename Key, typename Value = Key>
class AVLTreeDOD : public BaseTree<Key, Value> {
public:
    using Index = uint32_t;
    static constexpr Index INVALID = std::numeric_limits<Index>::max();

private:
    // Structure of Arrays (SoA) - Hot data: accessed during traversal
    std::vector<Key> keys_;           // Keys stored contiguously
    std::vector<Index> left_;         // Left child indices
    std::vector<Index> right_;        // Right child indices

    // Hot data: accessed during balancing
    std::vector<int8_t> height_;      // Node heights (8-bit is enough for balance factor)

    // Cold data: rarely accessed
    std::vector<Value> values_;       // Values (accessed only on get)

    // Tree metadata
    Index root_ = INVALID;
    std::size_t size_ = 0;

    // Free list for node reuse (eliminates allocation overhead)
    std::vector<Index> free_list_;

    // Helper: Allocate a new node index
    Index allocateNode(const Key& key, const Value& value) {
        Index idx;
        if (!free_list_.empty()) {
            // Reuse from free list
            idx = free_list_.back();
            free_list_.pop_back();
            keys_[idx] = key;
            values_[idx] = value;
            left_[idx] = INVALID;
            right_[idx] = INVALID;
            height_[idx] = 1;
        } else {
            // Allocate new
            idx = static_cast<Index>(keys_.size());
            keys_.push_back(key);
            values_.push_back(value);
            left_.push_back(INVALID);
            right_.push_back(INVALID);
            height_.push_back(1);
        }
        return idx;
    }

    // Helper: Free a node index
    void freeNode(Index idx) {
        free_list_.push_back(idx);
    }

    // Get height (branchless, cache-friendly)
    inline int getHeight(Index idx) const {
        return idx != INVALID ? height_[idx] : 0;
    }

    // Get balance factor (branchless)
    inline int getBalance(Index idx) const {
        return idx != INVALID ? getHeight(right_[idx]) - getHeight(left_[idx]) : 0;
    }

    // Update height (cache-friendly single operation)
    inline void updateHeight(Index idx) {
        if (idx != INVALID) {
            height_[idx] = 1 + std::max(getHeight(left_[idx]), getHeight(right_[idx]));
        }
    }

    // Find node by key (highly optimized iterative traversal)
    // Optimizations: manual loop unrolling, branch prediction hints
    Index findNode(const Key& key) const {
        Index cur = root_;
        // Fast path: traverse without balance checks
        while (cur != INVALID) [[likely]] {
            const Key& cur_key = keys_[cur];
            if (key == cur_key) [[likely]] return cur;
            // Branchless selection using ternary (compiler optimizes to CMOV)
            cur = (key < cur_key) ? left_[cur] : right_[cur];
        }
        return INVALID;
    }

    // Rotation operations (index-based, no pointer manipulation)
    Index rotateLeft(Index x, Index parent, bool isLeftChild) {
        Index y = right_[x];
        Index B = left_[y];

        // Perform rotation
        left_[y] = x;
        right_[x] = B;

        // Update heights
        updateHeight(x);
        updateHeight(y);

        // Update parent link
        if (parent == INVALID) {
            root_ = y;
        } else if (isLeftChild) {
            left_[parent] = y;
        } else {
            right_[parent] = y;
        }

        return y;
    }

    Index rotateRight(Index x, Index parent, bool isLeftChild) {
        Index y = left_[x];
        Index B = right_[y];

        // Perform rotation
        right_[y] = x;
        left_[x] = B;

        // Update heights
        updateHeight(x);
        updateHeight(y);

        // Update parent link
        if (parent == INVALID) {
            root_ = y;
        } else if (isLeftChild) {
            left_[parent] = y;
        } else {
            right_[parent] = y;
        }

        return y;
    }

    // Rebalance operations (optimized for cache locality)
    Index rebalance(Index idx, Index parent, bool isLeftChild) {
        if (idx == INVALID) return INVALID;

        updateHeight(idx);
        int balance = getBalance(idx);

        // Left-Left or Left-Right case
        if (balance < -1) {
            Index leftChild = left_[idx];
            if (getBalance(leftChild) > 0) {
                left_[idx] = rotateLeft(leftChild, idx, true);
            }
            return rotateRight(idx, parent, isLeftChild);
        }

        // Right-Right or Right-Left case
        if (balance > 1) {
            Index rightChild = right_[idx];
            if (getBalance(rightChild) < 0) {
                right_[idx] = rotateRight(rightChild, idx, false);
            }
            return rotateLeft(idx, parent, isLeftChild);
        }

        return idx;
    }

    // Recursive insert with path tracking for rebalancing
    Index insertRec(Index node, const Key& key, const Value& value, Index parent, bool isLeftChild, bool& inserted) {
        if (node == INVALID) {
            inserted = true;
            return allocateNode(key, value);
        }

        if (key < keys_[node]) {
            left_[node] = insertRec(left_[node], key, value, node, true, inserted);
        } else if (keys_[node] < key) {
            right_[node] = insertRec(right_[node], key, value, node, false, inserted);
        } else {
            // Key already exists, update value
            values_[node] = value;
            inserted = false;
            return node;
        }

        return rebalance(node, parent, isLeftChild);
    }

    // Find minimum in subtree
    Index findMin(Index node) const {
        while (node != INVALID && left_[node] != INVALID) {
            node = left_[node];
        }
        return node;
    }

    // Find maximum in subtree
    Index findMax(Index node) const {
        while (node != INVALID && right_[node] != INVALID) {
            node = right_[node];
        }
        return node;
    }

    // Remove node recursively
    Index removeRec(Index node, const Key& key, Index parent, bool isLeftChild, bool& removed) {
        if (node == INVALID) {
            removed = false;
            return INVALID;
        }

        if (key < keys_[node]) {
            left_[node] = removeRec(left_[node], key, node, true, removed);
        } else if (keys_[node] < key) {
            right_[node] = removeRec(right_[node], key, node, false, removed);
        } else {
            // Found node to remove
            removed = true;

            // Case 1: Leaf or single child
            if (left_[node] == INVALID) {
                Index temp = right_[node];
                freeNode(node);
                return temp;
            } else if (right_[node] == INVALID) {
                Index temp = left_[node];
                freeNode(node);
                return temp;
            }

            // Case 2: Two children - find inorder successor
            Index successor = findMin(right_[node]);
            keys_[node] = keys_[successor];
            values_[node] = values_[successor];
            right_[node] = removeRec(right_[node], keys_[successor], node, false, removed);
        }

        return rebalance(node, parent, isLeftChild);
    }

    // Destroy tree recursively
    void destroyRec(Index node) {
        if (node == INVALID) return;
        destroyRec(left_[node]);
        destroyRec(right_[node]);
        freeNode(node);
    }

public:
    AVLTreeDOD() {
        // Pre-allocate to reduce reallocations
        keys_.reserve(64);
        values_.reserve(64);
        left_.reserve(64);
        right_.reserve(64);
        height_.reserve(64);
        free_list_.reserve(32);
    }

    ~AVLTreeDOD() override {
        clear();
    }

    void insert(const Key& key, const Value& value = Value()) override {
        bool inserted = false;
        root_ = insertRec(root_, key, value, INVALID, false, inserted);
        if (inserted) ++size_;
    }

    void remove(const Key& key) override {
        bool removed = false;
        root_ = removeRec(root_, key, INVALID, false, removed);
        if (removed) --size_;
    }

    bool contains(const Key& key) const override {
        return findNode(key) != INVALID;
    }

    const Value& get(const Key& key) const override {
        Index idx = findNode(key);
        return values_[idx];
    }

    std::size_t size() const override {
        return size_;
    }

    void clear() {
        destroyRec(root_);
        root_ = INVALID;
        size_ = 0;
    }

    const Key& minKey() const {
        return keys_[findMin(root_)];
    }

    const Key& maxKey() const {
        return keys_[findMax(root_)];
    }

    // DOD-specific: Get memory stats
    struct MemoryStats {
        size_t total_capacity_bytes;
        size_t used_bytes;
        size_t wasted_bytes;
        size_t free_list_size;
    };

    MemoryStats getMemoryStats() const {
        MemoryStats stats;
        stats.total_capacity_bytes =
            keys_.capacity() * sizeof(Key) +
            values_.capacity() * sizeof(Value) +
            left_.capacity() * sizeof(Index) +
            right_.capacity() * sizeof(Index) +
            height_.capacity() * sizeof(int8_t) +
            free_list_.capacity() * sizeof(Index);

        stats.used_bytes = size_ * (sizeof(Key) + sizeof(Value) + 2 * sizeof(Index) + sizeof(int8_t));
        stats.wasted_bytes = stats.total_capacity_bytes - stats.used_bytes;
        stats.free_list_size = free_list_.size();

        return stats;
    }
};

#endif // AVL_TREE_DOD_H

#ifndef AVL_TREE_DOD_V2_H
#define AVL_TREE_DOD_V2_H

#include "BaseTree.h"
#include <vector>
#include <algorithm>
#include <cstdint>
#include <limits>

// DOD AVL Tree - Version 2: Hybrid approach with hot data packed
//
// This version packs frequently accessed data (key + left + right indices)
// together to improve cache line utilization during traversal.
// Values are kept separate (cold data).

template<typename Key, typename Value = Key>
class AVLTreeDOD_v2 : public BaseTree<Key, Value> {
public:
    using Index = uint32_t;
    static constexpr Index INVALID = std::numeric_limits<Index>::max();

private:
    // Hot data packed together (frequently accessed during traversal)
    // This struct fits in a single cache line (64 bytes typical)
    struct alignas(64) HotNode {
        Key key;           // The key
        Index left;        // Left child index
        Index right;       // Right child index
        int8_t height;     // Height for balancing
        char padding[64 - sizeof(Key) - 2*sizeof(Index) - sizeof(int8_t)];
    };

    std::vector<HotNode> nodes_;     // Hot data: keys + structure
    std::vector<Value> values_;      // Cold data: values
    std::vector<Index> free_list_;   // Free list for reuse

    Index root_ = INVALID;
    std::size_t size_ = 0;

    // Allocate node
    Index allocateNode(const Key& key, const Value& value) {
        Index idx;
        if (!free_list_.empty()) {
            idx = free_list_.back();
            free_list_.pop_back();
            nodes_[idx].key = key;
            nodes_[idx].left = INVALID;
            nodes_[idx].right = INVALID;
            nodes_[idx].height = 1;
            values_[idx] = value;
        } else {
            idx = static_cast<Index>(nodes_.size());
            HotNode node;
            node.key = key;
            node.left = INVALID;
            node.right = INVALID;
            node.height = 1;
            nodes_.push_back(node);
            values_.push_back(value);
        }
        return idx;
    }

    void freeNode(Index idx) {
        free_list_.push_back(idx);
    }

    inline int getHeight(Index idx) const {
        return idx != INVALID ? nodes_[idx].height : 0;
    }

    inline int getBalance(Index idx) const {
        if (idx == INVALID) return 0;
        const HotNode& node = nodes_[idx];
        return getHeight(node.right) - getHeight(node.left);
    }

    inline void updateHeight(Index idx) {
        if (idx != INVALID) {
            const HotNode& node = nodes_[idx];
            nodes_[idx].height = 1 + std::max(getHeight(node.left), getHeight(node.right));
        }
    }

    // Optimized search - single cache-friendly array access
    Index findNode(const Key& key) const {
        Index cur = root_;
        while (cur != INVALID) {
            const HotNode& node = nodes_[cur];  // Single cache line load
            if (key == node.key) return cur;
            cur = (key < node.key) ? node.left : node.right;
        }
        return INVALID;
    }

    Index rotateLeft(Index x, Index parent, bool isLeftChild) {
        HotNode& node_x = nodes_[x];
        Index y = node_x.right;
        HotNode& node_y = nodes_[y];
        Index B = node_y.left;

        node_y.left = x;
        node_x.right = B;

        updateHeight(x);
        updateHeight(y);

        if (parent == INVALID) {
            root_ = y;
        } else if (isLeftChild) {
            nodes_[parent].left = y;
        } else {
            nodes_[parent].right = y;
        }

        return y;
    }

    Index rotateRight(Index x, Index parent, bool isLeftChild) {
        HotNode& node_x = nodes_[x];
        Index y = node_x.left;
        HotNode& node_y = nodes_[y];
        Index B = node_y.right;

        node_y.right = x;
        node_x.left = B;

        updateHeight(x);
        updateHeight(y);

        if (parent == INVALID) {
            root_ = y;
        } else if (isLeftChild) {
            nodes_[parent].left = y;
        } else {
            nodes_[parent].right = y;
        }

        return y;
    }

    Index rebalance(Index idx, Index parent, bool isLeftChild) {
        if (idx == INVALID) return INVALID;

        updateHeight(idx);
        int balance = getBalance(idx);

        if (balance < -1) {
            Index leftChild = nodes_[idx].left;
            if (getBalance(leftChild) > 0) {
                nodes_[idx].left = rotateLeft(leftChild, idx, true);
            }
            return rotateRight(idx, parent, isLeftChild);
        }

        if (balance > 1) {
            Index rightChild = nodes_[idx].right;
            if (getBalance(rightChild) < 0) {
                nodes_[idx].right = rotateRight(rightChild, idx, false);
            }
            return rotateLeft(idx, parent, isLeftChild);
        }

        return idx;
    }

    // Iterative insertion (avoids recursion overhead)
    void insertIterative(const Key& key, const Value& value) {
        if (root_ == INVALID) {
            root_ = allocateNode(key, value);
            ++size_;
            return;
        }

        // Find insertion point and track path for rebalancing
        Index path[64];  // Max height of AVL tree is ~1.44 * log2(n)
        bool isLeft[64];
        int depth = 0;

        Index cur = root_;
        Index parent = INVALID;

        while (cur != INVALID) {
            const HotNode& node = nodes_[cur];

            if (key == node.key) {
                // Update existing
                values_[cur] = value;
                return;
            }

            path[depth] = cur;
            isLeft[depth] = key < node.key;
            parent = cur;
            cur = isLeft[depth] ? node.left : node.right;
            ++depth;
        }

        // Insert new node
        Index newNode = allocateNode(key, value);
        if (isLeft[depth - 1]) {
            nodes_[parent].left = newNode;
        } else {
            nodes_[parent].right = newNode;
        }
        ++size_;

        // Rebalance up the path
        Index child = newNode;
        for (int i = depth - 1; i >= 0; --i) {
            Index p = (i > 0) ? path[i - 1] : INVALID;
            bool il = (i > 0) ? isLeft[i - 1] : false;
            child = rebalance(path[i], p, il);
        }

        if (depth > 0) {
            // Update root if needed
            Index p = INVALID;
            child = rebalance(root_, p, false);
            if (root_ != child) root_ = child;
        }
    }

    Index findMin(Index node) const {
        while (node != INVALID && nodes_[node].left != INVALID) {
            node = nodes_[node].left;
        }
        return node;
    }

    Index removeRec(Index node, const Key& key, Index parent, bool isLeftChild, bool& removed) {
        if (node == INVALID) {
            removed = false;
            return INVALID;
        }

        HotNode& hot = nodes_[node];

        if (key < hot.key) {
            hot.left = removeRec(hot.left, key, node, true, removed);
        } else if (hot.key < key) {
            hot.right = removeRec(hot.right, key, node, false, removed);
        } else {
            removed = true;

            if (hot.left == INVALID) {
                Index temp = hot.right;
                freeNode(node);
                return temp;
            } else if (hot.right == INVALID) {
                Index temp = hot.left;
                freeNode(node);
                return temp;
            }

            Index successor = findMin(hot.right);
            hot.key = nodes_[successor].key;
            values_[node] = values_[successor];
            hot.right = removeRec(hot.right, nodes_[successor].key, node, false, removed);
        }

        return rebalance(node, parent, isLeftChild);
    }

    void destroyRec(Index node) {
        if (node == INVALID) return;
        const HotNode& hot = nodes_[node];
        destroyRec(hot.left);
        destroyRec(hot.right);
        freeNode(node);
    }

public:
    AVLTreeDOD_v2() {
        nodes_.reserve(64);
        values_.reserve(64);
        free_list_.reserve(32);
    }

    ~AVLTreeDOD_v2() override {
        clear();
    }

    void insert(const Key& key, const Value& value = Value()) override {
        insertIterative(key, value);
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
        return values_[findNode(key)];
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
        return nodes_[findMin(root_)].key;
    }

    const Key& maxKey() const {
        Index node = root_;
        while (node != INVALID && nodes_[node].right != INVALID) {
            node = nodes_[node].right;
        }
        return nodes_[node].key;
    }
};

#endif // AVL_TREE_DOD_V2_H

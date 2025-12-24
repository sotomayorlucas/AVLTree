#ifndef AVL_TREE_OPTIMISTIC_LOCK_H
#define AVL_TREE_OPTIMISTIC_LOCK_H

#include "BaseTree.h"
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <algorithm>
#include <stdexcept>
#include <vector>

// AVL Tree with Optimistic Hand-over-Hand Locking
//
// Strategy: Lock granular por path con early release
// - Lock solo el camino desde raíz al nodo objetivo
// - Release locks tan pronto como sea seguro
// - Operaciones en diferentes subárboles NO se bloquean
// - Hand-over-hand locking: lock hijo, release padre
//
// Ejemplo de paralelismo real:
//   Thread 1: insert(10) en subárbol izquierdo
//   Thread 2: insert(90) en subárbol derecho
//   ↓ Ambos pueden trabajar simultáneamente!
//
// Mejora sobre AVLTreeConcurrent:
//   - Global lock: TODO serializado
//   - Granular lock: Solo paths que se cruzan se bloquean

template<typename Key, typename Value = Key>
class AVLTreeOptimisticLock : public BaseTree<Key, Value> {
private:
    struct Node {
        Key key;
        Value value;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        int height;
        mutable std::shared_mutex node_lock;  // Lock por nodo

        Node(const Key& k, const Value& v)
            : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

    std::shared_ptr<Node> root_;
    std::size_t size_;
    mutable std::mutex root_lock_;  // Solo para proteger puntero raíz

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

    // Hand-over-hand locking para insert
    // Lock parent, lock child, release parent (early release!)
    std::shared_ptr<Node> insertWithLocking(
        std::shared_ptr<Node> node,
        const Key& key,
        const Value& value,
        std::unique_lock<std::shared_mutex>* parent_lock,
        bool& inserted) {

        if (!node) {
            inserted = true;
            return std::make_shared<Node>(key, value);
        }

        // Lock este nodo
        std::unique_lock<std::shared_mutex> current_lock(node->node_lock);

        // Release parent lock (hand-over-hand!)
        if (parent_lock) {
            parent_lock->unlock();
        }

        if (key < node->key) {
            node->left = insertWithLocking(node->left, key, value, &current_lock, inserted);
        } else if (node->key < key) {
            node->right = insertWithLocking(node->right, key, value, &current_lock, inserted);
        } else {
            node->value = value;
            inserted = false;
            return node;
        }

        // Rebalance mientras tenemos el lock
        return rebalance(node);
    }

    // Búsqueda con shared locks (múltiples lectores permitidos)
    bool containsWithSharedLock(
        const std::shared_ptr<Node>& node,
        const Key& key,
        std::shared_lock<std::shared_mutex>* parent_lock) const {

        if (!node) return false;

        // Shared lock en este nodo (múltiples threads pueden tener shared lock)
        std::shared_lock<std::shared_mutex> current_lock(node->node_lock);

        // Release parent lock
        if (parent_lock) {
            parent_lock->unlock();
        }

        if (key == node->key) return true;

        // Continuar búsqueda (pasando nuestro lock como parent)
        return key < node->key
            ? containsWithSharedLock(node->left, key, &current_lock)
            : containsWithSharedLock(node->right, key, &current_lock);
    }

    static std::shared_ptr<Node> findMin(std::shared_ptr<Node> node) {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }

    std::shared_ptr<Node> removeWithLocking(
        std::shared_ptr<Node> node,
        const Key& key,
        std::unique_lock<std::shared_mutex>* parent_lock,
        bool& removed) {

        if (!node) {
            removed = false;
            return nullptr;
        }

        std::unique_lock<std::shared_mutex> current_lock(node->node_lock);

        if (parent_lock) {
            parent_lock->unlock();
        }

        if (key < node->key) {
            node->left = removeWithLocking(node->left, key, &current_lock, removed);
        } else if (node->key < key) {
            node->right = removeWithLocking(node->right, key, &current_lock, removed);
        } else {
            removed = true;

            if (!node->left) return node->right;
            if (!node->right) return node->left;

            auto successor = findMin(node->right);
            node->key = successor->key;
            node->value = successor->value;
            node->right = removeWithLocking(node->right, successor->key, &current_lock, removed);
        }

        return rebalance(node);
    }

public:
    AVLTreeOptimisticLock() : root_(nullptr), size_(0) {}

    ~AVLTreeOptimisticLock() override = default;

    void insert(const Key& key, const Value& value = Value()) override {
        std::unique_lock<std::mutex> root_guard(root_lock_);

        if (!root_) {
            root_ = std::make_shared<Node>(key, value);
            ++size_;
            return;
        }

        auto root_copy = root_;
        root_guard.unlock();  // Release root lock early!

        bool inserted = false;
        std::unique_lock<std::shared_mutex> dummy_lock;

        // Lock raíz y comenzar hand-over-hand
        root_ = insertWithLocking(root_copy, key, value, nullptr, inserted);

        if (inserted) {
            std::lock_guard<std::mutex> size_guard(root_lock_);
            ++size_;
        }
    }

    void remove(const Key& key) override {
        std::unique_lock<std::mutex> root_guard(root_lock_);

        if (!root_) return;

        auto root_copy = root_;
        root_guard.unlock();

        bool removed = false;
        root_ = removeWithLocking(root_copy, key, nullptr, removed);

        if (removed) {
            std::lock_guard<std::mutex> size_guard(root_lock_);
            --size_;
        }
    }

    bool contains(const Key& key) const override {
        std::unique_lock<std::mutex> root_guard(root_lock_);

        if (!root_) return false;

        auto root_copy = root_;
        root_guard.unlock();

        // Shared lock permite múltiples lectores simultáneos
        return containsWithSharedLock(root_copy, key, nullptr);
    }

    const Value& get(const Key& key) const override {
        std::unique_lock<std::mutex> root_guard(root_lock_);

        if (!root_) throw std::runtime_error("Key not found");

        auto root_copy = root_;
        root_guard.unlock();

        std::shared_ptr<Node> node = root_copy;
        while (node) {
            std::shared_lock<std::shared_mutex> lock(node->node_lock);

            if (key == node->key) return node->value;

            auto next = key < node->key ? node->left : node->right;
            lock.unlock();  // Release antes de ir al siguiente
            node = next;
        }

        throw std::runtime_error("Key not found");
    }

    std::size_t size() const override {
        std::lock_guard<std::mutex> lock(root_lock_);
        return size_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(root_lock_);
        root_ = nullptr;
        size_ = 0;
    }

    const Key& minKey() const {
        std::unique_lock<std::mutex> root_guard(root_lock_);
        auto node = findMin(root_);
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }

    const Key& maxKey() const {
        std::unique_lock<std::mutex> root_guard(root_lock_);
        auto node = root_;
        while (node && node->right) {
            node = node->right;
        }
        if (!node) throw std::runtime_error("Empty tree");
        return node->key;
    }
};

#endif // AVL_TREE_OPTIMISTIC_LOCK_H

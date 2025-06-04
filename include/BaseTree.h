#ifndef BASE_TREE_H
#define BASE_TREE_H

#include <cstddef>
#include <utility>

// Generic search tree interface
// Provides basic operations that all search trees should implement

template<typename Key, typename Value = Key>
class BaseTree {
public:
    virtual ~BaseTree() = default;

    // Insert key and associated value. For set-like structures the value can be omitted
    virtual void insert(const Key& key, const Value& value = Value()) = 0;

    // Remove a key from the tree. If the key is not present, the operation is a no-op
    virtual void remove(const Key& key) = 0;

    // Return true if the key exists in the tree
    virtual bool contains(const Key& key) const = 0;

    // Return the value associated to key. Behavior is undefined if key does not exist
    virtual const Value& get(const Key& key) const = 0;

    // Number of elements stored in the tree
    virtual std::size_t size() const = 0;
};

#endif // BASE_TREE_H

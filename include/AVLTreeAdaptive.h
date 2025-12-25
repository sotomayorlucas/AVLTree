#ifndef AVL_TREE_ADAPTIVE_H
#define AVL_TREE_ADAPTIVE_H

#include "BaseTree.h"
#include "AVLTree.h"
#include "AdaptiveRouter.h"
#include <vector>
#include <mutex>
#include <memory>
#include <iostream>
#include <iomanip>

// AVL Tree con Routing Adaptativo Inteligente
// Previene targeted attacks mediante redistribución automática

template<typename Key, typename Value = Key>
class AVLTreeAdaptive : public BaseTree<Key, Value> {
private:
    struct TreeShard {
        AVLTree<Key, Value> tree;
        mutable std::mutex lock;
        std::atomic<size_t> local_size{0};

        TreeShard() = default;
    };

    size_t num_shards_;
    std::vector<std::unique_ptr<TreeShard>> shards_;

    // Sistema de routing adaptativo
    std::unique_ptr<AdaptiveRouter<Key>> router_;
    typename AdaptiveRouter<Key>::Strategy routing_strategy_;

public:
    // Constructor
    AVLTreeAdaptive(
        size_t num_shards = 8,
        typename AdaptiveRouter<Key>::Strategy strategy =
            AdaptiveRouter<Key>::Strategy::INTELLIGENT
    ) : num_shards_(num_shards)
      , routing_strategy_(strategy)
    {
        // Crear router adaptativo
        router_ = std::make_unique<AdaptiveRouter<Key>>(num_shards_, strategy);

        // Crear shards
        for (size_t i = 0; i < num_shards_; ++i) {
            shards_.push_back(std::make_unique<TreeShard>());
        }
    }

    ~AVLTreeAdaptive() override = default;

    // Insert con routing adaptativo
    void insert(const Key& key, const Value& value = Value()) override {
        // Determinar shard con routing inteligente
        size_t shard_idx = router_->route(key);
        auto& shard = shards_[shard_idx];

        std::lock_guard<std::mutex> lock(shard->lock);

        size_t old_size = shard->tree.size();
        shard->tree.insert(key, value);
        size_t new_size = shard->tree.size();

        if (new_size > old_size) {
            shard->local_size++;
            // Notificar al router sobre la inserción
            router_->recordInsertion(shard_idx);
        }
    }

    // Remove
    void remove(const Key& key) override {
        // Buscar en todos los shards (no sabemos dónde está)
        for (size_t i = 0; i < num_shards_; ++i) {
            auto& shard = shards_[i];
            std::lock_guard<std::mutex> lock(shard->lock);

            size_t old_size = shard->tree.size();
            shard->tree.remove(key);
            size_t new_size = shard->tree.size();

            if (new_size < old_size) {
                shard->local_size--;
                router_->recordRemoval(i);
                return;
            }
        }
    }

    // Contains
    bool contains(const Key& key) const override {
        // Intentar shard sugerido primero
        size_t shard_idx = router_->route(key);
        {
            auto& shard = shards_[shard_idx];
            std::lock_guard<std::mutex> lock(shard->lock);
            if (shard->tree.contains(key)) {
                return true;
            }
        }

        // Buscar en otros shards (por si fue redirigido)
        for (size_t i = 0; i < num_shards_; ++i) {
            if (i == shard_idx) continue;

            auto& shard = shards_[i];
            std::lock_guard<std::mutex> lock(shard->lock);
            if (shard->tree.contains(key)) {
                return true;
            }
        }

        return false;
    }

    // Get
    const Value& get(const Key& key) const override {
        // Buscar en shard sugerido primero
        size_t shard_idx = router_->route(key);
        {
            auto& shard = shards_[shard_idx];
            std::lock_guard<std::mutex> lock(shard->lock);
            if (shard->tree.contains(key)) {
                return shard->tree.get(key);
            }
        }

        // Buscar en otros shards
        for (size_t i = 0; i < num_shards_; ++i) {
            if (i == shard_idx) continue;

            auto& shard = shards_[i];
            std::lock_guard<std::mutex> lock(shard->lock);
            if (shard->tree.contains(key)) {
                return shard->tree.get(key);
            }
        }

        throw std::runtime_error("Key not found");
    }

    // Size
    std::size_t size() const override {
        std::size_t total = 0;
        for (const auto& shard : shards_) {
            total += shard->local_size.load();
        }
        return total;
    }

    // Estadísticas del routing adaptativo
    struct AdaptiveStats {
        size_t num_shards;
        size_t total_elements;
        double avg_per_shard;
        size_t min_shard;
        size_t max_shard;
        double balance_score;
        bool has_hotspot;
        std::string strategy_name;
    };

    AdaptiveStats getAdaptiveStats() const {
        auto router_stats = router_->getStats();

        AdaptiveStats stats;
        stats.num_shards = num_shards_;
        stats.total_elements = router_stats.total_load;
        stats.avg_per_shard = router_stats.avg_load;
        stats.min_shard = router_stats.min_load;
        stats.max_shard = router_stats.max_load;
        stats.balance_score = router_stats.balance_score;
        stats.has_hotspot = router_stats.has_hotspot;

        // Nombre de estrategia
        switch (routing_strategy_) {
            case AdaptiveRouter<Key>::Strategy::STATIC_HASH:
                stats.strategy_name = "Static Hash";
                break;
            case AdaptiveRouter<Key>::Strategy::LOAD_AWARE:
                stats.strategy_name = "Load-Aware";
                break;
            case AdaptiveRouter<Key>::Strategy::VIRTUAL_NODES:
                stats.strategy_name = "Virtual Nodes";
                break;
            case AdaptiveRouter<Key>::Strategy::INTELLIGENT:
                stats.strategy_name = "Intelligent (Adaptive)";
                break;
        }

        return stats;
    }

    // Print distribution
    void printDistribution() const {
        auto stats = getAdaptiveStats();

        std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║  Adaptive Routing Statistics           ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝\n" << std::endl;

        std::cout << "Strategy: " << stats.strategy_name << std::endl;
        std::cout << "Shards: " << stats.num_shards << std::endl;
        std::cout << "Total elements: " << stats.total_elements << std::endl;
        std::cout << "Avg per shard: " << std::fixed << std::setprecision(1)
                  << stats.avg_per_shard << std::endl;
        std::cout << "Balance score: " << std::fixed << std::setprecision(2)
                  << (stats.balance_score * 100) << "%" << std::endl;

        if (stats.has_hotspot) {
            std::cout << "⚠️  Hotspot detected!" << std::endl;
        } else {
            std::cout << "✅ Well balanced" << std::endl;
        }

        std::cout << "\nShard Distribution:" << std::endl;
        for (size_t i = 0; i < num_shards_; ++i) {
            size_t count = shards_[i]->local_size.load();
            double pct = stats.total_elements > 0 ?
                        (count * 100.0 / stats.total_elements) : 0;

            std::cout << "  Shard " << i << ": "
                      << std::setw(6) << count << " elements ("
                      << std::fixed << std::setprecision(1) << std::setw(5) << pct << "%)" << std::endl;
        }

        std::cout << std::endl;
    }

    // Para testing: resetear estadísticas del router
    void resetRouterStats() {
        router_ = std::make_unique<AdaptiveRouter<Key>>(num_shards_, routing_strategy_);
    }
};

#endif // AVL_TREE_ADAPTIVE_H

//----------------------------------------------------------------------------
//! @file   spatial_grid_3d.h
//! @brief  3D空間グリッド（Broad-phase）
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/actor.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace Collision {

//============================================================================
//! @brief 3D空間グリッド
//!
//! 各セルにActorリストを保持。
//! 大きいオブジェクトは複数セルに登録される。
//============================================================================
class SpatialGrid3D {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param cellSize セルサイズ
    //------------------------------------------------------------------------
    explicit SpatialGrid3D(float cellSize = 10.0f) noexcept
        : cellSize_(cellSize), invCellSize_(1.0f / cellSize) {}

    //------------------------------------------------------------------------
    //! @brief 全セルをクリア
    //------------------------------------------------------------------------
    void Clear() noexcept {
        cells_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief セルサイズを設定
    //------------------------------------------------------------------------
    void SetCellSize(float size) noexcept {
        cellSize_ = size;
        invCellSize_ = 1.0f / size;
    }

    //------------------------------------------------------------------------
    //! @brief セルサイズを取得
    //------------------------------------------------------------------------
    [[nodiscard]] float GetCellSize() const noexcept { return cellSize_; }

    //------------------------------------------------------------------------
    //! @brief コライダーを登録（AABB境界）
    //! @param actor エンティティ
    //! @param minX, minY, minZ AABB最小点
    //! @param maxX, maxY, maxZ AABB最大点
    //------------------------------------------------------------------------
    void Insert(ECS::Actor actor,
                float minX, float minY, float minZ,
                float maxX, float maxY, float maxZ) {
        int minCellX = static_cast<int>(std::floor(minX * invCellSize_));
        int maxCellX = static_cast<int>(std::floor(maxX * invCellSize_));
        int minCellY = static_cast<int>(std::floor(minY * invCellSize_));
        int maxCellY = static_cast<int>(std::floor(maxY * invCellSize_));
        int minCellZ = static_cast<int>(std::floor(minZ * invCellSize_));
        int maxCellZ = static_cast<int>(std::floor(maxZ * invCellSize_));

        for (int cz = minCellZ; cz <= maxCellZ; ++cz) {
            for (int cy = minCellY; cy <= maxCellY; ++cy) {
                for (int cx = minCellX; cx <= maxCellX; ++cx) {
                    CellKey key{cx, cy, cz};
                    cells_[key].push_back(actor);
                }
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief 全ペアをコールバックで列挙（重複なし）
    //! @param callback コールバック関数 void(Actor a, Actor b)
    //------------------------------------------------------------------------
    template<typename Func>
    void QueryAllPairs(Func&& callback) const {
        std::unordered_set<uint64_t> testedPairs;

        for (const auto& [cellKey, actors] : cells_) {
            size_t count = actors.size();
            for (size_t i = 0; i < count; ++i) {
                for (size_t j = i + 1; j < count; ++j) {
                    uint64_t pairKey = MakePairKey(actors[i], actors[j]);
                    if (testedPairs.insert(pairKey).second) {
                        callback(actors[i], actors[j]);
                    }
                }
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief 指定範囲のコライダーを列挙
    //------------------------------------------------------------------------
    template<typename Func>
    void QueryRange(float minX, float minY, float minZ,
                    float maxX, float maxY, float maxZ,
                    Func&& callback) const {
        int minCellX = static_cast<int>(std::floor(minX * invCellSize_));
        int maxCellX = static_cast<int>(std::floor(maxX * invCellSize_));
        int minCellY = static_cast<int>(std::floor(minY * invCellSize_));
        int maxCellY = static_cast<int>(std::floor(maxY * invCellSize_));
        int minCellZ = static_cast<int>(std::floor(minZ * invCellSize_));
        int maxCellZ = static_cast<int>(std::floor(maxZ * invCellSize_));

        std::unordered_set<uint32_t> visitedActors;

        for (int cz = minCellZ; cz <= maxCellZ; ++cz) {
            for (int cy = minCellY; cy <= maxCellY; ++cy) {
                for (int cx = minCellX; cx <= maxCellX; ++cx) {
                    CellKey key{cx, cy, cz};
                    auto it = cells_.find(key);
                    if (it != cells_.end()) {
                        for (const auto& actor : it->second) {
                            if (visitedActors.insert(actor.GetId()).second) {
                                callback(actor);
                            }
                        }
                    }
                }
            }
        }
    }

    //------------------------------------------------------------------------
    //! @brief 登録されているセル数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetCellCount() const noexcept { return cells_.size(); }

private:
    //------------------------------------------------------------------------
    //! @brief 3Dセルキー
    //------------------------------------------------------------------------
    struct CellKey {
        int x, y, z;

        bool operator==(const CellKey& other) const noexcept {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const noexcept {
            // Simple hash combining
            size_t h = 0;
            h ^= std::hash<int>{}(k.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<int>{}(k.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<int>{}(k.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    //------------------------------------------------------------------------
    //! @brief ペアキーを生成（順序正規化）
    //------------------------------------------------------------------------
    [[nodiscard]] static uint64_t MakePairKey(ECS::Actor a, ECS::Actor b) noexcept {
        uint32_t idA = a.GetId(), idB = b.GetId();
        if (idA > idB) std::swap(idA, idB);
        return (static_cast<uint64_t>(idA) << 32) | idB;
    }

    float cellSize_;
    float invCellSize_;
    std::unordered_map<CellKey, std::vector<ECS::Actor>, CellKeyHash> cells_;
};

} // namespace Collision

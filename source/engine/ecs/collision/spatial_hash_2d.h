//----------------------------------------------------------------------------
//! @file   spatial_hash_2d.h
//! @brief  2D空間ハッシュグリッド（Broad-phase）
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
//! @brief 2D空間ハッシュグリッド
//!
//! 各セルにActorリストを保持。
//! 大きいオブジェクトは複数セルに登録される。
//============================================================================
class SpatialHash2D {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param cellSize セルサイズ（ピクセル）
    //------------------------------------------------------------------------
    explicit SpatialHash2D(float cellSize = 128.0f) noexcept
        : cellSize_(cellSize), invCellSize_(1.0f / cellSize) {}

    //------------------------------------------------------------------------
    //! @brief 全セルをクリア
    //------------------------------------------------------------------------
    void Clear() noexcept {
        cells_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief セルサイズを設定
    //! @param size セルサイズ
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
    //! @brief コライダーを登録
    //! @param actor エンティティ
    //! @param posX X座標
    //! @param posY Y座標
    //! @param halfW 半幅
    //! @param halfH 半高
    //------------------------------------------------------------------------
    void Insert(ECS::Actor actor, float posX, float posY, float halfW, float halfH) {
        int minCellX = static_cast<int>(std::floor((posX - halfW) * invCellSize_));
        int maxCellX = static_cast<int>(std::floor((posX + halfW) * invCellSize_));
        int minCellY = static_cast<int>(std::floor((posY - halfH) * invCellSize_));
        int maxCellY = static_cast<int>(std::floor((posY + halfH) * invCellSize_));

        for (int cy = minCellY; cy <= maxCellY; ++cy) {
            for (int cx = minCellX; cx <= maxCellX; ++cx) {
                uint64_t key = MakeKey(cx, cy);
                cells_[key].push_back(actor);
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
    //! @param posX X座標
    //! @param posY Y座標
    //! @param halfW 半幅
    //! @param halfH 半高
    //! @param callback コールバック関数 void(Actor actor)
    //------------------------------------------------------------------------
    template<typename Func>
    void QueryRange(float posX, float posY, float halfW, float halfH, Func&& callback) const {
        int minCellX = static_cast<int>(std::floor((posX - halfW) * invCellSize_));
        int maxCellX = static_cast<int>(std::floor((posX + halfW) * invCellSize_));
        int minCellY = static_cast<int>(std::floor((posY - halfH) * invCellSize_));
        int maxCellY = static_cast<int>(std::floor((posY + halfH) * invCellSize_));

        std::unordered_set<uint32_t> visitedActors;

        for (int cy = minCellY; cy <= maxCellY; ++cy) {
            for (int cx = minCellX; cx <= maxCellX; ++cx) {
                uint64_t key = MakeKey(cx, cy);
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

    //------------------------------------------------------------------------
    //! @brief 登録されているセル数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetCellCount() const noexcept { return cells_.size(); }

private:
    //------------------------------------------------------------------------
    //! @brief セルキーを生成
    //------------------------------------------------------------------------
    [[nodiscard]] static uint64_t MakeKey(int cx, int cy) noexcept {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) |
               static_cast<uint64_t>(static_cast<uint32_t>(cy));
    }

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
    std::unordered_map<uint64_t, std::vector<ECS::Actor>> cells_;
};

} // namespace Collision

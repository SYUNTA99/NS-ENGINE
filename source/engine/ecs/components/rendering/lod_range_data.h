//----------------------------------------------------------------------------
//! @file   lod_range_data.h
//! @brief  ECS LODRangeData - LOD距離設定
//! @ref    Unity DOTS LODRange
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"
#include <limits>
#include <cmath>

namespace ECS {

//============================================================================
//! @brief LOD距離データ（POD構造体）
//!
//! カメラからの距離に基づくLOD（Level of Detail）切り替え設定。
//! パフォーマンス最適化のため、遠距離オブジェクトを簡略化または非表示に。
//!
//! @note メモリレイアウト: 8 bytes
//!
//! @code
//! // LOD設定
//! world.AddComponent<LODRangeData>(actor, LODRangeData{0.0f, 50.0f});
//!
//! // カリングシステム
//! world.ForEach<LODRangeData, LocalToWorld>(
//!     [&](Actor e, auto& lod, auto& ltw) {
//!         float dist = (ltw.GetPosition() - cameraPos).Length();
//!         if (dist < lod.minDistance || dist > lod.maxDistance) {
//!             // 描画しない
//!             return;
//!         }
//!     });
//! @endcode
//============================================================================
struct LODRangeData : public IComponentData {
    float minDistance = 0.0f;    //!< 最小表示距離（この距離より近いと非表示）
    float maxDistance = 1000.0f; //!< 最大表示距離（この距離より遠いと非表示）

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    LODRangeData() = default;

    LODRangeData(float minDist, float maxDist) noexcept
        : minDistance(minDist), maxDistance(maxDist) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 無制限（常に表示）
    [[nodiscard]] static LODRangeData Unlimited() noexcept {
        return LODRangeData(0.0f, (std::numeric_limits<float>::max)());
    }

    //! @brief 近距離用（0-50m）
    [[nodiscard]] static LODRangeData Near() noexcept {
        return LODRangeData(0.0f, 50.0f);
    }

    //! @brief 中距離用（50-200m）
    [[nodiscard]] static LODRangeData Medium() noexcept {
        return LODRangeData(50.0f, 200.0f);
    }

    //! @brief 遠距離用（200m-）
    [[nodiscard]] static LODRangeData Far() noexcept {
        return LODRangeData(200.0f, (std::numeric_limits<float>::max)());
    }

    //! @brief LODレベル用（連続した範囲を設定）
    //! @param level LODレベル（0=最高詳細, 1, 2...）
    //! @param baseDistance 基準距離
    //! @param multiplier 距離倍率
    [[nodiscard]] static LODRangeData ForLevel(int level, float baseDistance = 25.0f, float multiplier = 2.0f) noexcept {
        float minDist = (level == 0) ? 0.0f : baseDistance * std::pow(multiplier, static_cast<float>(level - 1));
        float maxDist = baseDistance * std::pow(multiplier, static_cast<float>(level));
        return LODRangeData(minDist, maxDist);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 距離がLOD範囲内か
    [[nodiscard]] bool IsInRange(float distance) const noexcept {
        return distance >= minDistance && distance <= maxDistance;
    }

    //! @brief 距離が最小距離より近いか
    [[nodiscard]] bool IsTooClose(float distance) const noexcept {
        return distance < minDistance;
    }

    //! @brief 距離が最大距離より遠いか
    [[nodiscard]] bool IsTooFar(float distance) const noexcept {
        return distance > maxDistance;
    }

    //! @brief LOD範囲の幅を取得
    [[nodiscard]] float GetRange() const noexcept {
        return maxDistance - minDistance;
    }

    //! @brief 距離のLOD範囲内での正規化位置（0-1）
    [[nodiscard]] float GetNormalizedPosition(float distance) const noexcept {
        float range = GetRange();
        if (range <= 0.0f) return 0.0f;
        return (distance - minDistance) / range;
    }
};

// コンパイル時検証
ECS_COMPONENT(LODRangeData);
static_assert(sizeof(LODRangeData) == 8, "LODRangeData must be 8 bytes");

} // namespace ECS

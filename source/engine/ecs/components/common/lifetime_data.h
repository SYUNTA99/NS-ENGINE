//----------------------------------------------------------------------------
//! @file   lifetime_data.h
//! @brief  ECS LifetimeData - 寿命管理データ
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/component_data.h"

namespace ECS {

//============================================================================
//! @brief 寿命管理データ（POD構造体）
//!
//! エンティティの残り寿命を管理。弾、パーティクル、エフェクトなどの
//! 自動破棄に使用。
//!
//! @note メモリレイアウト: 4 bytes
//!
//! @code
//! // 5秒後に消える弾を作成
//! auto bullet = world.CreateActor();
//! world.AddComponent<LifetimeData>(bullet, LifetimeData{5.0f});
//!
//! // LifetimeSystem
//! world.ForEach<LifetimeData>([&](Actor e, LifetimeData& life) {
//!     life.remainingTime -= dt;
//!     if (life.IsExpired()) {
//!         world.DestroyActor(e);
//!     }
//! });
//! @endcode
//============================================================================
struct LifetimeData : public IComponentData {
    float remainingTime = 0.0f;  //!< 残り時間（秒）

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------
    LifetimeData() = default;

    explicit LifetimeData(float seconds) noexcept
        : remainingTime(seconds) {}

    //------------------------------------------------------------------------
    // ファクトリメソッド
    //------------------------------------------------------------------------

    //! @brief 即時破棄（次フレームで削除）
    [[nodiscard]] static LifetimeData Immediate() noexcept {
        return LifetimeData(0.0f);
    }

    //! @brief 1秒後に破棄
    [[nodiscard]] static LifetimeData OneSecond() noexcept {
        return LifetimeData(1.0f);
    }

    //! @brief 指定フレーム数後に破棄（60FPS想定）
    [[nodiscard]] static LifetimeData Frames(int frames) noexcept {
        return LifetimeData(static_cast<float>(frames) / 60.0f);
    }

    //------------------------------------------------------------------------
    // ヘルパー関数
    //------------------------------------------------------------------------

    //! @brief 寿命が尽きたか
    [[nodiscard]] bool IsExpired() const noexcept {
        return remainingTime <= 0.0f;
    }

    //! @brief 残り時間がまだあるか
    [[nodiscard]] bool IsAlive() const noexcept {
        return remainingTime > 0.0f;
    }

    //! @brief 時間を消費
    //! @param dt デルタタイム（秒）
    //! @return 寿命が尽きたらtrue
    bool Tick(float dt) noexcept {
        remainingTime -= dt;
        return IsExpired();
    }

    //! @brief 寿命を延長
    //! @param seconds 追加時間（秒）
    void Extend(float seconds) noexcept {
        remainingTime += seconds;
    }

    //! @brief 寿命をリセット
    //! @param seconds 新しい寿命（秒）
    void Reset(float seconds) noexcept {
        remainingTime = seconds;
    }

    //! @brief 残り寿命の割合を取得（初期値が必要）
    //! @param initialTime 初期寿命
    //! @return 0.0-1.0（0=期限切れ、1=開始直後）
    [[nodiscard]] float GetNormalizedRemaining(float initialTime) const noexcept {
        if (initialTime <= 0.0f) return 0.0f;
        return remainingTime / initialTime;
    }
};

// コンパイル時検証
ECS_COMPONENT(LifetimeData);
static_assert(sizeof(LifetimeData) == 4, "LifetimeData must be 4 bytes");

} // namespace ECS

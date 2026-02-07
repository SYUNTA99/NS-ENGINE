//----------------------------------------------------------------------------
//! @file   animator_system.h
//! @brief  ECS AnimatorSystem - スプライトアニメーション更新
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/animation/animator_data.h"

namespace ECS {

//============================================================================
//! @brief AnimatorSystem（クラスベース）
//!
//! AnimatorData のフレームカウンタを更新する。
//! SpriteRenderSystem より前に実行する必要がある。
//!
//! @note 優先度5（TransformSystemより後、描画より前）
//============================================================================
class AnimatorSystem final : public ISystem {
public:
    //------------------------------------------------------------------------
    //! @brief システム実行
    //------------------------------------------------------------------------
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        world.ForEach<AnimatorData>([](Actor, AnimatorData& anim) {
            // 再生中でなければスキップ
            if (!anim.IsPlaying()) {
                return;
            }

            // 現在行のフレーム間隔を取得
            uint8_t interval = anim.GetCurrentRowInterval();

            // カウンタをインクリメント
            ++anim.counter;

            // 間隔に達したらフレーム進行
            if (anim.counter >= interval) {
                anim.counter = 0;

                // 現在行の有効フレーム数
                uint8_t frameCount = anim.GetCurrentRowFrameCount();

                // 次のフレームへ
                ++anim.currentCol;

                // フレーム終端に達したら
                if (anim.currentCol >= frameCount) {
                    if (anim.IsLooping()) {
                        // ループ: 先頭に戻る
                        anim.currentCol = 0;
                    } else {
                        // 非ループ: 最終フレームで停止
                        anim.currentCol = static_cast<uint8_t>(frameCount - 1);
                        anim.SetPlaying(false);
                    }
                }
            }
        });
    }

    int Priority() const override { return 5; }
    const char* Name() const override { return "AnimatorSystem"; }
};

} // namespace ECS

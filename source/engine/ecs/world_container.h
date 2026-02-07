//----------------------------------------------------------------------------
//! @file   world_container.h
//! @brief  WorldContainer - ECS/GameObjectの統合コンテナ
//----------------------------------------------------------------------------
#pragma once


#include "chunk_storage.h"
#include "ecs_container.h"
#include "game_object_container.h"
#include "system_scheduler.h"
#include "deferred_queue.h"
#include "common/utility/non_copyable.h"

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief WorldContainer
//!
//! ECS/GameObjectの全データを統合管理するアンマネージドコンテナ。
//! Unity DOTSのWorld相当。
//!
//! 責務:
//! - ChunkStorage: 全チャンクの一元管理
//! - ECSContainer: Actor/Component/Hierarchy管理
//! - GameObjectContainer: OOP GameObject管理
//! - SystemScheduler: System管理と実行
//! - DeferredQueue: 遅延操作キュー
//!
//! @code
//! WorldContainer container;
//! container.Initialize(&world);
//!
//! // ECS操作
//! auto actor = container.ECS().Create();
//! container.ECS().Add<TransformData>(actor, pos);
//!
//! // GameObject操作
//! auto* go = container.GameObjects().Create("Player");
//!
//! // System登録
//! container.Systems().Register<MovementSystem>(world);
//!
//! // フレーム制御
//! container.BeginFrame();
//! container.FixedUpdate(dt);
//! container.Render(alpha);
//! container.EndFrame();
//! @endcode
//============================================================================
class WorldContainer : private NonCopyable {
public:
    WorldContainer() = default;
    ~WorldContainer() = default;

    //------------------------------------------------------------------------
    //! @brief 初期化
    //! @param world 親Worldへのポインタ
    //------------------------------------------------------------------------
    void Initialize(World* world) {
        world_ = world;
        gameObjects_.Initialize(world);
    }

    //========================================================================
    // コンテナアクセス
    //========================================================================

    //! ChunkStorage（チャンク一元管理）
    [[nodiscard]] ChunkStorage& Chunks() noexcept { return chunks_; }
    [[nodiscard]] const ChunkStorage& Chunks() const noexcept { return chunks_; }

    //! ECSContainer（Actor/Component/Hierarchy管理）
    [[nodiscard]] ECSContainer& ECS() noexcept { return ecs_; }
    [[nodiscard]] const ECSContainer& ECS() const noexcept { return ecs_; }

    //! GameObjectContainer（OOP GameObject管理）
    [[nodiscard]] GameObjectContainer& GameObjects() noexcept { return gameObjects_; }
    [[nodiscard]] const GameObjectContainer& GameObjects() const noexcept { return gameObjects_; }

    //! SystemScheduler（System管理）
    [[nodiscard]] SystemScheduler& Systems() noexcept { return systems_; }
    [[nodiscard]] const SystemScheduler& Systems() const noexcept { return systems_; }

    //! DeferredQueue（遅延操作キュー）
    [[nodiscard]] DeferredQueue& Deferred() noexcept { return deferred_; }
    [[nodiscard]] const DeferredQueue& Deferred() const noexcept { return deferred_; }

    //========================================================================
    // フレーム制御
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief フレーム開始処理
    //!
    //! 遅延操作の実行、キャッシュ無効化などを行う。
    //------------------------------------------------------------------------
    void BeginFrame();

    //------------------------------------------------------------------------
    //! @brief フレーム終了処理
    //!
    //! フレームカウンタの更新などを行う。
    //------------------------------------------------------------------------
    void EndFrame();

    //------------------------------------------------------------------------
    //! @brief 固定タイムステップ更新
    //! @param dt 固定デルタタイム
    //------------------------------------------------------------------------
    void FixedUpdate(float dt);

    //------------------------------------------------------------------------
    //! @brief 可変タイムステップ更新
    //! @param dt デルタタイム
    //------------------------------------------------------------------------
    void Update(float dt);

    //------------------------------------------------------------------------
    //! @brief 描画処理
    //! @param alpha 補間係数（0.0〜1.0）
    //------------------------------------------------------------------------
    void Render(float alpha);

    //========================================================================
    // 状態
    //========================================================================

    //! フレームカウンタを取得
    [[nodiscard]] uint32_t GetFrameCount() const noexcept { return frameCount_; }

    //========================================================================
    // クリア
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 全データをクリア（System以外）
    //------------------------------------------------------------------------
    void Clear();

    //------------------------------------------------------------------------
    //! @brief 完全クリア（Systemも含む）
    //------------------------------------------------------------------------
    void ClearAll();

private:
    World* world_ = nullptr;        //!< 親World参照

    ChunkStorage chunks_;           //!< チャンク一元管理
    ECSContainer ecs_;              //!< Actor/Component管理
    GameObjectContainer gameObjects_; //!< GameObject管理
    SystemScheduler systems_;       //!< System管理
    DeferredQueue deferred_;        //!< 遅延操作キュー

    uint32_t frameCount_ = 0;       //!< フレームカウンタ
};

} // namespace ECS

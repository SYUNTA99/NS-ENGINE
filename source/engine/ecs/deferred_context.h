//----------------------------------------------------------------------------
//! @file   deferred_context.h
//! @brief  ECS DeferredContext - 遅延操作のFluent API
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include "deferred_queue.h"
#include <vector>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief DeferredContext
//!
//! 遅延操作の流暢なAPIを提供する軽量クラス。
//! World::Deferred()から取得し、チェーンで操作を記述できる。
//!
//! @code
//! world.Deferred()
//!     .AddComponent<PositionData>(actor1, 1.0f, 2.0f, 3.0f)
//!     .AddComponent<VelocityData>(actor1, 10.0f, 0.0f, 0.0f)
//!     .DestroyActor(actor2);
//! @endcode
//!
//! @note インスタンスは軽量（ポインタ2つ）でコピー/ムーブ可能。
//!       World/DeferredQueueの寿命は呼び出し側が保証すること。
//============================================================================
class DeferredContext {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ（World内部から呼び出し）
    //! @param world Worldへの参照
    //! @param queue DeferredQueueへの参照
    //------------------------------------------------------------------------
    DeferredContext(World& world, DeferredQueue& queue) noexcept
        : world_(&world), queue_(&queue) {}

    // コピー/ムーブ許可（軽量オブジェクト）
    DeferredContext(const DeferredContext&) = default;
    DeferredContext& operator=(const DeferredContext&) = default;
    DeferredContext(DeferredContext&&) = default;
    DeferredContext& operator=(DeferredContext&&) = default;

    //------------------------------------------------------------------------
    //! @brief コンポーネントを遅延追加
    //! @tparam T コンポーネント型
    //! @tparam Args コンストラクタ引数の型
    //! @param actor 追加先のActor
    //! @param args コンストラクタ引数
    //! @return 自身への参照（チェーン可能）
    //!
    //! @code
    //! world.Deferred().AddComponent<TransformData>(actor, pos, rot, scale);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    DeferredContext& AddComponent(Actor actor, Args&&... args);

    //------------------------------------------------------------------------
    //! @brief コンポーネントを遅延削除
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //! @return 自身への参照（チェーン可能）
    //!
    //! @code
    //! world.Deferred().RemoveComponent<SpriteData>(actor);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    DeferredContext& RemoveComponent(Actor actor);

    //------------------------------------------------------------------------
    //! @brief Actorを遅延破棄
    //! @param actor 破棄するActor
    //! @return 自身への参照（チェーン可能）
    //!
    //! @code
    //! world.Deferred().DestroyActor(enemy);
    //! @endcode
    //------------------------------------------------------------------------
    DeferredContext& DestroyActor(Actor actor);

    //------------------------------------------------------------------------
    //! @brief 複数Actorを遅延破棄
    //! @param actors 破棄するActor配列
    //! @return 自身への参照（チェーン可能）
    //!
    //! @code
    //! world.Deferred().DestroyActors(expiredBullets);
    //! @endcode
    //------------------------------------------------------------------------
    DeferredContext& DestroyActors(const std::vector<Actor>& actors);

    //------------------------------------------------------------------------
    //! @brief キュー内の操作数を取得
    //! @return 未実行の操作数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t PendingCount() const noexcept {
        return queue_->Size();
    }

    //------------------------------------------------------------------------
    //! @brief キューが空か確認
    //! @return 空ならtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsEmpty() const noexcept {
        return queue_->IsEmpty();
    }

private:
    World* world_;
    DeferredQueue* queue_;
};

} // namespace ECS

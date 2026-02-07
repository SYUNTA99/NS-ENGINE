//----------------------------------------------------------------------------
//! @file   deferred_queue_impl.h
//! @brief  DeferredQueue テンプレート実装
//! @note   このファイルはworld.hの末尾でインクルードされる（循環依存回避）
//----------------------------------------------------------------------------
#pragma once


#include "deferred_queue.h"
// Worldの完全な定義が必要（world.hからインクルードされるため利用可能）

namespace ECS {

//----------------------------------------------------------------------------
template<typename T>
void DeferredQueue::PushAdd(Actor actor, T&& component) {
    using DecayedT = std::decay_t<T>;

    DeferredOp op;
    op.type = DeferredOpType::AddComponent;
    op.actor = actor;
    op.componentType = std::type_index(typeid(DecayedT));
    op.componentSize = sizeof(DecayedT);
    op.componentAlignment = alignof(DecayedT);

    // コンポーネントデータをコピー
    op.componentData = std::make_unique<std::byte[]>(sizeof(DecayedT));
    new (op.componentData.get()) DecayedT(std::forward<T>(component));

    // デストラクタを保存（キャンセル時用）
    op.destructor = [](void* ptr) {
        static_cast<DecayedT*>(ptr)->~DecayedT();
    };

    // 適用関数を保存（BeginFrameで呼ばれる）
    // 型Tはラムダのキャプチャでなくテンプレートインスタンス化時に決定
    op.applier = [](World& world, Actor a, void* data) {
        DecayedT* comp = static_cast<DecayedT*>(data);
        // 即時版AddComponentを呼び出し（ムーブ）
        world.template AddComponent<DecayedT>(a, std::move(*comp));
    };

    queue_.push_back(std::move(op));
}

//----------------------------------------------------------------------------
template<typename T>
void DeferredQueue::PushRemove(Actor actor) {
    DeferredOp op;
    op.type = DeferredOpType::RemoveComponent;
    op.actor = actor;
    op.componentType = std::type_index(typeid(T));

    // 削除関数を保存（BeginFrameで呼ばれる）
    op.remover = [](World& world, Actor a) {
        world.template RemoveComponent<T>(a);
    };

    queue_.push_back(std::move(op));
}

} // namespace ECS

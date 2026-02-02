//----------------------------------------------------------------------------
//! @file   world.cpp
//! @brief  ECS World - OOP互換API実装
//----------------------------------------------------------------------------
#include "world.h"
#include "engine/game_object/game_object.h"
#include "common/logging/logging.h"
#include <algorithm>
#include <format>

namespace ECS {

//----------------------------------------------------------------------------
// 特殊メンバー関数（unique_ptr<GameObject>のため完全型が必要）
//----------------------------------------------------------------------------
World::World() {
    // WorldContainerを初期化（自身への参照を渡す）
    container_.Initialize(this);
}

World::~World() = default;

//----------------------------------------------------------------------------
//! @brief Actorのワールド行列を取得（オンデマンド計算）
//----------------------------------------------------------------------------
Matrix World::GetWorldMatrix(Actor actor) const {
    if (!container_.ECS().IsAlive(actor)) {
        return Matrix::Identity;
    }

    // LocalToWorldがあればそれを使用
    auto* ltw = GetComponent<LocalToWorld>(actor);
    if (ltw) {
        return ltw->value;
    }

    // なければLocalTransformから計算
    auto* transform = GetComponent<LocalTransform>(actor);
    Matrix localMatrix = transform ? transform->ToMatrix() : Matrix::Identity;

    // PostTransformMatrixがあれば適用
    auto* postTransform = GetComponent<PostTransformMatrix>(actor);
    if (postTransform) {
        localMatrix = localMatrix * postTransform->value;
    }

    // 親がいれば親のワールド行列を乗算
    auto* parentData = GetComponent<Parent>(actor);
    if (parentData && parentData->HasParent() && container_.ECS().IsAlive(parentData->value)) {
        Matrix parentWorld = GetWorldMatrix(parentData->value);
        return localMatrix * parentWorld;
    }

    return localMatrix;
}

//----------------------------------------------------------------------------
//! @brief 指定時間後にActorを破棄
//----------------------------------------------------------------------------
void World::DestroyAfter(Actor actor, float delaySeconds) {
    if (!container_.ECS().IsAlive(actor)) {
        return;
    }

    if (delaySeconds <= 0.0f) {
        DestroyActor(actor);
        return;
    }

    deferredDestroys_.push_back({actor, delaySeconds});
}

//----------------------------------------------------------------------------
//! @brief 遅延破棄をキャンセル
//----------------------------------------------------------------------------
bool World::CancelDestroyAfter(Actor actor) {
    auto it = std::find_if(deferredDestroys_.begin(), deferredDestroys_.end(),
        [actor](const DeferredDestroy& d) { return d.actor == actor; });

    if (it != deferredDestroys_.end()) {
        deferredDestroys_.erase(it);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------
//! @brief 遅延破棄を処理
//----------------------------------------------------------------------------
void World::ProcessDeferredDestroys(float dt) {
    for (auto it = deferredDestroys_.begin(); it != deferredDestroys_.end(); ) {
        it->remainingTime -= dt;
        if (it->remainingTime <= 0.0f) {
            DestroyActor(it->actor);
            it = deferredDestroys_.erase(it);
        } else {
            ++it;
        }
    }
}

//----------------------------------------------------------------------------
//! @brief フレーム開始処理
//!
//! 遅延操作を例外安全に実行する。個別の操作で例外が発生しても
//! 他の操作は継続され、最終的にキューは必ずクリアされる。
//----------------------------------------------------------------------------
void World::BeginFrame() {
    auto& deferred = container_.Deferred();

    // RAIIガードで例外発生時も必ずクリアする
    auto guard = deferred.ScopedClear();

    // 遅延操作を実行
    for (auto& op : deferred.GetQueue()) {
        try {
            switch (op.type) {
            case DeferredOpType::Create:
                // Actor作成は既にCreateActor()で完了済み
                break;

            case DeferredOpType::Destroy:
                // 即時破棄を実行
                DestroyActor(op.actor);
                break;

            case DeferredOpType::AddComponent:
                // コンポーネント追加（applier経由で即時版AddComponentを呼ぶ）
                if (op.applier && container_.ECS().IsAlive(op.actor)) {
                    op.applier(*this, op.actor, op.componentData.get());
                    // applier実行後、moved-from オブジェクトのデストラクタを呼ぶ
                    // （~DeferredOpで呼ばれるが、ここで明示的に呼んで二重呼び出しを防ぐ）
                    if (op.destructor) {
                        op.destructor(op.componentData.get());
                        op.destructor = nullptr;
                    }
                }
                break;

            case DeferredOpType::RemoveComponent:
                // コンポーネント削除（remover経由で即時版RemoveComponentを呼ぶ）
                if (op.remover && container_.ECS().IsAlive(op.actor)) {
                    op.remover(*this, op.actor);
                }
                break;
            }
        } catch (const std::exception& e) {
            // 例外をログに記録し、他の操作は継続
            LOG_ERROR(std::format("Deferred operation failed for Actor {}: {}",
                      op.actor.id, e.what()));
        } catch (...) {
            // 不明な例外
            LOG_ERROR(std::format("Deferred operation failed for Actor {}: unknown exception",
                      op.actor.id));
        }
    }

    // guard のデストラクタで自動クリア

    // WorldContainerのBeginFrameを呼び出し（フレームカウンタ更新など）
    container_.BeginFrame();
}

//----------------------------------------------------------------------------
//! @brief フレーム終了処理
//----------------------------------------------------------------------------
void World::EndFrame() {
    // 現在は特に処理なし（将来の拡張用）
}

//----------------------------------------------------------------------------
//! @brief ゲームオブジェクトを生成
//----------------------------------------------------------------------------
GameObject* World::CreateGameObject(const std::string& name) {
    return container_.GameObjects().Create(name);
}

//----------------------------------------------------------------------------
//! @brief ゲームオブジェクトを破棄
//----------------------------------------------------------------------------
void World::DestroyGameObject(GameObject* go) {
    container_.GameObjects().Destroy(go);
}

//----------------------------------------------------------------------------
//! @brief 名前でゲームオブジェクトを検索
//----------------------------------------------------------------------------
GameObject* World::FindGameObject(const std::string& name) {
    return container_.GameObjects().Find(name);
}

//----------------------------------------------------------------------------
//! @brief 全アクターとコンポーネントをクリア
//----------------------------------------------------------------------------
void World::Clear() {
    deferredDestroys_.clear();  // 遅延破棄もクリア
    container_.Clear();  // ECS/GameObject/Deferredをクリア
    // Systemは保持
}

//----------------------------------------------------------------------------
//! @brief Systemもクリア
//----------------------------------------------------------------------------
void World::ClearAll() {
    Clear();
    container_.ClearAll();  // Systemもクリア
    systemsById_.clear();
    renderSystemsById_.clear();
    sortedSystems_.clear();
    sortedRenderSystems_.clear();
    systemGraph_.Clear();
    renderSystemGraph_.Clear();
    systemsDirty_ = false;
    renderSystemsDirty_ = false;
}

//----------------------------------------------------------------------------
//! @brief SystemEntryを登録
//----------------------------------------------------------------------------
void World::CommitSystem(SystemEntry entry) {
    SystemId id = entry.id;

    // グラフにIDと依存関係情報を追加
    systemGraph_.AddNode(id, entry.priority, entry.runAfter, entry.runBefore, entry.name);

    // System実体をマップに格納
    systemsById_[id] = std::move(entry.system);

    // 再構築が必要
    systemsDirty_ = true;
}

//----------------------------------------------------------------------------
//! @brief RenderSystemEntryを登録
//----------------------------------------------------------------------------
void World::CommitRenderSystem(RenderSystemEntry entry) {
    SystemId id = entry.id;

    // グラフにIDと依存関係情報を追加
    renderSystemGraph_.AddNode(id, entry.priority, entry.runAfter, entry.runBefore, entry.name);

    // RenderSystem実体をマップに格納
    renderSystemsById_[id] = std::move(entry.system);

    // 再構築が必要
    renderSystemsDirty_ = true;
}

//----------------------------------------------------------------------------
//! @brief Systemのソート済み配列を再構築
//----------------------------------------------------------------------------
void World::RebuildSortedSystems() {
    sortedSystems_.clear();

    std::vector<SystemId> sorted = systemGraph_.TopologicalSort();
    sortedSystems_.reserve(sorted.size());

    for (SystemId id : sorted) {
        auto it = systemsById_.find(id);
        if (it != systemsById_.end() && it->second) {
            sortedSystems_.push_back(it->second.get());
        }
    }

    systemsDirty_ = false;
}

//----------------------------------------------------------------------------
//! @brief RenderSystemのソート済み配列を再構築
//----------------------------------------------------------------------------
void World::RebuildSortedRenderSystems() {
    sortedRenderSystems_.clear();

    std::vector<SystemId> sorted = renderSystemGraph_.TopologicalSort();
    sortedRenderSystems_.reserve(sorted.size());

    for (SystemId id : sorted) {
        auto it = renderSystemsById_.find(id);
        if (it != renderSystemsById_.end() && it->second) {
            sortedRenderSystems_.push_back(it->second.get());
        }
    }

    renderSystemsDirty_ = false;
}

} // namespace ECS

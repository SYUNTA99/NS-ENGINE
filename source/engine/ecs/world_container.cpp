//----------------------------------------------------------------------------
//! @file   world_container.cpp
//! @brief  WorldContainer - ECS/GameObjectの統合コンテナ実装
//----------------------------------------------------------------------------

#include "world_container.h"
#include "world.h"

namespace ECS {

//----------------------------------------------------------------------------
void WorldContainer::BeginFrame() {
    // フレームカウンタを更新（キャッシュ無効化用）
    ++frameCount_;
}

//----------------------------------------------------------------------------
void WorldContainer::EndFrame() {
    // 現在は特に処理なし（将来の拡張用）
}

//----------------------------------------------------------------------------
void WorldContainer::FixedUpdate(float dt) {
    if (!world_) return;

    // GameObjectのFixedUpdate
    gameObjects_.FixedUpdateAll(dt);
}

//----------------------------------------------------------------------------
void WorldContainer::Update(float dt) {
    if (!world_) return;

    // Start()待ちコンポーネントを処理（最初のUpdate前に呼ばれる）
    gameObjects_.ProcessPendingStarts();

    // Systemの更新
    systems_.Update(*world_, dt);

    // GameObjectのUpdate
    gameObjects_.UpdateAll(dt);
}

//----------------------------------------------------------------------------
void WorldContainer::Render(float alpha) {
    if (!world_) return;

    // RenderSystemの実行
    systems_.Render(*world_, alpha);
}

//----------------------------------------------------------------------------
void WorldContainer::Clear() {
    // GameObjectをクリア（ECS Actorも破棄される）
    gameObjects_.Clear();

    // ECSデータをクリア
    ecs_.Clear();

    // 遅延操作をクリア
    deferred_.Clear();

    // ChunkStorageはクリアしない（再利用のため）
    // 必要に応じてchunks_.Trim()で未使用チャンクを解放
}

//----------------------------------------------------------------------------
void WorldContainer::ClearAll() {
    // Systemを破棄
    if (world_) {
        systems_.DestroyAll(*world_);
    }
    systems_.Clear();

    // 通常のクリア
    Clear();

    // ChunkStorageもクリア
    chunks_.Clear();

    // フレームカウンタをリセット
    frameCount_ = 0;
}

} // namespace ECS

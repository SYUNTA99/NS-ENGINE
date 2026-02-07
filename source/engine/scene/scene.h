//----------------------------------------------------------------------------
//! @file   scene.h
//! @brief  シーン基底クラス
//----------------------------------------------------------------------------
#pragma once


#include "common/stl/stl_common.h"
#include "common/stl/stl_threading.h"
#include "engine/texture/texture_types.h"
#include "engine/texture/texture_manager.h"
#include "engine/ecs/world.h"

//----------------------------------------------------------------------------
//! @brief シーン基底クラス
//!
//! ゲームの各画面（タイトル、ゲーム、リザルト等）の基底クラス。
//! 派生クラスで各仮想関数をオーバーライドして実装する。
//----------------------------------------------------------------------------
class Scene
{
public:
    Scene() = default;
    virtual ~Scene() noexcept = default;

    // コピー禁止、ムーブ許可
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    //----------------------------------------------------------
    //! @name ライフサイクル
    //----------------------------------------------------------
    //!@{

    //! @brief シーン開始時に呼ばれる
    virtual void OnEnter() {}

    //! @brief シーン終了時に呼ばれる
    virtual void OnExit() {}

    //!@}
    //----------------------------------------------------------
    //! @name 非同期ロード
    //----------------------------------------------------------
    //!@{

    //! @brief 非同期ロード処理（バックグラウンドスレッドで実行）
    //! @note テクスチャ、シェーダー等の重いリソースをここでロード
    //! @note D3D11はスレッドセーフなのでGPUリソース作成可能
    virtual void OnLoadAsync() {}

    //! @brief 非同期ロード完了後、メインスレッドで呼ばれる
    //! @note OnEnter()の前に呼ばれる
    virtual void OnLoadComplete() {}

    //! @brief ロード進捗を設定（0.0〜1.0）
    void SetLoadProgress(float progress) { loadProgress_.store(std::clamp(progress, 0.0f, 1.0f)); }

    //! @brief ロード進捗を取得
    [[nodiscard]] float GetLoadProgress() const { return loadProgress_.load(); }

    //!@}

    //----------------------------------------------------------
    //! @name フレームコールバック
    //----------------------------------------------------------
    //!@{

    //! @brief 固定タイムステップ更新（ECS推奨）
    //! @param dt 固定デルタタイム（通常1/60秒）
    //! @note 物理・ロジック更新に使用。Worldがある場合は自動でFixedUpdateが呼ばれる
    virtual void FixedUpdate([[maybe_unused]] float dt) {
        // デフォルト実装: Worldがあれば更新
        if (world_) {
            world_->FixedUpdate(dt);
        }
    }

    //! @brief 更新処理（従来互換・非推奨）
    //! @deprecated FixedUpdate()を使用してください
    virtual void Update() {}

    //! @brief 描画処理
    //! @param alpha 補間係数（0.0〜1.0）固定タイムステップ使用時
    virtual void Render([[maybe_unused]] float alpha = 1.0f) {
        // デフォルト実装: Worldがあれば描画
        if (world_) {
            world_->Render(alpha);
        }
    }

    //!@}

    //----------------------------------------------------------
    //! @name プロパティ
    //----------------------------------------------------------
    //!@{

    //! @brief シーン名を取得
    [[nodiscard]] virtual const char* GetName() const { return "Scene"; }

    //!@}

    //----------------------------------------------------------
    //! @name テクスチャスコープ管理（内部用）
    //----------------------------------------------------------
    //!@{

    //! @brief テクスチャスコープIDを設定（SceneManagerから呼ばれる）
    void SetTextureScopeId(TextureManager::ScopeId scopeId) noexcept { textureScopeId_ = scopeId; }

    //! @brief テクスチャスコープIDを取得
    [[nodiscard]] TextureManager::ScopeId GetTextureScopeId() const noexcept { return textureScopeId_; }

    //!@}

    //----------------------------------------------------------
    //! @name ECS World管理
    //----------------------------------------------------------
    //!@{

    //! @brief Worldを取得
    [[nodiscard]] ECS::World* GetWorld() noexcept { return world_.get(); }
    [[nodiscard]] const ECS::World* GetWorld() const noexcept { return world_.get(); }

    //! @brief Worldが存在するか確認
    [[nodiscard]] bool HasWorld() const noexcept { return world_ != nullptr; }

    //!@}

protected:
    //----------------------------------------------------------
    //! @name 派生クラス用ヘルパー
    //----------------------------------------------------------
    //!@{

    //! @brief Worldを初期化
    //! @note OnEnter()内で呼び出す
    void InitializeWorld() {
        world_ = std::make_unique<ECS::World>();
    }

    //! @brief Worldへの参照を取得（初期化済み前提）
    [[nodiscard]] ECS::World& GetWorldRef() {
        assert(world_ && "World not initialized. Call InitializeWorld() first.");
        return *world_;
    }

    //!@}

    std::unique_ptr<ECS::World> world_;  //!< ECS World（オプション）

private:
    std::atomic<float> loadProgress_{ 0.0f };  //!< ロード進捗（0.0〜1.0）
    TextureManager::ScopeId textureScopeId_ = TextureManager::kGlobalScope;  //!< テクスチャスコープID
};

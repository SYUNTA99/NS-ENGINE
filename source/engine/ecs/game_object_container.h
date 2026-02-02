//----------------------------------------------------------------------------
//! @file   game_object_container.h
//! @brief  GameObjectContainer - OOP GameObjectの管理
//----------------------------------------------------------------------------
#pragma once

#include "engine/game_object/game_object.h"
#include "common/utility/non_copyable.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ECS {

class World;

//============================================================================
//! @brief GameObjectContainer
//!
//! OOP StyleのGameObjectを管理するコンテナ。
//! ECSContainerとは独立して、GameObjectの生成/破棄/検索を行う。
//!
//! @note GameObjectはECS Actorをラップし、OOPコンポーネントを格納する。
//!       ECSデータはECSContainer側で管理される。
//!
//! @code
//! GameObjectContainer container;
//! container.Initialize(&world);
//!
//! auto* player = container.Create("Player");
//! player->AddECS<TransformData>(pos, rot, scale);
//! player->AddComponent<PlayerController>();
//!
//! auto* found = container.Find("Player");
//! container.Destroy(player);
//! @endcode
//============================================================================
class GameObjectContainer : private NonCopyable {
public:
    GameObjectContainer() = default;
    ~GameObjectContainer() { Clear(); }

    //------------------------------------------------------------------------
    //! @brief 初期化
    //! @param world ECS Worldへのポインタ（Actor生成用）
    //------------------------------------------------------------------------
    void Initialize(World* world) {
        world_ = world;
    }

    //------------------------------------------------------------------------
    //! @brief ゲームオブジェクトを生成
    //! @param name オブジェクト名（デフォルト: "GameObject"）
    //! @return 生成されたGameObjectへのポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] GameObject* Create(const std::string& name = "GameObject");

    //------------------------------------------------------------------------
    //! @brief ゲームオブジェクトを破棄
    //! @param gameObject 破棄するGameObject
    //------------------------------------------------------------------------
    void Destroy(GameObject* gameObject);

    //------------------------------------------------------------------------
    //! @brief Actorに対応するGameObjectを破棄
    //! @param actor 破棄するActor
    //------------------------------------------------------------------------
    void Destroy(Actor actor);

    //------------------------------------------------------------------------
    //! @brief 名前でゲームオブジェクトを検索
    //! @param name 検索する名前
    //! @return 見つかった場合はGameObjectへのポインタ、なければnullptr
    //! @note 同名が複数ある場合は最初に見つかったものを返す
    //------------------------------------------------------------------------
    [[nodiscard]] GameObject* Find(const std::string& name);

    //------------------------------------------------------------------------
    //! @brief Actorに対応するGameObjectを取得
    //! @param actor 検索するActor
    //! @return 見つかった場合はGameObjectへのポインタ、なければnullptr
    //------------------------------------------------------------------------
    [[nodiscard]] GameObject* GetByActor(Actor actor);

    //------------------------------------------------------------------------
    //! @brief Start()待ちのコンポーネントを処理
    //!
    //! 各フレームの最初（BeginFrame内）で呼び出される。
    //! まだStart()が呼ばれていないコンポーネントに対してStart()を呼び出す。
    //------------------------------------------------------------------------
    void ProcessPendingStarts();

    //------------------------------------------------------------------------
    //! @brief コンポーネントをStart待ちキューに追加
    //! @param comp Start()待ちのコンポーネント
    //!
    //! AddComponent後に呼び出される。
    //------------------------------------------------------------------------
    void RegisterForStart(Component* comp) {
        pendingStarts_.push_back(comp);
    }

    //------------------------------------------------------------------------
    //! @brief 全GameObjectのOOPコンポーネントを更新
    //! @param dt デルタタイム
    //------------------------------------------------------------------------
    void UpdateAll(float dt);

    //------------------------------------------------------------------------
    //! @brief 全GameObjectのOOPコンポーネントをFixedUpdate
    //! @param dt 固定デルタタイム
    //------------------------------------------------------------------------
    void FixedUpdateAll(float dt);

    //------------------------------------------------------------------------
    //! @brief 全GameObjectをクリア
    //------------------------------------------------------------------------
    void Clear();

    //------------------------------------------------------------------------
    //! @brief GameObjectの数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Count() const noexcept {
        return gameObjects_.size();
    }

    //------------------------------------------------------------------------
    //! @brief 全GameObjectへのイテレーション
    //! @param func 各GameObjectに対して呼び出す関数
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEach(Func&& func) {
        for (auto& go : gameObjects_) {
            if (go) {
                func(go.get());
            }
        }
    }

    template<typename Func>
    void ForEach(Func&& func) const {
        for (const auto& go : gameObjects_) {
            if (go) {
                func(go.get());
            }
        }
    }

private:
    World* world_ = nullptr;

    //! 所有するGameObject
    std::vector<std::unique_ptr<GameObject>> gameObjects_;

    //! Actor → GameObjectインデックスのマッピング
    std::unordered_map<uint32_t, size_t> actorToIndex_;

    //! 再利用可能なインデックスのリスト
    std::vector<size_t> freeIndices_;

    //! Start()待ちコンポーネントキュー
    std::vector<Component*> pendingStarts_;
};

} // namespace ECS

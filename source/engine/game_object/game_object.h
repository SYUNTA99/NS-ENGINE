//----------------------------------------------------------------------------
//! @file   game_object.h
//! @brief  ゲームオブジェクト - ECS Actorのラッパー + OOPコンポーネント
//----------------------------------------------------------------------------
#pragma once


#include "engine/ecs/actor.h"
#include "engine/ecs/component.h"
#include "engine/ecs/component_cache.h"
#include "engine/game_object/message.h"
#include <cassert>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ECS {
class World;
}

//============================================================================
//! @brief ゲームオブジェクトクラス
//!
//! ECS ActorのラッパーとOOPコンポーネントのコンテナとして動作。
//!
//! Hybrid OOP-ECS Architecture:
//! - ECSデータ（PositionData, SpriteData等）: 高速バッチ処理用
//! - OOPコンポーネント（PlayerController等）: 複雑なロジック用
//!
//! @code
//! auto* go = world.CreateGameObject("Player");
//!
//! // ECSデータ追加（高速、キャッシュフレンドリー）
//! go->AddECS<PositionData>(Vector3::Zero);
//! go->AddECS<SpriteData>(textureHandle);
//!
//! // OOPコンポーネント追加（複雑なロジック）
//! go->AddComponent<PlayerController>();
//!
//! // ECSデータアクセス（フレームキャッシュ経由）
//! auto& pos = go->GetECS<PositionData>();
//! pos.value.x += 10.0f;
//!
//! // OOPコンポーネントアクセス
//! auto* ctrl = go->GetComponent<PlayerController>();
//! if (ctrl) ctrl->SetSpeed(100.0f);
//! @endcode
//============================================================================
class GameObject {
public:
    //------------------------------------------------------------------------
    //! @brief コンストラクタ
    //! @param world ECS Worldへのポインタ
    //! @param actor ECS Actor
    //! @param name オブジェクト名
    //------------------------------------------------------------------------
    GameObject(ECS::World* world, ECS::Actor actor, const std::string& name = "GameObject")
        : world_(world)
        , actor_(actor)
        , name_(name)
    {}

    ~GameObject();

    // コピー禁止、ムーブ許可
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject& operator=(GameObject&&) = default;

    //========================================================================
    // ECSデータ操作（旧API互換）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを追加
    //! @tparam T ECSデータ型（IComponentDataを継承）
    //! @tparam Args コンストラクタ引数の型
    //! @param args コンストラクタ引数
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    void Add(Args&&... args);

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを取得（フレームキャッシュ経由）
    //! @tparam T ECSデータ型
    //! @return コンポーネントへの参照
    //! @note 存在しない場合は未定義動作。Has<T>()で確認すること。
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T& Get();

    template<typename T>
    [[nodiscard]] const T& Get() const;

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを所持しているか確認
    //! @tparam T ECSデータ型
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool Has() const;

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを削除
    //! @tparam T ECSデータ型
    //------------------------------------------------------------------------
    template<typename T>
    void Remove();

    //========================================================================
    // ECSデータ操作（新API: 明示的な命名）
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを追加
    //! @tparam T ECSデータ型（IComponentDataを継承）
    //! @tparam Args コンストラクタ引数の型
    //! @param args コンストラクタ引数
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    void AddECS(Args&&... args);

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを取得
    //! @tparam T ECSデータ型
    //! @return コンポーネントへの参照
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T& GetECS();

    template<typename T>
    [[nodiscard]] const T& GetECS() const;

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを所持しているか確認
    //! @tparam T ECSデータ型
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasECS() const;

    //------------------------------------------------------------------------
    //! @brief ECSデータコンポーネントを削除
    //! @tparam T ECSデータ型
    //------------------------------------------------------------------------
    template<typename T>
    void RemoveECS();

    //========================================================================
    // OOPコンポーネント操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief OOPコンポーネントを追加
    //! @tparam T OOPコンポーネント型（Componentを継承）
    //! @tparam Args コンストラクタ引数の型
    //! @param args コンストラクタ引数
    //! @return 追加されたコンポーネントへのポインタ
    //!
    //! @code
    //! auto* ctrl = go->AddComponent<PlayerController>(100.0f);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    T* AddComponent(Args&&... args);

    //------------------------------------------------------------------------
    //! @brief OOPコンポーネントを取得
    //! @tparam T OOPコンポーネント型
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //!
    //! @code
    //! auto* ctrl = go->GetComponent<PlayerController>();
    //! if (ctrl) ctrl->SetSpeed(200.0f);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponent();

    template<typename T>
    [[nodiscard]] const T* GetComponent() const;

    //------------------------------------------------------------------------
    //! @brief OOPコンポーネントを所持しているか確認
    //! @tparam T OOPコンポーネント型
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] bool HasComponent() const;

    //------------------------------------------------------------------------
    //! @brief OOPコンポーネントを削除
    //! @tparam T OOPコンポーネント型
    //------------------------------------------------------------------------
    template<typename T>
    void RemoveComponent();

    //========================================================================
    // メッセージング
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 同一GameObject内の全コンポーネントにメッセージを送信
    //! @tparam T メッセージ型
    //! @param msg 送信するメッセージ
    //!
    //! @code
    //! DamageMessage dmg(50.0f);
    //! gameObject->SendMsg(dmg);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    void SendMsg(const T& msg);

    //------------------------------------------------------------------------
    //! @brief 同一GameObject内の全コンポーネントにメッセージを送信
    //! @param msg 送信するメッセージ（IMessage参照）
    //------------------------------------------------------------------------
    void SendMsg(const IMessage& msg);

    //------------------------------------------------------------------------
    //! @brief 子階層を含む全コンポーネントにメッセージを送信
    //! @tparam T メッセージ型
    //! @param msg 送信するメッセージ
    //!
    //! 自身と全ての子孫GameObjectにメッセージを送信する。
    //!
    //! @code
    //! DamageMessage dmg(50.0f);
    //! gameObject->BroadcastMsg(dmg);  // 自身と全子孫に送信
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    void BroadcastMsg(const T& msg);

    //------------------------------------------------------------------------
    //! @brief 子階層を含む全コンポーネントにメッセージを送信
    //! @param msg 送信するメッセージ（IMessage参照）
    //------------------------------------------------------------------------
    void BroadcastMsg(const IMessage& msg);

    //------------------------------------------------------------------------
    //! @brief 親階層を含む全コンポーネントにメッセージを送信
    //! @tparam T メッセージ型
    //! @param msg 送信するメッセージ
    //!
    //! 自身と全ての祖先GameObjectにメッセージを送信する。
    //!
    //! @code
    //! DamageMessage dmg(50.0f);
    //! gameObject->SendMsgUpwards(dmg);  // 自身と全祖先に送信
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    void SendMsgUpwards(const T& msg);

    //------------------------------------------------------------------------
    //! @brief 親階層を含む全コンポーネントにメッセージを送信
    //! @param msg 送信するメッセージ（IMessage参照）
    //------------------------------------------------------------------------
    void SendMsgUpwards(const IMessage& msg);

    //========================================================================
    // 階層操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 親GameObjectを設定
    //! @param parent 親GameObject（nullptrでルートに設定）
    //!
    //! @code
    //! auto* child = world.CreateGameObject("Child");
    //! auto* parent = world.CreateGameObject("Parent");
    //! child->SetParent(parent);
    //! @endcode
    //------------------------------------------------------------------------
    void SetParent(GameObject* parent);

    //------------------------------------------------------------------------
    //! @brief 親GameObjectを取得
    //! @return 親GameObjectへのポインタ（ルートの場合はnullptr）
    //------------------------------------------------------------------------
    [[nodiscard]] GameObject* GetParent();

    //------------------------------------------------------------------------
    //! @brief 親GameObjectを取得（const版）
    //------------------------------------------------------------------------
    [[nodiscard]] const GameObject* GetParent() const;

    //------------------------------------------------------------------------
    //! @brief 子GameObjectの数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetChildCount() const;

    //------------------------------------------------------------------------
    //! @brief 子階層からコンポーネントを検索
    //! @tparam T OOPコンポーネント型
    //! @return 最初に見つかったコンポーネント（存在しない場合はnullptr）
    //!
    //! 自身の子、孫...を深さ優先で検索する。
    //!
    //! @code
    //! auto* enemy = parent->GetComponentInChildren<Enemy>();
    //! if (enemy) enemy->TakeDamage(10.0f);
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponentInChildren();

    template<typename T>
    [[nodiscard]] const T* GetComponentInChildren() const;

    //------------------------------------------------------------------------
    //! @brief 親階層からコンポーネントを検索
    //! @tparam T OOPコンポーネント型
    //! @return 最初に見つかったコンポーネント（存在しない場合はnullptr）
    //!
    //! 自身の親、祖父母...をルートまで検索する。
    //!
    //! @code
    //! auto* manager = child->GetComponentInParent<GameManager>();
    //! @endcode
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T* GetComponentInParent();

    template<typename T>
    [[nodiscard]] const T* GetComponentInParent() const;

    //------------------------------------------------------------------------
    //! @brief 全ての子GameObjectに対してアクションを実行
    //! @tparam Func 関数型 void(GameObject*)
    //! @param func 各子に対して呼び出す関数
    //!
    //! @code
    //! parent->ForEachChild([](GameObject* child) {
    //!     child->SetActive(false);
    //! });
    //! @endcode
    //------------------------------------------------------------------------
    template<typename Func>
    void ForEachChild(Func&& func);

    template<typename Func>
    void ForEachChild(Func&& func) const;

    //========================================================================
    // OOPコンポーネント更新
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 全OOPコンポーネントのUpdate()を呼び出す
    //! @param dt デルタタイム（秒）
    //------------------------------------------------------------------------
    void UpdateComponents(float dt);

    //------------------------------------------------------------------------
    //! @brief 全OOPコンポーネントのFixedUpdate()を呼び出す
    //! @param dt 固定デルタタイム
    //------------------------------------------------------------------------
    void FixedUpdateComponents(float dt);

    //------------------------------------------------------------------------
    //! @brief 全OOPコンポーネントのLateUpdate()を呼び出す
    //! @param dt デルタタイム（秒）
    //------------------------------------------------------------------------
    void LateUpdateComponents(float dt);

    //========================================================================
    // アクセサ
    //========================================================================

    //! ECS Actorを取得
    [[nodiscard]] ECS::Actor GetActor() const noexcept { return actor_; }

    //! 名前を取得
    [[nodiscard]] const std::string& GetName() const noexcept { return name_; }

    //! 名前を設定
    void SetName(const std::string& name) { name_ = name; }

    //! アクティブ状態を取得
    [[nodiscard]] bool IsActive() const noexcept { return active_; }

    //! アクティブ状態を設定
    void SetActive(bool active) noexcept { active_ = active; }

    //! ECS Worldを取得
    [[nodiscard]] ECS::World* GetWorld() const noexcept { return world_; }

    //! OOPコンポーネント数を取得
    [[nodiscard]] size_t GetComponentCount() const noexcept { return components_.size(); }

private:
    ECS::World* world_;                  //!< ECS World参照
    ECS::Actor actor_;                   //!< ECS Actor
    mutable ECS::ComponentCache cache_;  //!< ECSフレームキャッシュ
    std::string name_;                   //!< オブジェクト名
    bool active_ = true;                 //!< アクティブ状態

    //! OOPコンポーネントコンテナ
    std::vector<std::unique_ptr<Component>> components_;

    //! 型→インデックスマッピング（O(1)アクセス用）
    std::unordered_map<std::type_index, size_t> componentIndex_;
};

// テンプレート実装はworld.hインクルード後に必要
// game_object_impl.h で定義

//----------------------------------------------------------------------------
//! @file   ecs_container.h
//! @brief  ECSContainer - Actor/Component/Hierarchy管理の統合インターフェース
//----------------------------------------------------------------------------
#pragma once


#include "actor_registry.h"
#include "hierarchy_registry.h"

namespace ECS {

//============================================================================
//! @brief ECSContainer
//!
//! ActorRegistry + HierarchyRegistryを統合したECSコンテナ。
//! Unity DOTSのEntityManagerに相当する。
//!
//! @code
//! ECSContainer ecs;
//!
//! // Actor操作
//! auto actor = ecs.Create();
//! ecs.Add<TransformData>(actor, pos, rot, scale);
//! auto* transform = ecs.Get<TransformData>(actor);
//!
//! // 階層操作
//! auto parent = ecs.Create();
//! auto child = ecs.Create();
//! ecs.SetParent(child, parent);
//! auto parentActor = ecs.GetParent(child);
//! @endcode
//============================================================================
class ECSContainer : private NonCopyable {
public:
    ECSContainer() = default;
    ~ECSContainer() = default;

    //========================================================================
    // ActorRegistry委譲 - Actor操作
    //========================================================================

    //! @brief 新しいアクターを生成
    [[nodiscard]] Actor Create() {
        return actors_.Create();
    }

    //! @brief 複数アクターを一括生成（コンポーネントなし）
    [[nodiscard]] std::vector<Actor> Create(size_t count) {
        return actors_.Create(count);
    }

    //! @brief 複数アクターを一括生成（コンポーネント付き）
    template<typename... Ts>
    [[nodiscard]] std::vector<Actor> Create(size_t count) {
        return actors_.Create<Ts...>(count);
    }

    //! @brief アクターを破棄
    void Destroy(Actor actor) {
        actors_.Destroy(actor);
    }

    //! @brief アクターが生存しているか確認
    [[nodiscard]] bool IsAlive(Actor actor) const {
        return actors_.IsAlive(actor);
    }

    //! @brief 生存アクター数を取得
    [[nodiscard]] size_t Count() const noexcept {
        return actors_.Count();
    }

    //========================================================================
    // ActorRegistry委譲 - Component操作
    //========================================================================

    //! @brief コンポーネントを追加
    template<typename T, typename... Args>
    T* Add(Actor actor, Args&&... args) {
        return actors_.Add<T>(actor, std::forward<Args>(args)...);
    }

    //! @brief コンポーネントを取得
    template<typename T>
    [[nodiscard]] T* Get(Actor actor) {
        return actors_.Get<T>(actor);
    }

    template<typename T>
    [[nodiscard]] const T* Get(Actor actor) const {
        return actors_.Get<T>(actor);
    }

    //! @brief コンポーネントを所持しているか確認
    template<typename T>
    [[nodiscard]] bool Has(Actor actor) const {
        return actors_.Has<T>(actor);
    }

    //! @brief コンポーネントを削除
    template<typename T>
    void Remove(Actor actor) {
        actors_.Remove<T>(actor);
    }

    //========================================================================
    // ActorRegistry委譲 - クエリ
    //========================================================================

    //! @brief 型安全なクエリを構築
    template<typename... AccessModes>
    [[nodiscard]] auto Query() {
        return actors_.Query<AccessModes...>();
    }

    //========================================================================
    // 階層操作
    //========================================================================

    //! @brief 親を設定
    //! @param child 子Actor
    //! @param parent 親Actor（Actor::Invalid()でルートに設定）
    void SetParent(Actor child, Actor parent);

    //! @brief 親を取得
    //! @param actor 対象Actor
    //! @return 親Actor（ルートの場合はActor::Invalid()）
    [[nodiscard]] Actor GetParent(Actor actor) const;

    //! @brief 子を取得
    //! @param parent 親Actor
    //! @return 子Actor配列
    [[nodiscard]] std::vector<Actor> GetChildren(Actor parent) const;

    //! @brief 子の数を取得
    [[nodiscard]] size_t GetChildCount(Actor parent) const;

    //! @brief 子を持っているか確認
    [[nodiscard]] bool HasChildren(Actor parent) const;

    //========================================================================
    // 内部アクセス
    //========================================================================

    //! @brief ActorRegistryへの直接アクセス（内部用）
    [[nodiscard]] ActorRegistry& GetActorRegistry() noexcept {
        return actors_;
    }

    [[nodiscard]] const ActorRegistry& GetActorRegistry() const noexcept {
        return actors_;
    }

    //! @brief HierarchyRegistryへの直接アクセス（内部用）
    [[nodiscard]] HierarchyRegistry& GetHierarchy() noexcept {
        return hierarchy_;
    }

    [[nodiscard]] const HierarchyRegistry& GetHierarchy() const noexcept {
        return hierarchy_;
    }

    //! @brief ArchetypeStorageへのアクセス（内部用）
    [[nodiscard]] ArchetypeStorage& GetArchetypeStorage() noexcept {
        return actors_.GetArchetypeStorage();
    }

    [[nodiscard]] const ArchetypeStorage& GetArchetypeStorage() const noexcept {
        return actors_.GetArchetypeStorage();
    }

    //========================================================================
    // クリア
    //========================================================================

    //! @brief 全データをクリア
    void Clear() {
        actors_.Clear();
        hierarchy_.Clear();
    }

private:
    ActorRegistry actors_;       //!< Actor/Component管理
    HierarchyRegistry hierarchy_; //!< 階層管理
};

} // namespace ECS

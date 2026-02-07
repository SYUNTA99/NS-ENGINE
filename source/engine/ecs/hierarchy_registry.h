//----------------------------------------------------------------------------
//! @file   hierarchy_registry.h
//! @brief  HierarchyRegistry - 親子階層管理
//----------------------------------------------------------------------------
#pragma once


#include "actor.h"
#include "buffer/dynamic_buffer.h"
#include "components/transform/children.h"
#include <vector>

namespace ECS {

class World;

//============================================================================
//! @brief 親子階層レジストリ
//!
//! 親子関係をDynamicBuffer<Child>コンポーネントとして管理。
//! 子リストは親ActorのChunk内にインライン格納される。
//!
//! ECSには以下のコンポーネントで親子情報を格納:
//! - Parent: 親への参照（子のみ持つ）
//! - DynamicBuffer<Child>: 子リスト（親のみ持つ）
//! - HierarchyDepthData: 階層深度（TransformSystemのソート用）
//!
//! @code
//! HierarchyRegistry hierarchy;
//!
//! // 親子関係を構築
//! auto parent = world.CreateActor();
//! auto child1 = world.CreateActor();
//! auto child2 = world.CreateActor();
//!
//! hierarchy.SetParent(child1, parent, world);
//! hierarchy.SetParent(child2, parent, world);
//!
//! // 子を取得（DynamicBuffer経由）
//! auto children = hierarchy.GetChildren(parent, world);
//! for (const auto& child : children) {
//!     // child.value でActorにアクセス
//! }
//! @endcode
//============================================================================
class HierarchyRegistry {
public:
    HierarchyRegistry() = default;
    ~HierarchyRegistry() = default;

    // コピー禁止、ムーブ許可
    HierarchyRegistry(const HierarchyRegistry&) = delete;
    HierarchyRegistry& operator=(const HierarchyRegistry&) = delete;
    HierarchyRegistry(HierarchyRegistry&&) = default;
    HierarchyRegistry& operator=(HierarchyRegistry&&) = default;

    //========================================================================
    // 親子関係操作
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 親を設定
    //! @param child 子Actor
    //! @param parent 親Actor（Actor::Invalid()でルートに設定）
    //! @param world ECS World（Parent/Child/HierarchyDepthData更新用）
    //!
    //! ECSのParent, DynamicBuffer<Child>, HierarchyDepthDataを同時に更新する。
    //------------------------------------------------------------------------
    void SetParent(Actor child, Actor parent, World& world);

    //------------------------------------------------------------------------
    //! @brief 親子関係を解除（ルートに戻す）
    //! @param child 子Actor
    //! @param world ECS World
    //------------------------------------------------------------------------
    void ClearParent(Actor child, World& world);

    //========================================================================
    // 取得
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 子バッファを取得
    //! @param parent 親Actor
    //! @param world ECS World
    //! @return DynamicBuffer<Child>（子がいない場合は空または無効なバッファ）
    //!
    //! @code
    //! auto children = hierarchy.GetChildren(parent, world);
    //! if (children) {
    //!     for (const auto& child : children) {
    //!         // child.value でActorにアクセス
    //!     }
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] DynamicBuffer<Child> GetChildren(Actor parent, World& world) const;

    //------------------------------------------------------------------------
    //! @brief 子を持っているか確認
    //! @param parent 親Actor
    //! @param world ECS World
    //! @return 子が存在する場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] bool HasChildren(Actor parent, World& world) const;

    //------------------------------------------------------------------------
    //! @brief 子の数を取得
    //! @param parent 親Actor
    //! @param world ECS World
    //! @return 子の数
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetChildCount(Actor parent, World& world) const;

    //------------------------------------------------------------------------
    //! @brief ルートActorを取得
    //! @return ルートActor配列
    //!
    //! 親を持たないActorのリスト（HierarchyRootタグ付き）
    //------------------------------------------------------------------------
    [[nodiscard]] const std::vector<Actor>& GetRoots() const {
        return roots_;
    }

    //------------------------------------------------------------------------
    //! @brief ルートActorの数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t GetRootCount() const {
        return roots_.size();
    }

    //========================================================================
    // 階層検証
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief ancestorがdescendantの祖先かどうか判定
    //! @param ancestor 祖先候補のActor
    //! @param descendant 子孫候補のActor
    //! @param world ECS World
    //! @return ancestorがdescendantの祖先（親、祖父母、...）ならtrue
    //!
    //! 循環参照検出に使用。SetParent前にこれをチェックすることで
    //! 循環階層の発生を防止する。
    //!
    //! @code
    //! // A → B → C の階層で
    //! if (hierarchy.IsAncestorOf(C, A, world)) {
    //!     // CはAの子孫なので、AをCの子にすると循環が発生する
    //! }
    //! @endcode
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsAncestorOf(Actor ancestor, Actor descendant, World& world) const;

    //------------------------------------------------------------------------
    //! @brief 親子関係が循環を形成するか検証
    //! @param child 子として設定するActor
    //! @param parent 親として設定するActor
    //! @param world ECS World
    //! @return 循環が発生する場合はtrue
    //!
    //! SetParent(child, parent) を実行した場合に循環が発生するか判定。
    //------------------------------------------------------------------------
    [[nodiscard]] bool WouldCreateCycle(Actor child, Actor parent, World& world) const;

    //========================================================================
    // ユーティリティ
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief Actorを階層から完全に削除
    //! @param actor 削除するActor
    //! @param world ECS World
    //!
    //! 親子関係を解除し、子も再帰的にルートに戻す。
    //------------------------------------------------------------------------
    void RemoveActor(Actor actor, World& world);

    //------------------------------------------------------------------------
    //! @brief 全てクリア
    //------------------------------------------------------------------------
    void Clear() {
        roots_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief ルートとして登録
    //! @param actor 登録するActor
    //------------------------------------------------------------------------
    void RegisterAsRoot(Actor actor);

    //------------------------------------------------------------------------
    //! @brief ルートから削除
    //! @param actor 削除するActor
    //------------------------------------------------------------------------
    void UnregisterFromRoot(Actor actor);

private:
    //------------------------------------------------------------------------
    //! @brief 子を追加（内部用）
    //! @param parent 親Actor
    //! @param child 子Actor
    //! @param world ECS World
    //------------------------------------------------------------------------
    void AddChild(Actor parent, Actor child, World& world);

    //------------------------------------------------------------------------
    //! @brief 子を削除（内部用）
    //! @param parent 親Actor
    //! @param child 子Actor
    //! @param world ECS World
    //------------------------------------------------------------------------
    void RemoveChild(Actor parent, Actor child, World& world);

    //------------------------------------------------------------------------
    //! @brief 階層深度を再計算
    //! @param actor 対象Actor
    //! @param world ECS World
    //------------------------------------------------------------------------
    void UpdateHierarchyDepth(Actor actor, World& world);

    //------------------------------------------------------------------------
    //! @brief 子孫の階層深度を再帰的に更新
    //! @param parent 親Actor
    //! @param parentDepth 親の階層深度
    //! @param world ECS World
    //------------------------------------------------------------------------
    void UpdateChildrenDepthRecursive(Actor parent, uint16_t parentDepth, World& world);

private:
    //! ルートActor（親を持たない）のリスト
    std::vector<Actor> roots_;
};

} // namespace ECS

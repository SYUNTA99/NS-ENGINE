//----------------------------------------------------------------------------
//! @file   hierarchy_registry.cpp
//! @brief  HierarchyRegistry - 親子階層管理実装
//----------------------------------------------------------------------------
#include "hierarchy_registry.h"
#include "world.h"
#include "components/transform/parent.h"
#include "components/transform/hierarchy_depth_data.h"
#include "components/transform/transform_tags.h"
#include "common/assert/assert.h"
#include <algorithm>

namespace ECS {

//----------------------------------------------------------------------------
void HierarchyRegistry::SetParent(Actor child, Actor newParent, World& world) {
    if (!child.IsValid()) return;

    // 自分自身を親にはできない
    if (child == newParent) return;

    // 循環参照チェック
    if (WouldCreateCycle(child, newParent, world)) {
        // 循環が発生するので設定しない（警告のみ、assertは行わない）
        // Note: テストで意図的に循環を試行するケースがあるため、assertではなくreturnのみ
        return;
    }

    // 現在の親を取得
    Parent* parentComp = world.GetComponent<Parent>(child);
    Actor oldParent = parentComp ? parentComp->value : Actor::Invalid();

    // 同じ親なら何もしない
    if (oldParent == newParent) return;

    // 旧親から削除
    if (oldParent.IsValid()) {
        RemoveChild(oldParent, child, world);
    } else {
        // ルートから削除
        UnregisterFromRoot(child);
    }

    // 新しい親を設定
    if (newParent.IsValid()) {
        // 親子関係を追加
        AddChild(newParent, child, world);

        // ECSのParentを更新（なければ追加）
        if (!parentComp) {
            world.AddComponent<Parent>(child, newParent);
        } else {
            parentComp->value = newParent;
        }

        // HierarchyRootタグを削除
        world.RemoveComponent<HierarchyRoot>(child);
    } else {
        // ルートに設定
        RegisterAsRoot(child);

        // Parentを削除またはクリア
        if (parentComp) {
            world.RemoveComponent<Parent>(child);
        }

        // HierarchyRootタグを追加
        world.AddComponent<HierarchyRoot>(child);
    }

    // 階層深度を更新
    UpdateHierarchyDepth(child, world);

    // TransformDirtyタグを追加
    world.AddComponent<TransformDirty>(child);
}

//----------------------------------------------------------------------------
void HierarchyRegistry::ClearParent(Actor child, World& world) {
    SetParent(child, Actor::Invalid(), world);
}

//----------------------------------------------------------------------------
void HierarchyRegistry::AddChild(Actor parent, Actor child, World& world) {
    if (!parent.IsValid() || !child.IsValid()) return;

    // 親のChildrenバッファを取得（なければ追加）
    DynamicBuffer<Child> children = world.GetBuffer<Child>(parent);
    if (!children) {
        children = world.AddBuffer<Child>(parent);
    }

    // 重複チェック
    for (int32_t i = 0; i < children.Length(); ++i) {
        if (children[i].value == child) {
            return;  // 既に存在
        }
    }

    // 子を追加
    children.Add(Child{child});
}

//----------------------------------------------------------------------------
void HierarchyRegistry::RemoveChild(Actor parent, Actor child, World& world) {
    if (!parent.IsValid()) return;

    DynamicBuffer<Child> children = world.GetBuffer<Child>(parent);
    if (!children) return;

    // 子を探して削除（swap-and-pop）
    for (int32_t i = 0; i < children.Length(); ++i) {
        if (children[i].value == child) {
            children.RemoveAtSwapBack(i);
            break;
        }
    }

    // 子がなくなったらバッファを削除（オプション: メモリ節約のため）
    // Note: バッファを残しておくと再追加時にArchetype移動を回避できる
    // if (children.Length() == 0) {
    //     world.RemoveBuffer<Child>(parent);
    // }
}

//----------------------------------------------------------------------------
DynamicBuffer<Child> HierarchyRegistry::GetChildren(Actor parent, World& world) const {
    if (!parent.IsValid()) {
        return DynamicBuffer<Child>();
    }
    // constメソッドだがWorld::GetBufferは非constなのでconst_cast
    // (GetBufferは読み取り操作なので安全)
    return const_cast<World&>(world).GetBuffer<Child>(parent);
}

//----------------------------------------------------------------------------
bool HierarchyRegistry::HasChildren(Actor parent, World& world) const {
    auto children = GetChildren(parent, world);
    return children && children.Length() > 0;
}

//----------------------------------------------------------------------------
size_t HierarchyRegistry::GetChildCount(Actor parent, World& world) const {
    auto children = GetChildren(parent, world);
    return children ? static_cast<size_t>(children.Length()) : 0;
}

//----------------------------------------------------------------------------
bool HierarchyRegistry::IsAncestorOf(Actor ancestor, Actor descendant, World& world) const {
    if (!ancestor.IsValid() || !descendant.IsValid()) {
        return false;
    }

    // 同じActorは祖先ではない
    if (ancestor == descendant) {
        return false;
    }

    // descendantから親をたどってancestorを探す
    Actor current = descendant;
    while (current.IsValid()) {
        Parent* parentComp = const_cast<World&>(world).GetComponent<Parent>(current);
        if (!parentComp || !parentComp->HasParent()) {
            break;  // ルートに到達
        }

        Actor parent = parentComp->value;
        if (parent == ancestor) {
            return true;  // 発見
        }

        current = parent;
    }

    return false;
}

//----------------------------------------------------------------------------
bool HierarchyRegistry::WouldCreateCycle(Actor child, Actor parent, World& world) const {
    // 親がInvalidならルートに設定するので循環は発生しない
    if (!parent.IsValid()) {
        return false;
    }

    // childがparentの祖先であれば、parent→childの関係を作ると循環になる
    // つまり、childからparentへのパスが存在するかチェック
    return IsAncestorOf(child, parent, world);
}

//----------------------------------------------------------------------------
void HierarchyRegistry::RemoveActor(Actor actor, World& world) {
    if (!actor.IsValid()) return;

    // 自分の子をルートに戻す
    auto children = world.GetBuffer<Child>(actor);
    if (children && children.Length() > 0) {
        // コピー（イテレーション中に変更するため）
        std::vector<Actor> childrenCopy;
        childrenCopy.reserve(static_cast<size_t>(children.Length()));
        for (const auto& child : children) {
            childrenCopy.push_back(child.value);
        }

        for (Actor child : childrenCopy) {
            ClearParent(child, world);
        }
    }

    // 自分の親から削除
    Parent* parentComp = world.GetComponent<Parent>(actor);
    if (parentComp && parentComp->HasParent()) {
        RemoveChild(parentComp->value, actor, world);
    }

    // ルートから削除
    UnregisterFromRoot(actor);
}

//----------------------------------------------------------------------------
void HierarchyRegistry::RegisterAsRoot(Actor actor) {
    if (!actor.IsValid()) return;

    auto it = std::find(roots_.begin(), roots_.end(), actor);
    if (it == roots_.end()) {
        roots_.push_back(actor);
    }
}

//----------------------------------------------------------------------------
void HierarchyRegistry::UnregisterFromRoot(Actor actor) {
    roots_.erase(std::remove(roots_.begin(), roots_.end(), actor), roots_.end());
}

//----------------------------------------------------------------------------
void HierarchyRegistry::UpdateHierarchyDepth(Actor actor, World& world) {
    if (!actor.IsValid()) return;

    uint16_t depth = 0;

    // 親をたどって深度を計算
    Parent* parentComp = world.GetComponent<Parent>(actor);
    if (parentComp && parentComp->HasParent()) {
        HierarchyDepthData* parentDepth = world.GetComponent<HierarchyDepthData>(parentComp->value);
        if (parentDepth) {
            depth = parentDepth->depth + 1;
        } else {
            depth = 1;
        }
    }

    // HierarchyDepthDataを更新（なければ追加）
    HierarchyDepthData* depthData = world.GetComponent<HierarchyDepthData>(actor);
    if (!depthData) {
        world.AddComponent<HierarchyDepthData>(actor, depth);
    } else {
        depthData->depth = depth;
    }

    // 子孫の深度を再帰的に更新
    UpdateChildrenDepthRecursive(actor, depth, world);
}

//----------------------------------------------------------------------------
void HierarchyRegistry::UpdateChildrenDepthRecursive(Actor parent, uint16_t parentDepth, World& world) {
    auto children = GetChildren(parent, world);
    if (!children) return;

    for (const auto& childEntry : children) {
        Actor child = childEntry.value;
        uint16_t childDepth = parentDepth + 1;

        // HierarchyDepthDataを更新
        HierarchyDepthData* depthData = world.GetComponent<HierarchyDepthData>(child);
        if (!depthData) {
            world.AddComponent<HierarchyDepthData>(child, childDepth);
        } else {
            depthData->depth = childDepth;
        }

        // TransformDirtyタグを追加
        world.AddComponent<TransformDirty>(child);

        // 再帰
        UpdateChildrenDepthRecursive(child, childDepth, world);
    }
}

} // namespace ECS

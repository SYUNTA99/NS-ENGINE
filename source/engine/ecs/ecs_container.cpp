//----------------------------------------------------------------------------
//! @file   ecs_container.cpp
//! @brief  ECSContainer - 階層操作の実装
//----------------------------------------------------------------------------

#include "ecs_container.h"
#include "components/transform/parent.h"
#include "components/transform/children.h"

namespace ECS {

//----------------------------------------------------------------------------
void ECSContainer::SetParent(Actor child, Actor parent) {
    if (!actors_.IsAlive(child)) {
        return;
    }

    // 現在の親を取得
    Parent* parentComp = actors_.Get<Parent>(child);
    Actor oldParent = parentComp ? parentComp->value : Actor::Invalid();

    // 同じ親なら何もしない
    if (oldParent == parent) {
        return;
    }

    // 旧親の子リストから削除
    if (oldParent.IsValid() && actors_.IsAlive(oldParent)) {
        auto childBuffer = actors_.GetBuffer<Child>(oldParent);
        if (childBuffer) {
            for (int32_t i = 0; i < childBuffer.Length(); ++i) {
                if (childBuffer[i].value == child) {
                    childBuffer.RemoveAtSwapBack(i);
                    break;
                }
            }
        }
    } else if (oldParent == Actor::Invalid()) {
        // ルートから削除
        hierarchy_.UnregisterFromRoot(child);
    }

    // 新親を設定
    if (parent.IsValid()) {
        // Parentコンポーネントを追加/更新
        if (!parentComp) {
            actors_.Add<Parent>(child, parent);
        } else {
            parentComp->value = parent;
        }

        // 新親にChildバッファを追加（なければ作成）
        auto childBuffer = actors_.AddBuffer<Child>(parent);
        if (childBuffer) {
            childBuffer.Add(Child(child));
        }
    } else {
        // ルートに戻す
        if (parentComp) {
            actors_.Remove<Parent>(child);
        }
        hierarchy_.RegisterAsRoot(child);
    }
}

//----------------------------------------------------------------------------
Actor ECSContainer::GetParent(Actor actor) const {
    if (!actors_.IsAlive(actor)) {
        return Actor::Invalid();
    }

    const Parent* parentComp = actors_.Get<Parent>(actor);
    return parentComp ? parentComp->value : Actor::Invalid();
}

//----------------------------------------------------------------------------
std::vector<Actor> ECSContainer::GetChildren(Actor parent) const {
    std::vector<Actor> result;

    if (!actors_.IsAlive(parent)) {
        return result;
    }

    auto childBuffer = actors_.GetBuffer<Child>(parent);

    if (childBuffer) {
        result.reserve(static_cast<size_t>(childBuffer.Length()));
        for (const auto& child : childBuffer) {
            result.push_back(child.value);
        }
    }

    return result;
}

//----------------------------------------------------------------------------
size_t ECSContainer::GetChildCount(Actor parent) const {
    if (!actors_.IsAlive(parent)) {
        return 0;
    }

    auto childBuffer = actors_.GetBuffer<Child>(parent);

    return childBuffer ? static_cast<size_t>(childBuffer.Length()) : 0;
}

//----------------------------------------------------------------------------
bool ECSContainer::HasChildren(Actor parent) const {
    return GetChildCount(parent) > 0;
}

} // namespace ECS

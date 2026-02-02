//----------------------------------------------------------------------------
//! @file   parent_system.h
//! @brief  ECS ParentSystem - 親子関係管理システム
//----------------------------------------------------------------------------
#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/components/transform/parent.h"
#include "engine/ecs/components/transform/previous_parent.h"
#include "engine/ecs/components/transform/hierarchy_depth_data.h"
#include "engine/ecs/components/transform/children.h"
#include "engine/ecs/components/transform/transform_tags.h"

namespace ECS {

//============================================================================
//! @brief 親子関係管理システム
//!
//! 役割:
//! 1. Parent と PreviousParent を比較して親変更を検出
//! 2. Child バッファを自動管理（旧親から削除、新親に追加）
//! 3. PreviousParent を現在の Parent で更新
//! 4. HierarchyDepthData を更新
//! 5. TransformDirty タグを追加
//!
//! @note 優先度1（全システムの最初に実行）
//!
//! 使用例:
//! @code
//! world.RegisterSystem<ParentSystem>();
//!
//! // 親子関係を設定（直接コンポーネント操作でOK）
//! auto parent = world.CreateActor();
//! auto child = world.CreateActor();
//! world.AddComponent<Parent>(child, parent);
//!
//! // ParentSystemが自動で:
//! // - PreviousParent を追加/更新
//! // - 親の Child バッファを更新
//! // - HierarchyDepthData を計算
//! // - TransformDirty を追加
//! @endcode
//============================================================================
class ParentSystem final : public ISystem {
public:
    void OnUpdate(World& world, [[maybe_unused]] float dt) override {
        // Pass 1: 新規 Parent（PreviousParent なし）を処理
        ProcessNewParents(world);

        // Pass 2: 親変更検出と Child バッファ更新
        ProcessParentChanges(world);

        // Pass 3: HierarchyDepthData の更新
        UpdateHierarchyDepths(world);
    }

    int Priority() const override { return 1; }
    const char* Name() const override { return "ParentSystem"; }

private:
    //------------------------------------------------------------------------
    //! @brief 新規 Parent コンポーネントを処理
    //! PreviousParent がないものに追加し、Child バッファを更新
    //------------------------------------------------------------------------
    void ProcessNewParents(World& world) {
        // Parent を持つが PreviousParent を持たない Actor を収集
        newParentActors_.clear();

        world.ForEach<Parent>([this, &world](Actor actor, Parent&) {
            if (!world.HasComponent<PreviousParent>(actor)) {
                newParentActors_.push_back(actor);
            }
        });

        // 新規 Parent を処理
        for (Actor actor : newParentActors_) {
            auto* parent = world.GetComponent<Parent>(actor);
            if (!parent) continue;

            // 親Actorを保存（AddComponent後にポインタが無効になるため）
            Actor parentActor = parent->value;
            bool hasParent = parent->HasParent();

            // PreviousParent を追加（現在の Parent で初期化 → ProcessParentChanges で重複処理を防ぐ）
            // Note: この呼び出し後、actorはArchetype移動する可能性があり、parentポインタは無効になる
            world.AddComponent<PreviousParent>(actor, parentActor);

            // 新しい親の Child バッファに追加
            if (hasParent) {
                AddToChildBuffer(world, parentActor, actor);
            }

            // HierarchyDepthData がなければ追加
            if (!world.HasComponent<HierarchyDepthData>(actor)) {
                world.AddComponent<HierarchyDepthData>(actor, static_cast<uint16_t>(0));
            }

            // TransformDirty タグを追加
            world.AddComponent<TransformDirty>(actor);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 親変更を検出し、Child バッファを更新
    //------------------------------------------------------------------------
    void ProcessParentChanges(World& world) {
        // 変更された Actor を収集（ForEach 中に変更を避けるため）
        changedActors_.clear();

        world.ForEach<Parent, PreviousParent>(
            [this](Actor actor, Parent& parent, PreviousParent& prevParent) {
                if (parent.value != prevParent.value) {
                    changedActors_.push_back({actor, prevParent.value, parent.value});
                }
            });

        // 変更を適用
        for (const auto& change : changedActors_) {
            // 旧親の Child バッファから削除
            if (change.oldParent.IsValid()) {
                RemoveFromChildBuffer(world, change.oldParent, change.actor);
            }

            // 新親の Child バッファに追加
            if (change.newParent.IsValid()) {
                AddToChildBuffer(world, change.newParent, change.actor);
            }

            // PreviousParent を更新
            auto* prevParent = world.GetComponent<PreviousParent>(change.actor);
            if (prevParent) {
                prevParent->value = change.newParent;
            }

            // TransformDirty タグを追加
            world.AddComponent<TransformDirty>(change.actor);
        }
    }

    //------------------------------------------------------------------------
    //! @brief HierarchyDepthData を更新
    //! 親をたどってルートからの深度を計算（親の深度が未計算でも正確）
    //------------------------------------------------------------------------
    void UpdateHierarchyDepths(World& world) {
        world.ForEach<Parent, HierarchyDepthData>(
            [&world](Actor, const Parent& parent, HierarchyDepthData& depth) {
                depth.depth = CalculateDepth(world, parent.value);
            });
    }

    //------------------------------------------------------------------------
    //! @brief 親をたどって深度を計算
    //------------------------------------------------------------------------
    static uint16_t CalculateDepth(World& world, Actor actor) {
        uint16_t depth = 0;
        Actor current = actor;

        while (current.IsValid()) {
            ++depth;
            auto* parentComp = world.GetComponent<Parent>(current);
            if (!parentComp || !parentComp->HasParent()) {
                break;
            }
            current = parentComp->value;

            // 無限ループ防止（循環がある場合）
            if (depth > 1000) {
                break;
            }
        }

        return depth;
    }

    //------------------------------------------------------------------------
    //! @brief 親の Child バッファに子を追加
    //------------------------------------------------------------------------
    void AddToChildBuffer(World& world, Actor parent, Actor child) {
        if (!parent.IsValid() || !child.IsValid()) return;
        if (!world.IsAlive(parent)) return;

        DynamicBuffer<Child> buffer;

        // バッファがなければ追加（AddBufferの戻り値を使用）
        if (!world.HasBuffer<Child>(parent)) {
            buffer = world.AddBuffer<Child>(parent);
        } else {
            buffer = world.GetBuffer<Child>(parent);
        }

        if (!buffer) return;

        // 重複チェック
        for (int32_t i = 0; i < buffer.Length(); ++i) {
            if (buffer[i].value == child) {
                return;  // 既に存在
            }
        }

        buffer.Add(Child{child});
    }

    //------------------------------------------------------------------------
    //! @brief 親の Child バッファから子を削除
    //------------------------------------------------------------------------
    void RemoveFromChildBuffer(World& world, Actor parent, Actor child) {
        if (!parent.IsValid()) return;

        auto buffer = world.GetBuffer<Child>(parent);
        if (!buffer) return;

        for (int32_t i = 0; i < buffer.Length(); ++i) {
            if (buffer[i].value == child) {
                buffer.RemoveAtSwapBack(i);
                return;
            }
        }
    }

private:
    //! 親変更情報
    struct ParentChange {
        Actor actor;
        Actor oldParent;
        Actor newParent;
    };

    std::vector<Actor> newParentActors_;          //!< 新規 Parent Actor
    std::vector<ParentChange> changedActors_;     //!< 親変更 Actor
};

} // namespace ECS

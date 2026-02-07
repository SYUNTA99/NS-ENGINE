//----------------------------------------------------------------------------
//! @file   system_scheduler.h
//! @brief  ECS SystemScheduler - System管理とスケジューリング
//----------------------------------------------------------------------------
#pragma once


#include "system.h"
#include "system_graph.h"
#include "common/utility/non_copyable.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief SystemScheduler
//!
//! Systemの登録、管理、スケジューリングを担当するクラス。
//! Worldから分離された責務:
//! - System登録/破棄
//! - 依存関係に基づくソート
//! - System実行
//!
//! @note システムのライフサイクル:
//!       Register時: OnCreate()
//!       毎フレーム: OnUpdate() / OnRender()
//!       World破棄時: OnDestroy()
//============================================================================
class SystemScheduler : private NonCopyable {
public:
    SystemScheduler() = default;
    ~SystemScheduler() = default;

    //========================================================================
    // System登録
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 更新Systemを登録
    //! @tparam T ISystemを継承したクラス
    //! @param world Worldへの参照（OnCreate呼び出し用）
    //------------------------------------------------------------------------
    template<typename T>
    void Register(World& world) {
        static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");
        auto system = std::make_unique<T>();

        SystemEntry entry;
        entry.id = std::type_index(typeid(T));
        entry.priority = system->Priority();
        entry.name = system->Name();

        // OnCreate呼び出し
        system->OnCreate(world);

        entry.system = std::move(system);
        CommitSystem(std::move(entry));
    }

    //------------------------------------------------------------------------
    //! @brief 描画Systemを登録
    //! @tparam T IRenderSystemを継承したクラス
    //! @param world Worldへの参照（OnCreate呼び出し用）
    //------------------------------------------------------------------------
    template<typename T>
    void RegisterRender(World& world) {
        static_assert(std::is_base_of_v<IRenderSystem, T>, "T must inherit from IRenderSystem");
        auto system = std::make_unique<T>();

        RenderSystemEntry entry;
        entry.id = std::type_index(typeid(T));
        entry.priority = system->Priority();
        entry.name = system->Name();

        // OnCreate呼び出し
        system->OnCreate(world);

        entry.system = std::move(system);
        CommitRenderSystem(std::move(entry));
    }

    //------------------------------------------------------------------------
    //! @brief SystemEntryを直接登録（SystemBuilderから呼ばれる）
    //------------------------------------------------------------------------
    void CommitSystem(SystemEntry entry) {
        SystemId id = entry.id;

        // グラフにノード追加
        systemGraph_.AddNode(
            id,
            entry.priority,
            entry.runAfter,
            entry.runBefore,
            entry.name
        );

        // 実体を保存
        systemsById_[id] = std::move(entry.system);
        systemsDirty_ = true;
    }

    //------------------------------------------------------------------------
    //! @brief RenderSystemEntryを直接登録
    //------------------------------------------------------------------------
    void CommitRenderSystem(RenderSystemEntry entry) {
        SystemId id = entry.id;

        renderSystemGraph_.AddNode(
            id,
            entry.priority,
            entry.runAfter,
            entry.runBefore,
            entry.name
        );

        renderSystemsById_[id] = std::move(entry.system);
        renderSystemsDirty_ = true;
    }

    //========================================================================
    // System実行
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 全更新Systemを実行
    //! @param world Worldへの参照
    //! @param dt デルタタイム
    //------------------------------------------------------------------------
    void Update(World& world, float dt) {
        if (systemsDirty_) {
            RebuildSortedSystems();
        }

        for (ISystem* system : sortedSystems_) {
            system->OnUpdate(world, dt);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 全描画Systemを実行
    //! @param world Worldへの参照
    //! @param alpha 補間係数（0.0〜1.0）
    //------------------------------------------------------------------------
    void Render(World& world, float alpha) {
        if (renderSystemsDirty_) {
            RebuildSortedRenderSystems();
        }

        for (IRenderSystem* system : sortedRenderSystems_) {
            system->OnRender(world, alpha);
        }
    }

    //------------------------------------------------------------------------
    //! @brief 全SystemのOnDestroyを呼び出し
    //! @param world Worldへの参照
    //------------------------------------------------------------------------
    void DestroyAll(World& world) {
        // 更新System
        for (auto& [id, system] : systemsById_) {
            if (system) {
                system->OnDestroy(world);
            }
        }

        // 描画System
        for (auto& [id, system] : renderSystemsById_) {
            if (system) {
                system->OnDestroy(world);
            }
        }
    }

    //========================================================================
    // クリア
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 全Systemをクリア（OnDestroyは呼ばれない）
    //------------------------------------------------------------------------
    void Clear() {
        systemGraph_.Clear();
        renderSystemGraph_.Clear();
        systemsById_.clear();
        renderSystemsById_.clear();
        sortedSystems_.clear();
        sortedRenderSystems_.clear();
        systemsDirty_ = false;
        renderSystemsDirty_ = false;
    }

    //========================================================================
    // 情報取得
    //========================================================================

    [[nodiscard]] size_t SystemCount() const noexcept {
        return systemsById_.size();
    }

    [[nodiscard]] size_t RenderSystemCount() const noexcept {
        return renderSystemsById_.size();
    }

private:
    //------------------------------------------------------------------------
    //! @brief ソート済みSystem配列を再構築
    //------------------------------------------------------------------------
    void RebuildSortedSystems() {
        sortedSystems_.clear();
        std::vector<SystemId> sortedIds = systemGraph_.TopologicalSort();
        sortedSystems_.reserve(sortedIds.size());

        for (SystemId id : sortedIds) {
            auto it = systemsById_.find(id);
            if (it != systemsById_.end() && it->second) {
                sortedSystems_.push_back(it->second.get());
            }
        }

        systemsDirty_ = false;
    }

    //------------------------------------------------------------------------
    //! @brief ソート済みRenderSystem配列を再構築
    //------------------------------------------------------------------------
    void RebuildSortedRenderSystems() {
        sortedRenderSystems_.clear();
        std::vector<SystemId> sortedIds = renderSystemGraph_.TopologicalSort();
        sortedRenderSystems_.reserve(sortedIds.size());

        for (SystemId id : sortedIds) {
            auto it = renderSystemsById_.find(id);
            if (it != renderSystemsById_.end() && it->second) {
                sortedRenderSystems_.push_back(it->second.get());
            }
        }

        renderSystemsDirty_ = false;
    }

private:
    //! System依存関係グラフ
    SystemGraph systemGraph_;
    RenderSystemGraph renderSystemGraph_;

    //! System実体
    std::unordered_map<SystemId, std::unique_ptr<ISystem>> systemsById_;
    std::unordered_map<SystemId, std::unique_ptr<IRenderSystem>> renderSystemsById_;

    //! ソート済みSystem配列（実行用）
    std::vector<ISystem*> sortedSystems_;
    std::vector<IRenderSystem*> sortedRenderSystems_;

    //! ソート済み配列の再構築フラグ
    bool systemsDirty_ = false;
    bool renderSystemsDirty_ = false;
};

} // namespace ECS

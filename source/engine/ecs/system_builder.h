//----------------------------------------------------------------------------
//! @file   system_builder.h
//! @brief  ECS SystemBuilder - System登録用Builderパターン
//----------------------------------------------------------------------------
#pragma once

#include "system.h"
#include "system_graph.h"
#include <memory>
#include <vector>
#include <typeindex>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief SystemBuilder
//!
//! System登録時に依存関係を指定するためのBuilderクラス。
//! デストラクタで実際の登録を行う。
//!
//! 使用例:
//! ```cpp
//! world.RegisterSystem<TransformSystem>();  // 即座に登録
//!
//! world.RegisterSystem<Collision2DSystem>()
//!      .After<TransformSystem>();            // TransformSystemの後に実行
//!
//! world.RegisterSystem<RenderSystem>()
//!      .After<TransformSystem>()
//!      .After<AnimationSystem>()
//!      .WithPriority(100);
//! ```
//============================================================================
template<typename T>
class SystemBuilder {
public:
    //! @brief コンストラクタ
    //! @param world 登録先World
    //! @param system Systemインスタンス
    SystemBuilder(World* world, std::unique_ptr<T> system)
        : world_(world)
        , system_(std::move(system))
        , priority_(system_ ? system_->Priority() : 0)
        , id_(std::type_index(typeid(T)))
        , name_(system_ ? system_->Name() : "Unknown")
    {}

    //! @brief ムーブコンストラクタ
    SystemBuilder(SystemBuilder&& other) noexcept
        : world_(other.world_)
        , system_(std::move(other.system_))
        , priority_(other.priority_)
        , id_(other.id_)
        , name_(other.name_)
        , runAfter_(std::move(other.runAfter_))
        , runBefore_(std::move(other.runBefore_))
    {
        other.world_ = nullptr;  // 移動元は登録しない
    }

    //! @brief ムーブ代入
    SystemBuilder& operator=(SystemBuilder&& other) noexcept {
        if (this != &other) {
            // 既存のsystemがあれば登録
            Commit();

            world_ = other.world_;
            system_ = std::move(other.system_);
            priority_ = other.priority_;
            id_ = other.id_;
            name_ = other.name_;
            runAfter_ = std::move(other.runAfter_);
            runBefore_ = std::move(other.runBefore_);
            other.world_ = nullptr;
        }
        return *this;
    }

    //! @brief デストラクタ - 実際の登録を実行
    ~SystemBuilder() {
        Commit();
    }

    // コピー禁止
    SystemBuilder(const SystemBuilder&) = delete;
    SystemBuilder& operator=(const SystemBuilder&) = delete;

    //------------------------------------------------------------------------
    //! @brief 指定Systemの後に実行
    //! @tparam U 依存先System型
    //! @return 自身への参照（チェーン可能）
    //------------------------------------------------------------------------
    template<typename U>
    SystemBuilder& After() {
        runAfter_.push_back(std::type_index(typeid(U)));
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief 指定Systemの前に実行
    //! @tparam U 依存先System型
    //! @return 自身への参照（チェーン可能）
    //------------------------------------------------------------------------
    template<typename U>
    SystemBuilder& Before() {
        runBefore_.push_back(std::type_index(typeid(U)));
        return *this;
    }

    //------------------------------------------------------------------------
    //! @brief 優先度を明示的に設定
    //! @param priority 優先度（依存関係がない場合のソート用）
    //! @return 自身への参照（チェーン可能）
    //------------------------------------------------------------------------
    SystemBuilder& WithPriority(int priority) {
        priority_ = priority;
        return *this;
    }

private:
    //! @brief Worldに登録を実行
    void Commit();

    World* world_;
    std::unique_ptr<T> system_;
    int priority_;
    SystemId id_;
    const char* name_;
    std::vector<SystemId> runAfter_;
    std::vector<SystemId> runBefore_;
};

//============================================================================
//! @brief RenderSystemBuilder
//!
//! RenderSystem登録用のBuilderクラス。
//============================================================================
template<typename T>
class RenderSystemBuilder {
public:
    RenderSystemBuilder(World* world, std::unique_ptr<T> system)
        : world_(world)
        , system_(std::move(system))
        , priority_(system_ ? system_->Priority() : 0)
        , id_(std::type_index(typeid(T)))
        , name_(system_ ? system_->Name() : "Unknown")
    {}

    RenderSystemBuilder(RenderSystemBuilder&& other) noexcept
        : world_(other.world_)
        , system_(std::move(other.system_))
        , priority_(other.priority_)
        , id_(other.id_)
        , name_(other.name_)
        , runAfter_(std::move(other.runAfter_))
        , runBefore_(std::move(other.runBefore_))
    {
        other.world_ = nullptr;
    }

    RenderSystemBuilder& operator=(RenderSystemBuilder&& other) noexcept {
        if (this != &other) {
            Commit();
            world_ = other.world_;
            system_ = std::move(other.system_);
            priority_ = other.priority_;
            id_ = other.id_;
            name_ = other.name_;
            runAfter_ = std::move(other.runAfter_);
            runBefore_ = std::move(other.runBefore_);
            other.world_ = nullptr;
        }
        return *this;
    }

    ~RenderSystemBuilder() {
        Commit();
    }

    RenderSystemBuilder(const RenderSystemBuilder&) = delete;
    RenderSystemBuilder& operator=(const RenderSystemBuilder&) = delete;

    template<typename U>
    RenderSystemBuilder& After() {
        runAfter_.push_back(std::type_index(typeid(U)));
        return *this;
    }

    template<typename U>
    RenderSystemBuilder& Before() {
        runBefore_.push_back(std::type_index(typeid(U)));
        return *this;
    }

    RenderSystemBuilder& WithPriority(int priority) {
        priority_ = priority;
        return *this;
    }

private:
    void Commit();

    World* world_;
    std::unique_ptr<T> system_;
    int priority_;
    SystemId id_;
    const char* name_;
    std::vector<SystemId> runAfter_;
    std::vector<SystemId> runBefore_;
};

} // namespace ECS

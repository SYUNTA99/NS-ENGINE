//----------------------------------------------------------------------------
//! @file   entity_command_buffer.h
//! @brief  ECS EntityCommandBuffer - スレッドセーフなエンティティ操作キュー
//!
//! 並列処理中にActorの作成/破棄/コンポーネント追加削除を記録し、
//! メインスレッドで安全に再生するためのバッファ。
//----------------------------------------------------------------------------
#pragma once


#include "actor.h"
#include <vector>
#include <memory>
#include <mutex>
#include <typeindex>
#include <functional>
#include <cassert>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief コマンドの種類
//============================================================================
enum class CommandType : uint8_t {
    DestroyActor,     //!< Actor破棄
    AddComponent,     //!< コンポーネント追加
    RemoveComponent,  //!< コンポーネント削除
};

//============================================================================
//! @brief エンティティコマンド
//!
//! 並列処理中に記録され、Playback時に実行される操作
//============================================================================
struct EntityCommand {
    CommandType type;                             //!< コマンド種別
    Actor actor;                                  //!< 対象Actor
    std::type_index componentType;                //!< コンポーネント型（Add/Remove用）
    std::unique_ptr<std::byte[]> componentData;   //!< コンポーネントデータ（Add用）
    size_t componentSize = 0;                     //!< コンポーネントサイズ
    size_t componentAlignment = 0;                //!< コンポーネントアラインメント
    std::function<void(void*)> destructor;        //!< デストラクタ（キャンセル時用）
    std::function<void(World&, Actor, void*)> applier;  //!< 適用関数

    // デフォルトコンストラクタ
    EntityCommand()
        : type(CommandType::DestroyActor)
        , actor(Actor::Invalid())
        , componentType(typeid(void))
    {}

    // ムーブのみ許可
    EntityCommand(EntityCommand&&) = default;
    EntityCommand& operator=(EntityCommand&&) = default;
    EntityCommand(const EntityCommand&) = delete;
    EntityCommand& operator=(const EntityCommand&) = delete;

    // デストラクタ - 未実行のAdd操作のデータを破棄
    ~EntityCommand() {
        if (componentData && destructor) {
            destructor(componentData.get());
        }
    }
};

//============================================================================
//! @brief EntityCommandBuffer
//!
//! 並列ジョブ内からエンティティ操作を記録するためのスレッドセーフなバッファ。
//! 記録された操作はPlayback()でメインスレッドから再生される。
//!
//! 使用例:
//! @code
//! EntityCommandBuffer ecb;
//!
//! world.ParallelForEach<InOut<HealthData>>(
//!     [&ecb](Actor e, HealthData& hp) {
//!         if (hp.value <= 0) {
//!             ecb.DestroyActor(e);  // スレッドセーフ
//!         }
//!     });
//!
//! // メインスレッドで再生
//! ecb.Playback(world);
//! @endcode
//!
//! @note スレッドセーフ: 複数スレッドから同時にコマンドを記録可能
//============================================================================
class EntityCommandBuffer {
public:
    EntityCommandBuffer() = default;
    ~EntityCommandBuffer() = default;

    // コピー禁止、ムーブ許可
    EntityCommandBuffer(const EntityCommandBuffer&) = delete;
    EntityCommandBuffer& operator=(const EntityCommandBuffer&) = delete;
    EntityCommandBuffer(EntityCommandBuffer&&) = default;
    EntityCommandBuffer& operator=(EntityCommandBuffer&&) = default;

    //------------------------------------------------------------------------
    //! @brief Actor破棄をキューに追加（スレッドセーフ）
    //! @param actor 破棄するActor
    //------------------------------------------------------------------------
    void DestroyActor(Actor actor) {
        EntityCommand cmd;
        cmd.type = CommandType::DestroyActor;
        cmd.actor = actor;

        std::lock_guard<std::mutex> lock(mutex_);
        commands_.push_back(std::move(cmd));
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネント追加をキューに追加（スレッドセーフ）
    //! @tparam T コンポーネント型
    //! @tparam Args コンストラクタ引数の型
    //! @param actor 対象Actor
    //! @param args コンストラクタ引数
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    void AddComponent(Actor actor, Args&&... args) {
        static_assert(std::is_trivially_copyable_v<T>,
            "Component type must be trivially copyable");

        EntityCommand cmd;
        cmd.type = CommandType::AddComponent;
        cmd.actor = actor;
        cmd.componentType = std::type_index(typeid(T));
        cmd.componentSize = sizeof(T);
        cmd.componentAlignment = alignof(T);

        // アラインされたメモリを確保してコンポーネントを構築
        cmd.componentData = std::make_unique<std::byte[]>(sizeof(T));
        new (cmd.componentData.get()) T(std::forward<Args>(args)...);

        // デストラクタ（trivially copyableなので通常は不要だが安全のため）
        cmd.destructor = [](void* ptr) {
            static_cast<T*>(ptr)->~T();
        };

        // 適用関数
        cmd.applier = [](World& world, Actor a, void* data) {
            // Worldに実際に追加
            world.AddComponent<T>(a, *static_cast<T*>(data));
        };

        std::lock_guard<std::mutex> lock(mutex_);
        commands_.push_back(std::move(cmd));
    }

    //------------------------------------------------------------------------
    //! @brief コンポーネント削除をキューに追加（スレッドセーフ）
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //------------------------------------------------------------------------
    template<typename T>
    void RemoveComponent(Actor actor) {
        EntityCommand cmd;
        cmd.type = CommandType::RemoveComponent;
        cmd.actor = actor;
        cmd.componentType = std::type_index(typeid(T));

        // 適用関数（削除用）
        cmd.applier = [](World& world, Actor a, void*) {
            world.RemoveComponent<T>(a);
        };

        std::lock_guard<std::mutex> lock(mutex_);
        commands_.push_back(std::move(cmd));
    }

    //------------------------------------------------------------------------
    //! @brief 記録されたコマンドをWorldに再生（メインスレッド専用）
    //! @param world 対象World
    //!
    //! @note この関数はメインスレッドから呼び出すこと。
    //!       再生後、バッファは自動的にクリアされる。
    //------------------------------------------------------------------------
    void Playback(World& world);

    //------------------------------------------------------------------------
    //! @brief バッファをクリア（未実行のコマンドは破棄される）
    //------------------------------------------------------------------------
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        commands_.clear();
    }

    //------------------------------------------------------------------------
    //! @brief 記録されたコマンド数を取得
    //------------------------------------------------------------------------
    [[nodiscard]] size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return commands_.size();
    }

    //------------------------------------------------------------------------
    //! @brief バッファが空かどうか
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return commands_.empty();
    }

private:
    mutable std::mutex mutex_;           //!< スレッドセーフ用ミューテックス
    std::vector<EntityCommand> commands_; //!< コマンドキュー
};

} // namespace ECS

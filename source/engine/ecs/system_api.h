//----------------------------------------------------------------------------
//! @file   system_api.h
//! @brief  SystemAPI - System内からの静的アクセスAPI
//----------------------------------------------------------------------------
#pragma once

#include "system_state.h"
#include "access_mode.h"

namespace ECS {

// 前方宣言
class ActorRegistry;
template<typename... AccessModes> class TypedQuery;

//============================================================================
//! @brief SystemAPI
//!
//! System実行中に静的メソッドでワールドにアクセスするためのAPI。
//! Unity DOTSのSystemAPIに相当。
//!
//! 使用前にSetCurrentState()で現在のSystemStateを設定する必要がある。
//! SystemSchedulerが各System実行前に自動的に設定する。
//!
//! @code
//! class MovementSystem final : public ISystem {
//!     void OnUpdate(World& world, float dt) override {
//!         // SystemAPI経由でアクセス
//!         SystemAPI::Query<InOut<TransformData>, In<VelocityData>>()
//!             .ForEach([](Actor e, TransformData& t, const VelocityData& v) {
//!                 t.position += v.velocity * SystemAPI::DeltaTime();
//!             });
//!
//!         // エンティティ操作
//!         Actor newActor = SystemAPI::CreateActor();
//!         SystemAPI::AddComponent<TransformData>(newActor, pos);
//!     }
//! };
//! @endcode
//!
//! @note スレッドローカルストレージを使用するため、マルチスレッド安全。
//!       各スレッドで独立したSystemStateを設定可能。
//============================================================================
class SystemAPI {
public:
    //========================================================================
    // 状態管理
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief 現在のSystemStateを設定
    //! @param state SystemStateへのポインタ
    //!
    //! SystemSchedulerが各System実行前に呼び出す。
    //------------------------------------------------------------------------
    static void SetCurrentState(SystemState* state) {
        currentState_ = state;
    }

    //------------------------------------------------------------------------
    //! @brief 現在のSystemStateを取得
    //! @return SystemStateへのポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] static SystemState* GetCurrentState() noexcept {
        return currentState_;
    }

    //------------------------------------------------------------------------
    //! @brief SystemStateが設定されているか
    //! @return 設定されている場合はtrue
    //------------------------------------------------------------------------
    [[nodiscard]] static bool HasCurrentState() noexcept {
        return currentState_ != nullptr;
    }

    //========================================================================
    // 時間
    //========================================================================

    //! デルタタイム取得
    [[nodiscard]] static float DeltaTime() noexcept {
        return currentState_ ? currentState_->deltaTime : 0.0f;
    }

    //! 経過時間取得
    [[nodiscard]] static float Time() noexcept {
        return currentState_ ? currentState_->time : 0.0f;
    }

    //! フレームカウント取得
    [[nodiscard]] static uint32_t FrameCount() noexcept {
        return currentState_ ? currentState_->frameCount : 0;
    }

    //========================================================================
    // EntityManager相当API
    //========================================================================

    //------------------------------------------------------------------------
    //! @brief EntityManager（ActorRegistry）を取得
    //! @return ActorRegistryへの参照
    //------------------------------------------------------------------------
    [[nodiscard]] static ActorRegistry& EntityManager();

    //------------------------------------------------------------------------
    //! @brief クエリを構築
    //! @tparam AccessModes アクセスモード群（In<T>, Out<T>, InOut<T>）
    //! @return TypedQueryオブジェクト
    //!
    //! @code
    //! SystemAPI::Query<InOut<TransformData>, In<VelocityData>>()
    //!     .ForEach([](Actor e, TransformData& t, const VelocityData& v) {
    //!         t.position += v.velocity;
    //!     });
    //! @endcode
    //------------------------------------------------------------------------
    template<typename... AccessModes>
    [[nodiscard]] static auto Query();

    //------------------------------------------------------------------------
    //! @brief Actorを生成
    //! @return 生成されたActor
    //------------------------------------------------------------------------
    [[nodiscard]] static Actor CreateActor();

    //------------------------------------------------------------------------
    //! @brief Actorを破棄
    //! @param actor 破棄するActor
    //------------------------------------------------------------------------
    static void DestroyActor(Actor actor);

    //------------------------------------------------------------------------
    //! @brief コンポーネントを取得
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //! @return コンポーネントへのポインタ（存在しない場合はnullptr）
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] static T* GetComponent(Actor actor);

    //------------------------------------------------------------------------
    //! @brief コンポーネントを追加
    //! @tparam T コンポーネント型
    //! @tparam Args コンストラクタ引数の型
    //! @param actor 対象Actor
    //! @param args コンストラクタ引数
    //! @return 追加されたコンポーネントへのポインタ
    //------------------------------------------------------------------------
    template<typename T, typename... Args>
    static T* AddComponent(Actor actor, Args&&... args);

    //------------------------------------------------------------------------
    //! @brief コンポーネントを所持しているか
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //! @return 所持している場合はtrue
    //------------------------------------------------------------------------
    template<typename T>
    [[nodiscard]] static bool HasComponent(Actor actor);

    //------------------------------------------------------------------------
    //! @brief コンポーネントを削除
    //! @tparam T コンポーネント型
    //! @param actor 対象Actor
    //------------------------------------------------------------------------
    template<typename T>
    static void RemoveComponent(Actor actor);

private:
    //! スレッドローカルな現在のSystemState
    static thread_local SystemState* currentState_;
};

} // namespace ECS

// テンプレート実装
#include "detail/system_api_impl.h"

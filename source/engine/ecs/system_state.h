//----------------------------------------------------------------------------
//! @file   system_state.h
//! @brief  SystemState - System実行時コンテキスト
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include <cstdint>

namespace ECS {

// 前方宣言
class World;
class ActorRegistry;

//============================================================================
//! @brief SystemState
//!
//! System実行時のコンテキスト情報を保持する。
//! Unity DOTSのSystemStateに相当。
//!
//! 各Systemは OnUpdate(World&, float dt) の代わりに OnUpdate(SystemState&) を
//! 使用することで、より統一されたAPIでアクセスできる。
//!
//! @code
//! class MovementSystem final : public ISystemBase {
//! public:
//!     void OnUpdate(SystemState& state) override {
//!         // クエリ経由でアクセス
//!         state.EntityManager().Query<InOut<TransformData>, In<VelocityData>>()
//!             .ForEach([&state](Actor e, TransformData& t, const VelocityData& v) {
//!                 t.position += v.velocity * state.DeltaTime();
//!             });
//!
//!         // 個別アクセス
//!         auto* transform = state.GetComponent<TransformData>(someActor);
//!     }
//! };
//! @endcode
//============================================================================
struct SystemState {
    World* world = nullptr;          //!< Worldへのポインタ
    float deltaTime = 0.0f;          //!< デルタタイム（秒）
    float time = 0.0f;               //!< 経過時間（秒）
    uint32_t frameCount = 0;         //!< フレームカウント

    //========================================================================
    // コンストラクタ
    //========================================================================

    SystemState() = default;

    SystemState(World* w, float dt, float t, uint32_t frame)
        : world(w)
        , deltaTime(dt)
        , time(t)
        , frameCount(frame)
    {}

    //========================================================================
    // 便利アクセサ
    //========================================================================

    //! デルタタイム取得
    [[nodiscard]] float DeltaTime() const noexcept { return deltaTime; }

    //! 経過時間取得
    [[nodiscard]] float Time() const noexcept { return time; }

    //! フレームカウント取得
    [[nodiscard]] uint32_t FrameCount() const noexcept { return frameCount; }

    //! Worldへのアクセス
    [[nodiscard]] World& GetWorld() noexcept { return *world; }
    [[nodiscard]] const World& GetWorld() const noexcept { return *world; }

    //========================================================================
    // EntityManager相当API
    //========================================================================

    //! ActorRegistryへのアクセス（Unity DOTSのEntityManager相当）
    [[nodiscard]] ActorRegistry& EntityManager();
    [[nodiscard]] const ActorRegistry& EntityManager() const;

    //! Actorを生成
    [[nodiscard]] Actor CreateActor();

    //! Actorを破棄
    void DestroyActor(Actor actor);

    //! コンポーネントを取得
    template<typename T>
    [[nodiscard]] T* GetComponent(Actor actor);

    template<typename T>
    [[nodiscard]] const T* GetComponent(Actor actor) const;

    //! コンポーネントを追加
    template<typename T, typename... Args>
    T* AddComponent(Actor actor, Args&&... args);

    //! コンポーネントを所持しているか
    template<typename T>
    [[nodiscard]] bool HasComponent(Actor actor) const;

    //! コンポーネントを削除
    template<typename T>
    void RemoveComponent(Actor actor);
};

} // namespace ECS

// テンプレート実装はworld.hの後でインクルードが必要
// system_state_impl.h で定義

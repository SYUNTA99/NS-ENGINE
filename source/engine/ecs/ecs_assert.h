//----------------------------------------------------------------------------
//! @file   ecs_assert.h
//! @brief  ECS専用アサートマクロ（Actor/Component情報付き）
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include "common/assert/assert.h"
#include <typeinfo>
#include <cstdio>

namespace ECS {

// 前方宣言
class World;

namespace detail {

//----------------------------------------------------------------------------
//! @brief Actor情報を文字列化
//! @param actor アクター
//! @return フォーマットされた文字列
//----------------------------------------------------------------------------
inline std::string FormatActorInfo(Actor actor) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
        "Actor[id=0x%08X, index=%u, generation=%u, valid=%s]",
        actor.id, actor.Index(), actor.Generation(),
        actor.IsValid() ? "true" : "false");
    return buffer;
}

//----------------------------------------------------------------------------
//! @brief コンポーネント型名を取得
//! @tparam T コンポーネント型
//! @return 型名文字列
//----------------------------------------------------------------------------
template<typename T>
const char* GetTypeName() {
    return typeid(T).name();
}

//----------------------------------------------------------------------------
//! @brief Actor情報付きアサート失敗ログ
//! @param message エラーメッセージ
//! @param actor 対象アクター
//! @param loc ソース位置情報
//----------------------------------------------------------------------------
inline void LogActorAssertFailure(
    const char* message,
    Actor actor,
    const std::source_location& loc)
{
    std::string actorInfo = FormatActorInfo(actor);
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
        "ECS_ASSERT FAILED: %s\n  %s\n  File: %s(%d)\n  Function: %s",
        message, actorInfo.c_str(),
        loc.file_name(), static_cast<int>(loc.line()),
        loc.function_name());
    LOG_ERROR(buffer);
}

//----------------------------------------------------------------------------
//! @brief コンポーネント情報付きアサート失敗ログ
//! @tparam T コンポーネント型
//! @param message エラーメッセージ
//! @param actor 対象アクター
//! @param loc ソース位置情報
//----------------------------------------------------------------------------
template<typename T>
void LogComponentAssertFailure(
    const char* message,
    Actor actor,
    const std::source_location& loc)
{
    std::string actorInfo = FormatActorInfo(actor);
    char buffer[640];
    snprintf(buffer, sizeof(buffer),
        "ECS_ASSERT FAILED: %s\n  %s\n  Component: %s\n  File: %s(%d)\n  Function: %s",
        message, actorInfo.c_str(), GetTypeName<T>(),
        loc.file_name(), static_cast<int>(loc.line()),
        loc.function_name());
    LOG_ERROR(buffer);
}

} // namespace detail

//============================================================================
// 並列コンテキスト検知（Debugビルドのみ）
//============================================================================
#ifdef _DEBUG
//! ParallelForEach内での構造変更を検知するためのスレッドローカルフラグ
inline thread_local bool g_inParallelEcsContext = false;

//! ParallelForEach実行中にフラグを立てるRAIIガード
struct ParallelContextGuard {
    ParallelContextGuard() noexcept { g_inParallelEcsContext = true; }
    ~ParallelContextGuard() noexcept { g_inParallelEcsContext = false; }
    ParallelContextGuard(const ParallelContextGuard&) = delete;
    ParallelContextGuard& operator=(const ParallelContextGuard&) = delete;
};
#endif

} // namespace ECS

//============================================================================
// ECS専用アサートマクロ
//============================================================================

#ifdef _DEBUG

//----------------------------------------------------------------------------
//! @brief 並列コンテキスト内での構造変更を禁止するアサート
//!
//! ParallelForEach内でCreateActor/DestroyActor/AddComponent/RemoveComponent
//! を呼び出すとアサートが発火する。
//!
//! @code
//! void MySpriteSystem::Execute(World& world, float dt) override {
//!     world.ParallelForEach<TransformData>([&](Actor e, auto& t) {
//!         // NG: world.AddComponent<VelocityData>(e); // アサート発火
//!         // OK: world.Deferred().AddComponent<VelocityData>(e);
//!     });
//! }
//! @endcode
//----------------------------------------------------------------------------
#define ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT() \
    assert(!ECS::g_inParallelEcsContext && \
        "Structural changes (CreateActor/DestroyActor/AddComponent/RemoveComponent) " \
        "are forbidden inside ParallelForEach! Use Deferred versions instead.")

//----------------------------------------------------------------------------
//! @brief Actorが有効かつ生存しているかアサート
//! @param world Worldへの参照
//! @param actor 検証するアクター
//!
//! @code
//! void ProcessActor(World& world, Actor actor) {
//!     ECS_ASSERT_VALID_ACTOR(world, actor);
//!     // 安全にactorを使用可能
//! }
//! @endcode
//----------------------------------------------------------------------------
#define ECS_ASSERT_VALID_ACTOR(world, actor) \
    do { \
        if (!(actor).IsValid() || !(world).IsAlive(actor)) { \
            ECS::detail::LogActorAssertFailure( \
                "Actor must be valid and alive", (actor), std::source_location::current()); \
            assert(false && "Actor must be valid and alive"); \
        } \
    } while(0)

//----------------------------------------------------------------------------
//! @brief Actorが指定コンポーネントを持っているかアサート
//! @param world Worldへの参照
//! @param actor 検証するアクター
//! @param T コンポーネント型
//!
//! @code
//! void UpdateTransform(World& world, Actor actor) {
//!     ECS_ASSERT_HAS_COMPONENT(world, actor, TransformData);
//!     auto* transform = world.GetComponent<TransformData>(actor);
//!     // transformは非null
//! }
//! @endcode
//----------------------------------------------------------------------------
#define ECS_ASSERT_HAS_COMPONENT(world, actor, T) \
    do { \
        if (!(world).HasComponent<T>(actor)) { \
            ECS::detail::LogComponentAssertFailure<T>( \
                "Actor must have component", (actor), std::source_location::current()); \
            assert(false && "Actor must have component"); \
        } \
    } while(0)

//----------------------------------------------------------------------------
//! @brief Actorが有効かつ指定コンポーネントを持っているかアサート（複合）
//! @param world Worldへの参照
//! @param actor 検証するアクター
//! @param T コンポーネント型
//!
//! @code
//! void UpdateSprite(World& world, Actor actor) {
//!     ECS_ASSERT_VALID_WITH_COMPONENT(world, actor, SpriteData);
//!     // actorは有効かつSpriteDataを持つ
//! }
//! @endcode
//----------------------------------------------------------------------------
#define ECS_ASSERT_VALID_WITH_COMPONENT(world, actor, T) \
    do { \
        ECS_ASSERT_VALID_ACTOR(world, actor); \
        ECS_ASSERT_HAS_COMPONENT(world, actor, T); \
    } while(0)

//----------------------------------------------------------------------------
//! @brief 汎用ECSアサート（メッセージ付き）
//! @param condition 条件式
//! @param message エラーメッセージ
//!
//! @code
//! ECS_ASSERT(ptr != nullptr, "Component must exist");
//! @endcode
//----------------------------------------------------------------------------
#define ECS_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("ECS_ASSERT FAILED: " message); \
            assert(false && message); \
        } \
    } while(0)

#else
// Releaseビルドでは無効化
#define ECS_ASSERT_NOT_IN_PARALLEL_CONTEXT() ((void)0)
#define ECS_ASSERT_VALID_ACTOR(world, actor) ((void)0)
#define ECS_ASSERT_HAS_COMPONENT(world, actor, T) ((void)0)
#define ECS_ASSERT_VALID_WITH_COMPONENT(world, actor, T) ((void)0)
#define ECS_ASSERT(condition, message) ((void)0)
#endif

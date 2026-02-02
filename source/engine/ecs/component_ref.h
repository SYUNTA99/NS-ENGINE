//----------------------------------------------------------------------------
//! @file   component_ref.h
//! @brief  ECS ComponentRef - 安全なコンポーネント参照
//!
//! フレームをまたいでコンポーネントへの参照を安全に保持するためのラッパー。
//! フレーム境界でのポインタ無効化を自動検出し、必要に応じて再取得する。
//----------------------------------------------------------------------------
#pragma once

#include "actor.h"
#include "ecs_assert.h"
#include <cstdint>

namespace ECS {

// 前方宣言
class World;

//============================================================================
//! @brief 安全なコンポーネント参照
//!
//! コンポーネントポインタをキャッシュし、フレーム境界での無効化を自動検出。
//! キャッシュが古い場合は自動的に再取得する。
//!
//! 使用例:
//! @code
//! // 初回取得
//! ComponentRef<TransformData> transformRef = world.GetRef<TransformData>(actor);
//!
//! // 以降はキャッシュから高速アクセス（同一フレーム内）
//! TransformData& t = transformRef.Get();
//! t.position += velocity;
//!
//! // 次フレームでも自動で再取得
//! world.BeginFrame();  // キャッシュ無効化
//! TransformData& t2 = transformRef.Get();  // 自動再取得
//! @endcode
//!
//! @tparam T コンポーネント型
//!
//! @note スレッドセーフではない。シングルスレッドでの使用を想定。
//! @note Worldの寿命より長く保持しないこと。
//============================================================================
template<typename T>
class ComponentRef {
public:
    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------

    //! デフォルトコンストラクタ（無効な参照）
    ComponentRef() noexcept = default;

    //! 明示的な構築
    //! @param world Worldへのポインタ
    //! @param actor 対象Actor
    //! @param cached 初期キャッシュ（nullptrの場合は遅延取得）
    //! @param frameCounter 取得時のフレームカウンター
    ComponentRef(World* world, Actor actor, T* cached, uint32_t frameCounter) noexcept
        : world_(world)
        , actor_(actor)
        , cached_(cached)
        , version_(frameCounter)
    {}

    //------------------------------------------------------------------------
    // アクセサ
    //------------------------------------------------------------------------

    //! コンポーネントへの参照を取得
    //! @return コンポーネントへの参照
    //! @throw なし（無効な場合はアサート）
    //!
    //! キャッシュが古い場合は自動的に再取得する。
    //! コンポーネントが存在しない場合はアサートで停止。
    [[nodiscard]] T& Get();

    //! コンポーネントへのポインタを取得（失敗時nullptr）
    //! @return コンポーネントへのポインタ、存在しない場合はnullptr
    //!
    //! 安全にコンポーネントを取得したい場合に使用。
    [[nodiscard]] T* TryGet();

    //! const版
    [[nodiscard]] const T& Get() const;
    [[nodiscard]] const T* TryGet() const;

    //! 参照が有効かどうか
    //! @return Worldとアクターが有効ならtrue
    [[nodiscard]] bool IsValid() const noexcept {
        return world_ != nullptr && actor_.IsValid();
    }

    //! 対象Actorを取得
    [[nodiscard]] Actor GetActor() const noexcept {
        return actor_;
    }

    //! Worldを取得
    [[nodiscard]] World& GetWorld() const noexcept {
        return *world_;
    }

    //! キャッシュを明示的に無効化
    void Invalidate() noexcept {
        cached_ = nullptr;
        version_ = UINT32_MAX;
    }

    //------------------------------------------------------------------------
    // 演算子
    //------------------------------------------------------------------------

    //! ポインタ風アクセス
    [[nodiscard]] T* operator->() { return &Get(); }
    [[nodiscard]] const T* operator->() const { return &Get(); }

    //! 参照風アクセス
    [[nodiscard]] T& operator*() { return Get(); }
    [[nodiscard]] const T& operator*() const { return Get(); }

    //! bool変換（有効性チェック）
    [[nodiscard]] explicit operator bool() const noexcept {
        return IsValid();
    }

private:
    //! キャッシュを更新
    void RefreshCache() const;

    World* world_ = nullptr;          //!< Worldへのポインタ
    Actor actor_;                      //!< 対象Actor
    mutable T* cached_ = nullptr;      //!< キャッシュされたポインタ
    mutable uint32_t version_ = UINT32_MAX;  //!< 取得時のフレームカウンター
};

//============================================================================
//! @brief 読み取り専用コンポーネント参照
//!
//! ComponentRefのconst版。書き込みを禁止した安全な参照。
//!
//! @tparam T コンポーネント型
//============================================================================
template<typename T>
class ComponentConstRef {
public:
    ComponentConstRef() noexcept = default;

    ComponentConstRef(const World* world, Actor actor, const T* cached, uint32_t frameCounter) noexcept
        : world_(world)
        , actor_(actor)
        , cached_(cached)
        , version_(frameCounter)
    {}

    //! ComponentRefからの暗黙変換
    ComponentConstRef(const ComponentRef<T>& ref) noexcept
        : world_(ref.IsValid() ? &ref.GetWorld() : nullptr)
        , actor_(ref.GetActor())
        , cached_(nullptr)
        , version_(UINT32_MAX)
    {}

    [[nodiscard]] const T& Get() const;
    [[nodiscard]] const T* TryGet() const;

    [[nodiscard]] bool IsValid() const noexcept {
        return world_ != nullptr && actor_.IsValid();
    }

    [[nodiscard]] Actor GetActor() const noexcept {
        return actor_;
    }

    void Invalidate() noexcept {
        cached_ = nullptr;
        version_ = UINT32_MAX;
    }

    [[nodiscard]] const T* operator->() const { return &Get(); }
    [[nodiscard]] const T& operator*() const { return Get(); }
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

private:
    void RefreshCache() const;

    const World* world_ = nullptr;
    Actor actor_;
    mutable const T* cached_ = nullptr;
    mutable uint32_t version_ = UINT32_MAX;
};

} // namespace ECS

//----------------------------------------------------------------------------
//! @file   foreach_helpers.h
//! @brief  ECS ForEach - SoA Variadic Template Helpers
//----------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ECS {

// 前方宣言
struct Actor;

namespace detail {

//============================================================================
//! @brief SoA用: index_sequenceを使ってコンポーネントポインタを展開し関数呼び出し
//!
//! @tparam Ts コンポーネント型群
//! @tparam Is インデックスシーケンス
//! @tparam Func コールバック関数型
//!
//! @param func コールバック関数
//! @param actor 対象Actor
//! @param entityIndex Chunk内のエンティティインデックス
//! @param arrayBases 各コンポーネント配列の先頭ポインタ
//============================================================================
template<typename... Ts, size_t... Is, typename Func>
inline void InvokeFuncImplSoA(
    Func&& func,
    Actor actor,
    uint16_t entityIndex,
    const std::array<std::byte*, sizeof...(Ts)>& arrayBases,
    std::index_sequence<Is...>)
{
    func(
        actor,
        reinterpret_cast<Ts*>(arrayBases[Is])[entityIndex]...
    );
}

//============================================================================
//! @brief SoA用: コンポーネントポインタを展開して関数を呼び出す
//!
//! std::index_sequence_for<Ts...> を自動生成してInvokeFuncImplSoAに委譲。
//!
//! @tparam Ts コンポーネント型群
//! @tparam Func コールバック関数型
//============================================================================
template<typename... Ts, typename Func>
inline void InvokeWithComponentsSoA(
    Func&& func,
    Actor actor,
    uint16_t entityIndex,
    const std::array<std::byte*, sizeof...(Ts)>& arrayBases)
{
    InvokeFuncImplSoA<Ts...>(
        std::forward<Func>(func),
        actor,
        entityIndex,
        arrayBases,
        std::index_sequence_for<Ts...>{}
    );
}

//============================================================================
//! @brief SoA const版 - 読み取り専用イテレーション用
//============================================================================
template<typename... Ts, size_t... Is, typename Func>
inline void InvokeFuncConstImplSoA(
    Func&& func,
    Actor actor,
    uint16_t entityIndex,
    const std::array<const std::byte*, sizeof...(Ts)>& arrayBases,
    std::index_sequence<Is...>)
{
    func(
        actor,
        reinterpret_cast<const Ts*>(arrayBases[Is])[entityIndex]...
    );
}

template<typename... Ts, typename Func>
inline void InvokeWithComponentsConstSoA(
    Func&& func,
    Actor actor,
    uint16_t entityIndex,
    const std::array<const std::byte*, sizeof...(Ts)>& arrayBases)
{
    InvokeFuncConstImplSoA<Ts...>(
        std::forward<Func>(func),
        actor,
        entityIndex,
        arrayBases,
        std::index_sequence_for<Ts...>{}
    );
}

} // namespace detail
} // namespace ECS

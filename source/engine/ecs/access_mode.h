//----------------------------------------------------------------------------
//! @file   access_mode.h
//! @brief  ECS Access Mode - コンポーネントアクセスモード指定
//----------------------------------------------------------------------------
#pragma once

#include <type_traits>

namespace ECS {

//============================================================================
//! @brief 読み取り専用アクセス（入力）
//!
//! ForEachでコンポーネントを読み取り専用でアクセスする場合に使用。
//! ラムダには const T& として渡される。
//!
//! @tparam T コンポーネント型
//!
//! @code
//! world.Actors().Query<In<VelocityData>>()
//!     .ForEach([](Actor e, const VelocityData& v) {
//!         // v は読み取り専用
//!     });
//! @endcode
//============================================================================
template<typename T>
struct In {
    using Type = T;
    static constexpr bool IsReadOnly = true;
    static constexpr bool IsWriteOnly = false;
};

//============================================================================
//! @brief 読み書き両方アクセス
//!
//! ForEachでコンポーネントを読み書き両方でアクセスする場合に使用。
//! ラムダには T& として渡される。
//!
//! @tparam T コンポーネント型
//!
//! @code
//! world.Actors().Query<InOut<TransformData>, In<VelocityData>>()
//!     .ForEach([dt](Actor e, TransformData& t, const VelocityData& v) {
//!         t.position += v.velocity * dt;  // 読み書き
//!     });
//! @endcode
//============================================================================
template<typename T>
struct InOut {
    using Type = T;
    static constexpr bool IsReadOnly = false;
    static constexpr bool IsWriteOnly = false;
};

//============================================================================
// 型特性（Type Traits）
//============================================================================

namespace detail {

// In<T> 判定
template<typename T>
struct is_in : std::false_type {};

template<typename T>
struct is_in<In<T>> : std::true_type {};

// InOut<T> 判定
template<typename T>
struct is_inout : std::false_type {};

template<typename T>
struct is_inout<InOut<T>> : std::true_type {};

// アクセスモード判定（In/InOutのいずれか）
template<typename T>
struct is_access_mode : std::bool_constant<
    is_in<T>::value || is_inout<T>::value
> {};

// コンポーネント型を抽出
template<typename T>
struct unwrap_access {
    using type = T;  // アクセスモードでない場合はそのまま
};

template<typename T>
struct unwrap_access<In<T>> {
    using type = T;
};

template<typename T>
struct unwrap_access<InOut<T>> {
    using type = T;
};

} // namespace detail

//============================================================================
// ヘルパー変数テンプレート
//============================================================================

template<typename T>
inline constexpr bool is_in_v = detail::is_in<T>::value;

template<typename T>
inline constexpr bool is_inout_v = detail::is_inout<T>::value;

template<typename T>
inline constexpr bool is_access_mode_v = detail::is_access_mode<T>::value;

//! @brief 全ての型がアクセスモード（In/InOut）かどうか
template<typename... Ts>
inline constexpr bool all_are_access_modes_v = (is_access_mode_v<Ts> && ...);

template<typename T>
using unwrap_access_t = typename detail::unwrap_access<T>::type;

//============================================================================
// 引数型の決定（In→const T&, Out/InOut→T&）
//============================================================================

namespace detail {

template<typename AccessMode>
struct arg_type {
    using type = unwrap_access_t<AccessMode>&;  // デフォルトはT&
};

template<typename T>
struct arg_type<In<T>> {
    using type = const T&;  // In<T>はconst T&
};

} // namespace detail

template<typename AccessMode>
using arg_type_t = typename detail::arg_type<AccessMode>::type;

//============================================================================
// 並列処理用のRead/Write分類
//============================================================================

namespace detail {

//! 読み取りアクセスかどうか（In<T>またはInOut<T>）
template<typename T>
struct is_read_access : std::false_type {};

template<typename T>
struct is_read_access<In<T>> : std::true_type {};

template<typename T>
struct is_read_access<InOut<T>> : std::true_type {};

//! 書き込みアクセスかどうか（InOut<T>）
template<typename T>
struct is_write_access : std::false_type {};

template<typename T>
struct is_write_access<InOut<T>> : std::true_type {};

//============================================================================
// 型リストフィルタリング（Read/Write抽出）
//============================================================================

// 型リスト
template<typename... Ts>
struct type_list {};

// 型リストに型を追加
template<typename List, typename T>
struct type_list_append;

template<typename... Ts, typename T>
struct type_list_append<type_list<Ts...>, T> {
    using type = type_list<Ts..., T>;
};

template<typename List, typename T>
using type_list_append_t = typename type_list_append<List, T>::type;

// 条件を満たす型を抽出（fold expression版）
template<template<typename> class Pred, typename... Ts>
struct filter_types;

template<template<typename> class Pred>
struct filter_types<Pred> {
    using type = type_list<>;
};

template<template<typename> class Pred, typename T, typename... Rest>
struct filter_types<Pred, T, Rest...> {
    using rest_filtered = typename filter_types<Pred, Rest...>::type;
    using type = std::conditional_t<
        Pred<T>::value,
        typename type_list_append<rest_filtered, unwrap_access_t<T>>::type,
        rest_filtered
    >;
};

template<template<typename> class Pred, typename... Ts>
using filter_types_t = typename filter_types<Pred, Ts...>::type;

// Readアクセスの型を抽出
template<typename... AccessModes>
using filter_reads_t = filter_types_t<is_read_access, AccessModes...>;

// Writeアクセスの型を抽出
template<typename... AccessModes>
using filter_writes_t = filter_types_t<is_write_access, AccessModes...>;

//============================================================================
// 型リスト内に特定の型が含まれるか
//============================================================================

template<typename T, typename List>
struct type_list_contains : std::false_type {};

template<typename T, typename... Ts>
struct type_list_contains<T, type_list<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template<typename T, typename List>
inline constexpr bool type_list_contains_v = type_list_contains<T, List>::value;

//============================================================================
// 2つの型リストに共通の型があるか（競合検出）
//============================================================================

template<typename List1, typename List2>
struct type_lists_overlap;

template<typename List2>
struct type_lists_overlap<type_list<>, List2> : std::false_type {};

template<typename T, typename... Rest, typename List2>
struct type_lists_overlap<type_list<T, Rest...>, List2>
    : std::bool_constant<
        type_list_contains_v<T, List2> ||
        type_lists_overlap<type_list<Rest...>, List2>::value
    > {};

template<typename List1, typename List2>
inline constexpr bool type_lists_overlap_v = type_lists_overlap<List1, List2>::value;

//============================================================================
// アクセスモード間の競合検出
//============================================================================

//! 2つのアクセスモードセット間で競合があるか
//! 競合 = 一方がWriteし、もう一方がRead/Writeする同じ型がある
template<typename AccessModes1, typename AccessModes2>
struct has_access_conflict;

template<typename... A1s, typename... A2s>
struct has_access_conflict<type_list<A1s...>, type_list<A2s...>> {
    using writes1 = filter_writes_t<A1s...>;
    using writes2 = filter_writes_t<A2s...>;
    using reads1 = filter_reads_t<A1s...>;
    using reads2 = filter_reads_t<A2s...>;

    // 競合条件:
    // - A1のWriteとA2のRead/Writeが重複
    // - A2のWriteとA1のRead/Writeが重複
    static constexpr bool value =
        type_lists_overlap_v<writes1, reads2> ||
        type_lists_overlap_v<writes1, writes2> ||
        type_lists_overlap_v<writes2, reads1>;
};

template<typename AccessModes1, typename AccessModes2>
inline constexpr bool has_access_conflict_v = has_access_conflict<AccessModes1, AccessModes2>::value;

//! 単一のアクセスモードセット内での重複チェック（同じ型を2回アクセス）
template<typename... AccessModes>
struct has_duplicate_component;

template<>
struct has_duplicate_component<> : std::false_type {};

template<typename First, typename... Rest>
struct has_duplicate_component<First, Rest...> {
    using T = unwrap_access_t<First>;
    // Restの中にTと同じコンポーネント型があるか
    static constexpr bool has_dup = ((std::is_same_v<T, unwrap_access_t<Rest>>) || ...);
    static constexpr bool value = has_dup || has_duplicate_component<Rest...>::value;
};

template<typename... AccessModes>
inline constexpr bool has_duplicate_component_v = has_duplicate_component<AccessModes...>::value;

} // namespace detail

//============================================================================
// 公開API用ヘルパー
//============================================================================

template<typename T>
inline constexpr bool is_read_access_v = detail::is_read_access<T>::value;

template<typename T>
inline constexpr bool is_write_access_v = detail::is_write_access<T>::value;

} // namespace ECS

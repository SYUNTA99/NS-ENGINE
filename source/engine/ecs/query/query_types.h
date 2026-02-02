//----------------------------------------------------------------------------
//! @file   query_types.h
//! @brief  ECS Query Types - Optional/Exclude タグ型と型特性
//----------------------------------------------------------------------------
#pragma once

#include <type_traits>
#include <tuple>

namespace ECS {

//============================================================================
//! @brief Optional<T> - コンポーネントが存在すればT*、なければnullptr
//!
//! 使用例:
//! ```cpp
//! world.Query<TransformData, Optional<VelocityData>>()
//!      .ForEach([](Actor e, TransformData& t, VelocityData* v) {
//!          if (v) {
//!              t.position += v->velocity * dt;
//!          }
//!      });
//! ```
//============================================================================
template<typename T>
struct Optional {
    using Type = T;
};

//============================================================================
//! @brief Exclude<T> - Tを持たないEntityのみマッチ
//!
//! 使用例:
//! ```cpp
//! world.Query<TransformData, Exclude<Dead>>()
//!      .ForEach([](Actor e, TransformData& t) {
//!          // Dead コンポーネントを持たない Entity のみ
//!      });
//! ```
//============================================================================
template<typename T>
struct Exclude {
    using Type = T;
};

//============================================================================
// 型特性ヘルパー
//============================================================================

namespace detail {

//! Optional<T> かどうか判定
template<typename T>
struct is_optional : std::false_type {};

template<typename T>
struct is_optional<Optional<T>> : std::true_type {};

//! Exclude<T> かどうか判定
template<typename T>
struct is_exclude : std::false_type {};

template<typename T>
struct is_exclude<Exclude<T>> : std::true_type {};

//! 内部コンポーネント型を抽出
template<typename T>
struct unwrap_component {
    using type = T;
};

template<typename T>
struct unwrap_component<Optional<T>> {
    using type = T;
};

template<typename T>
struct unwrap_component<Exclude<T>> {
    using type = T;
};

//! コールバック引数型を決定
//! T           -> T&
//! Optional<T> -> T*
//! Exclude<T>  -> (引数として渡さない)
template<typename T>
struct callback_arg_type {
    using type = T&;
};

template<typename T>
struct callback_arg_type<Optional<T>> {
    using type = T*;
};

// Exclude<T> は引数として渡さないため、特殊化しない

//============================================================================
// 型フィルタリングヘルパー
//============================================================================

//! Required（必須）型のみを抽出したタプル
template<typename... Ts>
struct filter_required;

template<>
struct filter_required<> {
    using types = std::tuple<>;
};

template<typename T, typename... Rest>
struct filter_required<T, Rest...> {
    using rest = typename filter_required<Rest...>::types;
    using types = std::conditional_t<
        is_exclude<T>::value,
        rest,  // Exclude は除外
        decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<rest>()))
    >;
};

//! Exclude型のみを抽出したタプル
template<typename... Ts>
struct filter_excludes;

template<>
struct filter_excludes<> {
    using types = std::tuple<>;
};

template<typename T, typename... Rest>
struct filter_excludes<T, Rest...> {
    using rest = typename filter_excludes<Rest...>::types;
    using types = std::conditional_t<
        is_exclude<T>::value,
        decltype(std::tuple_cat(std::declval<std::tuple<typename unwrap_component<T>::type>>(), std::declval<rest>())),
        rest  // Exclude でなければ除外
    >;
};

//! Optional型のみを抽出
template<typename... Ts>
struct filter_optionals;

template<>
struct filter_optionals<> {
    using types = std::tuple<>;
};

template<typename T, typename... Rest>
struct filter_optionals<T, Rest...> {
    using rest = typename filter_optionals<Rest...>::types;
    using types = std::conditional_t<
        is_optional<T>::value,
        decltype(std::tuple_cat(std::declval<std::tuple<typename unwrap_component<T>::type>>(), std::declval<rest>())),
        rest
    >;
};

//! 純粋なRequired型（Optional/Excludeでない）のみを抽出
template<typename... Ts>
struct filter_pure_required;

template<>
struct filter_pure_required<> {
    using types = std::tuple<>;
};

template<typename T, typename... Rest>
struct filter_pure_required<T, Rest...> {
    using rest = typename filter_pure_required<Rest...>::types;
    static constexpr bool is_pure = !is_optional<T>::value && !is_exclude<T>::value;
    using types = std::conditional_t<
        is_pure,
        decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<rest>())),
        rest
    >;
};

} // namespace detail

//============================================================================
// 便利なエイリアス
//============================================================================

template<typename T>
inline constexpr bool is_optional_v = detail::is_optional<T>::value;

template<typename T>
inline constexpr bool is_exclude_v = detail::is_exclude<T>::value;

template<typename T>
inline constexpr bool is_required_v = !is_optional_v<T> && !is_exclude_v<T>;

template<typename T>
using unwrap_component_t = typename detail::unwrap_component<T>::type;

template<typename T>
using callback_arg_t = typename detail::callback_arg_type<T>::type;

template<typename... Ts>
using filter_required_t = typename detail::filter_required<Ts...>::types;

template<typename... Ts>
using filter_excludes_t = typename detail::filter_excludes<Ts...>::types;

template<typename... Ts>
using filter_optionals_t = typename detail::filter_optionals<Ts...>::types;

template<typename... Ts>
using filter_pure_required_t = typename detail::filter_pure_required<Ts...>::types;

} // namespace ECS

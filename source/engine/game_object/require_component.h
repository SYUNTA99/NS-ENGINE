//----------------------------------------------------------------------------
//! @file   require_component.h
//! @brief  RequireComponent - コンポーネント依存宣言
//----------------------------------------------------------------------------
#pragma once

#include <tuple>
#include <type_traits>

//============================================================================
//! @brief RequireComponent検出用トレイト
//============================================================================
namespace detail {

// HasRequiredECSComponents 検出
template<typename T, typename = void>
struct HasRequiredECSComponents : std::false_type {};

template<typename T>
struct HasRequiredECSComponents<T, std::void_t<decltype(T::RequiredECSComponents())>> : std::true_type {};

template<typename T>
inline constexpr bool HasRequiredECSComponents_v = HasRequiredECSComponents<T>::value;

// HasRequiredOOPComponents 検出
template<typename T, typename = void>
struct HasRequiredOOPComponents : std::false_type {};

template<typename T>
struct HasRequiredOOPComponents<T, std::void_t<decltype(T::RequiredOOPComponents())>> : std::true_type {};

template<typename T>
inline constexpr bool HasRequiredOOPComponents_v = HasRequiredOOPComponents<T>::value;

} // namespace detail

//============================================================================
//! @brief ECSコンポーネント依存宣言マクロ
//!
//! このコンポーネントが必要とするECSデータコンポーネントを宣言。
//! AddComponent時に自動的に追加される。
//!
//! @code
//! class PlayerController final : public Component {
//!     REQUIRE_ECS_COMPONENTS(PositionData, RotationData);
//!     // ...
//! };
//! @endcode
//============================================================================
#define REQUIRE_ECS_COMPONENTS(...)                                           \
    static auto RequiredECSComponents() {                                     \
        return std::tuple<__VA_ARGS__>{};                                     \
    }                                                                         \
    using RequiredECSTypes = std::tuple<__VA_ARGS__>

//============================================================================
//! @brief OOPコンポーネント依存宣言マクロ
//!
//! このコンポーネントが必要とする他のOOPコンポーネントを宣言。
//! AddComponent時に自動的に追加される。
//!
//! @code
//! class PlayerAnimator final : public Component {
//!     REQUIRE_OOP_COMPONENTS(PlayerController);
//!     // ...
//! };
//! @endcode
//============================================================================
#define REQUIRE_OOP_COMPONENTS(...)                                           \
    static auto RequiredOOPComponents() {                                     \
        return std::tuple<__VA_ARGS__>{};                                     \
    }                                                                         \
    using RequiredOOPTypes = std::tuple<__VA_ARGS__>

//============================================================================
//! @brief RequireComponent適用ヘルパー
//============================================================================
namespace detail {

// ECSコンポーネント追加ヘルパー
template<typename GameObject, typename Tuple, size_t... Is>
void AddRequiredECSComponentsImpl(GameObject* go, std::index_sequence<Is...>) {
    // 各型についてHasECS<T>()がfalseならAddECS<T>()を呼ぶ
    (
        [&]() {
            using T = std::tuple_element_t<Is, Tuple>;
            if (!go->template HasECS<T>()) {
                go->template AddECS<T>();
            }
        }(),
        ...
    );
}

template<typename T, typename GameObject>
void AddRequiredECSComponents(GameObject* go) {
    if constexpr (HasRequiredECSComponents_v<T>) {
        using Tuple = decltype(T::RequiredECSComponents());
        constexpr size_t N = std::tuple_size_v<Tuple>;
        AddRequiredECSComponentsImpl<GameObject, Tuple>(go, std::make_index_sequence<N>{});
    }
}

// OOPコンポーネント追加ヘルパー
template<typename GameObject, typename Tuple, size_t... Is>
void AddRequiredOOPComponentsImpl(GameObject* go, std::index_sequence<Is...>) {
    // 各型についてHasComponent<T>()がfalseならAddComponent<T>()を呼ぶ
    (
        [&]() {
            using T = std::tuple_element_t<Is, Tuple>;
            if (!go->template HasComponent<T>()) {
                go->template AddComponent<T>();
            }
        }(),
        ...
    );
}

template<typename T, typename GameObject>
void AddRequiredOOPComponents(GameObject* go) {
    if constexpr (HasRequiredOOPComponents_v<T>) {
        using Tuple = decltype(T::RequiredOOPComponents());
        constexpr size_t N = std::tuple_size_v<Tuple>;
        AddRequiredOOPComponentsImpl<GameObject, Tuple>(go, std::make_index_sequence<N>{});
    }
}

} // namespace detail

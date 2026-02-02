//----------------------------------------------------------------------------
//! @file   component_data.h
//! @brief  ECS ComponentData基底クラス
//----------------------------------------------------------------------------
#pragma once

#include "common/stl/stl_metaprogramming.h"

namespace ECS {

//============================================================================
//! @brief コンポーネントデータの基底クラス
//!
//! 全てのECSコンポーネントはこれを継承する。
//! 型チェックとエディタ対応のため。
//!
//! @note trivially copyable を維持するため、virtualは使用しない。
//!       デストラクタはArchetypeが型消去経由で呼び出す。
//!
//! @code
//! struct VelocityData : IComponentData {
//!     Vector3 velocity;
//! };
//! ECS_COMPONENT(VelocityData);  // コンパイル時に検証
//! @endcode
//============================================================================
struct IComponentData {
    // trivially copyable維持のため、virtualデストラクタは使用しない
    // デストラクタが必要な場合は、Archetype側でComponentInfoを通じて呼び出す
};

//============================================================================
//! @brief Tagコンポーネントの基底クラス
//!
//! データを持たないマーカーコンポーネント。フィルタリング専用。
//! Archetype内でストレージを消費しない（サイズ0として扱う）。
//!
//! @code
//! struct DeadTag : ITagComponentData {};
//! struct PlayerTag : ITagComponentData {};
//! ECS_TAG_COMPONENT(DeadTag);
//!
//! // フィルタリングに使用
//! world.Query<TransformData>()
//!     .Exclude<DeadTag>()
//!     .ForEach([](Actor e, auto& t) { ... });
//! @endcode
//============================================================================
struct ITagComponentData : IComponentData {
    // 空の構造体（C++ではsizeof>=1だが、ECSでは0として扱う）
};

//============================================================================
//! @brief Tag型判定
//!
//! ITagComponentDataを継承し、かつ空の構造体であればtrue。
//! Archetype::CalculateLayout()等でサイズ0として扱うために使用。
//============================================================================
template<typename T>
inline constexpr bool is_tag_component_v =
    std::is_base_of_v<ITagComponentData, T> && std::is_empty_v<T>;

//============================================================================
//! @brief DynamicBuffer型のマーカー基底
//!
//! DynamicBuffer<T>がこれを継承することで、Archetypeがバッファコンポーネントを
//! 識別できるようにする。
//============================================================================
struct IBufferComponentData : IComponentData {
    // バッファコンポーネントのマーカー
};

//============================================================================
//! @brief Buffer型判定
//!
//! IBufferComponentDataを継承していればtrue。
//! Archetype::CalculateLayout()等でバッファ特殊処理に使用。
//============================================================================
template<typename T>
inline constexpr bool is_buffer_component_v =
    std::is_base_of_v<IBufferComponentData, T>;

} // namespace ECS

//============================================================================
//! @brief コンポーネント定義検証マクロ
//!
//! コンポーネント型がECSの要件を満たしているかコンパイル時に検証する。
//! - trivially copyable（memcpyで移動可能）
//! - IComponentDataを継承
//!
//! @param Type 検証するコンポーネント型
//!
//! @code
//! struct MyComponent : ECS::IComponentData {
//!     float value;
//! };
//! ECS_COMPONENT(MyComponent);  // OK
//!
//! struct BadComponent : ECS::IComponentData {
//!     std::string name;  // trivially copyableではない
//! };
//! ECS_COMPONENT(BadComponent);  // コンパイルエラー
//! @endcode
//============================================================================
#define ECS_COMPONENT(Type)                                                     \
    static_assert(std::is_trivially_copyable_v<Type>,                          \
        #Type " must be trivially copyable for ECS storage");                   \
    static_assert(std::is_base_of_v<ECS::IComponentData, Type>,                \
        #Type " must inherit from ECS::IComponentData")

//============================================================================
//! @brief Tagコンポーネント定義検証マクロ
//!
//! Tagコンポーネント型がECSの要件を満たしているかコンパイル時に検証する。
//! - ITagComponentDataを継承
//! - 空の構造体（メンバ変数なし）
//!
//! @param Type 検証するTagコンポーネント型
//!
//! @code
//! struct PlayerTag : ECS::ITagComponentData {};
//! ECS_TAG_COMPONENT(PlayerTag);  // OK
//!
//! struct BadTag : ECS::ITagComponentData {
//!     int data;  // メンバがあるのでNG
//! };
//! ECS_TAG_COMPONENT(BadTag);  // コンパイルエラー
//! @endcode
//============================================================================
#define ECS_TAG_COMPONENT(Type)                                                 \
    static_assert(std::is_base_of_v<ECS::ITagComponentData, Type>,             \
        #Type " must inherit from ECS::ITagComponentData");                     \
    static_assert(std::is_empty_v<Type>,                                        \
        #Type " must be an empty struct (no data members)")

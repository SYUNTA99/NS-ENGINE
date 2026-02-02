//----------------------------------------------------------------------------
//! @file   buffer_element.h
//! @brief  ECS IBufferElement - DynamicBuffer要素の基底クラス
//----------------------------------------------------------------------------
#pragma once

#include <type_traits>

namespace ECS {

//============================================================================
//! @brief DynamicBuffer要素の基底マーカー
//!
//! DynamicBufferに格納する要素型はこれを継承する必要がある。
//! trivially copyable でなければならない。
//!
//! @code
//! struct Waypoint : IBufferElement {
//!     float x, y, z;
//! };
//! ECS_BUFFER_ELEMENT(Waypoint);
//!
//! struct Child : IBufferElement {
//!     Actor actor;
//! };
//! ECS_BUFFER_ELEMENT(Child);
//! @endcode
//============================================================================
struct IBufferElement {
    // マーカー基底（メンバなし）
};

//============================================================================
//! @brief Buffer要素型判定
//!
//! IBufferElementを継承し、かつtrivially_copyableであればtrue。
//============================================================================
template<typename T>
inline constexpr bool is_buffer_element_v =
    std::is_base_of_v<IBufferElement, T> && std::is_trivially_copyable_v<T>;

} // namespace ECS

//============================================================================
//! @brief Buffer要素定義検証マクロ
//!
//! Buffer要素型がECSの要件を満たしているかコンパイル時に検証する。
//! - trivially copyable（memcpyで移動可能）
//! - IBufferElementを継承
//!
//! @param Type 検証するBuffer要素型
//!
//! @code
//! struct Waypoint : ECS::IBufferElement {
//!     float x, y, z;
//! };
//! ECS_BUFFER_ELEMENT(Waypoint);  // OK
//!
//! struct BadElement : ECS::IBufferElement {
//!     std::string name;  // trivially copyableではない
//! };
//! ECS_BUFFER_ELEMENT(BadElement);  // コンパイルエラー
//! @endcode
//============================================================================
#define ECS_BUFFER_ELEMENT(Type)                                               \
    static_assert(std::is_trivially_copyable_v<Type>,                         \
        #Type " must be trivially copyable for DynamicBuffer storage");        \
    static_assert(std::is_base_of_v<ECS::IBufferElement, Type>,               \
        #Type " must inherit from ECS::IBufferElement")

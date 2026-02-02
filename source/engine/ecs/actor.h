//----------------------------------------------------------------------------
//! @file   actor.h
//! @brief  ECS Actor - 軽量アクターID
//----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <functional>

namespace ECS {

//============================================================================
//! @brief アクターID
//!
//! 32ビットの軽量ID。インデックスと世代番号で構成される。
//! - index: アクター配列内の位置 (20bit, 最大約100万)
//! - generation: 再利用検出用の世代番号 (12bit, 最大4096)
//============================================================================
struct Actor {
    uint32_t id = kInvalidId;

    //! 無効なアクターを示す定数
    static constexpr uint32_t kInvalidId = 0xFFFFFFFF;
    static constexpr uint32_t kIndexBits = 20;
    static constexpr uint32_t kGenerationBits = 12;
    static constexpr uint32_t kIndexMask = (1u << kIndexBits) - 1;
    static constexpr uint32_t kGenerationMask = (1u << kGenerationBits) - 1;

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------

    constexpr Actor() noexcept = default;
    constexpr explicit Actor(uint32_t rawId) noexcept : id(rawId) {}
    constexpr Actor(uint32_t index, uint32_t generation) noexcept
        : id((generation << kIndexBits) | (index & kIndexMask)) {}

    //------------------------------------------------------------------------
    // アクセサ
    //------------------------------------------------------------------------

    //! インデックス取得
    [[nodiscard]] constexpr uint32_t Index() const noexcept {
        return id & kIndexMask;
    }

    //! 世代番号取得
    [[nodiscard]] constexpr uint32_t Generation() const noexcept {
        return (id >> kIndexBits) & kGenerationMask;
    }

    //! 有効なアクターかどうか
    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return id != kInvalidId;
    }

    //! 無効なアクターを返す
    [[nodiscard]] static constexpr Actor Invalid() noexcept {
        return Actor{kInvalidId};
    }

    //------------------------------------------------------------------------
    // 比較演算子
    //------------------------------------------------------------------------

    [[nodiscard]] constexpr bool operator==(Actor other) const noexcept {
        return id == other.id;
    }

    [[nodiscard]] constexpr bool operator!=(Actor other) const noexcept {
        return id != other.id;
    }

    [[nodiscard]] constexpr bool operator<(Actor other) const noexcept {
        return id < other.id;
    }
};
} // namespace ECS

//============================================================================
// std::hash 特殊化
//============================================================================
namespace std {

template<>
struct hash<ECS::Actor> {
    size_t operator()(ECS::Actor a) const noexcept {
        return static_cast<size_t>(a.id);
    }
};

} // namespace std

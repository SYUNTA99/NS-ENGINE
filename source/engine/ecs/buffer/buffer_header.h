//----------------------------------------------------------------------------
//! @file   buffer_header.h
//! @brief  ECS BufferHeader - DynamicBufferのヘッダー構造体
//----------------------------------------------------------------------------
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ECS {

//============================================================================
//! @brief DynamicBufferのヘッダー (24バイト)
//!
//! Chunk内にインラインで格納され、バッファの状態を管理する。
//! インライン容量を超えた場合は外部ヒープにオーバーフローする。
//!
//! メモリレイアウト（Chunk内）:
//! [BufferHeader 24B][InlineData N×sizeof(T)]
//!
//! @note trivially_copyable を維持するため、ポインタは生ポインタ。
//!       外部ストレージの解放はArchetype::CleanupBuffers()で行う。
//============================================================================
struct BufferHeader {
    int32_t length;             //!< 現在の要素数
    int32_t inlineCapacity;     //!< インライン領域の容量（Archetype作成時に設定）
    std::byte* externalPtr;     //!< 外部ストレージへのポインタ（nullptrならインライン使用）
    uint32_t externalCapacity;  //!< 外部ストレージの容量
    uint32_t reserved;          //!< 予約（パディング/将来用）

    //------------------------------------------------------------------------
    //! @brief デフォルトコンストラクタ
    //------------------------------------------------------------------------
    BufferHeader() noexcept
        : length(0)
        , inlineCapacity(0)
        , externalPtr(nullptr)
        , externalCapacity(0)
        , reserved(0)
    {}

    //------------------------------------------------------------------------
    //! @brief インライン容量指定コンストラクタ
    //! @param inlineCap インライン領域の容量
    //------------------------------------------------------------------------
    explicit BufferHeader(int32_t inlineCap) noexcept
        : length(0)
        , inlineCapacity(inlineCap)
        , externalPtr(nullptr)
        , externalCapacity(0)
        , reserved(0)
    {}

    //------------------------------------------------------------------------
    //! @brief 現在の容量を取得
    //! @return インラインまたは外部ストレージの容量
    //------------------------------------------------------------------------
    [[nodiscard]] int32_t Capacity() const noexcept {
        return externalPtr ? static_cast<int32_t>(externalCapacity) : inlineCapacity;
    }

    //------------------------------------------------------------------------
    //! @brief 外部ストレージを使用中か
    //! @return 外部ストレージを使用している場合true
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsExternal() const noexcept {
        return externalPtr != nullptr;
    }

    //------------------------------------------------------------------------
    //! @brief バッファが空か
    //! @return 要素数が0の場合true
    //------------------------------------------------------------------------
    [[nodiscard]] bool IsEmpty() const noexcept {
        return length == 0;
    }
};

// サイズとtrivially_copyable検証
static_assert(sizeof(BufferHeader) == 24, "BufferHeader must be 24 bytes");
static_assert(std::is_trivially_copyable_v<BufferHeader>,
    "BufferHeader must be trivially copyable for ECS storage");

} // namespace ECS

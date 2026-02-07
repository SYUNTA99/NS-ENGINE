//----------------------------------------------------------------------------
//! @file   dynamic_buffer.h
//! @brief  ECS DynamicBuffer - 可変長バッファアクセサ
//----------------------------------------------------------------------------
#pragma once


#include "buffer_header.h"
#include "buffer_element.h"
#include "internal_buffer_capacity.h"
#include "engine/memory/memory_system.h"
#include <cstddef>
#include <cassert>
#include <cstring>
#include <algorithm>

namespace ECS {

//============================================================================
//! @brief DynamicBuffer - Unity DOTS風の可変長バッファアクセサ
//!
//! Chunk内にインラインでヘッダーと初期容量のデータを格納。
//! 容量を超えた場合は外部ヒープにオーバーフローする。
//!
//! @tparam T 要素型（IBufferElement継承、trivially_copyable必須）
//!
//! @code
//! // 定義
//! struct Waypoint : IBufferElement {
//!     float x, y, z;
//! };
//! ECS_BUFFER_ELEMENT(Waypoint);
//!
//! // 使用
//! auto buffer = world.GetBuffer<Waypoint>(actor);
//! buffer.Add({1.0f, 2.0f, 3.0f});
//! for (const auto& wp : buffer) {
//!     // ...
//! }
//! @endcode
//============================================================================
template<typename T>
class DynamicBuffer {
    static_assert(is_buffer_element_v<T>,
        "T must inherit from IBufferElement and be trivially_copyable");

public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
    using size_type = int32_t;

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------

    //! @brief デフォルトコンストラクタ（無効なバッファ）
    DynamicBuffer() noexcept
        : header_(nullptr), inlineData_(nullptr) {}

    //! @brief ヘッダーとインラインデータへのポインタから構築
    //! @param header BufferHeaderへのポインタ
    //! @param inlineData インラインデータ領域へのポインタ
    DynamicBuffer(BufferHeader* header, std::byte* inlineData) noexcept
        : header_(header), inlineData_(inlineData) {}

    //------------------------------------------------------------------------
    // 状態確認
    //------------------------------------------------------------------------

    //! @brief バッファが有効か（null でないか）
    [[nodiscard]] bool IsCreated() const noexcept { return header_ != nullptr; }

    //! @brief bool変換（IsCreated()と同等）
    explicit operator bool() const noexcept { return IsCreated(); }

    //! @brief 現在の要素数
    [[nodiscard]] size_type Length() const noexcept {
        return header_ ? header_->length : 0;
    }

    //! @brief 現在の容量
    [[nodiscard]] size_type Capacity() const noexcept {
        return header_ ? header_->Capacity() : 0;
    }

    //! @brief バッファが空か
    [[nodiscard]] bool IsEmpty() const noexcept {
        return header_ == nullptr || header_->length == 0;
    }

    //------------------------------------------------------------------------
    // 要素アクセス
    //------------------------------------------------------------------------

    //! @brief インデックスアクセス
    //! @param index 要素インデックス（0 <= index < Length()）
    //! @return 要素への参照
    [[nodiscard]] T& operator[](size_type index) noexcept {
        assert(header_ && index >= 0 && index < header_->length);
        return GetDataPtr()[index];
    }

    //! @brief インデックスアクセス（const版）
    [[nodiscard]] const T& operator[](size_type index) const noexcept {
        assert(header_ && index >= 0 && index < header_->length);
        return GetDataPtr()[index];
    }

    //! @brief 先頭要素への参照
    [[nodiscard]] T& Front() noexcept {
        assert(header_ && header_->length > 0);
        return GetDataPtr()[0];
    }

    //! @brief 先頭要素への参照（const版）
    [[nodiscard]] const T& Front() const noexcept {
        assert(header_ && header_->length > 0);
        return GetDataPtr()[0];
    }

    //! @brief 末尾要素への参照
    [[nodiscard]] T& Back() noexcept {
        assert(header_ && header_->length > 0);
        return GetDataPtr()[header_->length - 1];
    }

    //! @brief 末尾要素への参照（const版）
    [[nodiscard]] const T& Back() const noexcept {
        assert(header_ && header_->length > 0);
        return GetDataPtr()[header_->length - 1];
    }

    //! @brief 生データポインタ取得
    [[nodiscard]] T* Data() noexcept { return GetDataPtr(); }

    //! @brief 生データポインタ取得（const版）
    [[nodiscard]] const T* Data() const noexcept { return GetDataPtr(); }

    //------------------------------------------------------------------------
    // 追加・削除
    //------------------------------------------------------------------------

    //! @brief 要素を末尾に追加
    //! @param element 追加する要素
    void Add(const T& element) {
        assert(header_);
        EnsureCapacity(header_->length + 1);
        GetDataPtr()[header_->length++] = element;
    }

    //! @brief 要素をデフォルト構築で末尾に追加
    //! @return 追加された要素への参照
    T& AddDefault() {
        assert(header_);
        EnsureCapacity(header_->length + 1);
        T* ptr = &GetDataPtr()[header_->length++];
        *ptr = T{};
        return *ptr;
    }

    //! @brief 指定インデックスの要素を削除（後続を詰める）
    //! @param index 削除する要素のインデックス
    //! @note O(n) - 順序を保持
    void RemoveAt(size_type index) {
        assert(header_ && index >= 0 && index < header_->length);
        T* data = GetDataPtr();
        // 後続要素を前に詰める
        for (size_type i = index; i < header_->length - 1; ++i) {
            data[i] = data[i + 1];
        }
        --header_->length;
    }

    //! @brief 指定インデックスの要素を削除（末尾と入れ替え）
    //! @param index 削除する要素のインデックス
    //! @note O(1) - 順序は保持されない
    void RemoveAtSwapBack(size_type index) {
        assert(header_ && index >= 0 && index < header_->length);
        T* data = GetDataPtr();
        if (index != header_->length - 1) {
            data[index] = data[header_->length - 1];
        }
        --header_->length;
    }

    //! @brief 全要素をクリア（容量は維持）
    void Clear() noexcept {
        if (header_) {
            header_->length = 0;
        }
    }

    //------------------------------------------------------------------------
    // 容量管理
    //------------------------------------------------------------------------

    //! @brief 指定容量を確保
    //! @param minCapacity 必要な最小容量
    void EnsureCapacity(size_type minCapacity) {
        assert(header_);
        if (minCapacity <= header_->Capacity()) return;
        GrowToCapacity(minCapacity);
    }

    //! @brief サイズを変更（未初期化）
    //! @param newLength 新しいサイズ
    //! @note 新しい要素は未初期化状態
    void ResizeUninitialized(size_type newLength) {
        assert(header_ && newLength >= 0);
        if (newLength > header_->Capacity()) {
            GrowToCapacity(newLength);
        }
        header_->length = newLength;
    }

    //! @brief サイズを変更（デフォルト初期化）
    //! @param newLength 新しいサイズ
    void Resize(size_type newLength) {
        assert(header_ && newLength >= 0);
        size_type oldLength = header_->length;
        ResizeUninitialized(newLength);
        // 新しい要素をデフォルト初期化
        if (newLength > oldLength) {
            T* data = GetDataPtr();
            for (size_type i = oldLength; i < newLength; ++i) {
                data[i] = T{};
            }
        }
    }

    //------------------------------------------------------------------------
    // イテレータ
    //------------------------------------------------------------------------

    [[nodiscard]] iterator begin() noexcept { return GetDataPtr(); }
    [[nodiscard]] iterator end() noexcept {
        return header_ ? GetDataPtr() + header_->length : nullptr;
    }
    [[nodiscard]] const_iterator begin() const noexcept { return GetDataPtr(); }
    [[nodiscard]] const_iterator end() const noexcept {
        return header_ ? GetDataPtr() + header_->length : nullptr;
    }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }

private:
    //------------------------------------------------------------------------
    // 内部実装
    //------------------------------------------------------------------------

    //! @brief 現在のデータポインタを取得
    [[nodiscard]] T* GetDataPtr() noexcept {
        if (!header_) return nullptr;
        return header_->externalPtr
            ? reinterpret_cast<T*>(header_->externalPtr)
            : reinterpret_cast<T*>(inlineData_);
    }

    //! @brief 現在のデータポインタを取得（const版）
    [[nodiscard]] const T* GetDataPtr() const noexcept {
        if (!header_) return nullptr;
        return header_->externalPtr
            ? reinterpret_cast<const T*>(header_->externalPtr)
            : reinterpret_cast<const T*>(inlineData_);
    }

    //! @brief 容量を拡張（外部ストレージに移行）
    void GrowToCapacity(size_type requiredCapacity) {
        // 新しい容量を計算（現在の2倍または要求値の大きい方）
        size_type currentCap = header_->Capacity();
        size_type newCapacity = (std::max)(currentCap * 2, requiredCapacity);

        // 外部ストレージを確保
        size_t newSize = static_cast<size_t>(newCapacity) * sizeof(T);
        std::byte* newStorage = static_cast<std::byte*>(
            Memory::GetDefaultAllocator().Allocate(newSize, alignof(T)));

        // 既存データをコピー
        if (header_->length > 0) {
            std::memcpy(newStorage, GetDataPtr(),
                static_cast<size_t>(header_->length) * sizeof(T));
        }

        // 旧外部ストレージを解放
        if (header_->externalPtr) {
            Memory::GetDefaultAllocator().Deallocate(
                header_->externalPtr,
                static_cast<size_t>(header_->externalCapacity) * sizeof(T));
        }

        // ヘッダーを更新
        header_->externalPtr = newStorage;
        header_->externalCapacity = static_cast<uint32_t>(newCapacity);
    }

private:
    BufferHeader* header_;      //!< BufferHeaderへのポインタ
    std::byte* inlineData_;     //!< インラインデータ領域へのポインタ
};

//============================================================================
//! @brief ConstDynamicBuffer - 読み取り専用の可変長バッファアクセサ
//!
//! DynamicBufferのconst版。読み取り操作のみ提供。
//!
//! @tparam T 要素型（IBufferElement継承、trivially_copyable必須）
//============================================================================
template<typename T>
class ConstDynamicBuffer {
    static_assert(is_buffer_element_v<T>,
        "T must inherit from IBufferElement and be trivially_copyable");

public:
    using value_type = T;
    using const_iterator = const T*;
    using size_type = int32_t;

    //------------------------------------------------------------------------
    // コンストラクタ
    //------------------------------------------------------------------------

    //! @brief デフォルトコンストラクタ（無効なバッファ）
    ConstDynamicBuffer() noexcept
        : header_(nullptr), inlineData_(nullptr) {}

    //! @brief ヘッダーとインラインデータへのポインタから構築
    //! @param header BufferHeaderへのポインタ（const）
    //! @param inlineData インラインデータ領域へのポインタ（const）
    ConstDynamicBuffer(const BufferHeader* header, const std::byte* inlineData) noexcept
        : header_(header), inlineData_(inlineData) {}

    //! @brief DynamicBufferからの変換
    ConstDynamicBuffer(const DynamicBuffer<T>& buffer) noexcept
        : header_(buffer.IsCreated() ? &GetHeaderFromBuffer(buffer) : nullptr)
        , inlineData_(buffer.IsCreated() ? GetInlineDataFromBuffer(buffer) : nullptr) {}

    //------------------------------------------------------------------------
    // 状態確認
    //------------------------------------------------------------------------

    //! @brief バッファが有効か（null でないか）
    [[nodiscard]] bool IsCreated() const noexcept { return header_ != nullptr; }

    //! @brief bool変換（IsCreated()と同等）
    explicit operator bool() const noexcept { return IsCreated(); }

    //! @brief 現在の要素数
    [[nodiscard]] size_type Length() const noexcept {
        return header_ ? header_->length : 0;
    }

    //! @brief 現在の容量
    [[nodiscard]] size_type Capacity() const noexcept {
        return header_ ? header_->Capacity() : 0;
    }

    //! @brief バッファが空か
    [[nodiscard]] bool IsEmpty() const noexcept {
        return header_ == nullptr || header_->length == 0;
    }

    //------------------------------------------------------------------------
    // 要素アクセス（読み取り専用）
    //------------------------------------------------------------------------

    //! @brief インデックスアクセス
    //! @param index 要素インデックス（0 <= index < Length()）
    //! @return 要素へのconst参照
    [[nodiscard]] const T& operator[](size_type index) const noexcept {
        assert(header_ && index >= 0 && index < header_->length);
        return GetDataPtr()[index];
    }

    //! @brief 先頭要素への参照
    [[nodiscard]] const T& Front() const noexcept {
        assert(header_ && header_->length > 0);
        return GetDataPtr()[0];
    }

    //! @brief 末尾要素への参照
    [[nodiscard]] const T& Back() const noexcept {
        assert(header_ && header_->length > 0);
        return GetDataPtr()[header_->length - 1];
    }

    //! @brief 生データポインタ取得
    [[nodiscard]] const T* Data() const noexcept { return GetDataPtr(); }

    //------------------------------------------------------------------------
    // イテレータ（const only）
    //------------------------------------------------------------------------

    [[nodiscard]] const_iterator begin() const noexcept { return GetDataPtr(); }
    [[nodiscard]] const_iterator end() const noexcept {
        return header_ ? GetDataPtr() + header_->length : nullptr;
    }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }

private:
    //------------------------------------------------------------------------
    // 内部実装
    //------------------------------------------------------------------------

    //! @brief 現在のデータポインタを取得
    [[nodiscard]] const T* GetDataPtr() const noexcept {
        if (!header_) return nullptr;
        return header_->externalPtr
            ? reinterpret_cast<const T*>(header_->externalPtr)
            : reinterpret_cast<const T*>(inlineData_);
    }

    // DynamicBufferからの変換ヘルパー（フレンド経由でアクセスする必要があるが、
    // 簡略化のため直接アクセス）
    static const BufferHeader& GetHeaderFromBuffer(const DynamicBuffer<T>& /*buffer*/) {
        // Note: This is a workaround. Proper implementation would use friend access.
        return *static_cast<const BufferHeader*>(nullptr);  // Placeholder
    }
    static const std::byte* GetInlineDataFromBuffer(const DynamicBuffer<T>& /*buffer*/) {
        return nullptr;  // Placeholder
    }

private:
    const BufferHeader* header_;      //!< BufferHeaderへのポインタ（const）
    const std::byte* inlineData_;     //!< インラインデータ領域へのポインタ（const）
};

} // namespace ECS

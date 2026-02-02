//----------------------------------------------------------------------------
//! @file   memory_utils.h
//! @brief  メモリアロケータ用ヘルパー関数
//----------------------------------------------------------------------------
#pragma once

#include "allocator.h"
#include <new>
#include <type_traits>
#include <utility>

namespace Memory {

//============================================================================
// 型安全な確保/解放
//============================================================================

//----------------------------------------------------------------------------
//! @brief 型安全なメモリ確保とコンストラクタ呼び出し
//! @tparam T 確保する型
//! @tparam Args コンストラクタ引数の型
//! @param alloc 使用するアロケータ
//! @param args コンストラクタ引数
//! @return 確保・構築されたオブジェクトへのポインタ
//----------------------------------------------------------------------------
template<typename T, typename... Args>
[[nodiscard]] T* AllocateNew(IAllocator& alloc, Args&&... args) {
    void* ptr = alloc.Allocate(sizeof(T), alignof(T));
    if (!ptr) {
        return nullptr;
    }
    return new (ptr) T(std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------
//! @brief 型安全なデストラクタ呼び出しとメモリ解放
//! @tparam T 解放する型
//! @param alloc 使用するアロケータ
//! @param ptr 解放するポインタ
//----------------------------------------------------------------------------
template<typename T>
void DeallocateDelete(IAllocator& alloc, T* ptr) {
    if (ptr) {
        ptr->~T();
        alloc.Deallocate(ptr, sizeof(T));
    }
}

//----------------------------------------------------------------------------
//! @brief 配列のメモリ確保とデフォルト構築
//! @tparam T 要素の型
//! @param alloc 使用するアロケータ
//! @param count 要素数
//! @return 確保・構築された配列へのポインタ
//----------------------------------------------------------------------------
template<typename T>
[[nodiscard]] T* AllocateArray(IAllocator& alloc, size_t count) {
    if (count == 0) {
        return nullptr;
    }

    void* ptr = alloc.Allocate(sizeof(T) * count, alignof(T));
    if (!ptr) {
        return nullptr;
    }

    T* arr = static_cast<T*>(ptr);

    // デフォルトコンストラクタを持つ型のみ
    if constexpr (std::is_default_constructible_v<T>) {
        for (size_t i = 0; i < count; ++i) {
            new (&arr[i]) T();
        }
    }

    return arr;
}

//----------------------------------------------------------------------------
//! @brief 配列のデストラクタ呼び出しとメモリ解放
//! @tparam T 要素の型
//! @param alloc 使用するアロケータ
//! @param ptr 解放するポインタ
//! @param count 要素数
//----------------------------------------------------------------------------
template<typename T>
void DeallocateArray(IAllocator& alloc, T* ptr, size_t count) {
    if (ptr && count > 0) {
        // 逆順でデストラクタ呼び出し
        for (size_t i = count; i > 0; --i) {
            ptr[i - 1].~T();
        }
        alloc.Deallocate(ptr, sizeof(T) * count);
    }
}

//============================================================================
// ユニークポインタ風ラッパー
//============================================================================

//----------------------------------------------------------------------------
//! @brief アロケータ対応デリータ
//! @tparam T 管理する型
//----------------------------------------------------------------------------
template<typename T>
class AllocatorDeleter {
public:
    AllocatorDeleter() noexcept : allocator_(nullptr) {}

    explicit AllocatorDeleter(IAllocator* alloc) noexcept : allocator_(alloc) {}

    void operator()(T* ptr) const {
        if (ptr && allocator_) {
            ptr->~T();
            allocator_->Deallocate(ptr, sizeof(T));
        }
    }

    [[nodiscard]] IAllocator* GetAllocator() const noexcept {
        return allocator_;
    }

private:
    IAllocator* allocator_;
};

//============================================================================
// メモリ操作ユーティリティ
//============================================================================

//----------------------------------------------------------------------------
//! @brief ポインタがアラインメントを満たしているか確認
//! @param ptr 確認するポインタ
//! @param alignment アラインメント要件
//! @return アラインメントを満たしている場合はtrue
//----------------------------------------------------------------------------
[[nodiscard]] inline bool IsAligned(const void* ptr, size_t alignment) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
}

//----------------------------------------------------------------------------
//! @brief サイズをアラインメントの倍数に切り上げ
//! @param size 元のサイズ
//! @param alignment アラインメント
//! @return 切り上げ後のサイズ
//----------------------------------------------------------------------------
[[nodiscard]] inline size_t AlignSize(size_t size, size_t alignment) noexcept {
    return (size + alignment - 1) & ~(alignment - 1);
}

} // namespace Memory

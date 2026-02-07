//----------------------------------------------------------------------------
//! @file   heap_allocator.h
//! @brief  ヒープアロケータ - 標準new/deleteのラッパー
//----------------------------------------------------------------------------
#pragma once


#include "allocator.h"
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <cassert>

#if defined(_WIN32)
#include <malloc.h>  // _aligned_malloc, _aligned_free
#endif

namespace Memory {

//============================================================================
//! @brief ヒープアロケータ
//!
//! 標準のnew/deleteをラップし、統計収集とスレッドセーフティを提供する。
//! デフォルトのフォールバックアロケータとして使用される。
//!
//! DEBUGビルドでは以下の追加機能を提供:
//! - ガードバイト: メモリ破壊の検出
//! - アロケーションヘッダー: サイズとマジックナンバーの記録
//!
//! @note スレッドセーフ（mutex保護）
//============================================================================
class HeapAllocator final : public IAllocator {
public:
    HeapAllocator() = default;
    ~HeapAllocator() override = default;

    // コピー禁止
    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;

    // ムーブ禁止（mutex保持のため）
    HeapAllocator(HeapAllocator&&) = delete;
    HeapAllocator& operator=(HeapAllocator&&) = delete;

    //------------------------------------------------------------------------
    //! @brief メモリ確保
    //! @param size 確保サイズ（バイト）
    //! @param alignment アラインメント要件
    //! @return 確保されたメモリへのポインタ
    //------------------------------------------------------------------------
    [[nodiscard]] void* Allocate(size_t size, size_t alignment) override {
        if (size == 0) {
            return nullptr;
        }

#if defined(_DEBUG)
        return AllocateWithGuards(size, alignment);
#else
        void* ptr = AllocateAligned(size, alignment);

        if (ptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.RecordAllocation(size);
        }

        return ptr;
#endif
    }

    //------------------------------------------------------------------------
    //! @brief メモリ解放
    //! @param ptr 解放するポインタ
    //! @param size 確保時のサイズ
    //------------------------------------------------------------------------
    void Deallocate(void* ptr, size_t size) override {
        if (!ptr) {
            return;
        }

#if defined(_DEBUG)
        DeallocateWithGuards(ptr, size);
#else
        DeallocateAligned(ptr);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.RecordDeallocation(size);
        }
#endif
    }

    //------------------------------------------------------------------------
    //! @brief アロケータ名取得
    //------------------------------------------------------------------------
    [[nodiscard]] const char* GetName() const noexcept override {
        return "HeapAllocator";
    }

    //------------------------------------------------------------------------
    //! @brief 統計情報取得
    //------------------------------------------------------------------------
    [[nodiscard]] AllocatorStats GetStats() const noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

private:
#if defined(_DEBUG)
    //========================================================================
    // デバッグ機能（DEBUGビルドのみ）
    //========================================================================

    //! ガードバイトサイズ
    static constexpr size_t kGuardSize = 16;

    //! ガードバイトのパターン（0xFD = "Freed"からの連想、MSVCと同様）
    static constexpr uint8_t kGuardPattern = 0xFD;

    //! アロケーションヘッダーのマジックナンバー
    static constexpr uint32_t kMagicNumber = 0xDEADBEEF;

    //! 解放済みマジックナンバー
    static constexpr uint32_t kFreedMagic = 0xFEEDFACE;

    //------------------------------------------------------------------------
    //! @brief アロケーションヘッダー
    //------------------------------------------------------------------------
    struct AllocationHeader {
        uint32_t magic;         //!< マジックナンバー
        uint32_t alignment;     //!< 元のアラインメント
        size_t size;            //!< ユーザー要求サイズ
        size_t totalSize;       //!< 実際の確保サイズ
    };

    //------------------------------------------------------------------------
    //! @brief ガードバイト付きメモリ確保
    //------------------------------------------------------------------------
    [[nodiscard]] void* AllocateWithGuards(size_t size, size_t alignment) {
        // レイアウト: [Header][Guard前][UserData][Guard後]
        // アラインメントを考慮した計算
        size_t headerSize = sizeof(AllocationHeader);
        size_t headerAndGuard = headerSize + kGuardSize;

        // ヘッダー+ガード後のアラインメントを確保
        size_t alignedHeaderSize = (headerAndGuard + alignment - 1) & ~(alignment - 1);
        size_t totalSize = alignedHeaderSize + size + kGuardSize;

        // 実際の確保（ヘッダーのアラインメントは最低限でOK）
        void* rawPtr = AllocateAligned(totalSize, alignof(AllocationHeader));
        if (!rawPtr) {
            return nullptr;
        }

        // ヘッダー設定
        auto* header = static_cast<AllocationHeader*>(rawPtr);
        header->magic = kMagicNumber;
        header->alignment = static_cast<uint32_t>(alignment);
        header->size = size;
        header->totalSize = totalSize;

        // ユーザーポインタ計算
        uint8_t* userPtr = static_cast<uint8_t*>(rawPtr) + alignedHeaderSize;

        // ガードバイト設定（前）
        uint8_t* guardBefore = userPtr - kGuardSize;
        std::memset(guardBefore, kGuardPattern, kGuardSize);

        // ガードバイト設定（後）
        uint8_t* guardAfter = userPtr + size;
        std::memset(guardAfter, kGuardPattern, kGuardSize);

        // 統計更新
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.RecordAllocation(size);
        }

        return userPtr;
    }

    //------------------------------------------------------------------------
    //! @brief ガードバイト付きメモリ解放
    //------------------------------------------------------------------------
    void DeallocateWithGuards(void* ptr, [[maybe_unused]] size_t size) {
        if (!ptr) return;

        uint8_t* userPtr = static_cast<uint8_t*>(ptr);

        // ガードバイト（前）を検索してヘッダーを見つける
        // ヘッダーはuserPtrの前にある
        uint8_t* guardBefore = userPtr - kGuardSize;

        // ヘッダーを探す（アラインメントを考慮）
        // ヘッダーはguardBeforeの前にある
        AllocationHeader* header = nullptr;

        // 最大探索範囲（最大アラインメント256を想定）
        for (size_t offset = sizeof(AllocationHeader); offset <= 256 + sizeof(AllocationHeader); offset += alignof(AllocationHeader)) {
            auto* candidate = reinterpret_cast<AllocationHeader*>(guardBefore - offset + kGuardSize);
            if (candidate->magic == kMagicNumber) {
                header = candidate;
                break;
            }
        }

        // ヘッダーが見つからない場合（不正なポインタまたは二重解放）
        if (!header) {
            assert(false && "HeapAllocator: Invalid pointer or double-free detected!");
            return;
        }

        // マジックナンバー検証（二重解放チェック）
        if (header->magic == kFreedMagic) {
            assert(false && "HeapAllocator: Double-free detected!");
            return;
        }

        size_t actualSize = header->size;

        // ガードバイト検証（前）
        bool guardBeforeOk = true;
        for (size_t i = 0; i < kGuardSize; ++i) {
            if (guardBefore[i] != kGuardPattern) {
                guardBeforeOk = false;
                break;
            }
        }
        assert(guardBeforeOk && "HeapAllocator: Memory corruption detected (underflow)!");

        // ガードバイト検証（後）
        uint8_t* guardAfter = userPtr + actualSize;
        bool guardAfterOk = true;
        for (size_t i = 0; i < kGuardSize; ++i) {
            if (guardAfter[i] != kGuardPattern) {
                guardAfterOk = false;
                break;
            }
        }
        assert(guardAfterOk && "HeapAllocator: Memory corruption detected (overflow)!");

        // 解放済みマークを設定
        header->magic = kFreedMagic;

        // 実際の解放
        DeallocateAligned(header);

        // 統計更新
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.RecordDeallocation(actualSize);
        }
    }
#endif  // _DEBUG

    //------------------------------------------------------------------------
    //! @brief アラインメント付きメモリ確保（プラットフォーム依存）
    //------------------------------------------------------------------------
    [[nodiscard]] static void* AllocateAligned(size_t size, size_t alignment) {
#if defined(_WIN32)
        return _aligned_malloc(size, alignment);
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return nullptr;
        }
        return ptr;
#endif
    }

    //------------------------------------------------------------------------
    //! @brief アラインメント付きメモリ解放（プラットフォーム依存）
    //------------------------------------------------------------------------
    static void DeallocateAligned(void* ptr) {
#if defined(_WIN32)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

private:
    mutable std::mutex mutex_;      //!< スレッドセーフティ用mutex
    AllocatorStats stats_;          //!< 統計情報
};

} // namespace Memory

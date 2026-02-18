/// @file MemoryBase.h
/// @brief メモリアロケータ基底クラスと定数
#pragma once

#include "HAL/AlignmentTemplates.h"
#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    // =========================================================================
    // アライメント定数
    // =========================================================================

    /// デフォルトアライメント（0 = サイズに応じて自動決定）
    constexpr uint32 kDefaultAlignment = 0;

    /// 最小アライメント（8バイト、64bitポインタ対応）
    constexpr uint32 kMinAlignment = 8;

    /// 最大サポートアライメント（128バイト、キャッシュライン×2）
    constexpr uint32 kMaxSupportedAlignment = 128;

    // =========================================================================
    // 列挙型
    // =========================================================================

    /// 割り当てヒント
    enum class AllocationHints
    {
        None = -1,     ///< ヒントなし
        Default = 0,   ///< デフォルト
        Temporary = 1, ///< 一時的（短命）
        SmallPool = 2  ///< 小さなプール向け
    };

    /// アロケータエラー状態
    enum class MallocError
    {
        None = 0,         ///< エラーなし
        OutOfMemory,      ///< メモリ不足
        InvalidAlignment, ///< 不正なアライメント（2の累乗でない）
        InvalidPointer,   ///< 不正なポインタ（Free/Reallocで）
        Corruption,       ///< ヒープ破損検出
        DoubleFree        ///< 二重解放検出
    };

    // =========================================================================
    // 統計情報
    // =========================================================================

    /// アロケータ統計情報
    struct AllocatorStats
    {
        SIZE_T totalAllocated;   ///< 総割り当てバイト数
        SIZE_T totalAllocations; ///< 総割り当て回数
        SIZE_T peakAllocated;    ///< ピーク割り当てバイト数
        SIZE_T currentUsed;      ///< 現在使用中バイト数
    };

    // =========================================================================
    // デバッグ検出機構
    // =========================================================================

    /// ガードバイトパターン（割り当て境界の前後に配置）
    constexpr uint8 kGuardByteFill = 0xFD;

    /// 解放済みメモリパターン（Free後に埋める）
    constexpr uint8 kFreedByteFill = 0xDD;

    /// 未初期化メモリパターン（Alloc直後に埋める）
    constexpr uint8 kUninitializedByteFill = 0xCD;

    /// ガードバイトサイズ（前後それぞれ）
    constexpr SIZE_T kGuardByteSize = 16;

/// デバッグ機能の有効化（ビルド設定で制御）
#ifndef NS_MALLOC_DEBUG
#if NS_BUILD_DEBUG
#define NS_MALLOC_DEBUG 1
#else
#define NS_MALLOC_DEBUG 0
#endif
#endif

    /// 割り当てヘッダ（デバッグビルド用）
    ///
    /// 各割り当ての先頭に配置し、メタデータを記録。
    /// DoubleFree/Corruption検出に使用。
    struct AllocationHeader
    {
        /// マジックナンバー（有効な割り当てを識別）
        static constexpr uint32 kMagicAllocated = 0xA110CA7E; // "ALLOCATE"風
        static constexpr uint32 kMagicFreed = 0xDEADBEEF;     // 解放済み

        uint32 magic;         ///< 状態識別マジック
        uint32 alignment;     ///< 要求アライメント
        SIZE_T requestedSize; ///< 要求サイズ
        SIZE_T actualSize;    ///< 実際のサイズ（ガード含む）
        void* originalPtr;    ///< アライメント調整前のポインタ

        /// 有効な割り当てかチェック
        [[nodiscard]] bool IsValidAllocation() const { return magic == kMagicAllocated; }

        /// 解放済みかチェック
        [[nodiscard]] bool IsFreed() const { return magic == kMagicFreed; }

        /// ガードバイト検証
        /// @param userPtr ユーザーに返したポインタ
        /// @return ガードが破損していなければtrue
        bool ValidateGuards(void* userPtr) const;

        /// 解放済みとしてマーク
        void MarkAsFreed() { magic = kMagicFreed; }
    };

    // =========================================================================
    // 前方宣言
    // =========================================================================

    class OutputDevice;

    // =========================================================================
    // Malloc基底クラス
    // =========================================================================

    /// メモリアロケータ基底クラス
    ///
    /// ## スレッドセーフティ
    ///
    /// `IsInternallyThreadSafe()` がtrueを返す場合、全メソッドはスレッドセーフ。
    /// falseの場合、呼び出し側で同期が必要。
    ///
    /// ## アライメント規約
    ///
    /// - alignment == 0: サイズに応じて自動決定（8〜16バイト）
    /// - alignment > 0: 2の累乗でなければならない
    /// - alignment > kMaxSupportedAlignment: 実装依存（失敗の可能性）
    class Malloc
    {
    public:
        Malloc() = default;
        virtual ~Malloc() = default;
        NS_DISALLOW_COPY_AND_MOVE(Malloc);

    public:
        // =====================================================================
        // 基本操作
        // =====================================================================

        /// メモリ割り当て（失敗時の動作は実装依存）
        ///
        /// @param count 割り当てバイト数（0の場合も有効なポインタを返す可能性あり）
        /// @param alignment アライメント（0=自動、それ以外は2の累乗）
        /// @return 割り当てたメモリへのポインタ、失敗時の動作は実装依存
        virtual void* Alloc(SIZE_T count, uint32 alignment = kDefaultAlignment) = 0;

        /// メモリ割り当て（失敗時はnullptr）
        ///
        /// @param count 割り当てバイト数
        /// @param alignment アライメント（0=自動、それ以外は2の累乗）
        /// @return 割り当てたメモリへのポインタ、失敗時nullptr
        virtual void* TryAlloc(SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// メモリ再割り当て（失敗時nullptr）
        ///
        /// @param ptr 既存のポインタ（nullptrの場合はTryAllocと同等）
        /// @param newCount 新しいサイズ（0の場合はFreeと同等）
        /// @param alignment アライメント
        /// @return 再割り当てしたメモリへのポインタ、失敗時nullptr
        virtual void* TryRealloc(void* ptr, SIZE_T newCount, uint32 alignment = kDefaultAlignment);

        /// メモリ再割り当て
        ///
        /// @param ptr 既存のポインタ（nullptrの場合はAllocと同等）
        /// @param newCount 新しいサイズ（0の場合はFreeと同等）
        /// @param alignment アライメント（元のアライメント以上を推奨）
        /// @return 再割り当てしたメモリへのポインタ
        virtual void* Realloc(void* ptr, SIZE_T newCount, uint32 alignment = kDefaultAlignment);

        /// メモリ解放
        ///
        /// @param ptr 解放するポインタ（nullptrは無視される）
        virtual void Free(void* ptr) = 0;

        /// ゼロ初期化付き割り当て
        virtual void* AllocZeroed(SIZE_T count, uint32 alignment = kDefaultAlignment);

        /// ゼロ初期化付き割り当て（失敗時nullptr）
        virtual void* TryAllocZeroed(SIZE_T count, uint32 alignment = kDefaultAlignment);

        // =====================================================================
        // サイズ情報
        // =====================================================================

        /// 割り当てサイズの量子化
        ///
        /// @param count 要求サイズ
        /// @param alignment アライメント
        /// @return 実際に割り当てられるサイズ（count以上）
        virtual SIZE_T QuantizeSize(SIZE_T count, uint32 alignment);

        /// 割り当てサイズ取得
        ///
        /// @param ptr 割り当て済みポインタ
        /// @param outSize 出力: 割り当てサイズ
        /// @return サイズ取得成功した場合true
        virtual bool GetAllocationSize(void* ptr, SIZE_T& outSize);

        // =====================================================================
        // 診断・デバッグ
        // =====================================================================

        /// ヒープ検証
        /// @return ヒープが正常ならtrue
        virtual bool ValidateHeap();

        /// メモリトリム（未使用メモリをOSに返却）
        virtual void Trim(bool trimThreadCaches);

        /// 最後のエラー取得
        [[nodiscard]] virtual MallocError GetLastError() const { return m_lastError; }

        /// エラーをクリア
        virtual void ClearError() { m_lastError = MallocError::None; }

        // =====================================================================
        // 統計情報
        // =====================================================================

        /// 統計情報を更新
        virtual void UpdateStats() {}

        /// アロケータ統計を取得
        /// @param outStats 出力先
        virtual void GetAllocatorStats(AllocatorStats& outStats);

        /// 統計をOutputDeviceに出力
        virtual void DumpAllocatorStats(OutputDevice& output);

        // =====================================================================
        // TLSキャッシュ管理
        // =====================================================================

        /// スレッドローカルキャッシュの初期化
        virtual void SetupTLSCachesOnCurrentThread() {}

        /// TLSキャッシュを使用中としてマーク
        virtual void MarkTLSCachesAsUsedOnCurrentThread() {}

        /// TLSキャッシュを未使用としてマーク
        virtual void MarkTLSCachesAsUnusedOnCurrentThread() {}

        /// TLSキャッシュをクリアして無効化
        virtual void ClearAndDisableTLSCachesOnCurrentThread() {}

        // =====================================================================
        // メタ情報
        // =====================================================================

        /// アロケータ名取得
        virtual const TCHAR* GetDescriptiveName() = 0;

        /// スレッドセーフかどうか
        [[nodiscard]] virtual bool IsInternallyThreadSafe() const { return false; }

        /// サポートする最大アライメント
        [[nodiscard]] virtual uint32 GetMaxSupportedAlignment() const { return kMaxSupportedAlignment; }

    protected:
        /// アライメント検証
        ///
        /// @param alignment 検証するアライメント
        /// @return 有効なアライメントならtrue
        bool ValidateAlignment(uint32 alignment)
        {
            if (alignment == 0)
            {
                return true;
            }
            if (!IsPowerOfTwo(alignment))
            {
                m_lastError = MallocError::InvalidAlignment;
                return false;
            }
            return true;
        }

        /// 実効アライメント計算
        ///
        /// @param count 割り当てサイズ
        /// @param alignment 要求アライメント（0=自動）
        /// @return 実効アライメント
        static uint32 GetEffectiveAlignment(SIZE_T count, uint32 alignment)
        {
            if (alignment != 0)
            {
                return alignment;
            }
            // 16バイト以上の割り当ては16バイトアライメント
            return (count >= 16) ? 16 : kMinAlignment;
        }

#if NS_MALLOC_DEBUG
        /// DoubleFree検出
        ///
        /// @param ptr 解放しようとしているポインタ
        /// @return DoubleFreeの場合true（エラー設定済み）
        bool DetectDoubleFree(void* ptr)
        {
            if (!ptr)
            {
                return false;
            }

            auto* header = GetAllocationHeader(ptr);
            if (header && header->IsFreed())
            {
                m_lastError = MallocError::DoubleFree;
                return true;
            }
            return false;
        }

        /// ヒープ破損検出
        ///
        /// @param ptr 検証するポインタ
        /// @return 破損している場合true（エラー設定済み）
        bool DetectCorruption(void* ptr)
        {
            if (!ptr)
            {
                return false;
            }

            auto* header = GetAllocationHeader(ptr);
            if (!header || !header->IsValidAllocation())
            {
                m_lastError = MallocError::InvalidPointer;
                return true;
            }

            if (!header->ValidateGuards(ptr))
            {
                m_lastError = MallocError::Corruption;
                return true;
            }
            return false;
        }

        /// 割り当てヘッダ取得
        ///
        /// @param userPtr ユーザーに返したポインタ
        /// @return ヘッダへのポインタ
        static AllocationHeader* GetAllocationHeader(void* userPtr)
        {
            if (!userPtr)
            {
                return nullptr;
            }
            return reinterpret_cast<AllocationHeader*>(static_cast<uint8*>(userPtr) - kGuardByteSize -
                                                       sizeof(AllocationHeader));
        }

        /// ガードバイト初期化
        ///
        /// @param header 割り当てヘッダ
        /// @param userPtr ユーザーに返すポインタ
        /// @param size ユーザー要求サイズ
        static void InitializeGuards(AllocationHeader* header, void* userPtr, SIZE_T size);

        /// 解放時のメモリポイズン
        ///
        /// @param userPtr ユーザーポインタ
        /// @param size サイズ
        static void PoisonFreedMemory(void* userPtr, SIZE_T size);
#endif

        MallocError m_lastError = MallocError::None;
    };

    // =========================================================================
    // グローバルアロケータ
    // =========================================================================

    /// グローバルアロケータ（起動時に設定）
    extern Malloc* g_gMalloc;

} // namespace NS

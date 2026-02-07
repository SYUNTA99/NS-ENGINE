/// @file RHIBufferAllocator.h
/// @brief バッファメモリアロケーター
/// @details リニア、リング、プール方式のバッファ割り当て機構を提供。
/// @see 11-02-buffer-allocator.md
#pragma once

#include "IRHIBuffer.h"
#include "RHIMacros.h"

namespace NS::RHI
{
    class IRHIDevice;

    //=========================================================================
    // RHIBufferAllocation (11-02)
    //=========================================================================

    /// バッファアロケーション
    /// アロケーターから割り当てられたバッファ領域
    struct RHI_API RHIBufferAllocation
    {
        IRHIBuffer* buffer = nullptr; ///< バッファ
        uint64 offset = 0;            ///< バッファ内オフセット
        uint64 size = 0;              ///< サイズ
        uint64 gpuAddress = 0;        ///< GPUアドレス
        void* cpuAddress = nullptr;   ///< CPUポインタ（Mapped時のみ有効）

        /// 有効か
        bool IsValid() const { return buffer != nullptr && size > 0; }

        /// GPUアドレス取得
        uint64 GetGPUAddress() const { return gpuAddress; }

        /// データ書き込み
        template <typename T> void Write(const T& data)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            NS_ASSERT(cpuAddress != nullptr);
            NS_ASSERT(sizeof(T) <= size);
            memcpy(cpuAddress, &data, sizeof(T));
        }

        /// 配列書き込み
        template <typename T> void WriteArray(const T* data, uint32 count)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            NS_ASSERT(cpuAddress != nullptr);
            NS_ASSERT(sizeof(T) * count <= size);
            memcpy(cpuAddress, data, sizeof(T) * count);
        }
    };

    //=========================================================================
    // RHILinearBufferAllocator (11-02)
    //=========================================================================

    /// リニアバッファアロケーター
    /// フレームごとにリセットされるリニア割り当て
    class RHI_API RHILinearBufferAllocator
    {
    public:
        RHILinearBufferAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint64 size, ERHIHeapType heapType = ERHIHeapType::Upload);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // 割り当て
        //=====================================================================

        /// メモリ割り当て
        RHIBufferAllocation Allocate(uint64 size, uint64 alignment = 0);

        /// 型付き割り当て
        template <typename T> RHIBufferAllocation Allocate(uint32 count = 1, uint64 alignment = 0)
        {
            return Allocate(sizeof(T) * count, alignment > 0 ? alignment : alignof(T));
        }

        //=====================================================================
        // リセット
        //=====================================================================

        /// リセット（次フレーム用）
        void Reset();

        //=====================================================================
        // 情報
        //=====================================================================

        uint64 GetUsedSize() const { return m_currentOffset; }
        uint64 GetRemainingSize() const { return m_totalSize - m_currentOffset; }
        uint64 GetTotalSize() const { return m_totalSize; }
        IRHIBuffer* GetBuffer() const { return m_buffer; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIBufferRef m_buffer;
        uint64 m_totalSize = 0;
        uint64 m_currentOffset = 0;
        void* m_mappedPtr = nullptr;
    };

    //=========================================================================
    // RHIRingBufferAllocator (11-02)
    //=========================================================================

    /// リングバッファアロケーター
    /// 複数フレームで使用するリング方式割り当て
    class RHI_API RHIRingBufferAllocator
    {
    public:
        RHIRingBufferAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device,
                        uint64 size,
                        uint32 numFrames,
                        ERHIHeapType heapType = ERHIHeapType::Upload);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始
        /// @param frameIndex 現在フレームインデックス
        /// @param completedFrame GPU側で完了済みのフェンス値
        void BeginFrame(uint32 frameIndex, uint64 completedFrame);

        /// フレーム終了
        void EndFrame(uint64 frameNumber);

        //=====================================================================
        // 割り当て
        //=====================================================================

        /// メモリ割り当て
        RHIBufferAllocation Allocate(uint64 size, uint64 alignment = 0);

        /// 型付き割り当て
        template <typename T> RHIBufferAllocation Allocate(uint32 count = 1)
        {
            return Allocate(sizeof(T) * count, alignof(T));
        }

        //=====================================================================
        // 情報
        //=====================================================================

        uint64 GetTotalSize() const { return m_totalSize; }
        uint64 GetUsedSize() const;
        IRHIBuffer* GetBuffer() const { return m_buffer; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIBufferRef m_buffer;

        uint64 m_totalSize = 0;
        uint64 m_head = 0;
        uint64 m_tail = 0;

        struct FrameAllocation
        {
            uint64 frameNumber;
            uint64 offset;
        };
        FrameAllocation* m_frameAllocations = nullptr;
        uint32 m_numFrames = 0;
        uint32 m_currentFrame = 0;

        void* m_mappedPtr = nullptr;
    };

    //=========================================================================
    // RHIBufferPoolConfig / RHIBufferPool (11-02)
    //=========================================================================

    /// バッファプール設定
    struct RHI_API RHIBufferPoolConfig
    {
        uint64 blockSize = 0;                          ///< ブロックサイズ
        uint32 initialBlockCount = 16;                 ///< 初期ブロック数
        uint32 maxBlockCount = 0;                      ///< 最大ブロック数（0で無制限）
        ERHIHeapType heapType = ERHIHeapType::Default; ///< ヒープタイプ
        ERHIBufferUsage usage = ERHIBufferUsage::None; ///< バッファ使用フラグ
    };

    /// プールバッファアロケーター
    /// 固定サイズバッファのプール管理
    class RHI_API RHIBufferPool
    {
    public:
        RHIBufferPool() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, const RHIBufferPoolConfig& config);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // 割り当て/解放
        //=====================================================================

        IRHIBuffer* Acquire();
        void Release(IRHIBuffer* buffer);

        //=====================================================================
        // 情報
        //=====================================================================

        uint64 GetBlockSize() const { return m_config.blockSize; }
        uint32 GetAvailableCount() const { return m_freeCount; }
        uint32 GetTotalCount() const { return m_totalCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIBufferPoolConfig m_config;

        IRHIBuffer** m_freeList = nullptr;
        uint32 m_freeCount = 0;
        uint32 m_freeCapacity = 0;
        uint32 m_totalCount = 0;
    };

    //=========================================================================
    // RHIMultiSizeBufferPool (11-02)
    //=========================================================================

    /// マルチサイズバッファプール
    /// 複数サイズのプールを統合管理
    /// @note スレッドセーフではない。外部同期が必要
    class RHI_API RHIMultiSizeBufferPool
    {
    public:
        RHIMultiSizeBufferPool() = default;

        /// 初期化
        /// @param sizes プールするサイズ配列
        /// @param count サイズ配列の要素数
        bool Initialize(IRHIDevice* device,
                        const uint64* sizes,
                        uint32 count,
                        ERHIHeapType heapType = ERHIHeapType::Default);

        /// シャットダウン
        void Shutdown();

        /// バッファ取得（要求サイズ以上の最小プールから）
        IRHIBuffer* Acquire(uint64 minSize);

        /// バッファ返却
        void Release(IRHIBuffer* buffer);

    private:
        IRHIDevice* m_device = nullptr;
        RHIBufferPool* m_pools = nullptr;
        uint32 m_poolCount = 0;
    };

    //=========================================================================
    // RHIConstantBufferAllocator (11-02)
    //=========================================================================

    /// 定数バッファアロケーター
    /// CBVのための256バイトアライメント割り当て
    class RHI_API RHIConstantBufferAllocator
    {
    public:
        static constexpr uint64 kCBVAlignment = 256;

        RHIConstantBufferAllocator() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint64 size);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(uint32 frameIndex);
        void EndFrame();

        //=====================================================================
        // 割り当て
        //=====================================================================

        /// 定数バッファ割り当て（256バイトアライン）
        RHIBufferAllocation Allocate(uint64 size);

        /// 型付き割り当て
        template <typename T> RHIBufferAllocation Allocate() { return Allocate(sizeof(T)); }

        /// 即座にデータ書き込み
        template <typename T> RHIBufferAllocation AllocateAndWrite(const T& data)
        {
            auto alloc = Allocate<T>();
            if (alloc.IsValid())
            {
                alloc.Write(data);
            }
            return alloc;
        }

        //=====================================================================
        // 情報
        //=====================================================================

        IRHIBuffer* GetBuffer() const { return m_ringBuffer.GetBuffer(); }

    private:
        RHIRingBufferAllocator m_ringBuffer;
    };

    //=========================================================================
    // RHIDynamicBufferManager (11-02)
    //=========================================================================

    /// ダイナミックバッファマネージャー
    /// フレームごとに変化するバッファデータの統合管理
    class RHI_API RHIDynamicBufferManager
    {
    public:
        RHIDynamicBufferManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device,
                        uint64 uploadBufferSize = 64 * 1024 * 1024,
                        uint64 constantBufferSize = 16 * 1024 * 1024);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(uint32 frameIndex, uint64 completedFrame);
        void EndFrame(uint64 frameNumber);

        //=====================================================================
        // 割り当て
        //=====================================================================

        /// 汎用アップロードバッファ割り当て
        RHIBufferAllocation AllocateUpload(uint64 size, uint64 alignment = 0)
        {
            return m_uploadAllocator.Allocate(size, alignment);
        }

        /// 定数バッファ割り当て
        RHIBufferAllocation AllocateConstant(uint64 size) { return m_constantAllocator.Allocate(size); }

        /// 型付き定数バッファ割り当て
        template <typename T> RHIBufferAllocation AllocateConstant(const T& data)
        {
            return m_constantAllocator.AllocateAndWrite(data);
        }

        /// 頂点バッファ用割り当て
        template <typename T> RHIBufferAllocation AllocateVertices(const T* vertices, uint32 count)
        {
            auto alloc = m_uploadAllocator.Allocate(sizeof(T) * count, alignof(T));
            if (alloc.IsValid())
            {
                alloc.WriteArray(vertices, count);
            }
            return alloc;
        }

        /// インデックスバッファ用割り当て（uint16）
        RHIBufferAllocation AllocateIndices(const uint16* indices, uint32 count)
        {
            auto alloc = m_uploadAllocator.Allocate(sizeof(uint16) * count, sizeof(uint16));
            if (alloc.IsValid())
            {
                alloc.WriteArray(indices, count);
            }
            return alloc;
        }

        /// インデックスバッファ用割り当て（uint32）
        RHIBufferAllocation AllocateIndices(const uint32* indices, uint32 count)
        {
            auto alloc = m_uploadAllocator.Allocate(sizeof(uint32) * count, sizeof(uint32));
            if (alloc.IsValid())
            {
                alloc.WriteArray(indices, count);
            }
            return alloc;
        }

    private:
        RHIRingBufferAllocator m_uploadAllocator;
        RHIConstantBufferAllocator m_constantAllocator;
    };

} // namespace NS::RHI

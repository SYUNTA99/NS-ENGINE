/// @file RHITransientAllocator.h
/// @brief Transientリソースアロケーター
/// @details フレーム内一時リソースの効率的な割り当て・メモリエイリアシングを提供。
/// @see 23-01-transient-allocator.md
#pragma once

#include "IRHIBuffer.h"
#include "IRHITexture.h"
#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    // 前方宣言
    class IRHICommandContext;
    class IRHIFence;

    //=========================================================================
    // RHITransientResourceLifetime (23-01)
    //=========================================================================

    /// Transientリソースの使用範囲
    struct RHITransientResourceLifetime
    {
        uint32 firstPassIndex = 0; ///< 最初に使用するパスのインデックス
        uint32 lastPassIndex = 0;  ///< 最後に使用するパスのインデックス

        bool Overlaps(const RHITransientResourceLifetime& other) const
        {
            return !(lastPassIndex < other.firstPassIndex || other.lastPassIndex < firstPassIndex);
        }
    };

    //=========================================================================
    // Transientリソース記述 (23-01)
    //=========================================================================

    /// Transientバッファ記述
    struct RHITransientBufferDesc
    {
        uint64 size = 0;
        ERHIBufferUsage usage = ERHIBufferUsage::Default;
        RHITransientResourceLifetime lifetime;
        const char* debugName = nullptr;
    };

    /// Transientテクスチャ記述
    struct RHITransientTextureDesc
    {
        uint32 width = 1;
        uint32 height = 1;
        uint32 depth = 1;
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;
        ERHITextureUsage usage = ERHITextureUsage::Default;
        uint32 mipLevels = 1;
        uint32 sampleCount = 1;
        ERHITextureDimension dimension = ERHITextureDimension::Texture2D;
        RHITransientResourceLifetime lifetime;
        const char* debugName = nullptr;

        /// 必要なメモリサイズ概算
        uint64 EstimateMemorySize() const;
    };

    //=========================================================================
    // Transientリソースハンドル (23-01)
    //=========================================================================

    /// Transientバッファハンドル
    /// 実際のIRHIBufferへのアクセスはAcquire後のみ有効
    class RHITransientBuffer
    {
    public:
        RHITransientBuffer() = default;

        bool IsValid() const { return m_allocator != nullptr; }

        /// 実際のバッファを取得（使用範囲内でのみ呼び出し可）
        IRHIBuffer* GetBuffer() const;

        /// バッファサイズ
        uint64 GetSize() const { return m_desc.size; }

    private:
        friend class IRHITransientResourceAllocator;

        IRHITransientResourceAllocator* m_allocator = nullptr;
        uint32 m_handle = 0;
        RHITransientBufferDesc m_desc;
    };

    /// Transientテクスチャハンドル
    class RHITransientTexture
    {
    public:
        RHITransientTexture() = default;

        bool IsValid() const { return m_allocator != nullptr; }

        /// 実際のテクスチャを取得（使用範囲内でのみ呼び出し可）
        IRHITexture* GetTexture() const;

        /// テクスチャ寸法
        uint32 GetWidth() const { return m_desc.width; }
        uint32 GetHeight() const { return m_desc.height; }
        ERHIPixelFormat GetFormat() const { return m_desc.format; }

    private:
        friend class IRHITransientResourceAllocator;

        IRHITransientResourceAllocator* m_allocator = nullptr;
        uint32 m_handle = 0;
        RHITransientTextureDesc m_desc;
    };

    //=========================================================================
    // RHITransientAllocatorStats (23-01)
    //=========================================================================

    /// Transientリソースアロケーター統計
    struct RHITransientAllocatorStats
    {
        uint64 totalHeapSize = 0;      ///< ヒープ総サイズ
        uint64 peakUsedMemory = 0;     ///< ピーク使用メモリ
        uint64 currentUsedMemory = 0;  ///< 現在の使用メモリ
        uint64 aliasedMemorySaved = 0; ///< エイリアシングで節約されたメモリ
        uint32 allocatedBuffers = 0;   ///< 割り当て済みバッファ数
        uint32 allocatedTextures = 0;  ///< 割り当て済みテクスチャ数
        uint32 reusedResources = 0;    ///< 再利用されたリソース数
    };

    //=========================================================================
    // ERHIAsyncComputeBudget (23-01)
    //=========================================================================

    /// AsyncComputeバジェット
    enum class ERHIAsyncComputeBudget : uint8
    {
        None = 0,          ///< AsyncCompute無効
        Quarter = 1,       ///< 1/4リソース
        Half = 2,          ///< 半分
        ThreeQuarters = 3, ///< 3/4リソース
        All = 4,           ///< 全リソース使用可能
    };

    //=========================================================================
    // RHITransientAllocationFences (23-01)
    //=========================================================================

    /// Transientリソース同期フェンス
    /// Graphics/AsyncComputeパイプライン間のエイリアシング同期
    struct RHITransientAllocationFences
    {
        IRHIFence* graphicsFence = nullptr;
        uint64 graphicsFenceValue = 0;

        IRHIFence* asyncComputeFence = nullptr;
        uint64 asyncComputeFenceValue = 0;

        IRHIFence* graphicsForkJoinFence = nullptr;
        uint64 graphicsForkJoinFenceValue = 0;
    };

    //=========================================================================
    // IRHITransientResourceAllocator (23-01)
    //=========================================================================

    /// Transientリソースアロケーター
    /// フレーム内の使い捨てリソースを効率的に管理
    class RHI_API IRHITransientResourceAllocator
    {
        friend class RHITransientBuffer;
        friend class RHITransientTexture;

    public:
        virtual ~IRHITransientResourceAllocator() = default;

        //=====================================================================
        // フレーム管理
        //=====================================================================

        /// フレーム開始（リソースリセット）
        virtual void BeginFrame() = 0;

        /// フレーム終了
        virtual void EndFrame() = 0;

        //=====================================================================
        // リソース割り当て
        //=====================================================================

        /// Transientバッファ割り当て
        virtual RHITransientBuffer AllocateBuffer(const RHITransientBufferDesc& desc) = 0;

        /// Transientテクスチャ割り当て
        virtual RHITransientTexture AllocateTexture(const RHITransientTextureDesc& desc) = 0;

        //=====================================================================
        // リソースアクセス
        //=====================================================================

        /// パス開始時にリソースをアクティブ化
        /// エイリアシングバリアが必要な場合に挿入
        virtual void AcquireResources(IRHICommandContext* context, uint32 passIndex) = 0;

        /// パス終了時にリソースを解放
        virtual void ReleaseResources(IRHICommandContext* context, uint32 passIndex) = 0;

        //=====================================================================
        // マルチパイプライン同期
        //=====================================================================

        /// パイプライン対応のリソースアクティブ化
        virtual void AcquireResourcesForPipeline(IRHICommandContext* context,
                                                 uint32 passIndex,
                                                 ERHIPipeline pipeline) = 0;

        /// パイプライン対応のリソース解放
        virtual void ReleaseResourcesForPipeline(IRHICommandContext* context,
                                                 uint32 passIndex,
                                                 ERHIPipeline pipeline) = 0;

        /// フェンス設定
        virtual void SetAllocationFences(const RHITransientAllocationFences& fences) = 0;

        /// AsyncComputeバジェット設定
        virtual void SetAsyncComputeBudget(ERHIAsyncComputeBudget budget) = 0;

        //=====================================================================
        // 統計
        //=====================================================================

        /// 統計情報取得
        virtual RHITransientAllocatorStats GetStats() const = 0;

        /// メモリ使用量のヒストグラムをログ出力
        virtual void DumpMemoryUsage() const = 0;

    protected:
        /// ハンドルからバッファ取得（内部用）
        virtual IRHIBuffer* GetBufferInternal(uint32 handle) const = 0;

        /// ハンドルからテクスチャ取得（内部用）
        virtual IRHITexture* GetTextureInternal(uint32 handle) const = 0;

        /// ハンドル構築ヘルパー（派生クラスから使用）
        /// @note friendは継承されないため、基底クラスで設定を代行する
        static void SetupBufferHandle(RHITransientBuffer& buf, IRHITransientResourceAllocator* alloc,
                                      uint32 handle, const RHITransientBufferDesc& desc)
        {
            buf.m_allocator = alloc;
            buf.m_handle = handle;
            buf.m_desc = desc;
        }

        static void SetupTextureHandle(RHITransientTexture& tex, IRHITransientResourceAllocator* alloc,
                                       uint32 handle, const RHITransientTextureDesc& desc)
        {
            tex.m_allocator = alloc;
            tex.m_handle = handle;
            tex.m_desc = desc;
        }
    };

    //=========================================================================
    // RHITransientAllocatorDesc (23-01)
    //=========================================================================

    /// Transientアロケーター作成記述
    struct RHITransientAllocatorDesc
    {
        uint64 initialHeapSize = 256 * 1024 * 1024; ///< 初期ヒープサイズ（256MB）
        uint64 maxHeapSize = 1024 * 1024 * 1024;    ///< 最大ヒープサイズ（1GB）
        bool allowGrowth = true;                    ///< ヒープの成長を許可
        const char* debugName = "TransientHeap";
    };

}} // namespace NS::RHI

/// @file RHIOfflineDescriptor.h
/// @brief オフラインディスクリプタ管理
/// @details CPU専用のオフラインディスクリプタヒープ、タイプ別マネージャー、ビューキャッシュを提供。
/// @see 10-03-offline-descriptor.md
#pragma once

#include "RHIDescriptorHeap.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIOfflineDescriptorHeap (10-03)
    //=========================================================================

    /// オフラインディスクリプタヒープ
    /// CPU専用、ビュー作成のステージング領域
    /// @threadsafety スレッドセーフではない。外部同期が必要。
    class RHI_API RHIOfflineDescriptorHeap
    {
    public:
        RHIOfflineDescriptorHeap() = default;

        /// 初期化
        /// @param device デバイス
        /// @param type ヒープタイプ
        /// @param numDescriptors ディスクリプタ数
        bool Initialize(IRHIDevice* device, ERHIDescriptorHeapType type, uint32 numDescriptors);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // ディスクリプタ割り当て
        //=====================================================================

        /// ディスクリプタ割り当て
        RHIDescriptorAllocation Allocate(uint32 count = 1);

        /// ディスクリプタ解放
        void Free(const RHIDescriptorAllocation& allocation);

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const { return m_allocator.GetAvailableCount(); }

        /// 総ディスクリプタ数
        uint32 GetTotalCount() const { return m_allocator.GetTotalCount(); }

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// ヒープタイプ取得
        ERHIDescriptorHeapType GetType() const { return m_type; }

        /// シェーダー可視か（オフラインヒープは常にfalse）
        bool IsShaderVisible() const { return false; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        RHIDescriptorHeapAllocator m_allocator;
        ERHIDescriptorHeapType m_type = ERHIDescriptorHeapType::CBV_SRV_UAV;
    };

    //=========================================================================
    // RHIOfflineDescriptorManager (10-03)
    //=========================================================================

    /// オフラインディスクリプタマネージャー
    /// 各タイプ別のオフラインヒープを管理
    /// @threadsafety Allocate/Freeはmutex保護を推奨。マルチスレッドのビュー作成時に競合する。
    class RHI_API RHIOfflineDescriptorManager
    {
    public:
        RHIOfflineDescriptorManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device,
                        uint32 cbvSrvUavCount = 10000,
                        uint32 samplerCount = 256,
                        uint32 rtvCount = 256,
                        uint32 dsvCount = 64);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // ディスクリプタ割り当て
        //=====================================================================

        /// CBV/SRV/UAVディスクリプタ割り当て
        RHIDescriptorAllocation AllocateCBVSRVUAV(uint32 count = 1) { return m_cbvSrvUavHeap.Allocate(count); }

        /// サンプラーディスクリプタ割り当て
        RHIDescriptorAllocation AllocateSampler(uint32 count = 1) { return m_samplerHeap.Allocate(count); }

        /// RTVディスクリプタ割り当て
        RHIDescriptorAllocation AllocateRTV(uint32 count = 1) { return m_rtvHeap.Allocate(count); }

        /// DSVディスクリプタ割り当て
        RHIDescriptorAllocation AllocateDSV(uint32 count = 1) { return m_dsvHeap.Allocate(count); }

        //=====================================================================
        // ディスクリプタ解放
        //=====================================================================

        void FreeCBVSRVUAV(const RHIDescriptorAllocation& allocation) { m_cbvSrvUavHeap.Free(allocation); }
        void FreeSampler(const RHIDescriptorAllocation& allocation) { m_samplerHeap.Free(allocation); }
        void FreeRTV(const RHIDescriptorAllocation& allocation) { m_rtvHeap.Free(allocation); }
        void FreeDSV(const RHIDescriptorAllocation& allocation) { m_dsvHeap.Free(allocation); }

        //=====================================================================
        // ヒープ取得
        //=====================================================================

        IRHIDescriptorHeap* GetCBVSRVUAVHeap() const { return m_cbvSrvUavHeap.GetHeap(); }
        IRHIDescriptorHeap* GetSamplerHeap() const { return m_samplerHeap.GetHeap(); }
        IRHIDescriptorHeap* GetRTVHeap() const { return m_rtvHeap.GetHeap(); }
        IRHIDescriptorHeap* GetDSVHeap() const { return m_dsvHeap.GetHeap(); }

    private:
        RHIOfflineDescriptorHeap m_cbvSrvUavHeap;
        RHIOfflineDescriptorHeap m_samplerHeap;
        RHIOfflineDescriptorHeap m_rtvHeap;
        RHIOfflineDescriptorHeap m_dsvHeap;
    };

    //=========================================================================
    // RHIViewCacheKey (10-03)
    //=========================================================================

    /// ビューキャッシュキー
    struct RHI_API RHIViewCacheKey
    {
        /// リソースポインタ
        IRHIResource* resource = nullptr;

        /// ビュー記述のハッシュ
        uint64 descHash = 0;

        bool operator==(const RHIViewCacheKey& other) const
        {
            return resource == other.resource && descHash == other.descHash;
        }

        bool operator!=(const RHIViewCacheKey& other) const { return !(*this == other); }
    };

    //=========================================================================
    // RHIViewCache (10-03)
    //=========================================================================

    /// ビューキャッシュ
    /// 同一リソース・同一記述のビューをキャッシュ
    /// @threadsafety スレッドセーフではない。外部同期が必要。
    template <typename ViewType, typename DescType> class RHIViewCache
    {
    public:
        RHIViewCache() = default;

        /// 初期化
        void Initialize(IRHIDevice* device, uint32 maxCacheSize = 1024);

        /// シャットダウン
        void Shutdown();

        /// ビュー取得（キャッシュヒット時は既存を返す）
        ViewType* GetOrCreate(IRHIResource* resource, const DescType& desc);

        /// キャッシュクリア
        void Clear();

        /// リソースに関連するビューを無効化
        void InvalidateResource(IRHIResource* resource);

        /// 統計
        uint32 GetCacheHits() const { return m_cacheHits; }
        uint32 GetCacheMisses() const { return m_cacheMisses; }

    private:
        IRHIDevice* m_device = nullptr;
        uint32 m_cacheHits = 0;
        uint32 m_cacheMisses = 0;
    };

    /// SRVキャッシュ
    using RHISRVCache = RHIViewCache<IRHIShaderResourceView, RHITextureSRVDesc>;

    /// UAVキャッシュ
    using RHIUAVCache = RHIViewCache<IRHIUnorderedAccessView, RHITextureUAVDesc>;

}} // namespace NS::RHI

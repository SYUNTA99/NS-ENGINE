# 10-03: オフラインディスクリプタ

## 目的

CPU専用のオフラインディスクリプタヒープ管理を定義する。

## 参照ドキュメント

- 10-01-descriptor-heap.md (ディスクリプタヒープ基本)
- 10-02-online-descriptor.md (オンラインディスクリプタ)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIOfflineDescriptor.h`

## TODO

### 1. オフラインディスクリプタヒープ

```cpp
#pragma once

#include "RHIDescriptorHeap.h"

namespace NS::RHI
{
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
        bool Initialize(
            IRHIDevice* device,
            ERHIDescriptorHeapType type,
            uint32 numDescriptors);

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

        /// 総デスクリプタ数
        uint32 GetTotalCount() const { return m_allocator.GetTotalCount(); }

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// ヒープタイプ取得
        ERHIDescriptorHeapType GetType() const { return m_type; }

        /// シェーダー可視か
        bool IsShaderVisible() const { return false; }  // オフラインヒープの非シェーダー可視

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        RHIDescriptorHeapAllocator m_allocator;
        ERHIDescriptorHeapType m_type = ERHIDescriptorHeapType::CBV_SRV_UAV;
    };
}
```

- [ ] RHIOfflineDescriptorHeap クラス

### 2. ビュータイプ別オフラインヒープ

```cpp
namespace NS::RHI
{
    /// オフラインディスクリプタマネージャー
    /// 各タイプ別のオフラインヒープを管理
    /// @threadsafety Allocate/Freeはmutex保護を推奨。マルチスレッドのビュー作成時に競合する。
    class RHI_API RHIOfflineDescriptorManager
    {
    public:
        RHIOfflineDescriptorManager() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
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
        RHIDescriptorAllocation AllocateCBVSRVUAV(uint32 count = 1) {
            return m_cbvSrvUavHeap.Allocate(count);
        }

        /// サンプラーディスクリプタ割り当て
        RHIDescriptorAllocation AllocateSampler(uint32 count = 1) {
            return m_samplerHeap.Allocate(count);
        }

        /// RTVディスクリプタ割り当て
        RHIDescriptorAllocation AllocateRTV(uint32 count = 1) {
            return m_rtvHeap.Allocate(count);
        }

        /// DSVディスクリプタ割り当て
        RHIDescriptorAllocation AllocateDSV(uint32 count = 1) {
            return m_dsvHeap.Allocate(count);
        }

        //=====================================================================
        // ディスクリプタ解放
        //=====================================================================

        void FreeCBVSRVUAV(const RHIDescriptorAllocation& allocation) {
            m_cbvSrvUavHeap.Free(allocation);
        }

        void FreeSampler(const RHIDescriptorAllocation& allocation) {
            m_samplerHeap.Free(allocation);
        }

        void FreeRTV(const RHIDescriptorAllocation& allocation) {
            m_rtvHeap.Free(allocation);
        }

        void FreeDSV(const RHIDescriptorAllocation& allocation) {
            m_dsvHeap.Free(allocation);
        }

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
}
```

- [ ] RHIOfflineDescriptorManager クラス
- [ ] 各タイプ別ヒープ管理

### 3. ビューとディスクリプタの関連付け

```cpp
namespace NS::RHI
{
    /// ディスクリプタ付きビュー基底
    /// オフラインディスクリプタを所有するビュー
    class RHI_API IRHIViewWithDescriptor
    {
    public:
        virtual ~IRHIViewWithDescriptor() = default;

        /// CPUディスクリプタハンドル取得
        virtual RHICPUDescriptorHandle GetCPUDescriptorHandle() const = 0;

        /// ディスクリプタヒープタイプ取得
        virtual ERHIDescriptorHeapType GetDescriptorHeapType() const = 0;
    };

    /// SRVにディスクリプタ情報追加
    class RHI_API IRHIShaderResourceView : public IRHIViewWithDescriptor
    {
    public:
        RHICPUDescriptorHandle GetCPUDescriptorHandle() const override;
        ERHIDescriptorHeapType GetDescriptorHeapType() const override {
            return ERHIDescriptorHeapType::CBV_SRV_UAV;
        }
    };

    /// UAVにディスクリプタ情報追加
    class RHI_API IRHIUnorderedAccessView : public IRHIViewWithDescriptor
    {
    public:
        RHICPUDescriptorHandle GetCPUDescriptorHandle() const override;
        ERHIDescriptorHeapType GetDescriptorHeapType() const override {
            return ERHIDescriptorHeapType::CBV_SRV_UAV;
        }
    };

    /// CBVにディスクリプタ情報追加
    class RHI_API IRHIConstantBufferView : public IRHIViewWithDescriptor
    {
    public:
        RHICPUDescriptorHandle GetCPUDescriptorHandle() const override;
        ERHIDescriptorHeapType GetDescriptorHeapType() const override {
            return ERHIDescriptorHeapType::CBV_SRV_UAV;
        }
    };

    /// Samplerにディスクリプタ情報追加
    class RHI_API IRHISampler : public IRHIViewWithDescriptor
    {
    public:
        RHICPUDescriptorHandle GetCPUDescriptorHandle() const override;
        ERHIDescriptorHeapType GetDescriptorHeapType() const override {
            return ERHIDescriptorHeapType::Sampler;
        }
    };
}
```

- [ ] IRHIViewWithDescriptor インターフェース
- [ ] 各キュークラスへの適用

### 4. ビュー作成時のディスクリプタ割り当て

```cpp
namespace NS::RHI
{
    /// ビュー作成とディスクリプタ管理の統合（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// オフラインディスクリプタマネージャー取得
        virtual RHIOfflineDescriptorManager* GetOfflineDescriptorManager() = 0;

        /// SRV作成（デスクリプタ自動割り当て）。
        /// 内のでオフラインヒープからデスクリプタを割り当て
        virtual IRHIShaderResourceView* CreateShaderResourceView(
            IRHIResource* resource,
            const RHIShaderResourceViewDesc& desc,
            const char* debugName = nullptr) = 0;

        /// UAV作成（デスクリプタ自動割り当て）。
        virtual IRHIUnorderedAccessView* CreateUnorderedAccessView(
            IRHIResource* resource,
            const RHIUnorderedAccessViewDesc& desc,
            const char* debugName = nullptr) = 0;

        /// CBV作成（デスクリプタ自動割り当て）。
        virtual IRHIConstantBufferView* CreateConstantBufferView(
            IRHIBuffer* buffer,
            const RHIConstantBufferViewDesc& desc,
            const char* debugName = nullptr) = 0;

        /// RTV作成（デスクリプタ自動割り当て）。
        virtual IRHIRenderTargetView* CreateRenderTargetView(
            IRHITexture* texture,
            const RHIRenderTargetViewDesc& desc,
            const char* debugName = nullptr) = 0;

        /// DSV作成（デスクリプタ自動割り当て）。
        virtual IRHIDepthStencilView* CreateDepthStencilView(
            IRHITexture* texture,
            const RHIDepthStencilViewDesc& desc,
            const char* debugName = nullptr) = 0;

        /// Sampler作成（デスクリプタ自動割り当て）。
        virtual IRHISampler* CreateSampler(
            const RHISamplerDesc& desc,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] オフラインディスクリプタ自動割り当て
- [ ] ビュー破棄（のディスクリプタ解放

### 5. キャッシュされたビュー

```cpp
namespace NS::RHI
{
    /// ビューキャッシュキー
    struct RHI_API RHIViewCacheKey
    {
        /// リソースポインタ
        IRHIResource* resource = nullptr;

        /// ビュー記述のハッシュ
        uint64 descHash = 0;

        bool operator==(const RHIViewCacheKey& other) const {
            return resource == other.resource && descHash == other.descHash;
        }
    };

    /// ビューキャッシュ
    /// 同一リソース（同一記述のビューをキャッシュ
    /// @threadsafety スレッドセーフではない。外部同期が必要。
    template<typename ViewType, typename DescType>
    class RHI_API RHIViewCache
    {
    public:
        RHIViewCache() = default;

        /// 初期化
        void Initialize(IRHIDevice* device, uint32 maxCacheSize = 1024);

        /// シャットダウン
        void Shutdown();

        /// ビュー取得（キャッシュヒット時は既存を返す）。
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
        // ハッシュマップ実装
        uint32 m_cacheHits = 0;
        uint32 m_cacheMisses = 0;
    };

    /// SRVキャッシュ
    using RHISRVCache = RHIViewCache<IRHIShaderResourceView, RHIShaderResourceViewDesc>;

    /// UAVキャッシュ
    using RHIUAVCache = RHIViewCache<IRHIUnorderedAccessView, RHIUnorderedAccessViewDesc>;
}
```

- [ ] RHIViewCacheKey 構造体
- [ ] RHIViewCache テンプレートクラス

## 検証方法

- [ ] オフラインヒープの割り当て/解放
- [ ] ビュー作成時のディスクリプタ割り当て
- [ ] ビュー破棄（のディスクリプタ解放
- [ ] ビューキャッシュの動作

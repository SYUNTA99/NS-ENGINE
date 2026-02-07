# 10-02: オンラインディスクリプタ

## 目的

GPU可視のオンラインディスクリプタヒープ管理を定義する。

## 参照ドキュメント

- 10-01-descriptor-heap.md (ディスクリプタヒープ基本)
- 02-01-command-context-base.md (IRHICommandContext)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIOnlineDescriptor.h`

## TODO

### 1. オンラインディスクリプタヒープ管理

```cpp
#pragma once

#include "RHIDescriptorHeap.h"

namespace NS::RHI
{
    /// オンラインディスクリプタヒープ
    /// フレームごとにリングバッファ方式で管理
    /// @threadsafety スレッドセーフではない。コマンドコンテキストごとに別インスタンスを使用すること。
    class RHI_API RHIOnlineDescriptorHeap
    {
    public:
        RHIOnlineDescriptorHeap() = default;

        /// 初期化
        /// @param device デバイス
        /// @param type ヒープタイプ
        /// @param numDescriptors ディスクリプタ数
        /// @param numBufferedFrames バッファリングフレーム数
        bool Initialize(
            IRHIDevice* device,
            ERHIDescriptorHeapType type,
            uint32 numDescriptors,
            uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始
        /// 古いレームのディスクリプタを解放
        void BeginFrame(uint64 frameNumber);

        /// フレーム終了
        void EndFrame();

        //=====================================================================
        // ディスクリプタ割り当て
        //=====================================================================

        /// ディスクリプタ割り当て（現在フレーム用）。
        /// @param count 連続デスクリプタ数
        /// @return アロケーション
        /// @note リングバッファ枯渇（head==tail）時はRHI_ASSERTでフェイルファスト。
        ///       ヒープサイズはフレーム内最大使用量の2倍以上を確保すること。
        RHIDescriptorAllocation Allocate(uint32 count = 1);

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const;

        /// 総デスクリプタ数
        uint32 GetTotalCount() const;

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// ヒープタイプ取得
        ERHIDescriptorHeapType GetType() const { return m_type; }

        /// シェーダー可視か
        bool IsShaderVisible() const { return true; }  // オンラインヒープの常にシェーダー可視

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        ERHIDescriptorHeapType m_type = ERHIDescriptorHeapType::CBV_SRV_UAV;

        /// リングバッファ管理
        uint32 m_headIndex = 0;      // 次の割り当て位置
        uint32 m_tailIndex = 0;      // 解放済み末尾
        uint32 m_totalCount = 0;

        /// フレーム境界
        struct FrameMarker {
            uint64 frameNumber;
            uint32 headIndex;
        };
        FrameMarker* m_frameMarkers = nullptr;
        uint32 m_numBufferedFrames = 0;
        uint32 m_currentFrame = 0;
    };
}
```

- [ ] RHIOnlineDescriptorHeap クラス
- [ ] リングバッファ管理
- [ ] フレーム境界管理

### 2. コンテキストへのヒープバインド

```cpp
namespace NS::RHI
{
    /// ディスクリプタヒープバインド（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// ディスクリプタヒープ設定
        /// CBV/SRV/UAVとSamplerの2つまで同時設定可能
        virtual void SetDescriptorHeaps(
            IRHIDescriptorHeap* cbvSrvUavHeap,
            IRHIDescriptorHeap* samplerHeap = nullptr) = 0;

        /// 現在のディスクリプタヒープ取得
        virtual IRHIDescriptorHeap* GetCBVSRVUAVHeap() const = 0;
        virtual IRHIDescriptorHeap* GetSamplerHeap() const = 0;
    };
}
```

- [ ] SetDescriptorHeaps
- [ ] GetCBVSRVUAVHeap / GetSamplerHeap

### 3. ディスクリプタテーブルバインド

```cpp
namespace NS::RHI
{
    /// ディスクリプタテーブルバインド（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// グラフィックスルートデスクリプタテーブル設定
        virtual void SetGraphicsRootDescriptorTable(
            uint32 rootParameterIndex,
            RHIGPUDescriptorHandle baseDescriptor) = 0;

        /// コンピュートルートデスクリプタテーブル設定
        virtual void SetComputeRootDescriptorTable(
            uint32 rootParameterIndex,
            RHIGPUDescriptorHandle baseDescriptor) = 0;

        //=====================================================================
        // ルートディスクリプタ直接設定
        //=====================================================================

        /// グラフィックスルートCBV設定
        virtual void SetGraphicsRootConstantBufferView(
            uint32 rootParameterIndex,
            uint64 bufferLocation) = 0;

        /// グラフィックスルートSRV設定
        virtual void SetGraphicsRootShaderResourceView(
            uint32 rootParameterIndex,
            uint64 bufferLocation) = 0;

        /// グラフィックスルートUAV設定
        virtual void SetGraphicsRootUnorderedAccessView(
            uint32 rootParameterIndex,
            uint64 bufferLocation) = 0;

        /// コンピュートルートCBV設定
        virtual void SetComputeRootConstantBufferView(
            uint32 rootParameterIndex,
            uint64 bufferLocation) = 0;

        /// コンピュートルートSRV設定
        virtual void SetComputeRootShaderResourceView(
            uint32 rootParameterIndex,
            uint64 bufferLocation) = 0;

        /// コンピュートルートUAV設定
        virtual void SetComputeRootUnorderedAccessView(
            uint32 rootParameterIndex,
            uint64 bufferLocation) = 0;
    };
}
```

- [ ] SetGraphicsRootDescriptorTable
- [ ] SetComputeRootDescriptorTable
- [ ] ルートディスクリプタ直接設定

### 4. オンラインディスクリプタマネージャー

```cpp
namespace NS::RHI
{
    /// オンラインディスクリプタマネージャー
    /// CBV/SRV/UAVとサンプラーの両ヒープを統合管理
    class RHI_API RHIOnlineDescriptorManager
    {
    public:
        RHIOnlineDescriptorManager() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            uint32 cbvSrvUavCount = 1000000,  // 100万デスクリプタ
            uint32 samplerCount = 2048,
            uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(uint64 frameNumber);
        void EndFrame();

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

        //=====================================================================
        // ヒープ取得
        //=====================================================================

        IRHIDescriptorHeap* GetCBVSRVUAVHeap() const { return m_cbvSrvUavHeap.GetHeap(); }
        IRHIDescriptorHeap* GetSamplerHeap() const { return m_samplerHeap.GetHeap(); }

        //=====================================================================
        // コンテキストへのバインド
        //=====================================================================

        /// コンテキストにヒープを設定
        void BindToContext(IRHICommandContext* context) {
            context->SetDescriptorHeaps(GetCBVSRVUAVHeap(), GetSamplerHeap());
        }

    private:
        RHIOnlineDescriptorHeap m_cbvSrvUavHeap;
        RHIOnlineDescriptorHeap m_samplerHeap;
    };
}
```

- [ ] RHIOnlineDescriptorManager クラス
- [ ] 両ヒープの統合管理

### 5. ディスクリプタステージング

```cpp
namespace NS::RHI
{
    /// ディスクリプタステージング
    /// オフラインヒープからオンラインヒープへのコピー管理
    /// @threadsafety スレッドセーフではない。コマンドコンテキストごとに別インスタンスを使用すること。
    class RHI_API RHIDescriptorStaging
    {
    public:
        RHIDescriptorStaging() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            RHIOnlineDescriptorManager* onlineManager);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // ステージング
        //=====================================================================

        /// ディスクリプタをステージング
        /// @param srcHandle オフラインヒープのソースハンドル
        /// @param type ディスクリプタタイプ
        /// @return オンラインヒープのGPUハンドル
        RHIGPUDescriptorHandle Stage(
            RHICPUDescriptorHandle srcHandle,
            ERHIDescriptorHeapType type);

        /// 複数ディスクリプタをステージング
        RHIGPUDescriptorHandle StageRange(
            RHICPUDescriptorHandle srcHandle,
            uint32 count,
            ERHIDescriptorHeapType type);

        /// ビューをステージング
        RHIGPUDescriptorHandle StageSRV(IRHIShaderResourceView* srv);
        RHIGPUDescriptorHandle StageUAV(IRHIUnorderedAccessView* uav);
        RHIGPUDescriptorHandle StageCBV(IRHIConstantBufferView* cbv);
        RHIGPUDescriptorHandle StageSampler(IRHISampler* sampler);

        //=====================================================================
        // バッチステージング
        //=====================================================================

        /// ステージングバッチ開始
        void BeginBatch();

        /// バッチに追加
        void AddToBatch(RHICPUDescriptorHandle srcHandle, ERHIDescriptorHeapType type);

        /// バッチ実行
        /// @return 先頭のGPUハンドル
        RHIGPUDescriptorHandle EndBatch();

    private:
        IRHIDevice* m_device = nullptr;
        RHIOnlineDescriptorManager* m_onlineManager = nullptr;

        /// バッチ用
        struct BatchEntry {
            RHICPUDescriptorHandle srcHandle;
            ERHIDescriptorHeapType type;
        };
        BatchEntry* m_batchEntries = nullptr;
        uint32 m_batchCount = 0;
        uint32 m_batchCapacity = 0;
    };
}
```

- [ ] RHIDescriptorStaging クラス
- [ ] バッチステージング

## 枯渇時フォールバック

1. リングバッファ枯渇（head==tail）→ GPU完了待ちフラッシュ後リトライ
2. フラッシュ後も枯渇 → フレーム内ディスクリプタ使用量ログ + RHI_ASSERT
3. 予防: 初期化時にフレーム最大使用量×2以上のヒープサイズを確保

## 検証方法

- [ ] オンラインヒープの割り当て/解放
- [ ] フレーム境界での解放
- [ ] コンテキストへのバインド
- [ ] ステージングの動作

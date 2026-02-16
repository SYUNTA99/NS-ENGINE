/// @file RHIOnlineDescriptor.h
/// @brief オンラインディスクリプタ管理
/// @details GPU可視のオンラインディスクリプタヒープ、マネージャー、ステージングを提供。
/// @see 10-02-online-descriptor.md
#pragma once

#include "RHIDescriptorHeap.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIOnlineDescriptorHeap (10-02)
    //=========================================================================

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
        bool Initialize(IRHIDevice* device,
                        ERHIDescriptorHeapType type,
                        uint32 numDescriptors,
                        uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始（古いフレームのディスクリプタを解放）
        void BeginFrame(uint64 frameNumber);

        /// フレーム終了
        void EndFrame();

        //=====================================================================
        // ディスクリプタ割り当て
        //=====================================================================

        /// ディスクリプタ割り当て（現在フレーム用）
        /// @param count 連続ディスクリプタ数
        /// @return アロケーション
        /// @note リングバッファ枯渇時はIsValid()==falseを返す。
        ///       ヒープサイズはフレーム内最大使用量の2倍以上を確保すること。
        RHIDescriptorAllocation Allocate(uint32 count = 1);

        /// 利用可能ディスクリプタ数
        uint32 GetAvailableCount() const;

        /// 総ディスクリプタ数
        uint32 GetTotalCount() const;

        //=====================================================================
        // ヒープ情報
        //=====================================================================

        /// ヒープ取得
        IRHIDescriptorHeap* GetHeap() const { return m_heap; }

        /// ヒープタイプ取得
        ERHIDescriptorHeapType GetType() const { return m_type; }

        /// シェーダー可視か（オンラインヒープは常にtrue）
        bool IsShaderVisible() const { return true; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIDescriptorHeapRef m_heap;
        ERHIDescriptorHeapType m_type = ERHIDescriptorHeapType::CBV_SRV_UAV;

        /// リングバッファ管理
        uint32 m_headIndex = 0;
        uint32 m_tailIndex = 0;
        uint32 m_totalCount = 0;

        /// フレーム境界
        struct FrameMarker
        {
            uint64 frameNumber;
            uint32 headIndex;
        };
        FrameMarker* m_frameMarkers = nullptr;
        uint32 m_numBufferedFrames = 0;
        uint32 m_currentFrame = 0;
    };

    //=========================================================================
    // RHIOnlineDescriptorManager (10-02)
    //=========================================================================

    /// オンラインディスクリプタマネージャー
    /// CBV/SRV/UAVとサンプラーの両ヒープを統合管理
    class RHI_API RHIOnlineDescriptorManager
    {
    public:
        RHIOnlineDescriptorManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device,
                        uint32 cbvSrvUavCount = 1000000,
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
        RHIDescriptorAllocation AllocateCBVSRVUAV(uint32 count = 1) { return m_cbvSrvUavHeap.Allocate(count); }

        /// サンプラーディスクリプタ割り当て
        RHIDescriptorAllocation AllocateSampler(uint32 count = 1) { return m_samplerHeap.Allocate(count); }

        //=====================================================================
        // ヒープ取得
        //=====================================================================

        IRHIDescriptorHeap* GetCBVSRVUAVHeap() const { return m_cbvSrvUavHeap.GetHeap(); }
        IRHIDescriptorHeap* GetSamplerHeap() const { return m_samplerHeap.GetHeap(); }

        //=====================================================================
        // コンテキストへのバインド
        //=====================================================================

        /// コンテキストにヒープを設定
        /// ※ 非インライン（IRHICommandContextの定義に依存）
        void BindToContext(IRHICommandContext* context);

    private:
        RHIOnlineDescriptorHeap m_cbvSrvUavHeap;
        RHIOnlineDescriptorHeap m_samplerHeap;
    };

    //=========================================================================
    // RHIDescriptorStaging (10-02)
    //=========================================================================

    /// ディスクリプタステージング
    /// オフラインヒープからオンラインヒープへのコピー管理
    /// @threadsafety スレッドセーフではない。コマンドコンテキストごとに別インスタンスを使用すること。
    class RHI_API RHIDescriptorStaging
    {
    public:
        RHIDescriptorStaging() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, RHIOnlineDescriptorManager* onlineManager);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // ステージング
        //=====================================================================

        /// ディスクリプタをステージング
        /// @param srcHandle オフラインヒープのソースハンドル
        /// @param type ディスクリプタタイプ
        /// @return オンラインヒープのGPUハンドル
        RHIGPUDescriptorHandle Stage(RHICPUDescriptorHandle srcHandle, ERHIDescriptorHeapType type);

        /// 複数ディスクリプタをステージング
        RHIGPUDescriptorHandle StageRange(RHICPUDescriptorHandle srcHandle, uint32 count, ERHIDescriptorHeapType type);

        /// ビューをステージング
        /// ※ 非インライン（IRHIShaderResourceView等の定義に依存）
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
        struct BatchEntry
        {
            RHICPUDescriptorHandle srcHandle;
            ERHIDescriptorHeapType type;
        };
        BatchEntry* m_batchEntries = nullptr;
        uint32 m_batchCount = 0;
        uint32 m_batchCapacity = 0;
    };

}} // namespace NS::RHI

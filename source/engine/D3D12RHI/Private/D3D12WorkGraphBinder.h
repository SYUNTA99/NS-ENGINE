/// @file D3D12WorkGraphBinder.h
/// @brief D3D12 Work Graph バインダー — ノード毎リソースバインド管理
#pragma once

#include "D3D12RHIPrivate.h"

#include <vector>

namespace NS::RHI
{
    class IRHIShaderResourceView;
    class IRHIUnorderedAccessView;
    class IRHIConstantBufferView;
    class IRHISampler;
    class IRHIResource;
} // namespace NS::RHI

namespace NS::D3D12RHI
{
    class D3D12Device;
    class D3D12WorkGraphPipeline;

    //=========================================================================
    // D3D12WorkGraphBinderOps — 遷移重複排除
    //=========================================================================

    /// ワーカースレッド単位のリソース遷移追跡。
    /// 並列RecordBindings時に各ワーカーが独立インスタンスを持ち、
    /// 完了後にワーカー0にマージして重複遷移を排除する。
    class D3D12WorkGraphBinderOps
    {
    public:
        static constexpr uint32 kMaxTrackedResources = 256;

        D3D12WorkGraphBinderOps() = default;

        /// リソース遷移追加（重複チェック付き）
        /// @return true=新規追加、false=既に追跡済み
        bool AddResourceTransition(NS::RHI::IRHIResource* resource);

        /// UAVクリア登録
        void AddUAVClear(NS::RHI::IRHIUnorderedAccessView* uav);

        /// 遷移リソース一覧取得
        const std::vector<NS::RHI::IRHIResource*>& GetTransitionResources() const { return transitionResources_; }

        /// UAVクリア一覧取得
        const std::vector<NS::RHI::IRHIUnorderedAccessView*>& GetClearUAVs() const { return clearUAVs_; }

        /// 別ワーカーの結果をマージ（重複排除あり）
        void MergeFrom(const D3D12WorkGraphBinderOps& other);

        /// 状態リセット
        void Reset();

        /// 追跡リソース数
        uint32 GetTransitionCount() const { return static_cast<uint32>(transitionResources_.size()); }

    private:
        std::vector<NS::RHI::IRHIResource*> transitionResources_;
        std::vector<NS::RHI::IRHIUnorderedAccessView*> clearUAVs_;
    };

    //=========================================================================
    // D3D12WorkGraphNodeBindings — ノード毎バインド状態
    //=========================================================================

    /// 単一ノードのリソースバインド状態。
    /// ビューポインタとGPU仮想アドレスを追跡し、
    /// ルート引数テーブルへの書き込みに使用。
    struct D3D12WorkGraphNodeBindings
    {
        static constexpr uint32 kMaxCBVs = 14;
        static constexpr uint32 kMaxSRVs = 64;
        static constexpr uint32 kMaxUAVs = 16;
        static constexpr uint32 kMaxSamplers = 16;

        /// バインド済みビューポインタ
        NS::RHI::IRHIShaderResourceView* srvs[kMaxSRVs] = {};
        NS::RHI::IRHIUnorderedAccessView* uavs[kMaxUAVs] = {};
        NS::RHI::IRHISampler* samplers[kMaxSamplers] = {};

        /// CBV GPU仮想アドレス（直接バインド用）
        D3D12_GPU_VIRTUAL_ADDRESS cbvAddresses[kMaxCBVs] = {};

        /// バインド済みビットマスク
        uint64 boundCBVMask = 0;
        uint64 boundSRVMask = 0;
        uint32 boundUAVMask = 0;
        uint32 boundSamplerMask = 0;

        /// リセット
        void Reset();
    };

    //=========================================================================
    // D3D12WorkGraphBinder — ワークグラフバインダー
    //=========================================================================

    /// ワークグラフのノード毎リソースバインド管理。
    /// 並列RecordBindingsをサポートし、ローカルルート引数テーブルを構築する。
    class D3D12WorkGraphBinder
    {
    public:
        static constexpr uint32 kMaxWorkers = 4;
        static constexpr uint32 kMaxNodes = 64;
        static constexpr uint32 kRootArgStrideAlignment = 16; // 16バイトアライメント

        D3D12WorkGraphBinder() = default;
        ~D3D12WorkGraphBinder() = default;

        /// 初期化
        /// @param device D3D12デバイス
        /// @param pipeline ワークグラフパイプライン
        /// @return 成功時true
        bool Init(D3D12Device* device, D3D12WorkGraphPipeline* pipeline);

        /// ノードへのSRVバインド
        void SetSRV(uint32 nodeIndex, uint32 slot, NS::RHI::IRHIShaderResourceView* srv, uint32 workerIndex = 0);

        /// ノードへのUAVバインド
        void SetUAV(uint32 nodeIndex,
                    uint32 slot,
                    NS::RHI::IRHIUnorderedAccessView* uav,
                    bool clearResource = false,
                    uint32 workerIndex = 0);

        /// ノードへのCBVバインド（アドレス直接）
        void SetCBV(uint32 nodeIndex, uint32 slot, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, uint32 workerIndex = 0);

        /// ノードへのSamplerバインド
        void SetSampler(uint32 nodeIndex, uint32 slot, NS::RHI::IRHISampler* sampler, uint32 workerIndex = 0);

        /// ルート引数テーブル構築（全ノード分）
        /// @param outRootArgs 出力先ルート引数配列
        /// @param outRootArgSize 出力サイズ（バイト）
        void BuildRootArgTable(std::vector<uint32>& outRootArgs, uint32& outRootArgSize) const;

        /// 並列ワーカー結果マージ（ワーカー1..Nの遷移をワーカー0にマージ）
        void MergeWorkerOps();

        /// ワーカー0の遷移Ops取得（マージ後）
        const D3D12WorkGraphBinderOps& GetMergedOps() const { return workerOps_[0]; }

        /// リセット
        void Reset();

        /// ノード数取得
        uint32 GetNodeCount() const { return nodeCount_; }

    private:
        D3D12Device* device_ = nullptr;
        D3D12WorkGraphPipeline* pipeline_ = nullptr;
        uint32 nodeCount_ = 0;

        /// ノード毎バインド状態
        D3D12WorkGraphNodeBindings nodeBindings_[kMaxNodes];

        /// ワーカー毎の遷移Ops
        D3D12WorkGraphBinderOps workerOps_[kMaxWorkers];
    };

} // namespace NS::D3D12RHI

/// @file D3D12WorkGraphBinder.cpp
/// @brief D3D12 Work Graph バインダー実装
#include "D3D12WorkGraphBinder.h"
#include "D3D12Device.h"
#include "D3D12WorkGraph.h"
#include "RHI/Public/IRHIViews.h"

#include <cstring>

namespace NS::D3D12RHI
{
    //=========================================================================
    // D3D12WorkGraphBinderOps
    //=========================================================================

    bool D3D12WorkGraphBinderOps::AddResourceTransition(NS::RHI::IRHIResource* resource)
    {
        if (!resource)
            return false;

        // 重複チェック（線形探索、kMaxTrackedResourcesまで）
        for (auto* r : transitionResources_)
        {
            if (r == resource)
                return false;
        }

        if (transitionResources_.size() >= kMaxTrackedResources)
            return false;

        transitionResources_.push_back(resource);
        return true;
    }

    void D3D12WorkGraphBinderOps::AddUAVClear(NS::RHI::IRHIUnorderedAccessView* uav)
    {
        if (!uav)
            return;

        clearUAVs_.push_back(uav);
    }

    void D3D12WorkGraphBinderOps::MergeFrom(const D3D12WorkGraphBinderOps& other)
    {
        // 遷移リソースをマージ（重複排除）
        for (auto* resource : other.transitionResources_)
        {
            AddResourceTransition(resource);
        }

        // UAVクリアはそのまま追加
        for (auto* uav : other.clearUAVs_)
        {
            clearUAVs_.push_back(uav);
        }
    }

    void D3D12WorkGraphBinderOps::Reset()
    {
        transitionResources_.clear();
        clearUAVs_.clear();
    }

    //=========================================================================
    // D3D12WorkGraphNodeBindings
    //=========================================================================

    void D3D12WorkGraphNodeBindings::Reset()
    {
        memset(srvs, 0, sizeof(srvs));
        memset(uavs, 0, sizeof(uavs));
        memset(samplers, 0, sizeof(samplers));
        memset(cbvAddresses, 0, sizeof(cbvAddresses));
        boundCBVMask = 0;
        boundSRVMask = 0;
        boundUAVMask = 0;
        boundSamplerMask = 0;
    }

    //=========================================================================
    // D3D12WorkGraphBinder
    //=========================================================================

    bool D3D12WorkGraphBinder::Init(D3D12Device* device, D3D12WorkGraphPipeline* pipeline)
    {
        if (!device || !pipeline)
            return false;

        device_ = device;
        pipeline_ = pipeline;
        nodeCount_ = pipeline->GetNodeCount();

        if (nodeCount_ > kMaxNodes)
            nodeCount_ = kMaxNodes;

        Reset();
        return true;
    }

    void D3D12WorkGraphBinder::SetSRV(uint32 nodeIndex,
                                      uint32 slot,
                                      NS::RHI::IRHIShaderResourceView* srv,
                                      uint32 workerIndex)
    {
        if (nodeIndex >= nodeCount_ || slot >= D3D12WorkGraphNodeBindings::kMaxSRVs)
            return;

        auto& bindings = nodeBindings_[nodeIndex];

        if (srv)
        {
            bindings.srvs[slot] = srv;
            bindings.boundSRVMask |= (1ULL << slot);

            // リソース遷移追跡
            if (workerIndex < kMaxWorkers && srv->GetResource())
            {
                workerOps_[workerIndex].AddResourceTransition(srv->GetResource());
            }
        }
        else
        {
            bindings.srvs[slot] = nullptr;
            bindings.boundSRVMask &= ~(1ULL << slot);
        }
    }

    void D3D12WorkGraphBinder::SetUAV(
        uint32 nodeIndex, uint32 slot, NS::RHI::IRHIUnorderedAccessView* uav, bool clearResource, uint32 workerIndex)
    {
        if (nodeIndex >= nodeCount_ || slot >= D3D12WorkGraphNodeBindings::kMaxUAVs)
            return;

        auto& bindings = nodeBindings_[nodeIndex];

        if (uav)
        {
            bindings.uavs[slot] = uav;
            bindings.boundUAVMask |= (1u << slot);

            if (workerIndex < kMaxWorkers)
            {
                if (uav->GetResource())
                    workerOps_[workerIndex].AddResourceTransition(uav->GetResource());

                if (clearResource)
                    workerOps_[workerIndex].AddUAVClear(uav);
            }
        }
        else
        {
            bindings.uavs[slot] = nullptr;
            bindings.boundUAVMask &= ~(1u << slot);
        }
    }

    void D3D12WorkGraphBinder::SetCBV(uint32 nodeIndex,
                                      uint32 slot,
                                      D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
                                      uint32 workerIndex)
    {
        (void)workerIndex;

        if (nodeIndex >= nodeCount_ || slot >= D3D12WorkGraphNodeBindings::kMaxCBVs)
            return;

        auto& bindings = nodeBindings_[nodeIndex];
        bindings.cbvAddresses[slot] = gpuAddress;
        bindings.boundCBVMask |= (1ULL << slot);
    }

    void D3D12WorkGraphBinder::SetSampler(uint32 nodeIndex,
                                          uint32 slot,
                                          NS::RHI::IRHISampler* sampler,
                                          uint32 workerIndex)
    {
        (void)workerIndex;

        if (nodeIndex >= nodeCount_ || slot >= D3D12WorkGraphNodeBindings::kMaxSamplers)
            return;

        auto& bindings = nodeBindings_[nodeIndex];

        if (sampler)
        {
            bindings.samplers[slot] = sampler;
            bindings.boundSamplerMask |= (1u << slot);
        }
        else
        {
            bindings.samplers[slot] = nullptr;
            bindings.boundSamplerMask &= ~(1u << slot);
        }
    }

    void D3D12WorkGraphBinder::BuildRootArgTable(std::vector<uint32>& outRootArgs, uint32& outRootArgSize) const
    {
        // ルート引数テーブル構築
        // 各ノードのCBV GPU仮想アドレスをDWORD配列に書き込む
        // レイアウト: [ノード0 CBV0_lo, CBV0_hi, CBV1_lo, CBV1_hi, ...][ノード1 ...][...]

        // 1ノードあたりのルート引数サイズ計算（CBVアドレスのみ）
        uint32 maxCBVCount = 0;
        for (uint32 n = 0; n < nodeCount_; ++n)
        {
            uint32 cbvCount = 0;
            uint64 mask = nodeBindings_[n].boundCBVMask;
            while (mask)
            {
                cbvCount++;
                mask &= (mask - 1);
            }
            if (cbvCount > maxCBVCount)
                maxCBVCount = cbvCount;
        }

        // 各CBVアドレスは8バイト(2 DWORD)
        uint32 nodeArgSizeBytes = maxCBVCount * 8;
        // 16バイトアライメント
        nodeArgSizeBytes = (nodeArgSizeBytes + kRootArgStrideAlignment - 1) & ~(kRootArgStrideAlignment - 1);
        if (nodeArgSizeBytes == 0)
            nodeArgSizeBytes = kRootArgStrideAlignment;

        uint32 nodeArgStrideDwords = nodeArgSizeBytes / 4;
        uint32 totalDwords = nodeArgStrideDwords * nodeCount_;

        outRootArgs.resize(totalDwords, 0);
        outRootArgSize = totalDwords * 4;

        for (uint32 n = 0; n < nodeCount_; ++n)
        {
            const auto& bindings = nodeBindings_[n];
            uint32 baseOffset = n * nodeArgStrideDwords;
            uint32 cbvSlot = 0;

            for (uint32 c = 0; c < D3D12WorkGraphNodeBindings::kMaxCBVs; ++c)
            {
                if (bindings.boundCBVMask & (1ULL << c))
                {
                    uint32 offset = baseOffset + cbvSlot * 2;
                    if (offset + 1 < totalDwords)
                    {
                        // GPU仮想アドレスを2つのDWORDとして格納
                        uint64 addr = bindings.cbvAddresses[c];
                        outRootArgs[offset] = static_cast<uint32>(addr & 0xFFFFFFFF);
                        outRootArgs[offset + 1] = static_cast<uint32>(addr >> 32);
                    }
                    cbvSlot++;
                }
            }
        }
    }

    void D3D12WorkGraphBinder::MergeWorkerOps()
    {
        // ワーカー1..Nの遷移をワーカー0にマージ
        for (uint32 w = 1; w < kMaxWorkers; ++w)
        {
            workerOps_[0].MergeFrom(workerOps_[w]);
        }
    }

    void D3D12WorkGraphBinder::Reset()
    {
        for (uint32 n = 0; n < kMaxNodes; ++n)
            nodeBindings_[n].Reset();

        for (uint32 w = 0; w < kMaxWorkers; ++w)
            workerOps_[w].Reset();
    }

} // namespace NS::D3D12RHI

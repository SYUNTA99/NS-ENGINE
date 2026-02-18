/// @file D3D12PipelineStateCache.h
/// @brief D3D12 PSOキャッシュ実装
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIPipelineState.h"

#include <shared_mutex>
#include <unordered_map>

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12PipelineStateCache
    //=========================================================================

    /// ハッシュベースPSOキャッシュ
    /// RWLock保護: Read=ヒット検索（共有）、Write=ミス挿入（排他）
    class D3D12PipelineStateCache final : public NS::RHI::IRHIPipelineStateCache
    {
    public:
        D3D12PipelineStateCache() = default;
        ~D3D12PipelineStateCache() override = default;

        /// IRHIPipelineStateCache
        void AddPipelineState(const void* descHash, size_t hashSize, NS::RHI::IRHIGraphicsPipelineState* pso) override;
        NS::RHI::IRHIGraphicsPipelineState* FindPipelineState(const void* descHash, size_t hashSize) override;
        bool SaveToFile(const char* path) override;
        bool LoadFromFile(const char* path) override;
        void Clear() override;
        uint32 GetEntryCount() const override;

        /// Compute PSO用拡張
        void AddComputePipelineState(uint64 hash, NS::RHI::IRHIComputePipelineState* pso);
        NS::RHI::IRHIComputePipelineState* FindComputePipelineState(uint64 hash);

    private:
        /// ハッシュキー計算
        static uint64 ComputeKey(const void* data, size_t size);

        mutable std::shared_mutex mutex_;
        std::unordered_map<uint64, NS::RHI::IRHIGraphicsPipelineState*> graphicsCache_;
        std::unordered_map<uint64, NS::RHI::IRHIComputePipelineState*> computeCache_;
    };

} // namespace NS::D3D12RHI

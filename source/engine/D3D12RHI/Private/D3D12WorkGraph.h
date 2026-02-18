/// @file D3D12WorkGraph.h
/// @brief D3D12 Work Graphs パイプライン
#pragma once

#include "D3D12RHIPrivate.h"
#include "RHI/Public/IRHIWorkGraph.h"
#include "RHI/Public/RHIWorkGraphTypes.h"

namespace NS::D3D12RHI
{
    class D3D12Device;

    //=========================================================================
    // D3D12WorkGraphPipeline — IRHIWorkGraphPipeline実装
    //=========================================================================

    class D3D12WorkGraphPipeline final : public NS::RHI::IRHIWorkGraphPipeline
    {
    public:
        D3D12WorkGraphPipeline() = default;
        ~D3D12WorkGraphPipeline() override = default;

        /// 初期化
        bool Init(D3D12Device* device, const NS::RHI::RHIWorkGraphPipelineDesc& desc);

        // --- IRHIWorkGraphPipeline ---
        uint64 GetProgramIdentifier() const override { return programId_; }
        uint64 GetBackingMemorySize() const override { return backingMemorySize_; }
        uint32 GetNodeCount() const override { return nodeCount_; }
        int32 GetNodeIndex(const char* nodeName) const override;
        uint32 GetEntryPointCount() const override { return entryPointCount_; }

        /// D3D12ネイティブ取得
        ID3D12StateObject* GetStateObject() const { return stateObject_.Get(); }

    private:
        D3D12Device* device_ = nullptr;
        ComPtr<ID3D12StateObject> stateObject_;
        uint64 programId_ = 0;
        uint64 backingMemorySize_ = 0;
        uint32 nodeCount_ = 0;
        uint32 entryPointCount_ = 0;
    };

} // namespace NS::D3D12RHI

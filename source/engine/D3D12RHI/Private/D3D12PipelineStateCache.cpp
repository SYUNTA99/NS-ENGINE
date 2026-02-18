/// @file D3D12PipelineStateCache.cpp
/// @brief D3D12 PSOキャッシュ実装
#include "D3D12PipelineStateCache.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // ハッシュ計算（FNV-1a 64bit）
    //=========================================================================

    uint64 D3D12PipelineStateCache::ComputeKey(const void* data, size_t size)
    {
        const auto* bytes = static_cast<const uint8*>(data);
        uint64 hash = 14695981039346656037ULL; // FNV offset basis
        for (size_t i = 0; i < size; ++i)
        {
            hash ^= static_cast<uint64>(bytes[i]);
            hash *= 1099511628211ULL; // FNV prime
        }
        return hash;
    }

    //=========================================================================
    // IRHIPipelineStateCache
    //=========================================================================

    void D3D12PipelineStateCache::AddPipelineState(const void* descHash,
                                                   size_t hashSize,
                                                   NS::RHI::IRHIGraphicsPipelineState* pso)
    {
        if (!descHash || hashSize == 0 || !pso)
            return;

        uint64 key = ComputeKey(descHash, hashSize);

        std::unique_lock lock(mutex_);
        graphicsCache_[key] = pso;
    }

    NS::RHI::IRHIGraphicsPipelineState* D3D12PipelineStateCache::FindPipelineState(const void* descHash,
                                                                                   size_t hashSize)
    {
        if (!descHash || hashSize == 0)
            return nullptr;

        uint64 key = ComputeKey(descHash, hashSize);

        std::shared_lock lock(mutex_);
        auto it = graphicsCache_.find(key);
        return (it != graphicsCache_.end()) ? it->second : nullptr;
    }

    bool D3D12PipelineStateCache::SaveToFile(const char* /*path*/)
    {
        // ディスクキャッシュ: 将来拡張
        return false;
    }

    bool D3D12PipelineStateCache::LoadFromFile(const char* /*path*/)
    {
        // ディスクキャッシュ: 将来拡張
        return false;
    }

    void D3D12PipelineStateCache::Clear()
    {
        std::unique_lock lock(mutex_);
        graphicsCache_.clear();
        computeCache_.clear();
    }

    uint32 D3D12PipelineStateCache::GetEntryCount() const
    {
        std::shared_lock lock(mutex_);
        return static_cast<uint32>(graphicsCache_.size() + computeCache_.size());
    }

    //=========================================================================
    // Compute PSO拡張
    //=========================================================================

    void D3D12PipelineStateCache::AddComputePipelineState(uint64 hash, NS::RHI::IRHIComputePipelineState* pso)
    {
        if (!pso)
            return;

        std::unique_lock lock(mutex_);
        computeCache_[hash] = pso;
    }

    NS::RHI::IRHIComputePipelineState* D3D12PipelineStateCache::FindComputePipelineState(uint64 hash)
    {
        std::shared_lock lock(mutex_);
        auto it = computeCache_.find(hash);
        return (it != computeCache_.end()) ? it->second : nullptr;
    }

} // namespace NS::D3D12RHI

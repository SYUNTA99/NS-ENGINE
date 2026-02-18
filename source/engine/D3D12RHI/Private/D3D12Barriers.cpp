/// @file D3D12Barriers.cpp
/// @brief D3D12 バリアシステム（Legacy + Enhanced）実装
#include "D3D12Barriers.h"
#include "D3D12Buffer.h"
#include "D3D12Resource.h"
#include "D3D12Texture.h"

namespace NS::D3D12RHI
{
    //=========================================================================
    // ヘルパー関数
    //=========================================================================

    D3D12_RESOURCE_BARRIER_FLAGS ConvertBarrierFlags(NS::RHI::ERHIBarrierFlags flags)
    {
        if ((flags & NS::RHI::ERHIBarrierFlags::BeginOnly) != NS::RHI::ERHIBarrierFlags::None)
            return D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
        if ((flags & NS::RHI::ERHIBarrierFlags::EndOnly) != NS::RHI::ERHIBarrierFlags::None)
            return D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
        return D3D12_RESOURCE_BARRIER_FLAG_NONE;
    }

    ID3D12Resource* GetD3D12Resource(NS::RHI::IRHIResource* resource)
    {
        if (!resource)
        {
            return nullptr;
        }

        auto type = resource->GetResourceType();
        if (type == NS::RHI::ERHIResourceType::Buffer)
        {
            return static_cast<D3D12Buffer*>(resource)->GetD3DResource();
        }
        if (type == NS::RHI::ERHIResourceType::Texture)
        {
            return static_cast<D3D12Texture*>(resource)->GetD3DResource();
        }
        return nullptr;
    }

    //=========================================================================
    // D3D12BarrierBatcher
    //=========================================================================

    int32 D3D12BarrierBatcher::AddTransition(ID3D12Resource* resource,
                                             D3D12_RESOURCE_STATES before,
                                             D3D12_RESOURCE_STATES after,
                                             uint32 subresource,
                                             D3D12_RESOURCE_BARRIER_FLAGS flags)
    {
        if (!resource || before == after)
        {
            return 0;
        }

        // 逆遷移キャンセル最適化:
        // 直前のバリアが同一リソース・同一サブリソースで逆方向の遷移なら両方除去
        if (count_ > 0 && flags == D3D12_RESOURCE_BARRIER_FLAG_NONE)
        {
            auto& last = barriers_[count_ - 1];
            if (last.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION && last.Flags == D3D12_RESOURCE_BARRIER_FLAG_NONE &&
                last.Transition.pResource == resource && last.Transition.Subresource == subresource &&
                last.Transition.StateBefore == after && last.Transition.StateAfter == before)
            {
                --count_;
                return 0;
            }
        }

        if (!HasCapacity())
        {
            FlushIfFull();
            if (!HasCapacity())
            {
                return -1;
            }
        }

        auto& barrier = barriers_[count_++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = flags;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = subresource;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        return 1;
    }

    void D3D12BarrierBatcher::AddUAV(ID3D12Resource* resource)
    {
        if (!HasCapacity())
        {
            FlushIfFull();
        }
        if (!HasCapacity())
        {
            return;
        }

        auto& barrier = barriers_[count_++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = resource;
    }

    void D3D12BarrierBatcher::AddAliasing(ID3D12Resource* before, ID3D12Resource* after)
    {
        if (!HasCapacity())
        {
            FlushIfFull();
        }
        if (!HasCapacity())
        {
            return;
        }

        auto& barrier = barriers_[count_++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Aliasing.pResourceBefore = before;
        barrier.Aliasing.pResourceAfter = after;
    }

    void D3D12BarrierBatcher::AddTransitionFromRHI(NS::RHI::IRHIResource* resource,
                                                   NS::RHI::ERHIResourceState before,
                                                   NS::RHI::ERHIResourceState after,
                                                   uint32 subresource,
                                                   NS::RHI::ERHIBarrierFlags flags)
    {
        auto* d3dRes = GetD3D12Resource(resource);
        if (!d3dRes)
        {
            return;
        }

        D3D12_RESOURCE_STATES stateBefore = D3D12GpuResource::ConvertToD3D12State(before);
        D3D12_RESOURCE_STATES stateAfter = D3D12GpuResource::ConvertToD3D12State(after);

        uint32 d3dSubresource =
            (subresource == NS::RHI::kAllSubresources) ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : subresource;

        AddTransition(d3dRes, stateBefore, stateAfter, d3dSubresource, ConvertBarrierFlags(flags));
    }

    void D3D12BarrierBatcher::Flush(ID3D12GraphicsCommandList* cmdList)
    {
        if (count_ == 0 || !cmdList)
        {
            return;
        }

        cmdList->ResourceBarrier(count_, barriers_);
        count_ = 0;
    }

    void D3D12BarrierBatcher::Reset()
    {
        count_ = 0;
    }

    //=========================================================================
    // Enhanced Barrier 変換ヘルパー
    //=========================================================================

    D3D12_BARRIER_SYNC ConvertBarrierSync(NS::RHI::ERHIBarrierSync sync)
    {
        using S = NS::RHI::ERHIBarrierSync;

        if (sync == S::None)
            return D3D12_BARRIER_SYNC_NONE;
        if (sync == S::All)
            return D3D12_BARRIER_SYNC_ALL;

        D3D12_BARRIER_SYNC result = D3D12_BARRIER_SYNC_NONE;

        if ((sync & S::Draw) != S::None)
            result |= D3D12_BARRIER_SYNC_DRAW;
        if ((sync & S::IndexInput) != S::None)
            result |= D3D12_BARRIER_SYNC_INDEX_INPUT;
        if ((sync & S::VertexShading) != S::None)
            result |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
        if ((sync & S::PixelShading) != S::None)
            result |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
        if ((sync & S::DepthStencil) != S::None)
            result |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
        if ((sync & S::RenderTarget) != S::None)
            result |= D3D12_BARRIER_SYNC_RENDER_TARGET;
        if ((sync & S::Compute) != S::None)
            result |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
        if ((sync & S::Raytracing) != S::None)
            result |= D3D12_BARRIER_SYNC_RAYTRACING;
        if ((sync & S::Copy) != S::None)
            result |= D3D12_BARRIER_SYNC_COPY;
        if ((sync & S::Resolve) != S::None)
            result |= D3D12_BARRIER_SYNC_RESOLVE;
        if ((sync & S::ExecuteIndirect) != S::None)
            result |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
        if ((sync & S::AllShading) != S::None)
            result |= D3D12_BARRIER_SYNC_ALL_SHADING;
        if ((sync & S::NonPixelShading) != S::None)
            result |= D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
        if ((sync & S::BuildRaytracingAccelerationStructure) != S::None)
            result |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
        if ((sync & S::CopyRaytracingAccelerationStructure) != S::None)
            result |= D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;
        if ((sync & S::Split) != S::None)
            result |= D3D12_BARRIER_SYNC_SPLIT;

        return result;
    }

    D3D12_BARRIER_ACCESS ConvertBarrierAccess(NS::RHI::ERHIBarrierAccess access)
    {
        using A = NS::RHI::ERHIBarrierAccess;

        if (access == A::Common || access == A::NoAccess)
            return D3D12_BARRIER_ACCESS_COMMON;

        D3D12_BARRIER_ACCESS result = D3D12_BARRIER_ACCESS_COMMON;

        if ((access & A::VertexBuffer) != A::Common)
            result |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
        if ((access & A::ConstantBuffer) != A::Common)
            result |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
        if ((access & A::IndexBuffer) != A::Common)
            result |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
        if ((access & A::RenderTarget) != A::Common)
            result |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
        if ((access & A::UnorderedAccess) != A::Common)
            result |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
        if ((access & A::DepthStencilWrite) != A::Common)
            result |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
        if ((access & A::DepthStencilRead) != A::Common)
            result |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
        if ((access & A::ShaderResource) != A::Common)
            result |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
        if ((access & A::StreamOutput) != A::Common)
            result |= D3D12_BARRIER_ACCESS_STREAM_OUTPUT;
        if ((access & A::IndirectArgument) != A::Common)
            result |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
        if ((access & A::CopyDest) != A::Common)
            result |= D3D12_BARRIER_ACCESS_COPY_DEST;
        if ((access & A::CopySource) != A::Common)
            result |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
        if ((access & A::ResolveDest) != A::Common)
            result |= D3D12_BARRIER_ACCESS_RESOLVE_DEST;
        if ((access & A::ResolveSource) != A::Common)
            result |= D3D12_BARRIER_ACCESS_RESOLVE_SOURCE;
        if ((access & A::RaytracingAccelerationStructureRead) != A::Common)
            result |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
        if ((access & A::RaytracingAccelerationStructureWrite) != A::Common)
            result |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
        if ((access & A::ShadingRate) != A::Common)
            result |= D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE;

        return result;
    }

    D3D12_BARRIER_LAYOUT ConvertBarrierLayout(NS::RHI::ERHIBarrierLayout layout)
    {
        switch (layout)
        {
        case NS::RHI::ERHIBarrierLayout::Undefined:
            return D3D12_BARRIER_LAYOUT_UNDEFINED;
        case NS::RHI::ERHIBarrierLayout::Common:
            return D3D12_BARRIER_LAYOUT_COMMON;
        case NS::RHI::ERHIBarrierLayout::Present:
            return D3D12_BARRIER_LAYOUT_PRESENT;
        case NS::RHI::ERHIBarrierLayout::GenericRead:
            return D3D12_BARRIER_LAYOUT_GENERIC_READ;
        case NS::RHI::ERHIBarrierLayout::RenderTarget:
            return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
        case NS::RHI::ERHIBarrierLayout::UnorderedAccess:
            return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
        case NS::RHI::ERHIBarrierLayout::DepthStencilWrite:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
        case NS::RHI::ERHIBarrierLayout::DepthStencilRead:
            return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
        case NS::RHI::ERHIBarrierLayout::ShaderResource:
            return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
        case NS::RHI::ERHIBarrierLayout::CopySource:
            return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
        case NS::RHI::ERHIBarrierLayout::CopyDest:
            return D3D12_BARRIER_LAYOUT_COPY_DEST;
        case NS::RHI::ERHIBarrierLayout::ResolveSource:
            return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
        case NS::RHI::ERHIBarrierLayout::ResolveDest:
            return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
        case NS::RHI::ERHIBarrierLayout::ShadingRate:
            return D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
        case NS::RHI::ERHIBarrierLayout::DirectQueueCommon:
            return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON;
        case NS::RHI::ERHIBarrierLayout::DirectQueueGenericRead:
            return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ;
        case NS::RHI::ERHIBarrierLayout::DirectQueueUnorderedAccess:
            return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS;
        case NS::RHI::ERHIBarrierLayout::DirectQueueShaderResource:
            return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
        case NS::RHI::ERHIBarrierLayout::DirectQueueCopySource:
            return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE;
        case NS::RHI::ERHIBarrierLayout::DirectQueueCopyDest:
            return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST;
        case NS::RHI::ERHIBarrierLayout::ComputeQueueCommon:
            return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON;
        case NS::RHI::ERHIBarrierLayout::ComputeQueueGenericRead:
            return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ;
        case NS::RHI::ERHIBarrierLayout::ComputeQueueUnorderedAccess:
            return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS;
        case NS::RHI::ERHIBarrierLayout::ComputeQueueShaderResource:
            return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE;
        case NS::RHI::ERHIBarrierLayout::ComputeQueueCopySource:
            return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE;
        case NS::RHI::ERHIBarrierLayout::ComputeQueueCopyDest:
            return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST;
        default:
            return D3D12_BARRIER_LAYOUT_COMMON;
        }
    }

    //=========================================================================
    // D3D12EnhancedBarrierBatcher
    //=========================================================================

    void D3D12EnhancedBarrierBatcher::AddGlobal(D3D12_BARRIER_SYNC syncBefore,
                                                D3D12_BARRIER_SYNC syncAfter,
                                                D3D12_BARRIER_ACCESS accessBefore,
                                                D3D12_BARRIER_ACCESS accessAfter)
    {
        if (globalCount_ >= kMaxBarriers)
        {
            FlushIfFull();
        }
        if (globalCount_ >= kMaxBarriers)
        {
            return;
        }

        auto& b = globalBarriers_[globalCount_++];
        b.SyncBefore = syncBefore;
        b.SyncAfter = syncAfter;
        b.AccessBefore = accessBefore;
        b.AccessAfter = accessAfter;
    }

    void D3D12EnhancedBarrierBatcher::AddTexture(ID3D12Resource* resource,
                                                 D3D12_BARRIER_SYNC syncBefore,
                                                 D3D12_BARRIER_SYNC syncAfter,
                                                 D3D12_BARRIER_ACCESS accessBefore,
                                                 D3D12_BARRIER_ACCESS accessAfter,
                                                 D3D12_BARRIER_LAYOUT layoutBefore,
                                                 D3D12_BARRIER_LAYOUT layoutAfter,
                                                 uint32 subresource,
                                                 D3D12_TEXTURE_BARRIER_FLAGS flags)
    {
        if (textureCount_ >= kMaxBarriers)
        {
            FlushIfFull();
        }
        if (textureCount_ >= kMaxBarriers)
        {
            return;
        }

        auto& b = textureBarriers_[textureCount_++];
        b.SyncBefore = syncBefore;
        b.SyncAfter = syncAfter;
        b.AccessBefore = accessBefore;
        b.AccessAfter = accessAfter;
        b.LayoutBefore = layoutBefore;
        b.LayoutAfter = layoutAfter;
        b.pResource = resource;
        b.Subresources.IndexOrFirstMipLevel = subresource;
        b.Subresources.NumMipLevels = 0;
        b.Subresources.FirstArraySlice = 0;
        b.Subresources.NumArraySlices = 0;
        b.Subresources.FirstPlane = 0;
        b.Subresources.NumPlanes = 0;
        b.Flags = flags;
    }

    void D3D12EnhancedBarrierBatcher::AddBuffer(ID3D12Resource* resource,
                                                D3D12_BARRIER_SYNC syncBefore,
                                                D3D12_BARRIER_SYNC syncAfter,
                                                D3D12_BARRIER_ACCESS accessBefore,
                                                D3D12_BARRIER_ACCESS accessAfter,
                                                uint64 offset,
                                                uint64 size)
    {
        if (bufferCount_ >= kMaxBarriers)
        {
            FlushIfFull();
        }
        if (bufferCount_ >= kMaxBarriers)
        {
            return;
        }

        auto& b = bufferBarriers_[bufferCount_++];
        b.SyncBefore = syncBefore;
        b.SyncAfter = syncAfter;
        b.AccessBefore = accessBefore;
        b.AccessAfter = accessAfter;
        b.pResource = resource;
        b.Offset = offset;
        b.Size = size;
    }

    void D3D12EnhancedBarrierBatcher::AddFromRHI(const NS::RHI::RHIEnhancedBarrierDesc& desc)
    {
        D3D12_BARRIER_SYNC syncBefore = ConvertBarrierSync(desc.syncBefore);
        D3D12_BARRIER_SYNC syncAfter = ConvertBarrierSync(desc.syncAfter);
        D3D12_BARRIER_ACCESS accessBefore = ConvertBarrierAccess(desc.accessBefore);
        D3D12_BARRIER_ACCESS accessAfter = ConvertBarrierAccess(desc.accessAfter);

        if (!desc.resource)
        {
            // グローバルバリア（リソース未指定）
            AddGlobal(syncBefore, syncAfter, accessBefore, accessAfter);
            return;
        }

        auto resourceType = desc.resource->GetResourceType();
        auto* d3dRes = GetD3D12Resource(desc.resource);
        if (!d3dRes)
        {
            return;
        }

        if (resourceType == NS::RHI::ERHIResourceType::Texture)
        {
            // テクスチャバリア
            D3D12_BARRIER_LAYOUT layoutBefore = ConvertBarrierLayout(desc.layoutBefore);
            D3D12_BARRIER_LAYOUT layoutAfter = ConvertBarrierLayout(desc.layoutAfter);

            if (textureCount_ >= kMaxBarriers)
            {
                FlushIfFull();
            }
            if (textureCount_ >= kMaxBarriers)
            {
                return;
            }

            auto& b = textureBarriers_[textureCount_++];
            b.SyncBefore = syncBefore;
            b.SyncAfter = syncAfter;
            b.AccessBefore = accessBefore;
            b.AccessAfter = accessAfter;
            b.LayoutBefore = layoutBefore;
            b.LayoutAfter = layoutAfter;
            b.pResource = d3dRes;
            b.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;

            // サブリソース範囲変換
            const auto& sr = desc.subresources;
            if (sr.levelCount == 0 && sr.layerCount == 0)
            {
                // All subresources
                b.Subresources.IndexOrFirstMipLevel = 0;
                b.Subresources.NumMipLevels = 0xFFFFFFFF;
                b.Subresources.FirstArraySlice = 0;
                b.Subresources.NumArraySlices = 0xFFFFFFFF;
                b.Subresources.FirstPlane = 0;
                b.Subresources.NumPlanes = 0xFFFFFFFF;
            }
            else
            {
                // Subresource range
                b.Subresources.IndexOrFirstMipLevel = sr.baseMipLevel;
                b.Subresources.NumMipLevels = sr.levelCount;
                b.Subresources.FirstArraySlice = sr.baseArrayLayer;
                b.Subresources.NumArraySlices = sr.layerCount;
                b.Subresources.FirstPlane = sr.planeSlice;
                b.Subresources.NumPlanes = 1;
            }
        }
        else
        {
            // バッファバリア
            AddBuffer(d3dRes, syncBefore, syncAfter, accessBefore, accessAfter);
        }
    }

    void D3D12EnhancedBarrierBatcher::Flush(ID3D12GraphicsCommandList7* cmdList)
    {
        if (!cmdList || IsEmpty())
        {
            return;
        }

        D3D12_BARRIER_GROUP groups[3] = {};
        uint32 groupCount = 0;

        if (globalCount_ > 0)
        {
            auto& g = groups[groupCount++];
            g.Type = D3D12_BARRIER_TYPE_GLOBAL;
            g.NumBarriers = globalCount_;
            g.pGlobalBarriers = globalBarriers_;
        }

        if (textureCount_ > 0)
        {
            auto& g = groups[groupCount++];
            g.Type = D3D12_BARRIER_TYPE_TEXTURE;
            g.NumBarriers = textureCount_;
            g.pTextureBarriers = textureBarriers_;
        }

        if (bufferCount_ > 0)
        {
            auto& g = groups[groupCount++];
            g.Type = D3D12_BARRIER_TYPE_BUFFER;
            g.NumBarriers = bufferCount_;
            g.pBufferBarriers = bufferBarriers_;
        }

        cmdList->Barrier(groupCount, groups);
        Reset();
    }

    void D3D12EnhancedBarrierBatcher::Reset()
    {
        globalCount_ = 0;
        textureCount_ = 0;
        bufferCount_ = 0;
    }

} // namespace NS::D3D12RHI

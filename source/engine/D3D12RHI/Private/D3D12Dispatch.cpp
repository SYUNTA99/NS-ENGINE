/// @file D3D12Dispatch.cpp
/// @brief D3D12 DispatchTable関数群 — Phase 1コアエントリ
#include "D3D12Dispatch.h"
#include "D3D12AccelerationStructure.h"
#include "D3D12Barriers.h"
#include "D3D12Bindless.h"
#include "D3D12Buffer.h"
#include "D3D12CommandContext.h"
#include "D3D12CommandList.h"
#include "D3D12Descriptors.h"
#include "D3D12Device.h"
#include "D3D12MeshShader.h"
#include "D3D12PipelineState.h"
#include "D3D12Query.h"
#include "D3D12RHIPrivate.h"
#include "D3D12RayTracingPSO.h"
#include "D3D12Resource.h"
#include "D3D12RootSignature.h"
#include "D3D12Texture.h"
#include "D3D12Upload.h"
#include "D3D12View.h"
#include "D3D12WorkGraph.h"
#include "RHI/Public/IRHIUploadContext.h"
#include "RHI/Public/RHIBarrier.h"

namespace NS::D3D12RHI
{
    // ヘルパー: コンテキストからID3D12GraphicsCommandListを取得
    // QueueTypeでGraphics/Computeを判別し、dynamic_castを回避
    static ID3D12GraphicsCommandList* GetCmdList(NS::RHI::IRHICommandContextBase* ctx)
    {
        if (!ctx)
        {
            return nullptr;
        }
        if (ctx->GetQueueType() == NS::RHI::ERHIQueueType::Compute)
        {
            return static_cast<D3D12ComputeContext*>(static_cast<NS::RHI::IRHIComputeContext*>(ctx))->GetD3DCommandList();
        }
        return static_cast<D3D12CommandContext*>(static_cast<NS::RHI::IRHICommandContext*>(ctx))->GetD3DCommandList();
    }

    // ヘルパー: コンテキストからバリアバッチャーを取得
    static D3D12BarrierBatcher* GetBatcher(NS::RHI::IRHICommandContextBase* ctx)
    {
        if (!ctx)
        {
            return nullptr;
        }
        if (ctx->GetQueueType() == NS::RHI::ERHIQueueType::Compute)
        {
            return &static_cast<D3D12ComputeContext*>(static_cast<NS::RHI::IRHIComputeContext*>(ctx))->GetBarrierBatcher();
        }
        return &static_cast<D3D12CommandContext*>(static_cast<NS::RHI::IRHICommandContext*>(ctx))->GetBarrierBatcher();
    }

    // ヘルパー: コンテキストのバリアをフラッシュ
    static void FlushContextBarriers(NS::RHI::IRHICommandContextBase* ctx)
    {
        if (!ctx)
        {
            return;
        }
        if (ctx->GetQueueType() == NS::RHI::ERHIQueueType::Compute)
        {
            static_cast<D3D12ComputeContext*>(static_cast<NS::RHI::IRHIComputeContext*>(ctx))->FlushBarriers();
        } else {
            static_cast<D3D12CommandContext*>(static_cast<NS::RHI::IRHICommandContext*>(ctx))->FlushBarriers();
        }
    }

    //=========================================================================
    // Base: プロパティ
    //=========================================================================

    static NS::RHI::IRHIDevice* D3D12_GetDevice(NS::RHI::IRHICommandContextBase* ctx)
    {
        return ctx->GetDevice();
    }

    static NS::RHI::GPUMask D3D12_GetGPUMask(NS::RHI::IRHICommandContextBase* ctx)
    {
        return ctx->GetGPUMask();
    }

    static NS::RHI::ERHIQueueType D3D12_GetQueueType(NS::RHI::IRHICommandContextBase* ctx)
    {
        return ctx->GetQueueType();
    }

    static NS::RHI::ERHIPipeline D3D12_GetPipeline(NS::RHI::IRHICommandContextBase* ctx)
    {
        return ctx->GetPipeline();
    }

    //=========================================================================
    // Base: ライフサイクル
    //=========================================================================

    static void D3D12_Begin(NS::RHI::IRHICommandContextBase* ctx, NS::RHI::IRHICommandAllocator* allocator)
    {
        ctx->Begin(allocator);
    }

    static NS::RHI::IRHICommandList* D3D12_Finish(NS::RHI::IRHICommandContextBase* ctx)
    {
        return ctx->Finish();
    }

    static void D3D12_Reset(NS::RHI::IRHICommandContextBase* ctx)
    {
        ctx->Reset();
    }

    static bool D3D12_IsRecording(NS::RHI::IRHICommandContextBase* ctx)
    {
        return ctx->IsRecording();
    }

    //=========================================================================
    // Base: バリア（Legacy Barrier — サブ計画29）
    //=========================================================================

    static void D3D12_TransitionResource(NS::RHI::IRHICommandContextBase* ctx,
                                         NS::RHI::IRHIResource* resource,
                                         NS::RHI::ERHIAccess /*before*/,
                                         NS::RHI::ERHIAccess /*after*/)
    {
        // ERHIAccess版はEnhanced Barrier（サブ計画30）で実装
        (void)ctx;
        (void)resource;
    }

    static void D3D12_UAVBarrier(NS::RHI::IRHICommandContextBase* ctx, NS::RHI::IRHIResource* resource)
    {
        auto* batcher = GetBatcher(ctx);
        if (!batcher)
        {
            return;
        }

        batcher->AddUAV(GetD3D12Resource(resource));

        // 上限到達時は自動フラッシュ
        if (batcher->GetPendingCount() >= D3D12BarrierBatcher::kMaxBatchedBarriers)
        {
            FlushContextBarriers(ctx);
        }
    }

    static void D3D12_AliasingBarrier(NS::RHI::IRHICommandContextBase* ctx,
                                      NS::RHI::IRHIResource* before,
                                      NS::RHI::IRHIResource* after)
    {
        auto* batcher = GetBatcher(ctx);
        if (!batcher)
        {
            return;
        }

        batcher->AddAliasing(GetD3D12Resource(before), GetD3D12Resource(after));

        if (batcher->GetPendingCount() >= D3D12BarrierBatcher::kMaxBatchedBarriers)
        {
            FlushContextBarriers(ctx);
        }
    }

    static void D3D12_FlushBarriers(NS::RHI::IRHICommandContextBase* ctx)
    {
        FlushContextBarriers(ctx);
    }

    //=========================================================================
    // Base: コピー（スタブ — サブ計画16-18で実装）
    //=========================================================================

    static void D3D12_CopyBuffer(NS::RHI::IRHICommandContextBase* ctx,
                                 NS::RHI::IRHIBuffer* dst,
                                 NS::RHI::IRHIBuffer* src)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !src)
        {
            return;
        }
        auto* d3dDst = static_cast<D3D12Buffer*>(dst)->GetD3DResource();
        auto* d3dSrc = static_cast<D3D12Buffer*>(src)->GetD3DResource();
        cmdList->CopyResource(d3dDst, d3dSrc);
    }

    static void D3D12_CopyBufferRegion(NS::RHI::IRHICommandContextBase* ctx,
                                       NS::RHI::IRHIBuffer* dst,
                                       uint64 dstOffset,
                                       NS::RHI::IRHIBuffer* src,
                                       uint64 srcOffset,
                                       uint64 size)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !src)
        {
            return;
        }
        auto* d3dDst = static_cast<D3D12Buffer*>(dst)->GetD3DResource();
        auto* d3dSrc = static_cast<D3D12Buffer*>(src)->GetD3DResource();
        cmdList->CopyBufferRegion(d3dDst, dstOffset, d3dSrc, srcOffset, size);
    }

    static void D3D12_CopyTexture(NS::RHI::IRHICommandContextBase* ctx,
                                  NS::RHI::IRHITexture* dst,
                                  NS::RHI::IRHITexture* src)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !src)
        {
            return;
        }
        auto* d3dDst = static_cast<D3D12Texture*>(dst)->GetD3DResource();
        auto* d3dSrc = static_cast<D3D12Texture*>(src)->GetD3DResource();
        cmdList->CopyResource(d3dDst, d3dSrc);
    }

    static void D3D12_CopyTextureRegion(NS::RHI::IRHICommandContextBase* ctx,
                                        NS::RHI::IRHITexture* dst,
                                        uint32 dstMip,
                                        uint32 dstSlice,
                                        NS::RHI::Offset3D dstOffset,
                                        NS::RHI::IRHITexture* src,
                                        uint32 srcMip,
                                        uint32 srcSlice,
                                        const NS::RHI::RHIBox* srcBox)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !src)
        {
            return;
        }

        auto* dstTex = static_cast<D3D12Texture*>(dst);
        auto* srcTex = static_cast<D3D12Texture*>(src);

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dstTex->GetD3DResource();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = dstMip + dstSlice * dst->GetMipLevels();

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = srcTex->GetD3DResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = srcMip + srcSlice * src->GetMipLevels();

        const D3D12_BOX* pSrcBox = nullptr;
        D3D12_BOX d3dBox{};
        if (srcBox)
        {
            d3dBox.left = srcBox->left;
            d3dBox.top = srcBox->top;
            d3dBox.front = srcBox->front;
            d3dBox.right = srcBox->right;
            d3dBox.bottom = srcBox->bottom;
            d3dBox.back = srcBox->back;
            pSrcBox = &d3dBox;
        }

        cmdList->CopyTextureRegion(&dstLoc,
                                   static_cast<UINT>(dstOffset.x),
                                   static_cast<UINT>(dstOffset.y),
                                   static_cast<UINT>(dstOffset.z),
                                   &srcLoc,
                                   pSrcBox);
    }

    static void D3D12_CopyBufferToTexture(NS::RHI::IRHICommandContextBase* ctx,
                                          NS::RHI::IRHITexture* dst,
                                          uint32 dstMip,
                                          uint32 dstSlice,
                                          NS::RHI::Offset3D dstOffset,
                                          NS::RHI::IRHIBuffer* src,
                                          uint64 srcOffset,
                                          uint32 srcRowPitch,
                                          uint32 /*srcDepthPitch*/)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !src)
        {
            return;
        }

        auto* dstTex = static_cast<D3D12Texture*>(dst);
        auto* srcBuf = static_cast<D3D12Buffer*>(src);

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dstTex->GetD3DResource();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = dstMip + dstSlice * dst->GetMipLevels();

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = srcBuf->GetD3DResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = srcOffset;
        srcLoc.PlacedFootprint.Footprint.Format = D3D12Texture::ConvertPixelFormat(dst->GetFormat());
        srcLoc.PlacedFootprint.Footprint.Width = dst->GetWidth() >> dstMip;
        srcLoc.PlacedFootprint.Footprint.Height = dst->GetHeight() >> dstMip;
        srcLoc.PlacedFootprint.Footprint.Depth = 1;
        srcLoc.PlacedFootprint.Footprint.RowPitch = srcRowPitch;
        if (srcLoc.PlacedFootprint.Footprint.Width == 0)
        {
            srcLoc.PlacedFootprint.Footprint.Width = 1;
        }
        if (srcLoc.PlacedFootprint.Footprint.Height == 0)
        {
            srcLoc.PlacedFootprint.Footprint.Height = 1;
        }

        cmdList->CopyTextureRegion(&dstLoc,
                                   static_cast<UINT>(dstOffset.x),
                                   static_cast<UINT>(dstOffset.y),
                                   static_cast<UINT>(dstOffset.z),
                                   &srcLoc,
                                   nullptr);
    }

    static void D3D12_CopyTextureToBuffer(NS::RHI::IRHICommandContextBase* ctx,
                                          NS::RHI::IRHIBuffer* dst,
                                          uint64 dstOffset,
                                          uint32 dstRowPitch,
                                          uint32 /*dstDepthPitch*/,
                                          NS::RHI::IRHITexture* src,
                                          uint32 srcMip,
                                          uint32 srcSlice,
                                          const NS::RHI::RHIBox* srcBox)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !src)
        {
            return;
        }

        auto* dstBuf = static_cast<D3D12Buffer*>(dst);
        auto* srcTex = static_cast<D3D12Texture*>(src);

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dstBuf->GetD3DResource();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLoc.PlacedFootprint.Offset = dstOffset;
        dstLoc.PlacedFootprint.Footprint.Format = D3D12Texture::ConvertPixelFormat(src->GetFormat());
        dstLoc.PlacedFootprint.Footprint.Width = src->GetWidth() >> srcMip;
        dstLoc.PlacedFootprint.Footprint.Height = src->GetHeight() >> srcMip;
        dstLoc.PlacedFootprint.Footprint.Depth = 1;
        dstLoc.PlacedFootprint.Footprint.RowPitch = dstRowPitch;
        if (dstLoc.PlacedFootprint.Footprint.Width == 0)
        {
            dstLoc.PlacedFootprint.Footprint.Width = 1;
        }
        if (dstLoc.PlacedFootprint.Footprint.Height == 0)
        {
            dstLoc.PlacedFootprint.Footprint.Height = 1;
        }

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = srcTex->GetD3DResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = srcMip + srcSlice * src->GetMipLevels();

        const D3D12_BOX* pSrcBox = nullptr;
        D3D12_BOX d3dBox{};
        if (srcBox)
        {
            d3dBox.left = srcBox->left;
            d3dBox.top = srcBox->top;
            d3dBox.front = srcBox->front;
            d3dBox.right = srcBox->right;
            d3dBox.bottom = srcBox->bottom;
            d3dBox.back = srcBox->back;
            pSrcBox = &d3dBox;
        }

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, pSrcBox);
    }

    static void D3D12_CopyToStagingBuffer(NS::RHI::IRHICommandContextBase* /*ctx*/,
                                          NS::RHI::IRHIStagingBuffer* /*dst*/,
                                          uint64 /*dstOffset*/,
                                          NS::RHI::IRHIResource* /*src*/,
                                          uint64 /*srcOffset*/,
                                          uint64 /*size*/)
    {}

    static void D3D12_ResolveTexture(NS::RHI::IRHICommandContextBase* /*ctx*/,
                                     NS::RHI::IRHITexture* /*dst*/,
                                     NS::RHI::IRHITexture* /*src*/)
    {}

    static void D3D12_ResolveTextureRegion(NS::RHI::IRHICommandContextBase* /*ctx*/,
                                           NS::RHI::IRHITexture* /*dst*/,
                                           uint32 /*dstMip*/,
                                           uint32 /*dstSlice*/,
                                           NS::RHI::IRHITexture* /*src*/,
                                           uint32 /*srcMip*/,
                                           uint32 /*srcSlice*/)
    {}

    //=========================================================================
    // Base: デバッグ
    //=========================================================================

    static void D3D12_BeginDebugEvent(NS::RHI::IRHICommandContextBase* /*ctx*/, const char* /*name*/, uint32 /*color*/)
    {
        // PIX SDK統合時に実装
    }

    static void D3D12_EndDebugEvent(NS::RHI::IRHICommandContextBase* /*ctx*/) {}

    static void D3D12_InsertDebugMarker(NS::RHI::IRHICommandContextBase* /*ctx*/,
                                        const char* /*name*/,
                                        uint32 /*color*/)
    {}

    static void D3D12_InsertBreadcrumb(NS::RHI::IRHICommandContextBase* /*ctx*/, uint32 /*id*/, const char* /*message*/)
    {}

    //=========================================================================
    // ImmediateContext
    //=========================================================================

    static void D3D12_Flush(NS::RHI::IRHIImmediateContext* /*ctx*/) {}

    static void* D3D12_GetNativeContext(NS::RHI::IRHIImmediateContext* /*ctx*/)
    {
        return nullptr;
    }

    //=========================================================================
    // Compute: パイプラインステート（スタブ — サブ計画23で実装）
    //=========================================================================

    static void D3D12_SetComputePipelineState(NS::RHI::IRHIComputeContext* ctx, NS::RHI::IRHIComputePipelineState* pso)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !pso)
        {
            return;
        }
        cmdList->SetPipelineState(static_cast<D3D12ComputePipelineState*>(pso)->GetD3DPipelineState());
    }

    static void D3D12_SetComputeRootSignature(NS::RHI::IRHIComputeContext* ctx, NS::RHI::IRHIRootSignature* rootSig)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !rootSig)
        {
            return;
        }
        cmdList->SetComputeRootSignature(static_cast<D3D12RootSignature*>(rootSig)->GetD3DRootSignature());
    }

    //=========================================================================
    // Compute: ルート定数・ディスクリプタ
    //=========================================================================

    static void D3D12_SetComputeRoot32BitConstants(NS::RHI::IRHIComputeContext* ctx,
                                                   uint32 rootParameterIndex,
                                                   uint32 num32BitValues,
                                                   const void* data,
                                                   uint32 destOffset)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffset);
        }
    }

    static void D3D12_SetComputeRootCBV(NS::RHI::IRHIComputeContext* ctx,
                                        uint32 rootParameterIndex,
                                        uint64 bufferAddress)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetComputeRootConstantBufferView(rootParameterIndex, bufferAddress);
        }
    }

    static void D3D12_SetComputeRootSRV(NS::RHI::IRHIComputeContext* ctx,
                                        uint32 rootParameterIndex,
                                        uint64 bufferAddress)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetComputeRootShaderResourceView(rootParameterIndex, bufferAddress);
        }
    }

    static void D3D12_SetComputeRootUAV(NS::RHI::IRHIComputeContext* ctx,
                                        uint32 rootParameterIndex,
                                        uint64 bufferAddress)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetComputeRootUnorderedAccessView(rootParameterIndex, bufferAddress);
        }
    }

    static void D3D12_SetDescriptorHeaps(NS::RHI::IRHIComputeContext* ctx,
                                         NS::RHI::IRHIDescriptorHeap* cbvSrvUavHeap,
                                         NS::RHI::IRHIDescriptorHeap* samplerHeap)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12DescriptorHeap* heaps[2] = {};
        uint32 heapCount = 0;

        if (cbvSrvUavHeap)
        {
            auto* d3dHeap = static_cast<D3D12DescriptorHeap*>(cbvSrvUavHeap);
            heaps[heapCount++] = d3dHeap->GetD3DHeap();
        }
        if (samplerHeap)
        {
            auto* d3dHeap = static_cast<D3D12DescriptorHeap*>(samplerHeap);
            heaps[heapCount++] = d3dHeap->GetD3DHeap();
        }

        if (heapCount > 0)
        {
            cmdList->SetDescriptorHeaps(heapCount, heaps);
        }
    }

    static NS::RHI::IRHIDescriptorHeap* D3D12_GetCBVSRVUAVHeap(NS::RHI::IRHIComputeContext* ctx)
    {
        return ctx->GetCBVSRVUAVHeap();
    }

    static NS::RHI::IRHIDescriptorHeap* D3D12_GetSamplerHeap(NS::RHI::IRHIComputeContext* ctx)
    {
        return ctx->GetSamplerHeap();
    }

    static void D3D12_SetComputeRootDescriptorTable(NS::RHI::IRHIComputeContext* ctx,
                                                    uint32 rootParameterIndex,
                                                    NS::RHI::RHIGPUDescriptorHandle baseDescriptor)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle;
            handle.ptr = baseDescriptor.ptr;
            cmdList->SetComputeRootDescriptorTable(rootParameterIndex, handle);
        }
    }

    //=========================================================================
    // Compute: ディスパッチ
    //=========================================================================

    static void D3D12_Dispatch(NS::RHI::IRHIComputeContext* ctx,
                               uint32 groupCountX,
                               uint32 groupCountY,
                               uint32 groupCountZ)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);
        }
    }

    static void D3D12_DispatchIndirect(NS::RHI::IRHIComputeContext* /*ctx*/,
                                       NS::RHI::IRHIBuffer* /*argsBuffer*/,
                                       uint64 /*argsOffset*/)
    {
        // サブ計画16でIRHIBuffer→ID3D12Resource変換後に実装
    }

    static void D3D12_DispatchIndirectMulti(NS::RHI::IRHIComputeContext* /*ctx*/,
                                            NS::RHI::IRHIBuffer* /*argsBuffer*/,
                                            uint64 /*argsOffset*/,
                                            uint32 /*dispatchCount*/,
                                            uint32 /*stride*/)
    {}

    //=========================================================================
    // Compute: UAVクリア・タイムスタンプ・クエリ（スタブ）
    //=========================================================================

    static void D3D12_ClearUnorderedAccessViewUint(NS::RHI::IRHIComputeContext* /*ctx*/,
                                                   NS::RHI::IRHIUnorderedAccessView* /*uav*/,
                                                   const uint32 /*values*/[4])
    {}

    static void D3D12_ClearUnorderedAccessViewFloat(NS::RHI::IRHIComputeContext* /*ctx*/,
                                                    NS::RHI::IRHIUnorderedAccessView* /*uav*/,
                                                    const float /*values*/[4])
    {}

    static void D3D12_WriteTimestamp(NS::RHI::IRHIComputeContext* ctx,
                                     NS::RHI::IRHIQueryHeap* queryHeap,
                                     uint32 queryIndex)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !queryHeap)
        {
            return;
        }
        auto* d3dHeap = static_cast<D3D12QueryHeap*>(queryHeap);
        // Timestamp: EndQueryのみ（BeginQueryは不可）
        cmdList->EndQuery(d3dHeap->GetD3DQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
    }

    static void D3D12_BeginQuery(NS::RHI::IRHIComputeContext* ctx, NS::RHI::IRHIQueryHeap* queryHeap, uint32 queryIndex)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !queryHeap)
        {
            return;
        }
        auto* d3dHeap = static_cast<D3D12QueryHeap*>(queryHeap);
        cmdList->BeginQuery(d3dHeap->GetD3DQueryHeap(), d3dHeap->GetD3DQueryType(), queryIndex);
    }

    static void D3D12_EndQuery(NS::RHI::IRHIComputeContext* ctx, NS::RHI::IRHIQueryHeap* queryHeap, uint32 queryIndex)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !queryHeap)
        {
            return;
        }
        auto* d3dHeap = static_cast<D3D12QueryHeap*>(queryHeap);
        cmdList->EndQuery(d3dHeap->GetD3DQueryHeap(), d3dHeap->GetD3DQueryType(), queryIndex);
    }

    static void D3D12_ResolveQueryData(NS::RHI::IRHIComputeContext* ctx,
                                       NS::RHI::IRHIQueryHeap* queryHeap,
                                       uint32 startIndex,
                                       uint32 numQueries,
                                       NS::RHI::IRHIBuffer* destBuffer,
                                       uint64 destOffset)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !queryHeap)
        {
            return;
        }
        auto* d3dHeap = static_cast<D3D12QueryHeap*>(queryHeap);

        // ResolveQueryData先: ユーザー指定バッファまたはヒープ内蔵Readbackバッファ
        ID3D12Resource* destResource = nullptr;
        if (destBuffer)
        {
            auto* d3dBuffer = static_cast<D3D12Buffer*>(destBuffer);
            destResource = d3dBuffer->GetD3DResource();
        }
        else
        {
            destResource = d3dHeap->GetReadbackBuffer();
        }

        if (!destResource)
        {
            return;
        }

        cmdList->ResolveQueryData(
            d3dHeap->GetD3DQueryHeap(), d3dHeap->GetD3DQueryType(), startIndex, numQueries, destResource, destOffset);
    }

    static bool D3D12_GetQueryResult(NS::RHI::IRHIComputeContext* /*ctx*/,
                                     NS::RHI::IRHIQueryHeap* queryHeap,
                                     uint32 queryIndex,
                                     uint64* outResult,
                                     bool /*bWait*/)
    {
        if (!queryHeap || !outResult)
        {
            return false;
        }
        auto* d3dHeap = static_cast<D3D12QueryHeap*>(queryHeap);
        const void* mappedPtr = d3dHeap->GetMappedPtr();
        if (!mappedPtr)
        {
            return false;
        }

        // 永続Map済みReadbackバッファから直接読取
        uint32 resultSize = d3dHeap->GetQueryResultSize();
        const uint8* srcPtr = static_cast<const uint8*>(mappedPtr) + static_cast<uint64>(queryIndex) * resultSize;
        memcpy(outResult, srcPtr, sizeof(uint64));
        return true;
    }

    //=========================================================================
    // Graphics: パイプラインステート（スタブ — サブ計画23で実装）
    //=========================================================================

    static void D3D12_SetGraphicsPipelineState(NS::RHI::IRHICommandContext* ctx,
                                               NS::RHI::IRHIGraphicsPipelineState* pso)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !pso)
        {
            return;
        }
        cmdList->SetPipelineState(static_cast<D3D12GraphicsPipelineState*>(pso)->GetD3DPipelineState());
    }

    static void D3D12_SetGraphicsRootSignature(NS::RHI::IRHICommandContext* ctx, NS::RHI::IRHIRootSignature* rootSig)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !rootSig)
        {
            return;
        }
        cmdList->SetGraphicsRootSignature(static_cast<D3D12RootSignature*>(rootSig)->GetD3DRootSignature());
    }

    //=========================================================================
    // Graphics: レンダーターゲット（スタブ — サブ計画20で実装）
    //=========================================================================

    static void D3D12_SetRenderTargets(NS::RHI::IRHICommandContext* ctx,
                                       uint32 numRTVs,
                                       NS::RHI::IRHIRenderTargetView* const* rtvs,
                                       NS::RHI::IRHIDepthStencilView* dsv)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[8] = {};
        uint32 count = (numRTVs < 8) ? numRTVs : 8;
        for (uint32 i = 0; i < count; ++i)
        {
            if (rtvs && rtvs[i])
            {
                rtvHandles[i].ptr = rtvs[i]->GetCPUHandle().ptr;
            }
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE* pDsvHandle = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
        if (dsv)
        {
            dsvHandle.ptr = dsv->GetCPUHandle().ptr;
            pDsvHandle = &dsvHandle;
        }

        cmdList->OMSetRenderTargets(count, count > 0 ? rtvHandles : nullptr, FALSE, pDsvHandle);
    }

    //=========================================================================
    // Graphics: クリア（スタブ — サブ計画20で実装）
    //=========================================================================

    static void D3D12_ClearRenderTargetView(NS::RHI::IRHICommandContext* ctx,
                                            NS::RHI::IRHIRenderTargetView* rtv,
                                            const float color[4])
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !rtv)
        {
            return;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = rtv->GetCPUHandle().ptr;
        cmdList->ClearRenderTargetView(handle, color, 0, nullptr);
    }

    static void D3D12_ClearDepthStencilView(NS::RHI::IRHICommandContext* ctx,
                                            NS::RHI::IRHIDepthStencilView* dsv,
                                            bool clearDepth,
                                            float depth,
                                            bool clearStencil,
                                            uint8 stencil)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dsv)
        {
            return;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        handle.ptr = dsv->GetCPUHandle().ptr;
        D3D12_CLEAR_FLAGS flags = static_cast<D3D12_CLEAR_FLAGS>(0);
        if (clearDepth)
            flags = static_cast<D3D12_CLEAR_FLAGS>(flags | D3D12_CLEAR_FLAG_DEPTH);
        if (clearStencil)
            flags = static_cast<D3D12_CLEAR_FLAGS>(flags | D3D12_CLEAR_FLAG_STENCIL);
        cmdList->ClearDepthStencilView(handle, flags, depth, stencil, 0, nullptr);
    }

    //=========================================================================
    // Graphics: ビューポート・シザー
    //=========================================================================

    static void D3D12_SetViewports(NS::RHI::IRHICommandContext* ctx,
                                   uint32 count,
                                   const NS::RHI::RHIViewport* viewports)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !viewports || count == 0)
        {
            return;
        }

        // RHIViewportとD3D12_VIEWPORTは同じレイアウト
        static_assert(sizeof(NS::RHI::RHIViewport) == sizeof(D3D12_VIEWPORT),
                      "RHIViewport and D3D12_VIEWPORT must have the same size");
        cmdList->RSSetViewports(count, reinterpret_cast<const D3D12_VIEWPORT*>(viewports));
    }

    static void D3D12_SetScissorRects(NS::RHI::IRHICommandContext* ctx, uint32 count, const NS::RHI::RHIRect* rects)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !rects || count == 0)
        {
            return;
        }

        // D3D12_RECTはLONG型、RHIRectはint32型 — 同レイアウト想定
        static_assert(sizeof(NS::RHI::RHIRect) == sizeof(D3D12_RECT),
                      "RHIRect and D3D12_RECT must have the same size");
        cmdList->RSSetScissorRects(count, reinterpret_cast<const D3D12_RECT*>(rects));
    }

    //=========================================================================
    // Graphics: 頂点・インデックスバッファ
    //=========================================================================

    static void D3D12_SetVertexBuffers(NS::RHI::IRHICommandContext* ctx,
                                       uint32 startSlot,
                                       uint32 numBuffers,
                                       const NS::RHI::RHIVertexBufferView* views)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !views || numBuffers == 0)
        {
            return;
        }

        // RHIVertexBufferViewとD3D12_VERTEX_BUFFER_VIEWは同じレイアウト
        cmdList->IASetVertexBuffers(startSlot, numBuffers, reinterpret_cast<const D3D12_VERTEX_BUFFER_VIEW*>(views));
    }

    static void D3D12_SetIndexBuffer(NS::RHI::IRHICommandContext* ctx, const NS::RHI::RHIIndexBufferView* view)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        if (view)
        {
            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = view->bufferAddress;
            ibv.SizeInBytes = view->size;
            ibv.Format =
                (view->format == NS::RHI::ERHIIndexFormat::UInt16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            cmdList->IASetIndexBuffer(&ibv);
        }
        else
        {
            cmdList->IASetIndexBuffer(nullptr);
        }
    }

    static void D3D12_SetPrimitiveTopology(NS::RHI::IRHICommandContext* ctx, NS::RHI::ERHIPrimitiveTopology topology)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        D3D12_PRIMITIVE_TOPOLOGY d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        switch (topology)
        {
        case NS::RHI::ERHIPrimitiveTopology::PointList:
            d3dTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case NS::RHI::ERHIPrimitiveTopology::LineList:
            d3dTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case NS::RHI::ERHIPrimitiveTopology::LineStrip:
            d3dTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case NS::RHI::ERHIPrimitiveTopology::TriangleList:
            d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case NS::RHI::ERHIPrimitiveTopology::TriangleStrip:
            d3dTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        default:
            break;
        }
        cmdList->IASetPrimitiveTopology(d3dTopology);
    }

    //=========================================================================
    // Graphics: 描画
    //=========================================================================

    static void D3D12_Draw(NS::RHI::IRHICommandContext* ctx,
                           uint32 vertexCount,
                           uint32 instanceCount,
                           uint32 startVertex,
                           uint32 startInstance)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
        }
    }

    static void D3D12_DrawIndexed(NS::RHI::IRHICommandContext* ctx,
                                  uint32 indexCount,
                                  uint32 instanceCount,
                                  uint32 startIndex,
                                  int32 baseVertex,
                                  uint32 startInstance)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
        }
    }

    //=========================================================================
    // Graphics: 間接描画（スタブ — サブ計画16で実装）
    //=========================================================================

    static void D3D12_DrawIndirect(NS::RHI::IRHICommandContext* /*ctx*/,
                                   NS::RHI::IRHIBuffer* /*argsBuffer*/,
                                   uint64 /*argsOffset*/)
    {}

    static void D3D12_DrawIndexedIndirect(NS::RHI::IRHICommandContext* /*ctx*/,
                                          NS::RHI::IRHIBuffer* /*argsBuffer*/,
                                          uint64 /*argsOffset*/)
    {}

    static void D3D12_MultiDrawIndirect(NS::RHI::IRHICommandContext* /*ctx*/,
                                        NS::RHI::IRHIBuffer* /*argsBuffer*/,
                                        uint32 /*drawCount*/,
                                        uint64 /*argsOffset*/,
                                        uint32 /*argsStride*/)
    {}

    static void D3D12_MultiDrawIndirectCount(NS::RHI::IRHICommandContext* /*ctx*/,
                                             NS::RHI::IRHIBuffer* /*argsBuffer*/,
                                             uint64 /*argsOffset*/,
                                             NS::RHI::IRHIBuffer* /*countBuffer*/,
                                             uint64 /*countOffset*/,
                                             uint32 /*maxDrawCount*/,
                                             uint32 /*argsStride*/)
    {}

    //=========================================================================
    // Graphics: ルート定数・ディスクリプタ
    //=========================================================================

    static void D3D12_SetGraphicsRootDescriptorTable(NS::RHI::IRHICommandContext* ctx,
                                                     uint32 rootParameterIndex,
                                                     NS::RHI::RHIGPUDescriptorHandle baseDescriptor)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle;
            handle.ptr = baseDescriptor.ptr;
            cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, handle);
        }
    }

    static void D3D12_SetGraphicsRootCBV(NS::RHI::IRHICommandContext* ctx,
                                         uint32 rootParameterIndex,
                                         uint64 bufferLocation)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, bufferLocation);
        }
    }

    static void D3D12_SetGraphicsRootSRV(NS::RHI::IRHICommandContext* ctx,
                                         uint32 rootParameterIndex,
                                         uint64 bufferLocation)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetGraphicsRootShaderResourceView(rootParameterIndex, bufferLocation);
        }
    }

    static void D3D12_SetGraphicsRootUAV(NS::RHI::IRHICommandContext* ctx,
                                         uint32 rootParameterIndex,
                                         uint64 bufferLocation)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, bufferLocation);
        }
    }

    static void D3D12_SetGraphicsRoot32BitConstants(NS::RHI::IRHICommandContext* ctx,
                                                    uint32 rootParameterIndex,
                                                    uint32 num32BitValues,
                                                    const void* data,
                                                    uint32 destOffset)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffset);
        }
    }

    //=========================================================================
    // Graphics: ブレンド・ステンシル
    //=========================================================================

    static void D3D12_SetBlendFactor(NS::RHI::IRHICommandContext* ctx, const float factor[4])
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->OMSetBlendFactor(factor);
        }
    }

    static void D3D12_SetStencilRef(NS::RHI::IRHICommandContext* ctx, uint32 refValue)
    {
        auto* cmdList = GetCmdList(ctx);
        if (cmdList)
        {
            cmdList->OMSetStencilRef(refValue);
        }
    }

    static void D3D12_SetLineWidth(NS::RHI::IRHICommandContext* /*ctx*/, float /*width*/)
    {
        // D3D12ではライン幅1.0f固定
    }

    //=========================================================================
    // Graphics: その他（スタブ）
    //=========================================================================

    static void D3D12_SetDepthBounds(NS::RHI::IRHICommandContext* /*ctx*/, float /*minDepth*/, float /*maxDepth*/) {}

    static void D3D12_SetPredication(NS::RHI::IRHICommandContext* ctx,
                                     NS::RHI::IRHIBuffer* buffer,
                                     uint64 offset,
                                     NS::RHI::ERHIPredicationOp operation)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12Resource* d3dResource = nullptr;
        if (buffer)
        {
            auto* d3dBuffer = static_cast<D3D12Buffer*>(buffer);
            d3dResource = d3dBuffer->GetD3DResource();
        }

        D3D12_PREDICATION_OP d3dOp = (operation == NS::RHI::ERHIPredicationOp::EqualZero)
                                         ? D3D12_PREDICATION_OP_EQUAL_ZERO
                                         : D3D12_PREDICATION_OP_NOT_EQUAL_ZERO;

        cmdList->SetPredication(d3dResource, offset, d3dOp);
    }

    static void D3D12_ExecuteIndirect(NS::RHI::IRHICommandContext* /*ctx*/,
                                      NS::RHI::IRHICommandSignature* /*sig*/,
                                      uint32 /*maxCmdCount*/,
                                      NS::RHI::IRHIBuffer* /*argBuffer*/,
                                      uint64 /*argOffset*/,
                                      NS::RHI::IRHIBuffer* /*countBuffer*/,
                                      uint64 /*countOffset*/)
    {}

    static void D3D12_BeginBreadcrumbGPU(NS::RHI::IRHICommandContext* /*ctx*/, NS::RHI::RHIBreadcrumbNode* /*node*/) {}

    static void D3D12_EndBreadcrumbGPU(NS::RHI::IRHICommandContext* /*ctx*/, NS::RHI::RHIBreadcrumbNode* /*node*/) {}

    //=========================================================================
    // Graphics: レイトレーシング
    //=========================================================================

    static void D3D12_BuildRaytracingAccelerationStructure(NS::RHI::IRHICommandContext* ctx,
                                                           const NS::RHI::RHIAccelerationStructureBuildDesc* desc)
    {
        if (!desc || !desc->dest)
        {
            return;
        }

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        // ID3D12GraphicsCommandList4が必要
        ID3D12GraphicsCommandList4* cmdList4 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4))))
        {
            return;
        }

        // ビルド入力の変換
        constexpr uint32 kMaxGeometries = 64;
        D3D12_RAYTRACING_GEOMETRY_DESC geometryDescs[kMaxGeometries];
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS d3dInputs{};
        ConvertBuildInputs(desc->inputs, d3dInputs, geometryDescs, kMaxGeometries);

        // ビルド記述の構築
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3dDesc{};
        d3dDesc.Inputs = d3dInputs;
        d3dDesc.DestAccelerationStructureData =
            static_cast<D3D12AccelerationStructure*>(desc->dest)->GetGPUVirtualAddress();
        d3dDesc.ScratchAccelerationStructureData = desc->scratchBufferAddress;

        if (desc->source)
        {
            d3dDesc.SourceAccelerationStructureData =
                static_cast<D3D12AccelerationStructure*>(desc->source)->GetGPUVirtualAddress();
        }

        cmdList4->BuildRaytracingAccelerationStructure(&d3dDesc, 0, nullptr);
        cmdList4->Release();
    }

    static void D3D12_CopyRaytracingAccelerationStructure(NS::RHI::IRHICommandContext* ctx,
                                                          NS::RHI::IRHIAccelerationStructure* dest,
                                                          NS::RHI::IRHIAccelerationStructure* source,
                                                          NS::RHI::ERHIRaytracingCopyMode mode)
    {
        if (!dest || !source)
        {
            return;
        }

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList4* cmdList4 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4))))
        {
            return;
        }

        auto* d3dDest = static_cast<D3D12AccelerationStructure*>(dest);
        auto* d3dSrc = static_cast<D3D12AccelerationStructure*>(source);

        cmdList4->CopyRaytracingAccelerationStructure(
            d3dDest->GetGPUVirtualAddress(), d3dSrc->GetGPUVirtualAddress(), ConvertCopyMode(mode));
        cmdList4->Release();
    }

    static void D3D12_SetRaytracingPipelineState(NS::RHI::IRHICommandContext* ctx,
                                                 NS::RHI::IRHIRaytracingPipelineState* pso)
    {
        if (!pso)
        {
            return;
        }

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList4* cmdList4 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4))))
        {
            return;
        }

        auto* d3dPSO = static_cast<D3D12RaytracingPipelineState*>(pso);
        cmdList4->SetPipelineState1(d3dPSO->GetStateObject());
        cmdList4->Release();
    }

    static void D3D12_DispatchRays(NS::RHI::IRHICommandContext* ctx, const NS::RHI::RHIDispatchRaysDesc* desc)
    {
        if (!desc)
        {
            return;
        }

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList4* cmdList4 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4))))
        {
            return;
        }

        D3D12_DISPATCH_RAYS_DESC d3dDesc{};
        d3dDesc.RayGenerationShaderRecord.StartAddress = desc->rayGenShaderTable.startAddress;
        d3dDesc.RayGenerationShaderRecord.SizeInBytes = desc->rayGenShaderTable.size;
        d3dDesc.MissShaderTable.StartAddress = desc->missShaderTable.startAddress;
        d3dDesc.MissShaderTable.SizeInBytes = desc->missShaderTable.size;
        d3dDesc.MissShaderTable.StrideInBytes = desc->missShaderTable.stride;
        d3dDesc.HitGroupTable.StartAddress = desc->hitGroupTable.startAddress;
        d3dDesc.HitGroupTable.SizeInBytes = desc->hitGroupTable.size;
        d3dDesc.HitGroupTable.StrideInBytes = desc->hitGroupTable.stride;
        d3dDesc.CallableShaderTable.StartAddress = desc->callableShaderTable.startAddress;
        d3dDesc.CallableShaderTable.SizeInBytes = desc->callableShaderTable.size;
        d3dDesc.CallableShaderTable.StrideInBytes = desc->callableShaderTable.stride;
        d3dDesc.Width = desc->width;
        d3dDesc.Height = desc->height;
        d3dDesc.Depth = desc->depth;

        cmdList4->DispatchRays(&d3dDesc);
        cmdList4->Release();
    }

    //=========================================================================
    // Graphics: ワークグラフ
    //=========================================================================

    static void D3D12_SetWorkGraphPipeline(NS::RHI::IRHICommandContext* ctx, NS::RHI::IRHIWorkGraphPipeline* pipeline)
    {
        if (!pipeline)
        {
            return;
        }

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList10* cmdList10 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList10))))
        {
            return;
        }

        auto* wg = static_cast<D3D12WorkGraphPipeline*>(pipeline);
        cmdList10->SetPipelineState1(wg->GetStateObject());
        cmdList10->Release();
    }

    static void D3D12_DispatchGraph(NS::RHI::IRHICommandContext* ctx, const NS::RHI::RHIWorkGraphDispatchDesc* desc)
    {
#ifdef D3D12_STATE_OBJECT_TYPE_EXECUTABLE
        if (!desc || !desc->pipeline)
            return;

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
            return;

        ID3D12GraphicsCommandList10* cmdList10 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList10))))
            return;

        auto* wg = static_cast<D3D12WorkGraphPipeline*>(desc->pipeline);

        D3D12_SET_PROGRAM_DESC programDesc{};
        programDesc.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
        programDesc.WorkGraph.ProgramIdentifier.OpaqueData[0] = wg->GetProgramIdentifier();
        programDesc.WorkGraph.ProgramIdentifier.OpaqueData[1] = 0;

        // バッキングメモリ設定
        if (desc->backingMemory.buffer)
        {
            auto* backingBuf = static_cast<D3D12Buffer*>(desc->backingMemory.buffer);
            programDesc.WorkGraph.BackingMemory.StartAddress =
                backingBuf->GetGPUVirtualAddress() + desc->backingMemory.offset;
            programDesc.WorkGraph.BackingMemory.SizeInBytes = desc->backingMemory.size;
        }

        programDesc.WorkGraph.Flags = (desc->mode == NS::RHI::ERHIWorkGraphDispatchMode::Initialize)
                                          ? D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE
                                          : D3D12_SET_WORK_GRAPH_FLAG_NONE;

        cmdList10->SetProgram(&programDesc);

        // ディスパッチ
        D3D12_DISPATCH_GRAPH_DESC dispatchDesc{};
        if (desc->inputRecords.data && desc->inputRecords.count > 0)
        {
            dispatchDesc.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT;
            dispatchDesc.NodeCPUInput.EntrypointIndex = 0;
            dispatchDesc.NodeCPUInput.NumRecords = desc->inputRecords.count;
            dispatchDesc.NodeCPUInput.RecordStrideInBytes = desc->inputRecords.sizeInBytes;
            dispatchDesc.NodeCPUInput.pRecords = desc->inputRecords.data;
        }
        else
        {
            dispatchDesc.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT;
            dispatchDesc.NodeCPUInput.EntrypointIndex = 0;
            dispatchDesc.NodeCPUInput.NumRecords = 0;
            dispatchDesc.NodeCPUInput.RecordStrideInBytes = 0;
            dispatchDesc.NodeCPUInput.pRecords = nullptr;
        }

        cmdList10->DispatchGraph(&dispatchDesc);
        cmdList10->Release();
#else
        (void)ctx;
        (void)desc;
#endif
    }

    static void D3D12_InitializeWorkGraphBackingMemory(NS::RHI::IRHICommandContext* ctx,
                                                       NS::RHI::IRHIWorkGraphPipeline* pipeline,
                                                       const NS::RHI::RHIWorkGraphBackingMemory* memory)
    {
#ifdef D3D12_STATE_OBJECT_TYPE_EXECUTABLE
        if (!pipeline || !memory || !memory->buffer)
            return;

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
            return;

        ID3D12GraphicsCommandList10* cmdList10 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList10))))
            return;

        auto* wg = static_cast<D3D12WorkGraphPipeline*>(pipeline);
        auto* backingBuf = static_cast<D3D12Buffer*>(memory->buffer);

        D3D12_SET_PROGRAM_DESC programDesc{};
        programDesc.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
        programDesc.WorkGraph.ProgramIdentifier.OpaqueData[0] = wg->GetProgramIdentifier();
        programDesc.WorkGraph.ProgramIdentifier.OpaqueData[1] = 0;
        programDesc.WorkGraph.BackingMemory.StartAddress = backingBuf->GetGPUVirtualAddress() + memory->offset;
        programDesc.WorkGraph.BackingMemory.SizeInBytes = memory->size;
        programDesc.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;

        cmdList10->SetProgram(&programDesc);
        cmdList10->Release();
#else
        (void)ctx;
        (void)pipeline;
        (void)memory;
#endif
    }

    //=========================================================================
    // Graphics: メッシュシェーダー
    //=========================================================================

    static void D3D12_SetMeshPipelineState(NS::RHI::IRHICommandContext* ctx, NS::RHI::IRHIMeshPipelineState* pso)
    {
        if (!pso)
        {
            return;
        }

        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        auto* d3dPSO = static_cast<D3D12MeshPipelineState*>(pso);
        cmdList->SetPipelineState(d3dPSO->GetD3DPipelineState());
    }

    static void D3D12_DispatchMesh(NS::RHI::IRHICommandContext* ctx,
                                   uint32 groupCountX,
                                   uint32 groupCountY,
                                   uint32 groupCountZ)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList6* cmdList6 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList6))))
        {
            return;
        }

        cmdList6->DispatchMesh(groupCountX, groupCountY, groupCountZ);
        cmdList6->Release();
    }

    static void D3D12_DispatchMeshIndirect(NS::RHI::IRHICommandContext* /*ctx*/,
                                           NS::RHI::IRHIBuffer* /*argumentBuffer*/,
                                           uint64 /*argumentOffset*/)
    {
        // ExecuteIndirect + D3D12_DISPATCH_MESH_ARGUMENTS — コマンドシグネチャ対応時に実装
    }

    static void D3D12_DispatchMeshIndirectCount(NS::RHI::IRHICommandContext* /*ctx*/,
                                                NS::RHI::IRHIBuffer* /*argumentBuffer*/,
                                                uint64 /*argumentOffset*/,
                                                NS::RHI::IRHIBuffer* /*countBuffer*/,
                                                uint64 /*countOffset*/,
                                                uint32 /*maxDispatchCount*/)
    {
        // ExecuteIndirect + CountBuffer — コマンドシグネチャ対応時に実装
    }

    //=========================================================================
    // Graphics: VRS (Variable Rate Shading)
    //=========================================================================

    static void D3D12_SetShadingRate(NS::RHI::IRHICommandContext* ctx,
                                     NS::RHI::ERHIShadingRate rate,
                                     const NS::RHI::ERHIVRSCombiner* combiners)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList5* cmdList5 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList5))))
        {
            return;
        }

        // D3D12_SHADING_RATE is same encoding as ERHIShadingRate
        D3D12_SHADING_RATE d3dRate = static_cast<D3D12_SHADING_RATE>(rate);

        D3D12_SHADING_RATE_COMBINER d3dCombiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT]{};
        if (combiners)
        {
            for (uint32 i = 0; i < D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT; ++i)
            {
                switch (combiners[i])
                {
                case NS::RHI::ERHIVRSCombiner::Passthrough:
                    d3dCombiners[i] = D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
                    break;
                case NS::RHI::ERHIVRSCombiner::Override:
                    d3dCombiners[i] = D3D12_SHADING_RATE_COMBINER_OVERRIDE;
                    break;
                case NS::RHI::ERHIVRSCombiner::Min:
                    d3dCombiners[i] = D3D12_SHADING_RATE_COMBINER_MIN;
                    break;
                case NS::RHI::ERHIVRSCombiner::Max:
                    d3dCombiners[i] = D3D12_SHADING_RATE_COMBINER_MAX;
                    break;
                case NS::RHI::ERHIVRSCombiner::Sum:
                    d3dCombiners[i] = D3D12_SHADING_RATE_COMBINER_SUM;
                    break;
                }
            }
        }
        else
        {
            d3dCombiners[0] = D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
            d3dCombiners[1] = D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
        }

        cmdList5->RSSetShadingRate(d3dRate, d3dCombiners);
        cmdList5->Release();
    }

    static void D3D12_SetShadingRateImage(NS::RHI::IRHICommandContext* ctx, NS::RHI::IRHITexture* vrsImage)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList)
        {
            return;
        }

        ID3D12GraphicsCommandList5* cmdList5 = nullptr;
        if (FAILED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList5))))
        {
            return;
        }

        ID3D12Resource* d3dResource = nullptr;
        if (vrsImage)
        {
            auto* d3dTexture = static_cast<D3D12Texture*>(vrsImage);
            d3dResource = d3dTexture->GetD3DResource();
        }

        cmdList5->RSSetShadingRateImage(d3dResource);
        cmdList5->Release();
    }

    //=========================================================================
    // Graphics: レンダーパス
    //=========================================================================

    static void D3D12_BeginRenderPass(NS::RHI::IRHICommandContext* /*ctx*/, const NS::RHI::RHIRenderPassDesc* /*desc*/)
    {}

    static void D3D12_EndRenderPass(NS::RHI::IRHICommandContext* /*ctx*/) {}

    static bool D3D12_IsInRenderPass(NS::RHI::IRHICommandContext* ctx)
    {
        return ctx->IsInRenderPass();
    }

    static const NS::RHI::RHIRenderPassDesc* D3D12_GetCurrentRenderPassDesc(NS::RHI::IRHICommandContext* ctx)
    {
        return ctx->GetCurrentRenderPassDesc();
    }

    static void D3D12_NextSubpass(NS::RHI::IRHICommandContext* /*ctx*/) {}

    static uint32 D3D12_GetCurrentSubpassIndex(NS::RHI::IRHICommandContext* ctx)
    {
        return ctx->GetCurrentSubpassIndex();
    }

    static bool D3D12_GetRenderPassStatistics(NS::RHI::IRHICommandContext* ctx,
                                              NS::RHI::RHIRenderPassStatistics* outStats)
    {
        if (outStats)
        {
            return ctx->GetRenderPassStatistics(*outStats);
        }
        return false;
    }

    static void D3D12_ResetStatistics(NS::RHI::IRHICommandContext* ctx)
    {
        ctx->ResetStatistics();
    }

    //=========================================================================
    // Graphics: バリア（Legacy Barrier — バッチ発行・スプリットバリア対応）
    //=========================================================================

    static void D3D12_TransitionBarrier(NS::RHI::IRHICommandContext* ctx,
                                        NS::RHI::IRHIResource* resource,
                                        NS::RHI::ERHIResourceState before,
                                        NS::RHI::ERHIResourceState after,
                                        uint32 subresource)
    {
        auto* batcher = GetBatcher(ctx);
        if (!batcher)
        {
            return;
        }

        batcher->AddTransitionFromRHI(resource, before, after, subresource);

        if (batcher->GetPendingCount() >= D3D12BarrierBatcher::kMaxBatchedBarriers)
        {
            FlushContextBarriers(ctx);
        }
    }

    static void D3D12_TransitionBarriers(NS::RHI::IRHICommandContext* ctx,
                                         const NS::RHI::RHITransitionBarrier* barriers,
                                         uint32 count)
    {
        auto* batcher = GetBatcher(ctx);
        if (!batcher || !barriers || count == 0)
        {
            return;
        }

        for (uint32 i = 0; i < count; ++i)
        {
            batcher->AddTransitionFromRHI(barriers[i].resource,
                                          barriers[i].stateBefore,
                                          barriers[i].stateAfter,
                                          barriers[i].subresource,
                                          barriers[i].flags);

            if (batcher->GetPendingCount() >= D3D12BarrierBatcher::kMaxBatchedBarriers)
            {
                FlushContextBarriers(ctx);
            }
        }
    }

    static void D3D12_UAVBarriers(NS::RHI::IRHICommandContext* ctx,
                                  const NS::RHI::RHIUAVBarrier* barriers,
                                  uint32 count)
    {
        auto* batcher = GetBatcher(ctx);
        if (!batcher || !barriers || count == 0)
        {
            return;
        }

        for (uint32 i = 0; i < count; ++i)
        {
            batcher->AddUAV(GetD3D12Resource(barriers[i].resource));
            if (batcher->GetPendingCount() >= D3D12BarrierBatcher::kMaxBatchedBarriers)
            {
                FlushContextBarriers(ctx);
            }
        }
    }

    static void D3D12_AliasingBarriers(NS::RHI::IRHICommandContext* ctx,
                                       const NS::RHI::RHIAliasingBarrier* barriers,
                                       uint32 count)
    {
        auto* batcher = GetBatcher(ctx);
        if (!batcher || !barriers || count == 0)
        {
            return;
        }

        for (uint32 i = 0; i < count; ++i)
        {
            batcher->AddAliasing(GetD3D12Resource(barriers[i].resourceBefore),
                                 GetD3D12Resource(barriers[i].resourceAfter));
            if (batcher->GetPendingCount() >= D3D12BarrierBatcher::kMaxBatchedBarriers)
            {
                FlushContextBarriers(ctx);
            }
        }
    }

    //=========================================================================
    // Graphics: Reserved Resource（スタブ）
    //=========================================================================

    static void D3D12_CommitBuffer(NS::RHI::IRHICommandContext* /*ctx*/,
                                   NS::RHI::IRHIBuffer* /*buffer*/,
                                   uint64 /*newCommitSize*/)
    {}

    static void D3D12_CommitTextureRegions(NS::RHI::IRHICommandContext* /*ctx*/,
                                           NS::RHI::IRHITexture* /*texture*/,
                                           const NS::RHI::RHITextureCommitRegion* /*regions*/,
                                           uint32 /*regionCount*/,
                                           bool /*commit*/)
    {}

    //=========================================================================
    // Upload（スタブ — サブ計画18で実装）
    //=========================================================================

    static void D3D12_UploadBuffer(NS::RHI::IRHIUploadContext* ctx,
                                   NS::RHI::IRHIBuffer* dst,
                                   uint64 dstOffset,
                                   const void* srcData,
                                   uint64 srcSize)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !srcData || srcSize == 0)
        {
            return;
        }

        auto* dstBuf = static_cast<D3D12Buffer*>(dst);
        auto* device = dstBuf->GetGpuResource().GetDevice();

        auto uploadBuffer = D3D12UploadHelper::CreateUploadBuffer(device, srcData, srcSize);
        if (!uploadBuffer)
        {
            return;
        }

        cmdList->CopyBufferRegion(dstBuf->GetD3DResource(), dstOffset, uploadBuffer.Get(), 0, srcSize);

        // コンテキストに一時バッファを保持し、GPU完了まで解放を遅延
        auto* gfxCtx = dynamic_cast<D3D12CommandContext*>(ctx);
        if (gfxCtx)
        {
            gfxCtx->DeferRelease(std::move(uploadBuffer));
        } else if (auto* compCtx = dynamic_cast<D3D12ComputeContext*>(ctx)) {
            compCtx->DeferRelease(std::move(uploadBuffer));
        }
    }

    static void D3D12_UploadTexture(NS::RHI::IRHIUploadContext* ctx,
                                    NS::RHI::IRHITexture* dst,
                                    uint32 dstMip,
                                    uint32 dstSlice,
                                    const void* srcData,
                                    uint32 srcRowPitch,
                                    uint32 /*srcDepthPitch*/)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !srcData)
        {
            return;
        }

        auto* dstTex = static_cast<D3D12Texture*>(dst);
        auto* device = dstTex->GetGpuResource().GetDevice();

        // サブリソースレイアウト取得
        auto layout = dstTex->GetSubresourceLayout(dstMip, dstSlice);

        auto uploadBuffer = D3D12UploadHelper::CreateUploadBuffer(device, srcData, layout.size);
        if (!uploadBuffer)
        {
            return;
        }

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dstTex->GetD3DResource();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = dstMip + dstSlice * dst->GetMipLevels();

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = uploadBuffer.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = 0;
        srcLoc.PlacedFootprint.Footprint.Format = D3D12Texture::ConvertPixelFormat(dst->GetFormat());
        srcLoc.PlacedFootprint.Footprint.Width = layout.width;
        srcLoc.PlacedFootprint.Footprint.Height = layout.height;
        srcLoc.PlacedFootprint.Footprint.Depth = layout.depth;
        srcLoc.PlacedFootprint.Footprint.RowPitch = srcRowPitch > 0 ? srcRowPitch : layout.rowPitch;

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        // コンテキストに一時バッファを保持し、GPU完了まで解放を遅延
        auto* gfxCtx = dynamic_cast<D3D12CommandContext*>(ctx);
        if (gfxCtx)
        {
            gfxCtx->DeferRelease(std::move(uploadBuffer));
        } else if (auto* compCtx = dynamic_cast<D3D12ComputeContext*>(ctx)) {
            compCtx->DeferRelease(std::move(uploadBuffer));
        }
    }

    static void D3D12_CopyStagingToTexture(NS::RHI::IRHIUploadContext* ctx,
                                           NS::RHI::IRHITexture* dst,
                                           uint32 dstMip,
                                           uint32 dstSlice,
                                           NS::RHI::Offset3D dstOffset,
                                           NS::RHI::IRHIBuffer* staging,
                                           uint64 stagingOffset,
                                           uint32 rowPitch,
                                           uint32 /*depthPitch*/)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !staging)
        {
            return;
        }

        auto* dstTex = static_cast<D3D12Texture*>(dst);
        auto* srcBuf = static_cast<D3D12Buffer*>(staging);

        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = dstTex->GetD3DResource();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = dstMip + dstSlice * dst->GetMipLevels();

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = srcBuf->GetD3DResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint.Offset = stagingOffset;
        srcLoc.PlacedFootprint.Footprint.Format = D3D12Texture::ConvertPixelFormat(dst->GetFormat());
        srcLoc.PlacedFootprint.Footprint.Width = dst->GetWidth() >> dstMip;
        srcLoc.PlacedFootprint.Footprint.Height = dst->GetHeight() >> dstMip;
        srcLoc.PlacedFootprint.Footprint.Depth = 1;
        srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;
        if (srcLoc.PlacedFootprint.Footprint.Width == 0)
        {
            srcLoc.PlacedFootprint.Footprint.Width = 1;
        }
        if (srcLoc.PlacedFootprint.Footprint.Height == 0)
        {
            srcLoc.PlacedFootprint.Footprint.Height = 1;
        }

        cmdList->CopyTextureRegion(&dstLoc,
                                   static_cast<UINT>(dstOffset.x),
                                   static_cast<UINT>(dstOffset.y),
                                   static_cast<UINT>(dstOffset.z),
                                   &srcLoc,
                                   nullptr);
    }

    static void D3D12_CopyStagingToBuffer(NS::RHI::IRHIUploadContext* ctx,
                                          NS::RHI::IRHIBuffer* dst,
                                          uint64 dstOffset,
                                          NS::RHI::IRHIBuffer* staging,
                                          uint64 stagingOffset,
                                          uint64 size)
    {
        auto* cmdList = GetCmdList(ctx);
        if (!cmdList || !dst || !staging)
        {
            return;
        }

        auto* d3dDst = static_cast<D3D12Buffer*>(dst)->GetD3DResource();
        auto* d3dSrc = static_cast<D3D12Buffer*>(staging)->GetD3DResource();
        cmdList->CopyBufferRegion(d3dDst, dstOffset, d3dSrc, stagingOffset, size);
    }

    //=========================================================================
    // 登録
    //=========================================================================

    void RegisterD3D12DispatchTable(NS::RHI::RHIDispatchTable& t)
    {
        // Base: プロパティ
        t.GetDevice = &D3D12_GetDevice;
        t.GetGPUMask = &D3D12_GetGPUMask;
        t.GetQueueType = &D3D12_GetQueueType;
        t.GetPipeline = &D3D12_GetPipeline;

        // Base: ライフサイクル
        t.Begin = &D3D12_Begin;
        t.Finish = &D3D12_Finish;
        t.Reset = &D3D12_Reset;
        t.IsRecording = &D3D12_IsRecording;

        // Base: バリア
        t.TransitionResource = &D3D12_TransitionResource;
        t.UAVBarrier = &D3D12_UAVBarrier;
        t.AliasingBarrier = &D3D12_AliasingBarrier;
        t.FlushBarriers = &D3D12_FlushBarriers;

        // Base: コピー
        t.CopyBuffer = &D3D12_CopyBuffer;
        t.CopyBufferRegion = &D3D12_CopyBufferRegion;
        t.CopyTexture = &D3D12_CopyTexture;
        t.CopyTextureRegion = &D3D12_CopyTextureRegion;
        t.CopyBufferToTexture = &D3D12_CopyBufferToTexture;
        t.CopyTextureToBuffer = &D3D12_CopyTextureToBuffer;
        t.CopyToStagingBuffer = &D3D12_CopyToStagingBuffer;

        // Base: MSAA解決
        t.ResolveTexture = &D3D12_ResolveTexture;
        t.ResolveTextureRegion = &D3D12_ResolveTextureRegion;

        // Base: デバッグ
        t.BeginDebugEvent = &D3D12_BeginDebugEvent;
        t.EndDebugEvent = &D3D12_EndDebugEvent;
        t.InsertDebugMarker = &D3D12_InsertDebugMarker;
        t.InsertBreadcrumb = &D3D12_InsertBreadcrumb;

        // ImmediateContext
        t.Flush = &D3D12_Flush;
        t.GetNativeContext = &D3D12_GetNativeContext;

        // Compute: パイプラインステート
        t.SetComputePipelineState = &D3D12_SetComputePipelineState;
        t.SetComputeRootSignature = &D3D12_SetComputeRootSignature;
        t.SetComputeRoot32BitConstants = &D3D12_SetComputeRoot32BitConstants;
        t.SetComputeRootCBV = &D3D12_SetComputeRootCBV;
        t.SetComputeRootSRV = &D3D12_SetComputeRootSRV;
        t.SetComputeRootUAV = &D3D12_SetComputeRootUAV;
        t.SetDescriptorHeaps = &D3D12_SetDescriptorHeaps;
        t.GetCBVSRVUAVHeap = &D3D12_GetCBVSRVUAVHeap;
        t.GetSamplerHeap = &D3D12_GetSamplerHeap;
        t.SetComputeRootDescriptorTable = &D3D12_SetComputeRootDescriptorTable;
        t.Dispatch = &D3D12_Dispatch;
        t.DispatchIndirect = &D3D12_DispatchIndirect;
        t.DispatchIndirectMulti = &D3D12_DispatchIndirectMulti;
        t.ClearUnorderedAccessViewUint = &D3D12_ClearUnorderedAccessViewUint;
        t.ClearUnorderedAccessViewFloat = &D3D12_ClearUnorderedAccessViewFloat;
        t.WriteTimestamp = &D3D12_WriteTimestamp;
        t.BeginQuery = &D3D12_BeginQuery;
        t.EndQuery = &D3D12_EndQuery;
        t.ResolveQueryData = &D3D12_ResolveQueryData;
        t.GetQueryResult = &D3D12_GetQueryResult;

        // Graphics: パイプラインステート
        t.SetGraphicsPipelineState = &D3D12_SetGraphicsPipelineState;
        t.SetGraphicsRootSignature = &D3D12_SetGraphicsRootSignature;
        t.SetRenderTargets = &D3D12_SetRenderTargets;
        t.ClearRenderTargetView = &D3D12_ClearRenderTargetView;
        t.ClearDepthStencilView = &D3D12_ClearDepthStencilView;
        t.SetViewports = &D3D12_SetViewports;
        t.SetScissorRects = &D3D12_SetScissorRects;
        t.SetVertexBuffers = &D3D12_SetVertexBuffers;
        t.SetIndexBuffer = &D3D12_SetIndexBuffer;
        t.SetPrimitiveTopology = &D3D12_SetPrimitiveTopology;
        t.Draw = &D3D12_Draw;
        t.DrawIndexed = &D3D12_DrawIndexed;
        t.DrawIndirect = &D3D12_DrawIndirect;
        t.DrawIndexedIndirect = &D3D12_DrawIndexedIndirect;
        t.MultiDrawIndirect = &D3D12_MultiDrawIndirect;
        t.MultiDrawIndirectCount = &D3D12_MultiDrawIndirectCount;

        // Graphics: ルート定数・ディスクリプタ
        t.SetGraphicsRootDescriptorTable = &D3D12_SetGraphicsRootDescriptorTable;
        t.SetGraphicsRootCBV = &D3D12_SetGraphicsRootCBV;
        t.SetGraphicsRootSRV = &D3D12_SetGraphicsRootSRV;
        t.SetGraphicsRootUAV = &D3D12_SetGraphicsRootUAV;
        t.SetGraphicsRoot32BitConstants = &D3D12_SetGraphicsRoot32BitConstants;
        t.SetBlendFactor = &D3D12_SetBlendFactor;
        t.SetStencilRef = &D3D12_SetStencilRef;
        t.SetLineWidth = &D3D12_SetLineWidth;
        t.SetDepthBounds = &D3D12_SetDepthBounds;

        // Graphics: レンダーパス
        t.BeginRenderPass = &D3D12_BeginRenderPass;
        t.EndRenderPass = &D3D12_EndRenderPass;
        t.IsInRenderPass = &D3D12_IsInRenderPass;
        t.GetCurrentRenderPassDesc = &D3D12_GetCurrentRenderPassDesc;
        t.NextSubpass = &D3D12_NextSubpass;
        t.GetCurrentSubpassIndex = &D3D12_GetCurrentSubpassIndex;
        t.GetRenderPassStatistics = &D3D12_GetRenderPassStatistics;
        t.ResetStatistics = &D3D12_ResetStatistics;

        // Graphics: バリア（バッチ）
        t.TransitionBarrier = &D3D12_TransitionBarrier;
        t.TransitionBarriers = &D3D12_TransitionBarriers;
        t.UAVBarriers = &D3D12_UAVBarriers;
        t.AliasingBarriers = &D3D12_AliasingBarriers;

        // Graphics: その他
        t.SetPredication = &D3D12_SetPredication;
        t.ExecuteIndirect = &D3D12_ExecuteIndirect;
        t.BeginBreadcrumbGPU = &D3D12_BeginBreadcrumbGPU;
        t.EndBreadcrumbGPU = &D3D12_EndBreadcrumbGPU;
        t.CommitBuffer = &D3D12_CommitBuffer;
        t.CommitTextureRegions = &D3D12_CommitTextureRegions;

        // Raytracing
        t.BuildRaytracingAccelerationStructure = &D3D12_BuildRaytracingAccelerationStructure;
        t.CopyRaytracingAccelerationStructure = &D3D12_CopyRaytracingAccelerationStructure;
        t.SetRaytracingPipelineState = &D3D12_SetRaytracingPipelineState;
        t.DispatchRays = &D3D12_DispatchRays;

        // Work Graphs
        t.SetWorkGraphPipeline = &D3D12_SetWorkGraphPipeline;
        t.DispatchGraph = &D3D12_DispatchGraph;
        t.InitializeWorkGraphBackingMemory = &D3D12_InitializeWorkGraphBackingMemory;

        // Mesh Shader
        t.SetMeshPipelineState = &D3D12_SetMeshPipelineState;
        t.DispatchMesh = &D3D12_DispatchMesh;
        t.DispatchMeshIndirect = &D3D12_DispatchMeshIndirect;
        t.DispatchMeshIndirectCount = &D3D12_DispatchMeshIndirectCount;

        // VRS
        t.SetShadingRate = &D3D12_SetShadingRate;
        t.SetShadingRateImage = &D3D12_SetShadingRateImage;

        // Upload
        t.UploadBuffer = &D3D12_UploadBuffer;
        t.UploadTexture = &D3D12_UploadTexture;
        t.CopyStagingToTexture = &D3D12_CopyStagingToTexture;
        t.CopyStagingToBuffer = &D3D12_CopyStagingToBuffer;
    }

} // namespace NS::D3D12RHI

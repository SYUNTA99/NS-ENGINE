/// @file RHICommands.h
/// @brief RHIコマンド構造体（遅延実行モデル）
/// @details コマンドをPOD構造体として定義し、静的Executeメソッドで実行する。
///          リニアアロケータに記録し、RHIスレッドで一括実行する。
/// @see 00_Design_Philosophy §1.2, §3.1
#pragma once

#include "RHIDispatchTable.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"

namespace NS::RHI
{
    //=========================================================================
    // コマンド基底
    //=========================================================================

    /// コマンドタイプ識別子
    enum class ERHICommandType : uint16
    {
        // 描画
        Draw,
        DrawIndexed,
        DrawIndirect,
        DrawIndexedIndirect,
        MultiDrawIndirect,
        MultiDrawIndirectCount,

        // コンピュート
        Dispatch,
        DispatchIndirect,
        DispatchIndirectMulti,

        // メッシュシェーダー
        DispatchMesh,
        DispatchMeshIndirect,
        DispatchMeshIndirectCount,

        // パイプラインステート
        SetGraphicsPipelineState,
        SetComputePipelineState,
        SetMeshPipelineState,
        SetGraphicsRootSignature,
        SetComputeRootSignature,

        // リソースバリア (Base)
        TransitionResource,
        UAVBarrier,
        AliasingBarrier,
        FlushBarriers,

        // リソースバリア (Graphics batch)
        TransitionBarrier,
        TransitionBarriers,
        UAVBarriers,
        AliasingBarriers,

        // コピー (Base)
        CopyBuffer,
        CopyBufferRegion,
        CopyTexture,
        CopyTextureRegion,
        CopyBufferToTexture,
        CopyTextureToBuffer,
        CopyToStagingBuffer,
        ResolveTexture,
        ResolveTextureRegion,

        // レンダーパス
        BeginRenderPass,
        EndRenderPass,
        NextSubpass,
        ResetStatistics,

        // ビューポート・シザー
        SetViewports,
        SetScissorRects,

        // 頂点・インデックスバッファ
        SetVertexBuffers,
        SetIndexBuffer,
        SetPrimitiveTopology,

        // レンダーターゲット
        SetRenderTargets,
        ClearRenderTargetView,
        ClearDepthStencilView,

        // デバッグ
        BeginDebugEvent,
        EndDebugEvent,
        InsertDebugMarker,
        InsertBreadcrumb,

        // Compute: ルート引数
        SetComputeRoot32BitConstants,
        SetComputeRootCBV,
        SetComputeRootSRV,
        SetComputeRootUAV,
        SetComputeRootDescriptorTable,
        SetDescriptorHeaps,

        // Compute: UAVクリア
        ClearUnorderedAccessViewUint,
        ClearUnorderedAccessViewFloat,

        // Compute: クエリ
        WriteTimestamp,
        BeginQuery,
        EndQuery,
        ResolveQueryData,

        // Graphics: ルート引数
        SetGraphicsRoot32BitConstants,
        SetGraphicsRootCBV,
        SetGraphicsRootSRV,
        SetGraphicsRootUAV,
        SetGraphicsRootDescriptorTable,

        // Graphics: ステート
        SetBlendFactor,
        SetStencilRef,
        SetLineWidth,
        SetDepthBounds,
        SetShadingRate,
        SetShadingRateImage,
        SetPredication,

        // Graphics: Reserved Resource
        CommitBuffer,
        CommitTextureRegions,

        // Graphics: ワークグラフ
        SetWorkGraphPipeline,
        DispatchGraph,
        InitializeWorkGraphBackingMemory,

        // Graphics: ExecuteIndirect
        ExecuteIndirect,

        // Graphics: Breadcrumb GPU
        BeginBreadcrumbGPU,
        EndBreadcrumbGPU,

        // Graphics: レイトレーシング
        BuildRaytracingAccelerationStructure,
        CopyRaytracingAccelerationStructure,
        SetRaytracingPipelineState,
        DispatchRays,

        // Upload
        UploadBuffer,
        UploadTexture,
        CopyStagingToTexture,
        CopyStagingToBuffer,

        Count
    };

    /// コマンドヘッダ
    /// 全コマンド構造体の先頭に配置される
    struct RHICommandHeader
    {
        ERHICommandType type; ///< コマンドタイプ
        uint16 size;          ///< コマンド全体サイズ（ヘッダ含む）
        uint32 nextOffset;    ///< 次コマンドへのオフセット（0=終端）
    };

    //=========================================================================
    // 描画コマンド
    //=========================================================================

    /// 設計書 §1.2: "コマンドは構造体として定義し、静的ディスパッチを用いること"

    struct RHI_API CmdDraw
    {
        static constexpr ERHICommandType kType = ERHICommandType::Draw;
        RHICommandHeader header;
        uint32 vertexCount;
        uint32 instanceCount;
        uint32 startVertex;
        uint32 startInstance;

        static void Execute(IRHICommandContext* ctx, const CmdDraw& cmd)
        {
            NS_RHI_DISPATCH(Draw, ctx, cmd.vertexCount, cmd.instanceCount, cmd.startVertex, cmd.startInstance);
        }
    };

    struct RHI_API CmdDrawIndexed
    {
        static constexpr ERHICommandType kType = ERHICommandType::DrawIndexed;
        RHICommandHeader header;
        uint32 indexCount;
        uint32 instanceCount;
        uint32 startIndex;
        int32 baseVertex;
        uint32 startInstance;

        static void Execute(IRHICommandContext* ctx, const CmdDrawIndexed& cmd)
        {
            NS_RHI_DISPATCH(
                DrawIndexed, ctx, cmd.indexCount, cmd.instanceCount, cmd.startIndex, cmd.baseVertex, cmd.startInstance);
        }
    };

    struct RHI_API CmdDrawIndirect
    {
        static constexpr ERHICommandType kType = ERHICommandType::DrawIndirect;
        RHICommandHeader header;
        IRHIBuffer* argsBuffer;
        uint64 argsOffset;

        static void Execute(IRHICommandContext* ctx, const CmdDrawIndirect& cmd)
        {
            NS_RHI_DISPATCH(DrawIndirect, ctx, cmd.argsBuffer, cmd.argsOffset);
        }
    };

    struct RHI_API CmdDrawIndexedIndirect
    {
        static constexpr ERHICommandType kType = ERHICommandType::DrawIndexedIndirect;
        RHICommandHeader header;
        IRHIBuffer* argsBuffer;
        uint64 argsOffset;

        static void Execute(IRHICommandContext* ctx, const CmdDrawIndexedIndirect& cmd)
        {
            NS_RHI_DISPATCH(DrawIndexedIndirect, ctx, cmd.argsBuffer, cmd.argsOffset);
        }
    };

    struct RHI_API CmdMultiDrawIndirect
    {
        static constexpr ERHICommandType kType = ERHICommandType::MultiDrawIndirect;
        RHICommandHeader header;
        IRHIBuffer* argsBuffer;
        uint32 drawCount;
        uint64 argsOffset;
        uint32 argsStride;

        static void Execute(IRHICommandContext* ctx, const CmdMultiDrawIndirect& cmd)
        {
            NS_RHI_DISPATCH(MultiDrawIndirect, ctx, cmd.argsBuffer, cmd.drawCount, cmd.argsOffset, cmd.argsStride);
        }
    };

    struct RHI_API CmdMultiDrawIndirectCount
    {
        static constexpr ERHICommandType kType = ERHICommandType::MultiDrawIndirectCount;
        RHICommandHeader header;
        IRHIBuffer* argsBuffer;
        uint64 argsOffset;
        IRHIBuffer* countBuffer;
        uint64 countOffset;
        uint32 maxDrawCount;
        uint32 argsStride;

        static void Execute(IRHICommandContext* ctx, const CmdMultiDrawIndirectCount& cmd)
        {
            NS_RHI_DISPATCH(MultiDrawIndirectCount,
                            ctx,
                            cmd.argsBuffer,
                            cmd.argsOffset,
                            cmd.countBuffer,
                            cmd.countOffset,
                            cmd.maxDrawCount,
                            cmd.argsStride);
        }
    };

    //=========================================================================
    // コンピュートコマンド
    //=========================================================================

    struct RHI_API CmdDispatch
    {
        static constexpr ERHICommandType kType = ERHICommandType::Dispatch;
        RHICommandHeader header;
        uint32 groupCountX;
        uint32 groupCountY;
        uint32 groupCountZ;

        static void Execute(IRHIComputeContext* ctx, const CmdDispatch& cmd)
        {
            NS_RHI_DISPATCH(Dispatch, ctx, cmd.groupCountX, cmd.groupCountY, cmd.groupCountZ);
        }
    };

    struct RHI_API CmdDispatchIndirect
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchIndirect;
        RHICommandHeader header;
        IRHIBuffer* argsBuffer;
        uint64 argsOffset;

        static void Execute(IRHIComputeContext* ctx, const CmdDispatchIndirect& cmd)
        {
            NS_RHI_DISPATCH(DispatchIndirect, ctx, cmd.argsBuffer, cmd.argsOffset);
        }
    };

    struct RHI_API CmdDispatchIndirectMulti
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchIndirectMulti;
        RHICommandHeader header;
        IRHIBuffer* argsBuffer;
        uint64 argsOffset;
        uint32 dispatchCount;
        uint32 stride;

        static void Execute(IRHIComputeContext* ctx, const CmdDispatchIndirectMulti& cmd)
        {
            NS_RHI_DISPATCH(DispatchIndirectMulti, ctx, cmd.argsBuffer, cmd.argsOffset, cmd.dispatchCount, cmd.stride);
        }
    };

    //=========================================================================
    // メッシュシェーダーコマンド
    //=========================================================================

    struct RHI_API CmdDispatchMesh
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchMesh;
        RHICommandHeader header;
        uint32 groupCountX;
        uint32 groupCountY;
        uint32 groupCountZ;

        static void Execute(IRHICommandContext* ctx, const CmdDispatchMesh& cmd)
        {
            NS_RHI_DISPATCH(DispatchMesh, ctx, cmd.groupCountX, cmd.groupCountY, cmd.groupCountZ);
        }
    };

    struct RHI_API CmdDispatchMeshIndirect
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchMeshIndirect;
        RHICommandHeader header;
        IRHIBuffer* argumentBuffer;
        uint64 argumentOffset;

        static void Execute(IRHICommandContext* ctx, const CmdDispatchMeshIndirect& cmd)
        {
            NS_RHI_DISPATCH(DispatchMeshIndirect, ctx, cmd.argumentBuffer, cmd.argumentOffset);
        }
    };

    struct RHI_API CmdDispatchMeshIndirectCount
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchMeshIndirectCount;
        RHICommandHeader header;
        IRHIBuffer* argumentBuffer;
        uint64 argumentOffset;
        IRHIBuffer* countBuffer;
        uint64 countOffset;
        uint32 maxDispatchCount;

        static void Execute(IRHICommandContext* ctx, const CmdDispatchMeshIndirectCount& cmd)
        {
            NS_RHI_DISPATCH(DispatchMeshIndirectCount, ctx, cmd.argumentBuffer, cmd.argumentOffset, cmd.countBuffer, cmd.countOffset, cmd.maxDispatchCount);
        }
    };

    //=========================================================================
    // パイプラインステートコマンド
    //=========================================================================

    struct RHI_API CmdSetGraphicsPSO
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsPipelineState;
        RHICommandHeader header;
        IRHIGraphicsPipelineState* pso;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsPSO& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsPipelineState, ctx, cmd.pso);
        }
    };

    struct RHI_API CmdSetComputePSO
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputePipelineState;
        RHICommandHeader header;
        IRHIComputePipelineState* pso;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputePSO& cmd)
        {
            NS_RHI_DISPATCH(SetComputePipelineState, ctx, cmd.pso);
        }
    };

    struct RHI_API CmdSetMeshPSO
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetMeshPipelineState;
        RHICommandHeader header;
        IRHIMeshPipelineState* pso;

        static void Execute(IRHICommandContext* ctx, const CmdSetMeshPSO& cmd)
        {
            NS_RHI_DISPATCH(SetMeshPipelineState, ctx, cmd.pso);
        }
    };

    struct RHI_API CmdSetGraphicsRootSignature
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsRootSignature;
        RHICommandHeader header;
        IRHIRootSignature* rootSignature;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsRootSignature& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsRootSignature, ctx, cmd.rootSignature);
        }
    };

    struct RHI_API CmdSetComputeRootSignature
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputeRootSignature;
        RHICommandHeader header;
        IRHIRootSignature* rootSignature;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputeRootSignature& cmd)
        {
            NS_RHI_DISPATCH(SetComputeRootSignature, ctx, cmd.rootSignature);
        }
    };

    //=========================================================================
    // バリアコマンド (Base)
    //=========================================================================

    struct RHI_API CmdTransitionResource
    {
        static constexpr ERHICommandType kType = ERHICommandType::TransitionResource;
        RHICommandHeader header;
        IRHIResource* resource;
        ERHIAccess stateBefore;
        ERHIAccess stateAfter;

        static void Execute(IRHICommandContextBase* ctx, const CmdTransitionResource& cmd)
        {
            NS_RHI_DISPATCH(TransitionResource, ctx, cmd.resource, cmd.stateBefore, cmd.stateAfter);
        }
    };

    struct RHI_API CmdUAVBarrier
    {
        static constexpr ERHICommandType kType = ERHICommandType::UAVBarrier;
        RHICommandHeader header;
        IRHIResource* resource;

        static void Execute(IRHICommandContextBase* ctx, const CmdUAVBarrier& cmd)
        {
            NS_RHI_DISPATCH(UAVBarrier, ctx, cmd.resource);
        }
    };

    struct RHI_API CmdAliasingBarrier
    {
        static constexpr ERHICommandType kType = ERHICommandType::AliasingBarrier;
        RHICommandHeader header;
        IRHIResource* resourceBefore;
        IRHIResource* resourceAfter;

        static void Execute(IRHICommandContextBase* ctx, const CmdAliasingBarrier& cmd)
        {
            NS_RHI_DISPATCH(AliasingBarrier, ctx, cmd.resourceBefore, cmd.resourceAfter);
        }
    };

    struct RHI_API CmdFlushBarriers
    {
        static constexpr ERHICommandType kType = ERHICommandType::FlushBarriers;
        RHICommandHeader header;

        static void Execute(IRHICommandContextBase* ctx, const CmdFlushBarriers& /*cmd*/)
        {
            NS_RHI_DISPATCH(FlushBarriers, ctx);
        }
    };

    //=========================================================================
    // バリアコマンド (Graphics batch)
    //=========================================================================

    struct RHI_API CmdTransitionBarrier
    {
        static constexpr ERHICommandType kType = ERHICommandType::TransitionBarrier;
        RHICommandHeader header;
        IRHIResource* resource;
        ERHIResourceState stateBefore;
        ERHIResourceState stateAfter;
        uint32 subresource;

        static void Execute(IRHICommandContext* ctx, const CmdTransitionBarrier& cmd)
        {
            NS_RHI_DISPATCH(TransitionBarrier, ctx, cmd.resource, cmd.stateBefore, cmd.stateAfter, cmd.subresource);
        }
    };

    struct RHI_API CmdTransitionBarriers
    {
        static constexpr ERHICommandType kType = ERHICommandType::TransitionBarriers;
        RHICommandHeader header;
        const RHITransitionBarrier* barriers;
        uint32 count;

        static void Execute(IRHICommandContext* ctx, const CmdTransitionBarriers& cmd)
        {
            NS_RHI_DISPATCH(TransitionBarriers, ctx, cmd.barriers, cmd.count);
        }
    };

    struct RHI_API CmdUAVBarriers
    {
        static constexpr ERHICommandType kType = ERHICommandType::UAVBarriers;
        RHICommandHeader header;
        const RHIUAVBarrier* barriers;
        uint32 count;

        static void Execute(IRHICommandContext* ctx, const CmdUAVBarriers& cmd)
        {
            NS_RHI_DISPATCH(UAVBarriers, ctx, cmd.barriers, cmd.count);
        }
    };

    struct RHI_API CmdAliasingBarriers
    {
        static constexpr ERHICommandType kType = ERHICommandType::AliasingBarriers;
        RHICommandHeader header;
        const RHIAliasingBarrier* barriers;
        uint32 count;

        static void Execute(IRHICommandContext* ctx, const CmdAliasingBarriers& cmd)
        {
            NS_RHI_DISPATCH(AliasingBarriers, ctx, cmd.barriers, cmd.count);
        }
    };

    //=========================================================================
    // コピーコマンド
    //=========================================================================

    struct RHI_API CmdCopyBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyBuffer;
        RHICommandHeader header;
        IRHIBuffer* dst;
        IRHIBuffer* src;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyBuffer& cmd)
        {
            NS_RHI_DISPATCH(CopyBuffer, ctx, cmd.dst, cmd.src);
        }
    };

    struct RHI_API CmdCopyBufferRegion
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyBufferRegion;
        RHICommandHeader header;
        IRHIBuffer* dst;
        uint64 dstOffset;
        IRHIBuffer* src;
        uint64 srcOffset;
        uint64 size;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyBufferRegion& cmd)
        {
            NS_RHI_DISPATCH(CopyBufferRegion, ctx, cmd.dst, cmd.dstOffset, cmd.src, cmd.srcOffset, cmd.size);
        }
    };

    struct RHI_API CmdCopyTexture
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyTexture;
        RHICommandHeader header;
        IRHITexture* dst;
        IRHITexture* src;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyTexture& cmd)
        {
            NS_RHI_DISPATCH(CopyTexture, ctx, cmd.dst, cmd.src);
        }
    };

    struct RHI_API CmdCopyTextureRegion
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyTextureRegion;
        RHICommandHeader header;
        IRHITexture* dst;
        uint32 dstMip;
        uint32 dstSlice;
        Offset3D dstOffset;
        IRHITexture* src;
        uint32 srcMip;
        uint32 srcSlice;
        const RHIBox* srcBox;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyTextureRegion& cmd)
        {
            NS_RHI_DISPATCH(CopyTextureRegion, ctx, cmd.dst, cmd.dstMip, cmd.dstSlice, cmd.dstOffset, cmd.src, cmd.srcMip, cmd.srcSlice, cmd.srcBox);
        }
    };

    struct RHI_API CmdCopyBufferToTexture
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyBufferToTexture;
        RHICommandHeader header;
        IRHITexture* dst;
        uint32 dstMip;
        uint32 dstSlice;
        Offset3D dstOffset;
        IRHIBuffer* src;
        uint64 srcOffset;
        uint32 srcRowPitch;
        uint32 srcDepthPitch;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyBufferToTexture& cmd)
        {
            NS_RHI_DISPATCH(CopyBufferToTexture, ctx, cmd.dst, cmd.dstMip, cmd.dstSlice, cmd.dstOffset, cmd.src, cmd.srcOffset, cmd.srcRowPitch, cmd.srcDepthPitch);
        }
    };

    struct RHI_API CmdCopyTextureToBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyTextureToBuffer;
        RHICommandHeader header;
        IRHIBuffer* dst;
        uint64 dstOffset;
        uint32 dstRowPitch;
        uint32 dstDepthPitch;
        IRHITexture* src;
        uint32 srcMip;
        uint32 srcSlice;
        const RHIBox* srcBox;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyTextureToBuffer& cmd)
        {
            NS_RHI_DISPATCH(CopyTextureToBuffer, ctx, cmd.dst, cmd.dstOffset, cmd.dstRowPitch, cmd.dstDepthPitch, cmd.src, cmd.srcMip, cmd.srcSlice, cmd.srcBox);
        }
    };

    struct RHI_API CmdCopyToStagingBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyToStagingBuffer;
        RHICommandHeader header;
        IRHIStagingBuffer* dst;
        uint64 dstOffset;
        IRHIResource* src;
        uint64 srcOffset;
        uint64 size;

        static void Execute(IRHICommandContextBase* ctx, const CmdCopyToStagingBuffer& cmd)
        {
            NS_RHI_DISPATCH(CopyToStagingBuffer, ctx, cmd.dst, cmd.dstOffset, cmd.src, cmd.srcOffset, cmd.size);
        }
    };

    struct RHI_API CmdResolveTexture
    {
        static constexpr ERHICommandType kType = ERHICommandType::ResolveTexture;
        RHICommandHeader header;
        IRHITexture* dst;
        IRHITexture* src;

        static void Execute(IRHICommandContextBase* ctx, const CmdResolveTexture& cmd)
        {
            NS_RHI_DISPATCH(ResolveTexture, ctx, cmd.dst, cmd.src);
        }
    };

    struct RHI_API CmdResolveTextureRegion
    {
        static constexpr ERHICommandType kType = ERHICommandType::ResolveTextureRegion;
        RHICommandHeader header;
        IRHITexture* dst;
        uint32 dstMip;
        uint32 dstSlice;
        IRHITexture* src;
        uint32 srcMip;
        uint32 srcSlice;

        static void Execute(IRHICommandContextBase* ctx, const CmdResolveTextureRegion& cmd)
        {
            NS_RHI_DISPATCH(ResolveTextureRegion, ctx, cmd.dst, cmd.dstMip, cmd.dstSlice, cmd.src, cmd.srcMip, cmd.srcSlice);
        }
    };

    //=========================================================================
    // レンダーパスコマンド
    //=========================================================================

    struct RHI_API CmdBeginRenderPass
    {
        static constexpr ERHICommandType kType = ERHICommandType::BeginRenderPass;
        RHICommandHeader header;
        const RHIRenderPassDesc* desc;

        static void Execute(IRHICommandContext* ctx, const CmdBeginRenderPass& cmd)
        {
            NS_RHI_DISPATCH(BeginRenderPass, ctx, cmd.desc);
        }
    };

    struct RHI_API CmdEndRenderPass
    {
        static constexpr ERHICommandType kType = ERHICommandType::EndRenderPass;
        RHICommandHeader header;

        static void Execute(IRHICommandContext* ctx, const CmdEndRenderPass& /*cmd*/)
        {
            NS_RHI_DISPATCH(EndRenderPass, ctx);
        }
    };

    struct RHI_API CmdNextSubpass
    {
        static constexpr ERHICommandType kType = ERHICommandType::NextSubpass;
        RHICommandHeader header;

        static void Execute(IRHICommandContext* ctx, const CmdNextSubpass& /*cmd*/)
        {
            NS_RHI_DISPATCH(NextSubpass, ctx);
        }
    };

    struct RHI_API CmdResetStatistics
    {
        static constexpr ERHICommandType kType = ERHICommandType::ResetStatistics;
        RHICommandHeader header;

        static void Execute(IRHICommandContext* ctx, const CmdResetStatistics& /*cmd*/)
        {
            NS_RHI_DISPATCH(ResetStatistics, ctx);
        }
    };

    //=========================================================================
    // ビューポート・シザーコマンド
    //=========================================================================

    struct RHI_API CmdSetViewports
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetViewports;
        RHICommandHeader header;
        uint32 count;
        const RHIViewport* viewports;

        static void Execute(IRHICommandContext* ctx, const CmdSetViewports& cmd)
        {
            NS_RHI_DISPATCH(SetViewports, ctx, cmd.count, cmd.viewports);
        }
    };

    struct RHI_API CmdSetScissorRects
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetScissorRects;
        RHICommandHeader header;
        uint32 count;
        const RHIRect* rects;

        static void Execute(IRHICommandContext* ctx, const CmdSetScissorRects& cmd)
        {
            NS_RHI_DISPATCH(SetScissorRects, ctx, cmd.count, cmd.rects);
        }
    };

    //=========================================================================
    // 頂点・インデックスバッファコマンド
    //=========================================================================

    struct RHI_API CmdSetVertexBuffers
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetVertexBuffers;
        RHICommandHeader header;
        uint32 startSlot;
        uint32 numBuffers;
        const struct RHIVertexBufferView* views;

        static void Execute(IRHICommandContext* ctx, const CmdSetVertexBuffers& cmd)
        {
            NS_RHI_DISPATCH(SetVertexBuffers, ctx, cmd.startSlot, cmd.numBuffers, cmd.views);
        }
    };

    struct RHI_API CmdSetIndexBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetIndexBuffer;
        RHICommandHeader header;
        const struct RHIIndexBufferView* view;

        static void Execute(IRHICommandContext* ctx, const CmdSetIndexBuffer& cmd)
        {
            NS_RHI_DISPATCH(SetIndexBuffer, ctx, cmd.view);
        }
    };

    struct RHI_API CmdSetPrimitiveTopology
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetPrimitiveTopology;
        RHICommandHeader header;
        ERHIPrimitiveTopology topology;

        static void Execute(IRHICommandContext* ctx, const CmdSetPrimitiveTopology& cmd)
        {
            NS_RHI_DISPATCH(SetPrimitiveTopology, ctx, cmd.topology);
        }
    };

    //=========================================================================
    // レンダーターゲットコマンド
    //=========================================================================

    struct RHI_API CmdSetRenderTargets
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetRenderTargets;
        RHICommandHeader header;
        uint32 numRTVs;
        IRHIRenderTargetView* const* rtvs;
        IRHIDepthStencilView* dsv;

        static void Execute(IRHICommandContext* ctx, const CmdSetRenderTargets& cmd)
        {
            NS_RHI_DISPATCH(SetRenderTargets, ctx, cmd.numRTVs, cmd.rtvs, cmd.dsv);
        }
    };

    struct RHI_API CmdClearRenderTargetView
    {
        static constexpr ERHICommandType kType = ERHICommandType::ClearRenderTargetView;
        RHICommandHeader header;
        IRHIRenderTargetView* rtv;
        float color[4];

        static void Execute(IRHICommandContext* ctx, const CmdClearRenderTargetView& cmd)
        {
            NS_RHI_DISPATCH(ClearRenderTargetView, ctx, cmd.rtv, cmd.color);
        }
    };

    struct RHI_API CmdClearDepthStencilView
    {
        static constexpr ERHICommandType kType = ERHICommandType::ClearDepthStencilView;
        RHICommandHeader header;
        IRHIDepthStencilView* dsv;
        bool clearDepth;
        float depth;
        bool clearStencil;
        uint8 stencil;

        static void Execute(IRHICommandContext* ctx, const CmdClearDepthStencilView& cmd)
        {
            NS_RHI_DISPATCH(ClearDepthStencilView, ctx, cmd.dsv, cmd.clearDepth, cmd.depth, cmd.clearStencil, cmd.stencil);
        }
    };

    //=========================================================================
    // デバッグコマンド
    //=========================================================================

    struct RHI_API CmdBeginDebugEvent
    {
        static constexpr ERHICommandType kType = ERHICommandType::BeginDebugEvent;
        RHICommandHeader header;
        const char* name;
        uint32 color;

        static void Execute(IRHICommandContextBase* ctx, const CmdBeginDebugEvent& cmd)
        {
            NS_RHI_DISPATCH(BeginDebugEvent, ctx, cmd.name, cmd.color);
        }
    };

    struct RHI_API CmdEndDebugEvent
    {
        static constexpr ERHICommandType kType = ERHICommandType::EndDebugEvent;
        RHICommandHeader header;

        static void Execute(IRHICommandContextBase* ctx, const CmdEndDebugEvent& /*cmd*/)
        {
            NS_RHI_DISPATCH(EndDebugEvent, ctx);
        }
    };

    struct RHI_API CmdInsertDebugMarker
    {
        static constexpr ERHICommandType kType = ERHICommandType::InsertDebugMarker;
        RHICommandHeader header;
        const char* name;
        uint32 color;

        static void Execute(IRHICommandContextBase* ctx, const CmdInsertDebugMarker& cmd)
        {
            NS_RHI_DISPATCH(InsertDebugMarker, ctx, cmd.name, cmd.color);
        }
    };

    struct RHI_API CmdInsertBreadcrumb
    {
        static constexpr ERHICommandType kType = ERHICommandType::InsertBreadcrumb;
        RHICommandHeader header;
        uint32 id;
        const char* message;

        static void Execute(IRHICommandContextBase* ctx, const CmdInsertBreadcrumb& cmd)
        {
            NS_RHI_DISPATCH(InsertBreadcrumb, ctx, cmd.id, cmd.message);
        }
    };

    //=========================================================================
    // Compute: ルート引数コマンド
    //=========================================================================

    struct RHI_API CmdSetComputeRoot32BitConstants
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputeRoot32BitConstants;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint32 num32BitValues;
        const void* data;
        uint32 destOffset;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputeRoot32BitConstants& cmd)
        {
            NS_RHI_DISPATCH(SetComputeRoot32BitConstants, ctx, cmd.rootParameterIndex, cmd.num32BitValues, cmd.data, cmd.destOffset);
        }
    };

    struct RHI_API CmdSetComputeRootCBV
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputeRootCBV;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint64 bufferAddress;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputeRootCBV& cmd)
        {
            NS_RHI_DISPATCH(SetComputeRootCBV, ctx, cmd.rootParameterIndex, cmd.bufferAddress);
        }
    };

    struct RHI_API CmdSetComputeRootSRV
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputeRootSRV;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint64 bufferAddress;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputeRootSRV& cmd)
        {
            NS_RHI_DISPATCH(SetComputeRootSRV, ctx, cmd.rootParameterIndex, cmd.bufferAddress);
        }
    };

    struct RHI_API CmdSetComputeRootUAV
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputeRootUAV;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint64 bufferAddress;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputeRootUAV& cmd)
        {
            NS_RHI_DISPATCH(SetComputeRootUAV, ctx, cmd.rootParameterIndex, cmd.bufferAddress);
        }
    };

    struct RHI_API CmdSetComputeRootDescriptorTable
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetComputeRootDescriptorTable;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        RHIGPUDescriptorHandle baseDescriptor;

        static void Execute(IRHIComputeContext* ctx, const CmdSetComputeRootDescriptorTable& cmd)
        {
            NS_RHI_DISPATCH(SetComputeRootDescriptorTable, ctx, cmd.rootParameterIndex, cmd.baseDescriptor);
        }
    };

    struct RHI_API CmdSetDescriptorHeaps
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetDescriptorHeaps;
        RHICommandHeader header;
        IRHIDescriptorHeap* cbvSrvUavHeap;
        IRHIDescriptorHeap* samplerHeap;

        static void Execute(IRHIComputeContext* ctx, const CmdSetDescriptorHeaps& cmd)
        {
            NS_RHI_DISPATCH(SetDescriptorHeaps, ctx, cmd.cbvSrvUavHeap, cmd.samplerHeap);
        }
    };

    //=========================================================================
    // Compute: UAVクリアコマンド
    //=========================================================================

    struct RHI_API CmdClearUnorderedAccessViewUint
    {
        static constexpr ERHICommandType kType = ERHICommandType::ClearUnorderedAccessViewUint;
        RHICommandHeader header;
        IRHIUnorderedAccessView* uav;
        uint32 values[4];

        static void Execute(IRHIComputeContext* ctx, const CmdClearUnorderedAccessViewUint& cmd)
        {
            NS_RHI_DISPATCH(ClearUnorderedAccessViewUint, ctx, cmd.uav, cmd.values);
        }
    };

    struct RHI_API CmdClearUnorderedAccessViewFloat
    {
        static constexpr ERHICommandType kType = ERHICommandType::ClearUnorderedAccessViewFloat;
        RHICommandHeader header;
        IRHIUnorderedAccessView* uav;
        float values[4];

        static void Execute(IRHIComputeContext* ctx, const CmdClearUnorderedAccessViewFloat& cmd)
        {
            NS_RHI_DISPATCH(ClearUnorderedAccessViewFloat, ctx, cmd.uav, cmd.values);
        }
    };

    //=========================================================================
    // Compute: クエリコマンド
    //=========================================================================

    struct RHI_API CmdWriteTimestamp
    {
        static constexpr ERHICommandType kType = ERHICommandType::WriteTimestamp;
        RHICommandHeader header;
        IRHIQueryHeap* queryHeap;
        uint32 queryIndex;

        static void Execute(IRHIComputeContext* ctx, const CmdWriteTimestamp& cmd)
        {
            NS_RHI_DISPATCH(WriteTimestamp, ctx, cmd.queryHeap, cmd.queryIndex);
        }
    };

    struct RHI_API CmdBeginQuery
    {
        static constexpr ERHICommandType kType = ERHICommandType::BeginQuery;
        RHICommandHeader header;
        IRHIQueryHeap* queryHeap;
        uint32 queryIndex;

        static void Execute(IRHIComputeContext* ctx, const CmdBeginQuery& cmd)
        {
            NS_RHI_DISPATCH(BeginQuery, ctx, cmd.queryHeap, cmd.queryIndex);
        }
    };

    struct RHI_API CmdEndQuery
    {
        static constexpr ERHICommandType kType = ERHICommandType::EndQuery;
        RHICommandHeader header;
        IRHIQueryHeap* queryHeap;
        uint32 queryIndex;

        static void Execute(IRHIComputeContext* ctx, const CmdEndQuery& cmd)
        {
            NS_RHI_DISPATCH(EndQuery, ctx, cmd.queryHeap, cmd.queryIndex);
        }
    };

    struct RHI_API CmdResolveQueryData
    {
        static constexpr ERHICommandType kType = ERHICommandType::ResolveQueryData;
        RHICommandHeader header;
        IRHIQueryHeap* queryHeap;
        uint32 startIndex;
        uint32 numQueries;
        IRHIBuffer* destinationBuffer;
        uint64 destinationOffset;

        static void Execute(IRHIComputeContext* ctx, const CmdResolveQueryData& cmd)
        {
            NS_RHI_DISPATCH(ResolveQueryData, ctx, cmd.queryHeap, cmd.startIndex, cmd.numQueries, cmd.destinationBuffer, cmd.destinationOffset);
        }
    };

    //=========================================================================
    // Graphics: ルート引数コマンド
    //=========================================================================

    struct RHI_API CmdSetGraphicsRoot32BitConstants
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsRoot32BitConstants;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint32 num32BitValues;
        const void* data;
        uint32 destOffset;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsRoot32BitConstants& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsRoot32BitConstants, ctx, cmd.rootParameterIndex, cmd.num32BitValues, cmd.data, cmd.destOffset);
        }
    };

    struct RHI_API CmdSetGraphicsRootCBV
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsRootCBV;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint64 bufferLocation;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsRootCBV& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsRootCBV, ctx, cmd.rootParameterIndex, cmd.bufferLocation);
        }
    };

    struct RHI_API CmdSetGraphicsRootSRV
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsRootSRV;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint64 bufferLocation;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsRootSRV& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsRootSRV, ctx, cmd.rootParameterIndex, cmd.bufferLocation);
        }
    };

    struct RHI_API CmdSetGraphicsRootUAV
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsRootUAV;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        uint64 bufferLocation;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsRootUAV& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsRootUAV, ctx, cmd.rootParameterIndex, cmd.bufferLocation);
        }
    };

    struct RHI_API CmdSetGraphicsRootDescriptorTable
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetGraphicsRootDescriptorTable;
        RHICommandHeader header;
        uint32 rootParameterIndex;
        RHIGPUDescriptorHandle baseDescriptor;

        static void Execute(IRHICommandContext* ctx, const CmdSetGraphicsRootDescriptorTable& cmd)
        {
            NS_RHI_DISPATCH(SetGraphicsRootDescriptorTable, ctx, cmd.rootParameterIndex, cmd.baseDescriptor);
        }
    };

    //=========================================================================
    // Graphics: ステートコマンド
    //=========================================================================

    struct RHI_API CmdSetBlendFactor
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetBlendFactor;
        RHICommandHeader header;
        float factor[4];

        static void Execute(IRHICommandContext* ctx, const CmdSetBlendFactor& cmd)
        {
            NS_RHI_DISPATCH(SetBlendFactor, ctx, cmd.factor);
        }
    };

    struct RHI_API CmdSetStencilRef
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetStencilRef;
        RHICommandHeader header;
        uint32 refValue;

        static void Execute(IRHICommandContext* ctx, const CmdSetStencilRef& cmd)
        {
            NS_RHI_DISPATCH(SetStencilRef, ctx, cmd.refValue);
        }
    };

    struct RHI_API CmdSetLineWidth
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetLineWidth;
        RHICommandHeader header;
        float width;

        static void Execute(IRHICommandContext* ctx, const CmdSetLineWidth& cmd)
        {
            NS_RHI_DISPATCH(SetLineWidth, ctx, cmd.width);
        }
    };

    struct RHI_API CmdSetDepthBounds
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetDepthBounds;
        RHICommandHeader header;
        float minDepth;
        float maxDepth;

        static void Execute(IRHICommandContext* ctx, const CmdSetDepthBounds& cmd)
        {
            NS_RHI_DISPATCH(SetDepthBounds, ctx, cmd.minDepth, cmd.maxDepth);
        }
    };

    struct RHI_API CmdSetShadingRate
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetShadingRate;
        RHICommandHeader header;
        ERHIShadingRate rate;
        const ERHIVRSCombiner* combiners;

        static void Execute(IRHICommandContext* ctx, const CmdSetShadingRate& cmd)
        {
            NS_RHI_DISPATCH(SetShadingRate, ctx, cmd.rate, cmd.combiners);
        }
    };

    struct RHI_API CmdSetShadingRateImage
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetShadingRateImage;
        RHICommandHeader header;
        IRHITexture* vrsImage;

        static void Execute(IRHICommandContext* ctx, const CmdSetShadingRateImage& cmd)
        {
            NS_RHI_DISPATCH(SetShadingRateImage, ctx, cmd.vrsImage);
        }
    };

    struct RHI_API CmdSetPredication
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetPredication;
        RHICommandHeader header;
        IRHIBuffer* buffer;
        uint64 offset;
        ERHIPredicationOp operation;

        static void Execute(IRHICommandContext* ctx, const CmdSetPredication& cmd)
        {
            NS_RHI_DISPATCH(SetPredication, ctx, cmd.buffer, cmd.offset, cmd.operation);
        }
    };

    //=========================================================================
    // Graphics: Reserved Resourceコマンド
    //=========================================================================

    struct RHI_API CmdCommitBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::CommitBuffer;
        RHICommandHeader header;
        IRHIBuffer* buffer;
        uint64 newCommitSize;

        static void Execute(IRHICommandContext* ctx, const CmdCommitBuffer& cmd)
        {
            NS_RHI_DISPATCH(CommitBuffer, ctx, cmd.buffer, cmd.newCommitSize);
        }
    };

    struct RHI_API CmdCommitTextureRegions
    {
        static constexpr ERHICommandType kType = ERHICommandType::CommitTextureRegions;
        RHICommandHeader header;
        IRHITexture* texture;
        const RHITextureCommitRegion* regions;
        uint32 regionCount;
        bool commit;

        static void Execute(IRHICommandContext* ctx, const CmdCommitTextureRegions& cmd)
        {
            NS_RHI_DISPATCH(CommitTextureRegions, ctx, cmd.texture, cmd.regions, cmd.regionCount, cmd.commit);
        }
    };

    //=========================================================================
    // Graphics: ワークグラフコマンド
    //=========================================================================

    struct RHI_API CmdSetWorkGraphPipeline
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetWorkGraphPipeline;
        RHICommandHeader header;
        IRHIWorkGraphPipeline* pipeline;

        static void Execute(IRHICommandContext* ctx, const CmdSetWorkGraphPipeline& cmd)
        {
            NS_RHI_DISPATCH(SetWorkGraphPipeline, ctx, cmd.pipeline);
        }
    };

    struct RHI_API CmdDispatchGraph
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchGraph;
        RHICommandHeader header;
        const RHIWorkGraphDispatchDesc* desc;

        static void Execute(IRHICommandContext* ctx, const CmdDispatchGraph& cmd)
        {
            NS_RHI_DISPATCH(DispatchGraph, ctx, cmd.desc);
        }
    };

    struct RHI_API CmdInitializeWorkGraphBackingMemory
    {
        static constexpr ERHICommandType kType = ERHICommandType::InitializeWorkGraphBackingMemory;
        RHICommandHeader header;
        IRHIWorkGraphPipeline* pipeline;
        const RHIWorkGraphBackingMemory* memory;

        static void Execute(IRHICommandContext* ctx, const CmdInitializeWorkGraphBackingMemory& cmd)
        {
            NS_RHI_DISPATCH(InitializeWorkGraphBackingMemory, ctx, cmd.pipeline, cmd.memory);
        }
    };

    //=========================================================================
    // Graphics: ExecuteIndirectコマンド
    //=========================================================================

    struct RHI_API CmdExecuteIndirect
    {
        static constexpr ERHICommandType kType = ERHICommandType::ExecuteIndirect;
        RHICommandHeader header;
        IRHICommandSignature* commandSignature;
        uint32 maxCommandCount;
        IRHIBuffer* argumentBuffer;
        uint64 argumentOffset;
        IRHIBuffer* countBuffer;
        uint64 countOffset;

        static void Execute(IRHICommandContext* ctx, const CmdExecuteIndirect& cmd)
        {
            NS_RHI_DISPATCH(ExecuteIndirect, ctx, cmd.commandSignature, cmd.maxCommandCount, cmd.argumentBuffer, cmd.argumentOffset, cmd.countBuffer, cmd.countOffset);
        }
    };

    //=========================================================================
    // Graphics: Breadcrumb GPUコマンド
    //=========================================================================

    struct RHI_API CmdBeginBreadcrumbGPU
    {
        static constexpr ERHICommandType kType = ERHICommandType::BeginBreadcrumbGPU;
        RHICommandHeader header;
        RHIBreadcrumbNode* node;

        static void Execute(IRHICommandContext* ctx, const CmdBeginBreadcrumbGPU& cmd)
        {
            NS_RHI_DISPATCH(BeginBreadcrumbGPU, ctx, cmd.node);
        }
    };

    struct RHI_API CmdEndBreadcrumbGPU
    {
        static constexpr ERHICommandType kType = ERHICommandType::EndBreadcrumbGPU;
        RHICommandHeader header;
        RHIBreadcrumbNode* node;

        static void Execute(IRHICommandContext* ctx, const CmdEndBreadcrumbGPU& cmd)
        {
            NS_RHI_DISPATCH(EndBreadcrumbGPU, ctx, cmd.node);
        }
    };

    //=========================================================================
    // Graphics: レイトレーシングコマンド
    //=========================================================================

    struct RHI_API CmdBuildRaytracingAccelerationStructure
    {
        static constexpr ERHICommandType kType = ERHICommandType::BuildRaytracingAccelerationStructure;
        RHICommandHeader header;
        const RHIAccelerationStructureBuildDesc* desc;

        static void Execute(IRHICommandContext* ctx, const CmdBuildRaytracingAccelerationStructure& cmd)
        {
            NS_RHI_DISPATCH(BuildRaytracingAccelerationStructure, ctx, cmd.desc);
        }
    };

    struct RHI_API CmdCopyRaytracingAccelerationStructure
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyRaytracingAccelerationStructure;
        RHICommandHeader header;
        IRHIAccelerationStructure* dest;
        IRHIAccelerationStructure* source;
        ERHIRaytracingCopyMode mode;

        static void Execute(IRHICommandContext* ctx, const CmdCopyRaytracingAccelerationStructure& cmd)
        {
            NS_RHI_DISPATCH(CopyRaytracingAccelerationStructure, ctx, cmd.dest, cmd.source, cmd.mode);
        }
    };

    struct RHI_API CmdSetRaytracingPipelineState
    {
        static constexpr ERHICommandType kType = ERHICommandType::SetRaytracingPipelineState;
        RHICommandHeader header;
        IRHIRaytracingPipelineState* pso;

        static void Execute(IRHICommandContext* ctx, const CmdSetRaytracingPipelineState& cmd)
        {
            NS_RHI_DISPATCH(SetRaytracingPipelineState, ctx, cmd.pso);
        }
    };

    struct RHI_API CmdDispatchRays
    {
        static constexpr ERHICommandType kType = ERHICommandType::DispatchRays;
        RHICommandHeader header;
        const RHIDispatchRaysDesc* desc;

        static void Execute(IRHICommandContext* ctx, const CmdDispatchRays& cmd)
        {
            NS_RHI_DISPATCH(DispatchRays, ctx, cmd.desc);
        }
    };

    //=========================================================================
    // Uploadコマンド
    //=========================================================================

    struct RHI_API CmdUploadBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::UploadBuffer;
        RHICommandHeader header;
        IRHIBuffer* dst;
        uint64 dstOffset;
        const void* srcData;
        uint64 srcSize;

        static void Execute(IRHIUploadContext* ctx, const CmdUploadBuffer& cmd)
        {
            NS_RHI_DISPATCH(UploadBuffer, ctx, cmd.dst, cmd.dstOffset, cmd.srcData, cmd.srcSize);
        }
    };

    struct RHI_API CmdUploadTexture
    {
        static constexpr ERHICommandType kType = ERHICommandType::UploadTexture;
        RHICommandHeader header;
        IRHITexture* dst;
        uint32 dstMip;
        uint32 dstSlice;
        const void* srcData;
        uint32 srcRowPitch;
        uint32 srcDepthPitch;

        static void Execute(IRHIUploadContext* ctx, const CmdUploadTexture& cmd)
        {
            NS_RHI_DISPATCH(UploadTexture, ctx, cmd.dst, cmd.dstMip, cmd.dstSlice, cmd.srcData, cmd.srcRowPitch, cmd.srcDepthPitch);
        }
    };

    struct RHI_API CmdCopyStagingToTexture
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyStagingToTexture;
        RHICommandHeader header;
        IRHITexture* dst;
        uint32 dstMip;
        uint32 dstSlice;
        Offset3D dstOffset;
        IRHIBuffer* stagingBuffer;
        uint64 stagingOffset;
        uint32 rowPitch;
        uint32 depthPitch;

        static void Execute(IRHIUploadContext* ctx, const CmdCopyStagingToTexture& cmd)
        {
            NS_RHI_DISPATCH(CopyStagingToTexture, ctx, cmd.dst, cmd.dstMip, cmd.dstSlice, cmd.dstOffset, cmd.stagingBuffer, cmd.stagingOffset, cmd.rowPitch, cmd.depthPitch);
        }
    };

    struct RHI_API CmdCopyStagingToBuffer
    {
        static constexpr ERHICommandType kType = ERHICommandType::CopyStagingToBuffer;
        RHICommandHeader header;
        IRHIBuffer* dst;
        uint64 dstOffset;
        IRHIBuffer* stagingBuffer;
        uint64 stagingOffset;
        uint64 size;

        static void Execute(IRHIUploadContext* ctx, const CmdCopyStagingToBuffer& cmd)
        {
            NS_RHI_DISPATCH(CopyStagingToBuffer, ctx, cmd.dst, cmd.dstOffset, cmd.stagingBuffer, cmd.stagingOffset, cmd.size);
        }
    };

} // namespace NS::RHI

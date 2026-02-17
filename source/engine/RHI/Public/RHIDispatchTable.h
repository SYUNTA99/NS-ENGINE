/// @file RHIDispatchTable.h
/// @brief RHIディスパッチテーブル（ゼロコスト抽象化）
/// @details 開発ビルドでは関数ポインタテーブル経由の間接呼び出し（1回分のオーバーヘッド）、
///          出荷ビルドではコンパイル時選択による直接呼び出し（インライン化可能）を実現する。
/// @see 00_Design_Philosophy §1.2
#pragma once

#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHIResourceState.h"
#include "RHITypes.h"
#include "RHIVariableRateShading.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIDispatchTable: 関数ポインタテーブル
    //=========================================================================

    /// ディスパッチテーブル
    /// 開発ビルドでのバックエンド呼び出しに使用する。
    /// 出荷ビルドでは RHI_DISPATCH マクロがコンパイル時選択に切り替わるため不使用。
    ///
    /// 設計書 §1.2:
    ///   "ディスパッチテーブル（開発時）→ コンパイル時選択（出荷時）の段階的移行を採用する"
    ///   "間接呼び出し1回だが、vtableよりシンプル（thisポインタ不要）"
    struct RHI_API RHIDispatchTable
    {
        //=====================================================================
        // Base: プロパティ
        //=====================================================================

        IRHIDevice* (*GetDevice)(IRHICommandContextBase* ctx) = nullptr;
        GPUMask (*GetGPUMask)(IRHICommandContextBase* ctx) = nullptr;
        ERHIQueueType (*GetQueueType)(IRHICommandContextBase* ctx) = nullptr;
        ERHIPipeline (*GetPipeline)(IRHICommandContextBase* ctx) = nullptr;

        //=====================================================================
        // Base: ライフサイクル
        //=====================================================================

        void (*Begin)(IRHICommandContextBase* ctx,
                      IRHICommandAllocator* allocator) = nullptr;

        IRHICommandList* (*Finish)(IRHICommandContextBase* ctx) = nullptr;

        void (*Reset)(IRHICommandContextBase* ctx) = nullptr;

        bool (*IsRecording)(IRHICommandContextBase* ctx) = nullptr;

        //=====================================================================
        // Base: リソースバリア
        //=====================================================================

        void (*TransitionResource)(IRHICommandContextBase* ctx,
                                   IRHIResource* resource,
                                   ERHIAccess stateBefore,
                                   ERHIAccess stateAfter) = nullptr;

        void (*UAVBarrier)(IRHICommandContextBase* ctx,
                           IRHIResource* resource) = nullptr;

        void (*AliasingBarrier)(IRHICommandContextBase* ctx,
                                IRHIResource* resourceBefore,
                                IRHIResource* resourceAfter) = nullptr;

        void (*FlushBarriers)(IRHICommandContextBase* ctx) = nullptr;

        //=====================================================================
        // Base: バッファコピー
        //=====================================================================

        void (*CopyBuffer)(IRHICommandContextBase* ctx,
                           IRHIBuffer* dst,
                           IRHIBuffer* src) = nullptr;

        void (*CopyBufferRegion)(IRHICommandContextBase* ctx,
                                 IRHIBuffer* dst,
                                 uint64 dstOffset,
                                 IRHIBuffer* src,
                                 uint64 srcOffset,
                                 uint64 size) = nullptr;

        //=====================================================================
        // Base: テクスチャコピー
        //=====================================================================

        void (*CopyTexture)(IRHICommandContextBase* ctx,
                            IRHITexture* dst,
                            IRHITexture* src) = nullptr;

        void (*CopyTextureRegion)(IRHICommandContextBase* ctx,
                                  IRHITexture* dst,
                                  uint32 dstMip,
                                  uint32 dstSlice,
                                  Offset3D dstOffset,
                                  IRHITexture* src,
                                  uint32 srcMip,
                                  uint32 srcSlice,
                                  const RHIBox* srcBox) = nullptr;

        //=====================================================================
        // Base: バッファ ↔ テクスチャ
        //=====================================================================

        void (*CopyBufferToTexture)(IRHICommandContextBase* ctx,
                                    IRHITexture* dst,
                                    uint32 dstMip,
                                    uint32 dstSlice,
                                    Offset3D dstOffset,
                                    IRHIBuffer* src,
                                    uint64 srcOffset,
                                    uint32 srcRowPitch,
                                    uint32 srcDepthPitch) = nullptr;

        void (*CopyTextureToBuffer)(IRHICommandContextBase* ctx,
                                    IRHIBuffer* dst,
                                    uint64 dstOffset,
                                    uint32 dstRowPitch,
                                    uint32 dstDepthPitch,
                                    IRHITexture* src,
                                    uint32 srcMip,
                                    uint32 srcSlice,
                                    const RHIBox* srcBox) = nullptr;

        //=====================================================================
        // Base: ステージングコピー
        //=====================================================================

        void (*CopyToStagingBuffer)(IRHICommandContextBase* ctx,
                                    IRHIStagingBuffer* dst,
                                    uint64 dstOffset,
                                    IRHIResource* src,
                                    uint64 srcOffset,
                                    uint64 size) = nullptr;

        //=====================================================================
        // Base: MSAA解決
        //=====================================================================

        void (*ResolveTexture)(IRHICommandContextBase* ctx,
                               IRHITexture* dst,
                               IRHITexture* src) = nullptr;

        void (*ResolveTextureRegion)(IRHICommandContextBase* ctx,
                                     IRHITexture* dst,
                                     uint32 dstMip,
                                     uint32 dstSlice,
                                     IRHITexture* src,
                                     uint32 srcMip,
                                     uint32 srcSlice) = nullptr;

        //=====================================================================
        // Base: デバッグ
        //=====================================================================

        void (*BeginDebugEvent)(IRHICommandContextBase* ctx,
                                const char* name,
                                uint32 color) = nullptr;

        void (*EndDebugEvent)(IRHICommandContextBase* ctx) = nullptr;

        void (*InsertDebugMarker)(IRHICommandContextBase* ctx,
                                  const char* name,
                                  uint32 color) = nullptr;

        //=====================================================================
        // Base: ブレッドクラム
        //=====================================================================

        void (*InsertBreadcrumb)(IRHICommandContextBase* ctx,
                                 uint32 id,
                                 const char* message) = nullptr;

        //=====================================================================
        // ImmediateContext
        //=====================================================================

        void (*Flush)(IRHIImmediateContext* ctx) = nullptr;

        void* (*GetNativeContext)(IRHIImmediateContext* ctx) = nullptr;

        //=====================================================================
        // Compute: パイプラインステート
        //=====================================================================

        void (*SetComputePipelineState)(IRHIComputeContext* ctx,
                                        IRHIComputePipelineState* pso) = nullptr;

        void (*SetComputeRootSignature)(IRHIComputeContext* ctx,
                                        IRHIRootSignature* rootSignature) = nullptr;

        //=====================================================================
        // Compute: ルート定数
        //=====================================================================

        void (*SetComputeRoot32BitConstants)(IRHIComputeContext* ctx,
                                             uint32 rootParameterIndex,
                                             uint32 num32BitValues,
                                             const void* data,
                                             uint32 destOffset) = nullptr;

        //=====================================================================
        // Compute: ルートディスクリプタ
        //=====================================================================

        void (*SetComputeRootCBV)(IRHIComputeContext* ctx,
                                  uint32 rootParameterIndex,
                                  uint64 bufferAddress) = nullptr;

        void (*SetComputeRootSRV)(IRHIComputeContext* ctx,
                                  uint32 rootParameterIndex,
                                  uint64 bufferAddress) = nullptr;

        void (*SetComputeRootUAV)(IRHIComputeContext* ctx,
                                  uint32 rootParameterIndex,
                                  uint64 bufferAddress) = nullptr;

        //=====================================================================
        // Compute: ディスクリプタヒープ
        //=====================================================================

        void (*SetDescriptorHeaps)(IRHIComputeContext* ctx,
                                   IRHIDescriptorHeap* cbvSrvUavHeap,
                                   IRHIDescriptorHeap* samplerHeap) = nullptr;

        IRHIDescriptorHeap* (*GetCBVSRVUAVHeap)(IRHIComputeContext* ctx) = nullptr;

        IRHIDescriptorHeap* (*GetSamplerHeap)(IRHIComputeContext* ctx) = nullptr;

        //=====================================================================
        // Compute: ディスクリプタテーブル
        //=====================================================================

        void (*SetComputeRootDescriptorTable)(IRHIComputeContext* ctx,
                                              uint32 rootParameterIndex,
                                              RHIGPUDescriptorHandle baseDescriptor) = nullptr;

        //=====================================================================
        // Compute: ディスパッチ
        //=====================================================================

        void (*Dispatch)(IRHIComputeContext* ctx,
                         uint32 groupCountX,
                         uint32 groupCountY,
                         uint32 groupCountZ) = nullptr;

        void (*DispatchIndirect)(IRHIComputeContext* ctx,
                                 IRHIBuffer* argsBuffer,
                                 uint64 argsOffset) = nullptr;

        void (*DispatchIndirectMulti)(IRHIComputeContext* ctx,
                                      IRHIBuffer* argsBuffer,
                                      uint64 argsOffset,
                                      uint32 dispatchCount,
                                      uint32 stride) = nullptr;

        //=====================================================================
        // Compute: UAVクリア
        //=====================================================================

        void (*ClearUnorderedAccessViewUint)(IRHIComputeContext* ctx,
                                             IRHIUnorderedAccessView* uav,
                                             const uint32 values[4]) = nullptr;

        void (*ClearUnorderedAccessViewFloat)(IRHIComputeContext* ctx,
                                              IRHIUnorderedAccessView* uav,
                                              const float values[4]) = nullptr;

        //=====================================================================
        // Compute: タイムスタンプ
        //=====================================================================

        void (*WriteTimestamp)(IRHIComputeContext* ctx,
                              IRHIQueryHeap* queryHeap,
                              uint32 queryIndex) = nullptr;

        //=====================================================================
        // Compute: クエリ
        //=====================================================================

        void (*BeginQuery)(IRHIComputeContext* ctx,
                           IRHIQueryHeap* queryHeap,
                           uint32 queryIndex) = nullptr;

        void (*EndQuery)(IRHIComputeContext* ctx,
                         IRHIQueryHeap* queryHeap,
                         uint32 queryIndex) = nullptr;

        void (*ResolveQueryData)(IRHIComputeContext* ctx,
                                 IRHIQueryHeap* queryHeap,
                                 uint32 startIndex,
                                 uint32 numQueries,
                                 IRHIBuffer* destinationBuffer,
                                 uint64 destinationOffset) = nullptr;

        bool (*GetQueryResult)(IRHIComputeContext* ctx,
                               IRHIQueryHeap* queryHeap,
                               uint32 queryIndex,
                               uint64* outResult,
                               bool bWait) = nullptr;

        //=====================================================================
        // Graphics: パイプラインステート
        //=====================================================================

        void (*SetGraphicsPipelineState)(IRHICommandContext* ctx,
                                         IRHIGraphicsPipelineState* pso) = nullptr;

        void (*SetGraphicsRootSignature)(IRHICommandContext* ctx,
                                         IRHIRootSignature* rootSignature) = nullptr;

        //=====================================================================
        // Graphics: レンダーターゲット
        //=====================================================================

        void (*SetRenderTargets)(IRHICommandContext* ctx,
                                 uint32 numRTVs,
                                 IRHIRenderTargetView* const* rtvs,
                                 IRHIDepthStencilView* dsv) = nullptr;

        //=====================================================================
        // Graphics: クリア
        //=====================================================================

        void (*ClearRenderTargetView)(IRHICommandContext* ctx,
                                      IRHIRenderTargetView* rtv,
                                      const float color[4]) = nullptr;

        void (*ClearDepthStencilView)(IRHICommandContext* ctx,
                                      IRHIDepthStencilView* dsv,
                                      bool clearDepth,
                                      float depth,
                                      bool clearStencil,
                                      uint8 stencil) = nullptr;

        //=====================================================================
        // Graphics: ビューポート・シザー
        //=====================================================================

        void (*SetViewports)(IRHICommandContext* ctx,
                             uint32 count,
                             const RHIViewport* viewports) = nullptr;

        void (*SetScissorRects)(IRHICommandContext* ctx,
                                uint32 count,
                                const RHIRect* rects) = nullptr;

        //=====================================================================
        // Graphics: 頂点・インデックスバッファ
        //=====================================================================

        void (*SetVertexBuffers)(IRHICommandContext* ctx,
                                 uint32 startSlot,
                                 uint32 numBuffers,
                                 const struct RHIVertexBufferView* views) = nullptr;

        void (*SetIndexBuffer)(IRHICommandContext* ctx,
                               const struct RHIIndexBufferView* view) = nullptr;

        void (*SetPrimitiveTopology)(IRHICommandContext* ctx,
                                     ERHIPrimitiveTopology topology) = nullptr;

        //=====================================================================
        // Graphics: 描画
        //=====================================================================

        void (*Draw)(IRHICommandContext* ctx,
                     uint32 vertexCount,
                     uint32 instanceCount,
                     uint32 startVertex,
                     uint32 startInstance) = nullptr;

        void (*DrawIndexed)(IRHICommandContext* ctx,
                            uint32 indexCount,
                            uint32 instanceCount,
                            uint32 startIndex,
                            int32 baseVertex,
                            uint32 startInstance) = nullptr;

        //=====================================================================
        // Graphics: 間接描画
        //=====================================================================

        void (*DrawIndirect)(IRHICommandContext* ctx,
                             IRHIBuffer* argsBuffer,
                             uint64 argsOffset) = nullptr;

        void (*DrawIndexedIndirect)(IRHICommandContext* ctx,
                                    IRHIBuffer* argsBuffer,
                                    uint64 argsOffset) = nullptr;

        void (*MultiDrawIndirect)(IRHICommandContext* ctx,
                                  IRHIBuffer* argsBuffer,
                                  uint32 drawCount,
                                  uint64 argsOffset,
                                  uint32 argsStride) = nullptr;

        void (*MultiDrawIndirectCount)(IRHICommandContext* ctx,
                                       IRHIBuffer* argsBuffer,
                                       uint64 argsOffset,
                                       IRHIBuffer* countBuffer,
                                       uint64 countOffset,
                                       uint32 maxDrawCount,
                                       uint32 argsStride) = nullptr;

        //=====================================================================
        // Graphics: ワークグラフ
        //=====================================================================

        void (*SetWorkGraphPipeline)(IRHICommandContext* ctx,
                                     IRHIWorkGraphPipeline* pipeline) = nullptr;

        void (*DispatchGraph)(IRHICommandContext* ctx,
                              const RHIWorkGraphDispatchDesc* desc) = nullptr;

        void (*InitializeWorkGraphBackingMemory)(IRHICommandContext* ctx,
                                                  IRHIWorkGraphPipeline* pipeline,
                                                  const RHIWorkGraphBackingMemory* memory) = nullptr;

        //=====================================================================
        // Graphics: デプスバウンド
        //=====================================================================

        void (*SetDepthBounds)(IRHICommandContext* ctx,
                               float minDepth,
                               float maxDepth) = nullptr;

        //=====================================================================
        // Graphics: ディスクリプタテーブル
        //=====================================================================

        void (*SetGraphicsRootDescriptorTable)(IRHICommandContext* ctx,
                                               uint32 rootParameterIndex,
                                               RHIGPUDescriptorHandle baseDescriptor) = nullptr;

        //=====================================================================
        // Graphics: ルートディスクリプタ直接設定
        //=====================================================================

        void (*SetGraphicsRootCBV)(IRHICommandContext* ctx,
                                   uint32 rootParameterIndex,
                                   uint64 bufferLocation) = nullptr;

        void (*SetGraphicsRootSRV)(IRHICommandContext* ctx,
                                   uint32 rootParameterIndex,
                                   uint64 bufferLocation) = nullptr;

        void (*SetGraphicsRootUAV)(IRHICommandContext* ctx,
                                   uint32 rootParameterIndex,
                                   uint64 bufferLocation) = nullptr;

        //=====================================================================
        // Graphics: ルート定数
        //=====================================================================

        void (*SetGraphicsRoot32BitConstants)(IRHICommandContext* ctx,
                                              uint32 rootParameterIndex,
                                              uint32 num32BitValues,
                                              const void* data,
                                              uint32 destOffset) = nullptr;

        //=====================================================================
        // Graphics: ブレンド・ステンシル・ライン
        //=====================================================================

        void (*SetBlendFactor)(IRHICommandContext* ctx,
                               const float factor[4]) = nullptr;

        void (*SetStencilRef)(IRHICommandContext* ctx,
                              uint32 refValue) = nullptr;

        void (*SetLineWidth)(IRHICommandContext* ctx,
                             float width) = nullptr;

        //=====================================================================
        // Graphics: Variable Rate Shading
        //=====================================================================

        void (*SetShadingRate)(IRHICommandContext* ctx,
                               ERHIShadingRate rate,
                               const ERHIVRSCombiner* combiners) = nullptr;

        void (*SetShadingRateImage)(IRHICommandContext* ctx,
                                    IRHITexture* vrsImage) = nullptr;

        //=====================================================================
        // Graphics: Reserved Resource
        //=====================================================================

        void (*CommitBuffer)(IRHICommandContext* ctx,
                             IRHIBuffer* buffer,
                             uint64 newCommitSize) = nullptr;

        void (*CommitTextureRegions)(IRHICommandContext* ctx,
                                     IRHITexture* texture,
                                     const RHITextureCommitRegion* regions,
                                     uint32 regionCount,
                                     bool commit) = nullptr;

        //=====================================================================
        // Graphics: レンダーパス
        //=====================================================================

        void (*BeginRenderPass)(IRHICommandContext* ctx,
                                const RHIRenderPassDesc* desc) = nullptr;

        void (*EndRenderPass)(IRHICommandContext* ctx) = nullptr;

        bool (*IsInRenderPass)(IRHICommandContext* ctx) = nullptr;

        const RHIRenderPassDesc* (*GetCurrentRenderPassDesc)(IRHICommandContext* ctx) = nullptr;

        void (*NextSubpass)(IRHICommandContext* ctx) = nullptr;

        uint32 (*GetCurrentSubpassIndex)(IRHICommandContext* ctx) = nullptr;

        bool (*GetRenderPassStatistics)(IRHICommandContext* ctx,
                                        RHIRenderPassStatistics* outStats) = nullptr;

        void (*ResetStatistics)(IRHICommandContext* ctx) = nullptr;

        //=====================================================================
        // Graphics: リソース状態バリア（バッチ）
        //=====================================================================

        void (*TransitionBarrier)(IRHICommandContext* ctx,
                                  IRHIResource* resource,
                                  ERHIResourceState stateBefore,
                                  ERHIResourceState stateAfter,
                                  uint32 subresource) = nullptr;

        void (*TransitionBarriers)(IRHICommandContext* ctx,
                                   const RHITransitionBarrier* barriers,
                                   uint32 count) = nullptr;

        void (*UAVBarriers)(IRHICommandContext* ctx,
                            const RHIUAVBarrier* barriers,
                            uint32 count) = nullptr;

        void (*AliasingBarriers)(IRHICommandContext* ctx,
                                 const RHIAliasingBarrier* barriers,
                                 uint32 count) = nullptr;

        //=====================================================================
        // Graphics: プレディケーション
        //=====================================================================

        void (*SetPredication)(IRHICommandContext* ctx,
                               IRHIBuffer* buffer,
                               uint64 offset,
                               ERHIPredicationOp operation) = nullptr;

        //=====================================================================
        // Graphics: ExecuteIndirect
        //=====================================================================

        void (*ExecuteIndirect)(IRHICommandContext* ctx,
                                IRHICommandSignature* commandSignature,
                                uint32 maxCommandCount,
                                IRHIBuffer* argumentBuffer,
                                uint64 argumentOffset,
                                IRHIBuffer* countBuffer,
                                uint64 countOffset) = nullptr;

        //=====================================================================
        // Graphics: Breadcrumb GPU
        //=====================================================================

        void (*BeginBreadcrumbGPU)(IRHICommandContext* ctx,
                                   RHIBreadcrumbNode* node) = nullptr;

        void (*EndBreadcrumbGPU)(IRHICommandContext* ctx,
                                 RHIBreadcrumbNode* node) = nullptr;

        //=====================================================================
        // Graphics: メッシュシェーダー
        //=====================================================================

        void (*SetMeshPipelineState)(IRHICommandContext* ctx,
                                     IRHIMeshPipelineState* pso) = nullptr;

        void (*DispatchMesh)(IRHICommandContext* ctx,
                             uint32 groupCountX,
                             uint32 groupCountY,
                             uint32 groupCountZ) = nullptr;

        void (*DispatchMeshIndirect)(IRHICommandContext* ctx,
                                     IRHIBuffer* argumentBuffer,
                                     uint64 argumentOffset) = nullptr;

        void (*DispatchMeshIndirectCount)(IRHICommandContext* ctx,
                                          IRHIBuffer* argumentBuffer,
                                          uint64 argumentOffset,
                                          IRHIBuffer* countBuffer,
                                          uint64 countOffset,
                                          uint32 maxDispatchCount) = nullptr;

        //=====================================================================
        // Graphics: レイトレーシング
        //=====================================================================

        void (*BuildRaytracingAccelerationStructure)(IRHICommandContext* ctx,
                                                      const RHIAccelerationStructureBuildDesc* desc) = nullptr;

        void (*CopyRaytracingAccelerationStructure)(IRHICommandContext* ctx,
                                                     IRHIAccelerationStructure* dest,
                                                     IRHIAccelerationStructure* source,
                                                     ERHIRaytracingCopyMode mode) = nullptr;

        void (*SetRaytracingPipelineState)(IRHICommandContext* ctx,
                                           IRHIRaytracingPipelineState* pso) = nullptr;

        void (*DispatchRays)(IRHICommandContext* ctx,
                             const RHIDispatchRaysDesc* desc) = nullptr;

        //=====================================================================
        // Upload: データ転送
        //=====================================================================

        void (*UploadBuffer)(IRHIUploadContext* ctx,
                             IRHIBuffer* dst,
                             uint64 dstOffset,
                             const void* srcData,
                             uint64 srcSize) = nullptr;

        void (*UploadTexture)(IRHIUploadContext* ctx,
                              IRHITexture* dst,
                              uint32 dstMip,
                              uint32 dstSlice,
                              const void* srcData,
                              uint32 srcRowPitch,
                              uint32 srcDepthPitch) = nullptr;

        //=====================================================================
        // Upload: ステージング転送
        //=====================================================================

        void (*CopyStagingToTexture)(IRHIUploadContext* ctx,
                                     IRHITexture* dst,
                                     uint32 dstMip,
                                     uint32 dstSlice,
                                     Offset3D dstOffset,
                                     IRHIBuffer* stagingBuffer,
                                     uint64 stagingOffset,
                                     uint32 rowPitch,
                                     uint32 depthPitch) = nullptr;

        void (*CopyStagingToBuffer)(IRHIUploadContext* ctx,
                                    IRHIBuffer* dst,
                                    uint64 dstOffset,
                                    IRHIBuffer* stagingBuffer,
                                    uint64 stagingOffset,
                                    uint64 size) = nullptr;

        //=====================================================================
        // 検証
        //=====================================================================

        /// テーブルが有効か（全関数ポインタが設定済みか）
        /// 必須エントリが全て設定されているか（初期化時チェック用）
        [[nodiscard]] bool IsValid() const
        {
            // Base: プロパティ・ライフサイクル
            return GetDevice != nullptr
                && GetGPUMask != nullptr
                && GetQueueType != nullptr
                && GetPipeline != nullptr
                && Begin != nullptr
                && Finish != nullptr
                && Reset != nullptr
                && IsRecording != nullptr
                // Base: バリア
                && TransitionResource != nullptr
                && UAVBarrier != nullptr
                && AliasingBarrier != nullptr
                && FlushBarriers != nullptr
                // Base: バッファコピー
                && CopyBuffer != nullptr
                && CopyBufferRegion != nullptr
                // Base: テクスチャコピー
                && CopyTexture != nullptr
                && CopyTextureRegion != nullptr
                // Base: バッファ ↔ テクスチャ
                && CopyBufferToTexture != nullptr
                && CopyTextureToBuffer != nullptr
                // Base: ステージングコピー
                && CopyToStagingBuffer != nullptr
                // Base: MSAA解決
                && ResolveTexture != nullptr
                && ResolveTextureRegion != nullptr
                // Base: デバッグ
                && BeginDebugEvent != nullptr
                && EndDebugEvent != nullptr
                && InsertDebugMarker != nullptr
                // Base: ブレッドクラム
                && InsertBreadcrumb != nullptr
                // ImmediateContext
                && Flush != nullptr
                && GetNativeContext != nullptr
                // Compute: パイプラインステート
                && SetComputePipelineState != nullptr
                && SetComputeRootSignature != nullptr
                // Compute: ルート定数
                && SetComputeRoot32BitConstants != nullptr
                // Compute: ルートディスクリプタ
                && SetComputeRootCBV != nullptr
                && SetComputeRootSRV != nullptr
                && SetComputeRootUAV != nullptr
                // Compute: ディスクリプタヒープ
                && SetDescriptorHeaps != nullptr
                && GetCBVSRVUAVHeap != nullptr
                && GetSamplerHeap != nullptr
                // Compute: ディスクリプタテーブル
                && SetComputeRootDescriptorTable != nullptr
                // Compute: ディスパッチ
                && Dispatch != nullptr
                && DispatchIndirect != nullptr
                && DispatchIndirectMulti != nullptr
                // Compute: UAVクリア
                && ClearUnorderedAccessViewUint != nullptr
                && ClearUnorderedAccessViewFloat != nullptr
                // Compute: タイムスタンプ
                && WriteTimestamp != nullptr
                // Compute: クエリ
                && BeginQuery != nullptr
                && EndQuery != nullptr
                && ResolveQueryData != nullptr
                && GetQueryResult != nullptr
                // Graphics: パイプラインステート
                && SetGraphicsPipelineState != nullptr
                && SetGraphicsRootSignature != nullptr
                // Graphics: レンダーターゲット
                && SetRenderTargets != nullptr
                // Graphics: クリア
                && ClearRenderTargetView != nullptr
                && ClearDepthStencilView != nullptr
                // Graphics: ビューポート・シザー
                && SetViewports != nullptr
                && SetScissorRects != nullptr
                // Graphics: 頂点・インデックスバッファ
                && SetVertexBuffers != nullptr
                && SetIndexBuffer != nullptr
                && SetPrimitiveTopology != nullptr
                // Graphics: 描画
                && Draw != nullptr
                && DrawIndexed != nullptr
                // Graphics: 間接描画
                && DrawIndirect != nullptr
                && DrawIndexedIndirect != nullptr
                && MultiDrawIndirect != nullptr
                && MultiDrawIndirectCount != nullptr
                // Graphics: デプスバウンド
                && SetDepthBounds != nullptr
                // Graphics: ディスクリプタテーブル
                && SetGraphicsRootDescriptorTable != nullptr
                // Graphics: ルートディスクリプタ直接設定
                && SetGraphicsRootCBV != nullptr
                && SetGraphicsRootSRV != nullptr
                && SetGraphicsRootUAV != nullptr
                // Graphics: ルート定数
                && SetGraphicsRoot32BitConstants != nullptr
                // Graphics: ブレンド・ステンシル・ライン
                && SetBlendFactor != nullptr
                && SetStencilRef != nullptr
                && SetLineWidth != nullptr
                // Graphics: Reserved Resource
                && CommitBuffer != nullptr
                && CommitTextureRegions != nullptr
                // Graphics: レンダーパス
                && BeginRenderPass != nullptr
                && EndRenderPass != nullptr
                && IsInRenderPass != nullptr
                && GetCurrentRenderPassDesc != nullptr
                && NextSubpass != nullptr
                && GetCurrentSubpassIndex != nullptr
                && GetRenderPassStatistics != nullptr
                && ResetStatistics != nullptr
                // Graphics: リソース状態バリア（バッチ）
                && TransitionBarrier != nullptr
                && TransitionBarriers != nullptr
                && UAVBarriers != nullptr
                && AliasingBarriers != nullptr
                // Graphics: プレディケーション
                && SetPredication != nullptr
                // Graphics: ExecuteIndirect
                && ExecuteIndirect != nullptr
                // Graphics: Breadcrumb GPU
                && BeginBreadcrumbGPU != nullptr
                && EndBreadcrumbGPU != nullptr
                // Upload: データ転送
                && UploadBuffer != nullptr
                && UploadTexture != nullptr
                // Upload: ステージング転送
                && CopyStagingToTexture != nullptr
                && CopyStagingToBuffer != nullptr;
        }

        /// オプショナルエントリ（ケイパビリティチェック後に使用）
        /// バックエンドが機能をサポートしない場合はnull許容
        [[nodiscard]] bool HasMeshShaderSupport() const
        {
            return SetMeshPipelineState != nullptr
                && DispatchMesh != nullptr
                && DispatchMeshIndirect != nullptr
                && DispatchMeshIndirectCount != nullptr;
        }

        [[nodiscard]] bool HasRayTracingSupport() const
        {
            return BuildRaytracingAccelerationStructure != nullptr
                && CopyRaytracingAccelerationStructure != nullptr
                && SetRaytracingPipelineState != nullptr
                && DispatchRays != nullptr;
        }

        [[nodiscard]] bool HasWorkGraphSupport() const
        {
            return SetWorkGraphPipeline != nullptr
                && DispatchGraph != nullptr
                && InitializeWorkGraphBackingMemory != nullptr;
        }

        [[nodiscard]] bool HasVariableRateShadingSupport() const
        {
            return SetShadingRate != nullptr
                && SetShadingRateImage != nullptr;
        }
    };

    //=========================================================================
    // グローバルディスパッチテーブル
    //=========================================================================

    /// グローバルディスパッチテーブル（開発ビルド用）
    /// バックエンド初期化時に関数ポインタが設定される
    extern RHI_API RHIDispatchTable GRHIDispatchTable;

} // namespace NS::RHI

//=============================================================================
// ディスパッチマクロ
//=============================================================================
// 設計書 §1.2:
//   方式1（開発時）: GDispatch.Draw(ctx, ...)  — 関数ポインタ間接呼び出し
//   方式2（出荷時）: #define RHI_Draw D3D12_Draw — 直接呼び出し（LTOでインライン化）

#if NS_BUILD_SHIPPING && defined(NS_RHI_STATIC_BACKEND)
    //=========================================================================
    // 出荷ビルド: コンパイル時バックエンド選択（ゼロオーバーヘッド）
    //=========================================================================
    // バックエンドモジュール側で NS_RHI_STATIC_BACKEND が定義される。
    // 例: #define NS_RHI_STATIC_BACKEND D3D12
    //     → NS_RHI_DISPATCH(Draw, ...) が D3D12_Draw(...) に展開される
    //
    // LTO（リンク時最適化）により直接呼び出しがインライン化可能。
    // CPUコスト: ネイティブAPI直接呼び出しの1.05倍以内（設計目標）

    #define NS_RHI_PASTE(a, b) a##_##b
    #define NS_RHI_EXPAND(backend, func) NS_RHI_PASTE(backend, func)

    /// 出荷ビルド用ディスパッチマクロ
    /// バックエンド関数を直接呼び出す（インライン化可能）
    #define NS_RHI_DISPATCH(func, ...) \
        ::NS::RHI::NS_RHI_EXPAND(NS_RHI_STATIC_BACKEND, func)(__VA_ARGS__)

#else
    //=========================================================================
    // 開発ビルド: ディスパッチテーブル経由（関数ポインタ1回分）
    //=========================================================================
    // CPUコスト: 関数ポインタ間接呼び出し1回（1.2倍程度）
    // ランタイムでのバックエンド切替を許容する

    /// 開発ビルド用ディスパッチマクロ
    /// グローバルディスパッチテーブルの関数ポインタを呼び出す
    #define NS_RHI_DISPATCH(func, ...) \
        ::NS::RHI::GRHIDispatchTable.func(__VA_ARGS__)

#endif

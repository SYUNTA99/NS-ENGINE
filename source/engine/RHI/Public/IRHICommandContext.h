/// @file IRHICommandContext.h
/// @brief グラフィックスコマンドコンテキストインターフェース
/// @details グラフィックス+コンピュート機能を持つフルコンテキスト。描画、レンダーターゲット、ビューポート等を提供。
/// @see 02-03-graphics-context.md
#pragma once

#include "IRHIComputeContext.h"
#include "RHIBarrier.h"
#include "RHIRenderPass.h"
#include "RHIVariableRateShading.h"
#include "RHIWorkGraphTypes.h"

namespace NS
{
    namespace RHI
    {
        //=========================================================================
        // RHIVertexBufferView / RHIIndexBufferView
        //=========================================================================

        /// 頂点バッファビュー
        struct RHIVertexBufferView
        {
            uint64 bufferAddress = 0; ///< GPUアドレス
            uint32 size = 0;          ///< バッファサイズ（バイト）
            uint32 stride = 0;        ///< 頂点ストライド（バイト）
        };

        /// インデックスバッファビュー
        struct RHIIndexBufferView
        {
            uint64 bufferAddress = 0;                         ///< GPUアドレス
            uint32 size = 0;                                  ///< バッファサイズ（バイト）
            ERHIIndexFormat format = ERHIIndexFormat::UInt16; ///< インデックスフォーマット
        };

        //=========================================================================
        // IRHICommandContext
        //=========================================================================

        /// グラフィックスコマンドコンテキスト
        /// グラフィックス + コンピュート機能を持つフルコンテキスト
        class RHI_API IRHICommandContext : public IRHIComputeContext
        {
        public:
            virtual ~IRHICommandContext() = default;

            //=====================================================================
            // グラフィックスパイプライン
            //=====================================================================

            /// グラフィックスパイプラインステート設定
            void SetGraphicsPipelineState(IRHIGraphicsPipelineState* pso)
            {
                NS_RHI_DISPATCH(SetGraphicsPipelineState, this, pso);
            }

            /// グラフィックス用ルートシグネチャ設定
            void SetGraphicsRootSignature(IRHIRootSignature* rootSignature)
            {
                NS_RHI_DISPATCH(SetGraphicsRootSignature, this, rootSignature);
            }

            //=====================================================================
            // レンダーターゲット
            //=====================================================================

            /// レンダーターゲット設定
            /// @param numRTVs レンダーターゲット数
            /// @param rtvs レンダーターゲットビュー配列
            /// @param dsv デプスステンシルビュー（nullptrで無効）
            void SetRenderTargets(uint32 numRTVs,
                                  IRHIRenderTargetView* const* rtvs,
                                  IRHIDepthStencilView* dsv)
            {
                NS_RHI_DISPATCH(SetRenderTargets, this, numRTVs, rtvs, dsv);
            }

            /// 単一レンダーターゲット設定（便利関数）
            void SetRenderTarget(IRHIRenderTargetView* rtv, IRHIDepthStencilView* dsv = nullptr)
            {
                SetRenderTargets(rtv ? 1 : 0, rtv ? &rtv : nullptr, dsv);
            }

            //=====================================================================
            // クリア
            //=====================================================================

            /// レンダーターゲットクリア
            void ClearRenderTargetView(IRHIRenderTargetView* rtv, const float color[4])
            {
                NS_RHI_DISPATCH(ClearRenderTargetView, this, rtv, color);
            }

            /// デプスステンシルクリア
            void ClearDepthStencilView(
                IRHIDepthStencilView* dsv, bool clearDepth, float depth, bool clearStencil, uint8 stencil)
            {
                NS_RHI_DISPATCH(ClearDepthStencilView, this, dsv, clearDepth, depth, clearStencil, stencil);
            }

            //=====================================================================
            // ビューポート
            //=====================================================================

            /// ビューポート設定
            void SetViewports(uint32 numViewports, const RHIViewport* viewports)
            {
                NS_RHI_DISPATCH(SetViewports, this, numViewports, viewports);
            }

            /// 単一ビューポート設定
            void SetViewport(const RHIViewport& viewport) { SetViewports(1, &viewport); }

            //=====================================================================
            // シザー矩形
            //=====================================================================

            /// シザー矩形設定
            void SetScissorRects(uint32 numRects, const RHIRect* rects)
            {
                NS_RHI_DISPATCH(SetScissorRects, this, numRects, rects);
            }

            /// 単一シザー矩形設定
            void SetScissorRect(const RHIRect& rect) { SetScissorRects(1, &rect); }

            //=====================================================================
            // 頂点バッファ
            //=====================================================================

            /// 頂点バッファ設定
            void SetVertexBuffers(uint32 startSlot, uint32 numBuffers, const RHIVertexBufferView* views)
            {
                NS_RHI_DISPATCH(SetVertexBuffers, this, startSlot, numBuffers, views);
            }

            /// 単一頂点バッファ設定
            void SetVertexBuffer(uint32 slot, const RHIVertexBufferView& view) { SetVertexBuffers(slot, 1, &view); }

            //=====================================================================
            // インデックスバッファ
            //=====================================================================

            /// インデックスバッファ設定
            void SetIndexBuffer(const RHIIndexBufferView& view) { NS_RHI_DISPATCH(SetIndexBuffer, this, &view); }

            //=====================================================================
            // プリミティブトポロジー
            //=====================================================================

            /// プリミティブトポロジー設定
            void SetPrimitiveTopology(ERHIPrimitiveTopology topology)
            {
                NS_RHI_DISPATCH(SetPrimitiveTopology, this, topology);
            }

            //=====================================================================
            // 描画
            //=====================================================================

            /// 描画
            void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 startVertex = 0, uint32 startInstance = 0)
            {
                NS_RHI_DISPATCH(Draw, this, vertexCount, instanceCount, startVertex, startInstance);
            }

            /// インデックス付き描画
            void DrawIndexed(uint32 indexCount,
                             uint32 instanceCount = 1,
                             uint32 startIndex = 0,
                             int32 baseVertex = 0,
                             uint32 startInstance = 0)
            {
                NS_RHI_DISPATCH(DrawIndexed, this, indexCount, instanceCount, startIndex, baseVertex, startInstance);
            }

            //=====================================================================
            // 間接描画
            //=====================================================================

            /// 間接描画
            void DrawIndirect(IRHIBuffer* argsBuffer, uint64 argsOffset = 0)
            {
                NS_RHI_DISPATCH(DrawIndirect, this, argsBuffer, argsOffset);
            }

            /// 間接インデックス付き描画
            void DrawIndexedIndirect(IRHIBuffer* argsBuffer, uint64 argsOffset = 0)
            {
                NS_RHI_DISPATCH(DrawIndexedIndirect, this, argsBuffer, argsOffset);
            }

            /// 複数間接描画
            void MultiDrawIndirect(IRHIBuffer* argsBuffer,
                                   uint32 drawCount,
                                   uint64 argsOffset = 0,
                                   uint32 argsStride = 0)
            {
                NS_RHI_DISPATCH(MultiDrawIndirect, this, argsBuffer, drawCount, argsOffset, argsStride);
            }

            /// カウント付き複数間接描画
            void MultiDrawIndirectCount(IRHIBuffer* argsBuffer,
                                        uint64 argsOffset,
                                        IRHIBuffer* countBuffer,
                                        uint64 countOffset,
                                        uint32 maxDrawCount,
                                        uint32 argsStride)
            {
                NS_RHI_DISPATCH(MultiDrawIndirectCount,
                                this,
                                argsBuffer,
                                argsOffset,
                                countBuffer,
                                countOffset,
                                maxDrawCount,
                                argsStride);
            }

            //=====================================================================
            // ワークグラフ (02-06)
            //=====================================================================

            /// ワークグラフパイプライン設定
            void SetWorkGraphPipeline(IRHIWorkGraphPipeline* pipeline)
            {
                NS_RHI_DISPATCH(SetWorkGraphPipeline, this, pipeline);
            }

            /// ワークグラフディスパッチ
            void DispatchGraph(const RHIWorkGraphDispatchDesc& desc)
            {
                NS_RHI_DISPATCH(DispatchGraph, this, &desc);
            }

            /// バッキングメモリ初期化
            void InitializeWorkGraphBackingMemory(IRHIWorkGraphPipeline* pipeline,
                                                  const RHIWorkGraphBackingMemory& memory)
            {
                NS_RHI_DISPATCH(InitializeWorkGraphBackingMemory, this, pipeline, &memory);
            }

            //=====================================================================
            // デプスバウンド (05-04)
            //=====================================================================

            /// デプスバウンドテスト設定
            /// @note ハードウェアサポートが必要
            void SetDepthBounds(float minDepth, float maxDepth)
            {
                NS_RHI_DISPATCH(SetDepthBounds, this, minDepth, maxDepth);
            }

            /// デプスバウンド無効化
            void DisableDepthBounds() { SetDepthBounds(0.0f, 1.0f); }

            //=====================================================================
            // クリア便利関数 (05-03, 05-04)
            //=====================================================================

            /// デプスのみクリア
            void ClearDepth(IRHIDepthStencilView* dsv, float depth = 1.0f)
            {
                ClearDepthStencilView(dsv, true, depth, false, 0);
            }

            /// ステンシルのみクリア
            void ClearStencil(IRHIDepthStencilView* dsv, uint8 stencil = 0)
            {
                ClearDepthStencilView(dsv, false, 1.0f, true, stencil);
            }

            //=====================================================================
            // グラフィックスディスクリプタテーブル (10-02)
            //=====================================================================

            /// グラフィックスルートディスクリプタテーブル設定
            void SetGraphicsRootDescriptorTable(uint32 rootParameterIndex,
                                                RHIGPUDescriptorHandle baseDescriptor)
            {
                NS_RHI_DISPATCH(SetGraphicsRootDescriptorTable, this, rootParameterIndex, baseDescriptor);
            }

            //=====================================================================
            // グラフィックスルートディスクリプタ直接設定 (10-02)
            //=====================================================================

            /// グラフィックスルートCBV設定
            void SetGraphicsRootConstantBufferView(uint32 rootParameterIndex, uint64 bufferLocation)
            {
                NS_RHI_DISPATCH(SetGraphicsRootCBV, this, rootParameterIndex, bufferLocation);
            }

            /// グラフィックスルートSRV設定
            void SetGraphicsRootShaderResourceView(uint32 rootParameterIndex, uint64 bufferLocation)
            {
                NS_RHI_DISPATCH(SetGraphicsRootSRV, this, rootParameterIndex, bufferLocation);
            }

            /// グラフィックスルートUAV設定
            void SetGraphicsRootUnorderedAccessView(uint32 rootParameterIndex, uint64 bufferLocation)
            {
                NS_RHI_DISPATCH(SetGraphicsRootUAV, this, rootParameterIndex, bufferLocation);
            }

            //=====================================================================
            // グラフィックスルート定数 (05-05)
            //=====================================================================

            /// グラフィックス用ルート定数設定
            void SetGraphicsRoot32BitConstants(uint32 rootParameterIndex,
                                               uint32 num32BitValues,
                                               const void* data,
                                               uint32 destOffset = 0)
            {
                NS_RHI_DISPATCH(SetGraphicsRoot32BitConstants, this, rootParameterIndex, num32BitValues, data, destOffset);
            }

            /// 型付きグラフィックスルート定数設定
            template <typename T> void SetGraphicsRootConstants(uint32 rootIndex, const T& value)
            {
                static_assert(sizeof(T) % 4 == 0, "Size must be multiple of 4 bytes");
                SetGraphicsRoot32BitConstants(rootIndex, sizeof(T) / 4, &value);
            }

            //=====================================================================
            // ブレンドファクター (07-01)
            //=====================================================================

            /// ブレンドファクター設定
            void SetBlendFactor(const float factor[4])
            {
                NS_RHI_DISPATCH(SetBlendFactor, this, factor);
            }

            //=====================================================================
            // ステンシル参照値 (07-03)
            //=====================================================================

            /// ステンシル参照値設定
            void SetStencilRef(uint32 refValue)
            {
                NS_RHI_DISPATCH(SetStencilRef, this, refValue);
            }

            //=====================================================================
            // ライン幅 (07-02)
            //=====================================================================

            /// ライン幅設定
            void SetLineWidth(float width)
            {
                NS_RHI_DISPATCH(SetLineWidth, this, width);
            }

            //=====================================================================
            // ビューポート便利関数 (07-02)
            //=====================================================================

            /// ビューポートとシザーを同時設定
            void SetViewportAndScissor(const RHIViewport& viewport)
            {
                SetViewport(viewport);
                RHIRect scissor = RHIRect::FromExtent(static_cast<int32>(viewport.x),
                                                      static_cast<int32>(viewport.y),
                                                      static_cast<uint32>(viewport.width),
                                                      static_cast<uint32>(viewport.height));
                SetScissorRect(scissor);
            }

            //=====================================================================
            // Variable Rate Shading (07-07)
            //=====================================================================

            /// パイプラインシェーディングレート設定
            void SetShadingRate(ERHIShadingRate rate, const ERHIVRSCombiner* combiners = nullptr)
            {
                NS_RHI_DISPATCH(SetShadingRate, this, rate, combiners);
            }

            /// VRSイメージ設定
            void SetShadingRateImage(IRHITexture* vrsImage)
            {
                NS_RHI_DISPATCH(SetShadingRateImage, this, vrsImage);
            }

            //=====================================================================
            // Reserved Resource (11-06)
            //=====================================================================

            /// バッファのコミットサイズ変更
            /// @param buffer 対象バッファ（ReservedResource）
            /// @param newCommitSize 新しいコミットサイズ
            void CommitBuffer(IRHIBuffer* buffer, uint64 newCommitSize)
            {
                NS_RHI_DISPATCH(CommitBuffer, this, buffer, newCommitSize);
            }

            /// テクスチャ領域のコミット/デコミット
            /// @param texture 対象テクスチャ（ReservedResource）
            /// @param regions コミット領域配列
            /// @param regionCount 領域数
            /// @param commit true=コミット、false=デコミット
            void CommitTextureRegions(IRHITexture* texture,
                                      const RHITextureCommitRegion* regions,
                                      uint32 regionCount,
                                      bool commit)
            {
                NS_RHI_DISPATCH(CommitTextureRegions, this, texture, regions, regionCount, commit);
            }

            //=====================================================================
            // レンダーパス (13-01, 13-03)
            //=====================================================================

            /// レンダーパス開始
            void BeginRenderPass(const RHIRenderPassDesc& desc)
            {
                NS_RHI_DISPATCH(BeginRenderPass, this, &desc);
            }

            /// レンダーパス終了
            void EndRenderPass()
            {
                NS_RHI_DISPATCH(EndRenderPass, this);
            }

            /// レンダーパス中か
            virtual bool IsInRenderPass() const = 0;

            /// 現在のレンダーパス記述取得
            virtual const RHIRenderPassDesc* GetCurrentRenderPassDesc() const = 0;

            /// 次のサブパスへ進む
            void NextSubpass()
            {
                NS_RHI_DISPATCH(NextSubpass, this);
            }

            /// 現在のサブパスインデックス取得
            virtual uint32 GetCurrentSubpassIndex() const = 0;

            /// レンダーパス統計取得
            virtual bool GetRenderPassStatistics(RHIRenderPassStatistics& outStats) const = 0;

            /// 統計リセット
            virtual void ResetStatistics() = 0;

            //=====================================================================
            // リソース状態バリア (16-02)
            //=====================================================================

            /// リソース状態遷移バリア発行
            void TransitionBarrier(IRHIResource* resource,
                                   ERHIResourceState stateBefore,
                                   ERHIResourceState stateAfter,
                                   uint32 subresource = kAllSubresources)
            {
                NS_RHI_DISPATCH(TransitionBarrier, this, resource, stateBefore, stateAfter, subresource);
            }

            /// 複数遷移バリア発行
            void TransitionBarriers(const RHITransitionBarrier* barriers, uint32 count)
            {
                NS_RHI_DISPATCH(TransitionBarriers, this, barriers, count);
            }

            /// 複数UAVバリア発行
            void UAVBarriers(const RHIUAVBarrier* barriers, uint32 count)
            {
                NS_RHI_DISPATCH(UAVBarriers, this, barriers, count);
            }

            /// 複数エイリアシングバリア発行
            void AliasingBarriers(const RHIAliasingBarrier* barriers, uint32 count)
            {
                NS_RHI_DISPATCH(AliasingBarriers, this, barriers, count);
            }

            /// エイリアシングバリアバッチ発行 (23-04)
            void AliasingBarriers(const class RHIAliasingBarrierBatch& batch);

            //=====================================================================
            // プレディケーション (14-04)
            //=====================================================================

            /// プレディケーション設定
            /// @param buffer クエリ結果バッファ
            /// @param offset バッファ内のオフセット
            /// @param operation 比較操作
            void SetPredication(IRHIBuffer* buffer, uint64 offset, ERHIPredicationOp operation)
            {
                NS_RHI_DISPATCH(SetPredication, this, buffer, offset, operation);
            }

            /// プレディケーション解除
            void ClearPredication() { SetPredication(nullptr, 0, ERHIPredicationOp::EqualZero); }

            //=====================================================================
            // ExecuteIndirect (21-05)
            //=====================================================================

            /// ExecuteIndirect
            /// コマンドシグネチャに基づくGPU駆動コマンドを実行
            /// @param commandSignature コマンドシグネチャ
            /// @param maxCommandCount 最大コマンド数
            /// @param argumentBuffer 引数バッファ
            /// @param argumentOffset 引数バッファオフセット
            /// @param countBuffer カウントバッファ（nullptr=maxCommandCount使用）
            /// @param countOffset カウントバッファオフセット
            void ExecuteIndirect(IRHICommandSignature* commandSignature,
                                 uint32 maxCommandCount,
                                 IRHIBuffer* argumentBuffer,
                                 uint64 argumentOffset,
                                 IRHIBuffer* countBuffer = nullptr,
                                 uint64 countOffset = 0)
            {
                NS_RHI_DISPATCH(ExecuteIndirect, this, commandSignature, maxCommandCount, argumentBuffer, argumentOffset, countBuffer, countOffset);
            }

            //=====================================================================
            // Breadcrumb (17-04)
            //=====================================================================

            /// Breadcrumb開始
            void BeginBreadcrumbGPU(RHIBreadcrumbNode* node)
            {
                NS_RHI_DISPATCH(BeginBreadcrumbGPU, this, node);
            }

            /// Breadcrumb終了
            void EndBreadcrumbGPU(RHIBreadcrumbNode* node)
            {
                NS_RHI_DISPATCH(EndBreadcrumbGPU, this, node);
            }

            //=====================================================================
            // メッシュシェーダー (22-01, 22-04)
            //=====================================================================

            /// メッシュパイプラインステート設定
            void SetMeshPipelineState(IRHIMeshPipelineState* pso)
            {
                NS_RHI_DISPATCH(SetMeshPipelineState, this, pso);
            }

            /// メッシュシェーダーディスパッチ
            /// @param groupCountX X方向グループ数
            /// @param groupCountY Y方向グループ数
            /// @param groupCountZ Z方向グループ数
            void DispatchMesh(uint32 groupCountX, uint32 groupCountY = 1, uint32 groupCountZ = 1)
            {
                NS_RHI_DISPATCH(DispatchMesh, this, groupCountX, groupCountY, groupCountZ);
            }

            /// 間接メッシュシェーダーディスパッチ
            /// @param argumentBuffer 引数バッファ（RHIDispatchMeshArguments）
            /// @param argumentOffset バッファオフセット
            void DispatchMeshIndirect(IRHIBuffer* argumentBuffer, uint64 argumentOffset = 0)
            {
                NS_RHI_DISPATCH(DispatchMeshIndirect, this, argumentBuffer, argumentOffset);
            }

            /// カウントバッファ付きIndirectメッシュシェーダーディスパッチ
            /// @param argumentBuffer 引数バッファ
            /// @param argumentOffset 引数バッファオフセット
            /// @param countBuffer カウントバッファ
            /// @param countOffset カウントバッファオフセット
            /// @param maxDispatchCount 最大ディスパッチ数
            void DispatchMeshIndirectCount(IRHIBuffer* argumentBuffer,
                                           uint64 argumentOffset,
                                           IRHIBuffer* countBuffer,
                                           uint64 countOffset,
                                           uint32 maxDispatchCount)
            {
                NS_RHI_DISPATCH(DispatchMeshIndirectCount, this, argumentBuffer, argumentOffset, countBuffer, countOffset, maxDispatchCount);
            }

            //=====================================================================
            // レイトレーシング (19-01〜19-03)
            //=====================================================================

            /// 加速構造ビルド
            /// @param desc ビルド記述
            void BuildRaytracingAccelerationStructure(const RHIAccelerationStructureBuildDesc& desc)
            {
                NS_RHI_DISPATCH(BuildRaytracingAccelerationStructure, this, &desc);
            }

            /// 加速構造コピー
            /// @param dest コピー先
            /// @param source コピー元
            /// @param mode コピーモード
            void CopyRaytracingAccelerationStructure(IRHIAccelerationStructure* dest,
                                                     IRHIAccelerationStructure* source,
                                                     ERHIRaytracingCopyMode mode)
            {
                NS_RHI_DISPATCH(CopyRaytracingAccelerationStructure, this, dest, source, mode);
            }

            /// レイトレーシングPSO設定
            void SetRaytracingPipelineState(IRHIRaytracingPipelineState* pso)
            {
                NS_RHI_DISPATCH(SetRaytracingPipelineState, this, pso);
            }

            /// レイディスパッチ
            /// @param desc DispatchRays記述
            void DispatchRays(const RHIDispatchRaysDesc& desc)
            {
                NS_RHI_DISPATCH(DispatchRays, this, &desc);
            }
        };

        //=========================================================================
        // RHIScopedRenderPass インライン実装 (13-03)
        //=========================================================================

        inline RHIScopedRenderPass::RHIScopedRenderPass(IRHICommandContext* context, const RHIRenderPassDesc& desc)
            : m_context(context)
        {
            if (m_context)
            {
                m_context->BeginRenderPass(desc);
            }
        }

        inline RHIScopedRenderPass::~RHIScopedRenderPass()
        {
            if (m_context)
            {
                m_context->EndRenderPass();
            }
        }

    } // namespace RHI
} // namespace NS

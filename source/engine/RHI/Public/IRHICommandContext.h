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

namespace NS { namespace RHI {
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
        virtual void SetGraphicsPipelineState(IRHIGraphicsPipelineState* pso) = 0;

        /// グラフィックス用ルートシグネチャ設定
        virtual void SetGraphicsRootSignature(IRHIRootSignature* rootSignature) = 0;

        //=====================================================================
        // レンダーターゲット
        //=====================================================================

        /// レンダーターゲット設定
        /// @param numRTVs レンダーターゲット数
        /// @param rtvs レンダーターゲットビュー配列
        /// @param dsv デプスステンシルビュー（nullptrで無効）
        virtual void SetRenderTargets(uint32 numRTVs, IRHIRenderTargetView* const* rtvs, IRHIDepthStencilView* dsv) = 0;

        /// 単一レンダーターゲット設定（便利関数）
        void SetRenderTarget(IRHIRenderTargetView* rtv, IRHIDepthStencilView* dsv = nullptr)
        {
            SetRenderTargets(rtv ? 1 : 0, rtv ? &rtv : nullptr, dsv);
        }

        //=====================================================================
        // クリア
        //=====================================================================

        /// レンダーターゲットクリア
        virtual void ClearRenderTargetView(IRHIRenderTargetView* rtv, const float color[4]) = 0;

        /// デプスステンシルクリア
        virtual void ClearDepthStencilView(
            IRHIDepthStencilView* dsv, bool clearDepth, float depth, bool clearStencil, uint8 stencil) = 0;

        //=====================================================================
        // ビューポート
        //=====================================================================

        /// ビューポート設定
        virtual void SetViewports(uint32 numViewports, const RHIViewport* viewports) = 0;

        /// 単一ビューポート設定
        void SetViewport(const RHIViewport& viewport) { SetViewports(1, &viewport); }

        //=====================================================================
        // シザー矩形
        //=====================================================================

        /// シザー矩形設定
        virtual void SetScissorRects(uint32 numRects, const RHIRect* rects) = 0;

        /// 単一シザー矩形設定
        void SetScissorRect(const RHIRect& rect) { SetScissorRects(1, &rect); }

        //=====================================================================
        // 頂点バッファ
        //=====================================================================

        /// 頂点バッファ設定
        virtual void SetVertexBuffers(uint32 startSlot, uint32 numBuffers, const RHIVertexBufferView* views) = 0;

        /// 単一頂点バッファ設定
        void SetVertexBuffer(uint32 slot, const RHIVertexBufferView& view) { SetVertexBuffers(slot, 1, &view); }

        //=====================================================================
        // インデックスバッファ
        //=====================================================================

        /// インデックスバッファ設定
        virtual void SetIndexBuffer(const RHIIndexBufferView& view) = 0;

        //=====================================================================
        // プリミティブトポロジー
        //=====================================================================

        /// プリミティブトポロジー設定
        virtual void SetPrimitiveTopology(ERHIPrimitiveTopology topology) = 0;

        //=====================================================================
        // 描画
        //=====================================================================

        /// 描画
        virtual void Draw(uint32 vertexCount,
                          uint32 instanceCount = 1,
                          uint32 startVertex = 0,
                          uint32 startInstance = 0) = 0;

        /// インデックス付き描画
        virtual void DrawIndexed(uint32 indexCount,
                                 uint32 instanceCount = 1,
                                 uint32 startIndex = 0,
                                 int32 baseVertex = 0,
                                 uint32 startInstance = 0) = 0;

        //=====================================================================
        // 間接描画
        //=====================================================================

        /// 間接描画
        virtual void DrawIndirect(IRHIBuffer* argsBuffer, uint64 argsOffset = 0) = 0;

        /// 間接インデックス付き描画
        virtual void DrawIndexedIndirect(IRHIBuffer* argsBuffer, uint64 argsOffset = 0) = 0;

        /// 複数間接描画
        virtual void MultiDrawIndirect(IRHIBuffer* argsBuffer,
                                       uint32 drawCount,
                                       uint64 argsOffset = 0,
                                       uint32 argsStride = 0) = 0;

        /// カウント付き複数間接描画
        virtual void MultiDrawIndirectCount(IRHIBuffer* argsBuffer,
                                            uint64 argsOffset,
                                            IRHIBuffer* countBuffer,
                                            uint64 countOffset,
                                            uint32 maxDrawCount,
                                            uint32 argsStride) = 0;

        //=====================================================================
        // ワークグラフ (02-06)
        //=====================================================================

        /// ワークグラフパイプライン設定
        virtual void SetWorkGraphPipeline(IRHIWorkGraphPipeline* pipeline) = 0;

        /// ワークグラフディスパッチ
        virtual void DispatchGraph(const RHIWorkGraphDispatchDesc& desc) = 0;

        /// バッキングメモリ初期化
        virtual void InitializeWorkGraphBackingMemory(IRHIWorkGraphPipeline* pipeline,
                                                      const RHIWorkGraphBackingMemory& memory) = 0;

        //=====================================================================
        // デプスバウンド (05-04)
        //=====================================================================

        /// デプスバウンドテスト設定
        /// @note ハードウェアサポートが必要
        virtual void SetDepthBounds(float minDepth, float maxDepth) = 0;

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
        virtual void SetGraphicsRootDescriptorTable(uint32 rootParameterIndex,
                                                    RHIGPUDescriptorHandle baseDescriptor) = 0;

        //=====================================================================
        // グラフィックスルートディスクリプタ直接設定 (10-02)
        //=====================================================================

        /// グラフィックスルートCBV設定
        virtual void SetGraphicsRootConstantBufferView(uint32 rootParameterIndex, uint64 bufferLocation) = 0;

        /// グラフィックスルートSRV設定
        virtual void SetGraphicsRootShaderResourceView(uint32 rootParameterIndex, uint64 bufferLocation) = 0;

        /// グラフィックスルートUAV設定
        virtual void SetGraphicsRootUnorderedAccessView(uint32 rootParameterIndex, uint64 bufferLocation) = 0;

        //=====================================================================
        // グラフィックスルート定数 (05-05)
        //=====================================================================

        /// グラフィックス用ルート定数設定
        virtual void SetGraphicsRoot32BitConstants(uint32 rootParameterIndex,
                                                   uint32 num32BitValues,
                                                   const void* data,
                                                   uint32 destOffset = 0) = 0;

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
        virtual void SetBlendFactor(const float factor[4]) = 0;

        //=====================================================================
        // ステンシル参照値 (07-03)
        //=====================================================================

        /// ステンシル参照値設定
        virtual void SetStencilRef(uint32 refValue) = 0;

        //=====================================================================
        // ライン幅 (07-02)
        //=====================================================================

        /// ライン幅設定
        virtual void SetLineWidth(float width) = 0;

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
        virtual void SetShadingRate(ERHIShadingRate rate, const ERHIVRSCombiner* combiners = nullptr) = 0;

        /// VRSイメージ設定
        virtual void SetShadingRateImage(IRHITexture* vrsImage) = 0;

        //=====================================================================
        // Reserved Resource (11-06)
        //=====================================================================

        /// バッファのコミットサイズ変更
        /// @param buffer 対象バッファ（ReservedResource）
        /// @param newCommitSize 新しいコミットサイズ
        virtual void CommitBuffer(IRHIBuffer* buffer, uint64 newCommitSize) = 0;

        /// テクスチャ領域のコミット/デコミット
        /// @param texture 対象テクスチャ（ReservedResource）
        /// @param regions コミット領域配列
        /// @param regionCount 領域数
        /// @param commit true=コミット、false=デコミット
        virtual void CommitTextureRegions(IRHITexture* texture,
                                          const RHITextureCommitRegion* regions,
                                          uint32 regionCount,
                                          bool commit) = 0;

        //=====================================================================
        // レンダーパス (13-01, 13-03)
        //=====================================================================

        /// レンダーパス開始
        virtual void BeginRenderPass(const RHIRenderPassDesc& desc) = 0;

        /// レンダーパス終了
        virtual void EndRenderPass() = 0;

        /// レンダーパス中か
        virtual bool IsInRenderPass() const = 0;

        /// 現在のレンダーパス記述取得
        virtual const RHIRenderPassDesc* GetCurrentRenderPassDesc() const = 0;

        /// 次のサブパスへ進む
        virtual void NextSubpass() = 0;

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
        virtual void TransitionBarrier(IRHIResource* resource,
                                       ERHIResourceState stateBefore,
                                       ERHIResourceState stateAfter,
                                       uint32 subresource = kAllSubresources) = 0;

        /// 複数遷移バリア発行
        virtual void TransitionBarriers(const RHITransitionBarrier* barriers, uint32 count) = 0;

        /// 複数UAVバリア発行
        virtual void UAVBarriers(const RHIUAVBarrier* barriers, uint32 count) = 0;

        /// 複数エイリアシングバリア発行
        virtual void AliasingBarriers(const RHIAliasingBarrier* barriers, uint32 count) = 0;

        /// エイリアシングバリアバッチ発行 (23-04)
        void AliasingBarriers(const class RHIAliasingBarrierBatch& batch);

        //=====================================================================
        // プレディケーション (14-04)
        //=====================================================================

        /// プレディケーション設定
        /// @param buffer クエリ結果バッファ
        /// @param offset バッファ内のオフセット
        /// @param operation 比較操作
        virtual void SetPredication(IRHIBuffer* buffer, uint64 offset, ERHIPredicationOp operation) = 0;

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
        virtual void ExecuteIndirect(IRHICommandSignature* commandSignature,
                                     uint32 maxCommandCount,
                                     IRHIBuffer* argumentBuffer,
                                     uint64 argumentOffset,
                                     IRHIBuffer* countBuffer = nullptr,
                                     uint64 countOffset = 0) = 0;

        //=====================================================================
        // Breadcrumb (17-04)
        //=====================================================================

        /// Breadcrumb開始
        virtual void BeginBreadcrumbGPU(RHIBreadcrumbNode* node) = 0;

        /// Breadcrumb終了
        virtual void EndBreadcrumbGPU(RHIBreadcrumbNode* node) = 0;

        //=====================================================================
        // メッシュシェーダー (22-01, 22-04)
        //=====================================================================

        /// メッシュパイプラインステート設定
        virtual void SetMeshPipelineState(IRHIMeshPipelineState* pso) = 0;

        /// メッシュシェーダーディスパッチ
        /// @param groupCountX X方向グループ数
        /// @param groupCountY Y方向グループ数
        /// @param groupCountZ Z方向グループ数
        virtual void DispatchMesh(uint32 groupCountX, uint32 groupCountY = 1, uint32 groupCountZ = 1) = 0;

        /// 間接メッシュシェーダーディスパッチ
        /// @param argumentBuffer 引数バッファ（RHIDispatchMeshArguments）
        /// @param argumentOffset バッファオフセット
        virtual void DispatchMeshIndirect(IRHIBuffer* argumentBuffer, uint64 argumentOffset = 0) = 0;

        /// カウントバッファ付きIndirectメッシュシェーダーディスパッチ
        /// @param argumentBuffer 引数バッファ
        /// @param argumentOffset 引数バッファオフセット
        /// @param countBuffer カウントバッファ
        /// @param countOffset カウントバッファオフセット
        /// @param maxDispatchCount 最大ディスパッチ数
        virtual void DispatchMeshIndirectCount(IRHIBuffer* argumentBuffer,
                                               uint64 argumentOffset,
                                               IRHIBuffer* countBuffer,
                                               uint64 countOffset,
                                               uint32 maxDispatchCount) = 0;

        //=====================================================================
        // レイトレーシング (19-01〜19-03)
        //=====================================================================

        /// 加速構造ビルド
        /// @param desc ビルド記述
        virtual void BuildRaytracingAccelerationStructure(const RHIAccelerationStructureBuildDesc& desc) = 0;

        /// 加速構造コピー
        /// @param dest コピー先
        /// @param source コピー元
        /// @param mode コピーモード
        virtual void CopyRaytracingAccelerationStructure(IRHIAccelerationStructure* dest,
                                                          IRHIAccelerationStructure* source,
                                                          ERHIRaytracingCopyMode mode) = 0;

        /// レイトレーシングPSO設定
        virtual void SetRaytracingPipelineState(IRHIRaytracingPipelineState* pso) = 0;

        /// レイディスパッチ
        /// @param desc DispatchRays記述
        virtual void DispatchRays(const RHIDispatchRaysDesc& desc) = 0;
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

}} // namespace NS::RHI

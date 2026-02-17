/// @file RHICommandBuffer.h
/// @brief RHIコマンドバッファ（遅延実行用コマンド記録）
/// @details リニアアロケータ上にコマンド構造体を記録し、RHIスレッドで一括実行する。
///          Bypassモードでは記録をスキップして即時実行する。
/// @see 00_Design_Philosophy §3.1, §3.2
#pragma once

#include "Common/Utility/Macros.h"
#include "RHICommands.h"
#include "RHIMacros.h"

#include <cstdlib>
#include <cstring>

namespace NS::RHI
{
    //=========================================================================
    // RHICommandBuffer: コマンド記録バッファ
    //=========================================================================

    /// コマンドバッファ
    /// リニアアロケータでコマンド構造体を記録し、一括実行する。
    ///
    /// 設計書 §3.1:
    ///   "コマンドはリニアアロケータで高速に確保すること（malloc禁止）"
    ///   "アロケータはフレーム毎にリセットして再利用すること"
    ///
    /// 使用パターン:
    /// @code
    ///   RHICommandBuffer cmdBuf;
    ///   cmdBuf.Initialize(64 * 1024); // 64KB
    ///
    ///   // 記録フェーズ（レンダースレッド）
    ///   auto& cmd = cmdBuf.AllocCommand<CmdDraw>();
    ///   cmd.vertexCount = 36;
    ///   cmd.instanceCount = 1;
    ///
    ///   // 実行フェーズ（RHIスレッド）
    ///   cmdBuf.Execute(graphicsContext);
    ///
    ///   // フレーム終了後にリセット
    ///   cmdBuf.Reset();
    /// @endcode
    class RHI_API RHICommandBuffer
    {
    public:
        NS_DISALLOW_COPY(RHICommandBuffer);

        RHICommandBuffer() = default;
        ~RHICommandBuffer() { Shutdown(); }

        RHICommandBuffer(RHICommandBuffer&& other) noexcept
            : m_buffer(other.m_buffer)
            , m_capacity(other.m_capacity)
            , m_offset(other.m_offset)
            , m_commandCount(other.m_commandCount)
            , m_firstCommandOffset(other.m_firstCommandOffset)
            , m_lastCommandOffset(other.m_lastCommandOffset)
        {
            other.m_buffer = nullptr;
            other.m_capacity = 0;
            other.m_offset = 0;
            other.m_commandCount = 0;
            other.m_firstCommandOffset = 0;
            other.m_lastCommandOffset = 0;
        }

        RHICommandBuffer& operator=(RHICommandBuffer&& other) noexcept
        {
            if (this != &other)
            {
                Shutdown();
                m_buffer = other.m_buffer;
                m_capacity = other.m_capacity;
                m_offset = other.m_offset;
                m_commandCount = other.m_commandCount;
                m_firstCommandOffset = other.m_firstCommandOffset;
                m_lastCommandOffset = other.m_lastCommandOffset;
                other.m_buffer = nullptr;
                other.m_capacity = 0;
                other.m_offset = 0;
                other.m_commandCount = 0;
                other.m_firstCommandOffset = 0;
                other.m_lastCommandOffset = 0;
            }
            return *this;
        }

        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// 初期化（メモリ確保）
        /// @param capacityBytes 初期バッファサイズ（バイト）
        bool Initialize(uint64 capacityBytes)
        {
            NS_ASSERT(m_buffer == nullptr);
            m_buffer = static_cast<uint8*>(std::malloc(static_cast<size_t>(capacityBytes)));
            if (!m_buffer)
                return false;
            m_capacity = capacityBytes;
            m_offset = 0;
            m_commandCount = 0;
            m_firstCommandOffset = 0;
            m_lastCommandOffset = 0;
            return true;
        }

        /// シャットダウン（メモリ解放）
        void Shutdown()
        {
            if (m_buffer)
            {
                std::free(m_buffer);
                m_buffer = nullptr;
            }
            m_capacity = 0;
            m_offset = 0;
            m_commandCount = 0;
        }

        /// リセット（フレーム毎）
        /// メモリは解放せず、オフセットのみリセット
        void Reset()
        {
            m_offset = 0;
            m_commandCount = 0;
            m_firstCommandOffset = 0;
            m_lastCommandOffset = 0;
        }

        //=====================================================================
        // コマンド記録
        //=====================================================================

        /// コマンド構造体を割り当て・記録
        /// @tparam T コマンド構造体型（CmdDraw等、T::kType を持つこと）
        /// @return 割り当てられたコマンドへの参照
        template <typename T>
        T& AllocCommand()
        {
            static_assert(std::is_trivially_copyable_v<T>, "Command must be trivially copyable");
            static_assert(sizeof(T) <= UINT16_MAX, "Command size exceeds RHICommandHeader::size (uint16) limit");

            constexpr ERHICommandType type = T::kType;
            constexpr uint64 cmdSize = sizeof(T);
            constexpr uint64 alignment = alignof(T);

            // アラインメント調整
            uint64 alignedOffset = (m_offset + alignment - 1) & ~(alignment - 1);

            // 容量チェック（必要に応じて拡張）
            if (alignedOffset + cmdSize > m_capacity)
            {
                if (!Grow(alignedOffset + cmdSize))
                {
                    // OOM: 静的ダミーを返して書き込みクラッシュを防ぐ
                    static thread_local typename std::aligned_storage<sizeof(T), alignof(T)>::type s_dummy;
                    T* dummy = reinterpret_cast<T*>(&s_dummy);
                    std::memset(dummy, 0, sizeof(T));
                    dummy->header.type = type;
                    dummy->header.size = static_cast<uint16>(cmdSize);
                    return *dummy;
                }
            }

            // コマンド配置
            T* cmd = reinterpret_cast<T*>(m_buffer + alignedOffset);
            std::memset(cmd, 0, cmdSize);
            cmd->header.type = type;
            cmd->header.size = static_cast<uint16>(cmdSize);
            cmd->header.nextOffset = 0;

            // リンクリスト更新
            NS_ASSERT(alignedOffset <= UINT32_MAX); // nextOffset は uint32
            if (m_commandCount > 0)
            {
                auto* prevHeader = reinterpret_cast<RHICommandHeader*>(m_buffer + m_lastCommandOffset);
                prevHeader->nextOffset = static_cast<uint32>(alignedOffset);
            }
            else
            {
                m_firstCommandOffset = alignedOffset;
            }

            m_lastCommandOffset = alignedOffset;
            m_offset = alignedOffset + cmdSize;
            m_commandCount++;

            return *cmd;
        }

        //=====================================================================
        // コマンド実行（リプレイ）
        //=====================================================================

        /// 記録された全コマンドをGraphicsコンテキストで実行
        /// @param ctx 実行先コンテキスト
        void Execute(IRHICommandContext* ctx) const
        {
            if (m_commandCount == 0)
                return;

            uint64 offset = m_firstCommandOffset;
            while (offset < m_offset)
            {
                const auto* header = reinterpret_cast<const RHICommandHeader*>(m_buffer + offset);
                ExecuteCommand(ctx, header, offset);

                if (header->nextOffset == 0)
                    break;
                offset = header->nextOffset;
            }
        }

        /// 記録された全コマンドをComputeコンテキストで実行
        /// @param ctx 実行先コンテキスト
        void ExecuteCompute(IRHIComputeContext* ctx) const
        {
            if (m_commandCount == 0)
                return;

            uint64 offset = m_firstCommandOffset;
            while (offset < m_offset)
            {
                const auto* header = reinterpret_cast<const RHICommandHeader*>(m_buffer + offset);
                ExecuteComputeCommand(ctx, header, offset);

                if (header->nextOffset == 0)
                    break;
                offset = header->nextOffset;
            }
        }

        /// 記録された全コマンドをUploadコンテキストで実行
        /// @param ctx 実行先コンテキスト
        void ExecuteUpload(IRHIUploadContext* ctx) const
        {
            if (m_commandCount == 0)
                return;

            uint64 offset = m_firstCommandOffset;
            while (offset < m_offset)
            {
                const auto* header = reinterpret_cast<const RHICommandHeader*>(m_buffer + offset);
                ExecuteUploadCommand(ctx, header, offset);

                if (header->nextOffset == 0)
                    break;
                offset = header->nextOffset;
            }
        }

        //=====================================================================
        // 情報
        //=====================================================================

        uint32 GetCommandCount() const { return m_commandCount; }
        uint64 GetUsedBytes() const { return m_offset; }
        uint64 GetCapacity() const { return m_capacity; }
        bool IsEmpty() const { return m_commandCount == 0; }

    private:
        uint8* m_buffer = nullptr;
        uint64 m_capacity = 0;
        uint64 m_offset = 0;
        uint32 m_commandCount = 0;
        uint64 m_firstCommandOffset = 0;
        uint64 m_lastCommandOffset = 0;

        /// バッファ拡張（2倍成長）
        /// @return 成功時 true、OOM 時 false（m_buffer/m_capacity 変更なし）
        [[nodiscard]] bool Grow(uint64 requiredSize)
        {
            uint64 newCapacity = m_capacity * 2;
            if (newCapacity < requiredSize)
                newCapacity = requiredSize;

            uint8* newBuffer = static_cast<uint8*>(std::realloc(m_buffer, static_cast<size_t>(newCapacity)));
            if (!newBuffer)
            {
                NS_ASSERT(false && "RHICommandBuffer::Grow() - out of memory");
                return false;
            }
            m_buffer = newBuffer;
            m_capacity = newCapacity;
            return true;
        }

        /// Graphicsコンテキスト用コマンドディスパッチ
        void ExecuteCommand(IRHICommandContext* ctx, const RHICommandHeader* header, uint64 offset) const
        {
            switch (header->type)
            {
            // 描画
            case ERHICommandType::Draw:
                CmdDraw::Execute(ctx, *reinterpret_cast<const CmdDraw*>(m_buffer + offset));
                break;
            case ERHICommandType::DrawIndexed:
                CmdDrawIndexed::Execute(ctx, *reinterpret_cast<const CmdDrawIndexed*>(m_buffer + offset));
                break;
            case ERHICommandType::DrawIndirect:
                CmdDrawIndirect::Execute(ctx, *reinterpret_cast<const CmdDrawIndirect*>(m_buffer + offset));
                break;
            case ERHICommandType::DrawIndexedIndirect:
                CmdDrawIndexedIndirect::Execute(
                    ctx, *reinterpret_cast<const CmdDrawIndexedIndirect*>(m_buffer + offset));
                break;
            case ERHICommandType::MultiDrawIndirect:
                CmdMultiDrawIndirect::Execute(
                    ctx, *reinterpret_cast<const CmdMultiDrawIndirect*>(m_buffer + offset));
                break;
            case ERHICommandType::MultiDrawIndirectCount:
                CmdMultiDrawIndirectCount::Execute(
                    ctx, *reinterpret_cast<const CmdMultiDrawIndirectCount*>(m_buffer + offset));
                break;

            // コンピュート（Graphics contextはComputeを継承するため処理可能）
            case ERHICommandType::Dispatch:
                CmdDispatch::Execute(ctx, *reinterpret_cast<const CmdDispatch*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchIndirect:
                CmdDispatchIndirect::Execute(ctx, *reinterpret_cast<const CmdDispatchIndirect*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchIndirectMulti:
                CmdDispatchIndirectMulti::Execute(ctx, *reinterpret_cast<const CmdDispatchIndirectMulti*>(m_buffer + offset));
                break;

            // メッシュシェーダー
            case ERHICommandType::DispatchMesh:
                CmdDispatchMesh::Execute(ctx, *reinterpret_cast<const CmdDispatchMesh*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchMeshIndirect:
                CmdDispatchMeshIndirect::Execute(ctx, *reinterpret_cast<const CmdDispatchMeshIndirect*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchMeshIndirectCount:
                CmdDispatchMeshIndirectCount::Execute(ctx, *reinterpret_cast<const CmdDispatchMeshIndirectCount*>(m_buffer + offset));
                break;

            // パイプラインステート
            case ERHICommandType::SetGraphicsPipelineState:
                CmdSetGraphicsPSO::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsPSO*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputePipelineState:
                CmdSetComputePSO::Execute(ctx, *reinterpret_cast<const CmdSetComputePSO*>(m_buffer + offset));
                break;
            case ERHICommandType::SetMeshPipelineState:
                CmdSetMeshPSO::Execute(ctx, *reinterpret_cast<const CmdSetMeshPSO*>(m_buffer + offset));
                break;
            case ERHICommandType::SetGraphicsRootSignature:
                CmdSetGraphicsRootSignature::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsRootSignature*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootSignature:
                CmdSetComputeRootSignature::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootSignature*>(m_buffer + offset));
                break;

            // バリア (Base)
            case ERHICommandType::TransitionResource:
                CmdTransitionResource::Execute(
                    ctx, *reinterpret_cast<const CmdTransitionResource*>(m_buffer + offset));
                break;
            case ERHICommandType::UAVBarrier:
                CmdUAVBarrier::Execute(ctx, *reinterpret_cast<const CmdUAVBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::AliasingBarrier:
                CmdAliasingBarrier::Execute(ctx, *reinterpret_cast<const CmdAliasingBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::FlushBarriers:
                CmdFlushBarriers::Execute(ctx, *reinterpret_cast<const CmdFlushBarriers*>(m_buffer + offset));
                break;

            // バリア (Graphics batch)
            case ERHICommandType::TransitionBarrier:
                CmdTransitionBarrier::Execute(ctx, *reinterpret_cast<const CmdTransitionBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::TransitionBarriers:
                CmdTransitionBarriers::Execute(ctx, *reinterpret_cast<const CmdTransitionBarriers*>(m_buffer + offset));
                break;
            case ERHICommandType::UAVBarriers:
                CmdUAVBarriers::Execute(ctx, *reinterpret_cast<const CmdUAVBarriers*>(m_buffer + offset));
                break;
            case ERHICommandType::AliasingBarriers:
                CmdAliasingBarriers::Execute(ctx, *reinterpret_cast<const CmdAliasingBarriers*>(m_buffer + offset));
                break;

            // コピー
            case ERHICommandType::CopyBuffer:
                CmdCopyBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBufferRegion:
                CmdCopyBufferRegion::Execute(ctx, *reinterpret_cast<const CmdCopyBufferRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTexture:
                CmdCopyTexture::Execute(ctx, *reinterpret_cast<const CmdCopyTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTextureRegion:
                CmdCopyTextureRegion::Execute(ctx, *reinterpret_cast<const CmdCopyTextureRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBufferToTexture:
                CmdCopyBufferToTexture::Execute(ctx, *reinterpret_cast<const CmdCopyBufferToTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTextureToBuffer:
                CmdCopyTextureToBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyTextureToBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyToStagingBuffer:
                CmdCopyToStagingBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyToStagingBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveTexture:
                CmdResolveTexture::Execute(ctx, *reinterpret_cast<const CmdResolveTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveTextureRegion:
                CmdResolveTextureRegion::Execute(ctx, *reinterpret_cast<const CmdResolveTextureRegion*>(m_buffer + offset));
                break;

            // レンダーパス
            case ERHICommandType::BeginRenderPass:
                CmdBeginRenderPass::Execute(ctx, *reinterpret_cast<const CmdBeginRenderPass*>(m_buffer + offset));
                break;
            case ERHICommandType::EndRenderPass:
                CmdEndRenderPass::Execute(ctx, *reinterpret_cast<const CmdEndRenderPass*>(m_buffer + offset));
                break;
            case ERHICommandType::NextSubpass:
                CmdNextSubpass::Execute(ctx, *reinterpret_cast<const CmdNextSubpass*>(m_buffer + offset));
                break;
            case ERHICommandType::ResetStatistics:
                CmdResetStatistics::Execute(ctx, *reinterpret_cast<const CmdResetStatistics*>(m_buffer + offset));
                break;

            // ビューポート・シザー
            case ERHICommandType::SetViewports:
                CmdSetViewports::Execute(ctx, *reinterpret_cast<const CmdSetViewports*>(m_buffer + offset));
                break;
            case ERHICommandType::SetScissorRects:
                CmdSetScissorRects::Execute(ctx, *reinterpret_cast<const CmdSetScissorRects*>(m_buffer + offset));
                break;

            // 頂点・インデックスバッファ
            case ERHICommandType::SetVertexBuffers:
                CmdSetVertexBuffers::Execute(ctx, *reinterpret_cast<const CmdSetVertexBuffers*>(m_buffer + offset));
                break;
            case ERHICommandType::SetIndexBuffer:
                CmdSetIndexBuffer::Execute(ctx, *reinterpret_cast<const CmdSetIndexBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::SetPrimitiveTopology:
                CmdSetPrimitiveTopology::Execute(ctx, *reinterpret_cast<const CmdSetPrimitiveTopology*>(m_buffer + offset));
                break;

            // レンダーターゲット
            case ERHICommandType::SetRenderTargets:
                CmdSetRenderTargets::Execute(ctx, *reinterpret_cast<const CmdSetRenderTargets*>(m_buffer + offset));
                break;
            case ERHICommandType::ClearRenderTargetView:
                CmdClearRenderTargetView::Execute(ctx, *reinterpret_cast<const CmdClearRenderTargetView*>(m_buffer + offset));
                break;
            case ERHICommandType::ClearDepthStencilView:
                CmdClearDepthStencilView::Execute(ctx, *reinterpret_cast<const CmdClearDepthStencilView*>(m_buffer + offset));
                break;

            // デバッグ
            case ERHICommandType::BeginDebugEvent:
                CmdBeginDebugEvent::Execute(ctx, *reinterpret_cast<const CmdBeginDebugEvent*>(m_buffer + offset));
                break;
            case ERHICommandType::EndDebugEvent:
                CmdEndDebugEvent::Execute(ctx, *reinterpret_cast<const CmdEndDebugEvent*>(m_buffer + offset));
                break;
            case ERHICommandType::InsertDebugMarker:
                CmdInsertDebugMarker::Execute(ctx, *reinterpret_cast<const CmdInsertDebugMarker*>(m_buffer + offset));
                break;
            case ERHICommandType::InsertBreadcrumb:
                CmdInsertBreadcrumb::Execute(ctx, *reinterpret_cast<const CmdInsertBreadcrumb*>(m_buffer + offset));
                break;

            // Compute: ルート引数
            case ERHICommandType::SetComputeRoot32BitConstants:
                CmdSetComputeRoot32BitConstants::Execute(ctx, *reinterpret_cast<const CmdSetComputeRoot32BitConstants*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootCBV:
                CmdSetComputeRootCBV::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootCBV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootSRV:
                CmdSetComputeRootSRV::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootSRV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootUAV:
                CmdSetComputeRootUAV::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootUAV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootDescriptorTable:
                CmdSetComputeRootDescriptorTable::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootDescriptorTable*>(m_buffer + offset));
                break;
            case ERHICommandType::SetDescriptorHeaps:
                CmdSetDescriptorHeaps::Execute(ctx, *reinterpret_cast<const CmdSetDescriptorHeaps*>(m_buffer + offset));
                break;

            // Compute: UAVクリア
            case ERHICommandType::ClearUnorderedAccessViewUint:
                CmdClearUnorderedAccessViewUint::Execute(ctx, *reinterpret_cast<const CmdClearUnorderedAccessViewUint*>(m_buffer + offset));
                break;
            case ERHICommandType::ClearUnorderedAccessViewFloat:
                CmdClearUnorderedAccessViewFloat::Execute(ctx, *reinterpret_cast<const CmdClearUnorderedAccessViewFloat*>(m_buffer + offset));
                break;

            // Compute: クエリ
            case ERHICommandType::WriteTimestamp:
                CmdWriteTimestamp::Execute(ctx, *reinterpret_cast<const CmdWriteTimestamp*>(m_buffer + offset));
                break;
            case ERHICommandType::BeginQuery:
                CmdBeginQuery::Execute(ctx, *reinterpret_cast<const CmdBeginQuery*>(m_buffer + offset));
                break;
            case ERHICommandType::EndQuery:
                CmdEndQuery::Execute(ctx, *reinterpret_cast<const CmdEndQuery*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveQueryData:
                CmdResolveQueryData::Execute(ctx, *reinterpret_cast<const CmdResolveQueryData*>(m_buffer + offset));
                break;

            // Graphics: ルート引数
            case ERHICommandType::SetGraphicsRoot32BitConstants:
                CmdSetGraphicsRoot32BitConstants::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsRoot32BitConstants*>(m_buffer + offset));
                break;
            case ERHICommandType::SetGraphicsRootCBV:
                CmdSetGraphicsRootCBV::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsRootCBV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetGraphicsRootSRV:
                CmdSetGraphicsRootSRV::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsRootSRV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetGraphicsRootUAV:
                CmdSetGraphicsRootUAV::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsRootUAV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetGraphicsRootDescriptorTable:
                CmdSetGraphicsRootDescriptorTable::Execute(ctx, *reinterpret_cast<const CmdSetGraphicsRootDescriptorTable*>(m_buffer + offset));
                break;

            // Graphics: ステート
            case ERHICommandType::SetBlendFactor:
                CmdSetBlendFactor::Execute(ctx, *reinterpret_cast<const CmdSetBlendFactor*>(m_buffer + offset));
                break;
            case ERHICommandType::SetStencilRef:
                CmdSetStencilRef::Execute(ctx, *reinterpret_cast<const CmdSetStencilRef*>(m_buffer + offset));
                break;
            case ERHICommandType::SetLineWidth:
                CmdSetLineWidth::Execute(ctx, *reinterpret_cast<const CmdSetLineWidth*>(m_buffer + offset));
                break;
            case ERHICommandType::SetDepthBounds:
                CmdSetDepthBounds::Execute(ctx, *reinterpret_cast<const CmdSetDepthBounds*>(m_buffer + offset));
                break;
            case ERHICommandType::SetShadingRate:
                CmdSetShadingRate::Execute(ctx, *reinterpret_cast<const CmdSetShadingRate*>(m_buffer + offset));
                break;
            case ERHICommandType::SetShadingRateImage:
                CmdSetShadingRateImage::Execute(ctx, *reinterpret_cast<const CmdSetShadingRateImage*>(m_buffer + offset));
                break;
            case ERHICommandType::SetPredication:
                CmdSetPredication::Execute(ctx, *reinterpret_cast<const CmdSetPredication*>(m_buffer + offset));
                break;

            // Graphics: Reserved Resource
            case ERHICommandType::CommitBuffer:
                CmdCommitBuffer::Execute(ctx, *reinterpret_cast<const CmdCommitBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CommitTextureRegions:
                CmdCommitTextureRegions::Execute(ctx, *reinterpret_cast<const CmdCommitTextureRegions*>(m_buffer + offset));
                break;

            // Graphics: ワークグラフ
            case ERHICommandType::SetWorkGraphPipeline:
                CmdSetWorkGraphPipeline::Execute(ctx, *reinterpret_cast<const CmdSetWorkGraphPipeline*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchGraph:
                CmdDispatchGraph::Execute(ctx, *reinterpret_cast<const CmdDispatchGraph*>(m_buffer + offset));
                break;
            case ERHICommandType::InitializeWorkGraphBackingMemory:
                CmdInitializeWorkGraphBackingMemory::Execute(ctx, *reinterpret_cast<const CmdInitializeWorkGraphBackingMemory*>(m_buffer + offset));
                break;

            // Graphics: ExecuteIndirect
            case ERHICommandType::ExecuteIndirect:
                CmdExecuteIndirect::Execute(ctx, *reinterpret_cast<const CmdExecuteIndirect*>(m_buffer + offset));
                break;

            // Graphics: Breadcrumb GPU
            case ERHICommandType::BeginBreadcrumbGPU:
                CmdBeginBreadcrumbGPU::Execute(ctx, *reinterpret_cast<const CmdBeginBreadcrumbGPU*>(m_buffer + offset));
                break;
            case ERHICommandType::EndBreadcrumbGPU:
                CmdEndBreadcrumbGPU::Execute(ctx, *reinterpret_cast<const CmdEndBreadcrumbGPU*>(m_buffer + offset));
                break;

            // Graphics: レイトレーシング
            case ERHICommandType::BuildRaytracingAccelerationStructure:
                CmdBuildRaytracingAccelerationStructure::Execute(ctx, *reinterpret_cast<const CmdBuildRaytracingAccelerationStructure*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyRaytracingAccelerationStructure:
                CmdCopyRaytracingAccelerationStructure::Execute(ctx, *reinterpret_cast<const CmdCopyRaytracingAccelerationStructure*>(m_buffer + offset));
                break;
            case ERHICommandType::SetRaytracingPipelineState:
                CmdSetRaytracingPipelineState::Execute(ctx, *reinterpret_cast<const CmdSetRaytracingPipelineState*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchRays:
                CmdDispatchRays::Execute(ctx, *reinterpret_cast<const CmdDispatchRays*>(m_buffer + offset));
                break;

            default:
                NS_ASSERT(false); // Graphics コンテキストで未対応のコマンドタイプ
                break;
            }
        }

        /// Computeコンテキスト用コマンドディスパッチ
        void ExecuteComputeCommand(IRHIComputeContext* ctx, const RHICommandHeader* header, uint64 offset) const
        {
            switch (header->type)
            {
            // コンピュート
            case ERHICommandType::Dispatch:
                CmdDispatch::Execute(ctx, *reinterpret_cast<const CmdDispatch*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchIndirect:
                CmdDispatchIndirect::Execute(ctx, *reinterpret_cast<const CmdDispatchIndirect*>(m_buffer + offset));
                break;
            case ERHICommandType::DispatchIndirectMulti:
                CmdDispatchIndirectMulti::Execute(ctx, *reinterpret_cast<const CmdDispatchIndirectMulti*>(m_buffer + offset));
                break;

            // パイプラインステート
            case ERHICommandType::SetComputePipelineState:
                CmdSetComputePSO::Execute(ctx, *reinterpret_cast<const CmdSetComputePSO*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootSignature:
                CmdSetComputeRootSignature::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootSignature*>(m_buffer + offset));
                break;

            // バリア (Base)
            case ERHICommandType::TransitionResource:
                CmdTransitionResource::Execute(
                    ctx, *reinterpret_cast<const CmdTransitionResource*>(m_buffer + offset));
                break;
            case ERHICommandType::UAVBarrier:
                CmdUAVBarrier::Execute(ctx, *reinterpret_cast<const CmdUAVBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::AliasingBarrier:
                CmdAliasingBarrier::Execute(ctx, *reinterpret_cast<const CmdAliasingBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::FlushBarriers:
                CmdFlushBarriers::Execute(ctx, *reinterpret_cast<const CmdFlushBarriers*>(m_buffer + offset));
                break;

            // コピー
            case ERHICommandType::CopyBuffer:
                CmdCopyBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBufferRegion:
                CmdCopyBufferRegion::Execute(ctx, *reinterpret_cast<const CmdCopyBufferRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTexture:
                CmdCopyTexture::Execute(ctx, *reinterpret_cast<const CmdCopyTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTextureRegion:
                CmdCopyTextureRegion::Execute(ctx, *reinterpret_cast<const CmdCopyTextureRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBufferToTexture:
                CmdCopyBufferToTexture::Execute(ctx, *reinterpret_cast<const CmdCopyBufferToTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTextureToBuffer:
                CmdCopyTextureToBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyTextureToBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyToStagingBuffer:
                CmdCopyToStagingBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyToStagingBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveTexture:
                CmdResolveTexture::Execute(ctx, *reinterpret_cast<const CmdResolveTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveTextureRegion:
                CmdResolveTextureRegion::Execute(ctx, *reinterpret_cast<const CmdResolveTextureRegion*>(m_buffer + offset));
                break;

            // Compute: ルート引数
            case ERHICommandType::SetComputeRoot32BitConstants:
                CmdSetComputeRoot32BitConstants::Execute(ctx, *reinterpret_cast<const CmdSetComputeRoot32BitConstants*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootCBV:
                CmdSetComputeRootCBV::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootCBV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootSRV:
                CmdSetComputeRootSRV::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootSRV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootUAV:
                CmdSetComputeRootUAV::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootUAV*>(m_buffer + offset));
                break;
            case ERHICommandType::SetComputeRootDescriptorTable:
                CmdSetComputeRootDescriptorTable::Execute(ctx, *reinterpret_cast<const CmdSetComputeRootDescriptorTable*>(m_buffer + offset));
                break;
            case ERHICommandType::SetDescriptorHeaps:
                CmdSetDescriptorHeaps::Execute(ctx, *reinterpret_cast<const CmdSetDescriptorHeaps*>(m_buffer + offset));
                break;

            // Compute: UAVクリア
            case ERHICommandType::ClearUnorderedAccessViewUint:
                CmdClearUnorderedAccessViewUint::Execute(ctx, *reinterpret_cast<const CmdClearUnorderedAccessViewUint*>(m_buffer + offset));
                break;
            case ERHICommandType::ClearUnorderedAccessViewFloat:
                CmdClearUnorderedAccessViewFloat::Execute(ctx, *reinterpret_cast<const CmdClearUnorderedAccessViewFloat*>(m_buffer + offset));
                break;

            // Compute: クエリ
            case ERHICommandType::WriteTimestamp:
                CmdWriteTimestamp::Execute(ctx, *reinterpret_cast<const CmdWriteTimestamp*>(m_buffer + offset));
                break;
            case ERHICommandType::BeginQuery:
                CmdBeginQuery::Execute(ctx, *reinterpret_cast<const CmdBeginQuery*>(m_buffer + offset));
                break;
            case ERHICommandType::EndQuery:
                CmdEndQuery::Execute(ctx, *reinterpret_cast<const CmdEndQuery*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveQueryData:
                CmdResolveQueryData::Execute(ctx, *reinterpret_cast<const CmdResolveQueryData*>(m_buffer + offset));
                break;

            // デバッグ
            case ERHICommandType::BeginDebugEvent:
                CmdBeginDebugEvent::Execute(ctx, *reinterpret_cast<const CmdBeginDebugEvent*>(m_buffer + offset));
                break;
            case ERHICommandType::EndDebugEvent:
                CmdEndDebugEvent::Execute(ctx, *reinterpret_cast<const CmdEndDebugEvent*>(m_buffer + offset));
                break;
            case ERHICommandType::InsertDebugMarker:
                CmdInsertDebugMarker::Execute(ctx, *reinterpret_cast<const CmdInsertDebugMarker*>(m_buffer + offset));
                break;
            case ERHICommandType::InsertBreadcrumb:
                CmdInsertBreadcrumb::Execute(ctx, *reinterpret_cast<const CmdInsertBreadcrumb*>(m_buffer + offset));
                break;

            default:
                NS_ASSERT(false); // Compute コンテキストで未対応のコマンドタイプ
                break;
            }
        }

        /// Uploadコンテキスト用コマンドディスパッチ
        void ExecuteUploadCommand(IRHIUploadContext* ctx, const RHICommandHeader* header, uint64 offset) const
        {
            switch (header->type)
            {
            // Upload
            case ERHICommandType::UploadBuffer:
                CmdUploadBuffer::Execute(ctx, *reinterpret_cast<const CmdUploadBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::UploadTexture:
                CmdUploadTexture::Execute(ctx, *reinterpret_cast<const CmdUploadTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyStagingToTexture:
                CmdCopyStagingToTexture::Execute(ctx, *reinterpret_cast<const CmdCopyStagingToTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyStagingToBuffer:
                CmdCopyStagingToBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyStagingToBuffer*>(m_buffer + offset));
                break;

            // Base operations also valid for Upload context
            case ERHICommandType::TransitionResource:
                CmdTransitionResource::Execute(ctx, *reinterpret_cast<const CmdTransitionResource*>(m_buffer + offset));
                break;
            case ERHICommandType::UAVBarrier:
                CmdUAVBarrier::Execute(ctx, *reinterpret_cast<const CmdUAVBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::AliasingBarrier:
                CmdAliasingBarrier::Execute(ctx, *reinterpret_cast<const CmdAliasingBarrier*>(m_buffer + offset));
                break;
            case ERHICommandType::FlushBarriers:
                CmdFlushBarriers::Execute(ctx, *reinterpret_cast<const CmdFlushBarriers*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBuffer:
                CmdCopyBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBufferRegion:
                CmdCopyBufferRegion::Execute(ctx, *reinterpret_cast<const CmdCopyBufferRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTexture:
                CmdCopyTexture::Execute(ctx, *reinterpret_cast<const CmdCopyTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTextureRegion:
                CmdCopyTextureRegion::Execute(ctx, *reinterpret_cast<const CmdCopyTextureRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyBufferToTexture:
                CmdCopyBufferToTexture::Execute(ctx, *reinterpret_cast<const CmdCopyBufferToTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyTextureToBuffer:
                CmdCopyTextureToBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyTextureToBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::CopyToStagingBuffer:
                CmdCopyToStagingBuffer::Execute(ctx, *reinterpret_cast<const CmdCopyToStagingBuffer*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveTexture:
                CmdResolveTexture::Execute(ctx, *reinterpret_cast<const CmdResolveTexture*>(m_buffer + offset));
                break;
            case ERHICommandType::ResolveTextureRegion:
                CmdResolveTextureRegion::Execute(ctx, *reinterpret_cast<const CmdResolveTextureRegion*>(m_buffer + offset));
                break;
            case ERHICommandType::BeginDebugEvent:
                CmdBeginDebugEvent::Execute(ctx, *reinterpret_cast<const CmdBeginDebugEvent*>(m_buffer + offset));
                break;
            case ERHICommandType::EndDebugEvent:
                CmdEndDebugEvent::Execute(ctx, *reinterpret_cast<const CmdEndDebugEvent*>(m_buffer + offset));
                break;
            case ERHICommandType::InsertDebugMarker:
                CmdInsertDebugMarker::Execute(ctx, *reinterpret_cast<const CmdInsertDebugMarker*>(m_buffer + offset));
                break;
            case ERHICommandType::InsertBreadcrumb:
                CmdInsertBreadcrumb::Execute(ctx, *reinterpret_cast<const CmdInsertBreadcrumb*>(m_buffer + offset));
                break;

            default:
                NS_ASSERT(false); // Upload コンテキストで未対応のコマンドタイプ
                break;
            }
        }
    };

} // namespace NS::RHI

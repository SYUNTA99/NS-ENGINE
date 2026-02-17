/// @file IRHICommandContextBase.h
/// @brief コマンドコンテキスト基底インターフェース
/// @details 全コマンドコンテキストに共通するインターフェース。ライフサイクル、バリア、コピー、デバッグ機能を提供。
/// @see 02-01-command-context-base.md
#pragma once

#include "Common/Utility/Macros.h"
#include "RHIEnums.h"
#include "RHIFwd.h"
#include "RHIMacros.h"
#include "RHITypes.h"
#include "RHIDispatchTable.h"

namespace NS { namespace RHI {
    //=========================================================================
    // IRHICommandContextBase
    //=========================================================================

    /// コマンドコンテキスト基底
    /// 全てのコマンドコンテキストに共通するインターフェース
    class RHI_API IRHICommandContextBase
    {
    public:
        virtual ~IRHICommandContextBase() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// GPUマスク取得
        virtual GPUMask GetGPUMask() const = 0;

        /// キュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;

        /// パイプラインタイプ取得
        virtual ERHIPipeline GetPipeline() const = 0;

        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// コンテキスト開始
        /// @param allocator 使用するコマンドアロケーター
        virtual void Begin(IRHICommandAllocator* allocator) = 0;

        /// コンテキスト終了
        /// @return 記録されたコマンドリスト
        virtual IRHICommandList* Finish() = 0;

        /// コンテキストリセット
        virtual void Reset() = 0;

        /// コマンド記録中か
        virtual bool IsRecording() const = 0;

        //=====================================================================
        // リソースバリア
        //=====================================================================

        /// 単一リソースの状態遷移
        /// @param resource 対象リソース
        /// @param stateBefore 遷移前の状態
        /// @param stateAfter 遷移後の状態
        void TransitionResource(IRHIResource* resource, ERHIAccess stateBefore, ERHIAccess stateAfter)
        {
            NS_RHI_DISPATCH(TransitionResource, this, resource, stateBefore, stateAfter);
        }

        /// UAVバリア
        /// @param resource 対象リソース（nullptrで全UAV）
        void UAVBarrier(IRHIResource* resource = nullptr)
        {
            NS_RHI_DISPATCH(UAVBarrier, this, resource);
        }

        /// エイリアシングバリア
        /// @param resourceBefore 使用終了リソース
        /// @param resourceAfter 使用開始リソース
        void AliasingBarrier(IRHIResource* resourceBefore, IRHIResource* resourceAfter)
        {
            NS_RHI_DISPATCH(AliasingBarrier, this, resourceBefore, resourceAfter);
        }

        /// バリアをフラッシュ（遅延バリアの場合）
        void FlushBarriers()
        {
            NS_RHI_DISPATCH(FlushBarriers, this);
        }

        //=====================================================================
        // バッファコピー
        //=====================================================================

        /// バッファ全体コピー
        void CopyBuffer(IRHIBuffer* dst, IRHIBuffer* src)
        {
            NS_RHI_DISPATCH(CopyBuffer, this, dst, src);
        }

        /// バッファ部分コピー
        void CopyBufferRegion(
            IRHIBuffer* dst, uint64 dstOffset, IRHIBuffer* src, uint64 srcOffset, uint64 size)
        {
            NS_RHI_DISPATCH(CopyBufferRegion, this, dst, dstOffset, src, srcOffset, size);
        }

        //=====================================================================
        // テクスチャコピー
        //=====================================================================

        /// テクスチャ全体コピー
        void CopyTexture(IRHITexture* dst, IRHITexture* src)
        {
            NS_RHI_DISPATCH(CopyTexture, this, dst, src);
        }

        /// テクスチャ部分コピー
        void CopyTextureRegion(IRHITexture* dst,
                               uint32 dstMip,
                               uint32 dstSlice,
                               Offset3D dstOffset,
                               IRHITexture* src,
                               uint32 srcMip,
                               uint32 srcSlice,
                               const RHIBox* srcBox = nullptr)
        {
            NS_RHI_DISPATCH(CopyTextureRegion, this, dst, dstMip, dstSlice, dstOffset, src, srcMip, srcSlice, srcBox);
        }

        //=====================================================================
        // バッファ ↔ テクスチャ
        //=====================================================================

        /// バッファからテクスチャへコピー
        void CopyBufferToTexture(IRHITexture* dst,
                                 uint32 dstMip,
                                 uint32 dstSlice,
                                 Offset3D dstOffset,
                                 IRHIBuffer* src,
                                 uint64 srcOffset,
                                 uint32 srcRowPitch,
                                 uint32 srcDepthPitch)
        {
            NS_RHI_DISPATCH(CopyBufferToTexture, this, dst, dstMip, dstSlice, dstOffset, src, srcOffset, srcRowPitch, srcDepthPitch);
        }

        /// テクスチャからバッファへコピー
        void CopyTextureToBuffer(IRHIBuffer* dst,
                                 uint64 dstOffset,
                                 uint32 dstRowPitch,
                                 uint32 dstDepthPitch,
                                 IRHITexture* src,
                                 uint32 srcMip,
                                 uint32 srcSlice,
                                 const RHIBox* srcBox = nullptr)
        {
            NS_RHI_DISPATCH(CopyTextureToBuffer, this, dst, dstOffset, dstRowPitch, dstDepthPitch, src, srcMip, srcSlice, srcBox);
        }

        //=====================================================================
        // ステージングコピー
        //=====================================================================

        /// ステージングバッファへコピー
        /// @param dst ステージングバッファ
        /// @param dstOffset ステージングバッファ内オフセット
        /// @param src コピー元リソース
        /// @param srcOffset コピー元オフセット
        /// @param size コピーサイズ
        void CopyToStagingBuffer(IRHIStagingBuffer* dst, uint64 dstOffset, IRHIResource* src, uint64 srcOffset, uint64 size)
        {
            NS_RHI_DISPATCH(CopyToStagingBuffer, this, dst, dstOffset, src, srcOffset, size);
        }

        //=====================================================================
        // MSAA解決
        //=====================================================================

        /// テクスチャ全体のMSAA解決
        /// @param dst 解決先テクスチャ（非MSAA）
        /// @param src 解決元テクスチャ（MSAA）
        void ResolveTexture(IRHITexture* dst, IRHITexture* src)
        {
            NS_RHI_DISPATCH(ResolveTexture, this, dst, src);
        }

        /// テクスチャ部分MSAA解決
        /// @param dst 解決先テクスチャ
        /// @param dstMip 解決先Mipレベル
        /// @param dstSlice 解決先配列スライス
        /// @param src 解決元テクスチャ
        /// @param srcMip 解決元Mipレベル
        /// @param srcSlice 解決元配列スライス
        void ResolveTextureRegion(IRHITexture* dst,
                                  uint32 dstMip,
                                  uint32 dstSlice,
                                  IRHITexture* src,
                                  uint32 srcMip,
                                  uint32 srcSlice)
        {
            NS_RHI_DISPATCH(ResolveTextureRegion, this, dst, dstMip, dstSlice, src, srcMip, srcSlice);
        }

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグイベント開始
        void BeginDebugEvent(const char* name, uint32 color = 0)
        {
            NS_RHI_DISPATCH(BeginDebugEvent, this, name, color);
        }

        /// デバッグイベント終了
        void EndDebugEvent()
        {
            NS_RHI_DISPATCH(EndDebugEvent, this);
        }

        /// デバッグマーカー挿入
        void InsertDebugMarker(const char* name, uint32 color = 0)
        {
            NS_RHI_DISPATCH(InsertDebugMarker, this, name, color);
        }

        //=====================================================================
        // ブレッドクラム (09-03)
        //=====================================================================

        /// ブレッドクラム挿入（GPUクラッシュ診断用）
        void InsertBreadcrumb(uint32 id, const char* message = nullptr)
        {
            NS_RHI_DISPATCH(InsertBreadcrumb, this, id, message);
        }
    };

    //=========================================================================
    // IRHIImmediateContext: 即時実行コンテキスト
    //=========================================================================

    /// 即時実行コンテキスト
    /// 通常コンテキストと同一インターフェースだが、コマンドを即座にGPUに発行する。
    /// Begin/Finishのペアではなく、各コマンドが即時サブミットされる。
    ///
    /// 使用制約:
    /// - RHIスレッドからのみ使用可能
    /// - 並列記録不可（単一インスタンスのみ）
    /// - パフォーマンスコストが高い（毎コマンドでフラッシュ可能なため）
    /// - 通常の遅延実行パスで代替可能な場合は使用しない
    ///
    /// 使用例:
    /// - デバイス初期化時のリソースセットアップ
    /// - デバッグ用の即時描画
    /// - GPU readbackの即時完了待ち
    class RHI_API IRHIImmediateContext : public IRHICommandContextBase
    {
    public:
        /// 即時フラッシュ
        /// 記録済みコマンドをGPUに送信し、完了を待つ
        virtual void Flush() = 0;

        /// ネイティブコンテキスト取得（プラットフォーム固有操作用）
        virtual void* GetNativeContext() = 0;
    };

    //=========================================================================
    // RHIDebugEventScope（RAII）
    //=========================================================================

    /// デバッグイベントスコープ（RAII）
    struct RHIDebugEventScope
    {
        NS_DISALLOW_COPY(RHIDebugEventScope);

        IRHICommandContextBase* context;

        RHIDebugEventScope(IRHICommandContextBase* ctx, const char* name, uint32 color = 0) : context(ctx)
        {
            if (context)
                context->BeginDebugEvent(name, color);
        }

        ~RHIDebugEventScope()
        {
            if (context)
                context->EndDebugEvent();
        }
    };

#define RHI_DEBUG_EVENT(ctx, name)                                                                                     \
    ::NS::RHI::RHIDebugEventScope NS_MACRO_CONCATENATE(_rhiDebugEvent, __LINE__)(ctx, name)

}} // namespace NS::RHI

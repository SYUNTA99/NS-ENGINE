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

namespace NS::RHI
{
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
        virtual void TransitionResource(IRHIResource* resource, ERHIAccess stateBefore, ERHIAccess stateAfter) = 0;

        /// UAVバリア
        /// @param resource 対象リソース（nullptrで全UAV）
        virtual void UAVBarrier(IRHIResource* resource = nullptr) = 0;

        /// エイリアシングバリア
        /// @param resourceBefore 使用終了リソース
        /// @param resourceAfter 使用開始リソース
        virtual void AliasingBarrier(IRHIResource* resourceBefore, IRHIResource* resourceAfter) = 0;

        /// バリアをフラッシュ（遅延バリアの場合）
        virtual void FlushBarriers() = 0;

        //=====================================================================
        // バッファコピー
        //=====================================================================

        /// バッファ全体コピー
        virtual void CopyBuffer(IRHIBuffer* dst, IRHIBuffer* src) = 0;

        /// バッファ部分コピー
        virtual void CopyBufferRegion(
            IRHIBuffer* dst, uint64 dstOffset, IRHIBuffer* src, uint64 srcOffset, uint64 size) = 0;

        //=====================================================================
        // テクスチャコピー
        //=====================================================================

        /// テクスチャ全体コピー
        virtual void CopyTexture(IRHITexture* dst, IRHITexture* src) = 0;

        /// テクスチャ部分コピー
        virtual void CopyTextureRegion(IRHITexture* dst,
                                       uint32 dstMip,
                                       uint32 dstSlice,
                                       Offset3D dstOffset,
                                       IRHITexture* src,
                                       uint32 srcMip,
                                       uint32 srcSlice,
                                       const RHIBox* srcBox = nullptr) = 0;

        //=====================================================================
        // バッファ ↔ テクスチャ
        //=====================================================================

        /// バッファからテクスチャへコピー
        virtual void CopyBufferToTexture(IRHITexture* dst,
                                         uint32 dstMip,
                                         uint32 dstSlice,
                                         Offset3D dstOffset,
                                         IRHIBuffer* src,
                                         uint64 srcOffset,
                                         uint32 srcRowPitch,
                                         uint32 srcDepthPitch) = 0;

        /// テクスチャからバッファへコピー
        virtual void CopyTextureToBuffer(IRHIBuffer* dst,
                                         uint64 dstOffset,
                                         uint32 dstRowPitch,
                                         uint32 dstDepthPitch,
                                         IRHITexture* src,
                                         uint32 srcMip,
                                         uint32 srcSlice,
                                         const RHIBox* srcBox = nullptr) = 0;

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグイベント開始
        virtual void BeginDebugEvent(const char* name, uint32 color = 0) = 0;

        /// デバッグイベント終了
        virtual void EndDebugEvent() = 0;

        /// デバッグマーカー挿入
        virtual void InsertDebugMarker(const char* name, uint32 color = 0) = 0;

        //=====================================================================
        // ブレッドクラム (09-03)
        //=====================================================================

        /// ブレッドクラム挿入（GPUクラッシュ診断用）
        virtual void InsertBreadcrumb(uint32 id, const char* message = nullptr) = 0;
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

} // namespace NS::RHI

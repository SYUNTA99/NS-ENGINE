/// @file IRHIUploadContext.h
/// @brief アップロードコンテキストインターフェース
/// @details CPU→GPU転送専用の独立コンテキスト。Graphics/Computeコンテキストと並行動作可能。
///          仮想関数は使用せず、ディスパッチテーブル経由で呼び出す（設計書 §1.2）。
/// @see 03_Command_Execution §2
#pragma once

#include "IRHICommandContextBase.h"

namespace NS::RHI
{
    //=========================================================================
    // IRHIUploadContext
    //=========================================================================

    /// アップロードコンテキスト
    /// CPU→GPUデータ転送専用の独立コンテキスト。
    /// Graphics/Computeキューと並行してコピーキューで動作する。
    ///
    /// 使用パターン:
    /// @code
    ///   auto* ctx = device->GetUploadContext();
    ///   ctx->Begin(allocator);
    ///   ctx->UploadBuffer(dst, 0, srcData, dataSize);
    ///   ctx->UploadTexture(tex, 0, 0, pixelData, rowPitch);
    ///   auto* cmdList = ctx->Finish();
    ///   uploadQueue->Submit(cmdList);
    /// @endcode
    class RHI_API IRHIUploadContext : public IRHICommandContextBase
    {
    public:
        ~IRHIUploadContext() = default;

        //=====================================================================
        // バッファアップロード
        //=====================================================================

        /// バッファデータのアップロード（CPU→GPU）
        /// @param dst アップロード先バッファ
        /// @param dstOffset バッファ内のオフセット
        /// @param srcData ソースデータポインタ（CPU側）
        /// @param srcSize データサイズ（バイト）
        void UploadBuffer(IRHIBuffer* dst, uint64 dstOffset, const void* srcData, uint64 srcSize)
        {
            NS_RHI_DISPATCH(UploadBuffer, this, dst, dstOffset, srcData, srcSize);
        }

        //=====================================================================
        // テクスチャアップロード
        //=====================================================================

        /// テクスチャデータのアップロード（CPU→GPU）
        /// @param dst アップロード先テクスチャ
        /// @param dstMip 対象Mipレベル
        /// @param dstSlice 対象配列スライス
        /// @param srcData ソースデータポインタ（CPU側）
        /// @param srcRowPitch ソースの行ピッチ（バイト）
        /// @param srcDepthPitch ソースの深度ピッチ（3Dテクスチャ用、0=2Dテクスチャ）
        void UploadTexture(IRHITexture* dst,
                           uint32 dstMip,
                           uint32 dstSlice,
                           const void* srcData,
                           uint32 srcRowPitch,
                           uint32 srcDepthPitch = 0)
        {
            NS_RHI_DISPATCH(UploadTexture, this, dst, dstMip, dstSlice, srcData, srcRowPitch, srcDepthPitch);
        }

        //=====================================================================
        // ステージングバッファ転送
        //=====================================================================

        /// ステージングバッファからテクスチャへ転送
        /// @param dst アップロード先テクスチャ
        /// @param dstMip 対象Mipレベル
        /// @param dstSlice 対象配列スライス
        /// @param dstOffset テクスチャ内オフセット
        /// @param stagingBuffer ステージングバッファ
        /// @param stagingOffset ステージングバッファ内オフセット
        /// @param rowPitch 行ピッチ
        /// @param depthPitch 深度ピッチ
        void CopyStagingToTexture(IRHITexture* dst,
                                  uint32 dstMip,
                                  uint32 dstSlice,
                                  Offset3D dstOffset,
                                  IRHIBuffer* stagingBuffer,
                                  uint64 stagingOffset,
                                  uint32 rowPitch,
                                  uint32 depthPitch)
        {
            NS_RHI_DISPATCH(CopyStagingToTexture, this, dst, dstMip, dstSlice, dstOffset, stagingBuffer, stagingOffset, rowPitch, depthPitch);
        }

        /// ステージングバッファからバッファへ転送
        /// @param dst アップロード先バッファ
        /// @param dstOffset バッファ内オフセット
        /// @param stagingBuffer ステージングバッファ
        /// @param stagingOffset ステージングバッファ内オフセット
        /// @param size コピーサイズ
        void CopyStagingToBuffer(
            IRHIBuffer* dst, uint64 dstOffset, IRHIBuffer* stagingBuffer, uint64 stagingOffset, uint64 size)
        {
            NS_RHI_DISPATCH(CopyStagingToBuffer, this, dst, dstOffset, stagingBuffer, stagingOffset, size);
        }
    };

} // namespace NS::RHI

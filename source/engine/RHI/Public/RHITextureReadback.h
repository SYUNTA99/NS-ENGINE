/// @file RHITextureReadback.h
/// @brief テクスチャリードバック・スクリーンキャプチャ・デバッグビューア
/// @details GPUテクスチャデータのCPU読み戻し、スクリーンキャプチャ、
///          テクスチャデバッグビューアを提供。
/// @see 20-04-texture-readback.md
#pragma once

#include "Common/Result/Core/Result.h"
#include "IRHIResource.h"
#include "RHIMacros.h"
#include "RHITypes.h"

#include <functional>
#include <vector>

namespace NS { namespace RHI {
    //=========================================================================
    // RHITextureReadbackDesc (20-04)
    //=========================================================================

    /// テクスチャリードバック記述
    struct RHITextureReadbackDesc
    {
        uint32 width = 0;
        uint32 height = 0;
        ERHIPixelFormat format = ERHIPixelFormat::RGBA8_UNORM;
        const char* debugName = nullptr;
    };

    //=========================================================================
    // IRHITextureReadback (20-04)
    //=========================================================================

    /// テクスチャリードバックインターフェース
    class RHI_API IRHITextureReadback : public IRHIResource
    {
    public:
        virtual ~IRHITextureReadback() = default;

        //=====================================================================
        // リードバック操作
        //=====================================================================

        /// リードバック開始
        /// @pre sourceTextureはERHIResourceState::CopySource状態であること
        virtual void EnqueueCopy(IRHICommandContext* context,
                                 IRHITexture* sourceTexture,
                                 uint32 mipLevel = 0,
                                 uint32 arraySlice = 0) = 0;

        /// 矩形領域のリードバック
        virtual void EnqueueCopyRegion(IRHICommandContext* context,
                                       IRHITexture* sourceTexture,
                                       uint32 srcX,
                                       uint32 srcY,
                                       uint32 width,
                                       uint32 height,
                                       uint32 mipLevel = 0,
                                       uint32 arraySlice = 0) = 0;

        //=====================================================================
        // 状態管理
        //=====================================================================

        /// 準備完了か
        virtual bool IsReady() const = 0;

        /// 待機
        virtual bool Wait(uint32 timeoutMs = 0) = 0;

        //=====================================================================
        // データ取得
        //=====================================================================

        /// データサイズ取得
        virtual uint64 GetDataSize() const = 0;

        /// 行ピッチ取得
        virtual uint32 GetRowPitch() const = 0;

        /// データ取得
        virtual NS::Result GetData(void* outData, uint64 size) = 0;

        /// マップしてポインタ取得
        virtual const void* Lock() = 0;

        /// マップ解除
        virtual void Unlock() = 0;

        //=====================================================================
        // プロパティ
        //=====================================================================

        /// 幅
        virtual uint32 GetWidth() const = 0;

        /// 高さ
        virtual uint32 GetHeight() const = 0;

        /// フォーマット
        virtual ERHIPixelFormat GetFormat() const = 0;
    };

    using RHITextureReadbackRef = TRefCountPtr<IRHITextureReadback>;

    //=========================================================================
    // RHIScreenCapture (20-04)
    //=========================================================================

    /// スクリーンキャプチャヘルパー
    class RHI_API RHIScreenCapture
    {
    public:
        explicit RHIScreenCapture(IRHIDevice* device);

        /// キャプチャ要求
        void RequestCapture(IRHICommandContext* context, IRHITexture* backBuffer);

        /// キャプチャ完了待機し、画像を保存
        bool SaveToPNG(const char* filename);
        bool SaveToJPG(const char* filename, int quality = 90);
        bool SaveToBMP(const char* filename);

        /// 生データ取得
        bool GetPixelData(std::vector<uint8>& outData);

        /// 非同期キャプチャ（コールバック）
        void RequestCaptureAsync(IRHICommandContext* context,
                                 IRHITexture* backBuffer,
                                 std::function<void(const uint8*, uint32, uint32)> callback);

    private:
        RHITextureReadbackRef m_readback;
        uint32 m_width = 0;
        uint32 m_height = 0;
        std::function<void(const uint8*, uint32, uint32)> m_pendingCallback;
    };

    //=========================================================================
    // RHITextureDebugViewer (20-04)
    //=========================================================================

    /// テクスチャデバッグビューア
    /// コンピュートシェーダー出力等のデバッグ用
    class RHI_API RHITextureDebugViewer
    {
    public:
        explicit RHITextureDebugViewer(IRHIDevice* device, uint32 maxWidth, uint32 maxHeight);

        /// デバッグ対象テクスチャ設定
        void SetTargetTexture(IRHITexture* texture);

        /// 指定ピクセルの値を取得
        void QueryPixel(IRHICommandContext* context, uint32 x, uint32 y, std::function<void(const float*)> callback);

        /// ヒストグラム計算
        void ComputeHistogram(IRHICommandContext* context,
                              std::function<void(const uint32* histogram, uint32 binCount)> callback);

    private:
        RHITextureReadbackRef m_readback;
        IRHITexture* m_targetTexture = nullptr;
    };

}} // namespace NS::RHI

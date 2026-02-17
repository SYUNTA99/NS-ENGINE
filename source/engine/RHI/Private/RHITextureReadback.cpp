/// @file RHITextureReadback.cpp
/// @brief スクリーンキャプチャ・テクスチャデバッグビューア実装
#include "RHITextureReadback.h"
#include "IRHICommandContext.h"
#include "IRHIDevice.h"

namespace NS::RHI
{
    //=========================================================================
    // RHIScreenCapture
    //=========================================================================

    RHIScreenCapture::RHIScreenCapture(IRHIDevice* device)
    {
        // テクスチャリードバック作成はキャプチャ時にサイズ指定で行う
        (void)device;
    }

    void RHIScreenCapture::RequestCapture(IRHICommandContext* context, IRHITexture* backBuffer)
    {
        if ((context == nullptr) || (backBuffer == nullptr))
        {
            return;
        }

        m_width = backBuffer->GetWidth();
        m_height = backBuffer->GetHeight();

        if (m_readback)
            m_readback->EnqueueCopy(context, backBuffer, 0, 0);
    }

    bool RHIScreenCapture::SaveToPNG(const char* filename)
    {
        (void)filename;
        // PNG保存は画像ライブラリ依存
        // stb_image_write等を使用
        return false;
    }

    bool RHIScreenCapture::SaveToJPG(const char* filename, int quality)
    {
        (void)filename;
        (void)quality;
        return false;
    }

    bool RHIScreenCapture::SaveToBMP(const char* filename)
    {
        (void)filename;
        return false;
    }

    bool RHIScreenCapture::GetPixelData(std::vector<uint8>& outData)
    {
        if (!m_readback || !m_readback->IsReady())
        {
            return false;
        }

        uint64 dataSize = m_readback->GetDataSize();
        outData.resize(static_cast<size_t>(dataSize));

        return m_readback->GetData(outData.data(), dataSize).IsSuccess();
    }

    void RHIScreenCapture::RequestCaptureAsync(IRHICommandContext* context,
                                               IRHITexture* backBuffer,
                                               const std::function<void(const uint8*, uint32, uint32)>& callback)
    {
        m_pendingCallback = callback;
        RequestCapture(context, backBuffer);
    }

    //=========================================================================
    // RHITextureDebugViewer
    //=========================================================================

    RHITextureDebugViewer::RHITextureDebugViewer(IRHIDevice* device, uint32 maxWidth, uint32 maxHeight)
    {
        // 最大サイズのリードバックバッファを事前確保
        (void)device;
        (void)maxWidth;
        (void)maxHeight;
    }

    void RHITextureDebugViewer::SetTargetTexture(IRHITexture* texture)
    {
        m_targetTexture = texture;
    }

    void RHITextureDebugViewer::QueryPixel(IRHICommandContext* context,
                                           uint32 x,
                                           uint32 y,
                                           std::function<void(const float*)> callback)
    {
        // 1x1ピクセルのリードバック
        // コンピュートシェーダーで指定ピクセルをバッファにコピーし、リードバック
        (void)context;
        (void)x;
        (void)y;
        (void)callback;
    }

    void RHITextureDebugViewer::ComputeHistogram(IRHICommandContext* context,
                                                 std::function<void(const uint32* histogram, uint32 binCount)> callback)
    {
        // コンピュートシェーダーでヒストグラム計算
        // バックエンド依存
        (void)context;
        (void)callback;
    }

} // namespace NS::RHI

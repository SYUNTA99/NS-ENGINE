# 20-04: テクスチャリードバック

## 概要

GPUテクスチャデータをCPUへ読み戻す専用インターフェース、
スクリーンキャプチャ、GPUデバッグ、コンピュートシェーダー結果の可視化に使用、

## ファイル

- `Source/Engine/RHI/Public/RHITextureReadback.h`

## 依存

- 20-01-staging-buffer.md (ステージングバッファ)
- 04-02-texture-interface.md (IRHITexture)

## 定義

```cpp
namespace NS::RHI
{

/// テクスチャリードバック記述
struct RHITextureReadbackDesc
{
    uint32 width = 0;
    uint32 height = 0;
    ERHIPixelFormat format = ERHIPixelFormat::RGBA8_UNORM;
    const char* debugName = nullptr;
};

/// テクスチャリードバックインターフェース
class IRHITextureReadback : public IRHIResource
{
public:
    virtual ~IRHITextureReadback() = default;

    /// リードバック開始
    /// @pre sourceTextureはERHIResourceState::CopySource状態であること。
    ///      呼び出し前に遷移バリアを発行すること。
    virtual void EnqueueCopy(
        IRHICommandContext* context,
        IRHITexture* sourceTexture,
        uint32 mipLevel = 0,
        uint32 arraySlice = 0) = 0;

    /// 矩形領域のリードバック
    virtual void EnqueueCopyRegion(
        IRHICommandContext* context,
        IRHITexture* sourceTexture,
        uint32 srcX, uint32 srcY,
        uint32 width, uint32 height,
        uint32 mipLevel = 0,
        uint32 arraySlice = 0) = 0;

    /// 準備完了後
    virtual bool IsReady() const = 0;

    /// 得）。
    virtual bool Wait(uint32 timeoutMs = 0) = 0;

    /// データサイズ取得
    virtual uint64 GetDataSize() const = 0;

    /// 行ピッチを取得
    virtual uint32 GetRowPitch() const = 0;

    /// データ取得
    virtual NS::Result GetData(void* outData, uint64 size) = 0;

    /// マップしてポインタ取得
    virtual const void* Lock() = 0;
    virtual void Unlock() = 0;

    /// 幅
    virtual uint32 GetWidth() const = 0;

    /// 高さ
    virtual uint32 GetHeight() const = 0;

    /// フォーマット
    virtual ERHIPixelFormat GetFormat() const = 0;
};

using RHITextureReadbackRef = TRefCountPtr<IRHITextureReadback>;

// ════════════════════════════════════════════════════════════════
// スクリーンキャプチャ
// ════════════════════════════════════════════════════════════════

/// スクリーンキャプチャヘルパー
class RHIScreenCapture
{
public:
    explicit RHIScreenCapture(IRHIDevice* device);

    /// キャプチャ要求
    void RequestCapture(
        IRHICommandContext* context,
        IRHITexture* backBuffer);

    /// キャプチャ完了後待機し、画像を保存
    bool SaveToPNG(const char* filename);
    bool SaveToJPG(const char* filename, int quality = 90);
    bool SaveToBMP(const char* filename);

    /// 生データ取得
    bool GetPixelData(std::vector<uint8>& outData);

    /// 非同期キャプチャ（コールバック）。
    void RequestCaptureAsync(
        IRHICommandContext* context,
        IRHITexture* backBuffer,
        std::function<void(const uint8*, uint32, uint32)> callback);

private:
    RHITextureReadbackRef m_readback;
    uint32 m_width = 0;
    uint32 m_height = 0;
    std::function<void(const uint8*, uint32, uint32)> m_pendingCallback;
};

// ════════════════════════════════════════════════════════════════
// GPUテクスチャデバッグ
// ════════════════════════════════════════════════════════════════

/// テクスチャデバッグビューア
/// コンピュートシェーダー出力等のデバッグ用
class RHITextureDebugViewer
{
public:
    explicit RHITextureDebugViewer(IRHIDevice* device, uint32 maxWidth, uint32 maxHeight);

    /// デバッグ対象テクスチャを設定
    void SetTargetTexture(IRHITexture* texture);

    /// 指定ピクセルの値を取得
    void QueryPixel(
        IRHICommandContext* context,
        uint32 x, uint32 y,
        std::function<void(const float4&)> callback);

    /// ヒストグラム計算
    void ComputeHistogram(
        IRHICommandContext* context,
        std::function<void(const uint32* histogram, uint32 binCount)> callback);

    /// ImGuiで表示
    void DrawImGui();

private:
    RHITextureReadbackRef m_readback;
    IRHITexture* m_targetTexture = nullptr;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// スクリーンショット
RHIScreenCapture capture(device);

void OnKeyPress(Key key)
{
    if (key == Key::F12)
    {
        capture.RequestCapture(context, swapChain->GetCurrentBackBuffer());
    }
}

void OnFrameEnd()
{
    if (capture.SaveToPNG("screenshot.png"))
    {
        NS_LOG_INFO("Screenshot saved");
    }
}

// GPUテクスチャデバッグ
RHITextureDebugViewer debugViewer(device, 1920, 1080);

debugViewer.SetTargetTexture(gbufferNormal);
debugViewer.QueryPixel(context, mouseX, mouseY, [](const float4& value) {
    NS_LOG_DEBUG("Normal at cursor: (%.2f, %.2f, %.2f)", value.x, value.y, value.z);
});
```

## 検証

- [ ] 各フォーマットのリードバック
- [ ] 行ピッチの正確性
- [ ] 矩形領域リードバック
- [ ] ファイル保存機能

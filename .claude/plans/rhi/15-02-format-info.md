# 15-02: フォーマット情報

## 目的

ピクセルフォーマットの詳細情報を取得機能を定義する。

## 参照ドキュメント

- 15-01-pixel-format-enum.md (ERHIPixelFormat)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIPixelFormat.h` (部分

## TODO

### 1. フォーマット情報構造体

```cpp
namespace NS::RHI
{
    /// フォーマット情報
    struct RHI_API RHIFormatInfo
    {
        /// フォーマット
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;

        /// 名前
        const char* name = nullptr;

        /// 1ピクセル/ブロックあたりのバイト数
        uint8 bytesPerPixelOrBlock = 0;

        /// チャンネル数
        uint8 channelCount = 0;

        /// 全チャンネルのビット数
        uint8 redBits = 0;
        uint8 greenBits = 0;
        uint8 blueBits = 0;
        uint8 alphaBits = 0;
        uint8 depthBits = 0;
        uint8 stencilBits = 0;

        /// ブロックサイズ
        RHIBlockSize blockSize;

        /// カテゴリ
        ERHIFormatCategory category = ERHIFormatCategory::Unknown;

        /// タイプ
        ERHIFormatType type = ERHIFormatType::Typeless;

        /// 圧縮ファミリー
        ERHICompressionFamily compression = ERHICompressionFamily::None;

        //=====================================================================
        // ヘルパー
        //=====================================================================

        /// 圧縮フォーマットか
        bool IsCompressed() const { return compression != ERHICompressionFamily::None; }

        /// デプスフォーマットか
        bool IsDepth() const { return depthBits > 0; }

        /// ステンシルフォーマットか
        bool IsStencil() const { return stencilBits > 0; }

        /// デプスステンシルフォーマットか
        bool IsDepthStencil() const { return IsDepth() || IsStencil(); }

        /// sRGBか
        bool IsSRGB() const { return category == ERHIFormatCategory::sRGB; }

        /// 浮動小数点か
        bool IsFloat() const { return type == ERHIFormatType::Float; }

        /// 整数か
        bool IsInteger() const { return type == ERHIFormatType::UInt || type == ERHIFormatType::SInt; }

        /// 正規化か
        bool IsNormalized() const { return type == ERHIFormatType::UNorm || type == ERHIFormatType::SNorm; }

        /// アルファチャンネルありか
        bool HasAlpha() const { return alphaBits > 0; }

        /// 総ビット数
        uint32 GetTotalBits() const { return bytesPerPixelOrBlock * 8; }
    };
}
```

- [ ] RHIFormatInfo 構造体

### 2. フォーマット情報取得

```cpp
namespace NS::RHI
{
    /// フォーマット情報取得
    RHI_API const RHIFormatInfo& GetFormatInfo(ERHIPixelFormat format);

    /// フォーマット名取得
    RHI_API const char* GetFormatName(ERHIPixelFormat format);

    /// ピクセル/ブロックサイズ取得
    RHI_API uint32 GetFormatBytesPerPixelOrBlock(ERHIPixelFormat format);

    /// ブロックサイズ取得
    RHI_API RHIBlockSize GetFormatBlockSize(ERHIPixelFormat format);

    /// 行ピッチ計算
    RHI_API uint32 CalculateRowPitch(ERHIPixelFormat format, uint32 width);

    /// スライスピッチ計算
    RHI_API uint32 CalculateSlicePitch(ERHIPixelFormat format, uint32 width, uint32 height);

    /// サブリソースサイズ計算
    RHI_API uint64 CalculateSubresourceSize(
        ERHIPixelFormat format,
        uint32 width,
        uint32 height,
        uint32 depth = 1);
}
```

- [ ] GetFormatInfo
- [ ] ピッチ計算関数

### 3. フォーマットサポート確認

```cpp
namespace NS::RHI
{
    /// フォーマットサポートフラグ
    enum class ERHIFormatSupport : uint32
    {
        None = 0,

        /// バッファ
        Buffer = 1 << 0,
        IndexBuffer = 1 << 1,
        VertexBuffer = 1 << 2,

        /// テクスチャ
        Texture1D = 1 << 3,
        Texture2D = 1 << 4,
        Texture3D = 1 << 5,
        TextureCube = 1 << 6,

        /// シェーダーリソース
        ShaderLoad = 1 << 7,
        ShaderSample = 1 << 8,
        ShaderSampleComparison = 1 << 9,
        ShaderSampleMonochrome = 1 << 10,
        ShaderGather = 1 << 11,
        ShaderGatherComparison = 1 << 12,

        /// アンオーダードアクセス
        UAVLoad = 1 << 13,
        UAVStore = 1 << 14,
        UAVAtomics = 1 << 15,

        /// レンダーターゲット
        RenderTarget = 1 << 16,
        RenderTargetBlend = 1 << 17,
        RenderTargetOutput = 1 << 18,

        /// デプスステンシル
        DepthStencil = 1 << 19,

        /// その他
        Display = 1 << 20,          // スワップチェーン
        BackBufferCast = 1 << 21,   // バックバッファキャスト
        Multisample2x = 1 << 22,
        Multisample4x = 1 << 23,
        Multisample8x = 1 << 24,
        Multisample16x = 1 << 25,
        MultisampleResolve = 1 << 26,
        MultisampleLoad = 1 << 27,
        VideoEncode = 1 << 28,
        VideoDecode = 1 << 29,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIFormatSupport)

    /// フォーマットサポート確認（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// フォーマットサポート取得
        virtual ERHIFormatSupport GetFormatSupport(ERHIPixelFormat format) const = 0;

        /// 特定のサポートがあるか
        bool SupportsFormat(ERHIPixelFormat format, ERHIFormatSupport required) const {
            return (GetFormatSupport(format) & required) == required;
        }

        /// レンダーターゲットとしてサポートされるか
        bool SupportsRenderTarget(ERHIPixelFormat format) const {
            return SupportsFormat(format, ERHIFormatSupport::RenderTarget);
        }

        /// デプスステンシルとしてサポートされるか
        bool SupportsDepthStencil(ERHIPixelFormat format) const {
            return SupportsFormat(format, ERHIFormatSupport::DepthStencil);
        }

        /// UAVとしてサポートされるか
        bool SupportsUAV(ERHIPixelFormat format) const {
            return SupportsFormat(format,
                ERHIFormatSupport::UAVLoad | ERHIFormatSupport::UAVStore);
        }
    };
}
```

- [ ] ERHIFormatSupport 列挙型
- [ ] GetFormatSupport

### 4. MSAAサポート確認

```cpp
namespace NS::RHI
{
    /// MSAAサポート情報
    struct RHI_API RHIMSAASupportInfo
    {
        /// サポートされるサンプル数（ビットマスク）。
        /// bit 0 = 1x, bit 1 = 2x, bit 2 = 4x, bit 3 = 8x, bit 4 = 16x
        uint32 supportedSampleCounts = 0;

        /// 最大サンプル数
        uint32 maxSampleCount = 1;

        /// 指定サンプル数の品質レベル数
        uint32 qualityLevels[5] = {};  // 1x, 2x, 4x, 8x, 16x

        /// サポートされているか
        bool IsSupported(uint32 sampleCount) const {
            switch (sampleCount) {
                case 1:  return true;
                case 2:  return (supportedSampleCounts & (1 << 1)) != 0;
                case 4:  return (supportedSampleCounts & (1 << 2)) != 0;
                case 8:  return (supportedSampleCounts & (1 << 3)) != 0;
                case 16: return (supportedSampleCounts & (1 << 4)) != 0;
                default: return false;
            }
        }
    };

    /// MSAAサポート確認（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// MSAAサポート取得
        virtual RHIMSAASupportInfo GetMSAASupport(
            ERHIPixelFormat format,
            bool isRenderTarget = true) const = 0;
    };
}
```

- [ ] RHIMSAASupportInfo 構造体
- [ ] GetMSAASupport

## 検証方法

- [ ] フォーマット情報の正確性
- [ ] ピッチ計算の正確性
- [ ] サポートフラグの検出
- [ ] MSAAサポートの検出

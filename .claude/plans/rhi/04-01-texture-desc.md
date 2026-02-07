# 04-01: テクスチャ記述情報

## 目的

テクスチャ作成に必要な記述構造体を定義する。

## 参照ドキュメント

- 01-09-enums-texture.md (ERHITextureUsage, ERHITextureDimension)
- 01-03-types-geometry.md (Extent2D, Extent3D)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHITexture.h` (部分

## TODO

### 1. RHITextureDesc: 基本記述構造体

```cpp
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHITypes.h"
#include "RHIPixelFormat.h"

namespace NS::RHI
{
    /// テクスチャ作成記述
    struct RHI_API RHITextureDesc
    {
        //=====================================================================
        // 次元のサイズ
        //=====================================================================

        /// テクスチャ次元
        ERHITextureDimension dimension = ERHITextureDimension::Texture2D;

        /// 幅（ピクセル）。
        uint32 width = 1;

        /// 高さ（ピクセル、1D の場合は 1）。
        uint32 height = 1;

        /// 深度（3D の場合）または配列サイズ
        uint32 depthOrArraySize = 1;

        //=====================================================================
        // フォーマット
        //=====================================================================

        /// ピクセルフォーマット
        EPixelFormat format = EPixelFormat::Unknown;

        //=====================================================================
        // MIPマッチ
        //=====================================================================

        /// MIPレベル数（ = 自動計算）
        uint32 mipLevels = 1;

        //=====================================================================
        // マルチサンプル
        //=====================================================================

        /// サンプル数
        ERHISampleCount sampleCount = ERHISampleCount::Count1;

        /// サンプル品質
        uint32 sampleQuality = 0;

        //=====================================================================
        // 使用フラグ
        //=====================================================================

        /// 使用フラグ
        ERHITextureUsage usage = ERHITextureUsage::Default;

        //=====================================================================
        // レイアウト
        //=====================================================================

        /// 初期レイアウト
        ERHITextureLayout initialLayout = ERHITextureLayout::Unknown;

        //=====================================================================
        // マルチGPU
        //=====================================================================

        /// 対象GPU
        GPUMask gpuMask = GPUMask::GPU0();

        //=====================================================================
        // 最適化ヒント
        //=====================================================================

        /// クリア値（レンダーターゲット/デプス用）。
        RHIClearValue clearValue;

        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグ名
        const char* debugName = nullptr;

        //=====================================================================
        // ビルダーパターン
        //=====================================================================

        RHITextureDesc& SetDimension(ERHITextureDimension d) { dimension = d; return *this; }
        RHITextureDesc& SetWidth(uint32 w) { width = w; return *this; }
        RHITextureDesc& SetHeight(uint32 h) { height = h; return *this; }
        RHITextureDesc& SetDepth(uint32 d) { depthOrArraySize = d; return *this; }
        RHITextureDesc& SetArraySize(uint32 s) { depthOrArraySize = s; return *this; }
        RHITextureDesc& SetFormat(EPixelFormat f) { format = f; return *this; }
        RHITextureDesc& SetMipLevels(uint32 m) { mipLevels = m; return *this; }
        RHITextureDesc& SetSampleCount(ERHISampleCount c) { sampleCount = c; return *this; }
        RHITextureDesc& SetUsage(ERHITextureUsage u) { usage = u; return *this; }
        RHITextureDesc& SetGPUMask(GPUMask m) { gpuMask = m; return *this; }
        RHITextureDesc& SetDebugName(const char* name) { debugName = name; return *this; }
        RHITextureDesc& SetClearValue(const RHIClearValue& cv) { clearValue = cv; return *this; }

        /// サイズ一括設定（2D）。
        RHITextureDesc& SetSize(uint32 w, uint32 h) {
            width = w;
            height = h;
            return *this;
        }

        /// サイズ一括設定（3D）。
        RHITextureDesc& SetSize3D(uint32 w, uint32 h, uint32 d) {
            width = w;
            height = h;
            depthOrArraySize = d;
            return *this;
        }
    };
}
```

- [ ] RHITextureDesc 基本構造体
- [ ] ビルダーパターンメソッド

### 2. RHIClearValue: クリア値

```cpp
namespace NS::RHI
{
    /// テクスチャクリア値
    struct RHI_API RHIClearValue
    {
        /// 色クリア値（GBA）。
        float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        /// デプスクリア値
        float depth = 1.0f;

        /// ステンシルクリア値
        uint8 stencil = 0;

        /// 色用クリア値作成
        static RHIClearValue Color(float r, float g, float b, float a = 1.0f) {
            RHIClearValue v;
            v.color[0] = r;
            v.color[1] = g;
            v.color[2] = b;
            v.color[3] = a;
            return v;
        }

        /// 黒でクリア
        static RHIClearValue Black() { return Color(0, 0, 0, 1); }

        /// 白でクリア
        static RHIClearValue White() { return Color(1, 1, 1, 1); }

        /// 透のでクリア
        static RHIClearValue Transparent() { return Color(0, 0, 0, 0); }

        /// デプス用クリア値作成
        static RHIClearValue Depth(float d = 1.0f, uint8 s = 0) {
            RHIClearValue v;
            v.depth = d;
            v.stencil = s;
            return v;
        }

        /// デプスリバース（Near = 0, Far = 1 の逆）。
        static RHIClearValue DepthReversed() { return Depth(0.0f, 0); }
    };
}
```

- [ ] RHIClearValue 構造体
- [ ] Color / Depth ファクトリ

### 3. テクスチャ作成ヘルパー

```cpp
namespace NS::RHI
{
    /// 2Dテクスチャ作成Desc
    inline RHITextureDesc CreateTexture2DDesc(
        uint32 width, uint32 height,
        EPixelFormat format,
        ERHITextureUsage usage = ERHITextureUsage::Default,
        uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.depthOrArraySize = 1;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// レンダーターゲット作成Desc
    inline RHITextureDesc CreateRenderTargetDesc(
        uint32 width, uint32 height,
        EPixelFormat format,
        ERHISampleCount sampleCount = ERHISampleCount::Count1,
        const RHIClearValue& clearValue = RHIClearValue::Black())
    {
        RHITextureDesc desc;
        desc.dimension = (sampleCount != ERHISampleCount::Count1)
            ? ERHITextureDimension::Texture2DMS
            : ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.sampleCount = sampleCount;
        desc.usage = ERHITextureUsage::RenderTargetShaderResource;
        desc.clearValue = clearValue;
        return desc;
    }

    /// デプスバッファ作成Desc
    inline RHITextureDesc CreateDepthStencilDesc(
        uint32 width, uint32 height,
        EPixelFormat format = EPixelFormat::D32_Float,
        ERHISampleCount sampleCount = ERHISampleCount::Count1,
        bool shaderResource = true,
        const RHIClearValue& clearValue = RHIClearValue::Depth())
    {
        RHITextureDesc desc;
        desc.dimension = (sampleCount != ERHISampleCount::Count1)
            ? ERHITextureDimension::Texture2DMS
            : ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.sampleCount = sampleCount;
        desc.usage = shaderResource
            ? ERHITextureUsage::DepthShaderResource
            : ERHITextureUsage::DepthStencil;
        desc.clearValue = clearValue;
        return desc;
    }

    /// 3Dテクスチャ作成Desc
    inline RHITextureDesc CreateTexture3DDesc(
        uint32 width, uint32 height, uint32 depth,
        EPixelFormat format,
        ERHITextureUsage usage = ERHITextureUsage::Default,
        uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture3D;
        desc.width = width;
        desc.height = height;
        desc.depthOrArraySize = depth;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// テクスチャ配列作成Desc
    inline RHITextureDesc CreateTexture2DArrayDesc(
        uint32 width, uint32 height,
        uint32 arraySize,
        EPixelFormat format,
        ERHITextureUsage usage = ERHITextureUsage::Default,
        uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture2DArray;
        desc.width = width;
        desc.height = height;
        desc.depthOrArraySize = arraySize;
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// キューブマップの作成Desc
    inline RHITextureDesc CreateTextureCubeDesc(
        uint32 size,
        EPixelFormat format,
        ERHITextureUsage usage = ERHITextureUsage::Default,
        uint32 mipLevels = 1)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::TextureCube;
        desc.width = size;
        desc.height = size;
        desc.depthOrArraySize = 6;  // 6面
        desc.format = format;
        desc.mipLevels = mipLevels;
        desc.usage = usage;
        return desc;
    }

    /// UAVテクスチャ作成Desc
    inline RHITextureDesc CreateUAVTextureDesc(
        uint32 width, uint32 height,
        EPixelFormat format)
    {
        RHITextureDesc desc;
        desc.dimension = ERHITextureDimension::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.format = format;
        desc.usage = ERHITextureUsage::UnorderedShaderResource;
        return desc;
    }
}
```

- [ ] CreateTexture2DDesc
- [ ] CreateRenderTargetDesc
- [ ] CreateDepthStencilDesc
- [ ] CreateTexture3DDesc / CreateTexture2DArrayDesc
- [ ] CreateTextureCubeDesc
- [ ] CreateUAVTextureDesc

### 4. MIPレベル計算

```cpp
namespace NS::RHI
{
    /// 最大MIPレベル数を計算
    inline uint32 CalculateMaxMipLevels(uint32 width, uint32 height = 1, uint32 depth = 1)
    {
        uint32 maxDim = std::max({width, height, depth});
        uint32 levels = 1;
        while (maxDim > 1) {
            maxDim >>= 1;
            ++levels;
        }
        return levels;
    }

    /// 指定MIPレベルのサイズを計算
    inline uint32 CalculateMipSize(uint32 baseSize, uint32 mipLevel)
    {
        return std::max(1u, baseSize >> mipLevel);
    }

    /// Descの自動MIPレベル数を解決
    inline void ResolveMipLevels(RHITextureDesc& desc)
    {
        if (desc.mipLevels == 0) {
            desc.mipLevels = CalculateMaxMipLevels(
                desc.width, desc.height,
                Is3DTexture(desc.dimension) ? desc.depthOrArraySize : 1);
        }
    }

    /// 指定MIPレベルの3Dサイズ取得
    inline Extent3D GetMipLevelExtent(const RHITextureDesc& desc, uint32 mipLevel)
    {
        return Extent3D{
            CalculateMipSize(desc.width, mipLevel),
            CalculateMipSize(desc.height, mipLevel),
            Is3DTexture(desc.dimension)
                ? CalculateMipSize(desc.depthOrArraySize, mipLevel)
                : 1
        };
    }
}
```

- [ ] CalculateMaxMipLevels
- [ ] CalculateMipSize
- [ ] ResolveMipLevels
- [ ] GetMipLevelExtent

### 5. テクスチャメモリサイズ推定

```cpp
namespace NS::RHI
{
    /// テクスチャのメモリサイズを推定（概算）
    /// @note 実際の割り当てはアライメント等で異なる場合あめ
    inline MemorySize EstimateTextureMemorySize(const RHITextureDesc& desc)
    {
        uint32 bpp = GetPixelFormatBitsPerPixel(desc.format);
        if (bpp == 0) return 0;

        MemorySize totalSize = 0;
        uint32 arraySize = IsArrayTexture(desc.dimension) ? desc.depthOrArraySize : 1;
        uint32 depth = Is3DTexture(desc.dimension) ? desc.depthOrArraySize : 1;

        // キューブマップのは6面
        if (IsCubeTexture(desc.dimension)) {
            arraySize = desc.depthOrArraySize; // 通常6の倍数
        }

        for (uint32 mip = 0; mip < desc.mipLevels; ++mip) {
            uint32 w = CalculateMipSize(desc.width, mip);
            uint32 h = CalculateMipSize(desc.height, mip);
            uint32 d = Is3DTexture(desc.dimension) ? CalculateMipSize(depth, mip) : 1;

            MemorySize mipSize = (w * h * d * bpp + 7) / 8;  // bits to bytes
            totalSize += mipSize * arraySize;
        }

        // マルチサンプル
        totalSize *= GetSampleCount(desc.sampleCount);

        return totalSize;
    }

    /// 人間が読みやすい式でサイズを取得
    inline const char* FormatMemorySize(MemorySize bytes, char* buffer, size_t bufferSize)
    {
        if (bytes >= kGigabyte) {
            snprintf(buffer, bufferSize, "%.2f GB", static_cast<double>(bytes) / kGigabyte);
        } else if (bytes >= kMegabyte) {
            snprintf(buffer, bufferSize, "%.2f MB", static_cast<double>(bytes) / kMegabyte);
        } else if (bytes >= kKilobyte) {
            snprintf(buffer, bufferSize, "%.2f KB", static_cast<double>(bytes) / kKilobyte);
        } else {
            snprintf(buffer, bufferSize, "%llu B", static_cast<unsigned long long>(bytes));
        }
        return buffer;
    }
}
```

- [ ] EstimateTextureMemorySize
- [ ] FormatMemorySize

## 検証方法

- [ ] Desc構造体のサイズ・アライメント確認
- [ ] ヘルパー関数の出力が正しいusageフラグを持つか
- [ ] MIP計算の正確性
- [ ] メモリサイズ推定の妥当性

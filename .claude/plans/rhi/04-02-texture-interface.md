# 04-02: IRHITextureインターフェース

## 目的

テクスチャリソースのコアインターフェースを定義する。

## 参照ドキュメント

- 04-01-texture-desc.md (RHITextureDesc)
- 01-12-resource-base.md (IRHIResource)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHITexture.h` (部分

## TODO

### 1. IRHITexture: 基本インターフェース

```cpp
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHITypes.h"
#include "RHIPixelFormat.h"

namespace NS::RHI
{
    /// テクスチャリソース
    /// GPU上のイメージデータ
    class RHI_API IRHITexture : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Texture)

        virtual ~IRHITexture() = default;

        //=====================================================================
        // ライフサイクル契約
        //=====================================================================
        //
        // - IRHITextureはTRefCountPtrで管理される
        // - 参照カウントが0になるとOnZeroRefCount()が呼ばれ、
        //   RHIDeferredDeleteQueue::Enqueue()によりGPU完了まで削除を遅延する
        // - GPU操作が完了するまでテクスチャメモリは解放されない
        // - ビュー（SRV/RTV/DSV/UAV）作成はスレッドセーフではない。
        //   同一テクスチャへの並行ビュー作成には外部同期が必要
        // - Placed resourceの場合、ヒープが全placed resourceより長生きすること
        //

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 次元取得
        virtual ERHITextureDimension GetDimension() const = 0;

        /// 幅取得（ピクセル）。
        virtual uint32 GetWidth() const = 0;

        /// 高さ取得（ピクセル）。
        virtual uint32 GetHeight() const = 0;

        /// 深度取得（3Dテクスチャの場合）
        virtual uint32 GetDepth() const = 0;

        /// 配列サイズ取得
        virtual uint32 GetArraySize() const = 0;

        /// 深度または配列サイズ取得
        uint32 GetDepthOrArraySize() const {
            return Is3DTexture(GetDimension()) ? GetDepth() : GetArraySize();
        }

        /// 2Dサイズ取得
        Extent2D GetSize2D() const {
            return Extent2D{GetWidth(), GetHeight()};
        }

        /// 3Dサイズ取得
        Extent3D GetSize3D() const {
            return Extent3D{GetWidth(), GetHeight(), GetDepth()};
        }

        //=====================================================================
        // フォーマット
        //=====================================================================

        /// ピクセルフォーマット取得
        virtual EPixelFormat GetFormat() const = 0;

        /// ピクセルあたりのビット数取得
        uint32 GetBitsPerPixel() const {
            return GetPixelFormatBitsPerPixel(GetFormat());
        }

        /// 圧縮フォーマットか
        bool IsCompressed() const {
            return IsCompressedFormat(GetFormat());
        }

        /// sRGBフォーマットか
        bool IsSRGB() const {
            return IsSRGBFormat(GetFormat());
        }
    };

    /// テクスチャ参照カウントポインタ
    using RHITextureRef = TRefCountPtr<IRHITexture>;
}
```

- [ ] IRHITexture 基本インターフェース
- [ ] サイズ・フォーマット取得

### 2. MIPマップのサンプル情報

```cpp
namespace NS::RHI
{
    class IRHITexture
    {
    public:
        //=====================================================================
        // MIPマッチ
        //=====================================================================

        /// MIPレベル数取得
        virtual uint32 GetMipLevels() const = 0;

        /// 指定MIPレベルのサイズ取得
        Extent3D GetMipSize(uint32 mipLevel) const {
            return Extent3D{
                CalculateMipSize(GetWidth(), mipLevel),
                CalculateMipSize(GetHeight(), mipLevel),
                Is3DTexture(GetDimension())
                    ? CalculateMipSize(GetDepth(), mipLevel)
                    : 1
            };
        }

        /// MIPレベルが有効か
        bool IsValidMipLevel(uint32 mipLevel) const {
            return mipLevel < GetMipLevels();
        }

        //=====================================================================
        // マルチサンプル
        //=====================================================================

        /// サンプル数取得
        virtual ERHISampleCount GetSampleCount() const = 0;

        /// サンプル品質取得
        virtual uint32 GetSampleQuality() const = 0;

        /// マルチサンプルか
        bool IsMultisampled() const {
            return NS::RHI::IsMultisampled(GetSampleCount());
        }
    };
}
```

- [ ] GetMipLevels / GetMipSize
- [ ] GetSampleCount / GetSampleQuality

### 3. 使用フラグ判定

```cpp
namespace NS::RHI
{
    class IRHITexture
    {
    public:
        //=====================================================================
        // 使用フラグ
        //=====================================================================

        /// 使用フラグ取得
        virtual ERHITextureUsage GetUsage() const = 0;

        /// シェーダーリソースとして使用可能か
        bool CanCreateSRV() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::ShaderResource);
        }

        /// UAVとして使用可能か
        bool CanCreateUAV() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::UnorderedAccess);
        }

        /// レンダーターゲットとして使用可能か
        bool CanCreateRTV() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::RenderTarget);
        }

        /// デプスステンシルとして使用可能か
        bool CanCreateDSV() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::DepthStencil);
        }

        /// CPUから読み取り可能か
        bool IsCPUReadable() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::CPUReadable);
        }

        /// CPUから書き込み可能か
        bool IsCPUWritable() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::CPUWritable);
        }

        /// プレゼント可能か
        bool IsPresentable() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::Present);
        }

        /// MIP自動生成対応か
        bool SupportsGGenerateMips() const {
            return EnumHasAnyFlags(GetUsage(), ERHITextureUsage::GGenerateMips);
        }
    };
}
```

- [ ] GetUsage
- [ ] 使用フラグ判定メソッド群

### 4. 次元判定メソッド

```cpp
namespace NS::RHI
{
    class IRHITexture
    {
    public:
        //=====================================================================
        // 次元判定
        //=====================================================================

        /// 1Dテクスチャか
        bool Is1D() const {
            auto dim = GetDimension();
            return dim == ERHITextureDimension::Texture1D
                || dim == ERHITextureDimension::Texture1DArray;
        }

        /// 2Dテクスチャか
        bool Is2D() const {
            auto dim = GetDimension();
            return dim == ERHITextureDimension::Texture2D
                || dim == ERHITextureDimension::Texture2DArray
                || dim == ERHITextureDimension::Texture2DMS
                || dim == ERHITextureDimension::Texture2DMSArray;
        }

        /// 3Dテクスチャか
        bool Is3D() const {
            return GetDimension() == ERHITextureDimension::Texture3D;
        }

        /// キューブマップのか
        bool IsCube() const {
            return IsCubeTexture(GetDimension());
        }

        /// 配列テクスチャか
        bool IsArray() const {
            return IsArrayTexture(GetDimension());
        }
    };
}
```

- [ ] 次元判定メソッド群

### 5. メモリ情報

```cpp
namespace NS::RHI
{
    /// テクスチャメモリ情報
    struct RHI_API RHITextureMemoryInfo
    {
        /// 実際に割り当てられたサイズ
        MemorySize allocatedSize = 0;

        /// 使用可能サイズ
        MemorySize usableSize = 0;

        /// メモリタイプ
        ERHIHeapType heapType = ERHIHeapType::Default;

        /// 行ピッチ（バイト、リニアレイアウトの場合）
        uint32 rowPitch = 0;

        /// スライスピッチ（バイト）
        uint32 slicePitch = 0;

        /// アライメント
        uint32 alignment = 0;

        /// タイル配置（Virtual Textureの場合）
        bool isTiled = false;
    };

    class IRHITexture
    {
    public:
        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// メモリ情報を取得
        virtual RHITextureMemoryInfo GetMemoryInfo() const = 0;

        /// 割り当てサイズ取得
        MemorySize GetAllocatedSize() const {
            return GetMemoryInfo().allocatedSize;
        }

        /// ヒープタイプ取得
        ERHIHeapType GetHeapType() const {
            return GetMemoryInfo().heapType;
        }
    };
}
```

- [ ] RHITextureMemoryInfo 構造体
- [ ] GetMemoryInfo

### 6. クリア値・最適化ヒント

```cpp
namespace NS::RHI
{
    class IRHITexture
    {
    public:
        //=====================================================================
        // クリア値
        //=====================================================================

        /// 最適クリア値取得
        /// レンダーターゲット/デプスの場合に有効
        virtual RHIClearValue GetClearValue() const = 0;

        /// 最適クリア値でクリアすべきか
        /// @note 作成時に指定したクリア値と一致する場合、最適化される
        bool HasOptimizedClearValue() const {
            return CanCreateRTV() || CanCreateDSV();
        }

        //=====================================================================
        // スワップチェーン関連
        //=====================================================================

        /// スワップチェーン所属か
        virtual bool IsSwapChainTexture() const { return false; }

        /// 所属スワップチェーン取得
        virtual IRHISwapChain* GetSwapChain() const { return nullptr; }

        /// スワップチェーン内インデックス取得
        virtual uint32 GetSwapChainBufferIndex() const { return 0; }
    };
}
```

- [ ] GetClearValue
- [ ] スワップチェーン関連

### 7. 記述情報を取得

```cpp
namespace NS::RHI
{
    class IRHITexture
    {
    public:
        //=====================================================================
        // 記述情報
        //=====================================================================

        /// 作成時の記述情報をの構築
        RHITextureDesc GetDesc() const {
            RHITextureDesc desc;
            desc.dimension = GetDimension();
            desc.width = GetWidth();
            desc.height = GetHeight();
            desc.depthOrArraySize = GetDepthOrArraySize();
            desc.format = GetFormat();
            desc.mipLevels = GetMipLevels();
            desc.sampleCount = GetSampleCount();
            desc.sampleQuality = GetSampleQuality();
            desc.usage = GetUsage();
            desc.clearValue = GetClearValue();
            desc.debugName = GetDebugName();
            return desc;
        }

        /// 同じパラメータで新しいテクスチャを作成可能なDescを取得
        RHITextureDesc GetCloneDesc() const {
            RHITextureDesc desc = GetDesc();
            desc.debugName = nullptr;  // デバッグ名のクリア
            return desc;
        }
    };
}
```

- [ ] GetDesc / GetCloneDesc

## 検証方法

- [ ] インターフェースの完全性
- [ ] 使用フラグ判定の正確性
- [ ] 次元判定の正確性
- [ ] メモリ情報の整合性

# 18-01: サンプラー記述

## 目的

テクスチャサンプラーの記述構造を定義する。

## 参照ドキュメント

- 08-03-root-signature.md (RHIStaticSamplerDesc)
- 01-10-enums-state.md (比較関数等

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHISampler.h`

## TODO

### 1. サンプラー記述

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// フィルターモード
    enum class ERHIFilter : uint8
    {
        Point,
        Linear,
        Anisotropic,
    };

    /// テクスチャアドレスモード
    enum class ERHITextureAddressMode : uint8
    {
        Wrap,
        Mirror,
        Clamp,
        Border,
        MirrorOnce,
    };

    /// ボーダーカラー
    enum class ERHIBorderColor : uint8
    {
        TransparentBlack,   // (0, 0, 0, 0)
        OpaqueBlack,        // (0, 0, 0, 1)
        OpaqueWhite,        // (1, 1, 1, 1)
    };

    /// サンプラー記述
    struct RHI_API RHISamplerDesc
    {
        /// 縮小フィルター
        ERHIFilter minFilter = ERHIFilter::Linear;

        /// 拡大フィルター
        ERHIFilter magFilter = ERHIFilter::Linear;

        /// ミップマップフィルター
        ERHIFilter mipFilter = ERHIFilter::Linear;

        /// アドレスモード
        ERHITextureAddressMode addressU = ERHITextureAddressMode::Wrap;
        ERHITextureAddressMode addressV = ERHITextureAddressMode::Wrap;
        ERHITextureAddressMode addressW = ERHITextureAddressMode::Wrap;

        /// MIPバイアス
        float mipLODBias = 0.0f;

        /// 最大異方性（Anisotropic時）
        uint32 maxAnisotropy = 16;

        /// 比較関数（シャドウマップ等）
        ERHICompareFunc comparisonFunc = ERHICompareFunc::Never;

        /// 比較サンプラーとして使用
        bool enableComparison = false;

        /// ボーダーカラー
        ERHIBorderColor borderColor = ERHIBorderColor::OpaqueBlack;

        /// カスタムボーダーカラー（BorderColor未使用時）
        float customBorderColor[4] = {0, 0, 0, 1};
        bool useCustomBorderColor = false;

        /// 最小LOD
        float minLOD = 0.0f;

        /// 最大LOD
        float maxLOD = 1000.0f;

        //=====================================================================
        // プリセット
        //=====================================================================

        /// ポイントサンプラー
        static RHISamplerDesc Point() {
            RHISamplerDesc desc;
            desc.minFilter = ERHIFilter::Point;
            desc.magFilter = ERHIFilter::Point;
            desc.mipFilter = ERHIFilter::Point;
            return desc;
        }

        /// ポイントクランプ
        static RHISamplerDesc PointClamp() {
            RHISamplerDesc desc = Point();
            desc.addressU = ERHITextureAddressMode::Clamp;
            desc.addressV = ERHITextureAddressMode::Clamp;
            desc.addressW = ERHITextureAddressMode::Clamp;
            return desc;
        }

        /// リニアサンプラー
        static RHISamplerDesc Linear() {
            RHISamplerDesc desc;
            desc.minFilter = ERHIFilter::Linear;
            desc.magFilter = ERHIFilter::Linear;
            desc.mipFilter = ERHIFilter::Linear;
            return desc;
        }

        /// リニアクランプ
        static RHISamplerDesc LinearClamp() {
            RHISamplerDesc desc = Linear();
            desc.addressU = ERHITextureAddressMode::Clamp;
            desc.addressV = ERHITextureAddressMode::Clamp;
            desc.addressW = ERHITextureAddressMode::Clamp;
            return desc;
        }

        /// 異方性サンプラー
        static RHISamplerDesc Anisotropic(uint32 maxAniso = 16) {
            RHISamplerDesc desc;
            desc.minFilter = ERHIFilter::Anisotropic;
            desc.magFilter = ERHIFilter::Anisotropic;
            desc.mipFilter = ERHIFilter::Linear;
            desc.maxAnisotropy = maxAniso;
            return desc;
        }

        /// シャドウマップ（PCF）。
        static RHISamplerDesc ShadowPCF() {
            RHISamplerDesc desc;
            desc.minFilter = ERHIFilter::Linear;
            desc.magFilter = ERHIFilter::Linear;
            desc.mipFilter = ERHIFilter::Point;
            desc.addressU = ERHITextureAddressMode::Border;
            desc.addressV = ERHITextureAddressMode::Border;
            desc.addressW = ERHITextureAddressMode::Border;
            desc.borderColor = ERHIBorderColor::OpaqueWhite;
            desc.enableComparison = true;
            desc.comparisonFunc = ERHICompareFunc::LessEqual;
            return desc;
        }

        /// シャドウマップ（ポイント）
        static RHISamplerDesc ShadowPoint() {
            RHISamplerDesc desc = ShadowPCF();
            desc.minFilter = ERHIFilter::Point;
            desc.magFilter = ERHIFilter::Point;
            return desc;
        }
    };
}
```

- [ ] ERHIFilter 列挙型
- [ ] ERHITextureAddressMode 列挙型
- [ ] RHISamplerDesc 構造体
- [ ] プリセット

### 2. サンプラービルダー

```cpp
namespace NS::RHI
{
    /// サンプラービルダー
    class RHI_API RHISamplerBuilder
    {
    public:
        RHISamplerBuilder() = default;

        /// フィルター設定
        RHISamplerBuilder& SetFilter(ERHIFilter min, ERHIFilter mag, ERHIFilter mip) {
            m_desc.minFilter = min;
            m_desc.magFilter = mag;
            m_desc.mipFilter = mip;
            return *this;
        }

        /// 全フィルター同一設定
        RHISamplerBuilder& SetFilter(ERHIFilter filter) {
            return SetFilter(filter, filter, filter);
        }

        /// 異方性設定
        RHISamplerBuilder& SetAnisotropic(uint32 maxAniso = 16) {
            m_desc.minFilter = ERHIFilter::Anisotropic;
            m_desc.magFilter = ERHIFilter::Anisotropic;
            m_desc.maxAnisotropy = maxAniso;
            return *this;
        }

        /// アドレスモード設定
        RHISamplerBuilder& SetAddressMode(
            ERHITextureAddressMode u,
            ERHITextureAddressMode v,
            ERHITextureAddressMode w)
        {
            m_desc.addressU = u;
            m_desc.addressV = v;
            m_desc.addressW = w;
            return *this;
        }

        /// 全アドレスモード同一設定
        RHISamplerBuilder& SetAddressMode(ERHITextureAddressMode mode) {
            return SetAddressMode(mode, mode, mode);
        }

        /// MIPバイアス設定
        RHISamplerBuilder& SetMipLODBias(float bias) {
            m_desc.mipLODBias = bias;
            return *this;
        }

        /// LOD範囲を設定
        RHISamplerBuilder& SetLODRange(float minLOD, float maxLOD) {
            m_desc.minLOD = minLOD;
            m_desc.maxLOD = maxLOD;
            return *this;
        }

        /// 比較関数設定
        RHISamplerBuilder& SetComparison(ERHICompareFunc func) {
            m_desc.enableComparison = true;
            m_desc.comparisonFunc = func;
            return *this;
        }

        /// ボーダーカラー設定
        RHISamplerBuilder& SetBorderColor(ERHIBorderColor color) {
            m_desc.borderColor = color;
            m_desc.useCustomBorderColor = false;
            return *this;
        }

        /// カスタムボーダーカラー設定
        RHISamplerBuilder& SetCustomBorderColor(float r, float g, float b, float a) {
            m_desc.customBorderColor[0] = r;
            m_desc.customBorderColor[1] = g;
            m_desc.customBorderColor[2] = b;
            m_desc.customBorderColor[3] = a;
            m_desc.useCustomBorderColor = true;
            return *this;
        }

        /// ビルド
        RHISamplerDesc Build() const { return m_desc; }

    private:
        RHISamplerDesc m_desc;
    };
}
```

- [ ] RHISamplerBuilder クラス

### 3. サンプラーハッシュ

```cpp
namespace NS::RHI
{
    /// サンプラー記述のハッシュ計算
    RHI_API uint64 CalculateSamplerDescHash(const RHISamplerDesc& desc);

    /// サンプラー記述の比較
    inline bool operator==(const RHISamplerDesc& a, const RHISamplerDesc& b) {
        return CalculateSamplerDescHash(a) == CalculateSamplerDescHash(b);
    }

    inline bool operator!=(const RHISamplerDesc& a, const RHISamplerDesc& b) {
        return !(a == b);
    }
}
```

- [ ] CalculateSamplerDescHash

## 検証方法

- [ ] 記述構造体の完全性
- [ ] プリセットの正確性
- [ ] ビルダーの動作
- [ ] ハッシュ計算

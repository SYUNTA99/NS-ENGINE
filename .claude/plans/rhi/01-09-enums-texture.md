# 01-09: テクスチャ使用フラグ列挙型

## 目的

テクスチャ作成時の使用フラグ、次元・関連列挙型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (テクスチャ)
- docs/RHI/RHI_Implementation_Guide_Part16.md (列挙型リファレンス)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIEnums.h` (部分)

## TODO

### 1. ERHITextureUsage: テクスチャ使用フラグ

```cpp
namespace NS::RHI
{
    /// テクスチャ使用フラグ（ビットフラグ）
    enum class ERHITextureUsage : uint32
    {
        None                = 0,

        //=========================================================================
        // シェーダーリソース
        //=========================================================================

        /// シェーダーリソースビュー（SRV）として使用
        ShaderResource      = 1 << 0,

        /// アンオーダードアクセスビュー（UAV）として使用
        UnorderedAccess     = 1 << 1,

        //=========================================================================
        // レンダーターゲット
        //=========================================================================

        /// レンダーターゲットとして使用
        RenderTarget        = 1 << 2,

        /// デプスステンシルとして使用
        DepthStencil        = 1 << 3,

        //=========================================================================
        // スワップチェーン/表示
        //=========================================================================

        /// プレゼント可能（スワップチェーンバックバッファ）
        Present             = 1 << 4,

        /// 他プロセス/デバイスと共有可能
        Shared              = 1 << 5,

        //=========================================================================
        // CPU アクセス
        //=========================================================================

        /// CPUから読み取り可能（リードバック）
        CPUReadable         = 1 << 6,

        /// CPUから書き込み可能（アップロード）
        CPUWritable         = 1 << 7,

        //=========================================================================
        // 特殊機能
        //=========================================================================

        /// MIPマップ自動生成可能
        GenerateMips        = 1 << 8,

        /// 仮想テクスチャ（タイルリソース）
        Virtual             = 1 << 9,

        /// ストリーミング対応
        Streamable          = 1 << 10,

        /// 可変レートシェーディングソース
        ShadingRateSource   = 1 << 11,

        /// メモリレス（タイルベースGPU用）
        Memoryless          = 1 << 12,

        /// リゾルブ元/先
        ResolveSource       = 1 << 13,
        ResolveDest         = 1 << 14,

        //=========================================================================
        // 便利な組み合わせ
        //=========================================================================

        /// 標準2Dテクスチャ
        Default = ShaderResource,

        /// レンダーターゲット + シェーダーリソース
        RenderTargetShaderResource = RenderTarget | ShaderResource,

        /// デプスバッファ + シェーダーリソース
        DepthShaderResource = DepthStencil | ShaderResource,

        /// UAV + SRV
        UnorderedShaderResource = UnorderedAccess | ShaderResource,

        /// ステージングテクスチャ
        Staging = CPUReadable | CPUWritable,
    };
    RHI_ENUM_CLASS_FLAGS(ERHITextureUsage)
}
```

- [ ] ERHITextureUsage ビットフラグ
- [ ] 便利な組み合わせ定数

### 2. ERHITextureDimension: テクスチャ次元

```cpp
namespace NS::RHI
{
    /// テクスチャ次元
    enum class ERHITextureDimension : uint8
    {
        Texture1D,          // 1Dテクスチャ
        Texture1DArray,     // 1D配列テクスチャ
        Texture2D,          // 2Dテクスチャ
        Texture2DArray,     // 2D配列テクスチャ
        Texture2DMS,        // 2Dマルチサンプル
        Texture2DMSArray,   // 2Dマルチサンプル配列
        Texture3D,          // 3Dテクスチャ（ボリューム）
        TextureCube,        // キューブマップ
        TextureCubeArray,   // キューブマップ配列
    };

    /// 次元名取得
    inline const char* GetTextureDimensionName(ERHITextureDimension dim)
    {
        switch (dim) {
            case ERHITextureDimension::Texture1D:       return "1D";
            case ERHITextureDimension::Texture1DArray:  return "1DArray";
            case ERHITextureDimension::Texture2D:       return "2D";
            case ERHITextureDimension::Texture2DArray:  return "2DArray";
            case ERHITextureDimension::Texture2DMS:     return "2DMS";
            case ERHITextureDimension::Texture2DMSArray:return "2DMSArray";
            case ERHITextureDimension::Texture3D:       return "3D";
            case ERHITextureDimension::TextureCube:     return "Cube";
            case ERHITextureDimension::TextureCubeArray:return "CubeArray";
            default:                                     return "Unknown";
        }
    }

    /// 配列テクスチャか
    inline bool IsArrayTexture(ERHITextureDimension dim) {
        switch (dim) {
            case ERHITextureDimension::Texture1DArray:
            case ERHITextureDimension::Texture2DArray:
            case ERHITextureDimension::Texture2DMSArray:
            case ERHITextureDimension::TextureCubeArray:
                return true;
            default:
                return false;
        }
    }

    /// マルチサンプルテクスチャか
    inline bool IsMultisampleTexture(ERHITextureDimension dim) {
        return dim == ERHITextureDimension::Texture2DMS
            || dim == ERHITextureDimension::Texture2DMSArray;
    }

    /// キューブマップか
    inline bool IsCubeTexture(ERHITextureDimension dim) {
        return dim == ERHITextureDimension::TextureCube
            || dim == ERHITextureDimension::TextureCubeArray;
    }

    /// 3Dテクスチャか
    inline bool Is3DTexture(ERHITextureDimension dim) {
        return dim == ERHITextureDimension::Texture3D;
    }
}
```

- [ ] ERHITextureDimension 列挙型
- [ ] 次元判定ユーティリティ

### 3. ERHITextureLayout: テクスチャレイアウト

```cpp
namespace NS::RHI
{
    /// テクスチャメモリレイアウト
    enum class ERHITextureLayout : uint8
    {
        /// 最適レイアウト（GPU内部形式）
        Optimal,

        /// 線形レイアウト（CPU読み書き可能）
        Linear,

        /// 不明/初期状態
        Unknown,
    };

    /// CPUアクセス可能なレイアウトか
    inline bool IsCPUAccessibleLayout(ERHITextureLayout layout) {
        return layout == ERHITextureLayout::Linear;
    }
}
```

- [ ] ERHITextureLayout 列挙型

### 4. ERHISampleCount: サンプル数

```cpp
namespace NS::RHI
{
    /// マルチサンプルカウント
    enum class ERHISampleCount : uint8
    {
        Count1  = 1,    // マルチサンプルなし
        Count2  = 2,
        Count4  = 4,
        Count8  = 8,
        Count16 = 16,
        Count32 = 32,
    };

    /// サンプル数が有効か（1より大きい）
    inline bool IsMultisampled(ERHISampleCount count) {
        return static_cast<uint8>(count) > 1;
    }

    /// サンプル数取得
    inline uint32 GetSampleCount(ERHISampleCount count) {
        return static_cast<uint32>(count);
    }
}
```

- [ ] ERHISampleCount 列挙型
- [ ] サンプル数ユーティリティ

### 5. ERHITextureSwizzle: テクスチャスウィズル

```cpp
namespace NS::RHI
{
    /// コンポーネントスウィズル
    enum class ERHIComponentSwizzle : uint8
    {
        Identity,   // そのまま
        Zero,       // 0
        One,        // 1
        R,          // Rチャンネル
        G,          // Gチャンネル
        B,          // Bチャンネル
        A,          // Aチャンネル
    };

    /// テクスチャスウィズルマッピング
    struct RHIComponentMapping
    {
        ERHIComponentSwizzle r = ERHIComponentSwizzle::Identity;
        ERHIComponentSwizzle g = ERHIComponentSwizzle::Identity;
        ERHIComponentSwizzle b = ERHIComponentSwizzle::Identity;
        ERHIComponentSwizzle a = ERHIComponentSwizzle::Identity;

        /// デフォルト（Identity）
        static constexpr RHIComponentMapping Identity() {
            return RHIComponentMapping{};
        }

        /// 全チャンネルを指定値に
        static constexpr RHIComponentMapping All(ERHIComponentSwizzle s) {
            return RHIComponentMapping{s, s, s, s};
        }
    };
}
```

- [ ] ERHIComponentSwizzle 列挙型
- [ ] RHIComponentMapping 構造体

## 検証方法

- [ ] フラグ組み合わせの妥当性
- [ ] 次元判定の正確性
- [ ] 使用例との整合性

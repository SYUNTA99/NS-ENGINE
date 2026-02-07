# 01-05: バックエンド・機能レベル・キュー列挙型

## 目的

RHIバックエンド、機能レベル、キュータイプの列挙型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (モジュール構成)
- docs/RHI/RHI_Implementation_Guide_Part9.md (ケイパビリティ)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIEnums.h` (部分)

## TODO

### 1. ERHIInterfaceType: バックエンド種別

```cpp
#pragma once

#include "Common/Utility/Types.h"
#include "RHIMacros.h"

namespace NS::RHI
{
    /// RHIバックエンド種別
    enum class ERHIInterfaceType : uint8
    {
        Hidden,     // 非表示（テスト用、内部実装）
        Null,       // Null実装（ヘッドレス）
        D3D11,      // DirectX 11
        D3D12,      // DirectX 12
        Vulkan,     // Vulkan
        Metal,      // Metal (macOS/iOS)

        Count
    };

    /// バックエンド名取得
    inline const char* GetRHIInterfaceTypeName(ERHIInterfaceType type)
    {
        switch (type) {
            case ERHIInterfaceType::Hidden:  return "Hidden";
            case ERHIInterfaceType::Null:    return "Null";
            case ERHIInterfaceType::D3D11:   return "D3D11";
            case ERHIInterfaceType::D3D12:   return "D3D12";
            case ERHIInterfaceType::Vulkan:  return "Vulkan";
            case ERHIInterfaceType::Metal:   return "Metal";
            default:                          return "Unknown";
        }
    }
}
```

- [ ] ERHIInterfaceType 列挙型
- [ ] 名前取得関数

### 2. ERHIFeatureLevel: 機能レベル

```cpp
namespace NS::RHI
{
    /// シェーダーモデル / 機能レベル
    enum class ERHIFeatureLevel : uint8
    {
        SM5,        // Shader Model 5.0 (D3D11相当)
        SM6,        // Shader Model 6.0
        SM6_1,      // Shader Model 6.1 (SV_Barycentrics)
        SM6_2,      // Shader Model 6.2 (FP16)
        SM6_3,      // Shader Model 6.3 (DXR 1.0)
        SM6_4,      // Shader Model 6.4 (VRS)
        SM6_5,      // Shader Model 6.5 (DXR 1.1, Mesh Shaders)
        SM6_6,      // Shader Model 6.6 (Atomic64, Dynamic Resources)
        SM6_7,      // Shader Model 6.7

        Count
    };

    /// 機能レベル名取得
    inline const char* GetFeatureLevelName(ERHIFeatureLevel level)
    {
        switch (level) {
            case ERHIFeatureLevel::SM5:   return "SM5";
            case ERHIFeatureLevel::SM6:   return "SM6.0";
            case ERHIFeatureLevel::SM6_1: return "SM6.1";
            case ERHIFeatureLevel::SM6_2: return "SM6.2";
            case ERHIFeatureLevel::SM6_3: return "SM6.3";
            case ERHIFeatureLevel::SM6_4: return "SM6.4";
            case ERHIFeatureLevel::SM6_5: return "SM6.5";
            case ERHIFeatureLevel::SM6_6: return "SM6.6";
            case ERHIFeatureLevel::SM6_7: return "SM6.7";
            default:                       return "Unknown";
        }
    }

    /// 機能レベル比較
    inline bool SupportsFeatureLevel(ERHIFeatureLevel actual, ERHIFeatureLevel required) {
        return static_cast<uint8>(actual) >= static_cast<uint8>(required);
    }
}
```

- [ ] ERHIFeatureLevel 列挙型
- [ ] 名前取得関数
- [ ] レベル比較関数

### 3. ERHIFeatureSupport: サポート状態

```cpp
namespace NS::RHI
{
    /// 機能サポート状態
    enum class ERHIFeatureSupport : uint8
    {
        Unsupported,          // 未サポート（ハードウェア非対応）
        RuntimeDependent,     // ランタイム依存（ドライバ確認必要）
        RuntimeGuaranteed,    // ランタイム保証（必ずサポート）
    };

    /// サポートしているか
    inline bool IsFeatureSupported(ERHIFeatureSupport support) {
        return support != ERHIFeatureSupport::Unsupported;
    }
}
```

- [ ] ERHIFeatureSupport 列挙型

### 4. ERHIQueueType: キュータイプ

```cpp
namespace NS::RHI
{
    /// コマンドキュータイプ
    enum class ERHIQueueType : uint8
    {
        Graphics,   // グラフィックス（描画 + コンピュート + コピー）
        Compute,    // 非同期コンピュート（コンピュート + コピー）
        Copy,       // コピー専用（DMA）

        Count
    };

    /// キュータイプ名取得
    inline const char* GetQueueTypeName(ERHIQueueType type)
    {
        switch (type) {
            case ERHIQueueType::Graphics: return "Graphics";
            case ERHIQueueType::Compute:  return "Compute";
            case ERHIQueueType::Copy:     return "Copy";
            default:                       return "Unknown";
        }
    }

    /// グラフィックス機能をサポートするか
    inline bool QueueSupportsGraphics(ERHIQueueType type) {
        return type == ERHIQueueType::Graphics;
    }

    /// コンピュート機能をサポートするか
    inline bool QueueSupportsCompute(ERHIQueueType type) {
        return type == ERHIQueueType::Graphics || type == ERHIQueueType::Compute;
    }

    /// コピー機能をサポートするか
    inline bool QueueSupportsCopy(ERHIQueueType type) {
        return true;  // 全キューでサポート
    }
}
```

- [ ] ERHIQueueType 列挙型
- [ ] 機能サポートクエリ関数

### 5. ERHIPipeline: パイプラインタイプ

```cpp
namespace NS::RHI
{
    /// パイプラインタイプ
    /// コマンドコンテキストの種類を識別
    enum class ERHIPipeline : uint8
    {
        Graphics,       // グラフィックスパイプライン
        AsyncCompute,   // 非同期コンピュートパイプライン

        Count
    };

    /// 対応するキュータイプ取得
    inline ERHIQueueType GetQueueTypeForPipeline(ERHIPipeline pipeline)
    {
        switch (pipeline) {
            case ERHIPipeline::Graphics:     return ERHIQueueType::Graphics;
            case ERHIPipeline::AsyncCompute: return ERHIQueueType::Compute;
            default:                          return ERHIQueueType::Graphics;
        }
    }
}
```

- [ ] ERHIPipeline 列挙型
- [ ] キュータイプ変換関数

## 検証方法

- [ ] 列挙値の重複なし
- [ ] 名前取得関数の網羅性
- [ ] 変換関数の正確性

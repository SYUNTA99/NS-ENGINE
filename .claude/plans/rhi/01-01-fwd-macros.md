# 01-01: 前方宣言・マクロ

## 目的

RHI全体で使用する前方宣言とマクロを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (モジュール構成)
- Source/Common/Utility/Macros.h (既存マクロパターン)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIFwd.h`
- `Source/Engine/RHI/Public/RHIMacros.h`

## TODO

### 1. RHIFwd.h: インターフェース前方宣言

```cpp
#pragma once

namespace NS::RHI
{
    // リソース基底
    class IRHIResource;

    // バッファ・テクスチャ
    class IRHIBuffer;
    class IRHITexture;

    // ビュー
    class IRHIShaderResourceView;
    class IRHIUnorderedAccessView;
    class IRHIRenderTargetView;
    class IRHIDepthStencilView;
    class IRHIConstantBufferView;

    // サンプラー
    class IRHISampler;

    // シェーダー・パイプライン
    class IRHIShader;
    class IRHIGraphicsPipelineState;
    class IRHIComputePipelineState;
    class IRHIRootSignature;

    // コマンド
    class IRHICommandContext;
    class IRHIComputeContext;
    class IRHICommandList;
    class IRHICommandAllocator;

    // 同期
    class IRHIFence;
    class IRHISyncPoint;

    // デバイス階層
    class IRHIQueue;
    class IRHIDevice;
    class IRHIAdapter;
    class IDynamicRHI;

    // スワップチェーン
    class IRHISwapChain;

    // クエリ
    class IRHIQueryHeap;

    // ディスクリプタ
    class IRHIDescriptorHeap;

    // スマートポインタ
    template<typename T> class TRefCountPtr;
}
```

- [ ] リソースインターフェース前方宣言
- [ ] コマンドインターフェース前方宣言
- [ ] デバイス階層前方宣言

### 2. RHIFwd.h: 構造体前方宣言

```cpp
namespace NS::RHI
{
    // 記述子
    struct RHIBufferDesc;
    struct RHITextureDesc;
    struct RHIShaderDesc;
    struct RHIGraphicsPipelineStateDesc;
    struct RHIComputePipelineStateDesc;
    struct RHISamplerDesc;
    struct RHISwapChainDesc;

    // アダプター情報
    struct RHIAdapterDesc;

    // レンダーパス
    struct RHIRenderPassInfo;

    // 状態
    struct RHIBlendStateDesc;
    struct RHIRasterizerStateDesc;
    struct RHIDepthStencilStateDesc;
}
```

- [ ] Desc構造体前方宣言

### 3. RHIMacros.h: エクスポートマクロ

```cpp
#pragma once

#include "Common/Utility/Macros.h"

//=============================================================================
// DLLエクスポート/インポート
//=============================================================================

#ifdef RHI_EXPORTS
    #define RHI_API NS_EXPORT
#else
    #define RHI_API NS_IMPORT
#endif

//=============================================================================
// インライン制御
//=============================================================================

#define RHI_FORCEINLINE NS_FORCEINLINE
#define RHI_NOINLINE NS_NOINLINE
```

- [ ] RHI_API マクロ
- [ ] インライン制御マクロ

### 4. RHIMacros.h: デバッグマクロ

```cpp
//=============================================================================
// デバッグ・検証
// チェックマクロ本体は RHICheck.h (17-01-rhi-result.md) で定義。
// RHIMacros.h では #include で取り込む。
//=============================================================================

#include "RHICheck.h"   // RHI_CHECK, RHI_CHECKF, RHI_ENSURE, RHI_VERIFY

// GPU検証レイヤー用
#if NS_BUILD_DEBUG
    #define RHI_GPU_VALIDATION 1
#else
    #define RHI_GPU_VALIDATION 0
#endif
```

マクロ体系:
- `RHICheck.h` (17-01): `RHI_CHECK`, `RHI_CHECKF`, `RHI_ENSURE`, `RHI_VERIFY` — 汎用チェック
- `RHIValidation.h` (17-03): `RHI_ASSERT`, `RHI_ASSERT_F`, `RHI_ASSERT_RESOURCE_VALID` 等 — デバッグ検証

- [ ] RHICheck.h のインクルード
- [ ] RHI_GPU_VALIDATION フラグ

### 5. RHIMacros.h: ビットフラグマクロ

```cpp
//=============================================================================
// enum class ビットフラグ演算子
//=============================================================================

#define RHI_ENUM_CLASS_FLAGS(EnumType) \
    inline constexpr EnumType operator|(EnumType a, EnumType b) { \
        return static_cast<EnumType>( \
            static_cast<uint32>(a) | static_cast<uint32>(b)); \
    } \
    inline constexpr EnumType operator&(EnumType a, EnumType b) { \
        return static_cast<EnumType>( \
            static_cast<uint32>(a) & static_cast<uint32>(b)); \
    } \
    inline constexpr EnumType operator^(EnumType a, EnumType b) { \
        return static_cast<EnumType>( \
            static_cast<uint32>(a) ^ static_cast<uint32>(b)); \
    } \
    inline constexpr EnumType operator~(EnumType a) { \
        return static_cast<EnumType>(~static_cast<uint32>(a)); \
    } \
    inline EnumType& operator|=(EnumType& a, EnumType b) { \
        return a = a | b; \
    } \
    inline EnumType& operator&=(EnumType& a, EnumType b) { \
        return a = a & b; \
    } \
    inline EnumType& operator^=(EnumType& a, EnumType b) { \
        return a = a ^ b; \
    } \
    inline constexpr bool EnumHasAnyFlags(EnumType a, EnumType b) { \
        return (static_cast<uint32>(a) & static_cast<uint32>(b)) != 0; \
    } \
    inline constexpr bool EnumHasAllFlags(EnumType a, EnumType b) { \
        return (static_cast<uint32>(a) & static_cast<uint32>(b)) \
            == static_cast<uint32>(b); \
    }
```

- [ ] RHI_ENUM_CLASS_FLAGS マクロ

## 検証方法

- [ ] コンパイル成功
- [ ] 循環依存なし
- [ ] 他RHIヘッダーからinclude可能

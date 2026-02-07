# 01-13: リソースタイプの型キャスト

## 目的

リソースタイプ識別と型安全なキャストユーティリティを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (リソース種別)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIResourceType.h`

## TODO

### 1. ERHIResourceType: リソースタイプ列挙

```cpp
#pragma once

#include "common/utility/types.h"

namespace NS::RHI
{
    /// RHIリソースタイプ
    enum class ERHIResourceType : uint8
    {
        Unknown = 0,

        //=====================================================================
        // GPU リソース
        //=====================================================================

        Buffer,             // バッファ
        Texture,            // テクスチャ

        //=====================================================================
        // ビュー
        //=====================================================================

        ShaderResourceView,     // SRV
        UnorderedAccessView,    // UAV
        RenderTargetView,       // RTV
        DepthStencilView,       // DSV
        ConstantBufferView,     // CBV

        //=====================================================================
        // サンプラー
        //=====================================================================

        Sampler,            // サンプラーステート

        //=====================================================================
        // シェーダー・パイプライン
        //=====================================================================

        Shader,                 // シェーダーバイトコード
        GraphicsPipelineState,  // グラフィックスPSO
        ComputePipelineState,   // コンピュートPSO
        RootSignature,          // ルートシグネチャ

        //=====================================================================
        // コマンド
        //=====================================================================

        CommandList,        // コマンドリスト
        CommandAllocator,   // コマンドアロケーター

        //=====================================================================
        // 同期
        //=====================================================================

        Fence,              // フェンス
        SyncPoint,          // 同期ポイント

        //=====================================================================
        // ディスクリプタ
        //=====================================================================

        DescriptorHeap,     // ディスクリプタヒープ

        //=====================================================================
        // クエリ
        //=====================================================================

        QueryHeap,          // クエリヒープ

        //=====================================================================
        // スワップチェーン
        //=====================================================================

        SwapChain,          // スワップチェーン

        //=====================================================================
        // レイトレーシング
        //=====================================================================

        AccelerationStructure,  // 加速構造（TLAS/BLAS）。
        RayTracingPSO,          // レイトレーシングPSO
        ShaderBindingTable,     // シェーダーバインディングテーブル

        //=====================================================================
        // メモリ
        //=====================================================================

        Heap,               // メモリヒープ

        //=====================================================================
        // その他
        //=====================================================================

        InputLayout,        // 入力レイアウト
        ShaderLibrary,      // シェーダーライブラリ
        ResourceCollection, // リソースコレクション

        Count
    };

    /// リソースタイプ名取得
    inline const char* GetResourceTypeName(ERHIResourceType type)
    {
        switch (type) {
            case ERHIResourceType::Buffer:              return "Buffer";
            case ERHIResourceType::Texture:             return "Texture";
            case ERHIResourceType::ShaderResourceView:  return "SRV";
            case ERHIResourceType::UnorderedAccessView: return "UAV";
            case ERHIResourceType::RenderTargetView:    return "RTV";
            case ERHIResourceType::DepthStencilView:    return "DSV";
            case ERHIResourceType::ConstantBufferView:  return "CBV";
            case ERHIResourceType::Sampler:             return "Sampler";
            case ERHIResourceType::Shader:              return "Shader";
            case ERHIResourceType::GraphicsPipelineState: return "GraphicsPSO";
            case ERHIResourceType::ComputePipelineState:  return "ComputePSO";
            case ERHIResourceType::RootSignature:       return "RootSignature";
            case ERHIResourceType::CommandList:         return "CommandList";
            case ERHIResourceType::CommandAllocator:    return "CommandAllocator";
            case ERHIResourceType::Fence:               return "Fence";
            case ERHIResourceType::SyncPoint:           return "SyncPoint";
            case ERHIResourceType::DescriptorHeap:      return "DescriptorHeap";
            case ERHIResourceType::QueryHeap:           return "QueryHeap";
            case ERHIResourceType::SwapChain:           return "SwapChain";
            case ERHIResourceType::AccelerationStructure: return "AccelerationStructure";
            case ERHIResourceType::RayTracingPSO:       return "RayTracingPSO";
            case ERHIResourceType::ShaderBindingTable:  return "ShaderBindingTable";
            case ERHIResourceType::Heap:                return "Heap";
            case ERHIResourceType::InputLayout:         return "InputLayout";
            case ERHIResourceType::ShaderLibrary:       return "ShaderLibrary";
            case ERHIResourceType::ResourceCollection:  return "ResourceCollection";
            default:                                     return "Unknown";
        }
    }
}
```

- [ ] ERHIResourceType 列挙型
- [ ] 名前取得関数

### 2. リソースタイプの分類

```cpp
namespace NS::RHI
{
    /// GPUメモリを占有するリソースか
    inline bool IsGPUResource(ERHIResourceType type)
    {
        switch (type) {
            case ERHIResourceType::Buffer:
            case ERHIResourceType::Texture:
            case ERHIResourceType::AccelerationStructure:
                return true;
            default:
                return false;
        }
    }

    /// ビュータイプか
    inline bool IsViewType(ERHIResourceType type)
    {
        switch (type) {
            case ERHIResourceType::ShaderResourceView:
            case ERHIResourceType::UnorderedAccessView:
            case ERHIResourceType::RenderTargetView:
            case ERHIResourceType::DepthStencilView:
            case ERHIResourceType::ConstantBufferView:
                return true;
            default:
                return false;
        }
    }

    /// パイプラインステートか
    inline bool IsPipelineState(ERHIResourceType type)
    {
        switch (type) {
            case ERHIResourceType::GraphicsPipelineState:
            case ERHIResourceType::ComputePipelineState:
            case ERHIResourceType::RayTracingPSO:
                return true;
            default:
                return false;
        }
    }

    /// コマンド関連か
    inline bool IsCommandResource(ERHIResourceType type)
    {
        return type == ERHIResourceType::CommandList
            || type == ERHIResourceType::CommandAllocator;
    }
}
```

- [ ] タイプの類関数

### 3. 型安全キャストのクロ

```cpp
namespace NS::RHI
{
    /// 静的リソースタイプ取得用マクロ
    /// 合リソースクラスで使用:
    ///   DECLARE_RHI_RESOURCE_TYPE(Buffer)
    ///   ↔ static ERHIResourceType StaticResourceType() { return ERHIResourceType::Buffer; }
    #define DECLARE_RHI_RESOURCE_TYPE(TypeName) \
        static ERHIResourceType StaticResourceType() { \
            return ERHIResourceType::TypeName; \
        } \
        ERHIResourceType GetResourceType() const override { \
            return ERHIResourceType::TypeName; \
        }
}
```

- [ ] DECLARE_RHI_RESOURCE_TYPE マクロ

### 4. RHICast: 型チェック付きキャスト

```cpp
namespace NS::RHI
{
    /// 型チェック付きダウンキャスト
    /// @tparam T 変換先の型（RHIBuffer等）
    /// @param resource 変換元リソース
    /// @return 成功時のキャスト結果、失敗時はnullptr
    template<typename T>
    T* RHICast(IRHIResource* resource)
    {
        if (resource && resource->GetResourceType() == T::StaticResourceType()) {
            return static_cast<T*>(resource);
        }
        return nullptr;
    }

    /// const牁
    template<typename T>
    const T* RHICast(const IRHIResource* resource)
    {
        if (resource && resource->GetResourceType() == T::StaticResourceType()) {
            return static_cast<const T*>(resource);
        }
        return nullptr;
    }

    /// TRefCountPtr牁
    template<typename T>
    TRefCountPtr<T> RHICast(const TRefCountPtr<IRHIResource>& resource)
    {
        if (resource && resource->GetResourceType() == T::StaticResourceType()) {
            return TRefCountPtr<T>(static_cast<T*>(resource.Get()));
        }
        return nullptr;
    }
}
```

- [ ] RHICast テンプレート関数

### 5. IsA: 型チェック

```cpp
namespace NS::RHI
{
    /// 型チェックキャストなし）
    /// @tparam T チェックする型
    /// @param resource チェック対象
    /// @return Tにキャスト可能ならtrue
    template<typename T>
    bool IsA(const IRHIResource* resource)
    {
        return resource && resource->GetResourceType() == T::StaticResourceType();
    }

    /// TRefCountPtr牁
    template<typename T, typename U>
    bool IsA(const TRefCountPtr<U>& resource)
    {
        return resource && resource->GetResourceType() == T::StaticResourceType();
    }

    /// 指定タイプか確認
    inline bool IsResourceType(const IRHIResource* resource, ERHIResourceType type)
    {
        return resource && resource->GetResourceType() == type;
    }
}

// 使用例
// if (auto* buffer = RHICast<IRHIBuffer>(resource)) { ... }
// if (IsA<IRHITexture>(resource)) { ... }
```

- [ ] IsA テンプレート関数
- [ ] IsResourceType 関数

## 検証方法

- [ ] 全リソースタイプの網羅性
- [ ] RHICast の正確性
- [ ] 不正キャストの検出

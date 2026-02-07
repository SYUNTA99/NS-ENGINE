# 16-01: リソース状態

## 目的

リソースの状態定義と遷移管理を定義する。

## 参照ドキュメント

- 01-07-enums-access.md (アクセスフラグ)
- 01-12-resource-base.md (IRHIResource)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIResourceState.h`

## TODO

### 1. リソース状態列挙型

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// リソース状態
    /// D3D12のD3D12_RESOURCE_STATESに対応
    enum class ERHIResourceState : uint32
    {
        /// 共通状態（任意のアクセス可能）。
        Common = 0,

        /// 頂点バッファとして使用
        VertexBuffer = 1 << 0,

        /// 定数バッファとして使用
        ConstantBuffer = 1 << 1,

        /// インデックスバッファとして使用
        IndexBuffer = 1 << 2,

        /// レンダーターゲットとして使用
        RenderTarget = 1 << 3,

        /// アンオーダードアクセス（読み書き）
        UnorderedAccess = 1 << 4,

        /// デプスステンシル書き込み
        DepthWrite = 1 << 5,

        /// デプスステンシル読み取り専用
        DepthRead = 1 << 6,

        /// シェーダーリソース（非ピクセルシェーダー）。
        NonPixelShaderResource = 1 << 7,

        /// ピクセルシェーダーリソース
        PixelShaderResource = 1 << 8,

        /// ストリームアウト
        StreamOut = 1 << 9,

        /// 間接引数
        IndirectArgument = 1 << 10,

        /// コピー先
        CopyDest = 1 << 11,

        /// コピー元
        CopySource = 1 << 12,

        /// リゾルブ。
        ResolveDest = 1 << 13,

        /// リゾルブ。
        ResolveSource = 1 << 14,

        /// レイトレーシング加速構造
        RaytracingAccelerationStructure = 1 << 15,

        /// シェーディングレートソース
        ShadingRateSource = 1 << 16,

        /// プレゼント
        Present = 1 << 17,

        /// プレディケーション
        Predication = 1 << 18,

        /// ビデオデコード読み取り
        VideoDecodeRead = 1 << 19,

        /// ビデオデコード書き込み
        VideoDecodeWrite = 1 << 20,

        /// ビデオプロセス読み取り
        VideoProcessRead = 1 << 21,

        /// ビデオプロセス書き込み
        VideoProcessWrite = 1 << 22,

        /// ビデオエンコード読み取り
        VideoEncodeRead = 1 << 23,

        /// ビデオエンコード書き込み
        VideoEncodeWrite = 1 << 24,

        //=====================================================================
        // 複数状態
        //=====================================================================

        /// 汎用読み取り（最適化ヒント）
        GenericRead = VertexBuffer | ConstantBuffer | IndexBuffer |
                      NonPixelShaderResource | PixelShaderResource |
                      IndirectArgument | CopySource,

        /// シェーダーリソース（全ステージ）。
        ShaderResource = NonPixelShaderResource | PixelShaderResource,

        /// すべてのシェーダー書き込み
        AllShaderWrite = UnorderedAccess | DepthWrite | RenderTarget,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIResourceState)
}
```

- [ ] ERHIResourceState 列挙型

### 2. 状態遷移ヘルパー

```cpp
namespace NS::RHI
{
    /// リソース状態ヘルパー。
    namespace RHIResourceStateHelper
    {
        /// 読み取り状態か
        inline bool IsReadState(ERHIResourceState state)
        {
            constexpr ERHIResourceState kReadStates =
                ERHIResourceState::VertexBuffer |
                ERHIResourceState::ConstantBuffer |
                ERHIResourceState::IndexBuffer |
                ERHIResourceState::NonPixelShaderResource |
                ERHIResourceState::PixelShaderResource |
                ERHIResourceState::IndirectArgument |
                ERHIResourceState::CopySource |
                ERHIResourceState::ResolveSource |
                ERHIResourceState::DepthRead |
                ERHIResourceState::Predication;

            return (state & kReadStates) != ERHIResourceState::Common;
        }

        /// 書き込み状態か
        inline bool IsWriteState(ERHIResourceState state)
        {
            constexpr ERHIResourceState kWriteStates =
                ERHIResourceState::RenderTarget |
                ERHIResourceState::UnorderedAccess |
                ERHIResourceState::DepthWrite |
                ERHIResourceState::StreamOut |
                ERHIResourceState::CopyDest |
                ERHIResourceState::ResolveDest;

            return (state & kWriteStates) != ERHIResourceState::Common;
        }

        /// 遷移が必要か
        inline bool NeedsTransition(ERHIResourceState before, ERHIResourceState after)
        {
            // 同一状態なら不要。
            if (before == after) return false;

            // 読み取りから読み取りへの遷移は一部不要。
            if (IsReadState(before) && IsReadState(after)) {
                // 両方が GenericRead に含まれるなら不要。
                if ((before & ERHIResourceState::GenericRead) == before &&
                    (after & ERHIResourceState::GenericRead) == after) {
                    return false;
                }
            }

            return true;
        }

        /// 同時アクセス可能か
        inline bool CanCoexist(ERHIResourceState a, ERHIResourceState b)
        {
            // どちらかが書き込み状態なら共存不可
            if (IsWriteState(a) || IsWriteState(b)) return false;

            // 両方読み取りなら共存可能
            return true;
        }

        /// 状態名取得
        RHI_API const char* GetStateName(ERHIResourceState state);
    }
}
```

- [ ] RHIResourceStateHelper

### 3. サブリソース状態

```cpp
namespace NS::RHI
{
    /// 全サブリソース定数
    constexpr uint32 kAllSubresources = ~0u;

    /// サブリソース状態
    struct RHI_API RHISubresourceState
    {
        /// リソース状態
        ERHIResourceState state = ERHIResourceState::Common;

        /// サブリソースインデックス（AllSubresourcesで全て）。
        uint32 subresource = kAllSubresources;
    };

    /// リソースの完全な状態追跡
    class RHI_API RHIResourceStateMap
    {
    public:
        RHIResourceStateMap() = default;

        /// 初期化（のサブリソース同一状態）
        void Initialize(uint32 subresourceCount, ERHIResourceState initialState);

        /// リセット
        void Reset();

        //=====================================================================
        // 状態取得
        //=====================================================================

        /// 全サブリソースが同一状態か
        bool IsUniform() const { return m_isUniform; }

        /// 統一状態取得（IsUniform時のみ有効）。
        ERHIResourceState GetUniformState() const { return m_uniformState; }

        /// サブリソース状態取得
        ERHIResourceState GetSubresourceState(uint32 subresource) const;

        //=====================================================================
        // 状態設定
        //=====================================================================

        /// 全サブリソースの状態設定
        void SetAllSubresourcesState(ERHIResourceState state);

        /// 個別サブリソースの状態設定
        void SetSubresourceState(uint32 subresource, ERHIResourceState state);

        /// 範囲のサブリソース状態設定
        void SetSubresourceRangeState(
            uint32 firstSubresource,
            uint32 count,
            ERHIResourceState state);

        //=====================================================================
        // 情報
        //=====================================================================

        /// サブリソース数
        uint32 GetSubresourceCount() const { return m_subresourceCount; }

    private:
        ERHIResourceState* m_states = nullptr;
        uint32 m_subresourceCount = 0;
        ERHIResourceState m_uniformState = ERHIResourceState::Common;
        bool m_isUniform = true;
    };
}
```

- [ ] RHISubresourceState 構造体
- [ ] RHIResourceStateMap クラス

### 4. リソース初期状態

```cpp
namespace NS::RHI
{
    /// リソース初期状態ヘルパー。
    namespace RHIInitialResourceState
    {
        /// バッファの初期状態
        inline ERHIResourceState ForBuffer(ERHIBufferUsage usage)
        {
            if (HasFlag(usage, ERHIBufferUsage::Upload)) {
                return ERHIResourceState::GenericRead;
            }
            if (HasFlag(usage, ERHIBufferUsage::Readback)) {
                return ERHIResourceState::CopyDest;
            }
            return ERHIResourceState::Common;
        }

        /// テクスチャの初期状態
        inline ERHIResourceState ForTexture(ERHITextureUsage usage)
        {
            if (HasFlag(usage, ERHITextureUsage::RenderTarget)) {
                return ERHIResourceState::RenderTarget;
            }
            if (HasFlag(usage, ERHITextureUsage::DepthStencil)) {
                return ERHIResourceState::DepthWrite;
            }
            if (HasFlag(usage, ERHITextureUsage::UnorderedAccess)) {
                return ERHIResourceState::UnorderedAccess;
            }
            return ERHIResourceState::Common;
        }

        /// スワップチェーンバックバッファの初期状態
        inline ERHIResourceState ForBackBuffer()
        {
            return ERHIResourceState::Present;
        }
    }
}
```

- [ ] RHIInitialResourceState ヘルパー

## 検証方法

- [ ] 状態フラグの網羅性
- [ ] 遷移判定の正確性
- [ ] サブリソース状態追跡
- [ ] 初期状態の正確性

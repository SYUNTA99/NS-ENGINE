# 14-01: クエリタイプ

## 目的

GPUクエリのタイプと記述を定義する。

## 参照ドキュメント

- 01-07-enums-access.md (アクセス関連列挙型

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIQuery.h`

## TODO

### 1. クエリタイプ列挙型

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// クエリタイプ
    enum class ERHIQueryType : uint8
    {
        /// オクルージョン
        /// 可視ピクセル数をカウント
        Occlusion,

        /// バイナリオクルージョン
        /// 1ピクセルでも可視ならtrue
        BinaryOcclusion,

        /// タイムスタンプ
        /// GPUタイムスタンプ値
        Timestamp,

        /// パイプライン統計
        /// 全パイプラインステージの統計
        PipelineStatistics,

        /// ストリームアウト統計
        /// ストリームアウトの統計
        StreamOutputStatistics,

        /// ストリームアウトオーバーフロー
        /// ストリームアウトバッファオーバーフロー検出
        StreamOutputOverflow,

        /// プレディケーション
        /// 条件付きレンダリング用
        Predication,
    };
}
```

- [ ] ERHIQueryType 列挙型

### 2. パイプライン統計フラグ

```cpp
namespace NS::RHI
{
    /// パイプライン統計フラグ
    enum class ERHIPipelineStatisticsFlags : uint32
    {
        None = 0,

        /// 入力アセンブラ頂点数
        IAVertices = 1 << 0,

        /// 入力アセンブラプリミティブ数
        IAPrimitives = 1 << 1,

        /// 頂点シェーダー呼び出し数
        VSInvocations = 1 << 2,

        /// ジオメトリシェーダー呼び出し数
        GSInvocations = 1 << 3,

        /// ジオメトリシェーダープリミティブ数
        GSPrimitives = 1 << 4,

        /// ラスタライザ呼び出し数
        CInvocations = 1 << 5,

        /// ラスタライザプリミティブ数
        CPrimitives = 1 << 6,

        /// ピクセルシェーダー呼び出し数
        PSInvocations = 1 << 7,

        /// ハルシェーダー呼び出し数
        HSInvocations = 1 << 8,

        /// ドメインシェーダー呼び出し数
        DSInvocations = 1 << 9,

        /// コンピュートシェーダー呼び出し数
        CSInvocations = 1 << 10,

        /// アンプリフィケーションシェーダー呼び出し数
        ASInvocations = 1 << 11,

        /// メッシュシェーダー呼び出し数
        MSInvocations = 1 << 12,

        /// 全統計
        All = 0x1FFF,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIPipelineStatisticsFlags)
}
```

- [ ] ERHIPipelineStatisticsFlags 列挙型

### 3. クエリ記述

```cpp
namespace NS::RHI
{
    /// クエリヒープ記述
    struct RHI_API RHIQueryHeapDesc
    {
        /// クエリタイプ
        ERHIQueryType type = ERHIQueryType::Timestamp;

        /// クエリ数
        uint32 count = 0;

        /// パイプライン統計フラグ（PipelineStatistics時のみ）。
        ERHIPipelineStatisticsFlags pipelineStatisticsFlags = ERHIPipelineStatisticsFlags::None;

        /// ノードマスク（マルチGPU用）。
        uint32 nodeMask = 0;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// タイムスタンプクエリヒープ
        static RHIQueryHeapDesc Timestamp(uint32 count) {
            RHIQueryHeapDesc desc;
            desc.type = ERHIQueryType::Timestamp;
            desc.count = count;
            return desc;
        }

        /// オクルージョンクエリヒープ
        static RHIQueryHeapDesc Occlusion(uint32 count) {
            RHIQueryHeapDesc desc;
            desc.type = ERHIQueryType::Occlusion;
            desc.count = count;
            return desc;
        }

        /// バイナリオクルージョンクエリヒープ
        static RHIQueryHeapDesc BinaryOcclusion(uint32 count) {
            RHIQueryHeapDesc desc;
            desc.type = ERHIQueryType::BinaryOcclusion;
            desc.count = count;
            return desc;
        }

        /// パイプライン統計クエリヒープ
        static RHIQueryHeapDesc PipelineStatistics(
            uint32 count,
            ERHIPipelineStatisticsFlags flags = ERHIPipelineStatisticsFlags::All)
        {
            RHIQueryHeapDesc desc;
            desc.type = ERHIQueryType::PipelineStatistics;
            desc.count = count;
            desc.pipelineStatisticsFlags = flags;
            return desc;
        }
    };
}
```

- [ ] RHIQueryHeapDesc 構造体

### 4. クエリ結果構造体

```cpp
namespace NS::RHI
{
    /// オクルージョンクエリ結果
    struct RHI_API RHIOcclusionQueryResult
    {
        /// 可視サンプル数
        uint64 visibleSamples = 0;

        /// バイナリ結果
        bool IsVisible() const { return visibleSamples > 0; }
    };

    /// パイプライン統計クエリ結果
    struct RHI_API RHIPipelineStatisticsResult
    {
        uint64 iaVertices = 0;
        uint64 iaPrimitives = 0;
        uint64 vsInvocations = 0;
        uint64 gsInvocations = 0;
        uint64 gsPrimitives = 0;
        uint64 cInvocations = 0;
        uint64 cPrimitives = 0;
        uint64 psInvocations = 0;
        uint64 hsInvocations = 0;
        uint64 dsInvocations = 0;
        uint64 csInvocations = 0;
    };

    /// ストリームアウト統計クエリ結果
    struct RHI_API RHIStreamOutputStatisticsResult
    {
        /// 書き込まれたプリミティブ数
        uint64 primitivesWritten = 0;

        /// 必要なプリミティブ数
        uint64 primitivesStorageNeeded = 0;

        /// オーバーフローしたか
        bool HasOverflow() const {
            return primitivesStorageNeeded > primitivesWritten;
        }
    };
}
```

- [ ] RHIOcclusionQueryResult 構造体
- [ ] RHIPipelineStatisticsResult 構造体
- [ ] RHIStreamOutputStatisticsResult 構造体

### 5. クエリフラグ

```cpp
namespace NS::RHI
{
    /// クエリフラグ
    enum class ERHIQueryFlags : uint32
    {
        None = 0,

        /// 結果が64bit
        Result64Bit = 1 << 0,

        /// 利用可能フラグを含む
        WithAvailability = 1 << 1,

        /// 結果待ち
        Wait = 1 << 2,

        /// 部分的な結果を許可
        PartialResults = 1 << 3,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIQueryFlags)
}
```

- [ ] ERHIQueryFlags 列挙型

## 検証方法

- [ ] クエリタイプの網羅性
- [ ] 統計フラグの組み合わせ
- [ ] 結果構造体のサイズ

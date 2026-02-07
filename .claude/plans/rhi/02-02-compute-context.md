# 02-02: コンピュートコンテキスト

## 目的

コンピュートシェーダー実行用のコンテキストインターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドコンテキスト

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIComputeContext.h`

## TODO

### 1. IRHIComputeContext: 基本定義

```cpp
#pragma once

#include "IRHICommandContextBase.h"

namespace NS::RHI
{
    /// コンピュートコンテキスト
    /// コンピュートシェーダー実行専用のコンテキスト
    class RHI_API IRHIComputeContext : public IRHICommandContextBase
    {
    public:
        virtual ~IRHIComputeContext() = default;

        //=====================================================================
        // パイプラインステート
        //=====================================================================

        /// コンピュートパイプラインステート設定
        virtual void SetComputePipelineState(
            IRHIComputePipelineState* pso) = 0;

        /// ルートシグネチャ設定
        virtual void SetComputeRootSignature(
            IRHIRootSignature* rootSignature) = 0;
    };
}
```

- [ ] 基本インターフェース定義
- [ ] パイプラインステート設定

### 2. リソースバインディング

```cpp
namespace NS::RHI
{
    class IRHIComputeContext
    {
    public:
        //=====================================================================
        // 定数バッファ
        //=====================================================================

        /// ルート定数設定（2bit値）。
        virtual void SetComputeRoot32BitConstants(
            uint32 rootParameterIndex,
            uint32 num32BitValues,
            const void* data,
            uint32 destOffset = 0) = 0;

        /// ルートCBV設定（GPUアドレス）。
        virtual void SetComputeRootConstantBufferView(
            uint32 rootParameterIndex,
            uint64 bufferAddress) = 0;

        //=====================================================================
        // SRV / UAV
        //=====================================================================

        /// ルートSRV設定（GPUアドレス）。
        virtual void SetComputeRootShaderResourceView(
            uint32 rootParameterIndex,
            uint64 bufferAddress) = 0;

        /// ルートUAV設定（GPUアドレス）。
        virtual void SetComputeRootUnorderedAccessView(
            uint32 rootParameterIndex,
            uint64 bufferAddress) = 0;

        //=====================================================================
        // ディスクリプタテーブル
        //=====================================================================

        /// ディスクリプタテーブル設定
        virtual void SetComputeRootDescriptorTable(
            uint32 rootParameterIndex,
            RHIGPUDescriptorHandle baseDescriptor) = 0;
    };
}
```

- [ ] ルート定数設定
- [ ] CBV/SRV/UAV設定
- [ ] ディスクリプタテーブル

### 3. ディスパッチ

```cpp
namespace NS::RHI
{
    class IRHIComputeContext
    {
    public:
        //=====================================================================
        // ディスパッチ
        //=====================================================================

        /// コンピュートシェーダーディスパッチ
        /// @param threadGroupCountX X方向スレッドグループ数
        /// @param threadGroupCountY Y方向スレッドグループ数
        /// @param threadGroupCountZ Z方向スレッドグループ数
        virtual void Dispatch(
            uint32 threadGroupCountX,
            uint32 threadGroupCountY,
            uint32 threadGroupCountZ) = 0;

        /// 間接ディスパッチ
        /// @param argsBuffer 引数バッファ
        /// @param argsOffset バッファ内のオフセット
        virtual void DispatchIndirect(
            IRHIBuffer* argsBuffer,
            uint64 argsOffset = 0) = 0;
    };
}
```

- [ ] Dispatch
- [ ] DispatchIndirect

### 4. UAVクリア

```cpp
namespace NS::RHI
{
    class IRHIComputeContext
    {
    public:
        //=====================================================================
        // UAVクリア
        //=====================================================================

        /// UAVをuint値でクリア
        /// @param uav クリア対象UAV
        /// @param values 4つのuint値
        virtual void ClearUnorderedAccessViewUint(
            IRHIUnorderedAccessView* uav,
            const uint32 values[4]) = 0;

        /// UAVをfloat値でクリア
        /// @param uav クリア対象UAV
        /// @param values 4つのfloat値
        virtual void ClearUnorderedAccessViewFloat(
            IRHIUnorderedAccessView* uav,
            const float values[4]) = 0;
    };
}
```

- [ ] ClearUnorderedAccessViewUint
- [ ] ClearUnorderedAccessViewFloat

### 5. タイムスタンプクエリ

```cpp
namespace NS::RHI
{
    class IRHIComputeContext
    {
    public:
        //=====================================================================
        // タイムスタンプ
        //=====================================================================

        /// タイムスタンプ書き込み
        /// @param queryHeap クエリプール
        /// @param queryIndex クエリインデックス
        virtual void WriteTimestamp(
            IRHIQueryHeap* queryHeap,
            uint32 queryIndex) = 0;

        //=====================================================================
        // パイプライン統計
        //=====================================================================

        /// クエリ開始
        virtual void BeginQuery(
            IRHIQueryHeap* queryHeap,
            uint32 queryIndex) = 0;

        /// クエリ終了
        virtual void EndQuery(
            IRHIQueryHeap* queryHeap,
            uint32 queryIndex) = 0;

        /// クエリ結果をバッファに書き込み
        virtual void ResolveQueryData(
            IRHIQueryHeap* queryHeap,
            uint32 startIndex,
            uint32 numQueries,
            IRHIBuffer* destinationBuffer,
            uint64 destinationOffset) = 0;
    };
}
```

- [ ] WriteTimestamp
- [ ] BeginQuery / EndQuery
- [ ] ResolveQueryData

## 検証方法

- [ ] コンピュート操作の網羅性
- [ ] リソースバインディングの正確性

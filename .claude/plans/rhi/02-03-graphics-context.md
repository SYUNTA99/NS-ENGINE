# 02-03: グラフィックスコンテキスト

## 目的

グラフィックス描画用のコマンドコンテキストインターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドコンテキスト)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHICommandContext.h`

## TODO

### 1. IRHICommandContext: 基本定義

```cpp
#pragma once

#include "IRHIComputeContext.h"

namespace NS::RHI
{
    /// グラフィックスコマンドコンテキスト
    /// グラフィックス + コンピュート機能を持つフルコンテキスト
    class RHI_API IRHICommandContext : public IRHIComputeContext
    {
    public:
        virtual ~IRHICommandContext() = default;

        //=====================================================================
        // グラフィックスパイプライン
        //=====================================================================

        /// グラフィックスパイプラインステート設定
        virtual void SetGraphicsPipelineState(
            IRHIGraphicsPipelineState* pso) = 0;

        /// グラフィックス用ルートシグネチャ設定
        virtual void SetGraphicsRootSignature(
            IRHIRootSignature* rootSignature) = 0;
    };
}
```

- [ ] 基本インターフェース定義
- [ ] グラフィックスPSO設定

### 2. レンダーターゲット設定

```cpp
namespace NS::RHI
{
    class IRHICommandContext
    {
    public:
        //=====================================================================
        // レンダーターゲット
        //=====================================================================

        /// レンダーターゲット設定
        /// @param numRTVs レンダーターゲット数
        /// @param rtvs レンダーターゲットビュー配列
        /// @param dsv デプスステンシルビュー（nullptrで無効）
        virtual void SetRenderTargets(
            uint32 numRTVs,
            IRHIRenderTargetView* const* rtvs,
            IRHIDepthStencilView* dsv) = 0;

        /// 単一レンダーターゲット設定（便利関数）
        void SetRenderTarget(IRHIRenderTargetView* rtv,
                             IRHIDepthStencilView* dsv = nullptr)
        {
            SetRenderTargets(rtv ? 1 : 0, rtv ? &rtv : nullptr, dsv);
        }

        //=====================================================================
        // クリア
        //=====================================================================

        /// レンダーターゲットクリア
        virtual void ClearRenderTargetView(
            IRHIRenderTargetView* rtv,
            const float color[4]) = 0;

        /// デプスステンシルクリア
        virtual void ClearDepthStencilView(
            IRHIDepthStencilView* dsv,
            bool clearDepth, float depth,
            bool clearStencil, uint8 stencil) = 0;
    };
}
```

- [ ] SetRenderTargets
- [ ] ClearRenderTargetView / ClearDepthStencilView

### 3. ビューポート/シザー

```cpp
namespace NS::RHI
{
    class IRHICommandContext
    {
    public:
        //=====================================================================
        // ビューポート
        //=====================================================================

        /// ビューポート設定
        virtual void SetViewports(
            uint32 numViewports,
            const RHIViewport* viewports) = 0;

        /// 単一ビューポート設定
        void SetViewport(const RHIViewport& viewport) {
            SetViewports(1, &viewport);
        }

        //=====================================================================
        // シザー矩形
        //=====================================================================

        /// シザー矩形設定
        virtual void SetScissorRects(
            uint32 numRects,
            const RHIRect* rects) = 0;

        /// 単一シザー矩形設定
        void SetScissorRect(const RHIRect& rect) {
            SetScissorRects(1, &rect);
        }
    };
}
```

- [ ] SetViewports
- [ ] SetScissorRects

### 4. 頂点/インデックスバッファ

```cpp
namespace NS::RHI
{
    /// 頂点バッファビュー
    struct RHIVertexBufferView
    {
        uint64 bufferAddress = 0;   // GPUアドレス
        uint32 size = 0;            // バッファサイズ（バイト）
        uint32 stride = 0;          // 頂点ストライド（バイト）
    };

    /// インデックスバッファビュー
    struct RHIIndexBufferView
    {
        uint64 bufferAddress = 0;   // GPUアドレス
        uint32 size = 0;            // バッファサイズ（バイト）
        ERHIIndexFormat format = ERHIIndexFormat::UInt16;
    };

    class IRHICommandContext
    {
    public:
        //=====================================================================
        // 頂点バッファ
        //=====================================================================

        /// 頂点バッファ設定
        virtual void SetVertexBuffers(
            uint32 startSlot,
            uint32 numBuffers,
            const RHIVertexBufferView* views) = 0;

        /// 単一頂点バッファ設定
        void SetVertexBuffer(uint32 slot, const RHIVertexBufferView& view) {
            SetVertexBuffers(slot, 1, &view);
        }

        //=====================================================================
        // インデックスバッファ
        //=====================================================================

        /// インデックスバッファ設定
        virtual void SetIndexBuffer(const RHIIndexBufferView& view) = 0;

        //=====================================================================
        // プリミティブトポロジー
        //=====================================================================

        /// プリミティブトポロジー設定
        virtual void SetPrimitiveTopology(ERHIPrimitiveTopology topology) = 0;
    };
}
```

- [ ] RHIVertexBufferView / RHIIndexBufferView
- [ ] SetVertexBuffers / SetIndexBuffer
- [ ] SetPrimitiveTopology

### 5. 描画コマンド

```cpp
namespace NS::RHI
{
    class IRHICommandContext
    {
    public:
        //=====================================================================
        // 描画
        //=====================================================================

        /// 描画
        virtual void Draw(
            uint32 vertexCount,
            uint32 instanceCount = 1,
            uint32 startVertex = 0,
            uint32 startInstance = 0) = 0;

        /// インデックス付き描画
        virtual void DrawIndexed(
            uint32 indexCount,
            uint32 instanceCount = 1,
            uint32 startIndex = 0,
            int32 baseVertex = 0,
            uint32 startInstance = 0) = 0;

        //=====================================================================
        // 間接描画
        //=====================================================================

        /// 間接描画
        virtual void DrawIndirect(
            IRHIBuffer* argsBuffer,
            uint64 argsOffset = 0) = 0;

        /// 間接インデックス付き描画
        virtual void DrawIndexedIndirect(
            IRHIBuffer* argsBuffer,
            uint64 argsOffset = 0) = 0;

        /// 複数間接描画
        virtual void MultiDrawIndirect(
            IRHIBuffer* argsBuffer,
            uint32 drawCount,
            uint64 argsOffset = 0,
            uint32 argsStride = 0) = 0;

        /// カウント付き複数間接描画
        virtual void MultiDrawIndirectCount(
            IRHIBuffer* argsBuffer,
            uint64 argsOffset,
            IRHIBuffer* countBuffer,
            uint64 countOffset,
            uint32 maxDrawCount,
            uint32 argsStride) = 0;
    };
}
```

- [ ] Draw / DrawIndexed
- [ ] 間接描画
- [ ] 複数間接描画

## 検証方法

- [ ] 描画コマンドの網羅性
- [ ] 状態設定の整合性

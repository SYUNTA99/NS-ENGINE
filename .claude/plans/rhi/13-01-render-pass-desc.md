# 13-01: レンダーパス記述

## 目的

レンダーパスの記述構造を定義する。

## 参照ドキュメント

- 05-03-rtv.md (IRHIRenderTargetView)
- 05-04-dsv.md (IRHIDepthStencilView)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIRenderPass.h`

## TODO

### 1. アタッチメント記述

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// レンダーターゲットアタッチメント記述
    struct RHI_API RHIRenderTargetAttachment
    {
        /// レンダーターゲットビュー
        IRHIRenderTargetView* rtv = nullptr;

        /// リゾルブターゲット（MSAA時）
        IRHIRenderTargetView* resolveTarget = nullptr;

        /// ロードアクション
        ERHILoadAction loadAction = ERHILoadAction::Load;

        /// ストアアクション
        ERHIStoreAction storeAction = ERHIStoreAction::Store;

        /// クリア値（LoadAction == Clearのとき）
        RHIClearValue clearValue = {};

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ロードして保存
        static RHIRenderTargetAttachment LoadStore(IRHIRenderTargetView* view) {
            RHIRenderTargetAttachment att;
            att.rtv = view;
            att.loadAction = ERHILoadAction::Load;
            att.storeAction = ERHIStoreAction::Store;
            return att;
        }

        /// クリアして保存
        static RHIRenderTargetAttachment ClearStore(
            IRHIRenderTargetView* view,
            float r = 0, float g = 0, float b = 0, float a = 1)
        {
            RHIRenderTargetAttachment att;
            att.rtv = view;
            att.loadAction = ERHILoadAction::Clear;
            att.storeAction = ERHIStoreAction::Store;
            att.clearValue.color[0] = r;
            att.clearValue.color[1] = g;
            att.clearValue.color[2] = b;
            att.clearValue.color[3] = a;
            return att;
        }

        /// ドントケアで保存
        static RHIRenderTargetAttachment DontCareStore(IRHIRenderTargetView* view) {
            RHIRenderTargetAttachment att;
            att.rtv = view;
            att.loadAction = ERHILoadAction::DontCare;
            att.storeAction = ERHIStoreAction::Store;
            return att;
        }

        /// クリアしてリゾルブ（MSAA）。
        static RHIRenderTargetAttachment ClearResolve(
            IRHIRenderTargetView* msaaView,
            IRHIRenderTargetView* resolveView,
            float r = 0, float g = 0, float b = 0, float a = 1)
        {
            RHIRenderTargetAttachment att;
            att.rtv = msaaView;
            att.resolveTarget = resolveView;
            att.loadAction = ERHILoadAction::Clear;
            att.storeAction = ERHIStoreAction::Resolve;
            att.clearValue.color[0] = r;
            att.clearValue.color[1] = g;
            att.clearValue.color[2] = b;
            att.clearValue.color[3] = a;
            return att;
        }
    };
}
```

- [ ] RHIRenderTargetAttachment 構造体
- [ ] ビルダーメソッド

### 2. デプスステンシルアタッチメント

```cpp
namespace NS::RHI
{
    /// デプスステンシルアタッチメント記述
    struct RHI_API RHIDepthStencilAttachment
    {
        /// デプスステンシルビュー
        IRHIDepthStencilView* dsv = nullptr;

        /// デプスロードアクション
        ERHILoadAction depthLoadAction = ERHILoadAction::Load;

        /// デプスストアアクション
        ERHIStoreAction depthStoreAction = ERHIStoreAction::Store;

        /// ステンシルロードアクション
        ERHILoadAction stencilLoadAction = ERHILoadAction::Load;

        /// ステンシルストアアクション
        ERHIStoreAction stencilStoreAction = ERHIStoreAction::Store;

        /// デプスクリア値
        float clearDepth = 1.0f;

        /// ステンシルクリア値
        uint8 clearStencil = 0;

        /// デプス書き込み有効
        bool depthWriteEnabled = true;

        /// ステンシル書き込み有効
        bool stencilWriteEnabled = true;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ロードして保存
        static RHIDepthStencilAttachment LoadStore(IRHIDepthStencilView* view) {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            return att;
        }

        /// クリアして保存
        static RHIDepthStencilAttachment ClearStore(
            IRHIDepthStencilView* view,
            float depth = 1.0f,
            uint8 stencil = 0)
        {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            att.depthLoadAction = ERHILoadAction::Clear;
            att.stencilLoadAction = ERHILoadAction::Clear;
            att.clearDepth = depth;
            att.clearStencil = stencil;
            return att;
        }

        /// 読み取り専用
        static RHIDepthStencilAttachment ReadOnly(IRHIDepthStencilView* view) {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            att.depthWriteEnabled = false;
            att.stencilWriteEnabled = false;
            att.depthStoreAction = ERHIStoreAction::DontCare;
            att.stencilStoreAction = ERHIStoreAction::DontCare;
            return att;
        }

        /// デプスのみクリア
        static RHIDepthStencilAttachment ClearDepthOnly(
            IRHIDepthStencilView* view,
            float depth = 1.0f)
        {
            RHIDepthStencilAttachment att;
            att.dsv = view;
            att.depthLoadAction = ERHILoadAction::Clear;
            att.stencilLoadAction = ERHILoadAction::DontCare;
            att.stencilStoreAction = ERHIStoreAction::DontCare;
            att.clearDepth = depth;
            return att;
        }
    };
}
```

- [ ] RHIDepthStencilAttachment 構造体
- [ ] ビルダーメソッド

### 3. レンダーパス記述

```cpp
namespace NS::RHI
{
    /// 最大レンダーターゲット数
    constexpr uint32 kMaxRenderTargets = 8;

    /// レンダーパス記述
    struct RHI_API RHIRenderPassDesc
    {
        /// レンダーターゲットアタッチメント
        RHIRenderTargetAttachment renderTargets[kMaxRenderTargets];

        /// レンダーターゲット数
        uint32 renderTargetCount = 0;

        /// デプスステンシルアタッチメント
        RHIDepthStencilAttachment depthStencil;

        /// デプスステンシル有効
        bool hasDepthStencil = false;

        /// レンダー領域
        uint32 renderAreaX = 0;
        uint32 renderAreaY = 0;
        uint32 renderAreaWidth = 0;
        uint32 renderAreaHeight = 0;

        /// シェーディングレートイメージ（VRS用）。
        IRHITexture* shadingRateImage = nullptr;

        /// フラグ
        enum class Flags : uint32 {
            None = 0,
            /// UAVを継続して使用
            SuspendingPass = 1 << 0,
            /// 前のパスからUAVを継続
            ResumingPass = 1 << 1,
            /// タイルベースレンダリングヒント
            TileBasedRenderingHint = 1 << 2,
        };
        Flags flags = Flags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// レンダーターゲット追加
        RHIRenderPassDesc& AddRenderTarget(const RHIRenderTargetAttachment& rt) {
            if (renderTargetCount < kMaxRenderTargets) {
                renderTargets[renderTargetCount++] = rt;
            }
            return *this;
        }

        /// デプスステンシル設定
        RHIRenderPassDesc& SetDepthStencil(const RHIDepthStencilAttachment& ds) {
            depthStencil = ds;
            hasDepthStencil = true;
            return *this;
        }

        /// レンダー領域設定
        RHIRenderPassDesc& SetRenderArea(uint32 x, uint32 y, uint32 w, uint32 h) {
            renderAreaX = x;
            renderAreaY = y;
            renderAreaWidth = w;
            renderAreaHeight = h;
            return *this;
        }

        /// フルスクリーンパス（RTサイズ全体）
        RHIRenderPassDesc& SetFullRenderArea() {
            renderAreaX = 0;
            renderAreaY = 0;
            renderAreaWidth = 0;  // 0 = 自動
            renderAreaHeight = 0;
            return *this;
        }
    };
    RHI_ENUM_CLASS_FLAGS(RHIRenderPassDesc::Flags)
}
```

- [ ] RHIRenderPassDesc 構造体
- [ ] ビルダーメソッド

### 4. 一般的なレンダーパスプリセット

```cpp
namespace NS::RHI
{
    /// レンダーパスプリセット
    namespace RHIRenderPassPresets
    {
        /// 単一RTクリア
        inline RHIRenderPassDesc SingleRTClear(
            IRHIRenderTargetView* rtv,
            float r = 0, float g = 0, float b = 0, float a = 1)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(rtv, r, g, b, a));
            return desc;
        }

        /// 単一RT + デプスクリア
        inline RHIRenderPassDesc SingleRTWithDepthClear(
            IRHIRenderTargetView* rtv,
            IRHIDepthStencilView* dsv,
            float r = 0, float g = 0, float b = 0, float a = 1,
            float depth = 1.0f)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(rtv, r, g, b, a));
            desc.SetDepthStencil(RHIDepthStencilAttachment::ClearStore(dsv, depth));
            return desc;
        }

        /// デプスオンリーパス
        inline RHIRenderPassDesc DepthOnly(
            IRHIDepthStencilView* dsv,
            bool clear = true,
            float depth = 1.0f)
        {
            RHIRenderPassDesc desc;
            if (clear) {
                desc.SetDepthStencil(RHIDepthStencilAttachment::ClearStore(dsv, depth));
            } else {
                desc.SetDepthStencil(RHIDepthStencilAttachment::LoadStore(dsv));
            }
            return desc;
        }

        /// GBufferパス
        inline RHIRenderPassDesc GBuffer(
            IRHIRenderTargetView* albedo,
            IRHIRenderTargetView* normal,
            IRHIRenderTargetView* material,
            IRHIDepthStencilView* depth)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(albedo));
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(normal));
            desc.AddRenderTarget(RHIRenderTargetAttachment::ClearStore(material));
            desc.SetDepthStencil(RHIDepthStencilAttachment::ClearStore(depth));
            return desc;
        }

        /// ポストプロセス（デプスなし）
        inline RHIRenderPassDesc PostProcess(IRHIRenderTargetView* output)
        {
            RHIRenderPassDesc desc;
            desc.AddRenderTarget(RHIRenderTargetAttachment::DontCareStore(output));
            return desc;
        }
    }
}
```

- [ ] RHIRenderPassPresets

## 検証方法

- [ ] アタッチメント記述の整合性
- [ ] ビルダーの動作
- [ ] プリセットの正確性

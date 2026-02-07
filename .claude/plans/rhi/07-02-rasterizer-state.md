# 07-02: ラスタライザーステート

## 目的

ラスタライザーステート記述を定義する。

## 参照ドキュメント

- 01-10-enums-state.md (ERHICullMode, ERHIFillMode)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIPipelineState.h` (部分

## TODO

### 1. ラスタライザー列挙型

```cpp
namespace NS::RHI
{
    /// カリングモード
    enum class ERHICullMode : uint8
    {
        None,       // カリングなし（両面描画）。
        Front,      // 前面カリング
        Back,       // 背面カリング
    };

    /// フィルモード
    enum class ERHIFillMode : uint8
    {
        Wireframe,  // ワイヤーフレーム
        Solid,      // ソリッド
    };

    /// 前面判定
    enum class ERHIFrontFace : uint8
    {
        CounterClockwise,   // 反時計回り = 前面
        Clockwise,          // 時計回り = 前面
    };

    /// 保守的ラスタライゼーションモード
    enum class ERHIConservativeRaster : uint8
    {
        Off,            // 無効
        On,             // 有効（オーバーエスティメート）
        Underestimate,  // アンダーエスティメート（サポート限定）
    };
}
```

- [ ] ERHICullMode / ERHIFillMode 列挙型
- [ ] ERHIFrontFace / ERHIConservativeRaster 列挙型

### 2. ラスタライザーステート記述

```cpp
namespace NS::RHI
{
    /// ラスタライザーステート記述
    struct RHI_API RHIRasterizerStateDesc
    {
        //=====================================================================
        // 基本設定
        //=====================================================================

        /// フィルモード
        ERHIFillMode fillMode = ERHIFillMode::Solid;

        /// カリングモード
        ERHICullMode cullMode = ERHICullMode::Back;

        /// 前面判定
        ERHIFrontFace frontFace = ERHIFrontFace::CounterClockwise;

        //=====================================================================
        // デプスバイアス
        //=====================================================================

        /// デプスバイアス（整数）。
        int32 depthBias = 0;

        /// デプスバイアスクランプ
        float depthBiasClamp = 0.0f;

        /// 傾斜スケールデプスバイアス
        float slopeScaledDepthBias = 0.0f;

        //=====================================================================
        // クリッピング・カリング
        //=====================================================================

        /// デプスクリップ有効
        bool depthClipEnable = true;

        /// シザーテスト有効
        bool scissorEnable = true;

        //=====================================================================
        // マルチサンプル
        //=====================================================================

        /// マルチサンプル有効
        bool multisampleEnable = false;

        /// アンチエイリアスライン
        bool antialiasedLineEnable = false;

        /// 強制サンプルカウント（0 = 無効）。
        uint32 forcedSampleCount = 0;

        //=====================================================================
        // 保守的ラスタライゼーション
        //=====================================================================

        /// 保守的ラスタライゼーションモード
        ERHIConservativeRaster conservativeRaster = ERHIConservativeRaster::Off;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIRasterizerStateDesc& SetFillMode(ERHIFillMode mode) { fillMode = mode; return *this; }
        RHIRasterizerStateDesc& SetCullMode(ERHICullMode mode) { cullMode = mode; return *this; }
        RHIRasterizerStateDesc& SetFrontFace(ERHIFrontFace face) { frontFace = face; return *this; }
        RHIRasterizerStateDesc& SetDepthBias(int32 bias, float clamp = 0.0f, float slope = 0.0f) {
            depthBias = bias;
            depthBiasClamp = clamp;
            slopeScaledDepthBias = slope;
            return *this;
        }
        RHIRasterizerStateDesc& SetDepthClip(bool enable) { depthClipEnable = enable; return *this; }
        RHIRasterizerStateDesc& SetScissor(bool enable) { scissorEnable = enable; return *this; }
        RHIRasterizerStateDesc& SetMultisample(bool enable) { multisampleEnable = enable; return *this; }
        RHIRasterizerStateDesc& SetConservativeRaster(ERHIConservativeRaster mode) {
            conservativeRaster = mode;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        /// デフォルト（ソリッド、背面カリング）。
        static RHIRasterizerStateDesc Default() {
            return RHIRasterizerStateDesc{};
        }

        /// カリングなし
        static RHIRasterizerStateDesc NoCull() {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::None;
            return desc;
        }

        /// 前面カリング（シャドウマップ等）
        static RHIRasterizerStateDesc FrontCull() {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::Front;
            return desc;
        }

        /// ワイヤーフレーム
        static RHIRasterizerStateDesc Wireframe() {
            RHIRasterizerStateDesc desc;
            desc.fillMode = ERHIFillMode::Wireframe;
            desc.cullMode = ERHICullMode::None;
            return desc;
        }

        /// シャドウマップ用（デプスバイアス付き）。
        static RHIRasterizerStateDesc ShadowMap(int32 bias = 100, float slopeScale = 1.0f) {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::Front;
            desc.depthBias = bias;
            desc.slopeScaledDepthBias = slopeScale;
            desc.depthBiasClamp = 0.0f;
            return desc;
        }

        /// スカイボックス用（前面カリング、デプスクリップなし）
        static RHIRasterizerStateDesc Skybox() {
            RHIRasterizerStateDesc desc;
            desc.cullMode = ERHICullMode::Front;
            desc.depthClipEnable = false;
            return desc;
        }
    };
}
```

- [ ] RHIRasterizerStateDesc 構造体
- [ ] デプスバイアス設定
- [ ] プリセット

### 3. ビューポートのシザー

```cpp
namespace NS::RHI
{
    /// ビューポート（1-03で定義済み、参照）。
    struct RHIViewport
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;

        static RHIViewport FromSize(uint32 w, uint32 h) {
            return RHIViewport{0, 0, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f};
        }

        static RHIViewport FromExtent(const Extent2D& extent) {
            return FromSize(extent.width, extent.height);
        }
    };

    /// シザー矩形（1-03で定義済み、参照）。
    struct RHIRect
    {
        int32 x = 0;
        int32 y = 0;
        int32 width = 0;
        int32 height = 0;

        static RHIRect FromSize(uint32 w, uint32 h) {
            return RHIRect{0, 0, static_cast<int32>(w), static_cast<int32>(h)};
        }

        static RHIRect FromExtent(const Extent2D& extent) {
            return FromSize(extent.width, extent.height);
        }

        /// ビューポートからシザー作成
        static RHIRect FromViewport(const RHIViewport& vp) {
            return RHIRect{
                static_cast<int32>(vp.x),
                static_cast<int32>(vp.y),
                static_cast<int32>(vp.width),
                static_cast<int32>(vp.height)
            };
        }
    };
}
```

- [ ] RHIViewport / RHIRect 確認

### 4. ビューポートのシザー設定

```cpp
namespace NS::RHI
{
    /// 最大ビューポート数
    constexpr uint32 kMaxViewports = 16;

    /// ビューポートの分
    struct RHI_API RHIViewportArray
    {
        RHIViewport viewports[kMaxViewports];
        uint32 count = 0;

        void Set(uint32 index, const RHIViewport& vp) {
            if (index < kMaxViewports) {
                viewports[index] = vp;
                if (index >= count) count = index + 1;
            }
        }

        void Add(const RHIViewport& vp) {
            if (count < kMaxViewports) {
                viewports[count++] = vp;
            }
        }
    };

    /// シザー配列
    struct RHI_API RHIScissorArray
    {
        RHIRect rects[kMaxViewports];
        uint32 count = 0;

        void Set(uint32 index, const RHIRect& rect) {
            if (index < kMaxViewports) {
                rects[index] = rect;
                if (index >= count) count = index + 1;
            }
        }

        void Add(const RHIRect& rect) {
            if (count < kMaxViewports) {
                rects[count++] = rect;
            }
        }

        /// ビューポート配列からシザー配列を生成
        static RHIScissorArray FromViewports(const RHIViewportArray& viewports) {
            RHIScissorArray scissors;
            for (uint32 i = 0; i < viewports.count; ++i) {
                scissors.Add(RHIRect::FromViewport(viewports.viewports[i]));
            }
            return scissors;
        }
    };

    /// ビューポートのシザー設定（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// ビューポート設定（配列）
        virtual void SetViewports(uint32 count, const RHIViewport* viewports) = 0;

        /// ビューポート設定（単一）。
        void SetViewport(const RHIViewport& viewport) {
            SetViewports(1, &viewport);
        }

        /// シザー設定（配列）
        virtual void SetScissorRects(uint32 count, const RHIRect* rects) = 0;

        /// シザー設定（単一）。
        void SetScissorRect(const RHIRect& rect) {
            SetScissorRects(1, &rect);
        }

        /// ビューポートとシザーを同時設定
        void SetViewportAndScissor(const RHIViewport& viewport) {
            SetViewport(viewport);
            SetScissorRect(RHIRect::FromViewport(viewport));
        }
    };
}
```

- [ ] RHIViewportArray / RHIScissorArray 構造体
- [ ] SetViewports / SetScissorRects

### 5. ラインステート

```cpp
namespace NS::RHI
{
    /// ライン描画ステート
    /// ワイヤーフレームやイン描画時の設定
    struct RHI_API RHILineState
    {
        /// ライン幅（サポートのハードウェア依存）
        float lineWidth = 1.0f;

        /// スティプルパターン有効
        bool stippleEnable = false;

        /// スティプルファクター
        uint32 stippleFactor = 1;

        /// スティプルパターン（6ビット）
        uint16 stipplePattern = 0xFFFF;
    };

    /// ライン幅設定（動的ステート）
    class IRHICommandContext
    {
    public:
        /// ライン幅設定
        /// @note サポートのRHISupports(ERHIFeature::WideLines)で確認
        virtual void SetLineWidth(float width) = 0;
    };
}
```

- [ ] RHILineState 構造体
- [ ] SetLineWidth

## 検証方法

- [ ] ラスタライザーステートの整合性
- [ ] プリセットの動作確認
- [ ] ビューポートのシザー設定
- [ ] デプスバイアスの効果確認

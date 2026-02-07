# 07-03: デプスステンシルステート

## 目的

デプス・ステンシルテストのステート記述を定義する。

## 参照ドキュメント

- 01-10-enums-state.md (ERHICompareFunc)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIPipelineState.h` (部分

## TODO

### 1. 比較関数・ステンシル演算列挙型

```cpp
namespace NS::RHI
{
    /// 比較関数
    enum class ERHICompareFunc : uint8
    {
        Never,          // 常にfalse
        Less,           // <
        Equal,          // ==
        LessEqual,      // <=
        Greater,        // >
        NotEqual,       // !=
        GreaterEqual,   // >=
        Always,         // 常にtrue
    };

    /// ステンシル演算
    enum class ERHIStencilOp : uint8
    {
        Keep,           // 現在値を維持
        Zero,           // 0にセット
        Replace,        // 参照値に置換
        IncrSat,        // インクリメント（サチュレート）
        DecrSat,        // デクリメント（サチュレート）
        Invert,         // ビット反転
        IncrWrap,       // インクリメント（ラップ）
        DecrWrap,       // デクリメント（ラップ）
    };
}
```

- [ ] ERHICompareFunc 列挙型
- [ ] ERHIStencilOp 列挙型

### 2. ステンシル面記述

```cpp
namespace NS::RHI
{
    /// ステンシル面記述
    struct RHI_API RHIStencilFaceDesc
    {
        /// ステンシルテスト失敗時の演算
        ERHIStencilOp stencilFailOp = ERHIStencilOp::Keep;

        /// ステンシルテスト成功のデプステスト失敗時の演算
        ERHIStencilOp stencilDepthFailOp = ERHIStencilOp::Keep;

        /// 両テストの成功時の演算
        ERHIStencilOp stencilPassOp = ERHIStencilOp::Keep;

        /// ステンシル比較関数
        ERHICompareFunc stencilFunc = ERHICompareFunc::Always;

        //=====================================================================
        // プリセット
        //=====================================================================

        /// 無効（テストなし）
        static RHIStencilFaceDesc Disabled() {
            return RHIStencilFaceDesc{};
        }

        /// インクリメント（シャドウボリューム用）。
        static RHIStencilFaceDesc Increment() {
            RHIStencilFaceDesc desc;
            desc.stencilFailOp = ERHIStencilOp::Keep;
            desc.stencilDepthFailOp = ERHIStencilOp::Keep;
            desc.stencilPassOp = ERHIStencilOp::IncrSat;
            desc.stencilFunc = ERHICompareFunc::Always;
            return desc;
        }

        /// デクリメント（シャドウボリューム用）。
        static RHIStencilFaceDesc Decrement() {
            RHIStencilFaceDesc desc;
            desc.stencilFailOp = ERHIStencilOp::Keep;
            desc.stencilDepthFailOp = ERHIStencilOp::Keep;
            desc.stencilPassOp = ERHIStencilOp::DecrSat;
            desc.stencilFunc = ERHICompareFunc::Always;
            return desc;
        }

        /// マスクテスト（ステンシルが参照値と等しいクセルのみ描画）。
        static RHIStencilFaceDesc MaskEqual() {
            RHIStencilFaceDesc desc;
            desc.stencilFunc = ERHICompareFunc::Equal;
            desc.stencilFailOp = ERHIStencilOp::Keep;
            desc.stencilDepthFailOp = ERHIStencilOp::Keep;
            desc.stencilPassOp = ERHIStencilOp::Keep;
            return desc;
        }

        /// マスク書き込み（常に参照値を書き込み）。
        static RHIStencilFaceDesc MaskWrite() {
            RHIStencilFaceDesc desc;
            desc.stencilFunc = ERHICompareFunc::Always;
            desc.stencilFailOp = ERHIStencilOp::Replace;
            desc.stencilDepthFailOp = ERHIStencilOp::Replace;
            desc.stencilPassOp = ERHIStencilOp::Replace;
            return desc;
        }
    };
}
```

- [ ] RHIStencilFaceDesc 構造体
- [ ] プリセット

### 3. デプスステンシルステート記述

```cpp
namespace NS::RHI
{
    /// デプスステンシルステート記述
    struct RHI_API RHIDepthStencilStateDesc
    {
        //=====================================================================
        // デプステスト
        //=====================================================================

        /// デプステスト有効
        bool depthTestEnable = true;

        /// デプス書き込み有効
        bool depthWriteEnable = true;

        /// デプス比較関数
        ERHICompareFunc depthFunc = ERHICompareFunc::Less;

        //=====================================================================
        // ステンシルテスト
        //=====================================================================

        /// ステンシルテスト有効
        bool stencilTestEnable = false;

        /// ステンシル読み取りマスク
        uint8 stencilReadMask = 0xFF;

        /// ステンシル書き込みマスク
        uint8 stencilWriteMask = 0xFF;

        /// 前面ステンシル記述
        RHIStencilFaceDesc frontFace;

        /// 背面ステンシル記述
        RHIStencilFaceDesc backFace;

        //=====================================================================
        // ビルダー
        //=====================================================================

        RHIDepthStencilStateDesc& SetDepthTest(bool enable, ERHICompareFunc func = ERHICompareFunc::Less) {
            depthTestEnable = enable;
            depthFunc = func;
            return *this;
        }

        RHIDepthStencilStateDesc& SetDepthWrite(bool enable) {
            depthWriteEnable = enable;
            return *this;
        }

        RHIDepthStencilStateDesc& SetStencilTest(bool enable) {
            stencilTestEnable = enable;
            return *this;
        }

        RHIDepthStencilStateDesc& SetStencilMasks(uint8 read, uint8 write) {
            stencilReadMask = read;
            stencilWriteMask = write;
            return *this;
        }

        RHIDepthStencilStateDesc& SetFrontFace(const RHIStencilFaceDesc& desc) {
            frontFace = desc;
            return *this;
        }

        RHIDepthStencilStateDesc& SetBackFace(const RHIStencilFaceDesc& desc) {
            backFace = desc;
            return *this;
        }

        RHIDepthStencilStateDesc& SetBothFaces(const RHIStencilFaceDesc& desc) {
            frontFace = desc;
            backFace = desc;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        /// デフォルト（デプステスト有効、書き込み有効、Less）。
        static RHIDepthStencilStateDesc Default() {
            return RHIDepthStencilStateDesc{};
        }

        /// リバースZ用のGreater比較関数。
        static RHIDepthStencilStateDesc ReversedZ() {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::Greater;
            return desc;
        }

        /// 書き込みなし（テストのみ）。
        static RHIDepthStencilStateDesc ReadOnly() {
            RHIDepthStencilStateDesc desc;
            desc.depthWriteEnable = false;
            return desc;
        }

        /// デプス無効
        static RHIDepthStencilStateDesc NoDepth() {
            RHIDepthStencilStateDesc desc;
            desc.depthTestEnable = false;
            desc.depthWriteEnable = false;
            return desc;
        }

        /// デプス等しい（プリパス後の描画用）。
        static RHIDepthStencilStateDesc DepthEqual() {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::Equal;
            desc.depthWriteEnable = false;
            return desc;
        }

        /// リバースZ + 等しか
        static RHIDepthStencilStateDesc ReversedZEqual() {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::GreaterEqual;
            desc.depthWriteEnable = false;
            return desc;
        }

        /// スカイボックス用（最遠描画）。
        static RHIDepthStencilStateDesc Skybox() {
            RHIDepthStencilStateDesc desc;
            desc.depthFunc = ERHICompareFunc::LessEqual;
            desc.depthWriteEnable = false;
            return desc;
        }

        /// ステンシルマスキング（等しい場合のみ描画）。
        static RHIDepthStencilStateDesc StencilMask() {
            RHIDepthStencilStateDesc desc;
            desc.stencilTestEnable = true;
            desc.SetBothFaces(RHIStencilFaceDesc::MaskEqual());
            return desc;
        }
    };
}
```

- [ ] RHIDepthStencilStateDesc 構造体
- [ ] ビルダーメソッド
- [ ] プリセット

### 4. 動的ステンシル参照値

```cpp
namespace NS::RHI
{
    /// ステンシル参照値設定（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// ステンシル参照値設定
        /// @param refValue ステンシル参照値（-255）。
        virtual void SetStencilRef(uint32 refValue) = 0;
    };
}
```

- [ ] SetStencilRef

### 5. デプスバウンドテスチ

```cpp
namespace NS::RHI
{
    /// デプスバウンドテスト記述
    /// ハードウェアによるオクルージョンカリング補助
    struct RHI_API RHIDepthBoundsTest
    {
        /// 有効
        bool enable = false;

        /// 最小デプス値
        float minDepth = 0.0f;

        /// 最大デプス値
        float maxDepth = 1.0f;

        static RHIDepthBoundsTest Disabled() {
            return RHIDepthBoundsTest{};
        }

        static RHIDepthBoundsTest Range(float min, float max) {
            RHIDepthBoundsTest test;
            test.enable = true;
            test.minDepth = min;
            test.maxDepth = max;
            return test;
        }
    };

    /// デプスバウンド設定（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// デプスバウンド設定
        virtual void SetDepthBounds(float minDepth, float maxDepth) = 0;

        void SetDepthBounds(const RHIDepthBoundsTest& bounds) {
            if (bounds.enable) {
                SetDepthBounds(bounds.minDepth, bounds.maxDepth);
            } else {
                SetDepthBounds(0.0f, 1.0f);
            }
        }
    };
}
```

- [ ] RHIDepthBoundsTest 構造体
- [ ] SetDepthBounds

## 検証方法

- [ ] デプスステンシルステートの整合性
- [ ] プリセットの動作確認
- [ ] ステンシル参照値設定
- [ ] デプスバウンドテストの動作

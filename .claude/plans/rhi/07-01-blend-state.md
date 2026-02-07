# 07-01: ブレンドステート

## 目的

出力マージャーのブレンドステート記述を定義する。

## 参照ドキュメント

- 01-10-enums-state.md (ERHIBlendFactor, ERHIBlendOp)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIPipelineState.h` (部分

## TODO

### 1. ブレンド列挙型（1-10で定義済み、参照用）。

```cpp
namespace NS::RHI
{
    /// ブレンドファクター
    enum class ERHIBlendFactor : uint8
    {
        Zero,
        One,
        SrcColor,
        InvSrcColor,
        SrcAlpha,
        InvSrcAlpha,
        DstColor,
        InvDstColor,
        DstAlpha,
        InvDstAlpha,
        SrcAlphaSat,
        BlendFactor,        // 定数ブレンドファクター
        InvBlendFactor,
        Src1Color,          // デュアルソースブレンド
        InvSrc1Color,
        Src1Alpha,
        InvSrc1Alpha,
    };

    /// ブレンド演算
    enum class ERHIBlendOp : uint8
    {
        Add,
        Subtract,
        RevSubtract,
        Min,
        Max,
    };

    /// カラー書き込みマスク
    enum class ERHIColorWriteMask : uint8
    {
        None  = 0,
        Red   = 1 << 0,
        Green = 1 << 1,
        Blue  = 1 << 2,
        Alpha = 1 << 3,
        All   = Red | Green | Blue | Alpha,
        RGB   = Red | Green | Blue,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIColorWriteMask)
}
```

- [ ] 列挙型確認（1-10で実装）。

### 2. レンダーターゲットブレンド記述

```cpp
namespace NS::RHI
{
    /// レンダーターゲットブレンド記述
    struct RHI_API RHIRenderTargetBlendDesc
    {
        /// ブレンド有効
        bool blendEnable = false;

        /// 論理演算有効（ブレンドと排他）
        bool logicOpEnable = false;

        //=====================================================================
        // カラーブレンド
        //=====================================================================

        /// ソースカラーブレンドファクター
        ERHIBlendFactor srcBlend = ERHIBlendFactor::One;

        /// デスティネーションカラーブレンドファクター
        ERHIBlendFactor dstBlend = ERHIBlendFactor::Zero;

        /// カラーブレンド演算
        ERHIBlendOp blendOp = ERHIBlendOp::Add;

        //=====================================================================
        // アルファブレンド
        //=====================================================================

        /// ソースアルファブレンドファクター
        ERHIBlendFactor srcBlendAlpha = ERHIBlendFactor::One;

        /// デスティネーションアルファブレンドファクター
        ERHIBlendFactor dstBlendAlpha = ERHIBlendFactor::Zero;

        /// アルファブレンド演算
        ERHIBlendOp blendOpAlpha = ERHIBlendOp::Add;

        //=====================================================================
        // その他
        //=====================================================================

        /// 論理演算（LogicOpEnableがtrueの場合）
        enum class LogicOp : uint8
        {
            Clear, Set, Copy, CopyInverted,
            NoOp, Invert, And, Nand,
            Or, Nor, Xor, Equiv,
            AndReverse, AndInverted,
            OrReverse, OrInverted,
        };
        LogicOp logicOp = LogicOp::NoOp;

        /// 書き込みマスク
        ERHIColorWriteMask writeMask = ERHIColorWriteMask::All;

        //=====================================================================
        // プリセット
        //=====================================================================

        /// 無効（ブレンドなし、書き込みあり）。
        static RHIRenderTargetBlendDesc Disabled() {
            return RHIRenderTargetBlendDesc{};
        }

        /// アルファブレンド（通常）。
        static RHIRenderTargetBlendDesc AlphaBlend() {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::SrcAlpha;
            desc.dstBlend = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::One;
            desc.dstBlendAlpha = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        /// 加算ブレンド
        static RHIRenderTargetBlendDesc Additive() {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::SrcAlpha;
            desc.dstBlend = ERHIBlendFactor::One;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::One;
            desc.dstBlendAlpha = ERHIBlendFactor::One;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        /// 乗算ブレンド
        static RHIRenderTargetBlendDesc Multiply() {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::DstColor;
            desc.dstBlend = ERHIBlendFactor::Zero;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::DstAlpha;
            desc.dstBlendAlpha = ERHIBlendFactor::Zero;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        /// プリマルチプライドアルファ
        static RHIRenderTargetBlendDesc PremultipliedAlpha() {
            RHIRenderTargetBlendDesc desc;
            desc.blendEnable = true;
            desc.srcBlend = ERHIBlendFactor::One;
            desc.dstBlend = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOp = ERHIBlendOp::Add;
            desc.srcBlendAlpha = ERHIBlendFactor::One;
            desc.dstBlendAlpha = ERHIBlendFactor::InvSrcAlpha;
            desc.blendOpAlpha = ERHIBlendOp::Add;
            return desc;
        }

        /// 書き込み無効
        static RHIRenderTargetBlendDesc NoWrite() {
            RHIRenderTargetBlendDesc desc;
            desc.writeMask = ERHIColorWriteMask::None;
            return desc;
        }
    };
}
```

- [ ] RHIRenderTargetBlendDesc 構造体
- [ ] プリセット関数

### 3. ブレンドステート記述

```cpp
namespace NS::RHI
{
    /// ブレンドステート記述
    struct RHI_API RHIBlendStateDesc
    {
        /// アルファtoカバレッジ有効
        bool alphaToCoverageEnable = false;

        /// 独立ブレンド有効（falseの場合、RT[0]を全RTに適用）。
        bool independentBlendEnable = false;

        /// レンダーターゲットブレンド記述
        RHIRenderTargetBlendDesc renderTarget[kMaxRenderTargets];

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// 全RT同一設定
        RHIBlendStateDesc& SetAll(const RHIRenderTargetBlendDesc& desc) {
            for (auto& rt : renderTarget) rt = desc;
            independentBlendEnable = false;
            return *this;
        }

        /// 特定RT設定
        RHIBlendStateDesc& SetRT(uint32 index, const RHIRenderTargetBlendDesc& desc) {
            if (index < kMaxRenderTargets) {
                renderTarget[index] = desc;
                if (index > 0) independentBlendEnable = true;
            }
            return *this;
        }

        /// アルファtoカバレッジ設定
        RHIBlendStateDesc& SetAlphaToCoverage(bool enable) {
            alphaToCoverageEnable = enable;
            return *this;
        }

        //=====================================================================
        // プリセット
        //=====================================================================

        /// デフォルト（ブレンドなし）
        static RHIBlendStateDesc Default() {
            return RHIBlendStateDesc{};
        }

        /// 全RT同一のアルファブレンド
        static RHIBlendStateDesc AlphaBlend() {
            RHIBlendStateDesc desc;
            desc.SetAll(RHIRenderTargetBlendDesc::AlphaBlend());
            return desc;
        }

        /// 加算ブレンド
        static RHIBlendStateDesc Additive() {
            RHIBlendStateDesc desc;
            desc.SetAll(RHIRenderTargetBlendDesc::Additive());
            return desc;
        }

        /// プリマルチプライドアルファ
        static RHIBlendStateDesc PremultipliedAlpha() {
            RHIBlendStateDesc desc;
            desc.SetAll(RHIRenderTargetBlendDesc::PremultipliedAlpha());
            return desc;
        }

        /// 不透明（高速パス）。
        static RHIBlendStateDesc Opaque() {
            return Default();
        }
    };
}
```

- [ ] RHIBlendStateDesc 構造体
- [ ] ビルダーメソッド
- [ ] プリセット

### 4. ブレンドファクター定数

```cpp
namespace NS::RHI
{
    /// ブレンドファクター定数（RHIBlendFactor::BlendFactor用）。
    struct RHI_API RHIBlendConstants
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;

        RHIBlendConstants() = default;
        RHIBlendConstants(float r_, float g_, float b_, float a_)
            : r(r_), g(g_), b(b_), a(a_) {}

        static RHIBlendConstants White() { return {1, 1, 1, 1}; }
        static RHIBlendConstants Black() { return {0, 0, 0, 1}; }
        static RHIBlendConstants Transparent() { return {0, 0, 0, 0}; }

        /// 配列として取得
        const float* AsArray() const { return &r; }
    };

    /// ブレンドファクター設定（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// ブレンドファクター設定
        virtual void SetBlendFactor(const float factor[4]) = 0;

        void SetBlendFactor(const RHIBlendConstants& constants) {
            SetBlendFactor(constants.AsArray());
        }
    };
}
```

- [ ] RHIBlendConstants 構造体
- [ ] SetBlendFactor

### 5. サンプルマスク

```cpp
namespace NS::RHI
{
    /// サンプルマスク
    /// マルチサンプルレンダリング時の書き込みマスク
    struct RHI_API RHISampleMask
    {
        /// サンプルマスク（ビットごとにサンプル有効/無効）。
        uint32 mask = 0xFFFFFFFF;

        RHISampleMask() = default;
        explicit RHISampleMask(uint32 m) : mask(m) {}

        /// 全サンプル有効
        static RHISampleMask All() { return RHISampleMask(0xFFFFFFFF); }

        /// 全サンプル無効
        static RHISampleMask None() { return RHISampleMask(0); }

        /// 特定サンプル数用
        static RHISampleMask ForSampleCount(ERHISampleCount count) {
            uint32 sampleCount = GetSampleCount(count);
            return RHISampleMask((1u << sampleCount) - 1);
        }
    };
}
```

- [ ] RHISampleMask 構造体

## 検証方法

- [ ] ブレンドステートの整合性
- [ ] プリセットの動作確認
- [ ] ブレンドファクター設定
- [ ] サンプルマスクの有効性

# 01-10: レンダーステート列挙型

## 目的

ブレンド、ラスタライザー、デプスステンシル等のレンダーステート列挙型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part3.md (パイプラインステート)
- docs/RHI/RHI_Implementation_Guide_Part16.md (列挙型リファレンス)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIEnums.h` (部分)

## TODO

### 1. ERHIBlendFactor: ブレンドファクター

```cpp
namespace NS::RHI
{
    /// ブレンドファクター
    enum class ERHIBlendFactor : uint8
    {
        Zero,               // 0
        One,                // 1
        SrcColor,           // ソースカラー
        InvSrcColor,        // 1 - ソースカラー
        SrcAlpha,           // ソースアルファ
        InvSrcAlpha,        // 1 - ソースアルファ
        DstColor,           // デスティネーションカラー
        InvDstColor,        // 1 - デスティネーションカラー
        DstAlpha,           // デスティネーションアルファ
        InvDstAlpha,        // 1 - デスティネーションアルファ
        SrcAlphaSaturate,   // min(SrcAlpha, 1 - DstAlpha)
        BlendFactor,        // 定数ブレンドファクター
        InvBlendFactor,     // 1 - 定数ブレンドファクター
        Src1Color,          // デュアルソースカラー
        InvSrc1Color,       // 1 - デュアルソースカラー
        Src1Alpha,          // デュアルソースアルファ
        InvSrc1Alpha,       // 1 - デュアルソースアルファ
    };
}
```

- [ ] ERHIBlendFactor 列挙型

### 2. ERHIBlendOp: ブレンド演算

```cpp
namespace NS::RHI
{
    /// ブレンド演算
    enum class ERHIBlendOp : uint8
    {
        Add,            // Src + Dst
        Subtract,       // Src - Dst
        RevSubtract,    // Dst - Src
        Min,            // min(Src, Dst)
        Max,            // max(Src, Dst)
    };
}
```

- [ ] ERHIBlendOp 列挙型

### 3. ERHIColorWriteMask: カラー書き込みマスク

```cpp
namespace NS::RHI
{
    /// カラー書き込みマスク
    enum class ERHIColorWriteMask : uint8
    {
        None  = 0,
        Red   = 1 << 0,
        Green = 1 << 1,
        Blue  = 1 << 2,
        Alpha = 1 << 3,

        RGB   = Red | Green | Blue,
        All   = Red | Green | Blue | Alpha,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIColorWriteMask)
}
```

- [ ] ERHIColorWriteMask ビットフラグ

### 4. ERHICompareFunc: 比較関数

```cpp
namespace NS::RHI
{
    /// 比較関数（デプス、ステンシル、サンプラー用）
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
}
```

- [ ] ERHICompareFunc 列挙型

### 5. ERHIStencilOp: ステンシル演算

```cpp
namespace NS::RHI
{
    /// ステンシル演算
    enum class ERHIStencilOp : uint8
    {
        Keep,           // 維持
        Zero,           // 0にセット
        Replace,        // 参照値で置換
        IncrSat,        // インクリメント（飽和）
        DecrSat,        // デクリメント（飽和）
        Invert,         // ビット反転
        IncrWrap,       // インクリメント（ラップ）
        DecrWrap,       // デクリメント（ラップ）
    };
}
```

- [ ] ERHIStencilOp 列挙型

### 6. ERHICullMode: カリングモード

```cpp
namespace NS::RHI
{
    /// カリングモード
    enum class ERHICullMode : uint8
    {
        None,           // カリングなし
        Front,          // 前面カリング
        Back,           // 背面カリング
    };
}
```

- [ ] ERHICullMode 列挙型

### 7. ERHIFillMode: フィルモード

```cpp
namespace NS::RHI
{
    /// フィルモード
    enum class ERHIFillMode : uint8
    {
        Solid,          // ソリッド（塗りつぶし）
        Wireframe,      // ワイヤーフレーム
    };
}
```

- [ ] ERHIFillMode 列挙型

### 8. ERHIPrimitiveTopology: プリミティブトポロジー

```cpp
namespace NS::RHI
{
    /// プリミティブトポロジー
    enum class ERHIPrimitiveTopology : uint8
    {
        PointList,              // 点リスト
        LineList,               // 線リスト
        LineStrip,              // 線ストリップ
        TriangleList,           // 三角形リスト
        TriangleStrip,          // 三角形ストリップ
        LineListAdj,            // 隣接付き線リスト
        LineStripAdj,           // 隣接付き線ストリップ
        TriangleListAdj,        // 隣接付き三角形リスト
        TriangleStripAdj,       // 隣接付き三角形ストリップ
        PatchList,              // パッチリスト（テッセレーション）
    };

    /// 三角形トポロジーか
    inline bool IsTriangleTopology(ERHIPrimitiveTopology topology) {
        switch (topology) {
            case ERHIPrimitiveTopology::TriangleList:
            case ERHIPrimitiveTopology::TriangleStrip:
            case ERHIPrimitiveTopology::TriangleListAdj:
            case ERHIPrimitiveTopology::TriangleStripAdj:
                return true;
            default:
                return false;
        }
    }
}
```

- [ ] ERHIPrimitiveTopology 列挙型
- [ ] トポロジー判定関数

### 9. ERHIFrontFace: 前面判定

```cpp
namespace NS::RHI
{
    /// 前面の巻き方向
    enum class ERHIFrontFace : uint8
    {
        CounterClockwise,   // 反時計回りが前面
        Clockwise,          // 時計回りが前面
    };
}
```

- [ ] ERHIFrontFace 列挙型

### 10. ERHILogicOp: 論理演算

```cpp
namespace NS::RHI
{
    /// 論理演算（ブレンド用）
    enum class ERHILogicOp : uint8
    {
        Clear,          // 0
        Set,            // 1
        Copy,           // Src
        CopyInverted,   // ~Src
        Noop,           // Dst
        Invert,         // ~Dst
        And,            // Src & Dst
        Nand,           // ~(Src & Dst)
        Or,             // Src | Dst
        Nor,            // ~(Src | Dst)
        Xor,            // Src ^ Dst
        Equiv,          // ~(Src ^ Dst)
        AndReverse,     // Src & ~Dst
        AndInverted,    // ~Src & Dst
        OrReverse,      // Src | ~Dst
        OrInverted,     // ~Src | Dst
    };
}
```

- [ ] ERHILogicOp 列挙型

## 検証方法

- [ ] D3D12/Vulkan列挙値との対応
- [ ] 全値の網羅性
- [ ] 使用例との整合性

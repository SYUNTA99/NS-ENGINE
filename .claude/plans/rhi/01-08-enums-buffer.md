# 01-08: バッファ使用フラグ列挙型

## 目的

バッファ作成時の使用フラグと関連列挙型を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (バッファ)
- docs/RHI/RHI_Implementation_Guide_Part16.md (列挙型リファレンス)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIEnums.h` (部分)

## TODO

### 1. ERHIBufferUsage: バッファ使用フラグ

```cpp
namespace NS::RHI
{
    /// バッファ使用フラグ（ビットフラグ）
    enum class ERHIBufferUsage : uint32
    {
        None                = 0,

        //=========================================================================
        // 基本用途
        //=========================================================================

        /// 頂点バッファとして使用
        VertexBuffer        = 1 << 0,

        /// インデックスバッファとして使用
        IndexBuffer         = 1 << 1,

        /// 定数バッファ（ユニフォームバッファ）として使用
        ConstantBuffer      = 1 << 2,

        //=========================================================================
        // シェーダーリソース
        //=========================================================================

        /// シェーダーリソースビュー（SRV）として使用
        ShaderResource      = 1 << 3,

        /// アンオーダードアクセスビュー（UAV）として使用
        UnorderedAccess     = 1 << 4,

        //=========================================================================
        // 構造化バッファ
        //=========================================================================

        /// 構造化バッファ（StructuredBuffer）
        StructuredBuffer    = 1 << 5,

        /// バイトアドレスバッファ（ByteAddressBuffer）
        ByteAddressBuffer   = 1 << 6,

        //=========================================================================
        // 特殊用途
        //=========================================================================

        /// 間接引数バッファ（DrawIndirect等）
        IndirectArgs        = 1 << 7,

        /// ストリームアウトバッファ
        StreamOutput        = 1 << 8,

        /// レイトレーシング加速構造
        AccelerationStructure = 1 << 9,

        //=========================================================================
        // メモリ・アクセスヒント
        //=========================================================================

        /// CPUからの読み取りアクセスを保持
        /// リードバック用（GPU → CPU）
        CPUReadable         = 1 << 10,

        /// CPUからの書き込みアクセス
        /// アップロード/ダイナミック更新用
        CPUWritable         = 1 << 11,

        /// 動的バッファ（毎フレーム更新想定）
        Dynamic             = 1 << 12,

        /// コピー元として使用可能
        CopySource          = 1 << 13,

        /// コピー先として使用可能
        CopyDest            = 1 << 14,

        //=========================================================================
        // 便利な組み合わせ
        //=========================================================================

        /// 動的頂点バッファ
        DynamicVertexBuffer = VertexBuffer | Dynamic | CPUWritable,

        /// 動的インデックスバッファ
        DynamicIndexBuffer = IndexBuffer | Dynamic | CPUWritable,

        /// 動的定数バッファ
        DynamicConstantBuffer = ConstantBuffer | Dynamic | CPUWritable,

        /// GPU専用バッファ（デフォルト）
        Default = ShaderResource,

        /// ステージングバッファ（CPU ↔ GPU転送用）
        Staging = CPUReadable | CPUWritable | CopySource | CopyDest,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIBufferUsage)
}
```

- [ ] ERHIBufferUsage ビットフラグ
- [ ] 便利な組み合わせ定数

### 2. バッファ使用フラグユーティリティ

```cpp
namespace NS::RHI
{
    /// 頂点/インデックスバッファか
    inline bool IsVertexOrIndexBuffer(ERHIBufferUsage usage) {
        return EnumHasAnyFlags(usage,
            ERHIBufferUsage::VertexBuffer | ERHIBufferUsage::IndexBuffer);
    }

    /// シェーダーからアクセス可能か
    inline bool IsShaderAccessible(ERHIBufferUsage usage) {
        return EnumHasAnyFlags(usage,
            ERHIBufferUsage::ShaderResource |
            ERHIBufferUsage::UnorderedAccess |
            ERHIBufferUsage::ConstantBuffer);
    }

    /// CPU書き込み可能か
    inline bool IsCPUWritable(ERHIBufferUsage usage) {
        return EnumHasAnyFlags(usage, ERHIBufferUsage::CPUWritable);
    }

    /// CPU読み取り可能か
    inline bool IsCPUReadable(ERHIBufferUsage usage) {
        return EnumHasAnyFlags(usage, ERHIBufferUsage::CPUReadable);
    }

    /// 動的バッファか
    inline bool IsDynamic(ERHIBufferUsage usage) {
        return EnumHasAnyFlags(usage, ERHIBufferUsage::Dynamic);
    }

    /// 構造化バッファか
    inline bool IsStructured(ERHIBufferUsage usage) {
        return EnumHasAnyFlags(usage,
            ERHIBufferUsage::StructuredBuffer | ERHIBufferUsage::ByteAddressBuffer);
    }
}
```

- [ ] 使用フラグ判定ユーティリティ

### 3. ERHIIndexFormat: インデックスフォーマット

```cpp
namespace NS::RHI
{
    /// インデックスバッファフォーマット
    enum class ERHIIndexFormat : uint8
    {
        UInt16,     // 16ビットインデックス
        UInt32,     // 32ビットインデックス
    };

    /// インデックスサイズ取得（バイト）
    inline uint32 GetIndexFormatSize(ERHIIndexFormat format)
    {
        switch (format) {
            case ERHIIndexFormat::UInt16: return 2;
            case ERHIIndexFormat::UInt32: return 4;
            default:                       return 4;
        }
    }
}
```

- [ ] ERHIIndexFormat 列挙型
- [ ] サイズ取得関数

### 4. ERHIMapMode: マップモード

```cpp
namespace NS::RHI
{
    /// バッファマップモード
    enum class ERHIMapMode : uint8
    {
        /// 読み取り専用
        Read,

        /// 書き込み専用
        Write,

        /// 読み書き両方
        ReadWrite,

        /// 書き込み（以前の内容を破棄）
        WriteDiscard,

        /// 書き込み（同期なし - 上書きしない領域）
        WriteNoOverwrite,
    };

    /// 読み取りアクセスを含むか
    inline bool MapModeHasRead(ERHIMapMode mode) {
        return mode == ERHIMapMode::Read || mode == ERHIMapMode::ReadWrite;
    }

    /// 書き込みアクセスを含むか
    inline bool MapModeHasWrite(ERHIMapMode mode) {
        return mode != ERHIMapMode::Read;
    }
}
```

- [ ] ERHIMapMode 列挙型
- [ ] アクセス判定関数

### 5. ERHIBufferSRVFormat: バッファSRVフォーマット

```cpp
namespace NS::RHI
{
    /// バッファSRVフォーマット（型付きバッファ用）
    enum class ERHIBufferSRVFormat : uint8
    {
        /// 構造化バッファ（フォーマットなし）
        Structured,

        /// バイトアドレスバッファ
        Raw,

        /// 型付きバッファ（要 ERHIPixelFormat指定）
        Typed,
    };
}
```

- [ ] ERHIBufferSRVFormat 列挙型

## 検証方法

- [ ] フラグ組み合わせの妥当性
- [ ] ビット重複なし
- [ ] 使用例との整合性

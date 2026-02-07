# 07-04: 入力レイアウト

## 目的

頂点入力レイアウトの記述を定義する。

## 参照ドキュメント

- 06-03-shader-reflection.md (RHIInputSignature)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIPipelineState.h` (部分

## TODO

### 1. 入力分類

```cpp
namespace NS::RHI
{
    /// 入力スロット分類。
    enum class ERHIInputClassification : uint8
    {
        PerVertex,      // 頂点ごとのデータ
        PerInstance,    // インスタンスごとのデータ
    };

    /// 頂点属性フォーマット
    /// 頂点バッファ内の互換性の形式
    enum class ERHIVertexFormat : uint8
    {
        Unknown,

        // 32ビット浮動小数点
        Float1,         // R32_Float
        Float2,         // R32G32_Float
        Float3,         // R32G32B32_Float
        Float4,         // R32G32B32A32_Float

        // 16ビット浮動小数点
        Half2,          // R16G16_Float
        Half4,          // R16G16B16A16_Float

        // 32ビット整数
        Int1,           // R32_Int
        Int2,           // R32G32_Int
        Int3,           // R32G32B32_Int
        Int4,           // R32G32B32A32_Int

        // 32ビット符号なし整数
        UInt1,          // R32_UInt
        UInt2,          // R32G32_UInt
        UInt3,          // R32G32B32_UInt
        UInt4,          // R32G32B32A32_UInt

        // 16ビット整数（正規化）。
        Short2N,        // R16G16_SNorm
        Short4N,        // R16G16B16A16_SNorm
        UShort2N,       // R16G16_UNorm
        UShort4N,       // R16G16B16A16_UNorm

        // 16ビット整数
        Short2,         // R16G16_SInt
        Short4,         // R16G16B16A16_SInt
        UShort2,        // R16G16_UInt
        UShort4,        // R16G16B16A16_UInt

        // 8ビット整数（正規化）。
        Byte4N,         // R8G8B8A8_SNorm
        UByte4N,        // R8G8B8A8_UNorm
        UByte4N_BGRA,   // B8G8R8A8_UNorm（カラー用）。

        // 8ビット整数
        Byte4,          // R8G8B8A8_SInt
        UByte4,         // R8G8B8A8_UInt

        // 10/10/10/2（正規化）。
        UInt1010102N,   // R10G10B10A2_UNorm
    };

    /// 頂点フォーマットサイズ取得（バイト）
    inline uint32 GetVertexFormatSize(ERHIVertexFormat format)
    {
        switch (format) {
            case ERHIVertexFormat::Float1:
            case ERHIVertexFormat::Int1:
            case ERHIVertexFormat::UInt1:
                return 4;

            case ERHIVertexFormat::Float2:
            case ERHIVertexFormat::Half4:
            case ERHIVertexFormat::Int2:
            case ERHIVertexFormat::UInt2:
            case ERHIVertexFormat::Short4N:
            case ERHIVertexFormat::UShort4N:
            case ERHIVertexFormat::Short4:
            case ERHIVertexFormat::UShort4:
            case ERHIVertexFormat::Byte4N:
            case ERHIVertexFormat::UByte4N:
            case ERHIVertexFormat::UByte4N_BGRA:
            case ERHIVertexFormat::Byte4:
            case ERHIVertexFormat::UByte4:
            case ERHIVertexFormat::UInt1010102N:
                return 4;

            case ERHIVertexFormat::Float3:
            case ERHIVertexFormat::Int3:
            case ERHIVertexFormat::UInt3:
                return 12;

            case ERHIVertexFormat::Float4:
            case ERHIVertexFormat::Int4:
            case ERHIVertexFormat::UInt4:
                return 16;

            case ERHIVertexFormat::Half2:
            case ERHIVertexFormat::Short2N:
            case ERHIVertexFormat::UShort2N:
            case ERHIVertexFormat::Short2:
            case ERHIVertexFormat::UShort2:
                return 4;

            default:
                return 0;
        }
    }
}
```

- [ ] ERHIInputClassification 列挙型
- [ ] ERHIVertexFormat 列挙型
- [ ] GetVertexFormatSize

### 2. 入力要素記述

```cpp
namespace NS::RHI
{
    /// 最大入力要素数
    constexpr uint32 kMaxInputElements = 32;

    /// 入力要素記述
    struct RHI_API RHIInputElementDesc
    {
        /// セマンデバッグ名（POSITION", "NORMAL", "TEXCOORD"等）
        const char* semanticName = nullptr;

        /// セマンデバッグインデックス（EXCOORD0, TEXCOORD1等）
        uint32 semanticIndex = 0;

        /// フォーマット
        ERHIVertexFormat format = ERHIVertexFormat::Float3;

        /// 入力スロット（頂点バッファインデックス）。
        uint32 inputSlot = 0;

        /// 要素オフセット（バイト、kAppendAlignedで自動計算）
        uint32 alignedByteOffset = 0;

        /// 入力分類
        ERHIInputClassification inputClass = ERHIInputClassification::PerVertex;

        /// インスタンステータステンプレート
        /// PerInstanceの場合、何インスタンスごとにデータを進めるか
        uint32 instanceDataStepRate = 0;

        /// オフセット自動計算用定数
        static constexpr uint32 kAppendAligned = ~0u;

        //=====================================================================
        // ビルダー
        //=====================================================================

        static RHIInputElementDesc Position(uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"POSITION", 0, ERHIVertexFormat::Float3, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc Normal(uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"NORMAL", 0, ERHIVertexFormat::Float3, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc Tangent(uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"TANGENT", 0, ERHIVertexFormat::Float4, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc TexCoord(uint32 index = 0, uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"TEXCOORD", index, ERHIVertexFormat::Float2, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc Color(uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"COLOR", 0, ERHIVertexFormat::UByte4N, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc BlendIndices(uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"BLENDINDICES", 0, ERHIVertexFormat::UByte4, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc BlendWeights(uint32 slot = 0, uint32 offset = kAppendAligned) {
            return {"BLENDWEIGHT", 0, ERHIVertexFormat::UByte4N, slot, offset, ERHIInputClassification::PerVertex, 0};
        }

        static RHIInputElementDesc InstanceTransform(uint32 row, uint32 slot = 1) {
            return {"INSTANCE_TRANSFORM", row, ERHIVertexFormat::Float4, slot, kAppendAligned, ERHIInputClassification::PerInstance, 1};
        }
    };
}
```

- [ ] RHIInputElementDesc 構造体
- [ ] ビルダーメソッド

### 3. 入力レイアウト記述

```cpp
namespace NS::RHI
{
    /// 入力レイアウト記述
    struct RHI_API RHIInputLayoutDesc
    {
        /// 入力要素配列
        const RHIInputElementDesc* elements = nullptr;

        /// 要素数
        uint32 elementCount = 0;

        /// 配列から作成
        template<uint32 N>
        static RHIInputLayoutDesc FromArray(const RHIInputElementDesc (&arr)[N]) {
            RHIInputLayoutDesc desc;
            desc.elements = arr;
            desc.elementCount = N;
            return desc;
        }

        /// 検証（オフセット計算含む）。
        bool Validate() const;

        /// 指定スロットのストライド計算
        uint32 CalculateStride(uint32 slot) const;
    };

    /// 入力レイアウトビルダー
    class RHI_API RHIInputLayoutBuilder
    {
    public:
        RHIInputLayoutBuilder() = default;

        /// 要素追加
        RHIInputLayoutBuilder& Add(const RHIInputElementDesc& element) {
            if (m_count < kMaxInputElements) {
                m_elements[m_count++] = element;
            }
            return *this;
        }

        /// 位置追加
        RHIInputLayoutBuilder& Position(uint32 slot = 0) {
            return Add(RHIInputElementDesc::Position(slot));
        }

        /// 法線追加
        RHIInputLayoutBuilder& Normal(uint32 slot = 0) {
            return Add(RHIInputElementDesc::Normal(slot));
        }

        /// タンジェント追加
        RHIInputLayoutBuilder& Tangent(uint32 slot = 0) {
            return Add(RHIInputElementDesc::Tangent(slot));
        }

        /// テクスチャ座標追加
        RHIInputLayoutBuilder& TexCoord(uint32 index = 0, uint32 slot = 0) {
            return Add(RHIInputElementDesc::TexCoord(index, slot));
        }

        /// カラー追加
        RHIInputLayoutBuilder& Color(uint32 slot = 0) {
            return Add(RHIInputElementDesc::Color(slot));
        }

        /// ブレンドインデックス追加
        RHIInputLayoutBuilder& BlendIndices(uint32 slot = 0) {
            return Add(RHIInputElementDesc::BlendIndices(slot));
        }

        /// ブレンドウェイト追加
        RHIInputLayoutBuilder& BlendWeights(uint32 slot = 0) {
            return Add(RHIInputElementDesc::BlendWeights(slot));
        }

        /// 記述取得
        RHIInputLayoutDesc Build() const {
            RHIInputLayoutDesc desc;
            desc.elements = m_elements;
            desc.elementCount = m_count;
            return desc;
        }

        /// 要素配列取得
        const RHIInputElementDesc* GetElements() const { return m_elements; }
        uint32 GetElementCount() const { return m_count; }

    private:
        RHIInputElementDesc m_elements[kMaxInputElements];
        uint32 m_count = 0;
    };
}
```

- [ ] RHIInputLayoutDesc 構造体
- [ ] RHIInputLayoutBuilder クラス

### 4. 一般的な頂点レイアウトのリセット

```cpp
namespace NS::RHI
{
    /// 頂点レイアウトのリセット
    namespace RHIVertexLayouts
    {
        /// 位置のみ
        inline RHIInputLayoutDesc PositionOnly() {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        /// 位置 + テクスチャ座標
        inline RHIInputLayoutDesc PositionTexCoord() {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        /// 位置 + 法線+ テクスチャ座標
        inline RHIInputLayoutDesc PositionNormalTexCoord() {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Normal(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        /// 位置 + 法線+ タンジェント+ テクスチャ座標
        inline RHIInputLayoutDesc PositionNormalTangentTexCoord() {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Normal(),
                RHIInputElementDesc::Tangent(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        /// スキニング用（位置 + 法線+ ブレンド+ テクスチャ座標）
        inline RHIInputLayoutDesc Skinned() {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Normal(),
                RHIInputElementDesc::BlendIndices(),
                RHIInputElementDesc::BlendWeights(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }

        /// UI用（位置 + カラー + テクスチャ座標）
        inline RHIInputLayoutDesc UI() {
            static RHIInputElementDesc elements[] = {
                RHIInputElementDesc::Position(),
                RHIInputElementDesc::Color(),
                RHIInputElementDesc::TexCoord(),
            };
            return RHIInputLayoutDesc::FromArray(elements);
        }
    }
}
```

- [ ] RHIVertexLayouts プリセット

### 5. 入力レイアウトオブジェクト

```cpp
namespace NS::RHI
{
    /// 入力レイアウトオブジェクト
    /// コンパイル済み入力レイアウト
    class RHI_API IRHIInputLayout : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(InputLayout)

        virtual ~IRHIInputLayout() = default;

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 要素数取得
        virtual uint32 GetElementCount() const = 0;

        /// 要素取得
        virtual bool GetElement(uint32 index, RHIInputElementDesc& outElement) const = 0;

        /// 指定スロットのストライド取得
        virtual uint32 GetStride(uint32 slot) const = 0;
    };

    using RHIInputLayoutRef = TRefCountPtr<IRHIInputLayout>;

    /// 入力レイアウト作成（RHIDeviceに追加）。
    ///
    /// 作成時検証（デバッグビルドのみ）:
    /// 実装はvertexShaderBytecodeからリフレクション（IRHIShaderReflection）を取得し、
    /// RHIInputSignature::FindBySemantic()で各入力エレメントのセマンティクスが
    /// VS入力シグネチャに存在するか照合する。不一致時はRHI_ASSERTでフェイルファスト。
    /// これによりVSが使用しないセマンティクスの指定や、
    /// VSが要求するセマンティクスの欠落を早期検出する。
    class IRHIDevice
    {
    public:
        /// 入力レイアウト作成
        virtual IRHIInputLayout* CreateInputLayout(
            const RHIInputLayoutDesc& desc,
            const RHIShaderBytecode& vertexShaderBytecode,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] IRHIInputLayout インターフェース
- [ ] CreateInputLayout

## 検証方法

- [ ] 入力レイアウトの整合性
- [ ] プリセットの動作確認
- [ ] ビルダーの動作確認
- [ ] シェーダーとの入力マッチング

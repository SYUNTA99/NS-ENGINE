# 24-01: UniformBufferレイアウト

## 概要

シェーダー定数バッファのメモリレイアウトを定義・管理する。
C++構造体とHLSL cbufferの互換性を保証、

## ファイル

- `Source/Engine/RHI/Public/RHIUniformBufferLayout.h`

## 依存

- 01-02-types-gpu.md (基本型
- 06-03-shader-reflection.md (リフレクション)

## 定義

```cpp
namespace NS::RHI
{

// ════════════════════════════════════════════════════════════════
// レイアウト要素型
// ════════════════════════════════════════════════════════════════

/// Uniform要素の基本型
enum class ERHIUniformType : uint8
{
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Float3x3,
    Float4x4,
    Float4x3,   ///< 4x3行列（3列、転置）。
    Bool,
    Struct,     ///< ネストした構造体
};

/// Uniform要素情報
struct RHIUniformElement
{
    const char* name = nullptr;         ///< 要素名
    ERHIUniformType type = ERHIUniformType::Float;
    uint32 offset = 0;                  ///< バッファ内のオフセット
    uint32 size = 0;                    ///< サイズ（バイト）
    uint32 arrayCount = 1;              ///< 配列要素数（1=非配列）
    uint32 arrayStride = 0;             ///< 配列ストライド
};

// ════════════════════════════════════════════════════════════════
// レイアウト定義
// ════════════════════════════════════════════════════════════════

/// UniformBufferレイアウト
/// 一度構築したら不変
class RHIUniformBufferLayout
{
public:
    RHIUniformBufferLayout() = default;

    /// 要素数取得
    uint32 GetElementCount() const { return static_cast<uint32>(m_elements.size()); }

    /// 要素取得
    const RHIUniformElement& GetElement(uint32 index) const { return m_elements[index]; }

    /// 名前で要素検索
    const RHIUniformElement* FindElement(const char* name) const;

    /// 総サイズ取得（16バイトアライン済み）。
    uint32 GetSize() const { return m_size; }

    /// ハッシュ取得（同一性チェック用）。
    uint64 GetHash() const { return m_hash; }

    /// デバッグ名
    const char* GetDebugName() const { return m_debugName; }

private:
    friend class RHIUniformBufferLayoutBuilder;

    std::vector<RHIUniformElement> m_elements;
    uint32 m_size = 0;
    uint64 m_hash = 0;
    const char* m_debugName = nullptr;
};

using RHIUniformBufferLayoutRef = std::shared_ptr<const RHIUniformBufferLayout>;

// ════════════════════════════════════════════════════════════════
// レイアウトビルダー
// ════════════════════════════════════════════════════════════════

/// UniformBufferレイアウトビルダー
/// HLSL cbufferルールに従ってオフセットをの動計算
class RHIUniformBufferLayoutBuilder
{
public:
    RHIUniformBufferLayoutBuilder& SetDebugName(const char* name)
    {
        m_debugName = name;
        return *this;
    }

    /// float追加
    RHIUniformBufferLayoutBuilder& AddFloat(const char* name)
    {
        return AddElement(name, ERHIUniformType::Float, 4, 4);
    }

    /// float2追加
    RHIUniformBufferLayoutBuilder& AddFloat2(const char* name)
    {
        return AddElement(name, ERHIUniformType::Float2, 8, 8);
    }

    /// float3追加
    RHIUniformBufferLayoutBuilder& AddFloat3(const char* name)
    {
        return AddElement(name, ERHIUniformType::Float3, 12, 16);
    }

    /// float4追加
    RHIUniformBufferLayoutBuilder& AddFloat4(const char* name)
    {
        return AddElement(name, ERHIUniformType::Float4, 16, 16);
    }

    /// int追加
    RHIUniformBufferLayoutBuilder& AddInt(const char* name)
    {
        return AddElement(name, ERHIUniformType::Int, 4, 4);
    }

    /// uint追加
    RHIUniformBufferLayoutBuilder& AddUInt(const char* name)
    {
        return AddElement(name, ERHIUniformType::UInt, 4, 4);
    }

    /// float4x4追加
    RHIUniformBufferLayoutBuilder& AddFloat4x4(const char* name)
    {
        return AddElement(name, ERHIUniformType::Float4x4, 64, 16);
    }

    /// float3x3追加（12バイト/行）
    RHIUniformBufferLayoutBuilder& AddFloat3x3(const char* name)
    {
        return AddElement(name, ERHIUniformType::Float3x3, 48, 16);
    }

    /// float4配列追加
    RHIUniformBufferLayoutBuilder& AddFloat4Array(const char* name, uint32 count)
    {
        return AddArrayElement(name, ERHIUniformType::Float4, 16, 16, count);
    }

    /// float4x4配列追加
    RHIUniformBufferLayoutBuilder& AddFloat4x4Array(const char* name, uint32 count)
    {
        return AddArrayElement(name, ERHIUniformType::Float4x4, 64, 16, count);
    }

    /// レイアウト構築
    RHIUniformBufferLayoutRef Build();

private:
    RHIUniformBufferLayoutBuilder& AddElement(
        const char* name,
        ERHIUniformType type,
        uint32 size,
        uint32 alignment);

    RHIUniformBufferLayoutBuilder& AddArrayElement(
        const char* name,
        ERHIUniformType type,
        uint32 elementSize,
        uint32 alignment,
        uint32 count);

    uint32 AlignOffset(uint32 offset, uint32 alignment) const
    {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    std::vector<RHIUniformElement> m_elements;
    uint32 m_currentOffset = 0;
    const char* m_debugName = nullptr;
};

// ════════════════════════════════════════════════════════════════
// 型安全なUniformBuffer
// ════════════════════════════════════════════════════════════════

/// 型付きUniformBuffer
/// C++構造体とGPUレイアウトの一致を保証
template<typename T>
class TRHIUniformBuffer
{
public:
    static_assert(sizeof(T) % 16 == 0, "Uniform buffer size must be 16-byte aligned");

    explicit TRHIUniformBuffer(RHIUniformBufferLayoutRef layout)
        : m_layout(std::move(layout))
    {
        NS_ASSERT(m_layout->GetSize() == sizeof(T),
            "Layout size mismatch: expected %u, got %u",
            sizeof(T), m_layout->GetSize());
    }

    /// データ設定
    void SetData(const T& data) { m_data = data; }

    /// データ取得
    const T& GetData() const { return m_data; }
    T& GetData() { return m_data; }

    /// 生データポインタ
    const void* GetRawData() const { return &m_data; }

    /// サイズ
    uint32 GetSize() const { return sizeof(T); }

    /// レイアウト
    const RHIUniformBufferLayout* GetLayout() const { return m_layout.get(); }

private:
    T m_data{};
    RHIUniformBufferLayoutRef m_layout;
};

// ════════════════════════════════════════════════════════════════
// 標準レイアウト定義マクロ
// ════════════════════════════════════════════════════════════════

/// UniformBuffer構造体宣言マクロ
/// HLSL cbufferと同じレイアウトを保証
#define RHI_UNIFORM_BUFFER_BEGIN(Name) \
    struct alignas(16) Name {

#define RHI_UNIFORM_BUFFER_END() \
    };

/// パディング挿入用
#define RHI_UNIFORM_PAD(bytes) \
    uint8 NS_MACRO_CONCATENATE(_pad_, __LINE__)[bytes]

} // namespace NS::RHI
```

## 使用例

```cpp
// C++側定義
RHI_UNIFORM_BUFFER_BEGIN(ViewConstants)
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix;
    float4 cameraPosition;      // xyz=位置, w=unused
    float4 screenSize;          // xy=解像度, zw=1/解像度
    float nearPlane;
    float farPlane;
    float time;
    float deltaTime;
RHI_UNIFORM_BUFFER_END()

// レイアウト構築
auto viewConstantsLayout = RHIUniformBufferLayoutBuilder()
    .SetDebugName("ViewConstants")
    .AddFloat4x4("viewMatrix")
    .AddFloat4x4("projMatrix")
    .AddFloat4x4("viewProjMatrix")
    .AddFloat4("cameraPosition")
    .AddFloat4("screenSize")
    .AddFloat("nearPlane")
    .AddFloat("farPlane")
    .AddFloat("time")
    .AddFloat("deltaTime")
    .Build();

// 型付きバッファ
TRHIUniformBuffer<ViewConstants> viewBuffer(viewConstantsLayout);
viewBuffer.GetData().viewMatrix = camera.GetViewMatrix();
viewBuffer.GetData().time = totalTime;

// GPUへアップロード
context->UpdateBuffer(gpuBuffer, viewBuffer.GetRawData(), viewBuffer.GetSize());
```

## HLSL側対応

```hlsl
// HLSLでの対応定義
cbuffer ViewConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 viewProjMatrix;
    float4 cameraPosition;
    float4 screenSize;
    float nearPlane;
    float farPlane;
    float time;
    float deltaTime;
};
```

## 検証

- [ ] HLSLパッキングルールとの一致
- [ ] 16バイトアライメント
- [ ] 配列ストライド
- [ ] ネスト構造体サポート

# 20-01: ステージングバッファ

## 概要

GPU↔CPU間のデータ転送に使用する中間バッファ。アップロードとリードバック両方向をサポート、

## ファイル

- `Source/Engine/RHI/Public/RHIStagingBuffer.h`

## 依存

- 03-02-buffer-interface.md (IRHIBuffer)
- 11-01-heap-types.md (ERHIHeapType)

## 定義

```cpp
namespace NS::RHI
{

/// ステージングバッファの用途
enum class ERHIStagingUsage : uint8
{
    Upload,     ///< CPU→GPU転送用（UPLOAD heap）。
    Readback    ///< GPU→CPU転送用（READBACK heap）。
};

/// ステージングバッファ記述
struct RHIStagingBufferDesc
{
    uint64 size = 0;
    ERHIStagingUsage usage = ERHIStagingUsage::Upload;
    const char* debugName = nullptr;
};

/// ステージングバッファインターフェース
/// CPU可視メモリに配置され、GPU転送のソース/デスティネーションとして使用
class IRHIStagingBuffer : public IRHIResource
{
public:
    virtual ~IRHIStagingBuffer() = default;

    // ────────────────────────────────────────────────────────────
    // プロパティ
    // ────────────────────────────────────────────────────────────

    /// バッファサイズ取得
    virtual uint64 GetSize() const = 0;

    /// 用途取得
    virtual ERHIStagingUsage GetUsage() const = 0;

    // ────────────────────────────────────────────────────────────
    // マッピング
    // ────────────────────────────────────────────────────────────

    /// CPUアドレスへマップ（永続マップ可能）。
    /// @param offset マップ開始オフセット
    /// @param size マップサイズ（=全体）
    /// @return マップされたポインタ、失敗時nullptr
    virtual void* Map(uint64 offset = 0, uint64 size = 0) = 0;

    /// マップ解除
    virtual void Unmap() = 0;

    /// マップ済みかどうか
    virtual bool IsMapped() const = 0;

    /// マップ済みポインタ取得（Map済みの場合のみ有効）。
    virtual void* GetMappedPointer() const = 0;

    // ────────────────────────────────────────────────────────────
    // データ操作（ヘルパー）。
    // ────────────────────────────────────────────────────────────

    /// データ書き込み（Upload用）。
    /// 自動的にMap/Unmapを行う
    virtual NS::Result WriteData(const void* data, uint64 size, uint64 offset = 0) = 0;

    /// データ読み取り（Readback用）。
    /// 自動的にMap/Unmapを行う
    virtual NS::Result ReadData(void* outData, uint64 size, uint64 offset = 0) = 0;
};

using RHIStagingBufferRef = TRefCountPtr<IRHIStagingBuffer>;

// ────────────────────────────────────────────────────────────────
// スコープヘルパー。
// ────────────────────────────────────────────────────────────────

/// RAII スコープマップ。
class RHIScopedStagingMap
{
public:
    RHIScopedStagingMap(IRHIStagingBuffer* buffer, uint64 offset = 0, uint64 size = 0)
        : m_buffer(buffer)
        , m_pointer(buffer ? buffer->Map(offset, size) : nullptr)
    {
    }

    ~RHIScopedStagingMap()
    {
        if (m_buffer && m_pointer)
        {
            m_buffer->Unmap();
        }
    }

    NS_DISALLOW_COPY(RHIScopedStagingMap);

public:
    void* Get() const { return m_pointer; }
    bool IsValid() const { return m_pointer != nullptr; }

    template<typename T>
    T* As() const { return static_cast<T*>(m_pointer); }

private:
    IRHIStagingBuffer* m_buffer;
    void* m_pointer;
};

// ────────────────────────────────────────────────────────────────
// ビルダー
// ────────────────────────────────────────────────────────────────

class RHIStagingBufferDescBuilder
{
public:
    RHIStagingBufferDescBuilder& Size(uint64 size)
    {
        m_desc.size = size;
        return *this;
    }

    RHIStagingBufferDescBuilder& ForUpload()
    {
        m_desc.usage = ERHIStagingUsage::Upload;
        return *this;
    }

    RHIStagingBufferDescBuilder& ForReadback()
    {
        m_desc.usage = ERHIStagingUsage::Readback;
        return *this;
    }

    RHIStagingBufferDescBuilder& DebugName(const char* name)
    {
        m_desc.debugName = name;
        return *this;
    }

    const RHIStagingBufferDesc& Build() const { return m_desc; }

private:
    RHIStagingBufferDesc m_desc;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// アップロード用
auto uploadBuffer = device->CreateStagingBuffer(
    RHIStagingBufferDescBuilder()
        .Size(vertexDataSize)
        .ForUpload()
        .DebugName("VertexUpload")
        .Build()
);

uploadBuffer->WriteData(vertices.data(), vertexDataSize);

// コマンドでコピー
context->CopyBuffer(uploadBuffer, gpuBuffer, vertexDataSize);

// リードバック用
auto readbackBuffer = device->CreateStagingBuffer(
    RHIStagingBufferDescBuilder()
        .Size(sizeof(uint32) * 1024)
        .ForReadback()
        .DebugName("QueryReadback")
        .Build()
);

// GPU→ステージングへコピー後、フェンス待ちしてから読み取り
fence->Wait(signalValue);
uint32 results[1024];
readbackBuffer->ReadData(results, sizeof(results));
```

## プラットフォーム実装アプローチ

| API | Upload実装| Readback実装|
|-----|-----------|--------------|
| D3D12 | D3D12_HEAP_TYPE_UPLOAD | D3D12_HEAP_TYPE_READBACK |
| Vulkan | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT + HOST_CACHED |
| Metal | MTLStorageModeShared | MTLStorageModeShared |

## 検証

- [ ] Upload/Readback両方向のマッピング
- [ ] スコープヘルパーのRAII動作
- [ ] 境界チェック（オフセット+サイズ）。

# 21-02: Indirect Draw

## 概要

GPU駆動の間接描画コマンド。描画パラメータをGPUバッファから読み取り、
CPUを介さずに描画を実行、

## ファイル

- `Source/Engine/RHI/Public/IRHICommandContext.h` (拡張)

## 依存

- 21-01-indirect-arguments.md (引数構造体
- 02-03-graphics-context.md (IRHIGraphicsContext)
- 03-02-buffer-interface.md (IRHIBuffer)

## 定義

```cpp
namespace NS::RHI
{

// IRHIGraphicsContextへの追加メソッド

class IRHIGraphicsContext : public IRHIComputeContext
{
public:
    // ... 既存メソッド ...

    // ════════════════════════════════════════════════════════════════
    // Indirect Draw
    // ════════════════════════════════════════════════════════════════

    /// 間接描画（インデックスなし）
    /// @param argumentBuffer 引数バッファ（RHIDrawArguments配列）。
    /// @param argumentOffset バッファ内のオフセット
    virtual void DrawIndirect(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset = 0) = 0;

    /// 間接描画（インデックス付き）。
    /// @param argumentBuffer 引数バッファ（RHIDrawIndexedArguments配列）。
    /// @param argumentOffset バッファ内のオフセット
    virtual void DrawIndexedIndirect(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset = 0) = 0;

    /// 複数回間接描画（インデックスなし）
    /// @param argumentBuffer 引数バッファ
    /// @param argumentOffset バッファ内のオフセット
    /// @param drawCount 描画コール数
    /// @param stride 引数間のストライド（=sizeof(RHIDrawArguments)）。
    virtual void DrawIndirectMulti(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        uint32 drawCount,
        uint32 stride = 0) = 0;

    /// 複数回間接描画（インデックス付き）。
    /// @param argumentBuffer 引数バッファ
    /// @param argumentOffset バッファ内のオフセット
    /// @param drawCount 描画コール数
    /// @param stride 引数間のストライド（=sizeof(RHIDrawIndexedArguments)）。
    virtual void DrawIndexedIndirectMulti(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        uint32 drawCount,
        uint32 stride = 0) = 0;

    // ════════════════════════════════════════════════════════════════
    // Indirect Draw with Count Buffer
    // ════════════════════════════════════════════════════════════════

    /// カウントバッファ付き間接描画
    /// GPUが描画回数を決定するシナリオ用
    /// @param argumentBuffer 引数バッファ
    /// @param argumentOffset 引数バッファオフセット
    /// @param countBuffer カウントバッファ（uint32）。
    /// @param countOffset カウントバッファオフセット
    /// @param maxDrawCount 最大描画回数
    /// @param stride 引数間のストライド
    /// @note 実際の描画数はmin(countBufferの値, maxDrawCount)にクランプされる。
    ///       この保証はD3D12/Vulkan両方のAPI仕様に基づく。
    ///       Metalバックエンドでは明示的なクランプ実装が必要。
    virtual void DrawIndirectCount(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        IRHIBuffer* countBuffer,
        uint64 countOffset,
        uint32 maxDrawCount,
        uint32 stride = 0) = 0;

    /// カウントバッファ付きインデックス間接描画
    virtual void DrawIndexedIndirectCount(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        IRHIBuffer* countBuffer,
        uint64 countOffset,
        uint32 maxDrawCount,
        uint32 stride = 0) = 0;
};

// ════════════════════════════════════════════════════════════════
// 間接描画バッファビルダー
// ════════════════════════════════════════════════════════════════

/// 間接描画バッファの構築ヘルパー。
class RHIIndirectDrawBufferBuilder
{
public:
    RHIIndirectDrawBufferBuilder& Reserve(uint32 maxDrawCalls)
    {
        m_drawArgs.reserve(maxDrawCalls);
        return *this;
    }

    RHIIndirectDrawBufferBuilder& AddDraw(
        uint32 vertexCount,
        uint32 instanceCount = 1,
        uint32 startVertex = 0,
        uint32 startInstance = 0)
    {
        m_drawArgs.push_back({ vertexCount, instanceCount, startVertex, startInstance });
        return *this;
    }

    uint32 GetDrawCount() const { return static_cast<uint32>(m_drawArgs.size()); }
    uint64 GetRequiredSize() const { return sizeof(RHIDrawArguments) * m_drawArgs.size(); }
    const RHIDrawArguments* GetData() const { return m_drawArgs.data(); }

    void Clear() { m_drawArgs.clear(); }

private:
    std::vector<RHIDrawArguments> m_drawArgs;
};

/// インデックス付き間接描画バッファの構築ヘルパー。
class RHIIndirectDrawIndexedBufferBuilder
{
public:
    RHIIndirectDrawIndexedBufferBuilder& Reserve(uint32 maxDrawCalls)
    {
        m_drawArgs.reserve(maxDrawCalls);
        return *this;
    }

    RHIIndirectDrawIndexedBufferBuilder& AddDraw(
        uint32 indexCount,
        uint32 instanceCount = 1,
        uint32 startIndex = 0,
        int32 baseVertex = 0,
        uint32 startInstance = 0)
    {
        m_drawArgs.push_back({ indexCount, instanceCount, startIndex, baseVertex, startInstance });
        return *this;
    }

    uint32 GetDrawCount() const { return static_cast<uint32>(m_drawArgs.size()); }
    uint64 GetRequiredSize() const { return sizeof(RHIDrawIndexedArguments) * m_drawArgs.size(); }
    const RHIDrawIndexedArguments* GetData() const { return m_drawArgs.data(); }

    void Clear() { m_drawArgs.clear(); }

private:
    std::vector<RHIDrawIndexedArguments> m_drawArgs;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// CPU側で引数バッファ構築
RHIIndirectDrawIndexedBufferBuilder builder;
builder.Reserve(visibleMeshCount);

for (const auto& mesh : culledMeshes)
{
    builder.AddDraw(
        mesh.indexCount,
        mesh.instanceCount,
        mesh.startIndex,
        mesh.baseVertex,
        mesh.startInstance
    );
}

// バッファにアップロード
uploadBuffer->WriteData(builder.GetData(), builder.GetRequiredSize());
context->CopyBuffer(uploadBuffer, indirectBuffer, builder.GetRequiredSize());

// 間接描画実行
context->DrawIndexedIndirectMulti(
    indirectBuffer,
    0,
    builder.GetDrawCount()
);

// GPU駆動カリング後の描画（カウントバッファ使用）。
context->DrawIndexedIndirectCount(
    indirectBuffer,       // GPUカリングシェーダーが書き込んだ引数
    0,
    visibleCountBuffer,   // GPUカリングシェーダーが書き込んだカウント
    0,
    kMaxDrawCalls
);
```

## プラットフォーム実装アプローチ

| API | 単一Indirect | Multi Indirect | Count付き |
|-----|-------------|----------------|----------|
| D3D12 | DrawInstanced/DrawIndexedInstanced | 複数回呼び出し | ExecuteIndirect |
| Vulkan | vkCmdDraw*Indirect | vkCmdDraw*Indirect | vkCmdDraw*IndirectCount |
| Metal | drawPrimitives(indirectBuffer:) | 複数回呼び出し | ICB使用 |

## 検証

- [ ] 単一間接描画の動作
- [ ] Multi描画のストライド計算
- [ ] CountBuffer付き描画のmaxDrawCount制限
- [ ] 空の描画（DrawCount=0の安全性

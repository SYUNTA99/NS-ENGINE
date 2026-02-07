# 21-05: ExecuteIndirect

## 概要

最も柔軟なGPU駆動描画/ディスパッチコマンド、
コマンドシグネチャに基づき、状態変更と描画を一括実行、

## ファイル

- `Source/Engine/RHI/Public/IRHICommandContext.h` (拡張)

## 依存

- 21-04-command-signature.md (IRHICommandSignature)
- 02-03-graphics-context.md (IRHIGraphicsContext)

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
    // ExecuteIndirect
    // ════════════════════════════════════════════════════════════════

    /// ExecuteIndirect
    /// コマンドシグネチャに基づいGPU駆動コマンドを実行
    /// @param commandSignature コマンドシグネチャ
    /// @param maxCommandCount 最大コマンド数
    /// @param argumentBuffer 引数バッファ
    /// @param argumentOffset 引数バッファオフセット
    /// @param countBuffer カウントバッファ（nullptr=maxCommandCount使用）。
    /// @param countOffset カウントバッファオフセット
    virtual void ExecuteIndirect(
        IRHICommandSignature* commandSignature,
        uint32 maxCommandCount,
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        IRHIBuffer* countBuffer = nullptr,
        uint64 countOffset = 0) = 0;
};

// ════════════════════════════════════════════════════════════════
// GPU駆動レンダリングパイプライン
// ════════════════════════════════════════════════════════════════

/// GPU駆動描画バッチ
/// カリング結果に基づく間接描画を管理
class RHIGPUDrivenBatch
{
public:
    struct PerDrawData
    {
        uint32 objectId;
        uint32 materialId;
        uint32 meshletOffset;
        uint32 meshletCount;
    };

    RHIGPUDrivenBatch(IRHIDevice* device, uint32 maxDraws);

    /// 描画データをGPUバッファにアップロード
    void UploadDrawData(const PerDrawData* data, uint32 count);

    /// カリングコンピュートシェーダー実行
    void ExecuteCulling(
        IRHIComputeContext* context,
        IRHIBuffer* visibilityBuffer,
        IRHIBuffer* instanceBuffer);

    /// 間接描画実行
    void ExecuteDraws(
        IRHIGraphicsContext* context,
        IRHICommandSignature* signature);

    /// 引数バッファ取得
    IRHIBuffer* GetArgumentBuffer() const { return m_argumentBuffer.Get(); }

    /// カウントバッファ取得
    IRHIBuffer* GetCountBuffer() const { return m_countBuffer.Get(); }

private:
    RHIBufferRef m_drawDataBuffer;      // PerDrawData配列
    RHIBufferRef m_argumentBuffer;       // Indirect引数
    RHIBufferRef m_countBuffer;          // 実際の描画数
    uint32 m_maxDraws;
};

// ════════════════════════════════════════════════════════════════
// メッシュレットカリング統合
// ════════════════════════════════════════════════════════════════

/// メッシュレットベースGPU駆動レンダリング
class RHIMeshletGPURenderer
{
public:
    struct MeshletBatch
    {
        uint32 meshletBufferOffset;
        uint32 meshletCount;
        uint32 materialId;
    };

    RHIMeshletGPURenderer(IRHIDevice* device, uint32 maxMeshlets);

    /// フラスタムカリング + オクルージョンカリング
    void ExecuteTwoPassCulling(
        IRHIComputeContext* context,
        const float4x4& viewProj,
        IRHITexture* hierarchicalZ);

    /// 可視メッシュレットを描画
    void ExecuteDraws(IRHIGraphicsContext* context);

    /// 統計取得
    uint32 GetVisibleMeshletCount() const;

private:
    RHIBufferRef m_meshletBuffer;
    RHIBufferRef m_visibleMeshletBuffer;
    RHIBufferRef m_indirectArgsBuffer;
    RHIBufferRef m_statsBuffer;
    RHICommandSignatureRef m_dispatchMeshSignature;
    uint32 m_maxMeshlets;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// GPU駆動シーン描画
void RenderScene(IRHIGraphicsContext* context)
{
    // 1. オブジェクトカリング（コンピュート）
    context->SetPSO(frustumCullPSO);
    context->SetComputeRootDescriptorTable(0, objectDataSRV);
    context->SetComputeRootDescriptorTable(1, visibilityUAV);
    context->Dispatch(objectGroups, 1, 1);

    // 2. Indirect引数生成（コンピュート）
    context->SetPSO(buildIndirectArgsPSO);
    context->Dispatch(1, 1, 1);

    // バリア（引数バッファとカウントバッファ両方）
    context->BufferBarrier(indirectArgsBuffer,
        ERHIResourceState::UnorderedAccess,
        ERHIResourceState::IndirectArgument);
    context->BufferBarrier(countBuffer,
        ERHIResourceState::UnorderedAccess,
        ERHIResourceState::IndirectArgument);

    // 3. ExecuteIndirect描画
    context->SetPSO(mainPassPSO);
    context->SetGraphicsRootSignature(mainRootSig);
    context->ExecuteIndirect(
        drawIndexedSignature,   // DrawIndexed + 定数変更
        kMaxObjects,
        indirectArgsBuffer,
        0,
        countBuffer,            // GPUが決定した描画数
        0
    );
}
```

## プラットフォーム実装アプローチ

| API | 最大コマンド数 | カウントバッファ | 備考|
|-----|--------------|----------------|------|
| D3D12 | 無制限| サポート| ID3D12GraphicsCommandList::ExecuteIndirect |
| Vulkan | デバイス依存| VK_KHR_draw_indirect_count | 拡張応|
| Metal | ICB制限| サポート| Indirect Command Buffer |

## 検証

- [ ] コマンドシグネチャとの整合性
- [ ] カウントバッファ動作
- [ ] 状態変更（B/IB/CBV等）の正確性
- [ ] maxCommandCount=0の安全性

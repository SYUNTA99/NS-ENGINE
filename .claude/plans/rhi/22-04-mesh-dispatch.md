# 22-04: メッシュディスパッチ

## 概要

メッシュシェーダーのディスパッチコマンドと関連ヘルパー、
グループ数計算、Indirect対応、パフォーマンス最適化、

## ファイル

- `Source/Engine/RHI/Public/RHIMeshDispatch.h`

## 依存

- 22-01-mesh-shader.md (IRHIMeshShader)
- 22-03-mesh-pso.md (IRHIMeshPipelineState)
- 21-01-indirect-arguments.md (Indirect引数)

## 定義

```cpp
namespace NS::RHI
{

/// メッシュディスパッチ引数（Direct）。
struct RHIMeshDispatchArgs
{
    uint32 groupCountX = 1;
    uint32 groupCountY = 1;
    uint32 groupCountZ = 1;

    static RHIMeshDispatchArgs FromMeshletCount(uint32 meshletCount, uint32 meshletsPerGroup = 1)
    {
        return { (meshletCount + meshletsPerGroup - 1) / meshletsPerGroup, 1, 1 };
    }
};

/// メッシュディスパッチ制限
struct RHIMeshDispatchLimits
{
    uint32 maxGroupCountX = 65535;
    uint32 maxGroupCountY = 65535;
    uint32 maxGroupCountZ = 65535;
    uint32 maxTotalGroups = 1 << 22;  ///< 約400万
};

// IRHIGraphicsContextへの追加

class IRHIGraphicsContext
{
public:
    // ... 既存メソッド ...

    // ════════════════════════════════════════════════════════════════
    // メッシュシェーダーディスパッチ
    // ════════════════════════════════════════════════════════════════

    /// 直接ディスパッチ
    virtual void DispatchMesh(uint32 groupX, uint32 groupY = 1, uint32 groupZ = 1) = 0;

    /// 引数構造体からディスパッチ
    void DispatchMesh(const RHIMeshDispatchArgs& args)
    {
        DispatchMesh(args.groupCountX, args.groupCountY, args.groupCountZ);
    }

    /// Indirectディスパッチ
    virtual void DispatchMeshIndirect(IRHIBuffer* argumentBuffer, uint64 offset = 0) = 0;

    /// カウントバッファ付きIndirectディスパッチ
    virtual void DispatchMeshIndirectCount(
        IRHIBuffer* argumentBuffer,
        uint64 argumentOffset,
        IRHIBuffer* countBuffer,
        uint64 countOffset,
        uint32 maxDispatchCount) = 0;
};

// ════════════════════════════════════════════════════════════════
// メッシュレット描画マネージャー
// ════════════════════════════════════════════════════════════════

/// メッシュレットバッチ
struct RHIMeshletBatch
{
    uint32 meshletOffset;       ///< メッシュレット配列内のオフセット
    uint32 meshletCount;        ///< メッシュレット数
    uint32 materialId;          ///< マテリアルID
    uint32 objectId;            ///< オブジェクトID（インスタンスデータ用）。
};

/// メッシュレット描画マネージャー
/// バッチング、ソート、Indirect引数生成を管理
class RHIMeshletDrawManager
{
public:
    explicit RHIMeshletDrawManager(IRHIDevice* device, uint32 maxBatches);

    /// バッチ追加
    void AddBatch(const RHIMeshletBatch& batch);

    /// マテリアルIDでソート
    void SortByMaterial();

    /// フラスタムカリング（GPU）
    void CullBatches(const float4 frustumPlanes[6]);

    /// Indirect引数バッファを構築
    void BuildIndirectBuffer(IRHICommandContext* context);

    /// すべてのバッチを描画
    void DrawAll(IRHIGraphicsContext* context, IRHIMeshPipelineState* pso);

    /// マテリアル別に描画（PSO切り替え）
    void DrawByMaterial(
        IRHIGraphicsContext* context,
        const std::function<IRHIMeshPipelineState*(uint32 materialId)>& psoGetter);

    /// 統計
    uint32 GetTotalMeshlets() const;
    uint32 GetVisibleMeshlets() const;
    uint32 GetBatchCount() const;

    /// クリア
    void Clear();

private:
    std::vector<RHIMeshletBatch> m_batches;
    RHIBufferRef m_indirectBuffer;
    RHIBufferRef m_countBuffer;
    uint32 m_maxBatches;
    uint32 m_visibleMeshlets = 0;
};

// ════════════════════════════════════════════════════════════════
// パフォーマンス最適化ヘルパー。
// ════════════════════════════════════════════════════════════════

/// メッシュシェーダー最適化ヒント
namespace RHIMeshShaderOptimization
{
    /// 推奨メッシュレットサイズ
    constexpr uint32 kRecommendedMeshletVertices = 64;
    constexpr uint32 kRecommendedMeshletTriangles = 126;  // 128 - 2 for alignment

    /// グループサイズ計算
    /// @param meshletCount 総メッシュレット数
    /// @param meshletsPerGroup グループあたりメッシュレット数
    inline RHIMeshDispatchArgs CalculateDispatchArgs(
        uint32 meshletCount,
        uint32 meshletsPerGroup = 1)
    {
        return RHIMeshDispatchArgs::FromMeshletCount(meshletCount, meshletsPerGroup);
    }

    /// 2Dグリッド（大量メッシュレット用）。
    inline RHIMeshDispatchArgs CalculateDispatchArgs2D(
        uint32 meshletCount,
        uint32 maxGroupX = 65535)
    {
        uint32 groupX = std::min(meshletCount, maxGroupX);
        uint32 groupY = (meshletCount + maxGroupX - 1) / maxGroupX;
        return { groupX, groupY, 1 };
    }

    /// Waveサイズに合わせたグループサイズ推奨
    inline uint32 GetRecommendedThreadGroupSize(uint32 waveSize)
    {
        // 通常は128（NVIDIA/AMD両方で効率的）。
        return std::max(128u, waveSize);
    }
}

} // namespace NS::RHI
```

## 使用例

```cpp
// 直接描画
context->SetMeshPipelineState(meshPSO);
context->DispatchMesh(
    RHIMeshDispatchArgs::FromMeshletCount(totalMeshlets)
);

// GPU駆動描画（Indirect）。
// 1. カリングコンピュートシェーダーで可視メッシュレットを決定
context->SetPSO(cullingPSO);
context->Dispatch(objectGroups, 1, 1);

// 2. Indirect引数生成
context->SetPSO(buildArgsPSO);
context->Dispatch(1, 1, 1);

// 3. メッシュシェーダー描画
context->BufferBarrier(indirectArgs, ERHIResourceState::UAV, ERHIResourceState::IndirectArgument);
context->SetMeshPipelineState(meshPSO);
context->DispatchMeshIndirectCount(
    indirectArgsBuffer, 0,
    countBuffer, 0,
    kMaxMeshletGroups
);

// マネージャー使用
RHIMeshletDrawManager drawManager(device, 10000);

for (const auto& mesh : meshes)
{
    drawManager.AddBatch({
        mesh.meshletOffset,
        mesh.meshletCount,
        mesh.materialId,
        mesh.objectId
    });
}

drawManager.SortByMaterial();
drawManager.CullBatches(frustumPlanes);
drawManager.BuildIndirectBuffer(context);
drawManager.DrawByMaterial(context, [](uint32 matId) {
    return materialPSOs[matId];
});
```

## 検証

- [ ] グループ数計算の正確性
- [ ] Indirect描画動作
- [ ] バッチのマネージャーのソート
- [ ] パフォーマンス最適化効果

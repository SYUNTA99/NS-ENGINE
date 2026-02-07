# 23-04: エイリアシングバリア

## 概要

同じ物理メモリを共有するリソース間の遷移を管理する。
Transientリソースのメモリ再利用を可能にする。

## ファイル

- `Source/Engine/RHICore/Public/RHIAliasingBarrier.h`

## 依存

- 16-02-barrier.md (バリア基盤)
- 23-01-transient-allocator.md (Transientアロケーター)

## 定義

```cpp
namespace NS::RHI
{

/// エイリアシングバリア記述
struct RHIAliasingBarrier
{
    IRHIResource* resourceBefore = nullptr; ///< 使用終了するリソース（nullptrで任意）
    IRHIResource* resourceAfter = nullptr;  ///< 使用開始するリソース（nullptrで任意）
};

/// エイリアシングバリアバッチ
class RHIAliasingBarrierBatch
{
public:
    /// バリア追加
    void Add(IRHIResource* before, IRHIResource* after)
    {
        m_barriers.push_back({ before, after });
    }

    /// 破棄（リソース使用終了）。
    void AddDiscard(IRHIResource* resource)
    {
        m_barriers.push_back({ resource, nullptr });
    }

    /// 取得バリア追加（リソース使用開始）
    void AddAcquire(IRHIResource* resource)
    {
        m_barriers.push_back({ nullptr, resource });
    }

    /// クリア
    void Clear() { m_barriers.clear(); }

    /// 空かどうか
    bool IsEmpty() const { return m_barriers.empty(); }

    /// バリア数
    uint32 GetCount() const { return static_cast<uint32>(m_barriers.size()); }

    /// データ取得
    const RHIAliasingBarrier* GetData() const { return m_barriers.data(); }

private:
    std::vector<RHIAliasingBarrier> m_barriers;
};

// IRHICommandContextへの追加

class IRHICommandContext
{
public:
    // ... 既存メソッド ...

    /// エイリアシングバリア発行
    /// @param barriers バリア配列
    /// @param count バリア数
    virtual void AliasingBarrier(
        const RHIAliasingBarrier* barriers,
        uint32 count) = 0;

    /// エイリアシングバリアバッチ発行
    void AliasingBarrier(const RHIAliasingBarrierBatch& batch)
    {
        if (!batch.IsEmpty())
        {
            AliasingBarrier(batch.GetData(), batch.GetCount());
        }
    }
};

// ════════════════════════════════════════════════════════════════
// 自動エイリアシング管理
// ════════════════════════════════════════════════════════════════

/// エイリアシンググループ
/// 同じ物理メモリを共有するリソースの集合
class RHIAliasingGroup
{
public:
    explicit RHIAliasingGroup(uint64 heapOffset, uint64 size)
        : m_heapOffset(heapOffset)
        , m_size(size)
    {
    }

    /// リソースをグループに追加
    void AddResource(IRHIResource* resource, uint32 firstPass, uint32 lastPass)
    {
        m_resources.push_back({ resource, firstPass, lastPass });
    }

    /// 指定パスで必要なバリアを生成
    /// @note 前のリソースがUAVとして使用されていた場合、
    ///       AliasingBarrierの前にUAVバリアが必要。
    ///       AcquireResources内で自動的に発行される。
    void GenerateBarriers(uint32 passIndex, RHIAliasingBarrierBatch& outBarriers) const
    {
        if (passIndex == 0) return; // 最初のパスではエイリアシング不要

        std::vector<IRHIResource*> resourcesBefore;
        IRHIResource* resourceAfter = nullptr;

        for (const auto& entry : m_resources)
        {
            // 前のパスで終了したリソース（複数の場合あり）
            if (entry.lastPass == passIndex - 1)
            {
                resourcesBefore.push_back(entry.resource);
            }
            // このパスで開始するリソース
            if (entry.firstPass == passIndex)
            {
                resourceAfter = entry.resource;
            }
        }

        if (resourceAfter)
        {
            if (resourcesBefore.empty())
            {
                outBarriers.AddAcquire(resourceAfter);
            }
            else
            {
                for (IRHIResource* before : resourcesBefore)
                {
                    outBarriers.Add(before, resourceAfter);
                }
            }
        }
    }

    /// メモリ領域取得
    uint64 GetHeapOffset() const { return m_heapOffset; }
    uint64 GetSize() const { return m_size; }

private:
    struct ResourceEntry
    {
        IRHIResource* resource;
        uint32 firstPass;
        uint32 lastPass;
    };

    uint64 m_heapOffset;
    uint64 m_size;
    std::vector<ResourceEntry> m_resources;
};

/// エイリアシングマネージャー
/// フレームグラフと連携してエイリアシングを最適化
class RHIAliasingManager
{
public:
    /// リソースを登録
    void RegisterResource(
        IRHIResource* resource,
        uint64 heapOffset,
        uint64 size,
        uint32 firstPass,
        uint32 lastPass);

    /// エイリアシング解析を実行
    void Analyze();

    /// 指定パスのバリアを生成
    void GenerateBarriersForPass(uint32 passIndex, RHIAliasingBarrierBatch& outBarriers) const;

    /// エイリアシングで節約されたメモリ量
    uint64 GetMemorySaved() const { return m_memorySaved; }

    /// リセット
    void Reset();

private:
    std::vector<RHIAliasingGroup> m_groups;
    uint64 m_memorySaved = 0;
};

} // namespace NS::RHI
```

## 使用例

```cpp
// フレームグラフ実行時のエイリアシングバリア

void ExecuteFrameGraph(IRHICommandContext* context)
{
    RHIAliasingManager aliasingManager;

    // リソース登録（フレームグラフ構築時）。
    aliasingManager.RegisterResource(shadowMap, 0, shadowMapSize, 0, 0);
    aliasingManager.RegisterResource(ssaoBuffer, 0, ssaoSize, 2, 3);  // 同じオフセット
    aliasingManager.Analyze();

    // パス実行
    for (uint32 passIndex = 0; passIndex < passCount; ++passIndex)
    {
        // エイリアシングバリア
        RHIAliasingBarrierBatch barriers;
        aliasingManager.GenerateBarriersForPass(passIndex, barriers);
        context->AliasingBarrier(barriers);

        // パス実行
        ExecutePass(context, passIndex);
    }

    NS_LOG_INFO("Aliasing saved %llu MB", aliasingManager.GetMemorySaved() / (1024 * 1024));
}
```

## プラットフォーム実装

| API | 実装|
|-----|------|
| D3D12 | ID3D12GraphicsCommandList::ResourceBarrier (D3D12_RESOURCE_BARRIER_TYPE_ALIASING) |
| Vulkan | vkCmdPipelineBarrier + メモリアライアシング |
| Metal | MTLRenderCommandEncoder内の自動管理|

## 検証

- [ ] バリア生成の正確性
- [ ] オーバーラップの検出
- [ ] メモリ節約量計算
- [ ] NULL before/afterの処理

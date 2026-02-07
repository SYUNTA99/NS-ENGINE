# 25-02: メモリ統計

## 概要

GPUメモリ使用状況の追跡・レポート、
リソースタイプ別、ヒープ別の使用量を可視化、

## ファイル

- `Source/Engine/RHI/Public/RHIMemoryStats.h`

## 依存

- 11-01-heap-types.md (ヒープタイプ
- 01-12-resource-base.md (IRHIResource)

## 定義

```cpp
namespace NS::RHI
{

/// リソースカテゴリ
enum class ERHIResourceCategory : uint8
{
    Buffer,
    Texture,
    RenderTarget,
    DepthStencil,
    Shader,
    PipelineState,
    QueryHeap,
    AccelerationStructure,
    Descriptor,
    Staging,
    Other,
    Count
};

/// カテゴリ別メモリ統計
struct RHICategoryMemoryStats
{
    uint64 allocatedBytes = 0;      ///< 割り当て済みバイト数
    uint64 usedBytes = 0;           ///< 実使用バイト数（パディング除く）
    uint32 resourceCount = 0;       ///< リソース数
    uint32 allocationCount = 0;     ///< アロケーション数

    float GetUtilization() const
    {
        return allocatedBytes > 0
            ? static_cast<float>(usedBytes) / allocatedBytes
            : 0.0f;
    }
};

/// ヒープ別メモリ統計
struct RHIHeapMemoryStats
{
    uint64 totalSize = 0;           ///< ヒープ総サイズ
    uint64 usedSize = 0;            ///< 使用サイズ
    uint64 peakUsedSize = 0;        ///< ピーク使用サイズ
    uint32 allocationCount = 0;     ///< アロケーション数
    uint32 fragmentationCount = 0;  ///< フラグメンテーション数

    float GetUsagePercent() const
    {
        return totalSize > 0
            ? static_cast<float>(usedSize) / totalSize * 100.0f
            : 0.0f;
    }
};

/// GPUメモリ統計（統合）
struct RHIMemoryStats
{
    // カテゴリ別
    std::array<RHICategoryMemoryStats, static_cast<size_t>(ERHIResourceCategory::Count)>
        categoryStats;

    // ヒープ別
    RHIHeapMemoryStats defaultHeap;     ///< GPU専用メモリ
    RHIHeapMemoryStats uploadHeap;      ///< アップロードヒープ
    RHIHeapMemoryStats readbackHeap;    ///< リードバックヒープ

    // 総計
    uint64 totalAllocatedBytes = 0;
    uint64 totalUsedBytes = 0;
    uint64 budgetBytes = 0;             ///< OSが許可するメモリ量
    uint64 availableBytes = 0;          ///< 利用可能なメモリ量

    // デバイス情報
    uint64 dedicatedVideoMemory = 0;    ///< 専用ビデオメモリ
    uint64 sharedSystemMemory = 0;      ///< 共有システムメモリ

    /// 予算超えてしていか
    bool IsOverBudget() const
    {
        return totalAllocatedBytes > budgetBytes;
    }

    /// カテゴリ名取得
    static const char* GetCategoryName(ERHIResourceCategory category);
};

// ════════════════════════════════════════════════════════════════
// メモリトラッカー
// ════════════════════════════════════════════════════════════════

/// リソースメモリ情報
struct RHIResourceMemoryInfo
{
    IRHIResource* resource = nullptr;
    ERHIResourceCategory category = ERHIResourceCategory::Other;
    uint64 allocatedSize = 0;
    uint64 usedSize = 0;
    const char* debugName = nullptr;
    uint64 allocationTime = 0;          ///< 割り当て時刻（マイクロ秒）
};

/// メモリトラップーインターフェース
class IRHIMemoryTracker
{
public:
    virtual ~IRHIMemoryTracker() = default;

    /// リソース割り当てを記録
    virtual void OnResourceAllocated(
        IRHIResource* resource,
        ERHIResourceCategory category,
        uint64 allocatedSize,
        uint64 usedSize,
        const char* debugName) = 0;

    /// リソース解放を記録
    virtual void OnResourceFreed(IRHIResource* resource) = 0;

    /// 現在の統計を取得
    virtual RHIMemoryStats GetStats() const = 0;

    /// 指定カテゴリのリソース一覧
    virtual void GetResourcesByCategory(
        ERHIResourceCategory category,
        std::vector<RHIResourceMemoryInfo>& outResources) const = 0;

    /// メモリ使用量上位Nリソース
    virtual void GetTopResources(
        uint32 count,
        std::vector<RHIResourceMemoryInfo>& outResources) const = 0;

    /// メモリリークチェック
    virtual void CheckForLeaks() const = 0;

    /// 統計をリセット（ピーク値等）
    virtual void ResetPeakStats() = 0;
};

// ════════════════════════════════════════════════════════════════
// メモリ警告システム
// ════════════════════════════════════════════════════════════════

/// メモリ警告レベル
enum class ERHIMemoryWarningLevel : uint8
{
    None,
    Low,        ///< 80%使用
    Medium,     ///< 90%使用
    High,       ///< 95%使用
    Critical    ///< 100%超えて
};

/// メモリ警告コールバック
using RHIMemoryWarningCallback = std::function<void(ERHIMemoryWarningLevel, const RHIMemoryStats&)>;

/// メモリ監視
class RHIMemoryMonitor
{
public:
    explicit RHIMemoryMonitor(IRHIMemoryTracker* tracker);

    /// 警告コールバック設定
    void SetWarningCallback(RHIMemoryWarningCallback callback);

    /// 警告閾値設定（パーセント）
    void SetWarningThresholds(float low, float medium, float high);

    /// 更新（毎フレーム呼び出し）
    void Update();

    /// 現在の警告レベル
    ERHIMemoryWarningLevel GetCurrentWarningLevel() const { return m_currentLevel; }

private:
    IRHIMemoryTracker* m_tracker;
    RHIMemoryWarningCallback m_callback;
    ERHIMemoryWarningLevel m_currentLevel = ERHIMemoryWarningLevel::None;
    float m_lowThreshold = 0.8f;
    float m_mediumThreshold = 0.9f;
    float m_highThreshold = 0.95f;
};

// ════════════════════════════════════════════════════════════════
// デバッグ出力
// ════════════════════════════════════════════════════════════════

/// メモリ統計をログ出力
void RHIPrintMemoryStats(const RHIMemoryStats& stats);

/// ImGuiメモリウィンドウ
void RHIDrawMemoryStatsImGui(IRHIMemoryTracker* tracker);

/// メモリ使用量グラフ描画
void RHIDrawMemoryGraph(const RHIMemoryStats& stats);

} // namespace NS::RHI
```

## 使用例

```cpp
// メモリ監視の設定
RHIMemoryMonitor memoryMonitor(device->GetMemoryTracker());

memoryMonitor.SetWarningCallback([](ERHIMemoryWarningLevel level, const RHIMemoryStats& stats) {
    if (level >= ERHIMemoryWarningLevel::High)
    {
        NS_LOG_WARN("GPU memory warning: %s (%.1f%% used)",
            level == ERHIMemoryWarningLevel::Critical ? "CRITICAL" : "HIGH",
            100.0f * stats.totalAllocatedBytes / stats.budgetBytes);

        // メモリ削減策を実行
        TextureManager::Get().UnloadUnusedTextures();
    }
});

// 毎フレーム更新
void OnFrameEnd()
{
    memoryMonitor.Update();

    // デバッグ表示
    if (showDebugUI)
    {
        RHIDrawMemoryStatsImGui(device->GetMemoryTracker());
    }
}

// リソースリーク検出（シャットダウン時）
void OnShutdown()
{
    device->GetMemoryTracker()->CheckForLeaks();
}
```

## 検証

- [ ] カテゴリ別統計の正確性
- [ ] ヒープ使用量追跡
- [ ] 警告コールバック動作
- [ ] リークチェック機能

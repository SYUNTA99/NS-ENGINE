# 25-01: コマンドリスト統計

## 概要

コマンドリストの実行統計を収集・類。描画コール数、状態変更回数、
リソースバインディング等を追跡し、パフォーマンス最適化に活用。

## ファイル

- `Source/Engine/RHI/Public/RHICommandListStats.h`

## 依存

- 02-05-command-list.md (IRHICommandList)

## 定義

```cpp
namespace NS::RHI
{

// ════════════════════════════════════════════════════════════════
// 統計データ構造
// ════════════════════════════════════════════════════════════════

/// 描画コール統計
struct RHIDrawCallStats
{
    uint32 drawCalls = 0;               ///< 通常描画コール
    uint32 drawIndexedCalls = 0;        ///< インデックス付き描画
    uint32 drawInstancedCalls = 0;      ///< インスタンス描画
    uint32 drawIndirectCalls = 0;       ///< Indirect描画
    uint32 dispatchCalls = 0;           ///< コンピュートディスパッチ
    uint32 dispatchIndirectCalls = 0;   ///< Indirectディスパッチ
    uint32 dispatchMeshCalls = 0;       ///< メッシュシェーダーディスパッチ
    uint32 dispatchRaysCalls = 0;       ///< レイトレーシングディスパッチ

    uint32 GetTotalDrawCalls() const
    {
        return drawCalls + drawIndexedCalls + drawInstancedCalls + drawIndirectCalls;
    }

    uint32 GetTotalDispatchCalls() const
    {
        return dispatchCalls + dispatchIndirectCalls + dispatchMeshCalls + dispatchRaysCalls;
    }
};

/// 状態変更統計
struct RHIStateChangeStats
{
    uint32 psoChanges = 0;              ///< PSO変更回数
    uint32 rootSignatureChanges = 0;    ///< ルートシグネチャ変更
    uint32 renderTargetChanges = 0;     ///< レンダーターゲット変更
    uint32 viewportChanges = 0;         ///< ビューポート変更
    uint32 scissorChanges = 0;          ///< シザー変更
    uint32 blendFactorChanges = 0;      ///< ブレンドファクター変更
    uint32 stencilRefChanges = 0;       ///< ステンシル参照値変更
    uint32 primitiveTopologyChanges = 0; ///< トポロジー変更
};

/// リソースバインディング統計
struct RHIBindingStats
{
    uint32 vertexBufferBinds = 0;       ///< 頂点バッファバインド
    uint32 indexBufferBinds = 0;        ///< インデックスバッファバインド
    uint32 constantBufferBinds = 0;     ///< 定数バッファバインド
    uint32 srvBinds = 0;                ///< SRVバインド
    uint32 uavBinds = 0;                ///< UAVバインド
    uint32 samplerBinds = 0;            ///< サンプラーバインド
    uint32 descriptorTableBinds = 0;    ///< ディスクリプタテーブルバインド
};

/// リソース遷移統計
struct RHIBarrierStats
{
    uint32 textureBarriers = 0;         ///< テクスチャバリア数
    uint32 bufferBarriers = 0;          ///< バッファバリア数
    uint32 uavBarriers = 0;             ///< UAVバリア数
    uint32 aliasingBarriers = 0;        ///< エイリアシングバリア数
    uint32 batchedBarriers = 0;         ///< バッチ処理されたバリア数
    uint32 redundantBarriers = 0;       ///< 冗長バリア（最適化で除去）。
};

/// メモリ操作統計
struct RHIMemoryOpStats
{
    uint32 bufferCopies = 0;            ///< バッファコピー回数
    uint32 textureCopies = 0;           ///< テクスチャコピー回数
    uint32 bufferUpdates = 0;           ///< バッファ更新回数
    uint64 totalCopyBytes = 0;          ///< コピー総バイト数
    uint64 totalUpdateBytes = 0;        ///< 更新総バイト数
};

/// コマンドリスト統計（統合）
struct RHICommandListStats
{
    RHIDrawCallStats draws;
    RHIStateChangeStats stateChanges;
    RHIBindingStats bindings;
    RHIBarrierStats barriers;
    RHIMemoryOpStats memoryOps;

    uint32 commandCount = 0;            ///< 総コマンド数
    uint32 renderPassCount = 0;         ///< レンダーパス数
    uint64 estimatedGPUCycles = 0;      ///< 推定GPUサイクル

    /// 統計をリセット
    void Reset()
    {
        *this = RHICommandListStats{};
    }

    /// 統計を加算
    void Accumulate(const RHICommandListStats& other);

    /// サマリー文字列生成
    std::string ToSummaryString() const;

    /// 詳細文字列生成
    std::string ToDetailedString() const;
};

// ════════════════════════════════════════════════════════════════
// 統計収集をンターフェース
// ════════════════════════════════════════════════════════════════

/// コマンドリスト統計コレクター
/// デバッグビルドでのみアクテスト
class IRHICommandListStatsCollector
{
public:
    virtual ~IRHICommandListStatsCollector() = default;

    /// 統計収集を開始
    virtual void BeginCollecting() = 0;

    /// 統計収集を終了
    virtual void EndCollecting() = 0;

    /// 収集中かどうか
    virtual bool IsCollecting() const = 0;

    /// 現在の統計を取得
    virtual RHICommandListStats GetStats() const = 0;

    /// 統計をリセット
    virtual void ResetStats() = 0;
};

// ════════════════════════════════════════════════════════════════
// フレーム統計
// ════════════════════════════════════════════════════════════════

/// フレーム統計（複数コマンドリストの集合。
struct RHIFrameStats
{
    RHICommandListStats accumulated;    ///< 集計
    uint32 commandListCount = 0;        ///< コマンドリスト数
    uint64 cpuRecordTimeUs = 0;         ///< CPU記録時間（マイクロ秒）
    uint64 gpuExecuteTimeUs = 0;        ///< GPU実行時間（マイクロ秒）

    /// 効率メトリクス
    float GetStateChangePerDrawCall() const
    {
        uint32 totalDraws = accumulated.draws.GetTotalDrawCalls();
        if (totalDraws == 0) return 0.0f;
        return static_cast<float>(accumulated.stateChanges.psoChanges) / totalDraws;
    }

    float GetBarriersPerRenderPass() const
    {
        if (accumulated.renderPassCount == 0) return 0.0f;
        uint32 totalBarriers = accumulated.barriers.textureBarriers +
                               accumulated.barriers.bufferBarriers;
        return static_cast<float>(totalBarriers) / accumulated.renderPassCount;
    }
};

/// フレーム統計トラッカー
class RHIFrameStatsTracker
{
public:
    /// フレーム開始
    void BeginFrame();

    /// コマンドリスト統計を追加
    void AddCommandListStats(const RHICommandListStats& stats);

    /// フレーム終了
    void EndFrame();

    /// 現在フレームの統計
    const RHIFrameStats& GetCurrentFrameStats() const { return m_currentFrame; }

    /// 過去N フレームの平均
    RHIFrameStats GetAverageStats(uint32 frameCount) const;

    /// ピーク統計（最大負荷フレーム）。
    const RHIFrameStats& GetPeakStats() const { return m_peakFrame; }

private:
    static constexpr uint32 kHistorySize = 120;

    RHIFrameStats m_currentFrame;
    RHIFrameStats m_peakFrame;
    std::array<RHIFrameStats, kHistorySize> m_history;
    uint32 m_historyIndex = 0;
    uint64 m_frameStartTime = 0;
};

// ════════════════════════════════════════════════════════════════
// デバッグ出力
// ════════════════════════════════════════════════════════════════

/// 統計をコンソール/ログに出力
void RHIPrintFrameStats(const RHIFrameStats& stats);

/// 統計をCSV形式で出力
void RHIExportStatsToCSV(const RHIFrameStats* stats, uint32 frameCount, const char* filename);

/// ImGui統計ウィンドウ描画
void RHIDrawStatsImGui(const RHIFrameStatsTracker& tracker);

} // namespace NS::RHI
```

## 使用例

```cpp
// 統計トラップーの使用
RHIFrameStatsTracker statsTracker;

void RenderFrame()
{
    statsTracker.BeginFrame();

    // 各コマンドリスト記録後に統計を追加
    for (auto& cmdList : commandLists)
    {
        cmdList->Close();
        statsTracker.AddCommandListStats(cmdList->GetStats());
    }

    statsTracker.EndFrame();

    // パフォーマンス警告
    auto& stats = statsTracker.GetCurrentFrameStats();
    if (stats.accumulated.draws.GetTotalDrawCalls() > 5000)
    {
        NS_LOG_WARN("High draw call count: %u",
            stats.accumulated.draws.GetTotalDrawCalls());
    }

    if (stats.GetStateChangePerDrawCall() > 0.5f)
    {
        NS_LOG_WARN("High state change ratio: %.2f",
            stats.GetStateChangePerDrawCall());
    }
}

// ImGuiでの表示
void DrawDebugUI()
{
    RHIDrawStatsImGui(statsTracker);
}
```

## 検証

- [ ] 全コマンドタイプのカウント
- [ ] 統計の精度（サンプルフレームで手動検証）。
- [ ] 履歴バッファのリングバッファ動作
- [ ] マルチスレッド安全性

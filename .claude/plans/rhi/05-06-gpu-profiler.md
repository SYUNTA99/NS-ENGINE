# 05-04: GPUプロファイラ

## 目的

GPU実行時間の計測とプロファイリングシステムを定義する。

## 参照ドキュメント

- .claude/plans/rhi/docs/RHI_Implementation_Guide_Part12.md (6. GPU Profiler)
- 05-03-query.md (タイムスタンプクエリ)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIGPUProfiler.h`
- `Source/Engine/RHI/Private/RHIGPUProfiler.cpp`

## TODO

### プロファイルイベントタイプ

```cpp
/// GPUプロファイルイベントタイプ
enum class ERHIGPUProfileEventType : uint8 {
    /// 描画コール
    Draw,

    /// コンピュートディスパッチ
    Dispatch,

    /// コピー操作
    Copy,

    /// レンダーパス
    RenderPass,

    /// カスタムマーカー
    Custom
};

/// プロファイルスコープフラグ
enum class ERHIGPUProfileFlags : uint32 {
    None = 0,

    /// CPU時間も計測
    IncludeCPUTime = 1 << 0,

    /// メモリ使用量も計測
    IncludeMemory = 1 << 1,

    /// パイプライン統計も計測
    IncludePipelineStats = 1 << 2,

    /// 詳細統計
    Verbose = 1 << 3
};
```

### プロファイルイベント

```cpp
/// GPUプロファイルイベント
struct RHIGPUProfileEvent {
    /// イベント名
    const char* name;

    /// イベントタイプ
    ERHIGPUProfileEventType type;

    /// 開始タイムスタンプ(GPU ticks)
    uint64 startTimestamp;

    /// 終了タイムスタンプ（GPU ticks)
    uint64 endTimestamp;

    /// 経過時間 (マイクロ秒
    double elapsedMicroseconds;

    /// 親イベントインデックス (-1 = ルート）
    int32 parentIndex;

    /// 深度レベル
    uint32 depth;

    /// フレーム番号
    uint64 frameNumber;

    /// 追加統計(フラグに応じて)
    struct {
        double cpuElapsedMicroseconds;
        uint64 memoryUsedBytes;
        uint64 drawCalls;
        uint64 primitives;
        uint64 vertices;
    } stats;
};
```

### プロファイラインターフェース

```cpp
/// GPUプロファイラ
class IRHIGPUProfiler {
public:
    virtual ~IRHIGPUProfiler() = default;

    //=========================================================================
    // 有効化無効化
    //=========================================================================

    /// プロファイリング開始
    virtual void BeginProfiling() = 0;

    /// プロファイリング終了
    virtual void EndProfiling() = 0;

    /// プロファイリング中か
    virtual bool IsProfiling() const = 0;

    //=========================================================================
    // スコープ操作
    //=========================================================================

    /// プロファイルスコープ開始
    /// @return スコープID (EndScope用)
    virtual uint32 BeginScope(
        IRHICommandContext* context,
        const char* name,
        ERHIGPUProfileEventType type = ERHIGPUProfileEventType::Custom,
        ERHIGPUProfileFlags flags = ERHIGPUProfileFlags::None
    ) = 0;

    /// プロファイルスコープ終了
    virtual void EndScope(
        IRHICommandContext* context,
        uint32 scopeId
    ) = 0;

    //=========================================================================
    // 結果取得
    //=========================================================================

    /// フレームの結果が準備完了後
    virtual bool IsFrameReady(uint64 frameNumber) const = 0;

    /// フレームの結果取得
    virtual bool GetFrameResults(
        uint64 frameNumber,
        TArray<RHIGPUProfileEvent>& outEvents
    ) = 0;

    /// 最新の完了フレーム番号取得
    virtual uint64 GetLatestCompletedFrame() const = 0;

    //=========================================================================
    // 統計
    //=========================================================================

    /// タイムスタンプ周波数取得(Hz)
    virtual uint64 GetTimestampFrequency() const = 0;

    /// フレーム全体のGPU時間取得(マイクロ秒
    virtual double GetFrameGPUTime(uint64 frameNumber) const = 0;
};
```

### スコープヘルパー。

```cpp
/// RAIIプロファイルスコープ
class FRHIGPUProfileScope {
public:
    FRHIGPUProfileScope(
        IRHIGPUProfiler* profiler,
        IRHICommandContext* context,
        const char* name,
        ERHIGPUProfileEventType type = ERHIGPUProfileEventType::Custom
    ) : m_profiler(profiler)
      , m_context(context)
      , m_scopeId(0)
    {
        if (m_profiler && m_profiler->IsProfiling()) {
            m_scopeId = m_profiler->BeginScope(m_context, name, type);
        }
    }

    ~FRHIGPUProfileScope() {
        if (m_profiler && m_scopeId != 0) {
            m_profiler->EndScope(m_context, m_scopeId);
        }
    }

    NS_DISALLOW_COPY(FRHIGPUProfileScope);
    IRHIGPUProfiler* m_profiler;
    IRHICommandContext* m_context;
    uint32 m_scopeId;
};

/// プロファイルスコープマクロ
#if NS_GPU_PROFILING_ENABLED
    #define RHI_GPU_PROFILE_SCOPE(Profiler, Context, Name) \
        FRHIGPUProfileScope NS_MACRO_CONCATENATE(_gpuProfileScope, __LINE__)(Profiler, Context, Name)

    #define RHI_GPU_PROFILE_SCOPE_TYPE(Profiler, Context, Name, Type) \
        FRHIGPUProfileScope NS_MACRO_CONCATENATE(_gpuProfileScope, __LINE__)(Profiler, Context, Name, Type)
#else
    #define RHI_GPU_PROFILE_SCOPE(Profiler, Context, Name)
    #define RHI_GPU_PROFILE_SCOPE_TYPE(Profiler, Context, Name, Type)
#endif
```

### フレームプロファイルデータ

```cpp
/// フレームプロファイルデータ
struct RHIFrameProfileData {
    /// フレーム番号
    uint64 frameNumber;

    /// フレーム全体のGPU時間 (マイクロ秒
    double totalGPUTime;

    /// フレーム全体のCPU時間 (マイクロ秒
    double totalCPUTime;

    /// イベントの分
    TArray<RHIGPUProfileEvent> events;

    /// カテゴリ別集合。
    struct CategoryStats {
        double drawTime;
        double dispatchTime;
        double copyTime;
        double renderPassTime;
        double otherTime;
    } categoryStats;

    /// 最も時間のかかったイベント(上位N件)
    TArray<uint32> topEventIndices;
};

/// プロファイルヒストリー
class FRHIProfileHistory {
public:
    /// フレームデータ追加
    void AddFrame(const RHIFrameProfileData& data);

    /// 平均GPU時間取得(過去N フレーム)
    double GetAverageGPUTime(uint32 frameCount = 60) const;

    /// 最大GPU時間取得
    double GetMaxGPUTime(uint32 frameCount = 60) const;

    /// フレームデータ取得
    const RHIFrameProfileData* GetFrame(uint64 frameNumber) const;

    /// 履歴クリア
    void Clear();

private:
    static constexpr uint32 kMaxHistoryFrames = 300;
    TArray<RHIFrameProfileData> m_history;
};
```

### デバイス拡張

```cpp
class IRHIDevice {
    // ... 既存メソッド...

    /// GPUプロファイラ取得
    virtual IRHIGPUProfiler* GetGPUProfiler() = 0;

    /// プロファイラが利用可能か
    virtual bool IsGPUProfilingSupported() const = 0;
};
```

### 使用パターン

```cpp
// 1. プロファイリング開始
auto profiler = device->GetGPUProfiler();
profiler->BeginProfiling();

// 2. フレームルーチ
while (running) {
    {
        RHI_GPU_PROFILE_SCOPE(profiler, context, "Frame");

        {
            RHI_GPU_PROFILE_SCOPE_TYPE(profiler, context, "ShadowPass",
                ERHIGPUProfileEventType::RenderPass);
            // シャドウパス描画
        }

        {
            RHI_GPU_PROFILE_SCOPE_TYPE(profiler, context, "GBuffer",
                ERHIGPUProfileEventType::RenderPass);
            // GBuffer描画
        }

        {
            RHI_GPU_PROFILE_SCOPE_TYPE(profiler, context, "Lighting",
                ERHIGPUProfileEventType::Dispatch);
            // ライティング計算
        }
    }

    // 結果取得(数フレーム遅延)
    uint64 readyFrame = profiler->GetLatestCompletedFrame();
    if (readyFrame > 0) {
        TArray<RHIGPUProfileEvent> events;
        if (profiler->GetFrameResults(readyFrame, events)) {
            for (const auto& event : events) {
                printf("%*s%s: %.3f ms\n",
                    event.depth * 2, "",
                    event.name,
                    event.elapsedMicroseconds / 1000.0);
            }
        }
    }
}

// 3. プロファイリング終了
profiler->EndProfiling();
```

### プロファイラ実装詳細

```cpp
/// 内部: タイムスタンプクエリ管理
struct FGPUProfilerQueryHeap {
    IRHIQueryHeap* timestampPool;
    uint32 currentIndex;
    uint32 maxQueries;

    uint32 AllocateQuery() {
        uint32 index = currentIndex;
        currentIndex = (currentIndex + 1) % maxQueries;
        return index;
    }
};

/// 内部: 保留中のスコープ
struct FPendingProfileScope {
    const char* name;
    ERHIGPUProfileEventType type;
    uint32 startQueryIndex;
    uint32 endQueryIndex;
    int32 parentScopeIndex;
    uint32 depth;
};
```

## 検証方法

- タイムスタンプ精度の検証
- 階層的スコープの正確性
- マルチフレーム遅延の処理
- メモリリーク検証
- スレッドセーフ性

## 備考

GPUプロファイリングは開発/デバッグビルドでのみ有効にすることを推奨、
リリースビルドではオーバのヘッド回避のため無効化する。

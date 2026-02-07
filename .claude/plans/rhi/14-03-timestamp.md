# 14-03: タイムスタンプ

## 目的

GPUタイムスタンプクエリの高レベルAPIを定義する。

## 参照ドキュメント

- 14-01-query-types.md (ERHIQueryType::Timestamp)
- 14-02-query-pool.md (IRHIQueryHeap)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHITimestamp.h`

## TODO

### 1. タイムスタンプ結果

```cpp
#pragma once

#include "RHIQuery.h"

namespace NS::RHI
{
    /// タイムスタンプ結果
    struct RHI_API RHITimestampResult
    {
        /// GPUタイムスタンプ値
        uint64 timestamp = 0;

        /// 有効か
        bool valid = false;

        /// ナノ秒への変換（周波数が必要。
        double ToNanoseconds(uint64 frequency) const {
            return valid ? (static_cast<double>(timestamp) * 1e9 / frequency) : 0.0;
        }

        /// ミリ秒への変換
        double ToMilliseconds(uint64 frequency) const {
            return ToNanoseconds(frequency) / 1e6;
        }

        /// 秒への変換
        double ToSeconds(uint64 frequency) const {
            return ToNanoseconds(frequency) / 1e9;
        }
    };

    /// タイムスタンプ間隁
    struct RHI_API RHITimestampInterval
    {
        /// 開始タイムスタンプ
        uint64 start = 0;

        /// 終了タイムスタンプ
        uint64 end = 0;

        /// 有効か
        bool valid = false;

        /// 経過時間（タイムスタンプ単位）
        uint64 GetElapsed() const { return valid ? (end - start) : 0; }

        /// ナノ秒への変換
        double ToNanoseconds(uint64 frequency) const {
            return valid ? (static_cast<double>(end - start) * 1e9 / frequency) : 0.0;
        }

        /// ミリ秒への変換
        double ToMilliseconds(uint64 frequency) const {
            return ToNanoseconds(frequency) / 1e6;
        }
    };
}
```

- [ ] RHITimestampResult 構造体
- [ ] RHITimestampInterval 構造体

### 2. GPUタイマ。

```cpp
namespace NS::RHI
{
    /// GPUタイマ。
    /// 特定の処理のGPU実行時間を計測
    class RHI_API RHIGPUTimer
    {
    public:
        RHIGPUTimer() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            uint32 maxMeasurements = 256,
            uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // 計測
        //=====================================================================

        /// 計測開始
        /// @param context コマンドコンテキスト
        /// @param name 計測合
        /// @return 計測ID（EndTimerに渡す）
        uint32 BeginTimer(IRHICommandContext* context, const char* name);

        /// 計測終了
        void EndTimer(IRHICommandContext* context, uint32 timerId);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// 結果準備完了後
        bool AreResultsReady() const;

        /// 計測結果取得（ミリ秒）
        double GetTimerResult(uint32 timerId) const;

        /// 計測結果取得（名前指定）
        double GetTimerResult(const char* name) const;

        /// 全計測結果取得
        struct TimerResult {
            const char* name;
            double milliseconds;
        };
        uint32 GetAllResults(TimerResult* outResults, uint32 maxResults) const;

        //=====================================================================
        // 情報
        //=====================================================================

        /// タイムスタンプ周波数取得
        uint64 GetTimestampFrequency() const { return m_frequency; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIQueryAllocator m_queryAllocator;
        uint64 m_frequency = 0;

        struct Measurement {
            const char* name;
            uint32 startQueryIndex;
            uint32 endQueryIndex;
        };
        Measurement* m_measurements = nullptr;
        uint32 m_measurementCount = 0;
        uint32 m_maxMeasurements = 0;
    };
}
```

- [ ] RHIGPUTimer クラス

### 3. スコープタイマ。

```cpp
namespace NS::RHI
{
    /// スコープGPUタイマ（RAII）。
    class RHI_API RHIScopedGPUTimer
    {
    public:
        RHIScopedGPUTimer(
            RHIGPUTimer* timer,
            IRHICommandContext* context,
            const char* name)
            : m_timer(timer)
            , m_context(context)
        {
            if (m_timer && m_context) {
                m_timerId = m_timer->BeginTimer(m_context, name);
            }
        }

        ~RHIScopedGPUTimer() {
            if (m_timer && m_context && m_timerId != ~0u) {
                m_timer->EndTimer(m_context, m_timerId);
            }
        }

        NS_DISALLOW_COPY(RHIScopedGPUTimer);
        RHIGPUTimer* m_timer = nullptr;
        IRHICommandContext* m_context = nullptr;
        uint32 m_timerId = ~0u;
    };

    /// スコープタイマのマクロ
    #define RHI_SCOPED_GPU_TIMER(timer, context, name) \
        NS::RHI::RHIScopedGPUTimer NS_MACRO_CONCATENATE(_rhiTimer, __LINE__)(timer, context, name)
}
```

- [ ] RHIScopedGPUTimer クラス

### 4. フレームタイムライン

```cpp
namespace NS::RHI
{
    /// フレームタイムラインエントリ
    struct RHI_API RHIFrameTimelineEntry
    {
        /// 名前
        const char* name = nullptr;

        /// 開始時間（フレーム開始からのミリ秒）
        double startMs = 0.0;

        /// 終了時間
        double endMs = 0.0;

        /// 持続時間
        double DurationMs() const { return endMs - startMs; }

        /// 階層レベル
        uint32 level = 0;

        /// カラー
        uint32 color = 0xFFFFFFFF;
    };

    /// フレームタイムライン
    /// 1フレームのGPU処理中タイムライン形式で記録
    class RHI_API RHIFrameTimeline
    {
    public:
        RHIFrameTimeline() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            uint32 maxEntries = 1024,
            uint32 numBufferedFrames = 3);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(IRHICommandContext* context);
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // 記録
        //=====================================================================

        /// 区間開始
        void BeginSection(
            IRHICommandContext* context,
            const char* name,
            uint32 color = 0xFFFFFFFF);

        /// 区間終了
        void EndSection(IRHICommandContext* context);

        /// マーカー（瞬間）
        void InsertMarker(
            IRHICommandContext* context,
            const char* name,
            uint32 color = 0xFFFFFFFF);

        //=====================================================================
        // 結果取得
        //=====================================================================

        /// 結果準備完了後
        bool AreResultsReady() const;

        /// フレーム全体の時間（ミリ秒）
        double GetFrameTimeMs() const;

        /// エントリ数取得
        uint32 GetEntryCount() const { return m_resultEntryCount; }

        /// エントリ取得
        const RHIFrameTimelineEntry* GetEntries() const { return m_resultEntries; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIQueryAllocator m_queryAllocator;
        uint64 m_frequency = 0;

        RHIFrameTimelineEntry* m_resultEntries = nullptr;
        uint32 m_resultEntryCount = 0;
        uint32 m_maxEntries = 0;

        uint32 m_currentLevel = 0;
    };
}
```

- [ ] RHIFrameTimelineEntry 構造体
- [ ] RHIFrameTimeline クラス

### 5. GPUプロファイラ統合

```cpp
namespace NS::RHI
{
    /// GPUプロファイラ
    /// タイマのとタイムラインの統合インターフェース
    class RHI_API RHIGPUProfiler
    {
    public:
        RHIGPUProfiler() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame(IRHICommandContext* context);
        void EndFrame(IRHICommandContext* context);

        //=====================================================================
        // プロファイリング
        //=====================================================================

        /// GPU計測開始
        void BeginGPUEvent(
            IRHICommandContext* context,
            const char* name,
            uint32 color = 0xFFFFFFFF);

        /// GPU計測終了
        void EndGPUEvent(IRHICommandContext* context);

        //=====================================================================
        // 結果
        //=====================================================================

        /// GPUタイマの取得
        RHIGPUTimer* GetTimer() { return &m_timer; }

        /// タイムライン取得
        RHIFrameTimeline* GetTimeline() { return &m_timeline; }

        /// 有効/無効切り替え
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

    private:
        RHIGPUTimer m_timer;
        RHIFrameTimeline m_timeline;
        bool m_enabled = true;
    };
}
```

- [ ] RHIGPUProfiler クラス

## 検証方法

- [ ] タイムスタンプ値の正確性
- [ ] タイマーの開始終了
- [ ] タイムラインの階層構造
- [ ] プロファイラ統合

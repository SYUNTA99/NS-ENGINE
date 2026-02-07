# 09-02: 同期ポイント

## 目的

フレーム同期やパイプライン同期のための同期ポイント抽象化を定義する。

## 参照ドキュメント

- 09-01-fence-interface.md (IRHIFence)
- 01-25-queue-interface.md (IRHIQueue)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHISyncPoint.h`

## TODO

### 1. 同期ポイント構造体

```cpp
#pragma once

#include "IRHIFence.h"

namespace NS::RHI
{
    /// 同期ポイント
    /// フェンスと値のペアで特定のGPU処理完了を表す
    struct RHI_API RHISyncPoint
    {
        /// フェンス
        IRHIFence* fence = nullptr;

        /// フェンス値
        uint64 value = 0;

        /// 有効か
        bool IsValid() const { return fence != nullptr; }

        /// 完了しているか
        bool IsCompleted() const {
            return fence && fence->IsCompleted(value);
        }

        /// CPU待ち
        /// @param timeoutMs タイムアウト（ミリ秒）。デフォルト30秒。
        ///        タイムアウト時はfalseを返す。
        ///        呼び出し側でfalse時にデバイスロスト状態をチェックすることを推奨。
        bool Wait(uint64 timeoutMs = 30000) const {
            return fence ? fence->Wait(value, timeoutMs) : true;
        }

        /// 比較演算子
        bool operator==(const RHISyncPoint& other) const {
            return fence == other.fence && value == other.value;
        }

        bool operator!=(const RHISyncPoint& other) const {
            return !(*this == other);
        }

        /// 無効な同期ポイント
        static RHISyncPoint Invalid() { return {}; }
    };
}
```

- [ ] RHISyncPoint 構造体

### 2. フレーム同期管理

```cpp
namespace NS::RHI
{
    /// フレーム同期管理
    /// ダブル/トリプルバッファリングのフレーム同期を管理
    class RHI_API RHIFrameSync
    {
    public:
        /// 最大バッファリングフレーム数
        static constexpr uint32 kMaxBufferedFrames = 4;

        RHIFrameSync() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device, uint32 numBufferedFrames);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始
        /// 前のフレームのGPU処理完了を待つ。
        void BeginFrame();

        /// フレーム終了
        /// GPUキューにフレーム完了シグナルを発行
        void EndFrame(IRHIQueue* queue);

        //=====================================================================
        // フレーム情報
        //=====================================================================

        /// 現在のフレームインデックス（〜numBufferedFrames-1）。
        uint32 GetCurrentFrameIndex() const { return m_currentFrameIndex; }

        /// 現在のCPUフレーム番号
        uint64 GetCurrentFrameNumber() const { return m_frameNumber; }

        /// 現在のGPU完了フレーム番号
        uint64 GetCompletedFrameNumber() const;

        /// フレームインフライト数
        uint32 GetFramesInFlight() const;

        /// バッファリングフレーム数
        uint32 GetNumBufferedFrames() const { return m_numBufferedFrames; }

        //=====================================================================
        // 同期ポイント
        //=====================================================================

        /// 現在フレームの同期ポイント取得
        RHISyncPoint GetCurrentFrameSyncPoint() const;

        /// 指定フレームの同期ポイント取得
        RHISyncPoint GetFrameSyncPoint(uint64 frameNumber) const;

        /// 指定フレームの完了を待つ。
        /// @param timeoutMs デフォルト30秒。超過時はログ警告+デバイスロストチェック
        bool WaitForFrame(uint64 frameNumber, uint64 timeoutMs = 30000);

        /// すべてのインフライトフレームの完了を待つ。
        /// 各フレームに30秒タイムアウトを適用し、
        /// 超過時はログ警告+デバイスロストチェックを実行。
        void WaitForAllFrames();

    private:
        IRHIDevice* m_device = nullptr;
        RHIFenceRef m_frameFence;

        uint32 m_numBufferedFrames = 2;
        uint32 m_currentFrameIndex = 0;
        uint64 m_frameNumber = 0;
        uint64 m_frameFenceValues[kMaxBufferedFrames] = {};
    };
}
```

- [ ] RHIFrameSync クラス
- [ ] フレーム操作
- [ ] 同期ポイント取得

### 3. パイプライン同期

```cpp
namespace NS::RHI
{
    /// パイプライン同期タイプ
    enum class ERHIPipelineSyncType : uint8
    {
        /// グラフィックス→グラフィックス
        GraphicsToGraphics,

        /// グラフィックス→コンピュート
        GraphicsToCompute,

        /// コンピュートのグラフィックス
        ComputeToGraphics,

        /// コンピュートのコンピュート
        ComputeToCompute,

        /// コピー→グラフィックス
        CopyToGraphics,

        /// コピー→コンピュート
        CopyToCompute,

        /// グラフィックス→コピー
        GraphicsToCopy,

        /// コンピュートのコピー
        ComputeToCopy,
    };

    /// パイプライン同期ヘルパー
    class RHI_API RHIPipelineSync
    {
    public:
        RHIPipelineSync() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // キュー間同期
        //=====================================================================

        /// 同期ポイント挿入
        /// @param fromQueue 発行のキュー
        /// @return 同期ポイント
        RHISyncPoint InsertSyncPoint(IRHIQueue* fromQueue);

        /// 同期ポイント待ち。
        /// @param queue 待機するキュー
        /// @param syncPoint 待機する同期ポイント
        void WaitForSyncPoint(IRHIQueue* queue, const RHISyncPoint& syncPoint);

        //=====================================================================
        // 便利メソッド
        //=====================================================================

        /// キュー間同期（発行と待機）
        /// @note デッドロック防止: 同一フレーム内で循環待ち
        ///       （A→B かつ B→A）が発生しないよう呼び出し側が保証すること。
        ///       通常はGraphics→Computeの一方向同期のみ使用する。
        ///       デバッグビルドでは循環検出のアサートを有効にする。
        void SyncQueues(IRHIQueue* fromQueue, IRHIQueue* toQueue) {
            RHI_ASSERT(fromQueue != toQueue, "Self-sync is not allowed");
#if NS_BUILD_DEBUG
            // 循環デッドロック検出: A→B + B→A 等のサイクルをDFSで検出
            const uint32 fromIdx = fromQueue->GetQueueIndex();
            const uint32 toIdx = toQueue->GetQueueIndex();
            RHI_ASSERT(ValidateNoCircularDependency(fromIdx, toIdx),
                "Circular queue dependency detected: adding %u->%u creates a cycle", fromIdx, toIdx);
            m_syncGraph[fromIdx][toIdx]++;
#endif
            RHISyncPoint sp = InsertSyncPoint(fromQueue);
            WaitForSyncPoint(toQueue, sp);
            // リリースビルドでの安全弁:
            // WaitForSyncPoint内部のフェンス待ちはタイムアウト付き。
            // タイムアウト発生時はログ警告を出力し、
            // デバイスロスト状態をチェックする。
        }

        /// グラフィックス→コンピュート同期
        void GraphicsToCompute(IRHIQueue* graphicsQueue, IRHIQueue* computeQueue) {
            SyncQueues(graphicsQueue, computeQueue);
        }

        /// コンピュート→グラフィックス同期
        void ComputeToGraphics(IRHIQueue* computeQueue, IRHIQueue* graphicsQueue) {
            SyncQueues(computeQueue, graphicsQueue);
        }

    private:
        IRHIDevice* m_device = nullptr;
        RHIFenceRef m_syncFence;
        uint64 m_nextSyncValue = 1;

        //=====================================================================
        // 循環デッドロック検出（デバッグビルドのみ）
        //=====================================================================

        /// キュー間同期依存グラフ（隣接リスト）
        /// m_syncGraph[from][to] が非0ならフレーム内で from→to の同期依存が存在
        static constexpr uint32 kMaxQueues = 8;
        uint32 m_syncGraph[kMaxQueues][kMaxQueues] = {};

        /// DFSサイクル検出で循環依存がないことを検証
        /// @param fromQueue 発行元キューインデックス
        /// @param toQueue 待機先キューインデックス
        /// @return 循環が存在しなければtrue
        /// @note デバッグビルドのみ有効。計算量: O(V+E) where V=kMaxQueues, E=辺数
        ///
        /// アルゴリズム骨格:
        ///   1. fromQueue→toQueue の辺を仮追加
        ///   2. visited[kMaxQueues] = {false}, inStack[kMaxQueues] = {false} を用意
        ///   3. toQueue を起点にDFS開始:
        ///      - 現ノードを visited/inStack にマーク
        ///      - m_syncGraph[current][next] > 0 の全隣接を探索
        ///      - 隣接が inStack 内 → サイクル検出 → return false
        ///      - 隣接が未訪問 → 再帰DFS
        ///      - DFS完了後 inStack から除去
        ///   4. サイクルなし → return true
        ///   kMaxQueues=8 のため配列はスタック上に置ける（ヒープ割り当て不要）
        bool ValidateNoCircularDependency(uint32 fromQueue, uint32 toQueue) const;

        /// フレーム開始時に同期グラフをリセット
        void ResetFrameGraph();
    };
}
```

- [ ] ERHIPipelineSyncType 列挙型
- [ ] RHIPipelineSync クラス

### 4. 複数同期ポイント待ち。

```cpp
namespace NS::RHI
{
    /// 複数同期ポイント待ち。
    class RHI_API RHISyncPointWaiter
    {
    public:
        /// 最大同期ポイント数
        static constexpr uint32 kMaxSyncPoints = 16;

        RHISyncPointWaiter() = default;

        /// 同期ポイント追加
        void Add(const RHISyncPoint& syncPoint) {
            if (m_count < kMaxSyncPoints && syncPoint.IsValid()) {
                m_syncPoints[m_count++] = syncPoint;
            }
        }

        /// すべて待機（CPU）。
        bool WaitAll(uint64 timeoutMs = UINT64_MAX);

        /// いずれか待機（CPU）。
        /// @return 完了した同期ポイントのインデックス（-1でタイムアウト）
        int32 WaitAny(uint64 timeoutMs = UINT64_MAX);

        /// すべて完了しているか
        bool AreAllCompleted() const;

        /// いれか完了しているか
        bool IsAnyCompleted() const;

        /// クリア
        void Clear() { m_count = 0; }

        /// 同期ポイント数
        uint32 GetCount() const { return m_count; }

    private:
        RHISyncPoint m_syncPoints[kMaxSyncPoints];
        uint32 m_count = 0;
    };
}
```

- [ ] RHISyncPointWaiter クラス

### 5. タイムライン同期

```cpp
namespace NS::RHI
{
    /// タイムライン同期ポイント
    /// 時系列での同期ポイント管理
    class RHI_API RHITimelineSync
    {
    public:
        RHITimelineSync() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // タイムライン操作
        //=====================================================================

        /// 現在のタイムライン値取得
        uint64 GetCurrentValue() const;

        /// 次のタイムライン値取得（インクリメント）
        uint64 AcquireNextValue() { return m_nextValue++; }

        /// キューでシグナル
        uint64 Signal(IRHIQueue* queue);

        /// キューで待つ。
        void Wait(IRHIQueue* queue, uint64 value);

        /// CPU待ち。
        bool WaitCPU(uint64 value, uint64 timeoutMs = UINT64_MAX);

        /// 同期ポイント取得
        RHISyncPoint GetSyncPoint(uint64 value) const;

        /// フェンス取得
        IRHIFence* GetFence() const { return m_fence; }

    private:
        RHIFenceRef m_fence;
        uint64 m_nextValue = 1;
    };
}
```

- [ ] RHITimelineSync クラス

## 検証方法

- [ ] フレーム同期の動作
- [ ] キュー間同期
- [ ] 複数同期ポイント待ち。
- [ ] タイムライン同期
- [ ] 循環依存検出テスト（A→B + B→A でassert発火を確認）

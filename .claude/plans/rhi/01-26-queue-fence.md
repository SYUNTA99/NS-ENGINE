# 01-26: IRHIQueueフェンス管理

## 目的

IRHIQueueのフェンス管理とペイロード処理インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (5. Queue - フェンス管理
- docs/RHI/RHI_Implementation_Guide_Part3.md (同期/フェンス)

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IRHIQueue.h`

## TODO

### 1. キュー専用フェンス

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // キュー専用フェンス
        //=====================================================================

        /// キュー専用フェンス取得
        /// このキューのコマンド完了追跡用
        virtual IRHIFence* GetFence() const = 0;

        /// 最後に送信されたフェンス値
        virtual uint64 GetLastSubmittedFenceValue() const = 0;

        /// 最後に完了したフェンス値
        virtual uint64 GetLastCompletedFenceValue() const = 0;

        /// 指定フェンス値が完了しているか
        bool IsFenceComplete(uint64 fenceValue) const {
            return GetLastCompletedFenceValue() >= fenceValue;
        }

        /// 次のフェンス値を発行（の部でインクリメント）
        virtual uint64 AdvanceFence() = 0;
    };
}
```

- [ ] GetFence
- [ ] フェンス値取得メソッド
- [ ] AdvanceFence

### 2. フェンス待ち。

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // フェンス待ち。
        //=====================================================================

        /// CPU側でフェンス完了を待つ。
        /// @param fenceValue 待機するフェンス値
        /// @param timeoutMs タイムアウト（ミリ秒、0=無限）
        /// @return 完了したらtrue、タイムアウトでfalse
        virtual bool WaitForFence(uint64 fenceValue, uint32 timeoutMs = 0) = 0;

        /// 最新の送信コマンドが完了するまで待つ。
        bool WaitForIdle(uint32 timeoutMs = 0) {
            return WaitForFence(GetLastSubmittedFenceValue(), timeoutMs);
        }

        /// フェンスイベントオブジェクト取得（Windows用）。
        /// 外部で待機する場合に使用
        virtual void* GetFenceEventHandle() const = 0;
    };
}
```

- [ ] WaitForFence
- [ ] WaitForIdle
- [ ] GetFenceEventHandle

### 3. キュー間同期

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // キュー間同期
        //=====================================================================

        /// 他キューのフェンスを待つ。
        /// @param otherQueue 待機対象のキュー
        /// @param fenceValue 待機するフェンス値
        virtual void WaitForQueue(
            IRHIQueue* otherQueue,
            uint64 fenceValue) = 0;

        /// 他キューの最新コマンドを待つ。
        void WaitForQueueIdle(IRHIQueue* otherQueue) {
            WaitForQueue(otherQueue, otherQueue->GetLastSubmittedFenceValue());
        }

        /// 外部フェンスを待つ。
        /// 共有フェンス等、キュー外で作成されたフェンス用
        virtual void WaitForExternalFence(
            IRHIFence* fence,
            uint64 value) = 0;
    };
}
```

- [ ] WaitForQueue
- [ ] WaitForExternalFence

### 4. 同期ポイント

```cpp
namespace NS::RHI
{
    /// 同期ポイント
    /// キュー + フェンス値のペアで特定のGPU時点を表す
    struct RHISyncPoint
    {
        IRHIQueue* queue = nullptr;
        uint64 fenceValue = 0;

        RHISyncPoint() = default;
        RHISyncPoint(IRHIQueue* q, uint64 v) : queue(q), fenceValue(v) {}

        bool IsValid() const { return queue != nullptr; }

        bool IsComplete() const {
            return queue && queue->IsFenceComplete(fenceValue);
        }

        void Wait(uint32 timeoutMs = 0) const {
            if (queue) queue->WaitForFence(fenceValue, timeoutMs);
        }
    };

    class IRHIQueue
    {
    public:
        //=====================================================================
        // 同期ポイント
        //=====================================================================

        /// 現在の同期ポイントを作成
        RHISyncPoint CreateSyncPoint() {
            return RHISyncPoint(this, GetLastSubmittedFenceValue());
        }

        /// 同期ポイントでシグナルを発行
        RHISyncPoint SignalSyncPoint() {
            uint64 value = AdvanceFence();
            Signal(GetFence(), value);
            return RHISyncPoint(this, value);
        }

        /// 同期ポイントを待つ。
        void WaitForSyncPoint(const RHISyncPoint& syncPoint) {
            if (syncPoint.queue && syncPoint.queue != this) {
                WaitForQueue(syncPoint.queue, syncPoint.fenceValue);
            }
        }
    };
}
```

- [ ] RHISyncPoint 構造体
- [ ] CreateSyncPoint / SignalSyncPoint
- [ ] WaitForSyncPoint

### 5. 統計デバッグ

```cpp
namespace NS::RHI
{
    /// キュー統計
    struct RHIQueueStats
    {
        /// 送信されたコマンドリスト数
        uint64 commandListsSubmitted = 0;

        /// 実行された描画呼び出し数
        uint64 drawCalls = 0;

        /// 実行されたディスパッチ数
        uint64 dispatches = 0;

        /// バリア数
        uint64 barriers = 0;

        /// 平均GPU時間（ミリ秒）
        float averageGPUTimeMs = 0.0f;
    };

    class IRHIQueue
    {
    public:
        //=====================================================================
        // 統計
        //=====================================================================

        /// キュー統計取得
        virtual RHIQueueStats GetStats() const = 0;

        /// 統計リセット
        virtual void ResetStats() = 0;

        //=====================================================================
        // GPU診断
        //=====================================================================

        /// GPUクラッシュダンプ有効化
        virtual void EnableGPUCrashDump(bool enable) = 0;

        /// Breadcrumb（追跡用マーカー（挿入
        /// GPUハング時の原因特定用
        virtual void InsertBreadcrumb(uint32 value) = 0;
    };
}
```

- [ ] RHIQueueStats 構造体
- [ ] GetStats / ResetStats
- [ ] GPU診断機能

## 検証方法

- [ ] フェンス値の整合性
- [ ] 同期ポイントの正確性
- [ ] キュー間同期の安全性

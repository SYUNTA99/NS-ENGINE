# 01-21: IRHIDeviceキュー管理

## 目的

IRHIDeviceのコマンドキュー管理インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (4. Device - キュー管理

## 変更対象ファイル

更新:
- `Source/Engine/RHI/Public/IRHIDevice.h`

## TODO

### 1. キュー取得

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // キュー取得
        //=====================================================================

        /// 指定タイプのキュー数取得
        /// @param type キュータイプ
        /// @return キュー数（通常1、複数ある場合もある）。
        virtual uint32 GetQueueCount(ERHIQueueType type) const = 0;

        /// キュー取得
        /// @param type キュータイプ
        /// @param index キューインデックス（通常0）。
        /// @return キュー（存在しない場合nullptr）。
        virtual IRHIQueue* GetQueue(ERHIQueueType type, uint32 index = 0) const = 0;
    };
}
```

- [ ] GetQueueCount
- [ ] GetQueue

### 2. 便利なキュー取得メソッド

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // 便利なキュー取得
        //=====================================================================

        /// グラフィックスキュー取得（プライマリ）。
        IRHIQueue* GetGraphicsQueue() const {
            return GetQueue(ERHIQueueType::Graphics);
        }

        /// 非同期コンピュートキュー取得
        /// @note 専用コンピュートキューがない場合のグラフィックスキュー
        IRHIQueue* GetComputeQueue() const {
            IRHIQueue* queue = GetQueue(ERHIQueueType::Compute);
            return queue ? queue : GetGraphicsQueue();
        }

        /// コピーキュー取得
        /// @note 専用コピーキューがない場合のグラフィックスキュー
        IRHIQueue* GetCopyQueue() const {
            IRHIQueue* queue = GetQueue(ERHIQueueType::Copy);
            return queue ? queue : GetGraphicsQueue();
        }

        /// 非同期コンピュートキューがあるか
        bool HasAsyncComputeQueue() const {
            return GetQueueCount(ERHIQueueType::Compute) > 0;
        }

        /// 専用コピーキューがあるか
        bool HasCopyQueue() const {
            return GetQueueCount(ERHIQueueType::Copy) > 0;
        }
    };
}
```

- [ ] GetGraphicsQueue
- [ ] GetComputeQueue
- [ ] GetCopyQueue
- [ ] キュー存在チェック

### 3. キュー機能クエリ

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // キュー機能クエリ
        //=====================================================================

        /// 指定キュータイプが利用可能か
        bool IsQueueTypeAvailable(ERHIQueueType type) const {
            return GetQueueCount(type) > 0;
        }

        /// キューがグラフィックス操作をサポートするか
        static bool QueueSupportsGraphics(ERHIQueueType type) {
            return type == ERHIQueueType::Graphics;
        }

        /// キューがコンピュート操作をサポートするか
        static bool QueueSupportsCompute(ERHIQueueType type) {
            return type == ERHIQueueType::Graphics
                || type == ERHIQueueType::Compute;
        }

        /// キューがコピー操作をサポートするか
        static bool QueueSupportsCopy(ERHIQueueType type) {
            return true;  // 全キューでサポート
        }
    };
}
```

- [ ] キュー機能クエリ

### 4. キュー同期

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // キュー同期
        //=====================================================================

        /// キューでフェンスをシグナル
        /// @param queue シグナルするキュー
        /// @param fence フェンス
        /// @param value シグナル値
        virtual void SignalQueue(
            IRHIQueue* queue,
            IRHIFence* fence,
            uint64 value) = 0;

        /// キューでフェンスを待つ。
        /// @param queue 待機するキュー
        /// @param fence フェンス
        /// @param value 待機する値
        virtual void WaitQueue(
            IRHIQueue* queue,
            IRHIFence* fence,
            uint64 value) = 0;

        /// キューの全コマンド完了を待つ。
        /// @param queue 待機するキュー
        virtual void FlushQueue(IRHIQueue* queue) = 0;

        /// 全キューの全コマンド完了を待つ。
        virtual void FlushAllQueues() = 0;
    };
}
```

- [ ] SignalQueue / WaitQueue
- [ ] FlushQueue / FlushAllQueues

### 5. キュー間同期

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // キュー間同期
        //=====================================================================

        /// キュー間バリア挿入
        /// @param srcQueue 完了を待つキュー
        /// @param dstQueue 待機するキュー
        /// @note 内のでフェンスを使用
        virtual void InsertQueueBarrier(
            IRHIQueue* srcQueue,
            IRHIQueue* dstQueue) = 0;

        /// グラフィックス→コンピュート同期
        void SyncGraphicsToCompute() {
            InsertQueueBarrier(GetGraphicsQueue(), GetComputeQueue());
        }

        /// コンピュートのグラフィックス同期
        void SyncComputeToGraphics() {
            InsertQueueBarrier(GetComputeQueue(), GetGraphicsQueue());
        }

        /// コピー→グラフィックス同期
        void SyncCopyToGraphics() {
            InsertQueueBarrier(GetCopyQueue(), GetGraphicsQueue());
        }
    };
}
```

- [ ] InsertQueueBarrier
- [ ] 便利な同期メソッド

## 検証方法

- [ ] キュー取得の正確性
- [ ] 同期フローの整合性
- [ ] フォールバックの正確性

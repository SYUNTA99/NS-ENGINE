# 01-17b: サブミッションパイプライン — スレッドモデル・ペイロード管理

## 目的

レンダースレッド→サブミッションスレッド→インタラプトスレッドの3段パイプラインを定義し、コマンド記録・GPU送信・完了後処理を分離する。01-17がIDynamicRHI上のコマンド実行APIを定義するのに対し、本計画はその内部実装のスレッドモデルとペイロード管理を定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (コマンド実行管理)
- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドコンテキスト)

## 前提条件

- 01-17-dynamic-rhi-command（コマンド実行管理API）
- 01-25-queue-interface（IRHIQueue）
- 09-01-fence-interface（IRHIFence）
- 09-02-sync-point（RHISyncPoint — 本計画で参照）
- 02-04-command-allocator（コマンドアロケーター）
- 02-05-command-list（コマンドリスト）

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Internal/RHISubmission.h` — RHIPayload、サブミッションキュー型定義
- `Source/Engine/RHI/Private/RHISubmissionThread.h` — サブミッション/インタラプトスレッド
- `Source/Engine/RHI/Private/RHISubmissionThread.cpp`
- `Source/Engine/RHI/Private/RHIObjectPool.h` — コンテキスト/アロケーター/コマンドリストプール
- `Source/Engine/RHI/Private/RHIObjectPool.cpp`

## TODO

### 1. RHIPayload — サブミッション単位

コマンドリスト群と同期情報をまとめた、GPUへの送信単位。

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHIEnums.h"
#include "RHISyncPoint.h"

namespace NS::RHI
{
    /// サブミッション単位
    /// レンダースレッドが作成し、サブミッションスレッドがGPUに送信する
    struct RHI_API RHIPayload
    {
        //=================================================================
        // コマンド
        //=================================================================

        /// 送信先キュータイプ
        ERHIQueueType queueType = ERHIQueueType::Graphics;

        /// 送信するコマンドリスト群
        IRHICommandList** commandLists = nullptr;
        uint32 commandListCount = 0;

        //=================================================================
        // 同期
        //=================================================================

        /// GPU側待機ポイント（送信前にGPUが待つ）
        RHISyncPoint* waitPoints = nullptr;
        uint32 waitPointCount = 0;

        /// GPU側シグナルポイント（送信後にGPUがシグナル）
        RHISyncPoint* signalPoints = nullptr;
        uint32 signalPointCount = 0;

        /// このPayload完了時のフェンス値
        /// インタラプトスレッドがこの値の完了を監視する
        uint64 completionFenceValue = 0;

        //=================================================================
        // クエリ
        //=================================================================

        /// クエリ情報（Part 14で詳細定義予定）
        // IRHIQueryRange* queries = nullptr;
        // uint32 queryCount = 0;

        //=================================================================
        // リソース参照
        //=================================================================

        /// このPayloadで使用したコマンドアロケーター群
        /// 完了後にプールに返却される
        IRHICommandAllocator** usedAllocators = nullptr;
        uint32 usedAllocatorCount = 0;
    };
}
```

- [ ] RHIPayload 定義（RHISyncPointは09-02から#include）
- [ ] コマンドリスト群
- [ ] 待機/シグナルポイント
- [ ] 完了フェンス値
- [ ] クエリ情報（Part 14で詳細定義予定）
- [ ] 使用アロケーター参照

### 2. サブミッションパイプライン — 3段構成

```
┌──────────────────┐    ┌──────────────────────┐    ┌──────────────────────┐
│  レンダースレッド  │    │ サブミッションスレッド  │    │ インタラプトスレッド   │
│                  │    │                      │    │                      │
│ コマンド記録      │    │ Dequeue              │    │ Dequeue              │
│       ↓          │    │       ↓              │    │       ↓              │
│ Payload作成      │    │ バリア挿入・バッチ化   │    │ フェンス完了待ち      │
│       ↓          │    │       ↓              │    │       ↓              │
│ PendingSubmission│───→│ ExecuteCommandLists  │    │ リソース解放          │
│  にEnqueue       │    │       ↓              │    │ プール返却            │
│                  │    │ フェンスシグナル       │    │ コールバック呼び出し   │
│                  │    │       ↓              │    │                      │
│                  │    │ PendingInterrupt     │───→│                      │
│                  │    │  にEnqueue           │    │                      │
└──────────────────┘    └──────────────────────┘    └──────────────────────┘
```

#### 2a. サブミッションスレッド

```cpp
namespace NS::RHI::Private
{
    /// サブミッションスレッド
    /// PendingSubmissionキューからPayloadを取り出し、GPUに送信する
    class RHISubmissionThread
    {
    public:
        RHISubmissionThread() = default;
        ~RHISubmissionThread();

        /// スレッド開始
        void Start();

        /// スレッド停止（全Payload処理完了後）
        /// @param timeoutMs タイムアウト（ミリ秒）。0で無制限。
        ///        タイムアウト発生時はログ警告を出力し、
        ///        未処理Payloadのリソースを強制解放してスレッドをjoinする。
        void Stop(uint64 timeoutMs = 5000);

        /// Payloadをキューに投入（レンダースレッドから呼ばれる）
        void EnqueuePayload(RHIPayload&& payload);

    private:
        /// スレッドメインループ
        void ThreadMain();

        /// Payloadを処理（バリア挿入→ExecuteCommandLists→フェンスシグナル）
        void ProcessPayload(const RHIPayload& payload);

        /// バリア挿入・コマンドリストのバッチ化
        /// 同じキュータイプの連続Payloadをまとめて送信可能
        void BatchAndSubmit(IRHIQueue* queue,
                           IRHICommandList* const* commandLists,
                           uint32 count);

        //=================================================================
        // メンバー変数（実装必須。コメントは型・名前の設計指針）
        //=================================================================

        // スレッド
        std::thread m_thread;
        std::atomic<bool> m_running{false};

        // ペンディングキュー（MPSC: レンダースレッド→サブミッションスレッド）
        // 実装時に TMpscQueue or std::queue+mutex を選択
        // TMpscQueue<RHIPayload> m_pendingQueue;

        // ウェイクアップイベント
        std::condition_variable m_wakeEvent;
        std::mutex m_wakeMutex;

        // キューテーブル（キュータイプ→IRHIQueue*の解決用）
        IRHIQueue* m_queues[ERHIQueueType::Count] = {};

        // キューごとのフェンス（IRHIQueue::GetFence()経由で初期化時に取得）
        IRHIFence* m_queueFences[ERHIQueueType::Count] = {};

        // バッチ化状態（ProcessPayload内で管理）
        uint32 m_numCommandListsInBatch = 0;
        static constexpr uint32 kMaxBatchSize = 64;
    };
}
```

- [ ] RHISubmissionThread クラス定義
- [ ] Start / Stop
- [ ] EnqueuePayload
- [ ] ProcessPayload（バリア挿入、ExecuteCommandLists、フェンスシグナル）
- [ ] BatchAndSubmit
- [ ] メンバー変数（スレッド、キュー、ウェイクアップ、バッチ状態）

**ProcessPayloadフロー:**
```
ProcessPayload(payload):
  1. queue = m_queues[payload.queueType] でキュー解決
     → queue == nullptr: RHI_CHECK失敗（初期化時に全キュー登録済みが前提）
  2. GPU側待機（waitPoints処理）
  3. バリア挿入・バッチ化
  4. queue->ExecuteCommandLists()
  5. queue->Signal(m_queueFences[payload.queueType], payload.completionFenceValue)
  6. PendingInterruptにEnqueue（fence=m_queueFences[payload.queueType], fenceValue=completionFenceValue）
```

**バッチ化ルール:**
- 同一queueTypeの連続Payloadをバッチ化
- kMaxBatchSize（64）を超えたらバッチを分割して送信
- waitPointsを持つPayloadはバッチを切る（現バッチをフラッシュしてから新バッチ開始）

**Stop()のドレイン保証:**
- Stop()は m_running=false 後、m_pendingQueue が空になるまでループ継続
- 全Payloadを処理完了してからスレッドjoin
- デフォルトタイムアウト5000ms。超過時:
  1. ログ警告出力 "[RHI] Submission drain timeout - forcing shutdown"
  2. デバイスロスト状態をチェック（CheckAndLogDeviceLost()）
  3. 残存Payloadのアロケーターを強制返却
  4. スレッドをdetachせずjoin（OS側でスレッド終了を保証）

#### 2b. インタラプトスレッド

```cpp
namespace NS::RHI::Private
{
    /// 完了待ちエントリ
    struct PendingInterrupt
    {
        ERHIQueueType queueType = ERHIQueueType::Graphics;  // プール返却先の解決に使用
        IRHIFence* fence = nullptr;
        uint64 fenceValue = 0;

        /// 完了時に解放するアロケーター群
        IRHICommandAllocator** allocators = nullptr;
        uint32 allocatorCount = 0;

        /// 完了時に遅延削除キューから解放するリソース群
        IRHIResource** deferredResources = nullptr;
        uint32 deferredResourceCount = 0;

        /// 完了時コールバック（オプション）
        void (*callback)(void* userData) = nullptr;
        void* callbackUserData = nullptr;
    };

    /// インタラプトスレッド
    /// GPU完了を監視し、リソース解放・プール返却・コールバックを実行する
    class RHIInterruptThread
    {
    public:
        RHIInterruptThread() = default;
        ~RHIInterruptThread();

        /// スレッド開始
        void Start();

        /// スレッド停止
        /// @param timeoutMs フェンス待ちタイムアウト（ミリ秒）。0で無制限。
        ///        タイムアウト超過時はデバイスロスト状態をチェックし、
        ///        フェンス完了を待たずにリソースを強制解放する。
        void Stop(uint64 timeoutMs = 5000);

        /// 完了待ちエントリを投入（サブミッションスレッドから呼ばれる）
        void EnqueueInterrupt(PendingInterrupt&& entry);

    private:
        /// スレッドメインループ
        void ThreadMain();

        /// 完了処理（リソース解放、プール返却、コールバック）
        void ProcessCompletion(const PendingInterrupt& entry);

        //=================================================================
        // メンバー変数（実装必須。コメントは型・名前の設計指針）
        //=================================================================

        // スレッド
        std::thread m_thread;
        std::atomic<bool> m_running{false};

        // ペンディングキュー（SPSC: サブミッション→インタラプト）
        // 実装時に TSpscQueue or std::queue+mutex を選択
        // TSpscQueue<PendingInterrupt> m_pendingQueue;

        // ウェイクアップイベント
        std::condition_variable m_wakeEvent;
        std::mutex m_wakeMutex;

        // オブジェクトプール参照（返却先）
        RHIObjectPool* m_objectPools[ERHIQueueType::Count] = {};
    };
}
```

- [ ] PendingInterrupt 構造体
- [ ] RHIInterruptThread クラス定義
- [ ] Start / Stop
- [ ] EnqueueInterrupt
- [ ] ProcessCompletion（リソース解放、プール返却、コールバック呼び出し）

**Stop()のドレイン保証:**
- Stop()は m_running=false 後、m_pendingQueue が空になるまでループ継続
- 全PendingInterruptのフェンス完了待ち→リソース返却を完了してからjoin
- フェンス待ちのタイムアウトはデフォルト5000ms
- タイムアウト超過時: デバイスロスト状態をチェックし、
  フェンス完了を待たずにリソースを強制解放（リーク防止優先）

**ProcessCompletionフロー:**
```
ProcessCompletion(entry):
  1. fence->Wait(fenceValue) でGPU完了待ち
  2. アロケーター群をm_objectPools[entry.queueType]に返却
  3. 遅延削除リソースをRHIDeferredDeleteQueue::DeleteImmediate()で即座に解放
     （フェンス完了済みのため再待機不要）
  4. コールバック呼び出し（設定時）
```

**エラー処理方針:**
- ProcessCompletion中のコールバック例外: catchしてログ出力、次のエントリの処理を継続
- フェンスがデバイスロスト状態: 完了扱いでリソース解放を実行（リーク防止）

### 3. オブジェクトプール — コンテキスト/アロケーター/コマンドリストプール

キュータイプごとのコマンドアロケーター/コマンドリスト/コンテキストのプール管理。フェンス値による再利用可能判定を行う。

```cpp
namespace NS::RHI::Private
{
    /// プール内アロケーターエントリ
    struct PooledAllocator
    {
        IRHICommandAllocator* allocator = nullptr;
        IRHIFence* fence = nullptr;       // 完了監視用フェンス
        uint64 fenceValue = 0;            // この値が完了したら再利用可能
    };

    /// キューレベルのオブジェクトプール
    /// コマンドアロケーター/コマンドリスト/コンテキストをプール管理する
    class RHIObjectPool
    {
    public:
        explicit RHIObjectPool(ERHIQueueType queueType);
        ~RHIObjectPool();

        //=================================================================
        // コンテキスト管理
        //=================================================================

        /// コンテキスト取得（プールから再利用 or 新規作成）
        /// コンストラクタで指定されたキュータイプを使用
        /// @return 使用可能なコンテキスト
        IRHICommandContext* ObtainContext();

        /// コンテキスト返却
        /// @param context 返却するコンテキスト
        void ReleaseContext(IRHICommandContext* context);

        //=================================================================
        // アロケーター管理
        //=================================================================

        /// コマンドアロケーター取得
        /// フェンス値チェックにより再利用可能なものを優先返却
        /// @return 使用可能なアロケーター
        IRHICommandAllocator* ObtainCommandAllocator();

        /// コマンドアロケーター返却
        /// GPU完了後にReset()を呼び出してから利用可能プールに移動する
        /// （Reset()はTrim()内でフェンス完了確認後に実行）
        /// @param allocator 返却するアロケーター
        /// @param fence 完了監視用フェンス
        /// @param fenceValue 完了フェンス値
        void ReleaseCommandAllocator(
            IRHICommandAllocator* allocator,
            IRHIFence* fence,
            uint64 fenceValue);

        //=================================================================
        // コマンドリスト管理
        //=================================================================

        /// コマンドリスト取得
        IRHICommandList* ObtainCommandList(IRHICommandAllocator* allocator);

        /// コマンドリスト返却
        void ReleaseCommandList(IRHICommandList* commandList);

        //=================================================================
        // メンテナンス
        //=================================================================

        /// 完了済みリソースの回収
        /// フェンス値チェックで再利用可能になったアロケーターを利用可能プールに移動
        void Trim();

        /// 全リソース解放（シャットダウン時）
        void ReleaseAll();

    private:
        ERHIQueueType m_queueType;

        // 利用可能（リセット済み）アロケーター
        // PooledAllocator* m_availableAllocators;

        // GPU実行中（フェンス待ち）アロケーター
        // PooledAllocator* m_pendingAllocators;

        // 利用可能コマンドリスト
        // IRHICommandList** m_availableCommandLists;
    };
}
```

- [ ] RHIObjectPool クラス定義
- [ ] ObtainContext / ReleaseContext
- [ ] ObtainCommandAllocator / ReleaseCommandAllocator
- [ ] ObtainCommandList / ReleaseCommandList
- [ ] フェンス値による再利用可能判定
- [ ] Trim（完了済み回収）
- [ ] ReleaseAll（シャットダウン）

### 4. IDynamicRHIとの統合

01-17で定義されたIDynamicRHIのコマンド実行APIは、内部的にこのサブミッションパイプラインを使用する。

```
所有関係:
  IDynamicRHI → RHISubmissionThread（1つ、全キューを処理）
  IDynamicRHI → RHIInterruptThread（1つ、全キューの完了を処理）
  IDynamicRHI → m_objectPools[ERHIQueueType::Count]（キューごとに1インスタンス）

IDynamicRHI::SubmitCommandLists()
    → RHIPayload作成
    → RHISubmissionThread::EnqueuePayload()

IDynamicRHI::ObtainCommandAllocator(queueType)
    → m_objectPools[queueType]->ObtainCommandAllocator()

IDynamicRHI::ReleaseCommandAllocator()
    → RHIObjectPool::ReleaseCommandAllocator()

IDynamicRHI::FlushCommands()
    → サブミッションキュー排出
    → 全フェンス完了待ち

IDynamicRHI::FlushQueue(queueType)
    → サブミッションキュー全体を排出（FlushCommandsと同様）
    → 該当キューのフェンスのみ完了待ち
    ※スレッド集中型モデルではキュー別の選択的排出は行わず、
      全Payloadを処理した上で該当キューの完了のみを待つ方式を採用。
      これにより単一キューのFIFO不変条件を維持する。

初期化シーケンス（IDynamicRHI::Init内で実行）:
  1. 全キュータイプのIRHIQueue*をRHISubmissionThread::m_queues[]に登録
  2. 各キューからIRHIQueue::GetFence()でフェンスを取得し、m_queueFences[]に登録
  3. RHISubmissionThread::Start()
  4. RHIInterruptThread::Start()
  ※シャットダウンはStop()→スレッドjoin→プール解放の逆順
```

※ドキュメントとの差異:
  参照ドキュメントではFYourQueueがPendingSubmission/PendingInterruptキューと
  ObjectPoolを直接所有するが、本計画ではスレッド集中型モデルを採用。
  RHISubmissionThread/RHIInterruptThreadが全キュー分のPayloadを処理し、
  Payload.queueTypeで送信先を振り分ける。
  オブジェクトプールはIDynamicRHI実装クラスがキューごとに保持する。
  キュータイプ名のマッピング: ドキュメントのDirect=ERHIQueueType::Graphics、
  Async=ERHIQueueType::Compute、Copy=ERHIQueueType::Copy。

- [ ] SubmitCommandListsとPayloadの対応
- [ ] ObtainXxx/ReleaseXxxとプールの対応
- [ ] FlushCommandsのフロー
- [ ] FlushQueueのフロー

## 既存計画との関係

| 計画 | 関係 |
|------|------|
| 01-17 (コマンド実行管理) | 本計画はその内部実装のスレッドモデル |
| 02-04 (コマンドアロケーター) | RHIObjectPoolはIRHICommandAllocatorPoolの具象実装を内包。ObtainCommandAllocator/ReleaseCommandAllocatorが対応 |
| 02-05 (コマンドリスト) | Payloadに含まれる対象 |
| 09-01 (フェンス) | 完了判定に使用 |
| 09-02 (同期ポイント) | 前提: RHISyncPointを#includeで使用 |
| 01-25 (IRHIQueue) | ExecuteCommandListsの送信先 |
| 01-14 (遅延削除キュー) | インタラプトスレッドがリソース解放時に使用 |

## スレッドセーフティ

| 操作 | スレッド | 同期方針 |
|------|---------|---------|
| EnqueuePayload | レンダースレッド→サブミッション | ロックフリーキュー or mutex |
| EnqueueInterrupt | サブミッション→インタラプト | ロックフリーキュー or mutex |
| ObtainCommandAllocator | レンダースレッド | mutex（低頻度） |
| ReleaseCommandAllocator | インタラプトスレッド | mutex（低頻度） |
| Trim | インタラプトスレッド | 内部mutex |

## エラー処理方針

| エラー状況 | 対処 |
|-----------|------|
| ProcessPayload: queue解決失敗 | RHI_CHECK（初期化時に全キュー登録済みが前提。到達は論理エラー） |
| ProcessPayload: ExecuteCommandLists失敗 | デバイスロスト処理に委譲（00-02-error-handling参照） |
| ProcessCompletion: コールバック例外 | catchしてログ出力、次エントリの処理を継続 |
| ProcessCompletion: フェンスがデバイスロスト | 完了扱いでリソース解放を実行（リーク防止） |
| EnqueuePayload: キュー容量 | 動的キューのため容量制限なし（メモリ不足時はOOM） |
| Stop(): ドレイン | デフォルト5000msタイムアウト。超過時はログ警告+強制シャットダウン |

## 検証方法

### 設計レビュー
- [ ] 3段パイプラインのフロー整合性
- [ ] RHIPayloadと01-17のAPIの対応
- [ ] オブジェクトプールのフェンスベース再利用の正確性
- [ ] スレッドセーフティの妥当性
- [ ] デッドロック防止（キュー方向が一方向であることの確認）

### ユニットテスト対象
- [ ] RHIObjectPool: Obtain→Release→Obtain のサイクルでプール再利用が動作すること
- [ ] RHIObjectPool: フェンス未完了時にObtainが新規作成すること
- [ ] RHIObjectPool: Trimで完了済みアロケーターが利用可能プールに移動すること
- [ ] バッチ化ロジック: kMaxBatchSize超過で分割されること
- [ ] バッチ化ロジック: waitPoints付きPayloadでバッチが切れること

### モック戦略
- IRHIQueue: ExecuteCommandLists呼び出し回数・引数を記録するモック
- IRHIFence: GetCompletedValue()を制御可能なモック（フェンス完了をシミュレート）
- IRHICommandAllocator/IRHICommandList: 生成・破棄カウントを追跡するモック

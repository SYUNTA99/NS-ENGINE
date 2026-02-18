# D3D12RHI 実装ガイド Part 3: コマンド実行とサブミッション

## 目次

1. [コマンド実行アーキテクチャ概要](#1-コマンド実行アーキテクチャ概要)
2. [FD3D12ContextCommon](#2-fd3d12contextcommon)
3. [FD3D12CommandContext](#3-fd3d12commandcontext)
4. [FD3D12CommandList](#4-fd3d12commandlist)
5. [FD3D12CommandAllocator](#5-fd3d12commandallocator)
6. [FD3D12Payload とサブミッションパイプライン](#6-fd3d12payload-とサブミッションパイプライン)
7. [FD3D12SyncPoint と FD3D12Fence](#7-fd3d12syncpoint-と-fd3d12fence)
8. [サブミッション / 割り込みスレッド](#8-サブミッション--割り込みスレッド)
9. [遅延削除システム](#9-遅延削除システム)

---

## 1. コマンド実行アーキテクチャ概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│        D3D12RHI コマンド実行アーキテクチャ                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌───────────────────┐         │
│  │ RHI スレッド  │    │ レンダー     │    │ タスクグラフ      │         │
│  │  (RHI Thread) │    │ スレッド     │    │ ワーカー          │         │
│  └──────┬───────┘    └──────┬───────┘    └────────┬──────────┘         │
│         │                    │                      │                    │
│         ▼                    ▼                      ▼                    │
│  ┌──────────────────────────────────────────────────────────┐           │
│  │         FD3D12CommandContext (ContextCommon)              │           │
│  │  ・D3D12 コマンド記録                                    │           │
│  │  ・バリアバッチング                                      │           │
│  │  ・ステートキャッシュ管理                                 │           │
│  │  ・FD3D12CommandList の割り当て/クローズ                  │           │
│  └──────────────────────┬───────────────────────────────────┘           │
│                          │ Finalize()                                   │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────────┐           │
│  │         FD3D12Payload (ペイロード)                       │           │
│  │  ・Wait: SyncPointsToWait, QueueFencesToWait             │           │
│  │  ・Execute: CommandListsToExecute                        │           │
│  │  ・Signal: SyncPointsToSignal, ManualFencesToSignal      │           │
│  └──────────────────────┬───────────────────────────────────┘           │
│                          │ SubmitPayloads() → MPSC Queue               │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────────┐           │
│  │         Submission Thread (サブミッションスレッド)        │           │
│  │  ・ペイロードのバッチ化                                  │           │
│  │  ・GPU フェンス Wait/Signal                               │           │
│  │  ・ID3D12CommandQueue::ExecuteCommandLists 呼び出し       │           │
│  │  ・レジデンシーセット管理                                 │           │
│  └──────────────────────┬───────────────────────────────────┘           │
│                          │ GPU 完了                                     │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────────┐           │
│  │         Interrupt Thread (割り込みスレッド)               │           │
│  │  ・フェンス完了検出                                      │           │
│  │  ・FGraphEvent シグナル                                   │           │
│  │  ・タイムスタンプ / クエリ結果処理                        │           │
│  │  ・オブジェクトプールへのリサイクル                        │           │
│  └──────────────────────────────────────────────────────────┘           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. FD3D12ContextCommon

### 2.1 クラス概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12ContextCommon (D3D12CommandContext.h)                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12FinalizedCommands インスタンスの記録を管理する基底クラス。        │
│  コマンドリストとアロケータの作成/リサイクルロジックを担当。            │
│                                                                         │
│  直接 D3D12 コマンドを記録し、ペイロードにパッケージングする。          │
│  バリアバッチング、SyncPoint の Wait/Signal を管理。                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 主要メンバ変数

| メンバ | 型 | 説明 |
|--------|-----|------|
| `Device` | `FD3D12Device* const` | 所有デバイス |
| `QueueType` | `ED3D12QueueType const` | キュータイプ |
| `bIsDefaultContext` | `bool const` | デフォルトコンテキストか |
| `CommandList` | `FD3D12CommandList*` | アクティブなコマンドリスト |
| `CommandAllocator` | `FD3D12CommandAllocator*` | 現在のコマンドアロケータ |
| `Payloads` | `TArray<FD3D12Payload*>` | 記録済みペイロード配列 |
| `ContextSyncPoint` | `FD3D12SyncPointRef` | コンテキスト完了 SyncPoint |
| `BatchedSyncPoints.ToWait` | `TArray<FD3D12SyncPointRef>` | バッチ先頭の Wait |
| `BatchedSyncPoints.ToSignal` | `TArray<FD3D12SyncPointRef>` | バッチ末尾の Signal |
| `Barriers` | `TUniquePtr<BarriersForContextType>` | バリアバッチングマネージャ |
| `TimestampQueries` | `FD3D12QueryAllocator` | タイムスタンプクエリアロケータ |
| `OcclusionQueries` | `FD3D12QueryAllocator` | オクルージョンクエリアロケータ |
| `PipelineStatsQueries` | `FD3D12QueryAllocator` | パイプラインステータスクエリ |
| `CurrentPhase` | `EPhase` | ペイロード内の現在フェーズ |
| `ActiveQueries` | `uint32` | アクティブなクエリ数 |

### 2.3 ペイロードフェーズモデル

```
┌─────────────────────────────────────────────────────────────────────────┐
│        Payload フェーズ順序 (EPhase)                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Wait → UpdateReservedResources → Execute → Signal                    │
│                                                                         │
│  ・GetPayload(Phase) で現在フェーズより前のフェーズが要求された場合     │
│    新しいペイロードを生成 (フェーズは単調増加でなければならない)        │
│  ・1コンテキストが複数ペイロードを生成することで                        │
│    コマンドリストの分割を実現                                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.4 コマンドリスト操作

| メソッド | 説明 |
|---------|------|
| `OpenCommandList()` | 新しいコマンドリストを開く (CommandAllocator 取得、CommandList 取得) |
| `CloseCommandList()` | 現在のコマンドリストをクローズしてペイロードに追加 |
| `GetCommandList()` | 現在のコマンドリスト取得 (未オープンなら自動オープン) |
| `OpenIfNotAlready()` | コマンドリストが未オープンなら開く |
| `FlushCommands(FlushFlags)` | ペンディングコマンドを GPU にフラッシュ |
| `ConditionalSplitCommandList()` | コマンド数が閾値超過時にコマンドリストを分割 |
| `Finalize(OutPayloads)` | 記録完了。全ペイロードを返し、コンテキストをリセット |

### 2.5 バリアとリソース遷移

| メソッド | 説明 |
|---------|------|
| `AddGlobalBarrier(Before, After)` | グローバルバリア追加 (UAV バリア等) |
| `AddBarrier(Resource, Before, After, Subresource)` | リソースバリア追加 |
| `FlushResourceBarriers()` | バッチ化バリアをコマンドリストに記録 |

### 2.6 同期操作

| メソッド | 説明 |
|---------|------|
| `SignalSyncPoint(SyncPoint)` | GPU キューで SyncPoint をシグナル |
| `WaitSyncPoint(SyncPoint)` | GPU キューで SyncPoint を待機 |
| `SignalManualFence(Fence, Value)` | D3D12 フェンスをシグナル |
| `WaitManualFence(Fence, Value)` | D3D12 フェンスを待機 |
| `InsertTimestamp(Units, Target)` | タイムスタンプクエリ挿入 |
| `AllocateQuery(Type, Target)` | クエリ割り当て |
| `GetContextSyncPoint()` | コンテキスト全体の完了 SyncPoint (遅延作成) |

### 2.7 コマンドリストアクセスパターン

```
┌─────────────────────────────────────────────────────────────────────────┐
│        コマンドリストアクセス (TRValuePtr パターン)                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12ContextCommon はコマンドリストインターフェースを                  │
│  直接公開するラッパーメソッドを提供:                                    │
│                                                                         │
│  ・GraphicsCommandList()  → コマンドリストの GL  を取得                │
│  ・GraphicsCommandList4() → GL4 (レイトレーシング) を取得              │
│  ・CopyCommandList()      → コピーコマンドリストを取得                 │
│  ・GraphicsCommandList10()→ GL10 (Work Graph) を取得                  │
│                                                                         │
│  TRValuePtr<T> はコマンドリストへのアクセスを追跡:                     │
│  ・operator-> で NumCommands++ (有用なコマンドをカウント)              │
│  ・r-value 参照のみ許可 (l-value 保存禁止)                             │
│    → コマンドリスト分割時の陳腐化を防止                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. FD3D12CommandContext

### 3.1 継承チェーン

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12CommandContext 継承階層                                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  IRHICommandContext (RHI 公開インターフェース)                          │
│    │                                                                    │
│    ▼                                                                    │
│  FD3D12CommandContextBase : IRHICommandContext, FD3D12AdapterChild      │
│    │  ・GPUMask / PhysicalGPUMask                                      │
│    │  ・RHIEndDrawingViewport()                                        │
│    │  ・AsRedirector() → nullptr (デフォルト)                          │
│    │                                                                    │
│    ▼                                                                    │
│  FD3D12CommandContext : FD3D12ContextCommon, FD3D12CommandContextBase,  │
│    │                    FD3D12DeviceChild                              │
│    │  ・ステートキャッシュ (FD3D12StateCache)                           │
│    │  ・ディスクリプタキャッシュ (FD3D12DescriptorCache)                │
│    │  ・全 RHI コマンド実装 (DrawPrimitive, Dispatch 等)               │
│    │  ・IRHIComputeContext インターフェース                              │
│    │                                                                    │
│    ▼                                                                    │
│  FD3D12ContextCopy : FD3D12ContextCommon (final)                       │
│    ・コピーキュー専用 (RHI インターフェース非実装)                       │
│    ・バッファ/テクスチャデータ転送に使用                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 FD3D12CommandContextRedirector

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12CommandContextRedirector (MGPU 用)                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  マルチ GPU (GNumExplicitGPUsForRendering > 1) 時に使用。              │
│  GPU マスクに基づき、コマンドを適切な FD3D12CommandContext に           │
│  ルーティングする。                                                     │
│                                                                         │
│  ・FD3D12Adapter::DefaultContextRedirector で管理                      │
│  ・AsRedirector() で識別                                               │
│  ・シングル GPU 時は使用されない                                       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.3 FD3D12CopyScope (コピーヘルパー)

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12CopyScope (D3D12CommandContext.h)                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  コピーキュー作業の記録・サブミットのための RAII ヘルパー。             │
│                                                                         │
│  コンストラクタ:                                                        │
│  ・FD3D12ContextCopy を Device から取得                                │
│  ・オプションで WaitSyncPoint 設定                                     │
│  ・SyncPoint を自動生成                                                │
│                                                                         │
│  デストラクタ:                                                          │
│  ・コンテキストを Finalize                                             │
│  ・ペイロードをサブミット                                               │
│  ・コンテキストを解放                                                   │
│                                                                         │
│  GetSyncPoint() で完了待ちに使用する SyncPoint を取得。                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. FD3D12CommandList

### 4.1 クラス概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12CommandList final (D3D12CommandList.h)                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  D3D12 コマンドリストオブジェクトのラッパー。                           │
│  コマンドコンテキストとサブミッションスレッドに必要な追加データを含む。  │
│  デバイスのオブジェクトプールから取得/返却される。                       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.2 ネイティブインターフェース (FInterfaces)

| インターフェース | 条件 | 説明 |
|---------------|------|------|
| `CommandList` | 常に | ID3D12CommandList (基底) |
| `CopyCommandList` | 常に | ID3D12CopyCommandList (=ID3D12GraphicsCommandList on Windows) |
| `GraphicsCommandList` | 常に | ID3D12GraphicsCommandList |
| `GraphicsCommandList1〜10` | D3D12_MAX_COMMANDLIST_INTERFACE >= N | 各バージョンインターフェース |
| `DebugCommandList` | D3D12_SUPPORTS_DEBUG_COMMAND_LIST | デバッグコマンドリスト |
| `AftermathHandle` | NV_AFTERMATH | NVIDIA Aftermath ハンドル |
| `IntelCommandListHandle` | INTEL_GPU_CRASH_DUMPS | Intel クラッシュダンプ |

### 4.3 FState (リセット可能状態)

| フィールド | 型 | 説明 |
|-----------|-----|------|
| `CommandAllocator` | `FD3D12CommandAllocator*` | 割り当てられたアロケータ |
| `BeginTimestamp` | `FD3D12QueryLocation` | 開始タイムスタンプ |
| `EndTimestamp` | `FD3D12QueryLocation` | 終了タイムスタンプ |
| `PipelineStats` | `FD3D12QueryLocation` | パイプライン統計クエリ |
| `TimestampQueries` | `TArray<FD3D12QueryLocation>` | タイムスタンプクエリ配列 |
| `OcclusionQueries` | `TArray<FD3D12QueryLocation>` | オクルージョンクエリ配列 |
| `PipelineStatsQueries` | `TArray<FD3D12QueryLocation>` | パイプライン統計クエリ配列 |
| `DeferredResidencyUpdateSet` | `TSet<const FD3D12Resource*>` | 遅延レジデンシー更新リソース |
| `NumCommands` | `uint32` | 記録されたコマンド数 |
| `IsClosed` | `bool` | クローズ済みフラグ |
| `ResourceBarriers` | `TArray<D3D12_RESOURCE_BARRIER>` | デバッグ用バリア追跡 (`#if DEBUG_RESOURCE_STATES`) |
| `EventStream` | `FEventStream` | GPU プロファイラーイベント (`#if RHI_NEW_GPU_PROFILER`) |
| `BeginEvents` | `TArray<FBeginEventData>` | 開始イベントデータ (`#if RHI_NEW_GPU_PROFILER`) |

### 4.4 ライフサイクル

```
┌─────────────────────────────────────────────────────────────────────────┐
│        FD3D12CommandList ライフサイクル                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────┐    ┌───────────┐    ┌──────────┐    ┌──────────────┐     │
│  │ Obtain  │───▶│  Record   │───▶│  Close   │───▶│  Submit      │     │
│  │ (取得)  │    │  (記録)   │    │  (閉)    │    │  (実行)      │     │
│  └─────────┘    └───────────┘    └──────────┘    └──────┬───────┘     │
│       ▲                                                   │             │
│       │                                                   ▼             │
│  ┌─────────┐                                     ┌──────────────┐     │
│  │ Reset   │◀────────────────────────────────────│  Recycle     │     │
│  │ (再初期化)│                                    │  (返却)      │     │
│  └─────────┘                                     └──────────────┘     │
│                                                                         │
│  1. Device::ObtainCommandList(Allocator, Timestamps, PipelineStats)    │
│     ・ObjectPool.Lists から取得 (なければ新規作成)                      │
│     ・Reset() で State を初期化                                        │
│     ・BeginLocalQueries() でタイムスタンプ開始                         │
│                                                                         │
│  2. コマンド記録                                                        │
│     ・GraphicsCommandList()->DrawInstanced() 等                        │
│     ・NumCommands が TRValuePtr のアクセスでインクリメント             │
│                                                                         │
│  3. Close()                                                            │
│     ・EndLocalQueries() でタイムスタンプ終了                            │
│     ・ID3D12GraphicsCommandList::Close() 呼び出し                      │
│     ・IsClosed = true                                                  │
│                                                                         │
│  4. Submit (サブミッションスレッド)                                      │
│     ・ID3D12CommandQueue::ExecuteCommandLists()                        │
│     ・フェンス Signal                                                   │
│                                                                         │
│  5. Recycle (割り込みスレッド)                                           │
│     ・GPU 完了後、Device::ReleaseCommandList() でプールに返却          │
│     ・CommandAllocator も別途プールに返却                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.5 レジデンシー管理

```
┌─────────────────────────────────────────────────────────────────────────┐
│        コマンドリストのレジデンシー管理                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  各 FD3D12CommandList は FD3D12ResidencySet を所有。                    │
│                                                                         │
│  ・UpdateResidency(Resource)                                           │
│    即座にレジデンシーハンドルを追加、または                              │
│    DeferredResidencyUpdateSet に遅延登録                                │
│    (Reserved Resource 等、ハンドルが未確定の場合)                       │
│                                                                         │
│  ・AddToResidencySet(Handles)                                          │
│    レジデンシーハンドルを直接追加                                       │
│                                                                         │
│  ・CloseResidencySet() [サブミッション時]                               │
│    ResidencySet をクローズして返す                                      │
│    → ExecuteCommandLists() と共に渡される                              │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 5. FD3D12CommandAllocator

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12CommandAllocator final (D3D12CommandList.h)                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  D3D12 コマンドアロケータオブジェクトのラッパー。                       │
│  デバイスのオブジェクトプールから取得/返却される。                       │
│                                                                         │
│  ・Device      (FD3D12Device* const)                                   │
│  ・QueueType   (ED3D12QueueType const)                                 │
│  ・CommandAllocator (TRefCountPtr<ID3D12CommandAllocator>)              │
│                                                                         │
│  ・Reset() — GPU がこのアロケータのコマンドリスト実行を完了した後に    │
│    呼び出す。メモリを再利用可能にする。                                  │
│                                                                         │
│  ライフサイクル:                                                        │
│  1. Device::ObtainCommandAllocator(QueueType) で取得                    │
│  2. コマンドリストに割り当て                                             │
│  3. Payload.AllocatorsToRelease に追加                                  │
│  4. 割り込みスレッドで GPU 完了後 Reset() → プールに返却               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 6. FD3D12Payload とサブミッションパイプライン

### 6.1 FD3D12PayloadBase

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12PayloadBase (D3D12Submission.h)                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  単一の GPU ノード・キュータイプに特化した作業単位。                     │
│  サブミッションスレッドが処理する。                                      │
│                                                                         │
│  【構造: 4フェーズ】                                                    │
│                                                                         │
│  ┌─ Wait ──────────────────────────────────────┐                       │
│  │  SyncPointsToWait     — GPU 側 SyncPoint 待機                      │
│  │  QueueFencesToWait    — キューフェンス待機                          │
│  │  ManualFencesToWait   — 手動フェンス待機                            │
│  └─────────────────────────────────────────────┘                       │
│                    ▼                                                    │
│  ┌─ UpdateReservedResources ───────────────────┐                       │
│  │  ReservedResourcesToCommit — 物理メモリコミット                     │
│  └─────────────────────────────────────────────┘                       │
│                    ▼                                                    │
│  ┌─ Execute ───────────────────────────────────┐                       │
│  │  PreExecuteCallback   — TFunction<void(ID3D12CommandQueue*)>      │
│  │  CommandListsToExecute — コマンドリスト配列                        │
│  └─────────────────────────────────────────────┘                       │
│                    ▼                                                    │
│  ┌─ Signal ────────────────────────────────────┐                       │
│  │  ManualFencesToSignal — 手動フェンスシグナル                        │
│  │  SyncPointsToSignal   — SyncPoint シグナル                         │
│  │  CompletionFenceValue — キューフェンス値                            │
│  │  SubmissionEvent      — サブミッション完了イベント                  │
│  │  Timing               — TOptional<FD3D12Timing*> GPUタイミング     │
│  │  bAlwaysSignal        — bool (空ペイロードでもシグナル)             │
│  │  bSubmitted           — std::atomic<bool>                          │
│  │  SubmissionTime       — TOptional<uint64>                          │
│  └─────────────────────────────────────────────┘                       │
│                                                                         │
│  【クリーンアップ】                                                      │
│  AllocatorsToRelease — 返却するコマンドアロケータ                      │
│  BatchedObjects      — バッチ化クエリオブジェクト                      │
│  BreadcrumbRange     — ブレッドクラム範囲 (`#if WITH_RHI_BREADCRUMBS`)│
│  BreadcrumbAllocators — ブレッドクラムアロケータ (`#if WITH_RHI_BREADCRUMBS`)│
│  EventStream         — GPUプロファイラーイベント (`#if RHI_NEW_GPU_PROFILER`)│
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.2 FManualFence

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12PayloadBase::FManualFence                                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  外部プラグインやクロスキュー同期で使用する手動フェンス:                │
│  ・Fence (TRefCountPtr<ID3D12Fence>) — D3D12 フェンスオブジェクト     │
│  ・Value (uint64) — シグナル/待機する値                                │
│                                                                         │
│  ManualFencesToWait:  GPU キューで待機するフェンス                     │
│  ManualFencesToSignal: GPU キューでシグナルするフェンス                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.3 サブミッションフロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│        ペイロードサブミッションフロー                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. FD3D12ContextCommon::Finalize(OutPayloads)                         │
│     ・現在のコマンドリストをクローズ                                    │
│     ・BatchedSyncPoints を先頭/末尾ペイロードに設定                    │
│     ・Payloads 配列を OutPayloads に追加                                │
│                                                                         │
│  2. FD3D12DynamicRHI::SubmitPayloads(Payloads)                         │
│     ・ペイロード配列を MPSC キューに投入                                │
│     ・PendingPayloadsForSubmission (TQueue, MPSC)                      │
│                                                                         │
│  3. Submission Thread: ProcessSubmissionQueue()                          │
│     ・MPSC キューからペイロードバッチを取得                             │
│     ・各キューの FD3D12Queue::PendingSubmission に振り分け             │
│     ・FlushBatchedPayloads() で実際のサブミッション実行:               │
│                                                                         │
│     for each Queue:                                                    │
│       a. Wait フェーズ                                                 │
│          ・SyncPointsToWait の解決を確認                                │
│          ・QueueFencesToWait → ID3D12CommandQueue::Wait()              │
│          ・ManualFencesToWait → ID3D12CommandQueue::Wait()             │
│                                                                         │
│       b. UpdateReservedResources フェーズ                               │
│          ・予約リソースの物理メモリコミット                              │
│          ・クロスキュー同期 (必要時)                                    │
│                                                                         │
│       c. Execute フェーズ                                               │
│          ・PreExecuteCallback 実行 (RHIRunOnQueue 用)                  │
│          ・FD3D12Queue::FinalizePayload()                              │
│            → コマンドリストをバッチ化                                  │
│            → ID3D12CommandQueue::ExecuteCommandLists()                 │
│            → ID3D12Fence::Signal()                                     │
│                                                                         │
│       d. Signal フェーズ                                                │
│          ・ManualFencesToSignal → ID3D12CommandQueue::Signal()         │
│          ・SyncPointsToSignal に ResolvedFence を設定                  │
│          ・SubmissionEvent をトリガー                                   │
│                                                                         │
│     ・処理済みペイロードを PendingInterrupt に移動                     │
│                                                                         │
│  4. Interrupt Thread: ProcessInterruptQueue()                           │
│     ・PendingInterrupt からペイロードを取得                             │
│     ・GPU フェンス完了を検出 (LastSignaledValue チェック)               │
│     ・完了した SyncPoint の FGraphEvent をシグナル                     │
│     ・タイムスタンプ / クエリ結果を処理                                  │
│     ・コマンドアロケータ / コマンドリストをプールに返却                 │
│     ・ペイロード削除                                                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.4 FD3D12BatchedPayloadObjects

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12BatchedPayloadObjects (D3D12Submission.h)                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  サブミッションスレッドがキューごとにバッチ化するオブジェクト群:         │
│                                                                         │
│  ・TimestampQueries    — タイムスタンプクエリ                          │
│  ・OcclusionQueries    — オクルージョンクエリ                          │
│  ・PipelineStatsQueries — パイプライン統計クエリ                       │
│  ・QueryRanges         — ヒープごとのクエリ範囲 (TMap)                │
│                                                                         │
│  IsEmpty() で空チェック (3配列 + QueryRanges TMap が全て空なら true)   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 7. FD3D12SyncPoint と FD3D12Fence

### 7.1 FD3D12SyncPoint

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12SyncPoint final : FThreadSafeRefCountedObject                   │
│  (D3D12Submission.h)                                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  GPU キュータイムライン上の論理的ポイント。                              │
│  他のキューや CPU から待機可能。                                        │
│  D3D12 フェンスを抽象化する。                                           │
│                                                                         │
│  【特性】                                                                │
│  ・ワンショット (単一タイムラインポイント)                               │
│  ・RefCount ベースの解放                                                │
│  ・メモリプール (TLockFreePointerListUnordered) で高速割当              │
│                                                                         │
│  【タイプ】                                                              │
│  ・ED3D12SyncPointType::GPUOnly                                        │
│    → FGraphEvent なし。CPU からの完了確認不可                          │
│  ・ED3D12SyncPointType::GPUAndCPU                                      │
│    → FGraphEvent あり。IsComplete() / Wait() で CPU 側から検査可能    │
│                                                                         │
│  【使い方】                                                              │
│  1. FD3D12SyncPoint::Create(Type, DebugName) で作成                    │
│  2. ペイロードの SyncPointsToSignal に追加                              │
│  3. サブミッションスレッドが ResolvedFence を設定                       │
│  4. 割り込みスレッドが GPU 完了後 GraphEvent をシグナル                 │
│                                                                         │
│  【内部状態】                                                            │
│  ・ResolvedFence (TOptional<FD3D12ResolvedFence>)                      │
│    → サブミッションスレッドが設定するフェンス + 値                     │
│  ・GraphEvent (FGraphEventRef)                                          │
│    → GPUAndCPU 時のみ存在。完了時にシグナルされる                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 7.2 FD3D12ResolvedFence

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12ResolvedFence (D3D12Submission.h)                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  SyncPoint と実際のフェンス値を紐付ける:                                │
│  ・Fence (FD3D12Fence&) — キューのフェンスへの参照                    │
│  ・Value (uint64) — シグナルされたフェンス値                           │
│                                                                         │
│  サブミッションスレッドが SyncPoint に設定し、                          │
│  割り込みスレッドが Fence.LastSignaledValue >= Value で完了を検出。     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 7.3 FD3D12CommitReservedResourceDesc

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12CommitReservedResourceDesc (D3D12Submission.h)                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Reserved Resource の物理メモリコミット記述:                             │
│  ・Resource         (FD3D12Resource*) — 対象リソース                   │
│  ・CommitSizeInBytes (uint64) — コミットするサイズ                     │
│                                                                         │
│  FD3D12ContextCommon::SetReservedBufferCommitSize() から設定され、      │
│  サブミッションスレッドの UpdateReservedResources フェーズで処理される。│
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 8. サブミッション / 割り込みスレッド

### 8.1 スレッドアーキテクチャ

```
┌─────────────────────────────────────────────────────────────────────────┐
│        D3D12RHI スレッドモデル                                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌─────────────────────────────────────────────────────┐               │
│  │  RHI/Render/Task スレッド群                         │               │
│  │  ・コマンドコンテキストに D3D12 コマンドを記録      │               │
│  │  ・Finalize() でペイロード生成                      │               │
│  │  ・SubmitPayloads() で MPSC キューに投入            │               │
│  └──────────────────────┬──────────────────────────────┘               │
│                          │                                              │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────┐              │
│  │  PendingPayloadsForSubmission (MPSC Queue)           │              │
│  │  TQueue<TArray<FD3D12Payload*>*, EQueueMode::Mpsc>   │              │
│  └──────────────────────┬───────────────────────────────┘              │
│                          │                                              │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────┐              │
│  │  Submission Thread (FD3D12Thread)                    │              │
│  │                                                      │              │
│  │  ProcessSubmissionQueue():                           │              │
│  │  ・MPSC キューからバッチ取得                         │              │
│  │  ・キューごとの PendingSubmission に振り分け         │              │
│  │  ・FlushBatchedPayloads():                           │              │
│  │    - Wait (GPU fence wait)                           │              │
│  │    - UpdateReservedResources                         │              │
│  │    - Execute (ExecuteCommandLists)                   │              │
│  │    - Signal (GPU fence signal)                       │              │
│  │  ・PendingInterrupt にペイロードを移動               │              │
│  │                                                      │              │
│  │  SubmissionCS (FCriticalSection) で保護               │              │
│  └──────────────────────┬───────────────────────────────┘              │
│                          │                                              │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────┐              │
│  │  FD3D12Queue::PendingInterrupt (SPSC Queue)         │              │
│  │  TSpscQueue<FD3D12Payload*>                          │              │
│  └──────────────────────┬───────────────────────────────┘              │
│                          │                                              │
│                          ▼                                              │
│  ┌──────────────────────────────────────────────────────┐              │
│  │  Interrupt Thread (FD3D12Thread)                     │              │
│  │                                                      │              │
│  │  ProcessInterruptQueue():                            │              │
│  │  ・各キューの PendingInterrupt をチェック            │              │
│  │  ・GPU フェンス完了検出 (LastSignaledValue)          │              │
│  │  ・完了ペイロードの処理:                             │              │
│  │    - SyncPoint の GraphEvent シグナル                │              │
│  │    - タイムスタンプ/クエリ結果コールバック           │              │
│  │    - CommandAllocator Reset + プール返却             │              │
│  │    - CommandList プール返却                          │              │
│  │    - ペイロード delete                               │              │
│  │                                                      │              │
│  │  InterruptCS (FCriticalSection) で保護                │              │
│  └──────────────────────────────────────────────────────┘              │
│                                                                         │
│  ※ D3D12_USE_INTERRUPT_THREAD=0 の場合、割り込み処理は                 │
│    ProcessInterruptQueueUntil() を呼び出すスレッドで実行                │
│    InterruptThreadID で「代理」スレッドを追跡                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.2 EQueueStatus

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FD3D12DynamicRHI::EQueueStatus                                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  None      = 0       — 何もなし                                        │
│  Processed = 1 << 0  — 作業が処理された                                │
│  Pending   = 1 << 1  — 未処理の作業が残っている                        │
│                                                                         │
│  FProcessResult:                                                       │
│  ・Status (EQueueStatus) — 処理結果                                    │
│  ・WaitTimeout (uint32 = INFINITE) — 次のウェイクアップまでの待機時間  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 8.3 フラッシュフラグ

```
┌─────────────────────────────────────────────────────────────────────────┐
│  ED3D12FlushFlags                                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  None              = 0  — 非同期サブミット (待機なし)                  │
│  WaitForSubmission = 1  — サブミッションスレッドが全作業をディスパッチ │
│                           するまで呼び出しスレッドをブロック            │
│  WaitForCompletion = 2  — GPU が全ディスパッチ作業を完了するまで       │
│                           呼び出しスレッドをブロック                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 9. 遅延削除システム

### 9.1 概要

```
┌─────────────────────────────────────────────────────────────────────────┐
│        遅延削除システム                                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  GPU がリソースを参照している間は削除できない。                          │
│  D3D12RHI は End-of-Pipe パターンで安全な削除を実現する。               │
│                                                                         │
│  1. DeferredDelete(Args...) 呼び出し                                   │
│     ・ObjectsToDelete 配列に FD3D12DeferredDeleteObject を追加         │
│     ・ObjectsToDeleteCS (FCriticalSection) で保護                       │
│                                                                         │
│  2. RHIProcessDeleteQueue() [フレーム終了時]                            │
│     ・ObjectsToDelete を Local にスワップ                               │
│     ・EnqueueEndOfPipeTask() で GPU 完了後タスクを登録                  │
│                                                                         │
│  3. EnqueueEndOfPipeTask(TaskFunc)                                      │
│     ・全キュー × 全デバイスでフェンスペイロードを生成                  │
│     ・各ペイロードに SyncPoint を追加                                   │
│     ・全 SyncPoint 完了後に TaskFunc を実行                             │
│     ・FFunctionGraphTask::CreateAndDispatchWhenReady() で登録          │
│                                                                         │
│  4. 実際の削除 (End-of-Pipe タスク内)                                   │
│     ・タイプに応じた削除処理:                                           │
│       RHIObject → Release() (RefCount == 1 検証)                      │
│       Heap → Release()                                                 │
│       DescriptorHeap → ImmediateFreeHeap()                             │
│       D3DObject → Release()                                            │
│       BindlessDescriptor → ImmediateFree()                             │
│       CPUAllocation → FMemory::Free()                                  │
│       Func → operator()() + delete                                     │
│       等                                                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 9.2 FD3D12DeferredDeleteObject タイプ一覧

| EType | Union メンバ | 削除処理 |
|-------|-------------|---------|
| `RHIObject` | `FD3D12Resource*` | `Release()` (RefCount==1 チェック) |
| `Heap` | `FD3D12Heap*` | `Release()` (追加参照あり得る) |
| `DescriptorHeap` | `FD3D12DescriptorHeap*` | `ImmediateFreeHeap()` |
| `D3DObject` | `ID3D12Object*` | `Release()` |
| `BindlessDescriptor` | `{Handle, Device}` | `ImmediateFree()` |
| `BindlessDescriptorHeap` | `FD3D12DescriptorHeap*` | `Recycle()` |
| `CPUAllocation` | `void*` | `FMemory::Free()` |
| `DescriptorBlock` | `{Block, Manager}` | `Manager->Recycle(Block)` |
| `VirtualAllocation` | `{VirtualBlock, Flags, ...}` | `DestroyVirtualTexture()` |
| `Func` | `TUniqueFunction<void()>*` | `(*Func)(); delete Func` |
| `TextureStagingBuffer` | `{Texture, LockedResource, Subresource}` | `ReuseStagingBuffer()` + `Release()` |

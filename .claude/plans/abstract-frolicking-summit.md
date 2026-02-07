# plan-doc-checker 指摘修正（第5ラウンド）

## Context

第4ラウンド修正適用後の plan-doc-checker (スコア91/100) で検出された WARNING 3件 + INFO 1件を修正する。
加えて前回計画済み（未適用）の14件も統合し、計18件の修正を一括適用する。

## 修正対象ファイル

| ファイル | 修正数 |
|---------|--------|
| `.claude/plans/rhi/01-17b-submission-pipeline.md` | 17件 |
| `.claude/plans/rhi/01-25-queue-interface.md` | 1件 (GetFence追加) |

---

## 既存14件（前回計画済み・未適用）

### FIX-1 [CRITICAL]: 01-25にGetFence()を追加

**対象**: `.claude/plans/rhi/01-25-queue-interface.md` — Synchronizationセクション(Signal/Wait付近)

**修正**: Signal/Wait/Flushと同じSynchronizationセクションに追加:
```cpp
        /// このキューに紐付くフェンスを取得
        /// @return キュー内部のフェンス（キューの寿命と同じ）
        virtual IRHIFence* GetFence() const = 0;
```

### FIX-2 [WARNING]: ERHIQueueType::Count配列サイズをstatic_cast化

**対象**: 01-17b L174, L177, L281, L419 (4箇所)

**修正**: `ERHIQueueType::Count` → `static_cast<uint32>(ERHIQueueType::Count)` に全4箇所を変更。

### FIX-3 [WARNING]: waitPoints処理の具体化

**対象**: 01-17b ProcessPayloadフロー L198

**修正**: Step 2を具体化:
```
  2. waitPoints処理:
     for i = 0..payload.waitPointCount-1:
       if payload.waitPoints[i].IsValid():
         queue->Wait(payload.waitPoints[i].fence, payload.waitPoints[i].value)
```

### FIX-4 [WARNING]: RHIObjectPoolにmutexメンバー追加

**対象**: 01-17b L386-397 (RHIObjectPool private members)

**修正**: privateメンバーに追加:
```cpp
        // スレッド同期（Obtain=レンダースレッド / Release=インタラプトスレッド）
        std::mutex m_mutex;
```

### FIX-5 [WARNING]: バリアアロケーター管理の追加

**対象**: 01-17b RHISubmissionThread privateメンバー(L179付近) + ProcessPayloadフロー(L199)

**修正A**: RHISubmissionThreadのメンバー変数に追加:
```cpp
        // バリア挿入用アロケーター（キューごと、サブミッションスレッド専用）
        IRHICommandAllocator* m_barrierAllocators[static_cast<uint32>(ERHIQueueType::Count)] = {};
```

**修正B**: ProcessPayloadフロー Step 3を具体化:
```
  3. バリア挿入・バッチ化:
     a. 必要ならm_barrierAllocators[payload.queueType]でバリアコマンドリスト記録
     b. バリアコマンドリストをバッチ先頭に挿入
     c. BatchAndSubmit()でGPUに送信
```

### FIX-6 [WARNING]: DeleteImmediateのループ処理明示 + const修飾

**対象**: 01-17b L231-233 (PendingInterrupt型) + L302 (ProcessCompletionフロー)

**修正A**: PendingInterruptの型をconst化:
```cpp
        /// 完了時に遅延削除キューから解放するリソース群
        const IRHIResource** deferredResources = nullptr;
        uint32 deferredResourceCount = 0;
```

**修正B**: ProcessCompletion Step 3をループ展開:
```
  3. 遅延削除リソースを即座に解放（フェンス完了済みのため再待機不要）:
     for i = 0..entry.deferredResourceCount-1:
       RHIDeferredDeleteQueue::DeleteImmediate(entry.deferredResources[i])
```

### FIX-7 [WARNING]: SubmitCommandLists→Payload変換フローの具体化

**対象**: 01-17b L421-423

**修正**:
```
IDynamicRHI::SubmitCommandLists(queueType, commandLists, count)
    → RHIPayload payload作成
    → payload.queueType = queueType
    → payload.commandLists/count設定
    → 各commandLists[i]->GetAllocator()でアロケーター収集（重複排除）
    → payload.usedAllocators/count設定
    → payload.completionFenceValue = m_fenceValueTrackers[queueType].GetNextValue()
    → RHISubmissionThread::EnqueuePayload(move(payload))
```

### FIX-8 [WARNING]: RHIPayloadのメモリ所有権を明記

**対象**: 01-17b RHIPayload構造体コメント(L45-46)

**修正**: コメントを更新:
```cpp
    /// サブミッション単位
    /// レンダースレッドが作成し、サブミッションスレッドがGPUに送信する
    ///
    /// メモリ所有権:
    ///   配列ポインタ（commandLists, waitPoints, signalPoints, usedAllocators）は
    ///   フレームリニアアロケーターから確保する。Payloadのライフタイムは
    ///   サブミッションスレッドでの処理完了まで（PendingInterruptへの変換時に
    ///   必要なデータはコピーされる）。フレーム終了時に一括解放。
    ///   commandListsはconst修飾なし（Payload所有者が配列構築時に書き込むため）。
    ///   ExecuteCommandListsにはIRHICommandList* const*として渡される。
```

### FIX-9 [STYLE]: ファイルパスをPascalCaseに統一

**対象**: 01-17b L24-28

**修正**:
```
- `Source/Engine/RHI/Public/RHISubmission.h`
- `Source/Engine/RHI/Private/RHISubmissionThread.h`
- `Source/Engine/RHI/Private/RHISubmissionThread.cpp`
- `Source/Engine/RHI/Private/RHIObjectPool.h`
- `Source/Engine/RHI/Private/RHIObjectPool.cpp`
```

### FIX-10 [WARNING]: completionFenceValue採番方式の明記

**対象**: 01-17b Section 4 所有関係 (L416-419付近)

**修正**: 所有関係の末尾に追加:
```
  IDynamicRHI → RHIFenceValueTracker[ERHIQueueType::Count]（キューごとのフェンス値管理、09-01定義）
```
※ 採番は FIX-7 の SubmitCommandLists フロー内に含む

### FIX-11 [WARNING]: signalPointsのGPU側処理ステップ追加

**対象**: 01-17b ProcessPayloadフロー (L201-202間)

**修正**: Step 5の後にStep 5.5を追加:
```
  5.5. signalPoints処理:
       for i = 0..payload.signalPointCount-1:
         if payload.signalPoints[i].IsValid():
           queue->Signal(payload.signalPoints[i].fence, payload.signalPoints[i].value)
```

### FIX-12 [WARNING]: ProcessPayload Step 6のPendingInterrupt組み立て明示

**対象**: 01-17b ProcessPayloadフロー L202

**修正**: Step 6を具体化:
```
  6. PendingInterrupt組み立て・Enqueue:
     entry.queueType = payload.queueType
     entry.fence = m_queueFences[payload.queueType]
     entry.fenceValue = payload.completionFenceValue
     entry.allocators = payload.usedAllocators
     entry.allocatorCount = payload.usedAllocatorCount
     entry.deferredResources = nullptr  // 01-14経由で別管理
     entry.deferredResourceCount = 0
     m_interruptThread->EnqueueInterrupt(move(entry))
```

### FIX-13 [INFO]: FlushCommands/FlushQueueの通信方式明記

**対象**: 01-17b Section 4 FlushCommands/FlushQueue (L431-440)

**修正**:
```
IDynamicRHI::FlushCommands()
    → サブミッションスレッドに排出を要求（特殊なFlush Payloadを投入）
    → サブミッションスレッドがFlush Payloadを処理し、排出完了イベントをシグナル
    → レンダースレッドは排出完了イベントを待機
    → 全フェンス完了待ち（fence->Wait）

IDynamicRHI::FlushQueue(queueType)
    → FlushCommandsと同じ排出フロー
    → 該当キューのフェンスのみ完了待ち
```

### FIX-14 [INFO]: ObtainContext型のキュータイプ依存性注記

**対象**: 01-17b Section 3 ObtainContextコメント (L338-340)

**修正**:
```cpp
        /// コンテキスト取得（プールから再利用 or 新規作成）
        /// コンストラクタで指定されたキュータイプを使用
        /// Graphics/Compute → IRHICommandContext、Copy → コピー特化コンテキスト
        /// @return 使用可能なコンテキスト
```

---

## 新規4件（今回checker検出）

### FIX-15 [WARNING]: ProcessCompletionにコマンドリスト返却ステップ追加

**問題**: ProcessCompletionフローにアロケーター返却と遅延削除はあるが、コマンドリストの返却ステップがない。ドキュメントではFYourPayloadにCommandListsが含まれ完了後にプールに返却される。PendingInterruptにもcommandListsフィールドがない。

**対象**: 01-17b PendingInterrupt構造体(L220-238) + ProcessCompletionフロー(L297-305)

**修正A**: PendingInterruptにコマンドリスト参照を追加:
```cpp
        /// 完了時にプールに返却するコマンドリスト群
        IRHICommandList** commandLists = nullptr;
        uint32 commandListCount = 0;
```

**修正B**: ProcessCompletionフロー Step 2の後にStep 2.5を追加:
```
  2.5. コマンドリスト群をm_objectPools[entry.queueType]に返却
       for i = 0..entry.commandListCount-1:
         m_objectPools[entry.queueType]->ReleaseCommandList(entry.commandLists[i])
```

**修正C**: FIX-12のPendingInterrupt組み立て(Step 6)にも追加:
```
     entry.commandLists = payload.commandLists
     entry.commandListCount = payload.commandListCount
```

### FIX-16 [WARNING]: ObtainContext呼び出し経路をSection 4に追記

**問題**: ObtainContext()がパラメータなし(FIX-2で修正済み)だが、IDynamicRHIからの呼び出し経路がSection 4に未記述。ObtainCommandAllocator/ReleaseCommandAllocatorは記述済み。

**対象**: 01-17b Section 4 IDynamicRHIとの統合 (L425-429付近)

**修正**: ObtainCommandAllocatorの前に追加:
```
IDynamicRHI::ObtainContext(queueType)
    → m_objectPools[queueType]->ObtainContext()

IDynamicRHI::ReleaseContext(context)
    → m_objectPools[context->GetQueueType()]->ReleaseContext(context)
```

### FIX-17 [INFO]: m_objectPools初期化ステップを初期化シーケンスに追記

**問題**: RHIInterruptThread::m_objectPools[](L281)への登録ステップが初期化シーケンスに未記述。m_queues/m_queueFencesの登録は記述済み。

**対象**: 01-17b Section 4 初期化シーケンス (L442-447)

**修正**: Step 2の後にStep 2.5を追加:
```
  2.5. RHIInterruptThread::m_objectPools[]に各キューのオブジェクトプールを登録
```

### FIX-18 [INFO]: RHIPayload::commandListsのconst*不一致を明記（FIX-8に統合済み）

**問題**: `ExecuteCommandLists(IRHICommandList* const* ...)` はconst*だがRHIPayload::commandListsは`IRHICommandList**`。

**対応**: FIX-8のメモリ所有権コメントに理由を含めた（Payload所有者が構築時に書き込むため非const）。追加修正不要。

---

## 検証

- FIX-1: 01-25のSynchronizationセクションにGetFence()が存在すること
- FIX-2: 01-17b内の全`ERHIQueueType::Count`がstatic_cast付きであること
- FIX-3: ProcessPayload Step 2がforループ+IsValid()+queue->Wait()であること
- FIX-4: RHIObjectPool privateにstd::mutex m_mutexがあること
- FIX-5: RHISubmissionThreadにm_barrierAllocatorsがあり、ProcessPayload Step 3が具体化されていること
- FIX-6: PendingInterruptのdeferredResourcesがconst化+ProcessCompletion Step 3がforループであること
- FIX-7: SubmitCommandListsフローにGetAllocator()収集+GetNextValue()採番が記述されていること
- FIX-8: RHIPayloadコメントにメモリ所有権+commandLists非const理由が記述されていること
- FIX-9: 変更対象ファイルのパスがPascalCaseであること
- FIX-10: Section 4所有関係にRHIFenceValueTrackerがあること
- FIX-11: ProcessPayloadフローにStep 5.5 signalPoints処理があること
- FIX-12: Step 6がPendingInterrupt全フィールド（commandLists含む）を列挙していること
- FIX-13: FlushCommands/FlushQueueに排出Payload+完了イベント待機が記述されていること
- FIX-14: ObtainContextコメントにGraphics/Compute vs Copyの型区別があること
- FIX-15: PendingInterruptにcommandListsフィールド+ProcessCompletionにStep 2.5があること
- FIX-16: Section 4にObtainContext/ReleaseContextの呼び出し経路があること
- FIX-17: 初期化シーケンスにm_objectPools登録ステップがあること

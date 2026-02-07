# 厳格レビュー指摘の修正計画

## Context

plan-doc-reviewによる厳格レビューでCRITICAL 4件、WARNING 9件、INFO 4件+追加問題が検出された。
計画ファイル(.md)の修正のみ。コード変更なし。対象は6ファイル。

---

## 修正対象ファイル一覧

| # | ファイル | CRITICAL | WARNING | INFO/他 |
|---|---------|----------|---------|---------|
| 1 | `09-02-sync-point.md` | 3 | 3 | 4 |
| 2 | `01-12-resource-base.md` | 0 | 4 | 1 |
| 3 | `01-15-dynamic-rhi-core.md` | 0 | 2 | 3 |
| 4 | `01-26-queue-fence.md` | 1 | 0 | 0 |
| 5 | `01-01-fwd-macros.md` | 1 | 0 | 0 |
| 6 | `11-05-residency.md` | 0 | 0 | 4 |

---

## ファイル1: `09-02-sync-point.md`

### [CRITICAL-2] m_nextSyncValue スレッドセーフティ (line 247)

`uint64 m_nextSyncValue = 1;` → スレッド制約コメント追加

```cpp
        IRHIDevice* m_device = nullptr;
        RHIFenceRef m_syncFence;
        uint64 m_nextSyncValue = 1;  ///< @note レンダースレッドからのみ操作。スレッドセーフティは呼び出し側が保証。
```

### [CRITICAL-3] デバッグ専用メンバの条件コンパイル (line 249-278)

m_syncGraph、ValidateNoCircularDependency、ResetFrameGraph を `#if NS_BUILD_DEBUG` で囲む。
`NS_RHI_DEBUG` → `NS_BUILD_DEBUG`（01-01で定義済みのマクロに統一）。

line 249-278 を以下に置換:

```cpp
#if NS_BUILD_DEBUG
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
        /// @note 計算量: O(V+E) where V=kMaxQueues, E=辺数
        ///
        /// アルゴリズム:
        ///   toQueue→...→fromQueue の到達可能性をDFSで確認。
        ///   到達可能ならfromQueue→toQueueを追加するとサイクルになる。
        ///   1. visited[kMaxQueues] = {false} を用意
        ///   2. toQueue を起点にDFS開始:
        ///      - m_syncGraph[current][next] > 0 の全隣接を探索
        ///      - next == fromQueue → サイクル検出 → return false
        ///      - next が未訪問 → 再帰DFS
        ///   3. サイクルなし → return true
        ///   kMaxQueues=8 のため配列はスタック上に置ける（ヒープ割り当て不要）
        bool ValidateNoCircularDependency(uint32 fromQueue, uint32 toQueue) const;

        /// フレーム開始時に同期グラフをリセット
        /// @note BeginFrame()から呼び出すこと
        void ResetFrameGraph();
#endif
```

### [CRITICAL-4] DFSアルゴリズム矛盾 + マクロ名統一 (line 220-232)

- `NS_RHI_ASSERT` → `RHI_CHECKF`（01-01で定義済み）
- `NS_RHI_DEBUG` → `NS_BUILD_DEBUG`
- SyncQueues内のコメント修正

line 220-232 を以下に置換:

```cpp
        void SyncQueues(IRHIQueue* fromQueue, IRHIQueue* toQueue) {
            RHI_CHECKF(fromQueue != toQueue, "Self-sync is not allowed");
#if NS_BUILD_DEBUG
            // 循環デッドロック検出: toQueue→...→fromQueueの到達可能性を確認
            const uint32 fromIdx = fromQueue->GetQueueIndex();
            const uint32 toIdx = toQueue->GetQueueIndex();
            RHI_CHECKF(ValidateNoCircularDependency(fromIdx, toIdx),
                "Circular queue dependency detected: adding %u->%u creates a cycle", fromIdx, toIdx);
            m_syncGraph[fromIdx][toIdx]++;
#endif
            RHISyncPoint sp = InsertSyncPoint(fromQueue);
            WaitForSyncPoint(toQueue, sp);
        }
```

### [WARNING-7] RHISyncPointWaiter::Add() サイレントドロップ (line 301-305)

```cpp
        void Add(const RHISyncPoint& syncPoint) {
            RHI_CHECKF(m_count < kMaxSyncPoints, "RHISyncPointWaiter overflow: max %u", kMaxSyncPoints);
            if (m_count < kMaxSyncPoints && syncPoint.IsValid()) {
                m_syncPoints[m_count++] = syncPoint;
            }
        }
```

### [WARNING-8] ResetFrameGraph 呼び出しタイミング

CRITICAL-3 の修正内に `@note BeginFrame()から呼び出すこと` として含めた。

### [WARNING-9] RHITimelineSync vs RHIFenceValueTracker 重複 (Section 5冒頭)

line 340-341 のコメントに関係性を追記:

```cpp
    /// タイムライン同期ポイント
    /// 時系列での同期ポイント管理
    /// @note 09-01 RHIFenceValueTrackerの上位互換。RHISyncPointの返却、
    ///       キュー間Wait機能を追加。新規コードではこちらを使用すること。
```

### [INFO-2] ERHIPipelineSyncType 未使用

削除はせず、SyncQueuesのデバッグログ用途としてコメント追記 (line 157):

```cpp
    /// パイプライン同期タイプ
    /// @note SyncQueues()のデバッグログ出力時に使用
```

### [INFO-3] コメント「の」→「→」統一 (line 166-182)

- line 166: `/// コンピュートのグラフィックス` → `/// コンピュート→グラフィックス`
- line 169: `/// コンピュートのコンピュート` → `/// コンピュート→コンピュート`
- line 181: `/// コンピュートのコピー` → `/// コンピュート→コピー`

### [追加] typo修正 (line 317)

`/// いれか完了しているか` → `/// いずれか完了しているか`

### [追加] RHITimelineSync::m_nextValue スレッド制約 (line 380)

```cpp
        uint64 m_nextValue = 1;  ///< @note レンダースレッドからのみ操作
```

---

## ファイル2: `01-12-resource-base.md`

### [WARNING-1] GetDebugName() ダングリングポインタ (line 111-118)

戻り値を `std::string` に変更:

```cpp
        /// デバッグ名取得
        /// @note スレッドセーフ。値コピーを返すため呼び出し後も安全に使用可能。
        std::string GetDebugName() const {
            std::lock_guard<std::mutex> lock(m_debugNameMutex);
            return m_debugName;
        }
```

### [WARNING-2] m_pendingDelete スレッドセーフティ (line 156)

```cpp
        mutable std::atomic<bool> m_pendingDelete{false};
```

line 144, 147 の noexcept は `std::atomic` の `load`/`store` が noexcept なのでそのまま。

### [WARNING-3] m_resourceType vs 純粋仮想 GetResourceType() 冗長 (line 82, 197, 200)

GetResourceType() を純粋仮想のまま維持し、冗長な m_resourceType と protected コンストラクタを削除:

- line 196-200 削除:
  ```
  (削除)    protected:
  (削除)        IRHIResource(ERHIResourceType type);
  (削除)    private:
  (削除)        ERHIResourceType m_resourceType;
  ```

### [WARNING-6] SetResidencyPriority 型不一致 (line 193)

`uint32` → `ERHIResidencyPriority`（11-05と統一）:

```cpp
        /// 常駐優先度設定
        /// @note デフォルトは何もしない。11-05 IRHIResidentResource で実装。
        virtual void SetResidencyPriority(ERHIResidencyPriority priority) { (void)priority; }
```

前方宣言セクション (line 34) に追加:
```cpp
    enum class ERHIResidencyPriority : uint8;
```

### [INFO-4] gpuVirtualAddress 命名 (line 228)

```cpp
        /// 基盤リソースのGPU仮想アドレス（ベースアドレス）
        uint64 baseGPUVirtualAddress = 0;
```

line 234-235 も連動修正:
```cpp
        uint64 GetGPUVirtualAddress() const {
            return baseGPUVirtualAddress + offset;
        }
```

---

## ファイル3: `01-15-dynamic-rhi-core.md`

### [WARNING-4] パス PascalCase違反 (line 14)

`source/engine/rhi/public/IDynamicRHI.h` → `Source/Engine/RHI/Public/IDynamicRHI.h`

### [WARNING-5] ERHIFeature::Count の static_cast 不足 (line 222-224)

```cpp
    /// GetFeatureSupport() 実装パターン:
    /// 内部テーブル配列 `ERHIFeatureSupport m_featureTable[static_cast<uint32>(ERHIFeature::Count)]` を使用。
    /// Init()内でアダプター機能クエリ結果を基にテーブルをポピュレートする。
```

### [INFO-1] GetName() 引用符欠落 (line 43)

`/// RHI名取得（D3D12", "Vulkan"等）` → `/// RHI名取得（"D3D12", "Vulkan"等）`

### [追加] mojibake残存 (line 122)

`シングルGPU環境界主要デバイス` → `シングルGPU環境の主要デバイス`

### [追加] mojibake残存 (line 153)

`拡張合` → `拡張名`

---

## ファイル4: `01-26-queue-fence.md`

### [CRITICAL-1a] RHISyncPoint 重複定義の解消 (line 126-150)

Section 4 の RHISyncPoint 定義を削除し、09-02 への参照に置換:

```markdown
### 4. 同期ポイント

> **注意**: `RHISyncPoint` の定義は `09-02-sync-point.md` Section 1 を参照。
> ここでは IRHIQueue における同期ポイント操作メソッドのみ定義する。
```

続くIRHIQueueメソッド (line 152-) は 09-02 の `RHISyncPoint` 定義に合わせて修正:

```cpp
    class IRHIQueue
    {
    public:
        //=====================================================================
        // 同期ポイント
        //=====================================================================

        /// 現在の同期ポイントを作成
        RHISyncPoint CreateSyncPoint() {
            return RHISyncPoint{ GetFence(), GetLastSubmittedFenceValue() };
        }

        /// 同期ポイントでシグナルを発行
        RHISyncPoint SignalSyncPoint() {
            uint64 value = AdvanceFence();
            Signal(GetFence(), value);
            return RHISyncPoint{ GetFence(), value };
        }

        /// 同期ポイントを待つ。
        void WaitForSyncPoint(const RHISyncPoint& syncPoint) {
            if (syncPoint.IsValid()) {
                Wait(syncPoint.fence, syncPoint.value);
            }
        }
```

---

## ファイル5: `01-01-fwd-macros.md`

### [CRITICAL-1b] 前方宣言の修正 (line 58)

`class IRHISyncPoint;` → `struct RHISyncPoint;`

---

## ファイル6: `11-05-residency.md`

### [追加] mojibake残存4箇所

- line 50: `/// 作` → `/// 低`
- line 123: `退避させめ` → `退避させる`
- line 131: `常駆` → `常駐`
- line 155: `（で自動）` → `（0で自動検出）`

---

## 実行順序

1. `01-01-fwd-macros.md` — 前方宣言修正（他ファイルが参照するため先に）
2. `01-26-queue-fence.md` — 重複RHISyncPoint削除
3. `09-02-sync-point.md` — CRITICAL 3件 + WARNING 3件 + INFO/追加
4. `01-12-resource-base.md` — WARNING 4件 + INFO 1件
5. `01-15-dynamic-rhi-core.md` — WARNING 2件 + INFO/追加 3件
6. `11-05-residency.md` — mojibake 4件

## 検証方法

- 修正後 `plan-doc-checker` スキルでスコア確認
- 修正後 `plan-doc-review` スキルでCRITICAL=0を確認
- RHISyncPoint の定義が単一箇所（09-02）のみであることをgrep確認
- マクロ名（NS_BUILD_DEBUG, RHI_CHECKF）が01-01の定義と一致することを確認

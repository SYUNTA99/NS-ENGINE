# RHI 設計仕様 — 同期とフレーム管理

---

## 1. フレームライフタイム

### 意図（Intent）
CPUとGPUの実行を分離し、スループットを最大化しつつ
リソース安全性と遅延上限を保証する。

### フレーム境界

```
BeginFrame():
  1. フェンス値を進行
  2. 遅延削除キューをクリーンアップ
  3. フレーム毎リソースを準備（コマンドアロケータ、アップロードバッファ等）

EndFrame():
  1. フレーム完了をシグナル
  2. 統計を収集
  3. Present をトリガー
```

### インフライトフレーム管理

```
K = 最大インフライトフレーム数（通常 2〜3）

CPU: [Record N] [Record N+1] [Record N+2] [WAIT]   [Record N+3]
GPU:            [Execute N]  [Execute N+1] [Execute N+2] [Execute N+3]

┌──────────────────────────────────────────────────────┐
│ フレーム毎リソース（K エントリのリングバッファ）      │
│                                                      │
│ Frame N   % K → [CmdAlloc A] [UploadBuf A] [DescA]  │
│ Frame N+1 % K → [CmdAlloc B] [UploadBuf B] [DescB]  │
│ Frame N+2 % K → [CmdAlloc C] [UploadBuf C] [DescC]  │
│                                                      │
│ リソース安全性:                                      │
│   CPU: fence[N % K] を確認してから Frame N+K を上書き │
│   GPU: Frame N 完了後に fence[N % K] をシグナル       │
└──────────────────────────────────────────────────────┘
```

### 要件（Requirements）
- フレーム境界の明確なマーカー（begin/end）
- 同時実行フレーム数の追跡と制限
- GPUフェンスに基づくリソース回収
- CPUがGPUに対し過剰に先行した場合のバックプレッシャー

### 不変条件（Invariants）
- CPU上で削除されたリソースは、それを参照するGPUワークが完了するまで解放されない
- Frame N+K は Frame N のGPUワークが完了するまで開始不可（K = 最大インフライト数）
- Present操作は暗黙的なフレーム境界を形成する
- フレームカウンタは単調増加し、リセットされない

### 設計判断（Design Decisions）

| K値 | 利点 | 欠点 |
|-----|------|------|
| K=1 | 低レイテンシ | CPUが頻繁にストール |
| K=2 | バランス型 | 標準的なメモリ使用量 |
| K=3 | 高スループット | 高レイテンシ、高メモリ使用量 |

> **参考: UE5実装** — `BeginFrame()`/`EndFrame()` がフレーム境界。
> フレームリソースはリングバッファで管理。

---

## 2. リソースライフタイム

### 意図（Intent）
参照カウントと遅延削除により、use-after-free とメモリリークを防止する。

### 遅延削除フロー

```
Frame N:
  CPU: CreateTexture() → RefCount = 1
  CPU: CmdList->Draw(Texture) → RefCount = 1（CmdListが参照保持）
  CPU: Texture.Release() → RefCount = 0 → MarkForDelete()

Frame N+1:
  CPU: BeginFrame()
    → GatherResourcesToDelete(): Frame N のフェンスを確認
    → Frame N GPU完了済み → DeleteResources([Texture])
    → GPUメモリ解放、CPU側ラッパー削除
```

### リソース基底クラスの責務

```
Resource {
  AddRef()       // 参照カウントをアトミック加算
  Release()      // アトミック減算; 0到達時 MarkForDelete()
  MarkForDelete() // 遅延削除キューに追加（現フレームフェンス値付き）
}

遅延削除キュー:
  グローバルまたはデバイス毎の (Resource*, FenceValue) ペアのキュー
  毎フレーム: 完了済みフェンスのリソースをバッチ削除
```

### 不変条件（Invariants）
- 参照カウントは 0 未満にならない
- CPUで RefCount → 0 でも、GPUがまだ参照している可能性がある
- リソースメモリはGPUフェンスがインフライト使用なしを確認後にのみ解放
- コマンドリスト記録は参照リソースのカウントを保持する

---

## 3. フェンスとタイムライン同期

### 意図（Intent）
パイプライン全体をブロックせずに、きめ細かな GPU-CPU / GPU-GPU 同期を提供する。

### フェンス抽象

```
Fence {
  Poll(uint64 value)                              // ノンブロッキング照会。指定値に到達済みなら true
  Wait(uint64 value, uint64 timeoutMs = UINT64_MAX) // GPU完了までブロッキング待機（タイムアウト付き）
}
```

### フェンスのデータフロー

```
GPU Graphics Queue:
  [描画コマンド] → Signal Fence A (value 100)

GPU Compute Queue:
  Wait Fence A (value 100) → [コンピュートワーク] → Signal Fence B (value 200)

CPU:
  Poll Fence B → GPUコンピュート完了時に戻る
```

### タイムラインセマンティクス

| 性質 | 説明 |
|------|------|
| 単調増加 | フェンス値は決して減少しない |
| 冪等ポーリング | 同一フェンスの複数回ポーリングは一貫した結果を返す |
| 完了済み待機 | 既に完了したフェンスへの Wait は即座に返る |
| クロスGPU | マルチGPUフェンスは GPU 毎の独立同期点を要求する |

### プラットフォームマッピング

| 概念 | D3D12 | Vulkan | Metal |
|------|-------|--------|-------|
| GPU-GPU同期 | ID3D12Fence | VkSemaphore | MTLEvent |
| CPU-GPU同期 | ID3D12Fence | VkFence | MTLFence |
| タイムライン | ネイティブ | Timeline Semaphore (1.2+) | MTLEvent |

### 不変条件（Invariants）
- フェンス値は単調進行（セッション内でラップ/減少しない）
- ポーリングは「未完了→完了→未完了」の状態遷移をしない
- 完了済みフェンスの待機は即座に返る
- タイトループでの同一スレッドポーリングはデッドロックのリスクあり

---

## 4. リソース状態遷移とバリア

### 意図（Intent）
リソースが異なる用途間で遷移する際のメモリ一貫性を保証し、ハザードを防止する。

### ハザードの種類

| ハザード | 説明 | バリア要否 |
|---------|------|-----------|
| RAW (Read-After-Write) | 書き込み後の読み取り | 必要 |
| WAR (Write-After-Read) | 読み取り後の書き込み | 必要 |
| WAW (Write-After-Write) | 書き込み後の書き込み | 必要 |
| RAR (Read-After-Read) | 読み取り後の読み取り | **不要** |

### 遷移記述

```
RHITransitionBarrier {
  resource        // テクスチャ/バッファ/UAV/BVH
  stateBefore     // 現在の状態 (ERHIResourceState)
  stateAfter      // 遷移先状態 (ERHIResourceState)
  subresourceRange // Mip/Array/Plane スライス指定
  flags           // ERHIBarrierFlags
}
```

### アクセス状態の合成ルール

```
読み取り専用状態は合成可能:
  SRVCompute | IndirectArgs → 有効（複数の読み取りアクセス）

書き込み状態は排他的:
  RTV | UAV → 無効（書き込み同士は排他）
  RTV | SRVCompute → 無効（書き込み+読み取りもバリア必要）

Unknown 状態:
  未追跡状態を示す → フルキャッシュフラッシュを強制
```

### スプリットバリア

```
BeginTransition():
  バリアの「開始半分」を発行（可用性操作）
  ↓
[リソースに依存しない他のワーク]
  ↓
EndTransition():
  バリアの「終了半分」を発行（可視性操作）

利点: バリアのレイテンシを独立ワークとオーバーラップ
注意: End の忘失はデータ破壊の危険あり
```

### バリアバッチング

```
複数遷移を蓄積してから一括発行:
  D3D12: ResourceBarrier(count, array)
  Vulkan: vkCmdPipelineBarrier(N barriers)

フラッシュタイミング:
  - コマンドリスト末尾
  - パイプライン切り替え時
  - 手動フラッシュ要求時
```

### レガシー vs エンハンスドバリア

| 項目 | レガシー | エンハンスド |
|------|---------|------------|
| 状態概念 | 単一モノリシック状態 | レイアウト + アクセス + 同期の分離 |
| 粒度 | サブリソース | サブリソース |
| 利点 | シンプル | 過剰同期を回避 |
| API要件 | D3D12基本 | D3D12.1+ / Vulkan |

### プラットフォームバリアマッピング

| 概念 | D3D12 レガシー | D3D12 エンハンスド | Vulkan | Metal |
|------|---------------|------------------|--------|-------|
| 状態 | D3D12_RESOURCE_STATES | Layout + Access + Sync | Layout + Access + Stage | 暗黙的レイアウト |
| バリア | ResourceBarrier(TRANSITION) | Barrier(TEXTURE/BUFFER) | ImageMemoryBarrier | エンコーダ境界 |
| 粒度 | サブリソース | サブリソース | サブリソース | リソース全体 |

### 遷移フラグ (ERHIBarrierFlags)

| フラグ | 説明 |
|--------|------|
| None | デフォルト（即座に完全なバリアを発行） |
| BeginOnly | スプリットバリアの開始半分のみを発行 |
| EndOnly | スプリットバリアの終了半分のみを発行 |

### 不変条件（Invariants）
- リソースは使用前に正しい状態でなければならない
- 読み取り専用状態の重複は許可（複数SRVアクセス）
- 書き込み状態の重複、読み書きの重複にはバリアが必要
- Unknown状態はフルキャッシュフラッシュを強制

---

## 5. キュー実行モデル

### 意図（Intent）
独立したワークロード（グラフィクス、非同期コンピュート、コピー）の並列実行を可能にし、
明示的な同期で正確性を維持する。

### キュー構成

```
┌─────────────────────────────────────────────────────────┐
│                    GPU                                   │
│                                                         │
│  Graphics Queue ─────[描画/レンダーパス]─────→           │
│                                                         │
│  Compute Queue  ─────[非同期ディスパッチ]───→           │
│                                                         │
│  Copy Queue     ─────[データ転送]───────────→           │
│                                                         │
│  各キューは独立に進行（明示的同期なしでは順序保証なし）   │
└─────────────────────────────────────────────────────────┘
```

### キュータイプマッピング

| 汎用名 | D3D12 | Vulkan |
|--------|-------|--------|
| Graphics/Direct | Direct | Graphics |
| Async Compute | Async | AsyncCompute |
| Copy/Transfer | Copy | Transfer |
| Present | (Graphics) | (Graphics or Compute) |

> **注意:** `ERHIQueueType::Compute` はキューのハードウェア種別を示す。`ERHIPipeline::AsyncCompute` はグラフィクスとは別のパイプラインで非同期にコンピュートワークを実行する論理概念を示す。キューtype はリソース配置やフェンス管理に使用され、Pipeline はコマンド記録先の切り替えに使用される。

### クロスキュー同期

```
タイムライン:
  Graphics: [Frame N 描画] → Signal(Gfx, 100) → [Frame N+1 描画]
  Compute:  Wait(Gfx, 100) → [非同期コンピュート] → Signal(Cmp, 50)
  Copy:     [データアップロード] → Signal(Copy, 1)
  Graphics: Wait(Cmp, 50) + Wait(Copy, 1) → [結果合成]
```

### クロスパイプライン遷移

```
SrcPipelines: パイプラインビットマスク（リソースの現在の使用元）
DstPipelines: パイプラインビットマスク（リソースの次の使用先）

例:
  SrcPipelines = Graphics
  DstPipelines = AsyncCompute
  → フェンス挿入: Graphics がシグナル、Compute が待機
```

### 不変条件（Invariants）
- 単一キュー内のワークはサブミッション順序で実行（FIFO）
- クロスキューワークは明示的フェンスなしでは順序保証なし
- 循環待機は防止されなければならない（デッドロック回避）
- 異なるキューで同一リソースにアクセスするには遷移が必要

---

## 6. 非同期コンピュートとオーバーラップ

### 意図（Intent）
グラフィクスレンダリングとコンピュートワークをオーバーラップさせ、
独立した実行ユニットを活用してGPU使用率を最大化する。

### オーバーラップパターン

```
Graphics Queue:
  [シャドウパス] → Signal(Gfx, 10) → [GBufferパス] → Wait(Cmp, 5) → [ライティング]

Async Compute Queue:
  Wait(Gfx, 10) → [SSAOコンピュート] → Signal(Cmp, 5)

並列性: SSAOはGBufferパスとオーバーラップ（異なるリソース使用）
```

### パイプラインモード切り替え

```
SwitchPipeline(Pipeline):
  実行ターゲットを Graphics ↔ AsyncCompute に切り替え
  共有リソースに対する保留中のワークがあれば暗黙的フェンスを挿入

ScopedPipeline (RAII):
  スコープ終了時にパイプラインを自動復元
  非同期コンピュートモードのグラフィクスへの漏洩を防止
```

### 設計判断（Design Decisions）

| 選択肢 | 利点 | 欠点 |
|--------|------|------|
| 有効化 | GPU使用率向上の可能性 | リソース競合のリスク（VRAM帯域、キャッシュ） |
| 無効化 | シンプル、予測可能な性能 | GPU実行ユニットが遊休 |

### 不変条件（Invariants）
- 非同期コンピュートはグラフィクス専用リソースに遷移なしでアクセス不可
- グラフィクスとコンピュートキューはリソース衝突がなければ並行実行可能
- 非同期コンピュートの無効化は同一キュー実行にフォールバック（正確だが遅い）

---

## 7. マルチGPU同期

### 意図（Intent）
複数GPUを協調動作させ、交互フレームレンダリング（AFR）や
分割フレームレンダリングで描画スループットを向上させる。

### GPUマスクシステム

```
GPUMask {
  ビットマスクでGPUセットを表現:
    0b0001 = GPU 0 のみ
    0b0011 = GPU 0 と GPU 1
    0b1111 = 4 GPU すべて

  ファクトリメソッド:
    GPU0(), FromIndex(i), All(), FilterGPUsBefore(i)

  照会メソッド:
    GetFirstIndex(), GetNumActive(), Contains(i), Intersects(mask)

  イテレーション:
    range-for ループ対応
}
```

### クロスGPU転送

```
Frame N:
  CPU: GPUMask(GPU0) でコマンド記録
  GPU 0: Frame N を実行

Frame N+1:
  CPU: GPUMask(GPU1) でコマンド記録
  転送: GPU 0 → GPU 1（モーションベクタ、レンダーターゲット）
  GPU 1: Frame N+1 を実行

AFR パターン:
  - 各GPU が交互にフレームをレンダリング
  - 時間依存データ（モーションベクタ）をクロスGPU転送
  - リソースは全GPUに複製（GPUMask = All）
```

### 転送メカニズム

| 方式 | 説明 |
|------|------|
| Push | ソースGPUがデータを送信 |
| Pull | デスティネーションGPUがデータを要求 |
| Lock-Step | 複数パスが同一領域に書き込む場合のハンドシェイク |
| 遅延フェンシング | 転送完了の非同期シグナル（待機を遅延可能） |

### リソース作成フラグ

| フラグ | 説明 |
|--------|------|
| MultiGPUAllocate | GPU毎に独立メモリを割り当て（ドライバーミラーではない） |
| MultiGPUGraphIgnore | 自動クロスGPU転送をスキップ（手動転送パス） |

### 設計判断（Design Decisions）

| 方式 | 利点 | 欠点 |
|------|------|------|
| AFR | 並列性によるフレームレート向上 | メモリ複製、転送帯域、時間依存の複雑さ |
| SFR | 単一フレームの分割処理 | ロードバランシングが困難 |

### 不変条件（Invariants）
- マルチGPU無効時（コンパイル時）、全マスク操作は constexpr 単一GPUに折り畳まれる
- GPUマスク指定で作成されたリソースは指定GPUにのみ存在
- 転送操作は明示的なソース/デスティネーション GPU インデックスを要求
- フェンスのポーリング/待機はターゲットGPUマスクを指定

---

## 8. コマンドサブミッションパイプライン

### 意図（Intent）
高レベル描画コマンドをGPU実行可能ワークに変換し、
非同期実行、並列化、完了追跡を管理する。

### サブミッションフロー

```
アプリケーションスレッド:
  ↓ コマンド記録
コマンドリスト（遅延）:
  ↓ Submit()
サブミッションスレッド:
  ↓ GPU APIにキュー投入
GPU キュー (Graphics/Compute/Copy):
  ↓ コマンド実行
GPU フェンスシグナル:
  ↓ 完了通知
CPU ポーリング/待機
```

### 実行モード

| モード | 説明 |
|--------|------|
| None | シングルスレッド、即時サブミッション |
| DedicatedThread | 専用RHIスレッドがコマンドリストを受け取りGPUに投入 |
| Tasks | タスクベースのコマンド変換並列化 |

### サブミッションフラグ

| フラグ | 説明 |
|--------|------|
| None | 非同期サブミッション |
| WaitForSubmission | サブミッションスレッド完了までブロック |
| WaitForCompletion | GPUワーク完了までブロック |

### 並列記録

```
メインスレッド:
  ParentCommandList.Begin()

ワーカースレッド 1-N:
  SubCommandList[i].Record(...)  // 独立コンテキスト

メインスレッド:
  ParentCommandList.Merge(SubCommandList[0..N])
  ParentCommandList.Submit()

制約:
  - サブリスト間の状態漏洩なし（各自が独立コンテキスト）
  - マージ前にすべてのサブリストが完了している必要あり
```

### 不変条件（Invariants）
- コマンドリスト内のコマンドは記録順に実行
- 同一キューへのサブミッションは投入順序を保持
- クロスキューワークは明示的フェンス/セマフォ同期を要求
- GPU実行中のコマンドリストは再記録不可

---

## 9. GPUプロファイリングとタイミング

### 意図（Intent）
GPU実行時間を測定し、性能ボトルネックを特定する。
計測が結果を撹乱しない非侵襲的な手法を用いる。

### タイムスタンプクエリのフロー

```
CPU（記録時）:
  BeginEvent("ShadowPass")
    → タイムスタンプクエリ挿入 (TOP)
    [シャドウ描画コマンド]
    → タイムスタンプクエリ挿入 (BOP)
  EndEvent()

GPU（実行時）:
  [最初のクエリ到達] → タイムスタンプ T1 をクエリバッファに書き込み
  [シャドウワーク実行]
  [2番目のクエリ到達] → タイムスタンプ T2 をクエリバッファに書き込み

CPU（リードバック）:
  クエリ結果をポーリング → T1, T2 を読み取り
  Duration = (T2 - T1) / GPUFrequency
```

### イベントストリームアーキテクチャ

```
追記専用イベントログ（キュー毎に独立）:

イベント種類:
  FrameBoundary     // フレーム終了マーカー + フレーム番号
  BeginWork/EndWork // GPU キューのビジー/アイドル遷移
  BeginBreadcrumb/EndBreadcrumb // 階層的タイミング領域
  Stats             // 描画/ディスパッチ回数、プリミティブ数
  SignalFence/WaitFence // クロスキュー同期イベント
  Flip/Vsync        // Present と垂直同期マーカー

イベントシンク:
  イベントストリームを消費する処理（CSV出力、ビジュアライザ、統計集計）
```

### フレーム時間履歴

```
循環バッファ（16エントリ）のフレームタイミング:

RHI スレッド:
  PushFrameCycles(GPUFrequency, GPUCycles)

Game/Render スレッド:
  PopFrameCycles(OutCycles64) → Ok / Disjoint / Empty

スレッド間同期: クリティカルセクション
```

### キャリブレーション

```
IRHIDevice::GetTimestampCalibration(uint64& outGPUTimestamp, uint64& outCPUTimestamp)

同時キャプチャにより GPU→CPU クロックドメインのマッピングを実現
→ GPUワークとCPUフレームの相関に必須
```

### 不変条件（Invariants）
- タイムスタンプはGPUがそのポイントに到達した時点で実行される（CPU記録時点ではない）
- タイムスタンプ周波数は単一フレーム内で一定
- Begin/End タイムスタンプペアは正しくネストされなければならない
- 結果はGPUワーク完了後にのみ利用可能

---

## 10. 横断的不変条件

### Happens-Before 関係

```
CPU record → GPU execute → GPU signal fence → CPU poll returns true
Transition Begin → Independent work → Transition End
Frame N Begin → Frame N commands → Frame N End
```

### 単調進行の原則
- フレーム番号: 常に増加、リセットなし
- フェンス値: 常に増加、ラップなし
- タイムラインセマフォ: 常に増加

### リソース状態の一貫性
- 任意の実行時点で、各リソースサブリソースは単一の定義済み状態を持つ
- 状態は明示的（トラック済み）または暗黙的（プラットフォーム管理）

### 循環依存の禁止
- コマンドリストサブミッショングラフは DAG（有向非巡回グラフ）でなければならない
- 循環フェンス待機はデッドロックを引き起こす

### プラットフォーム抽象境界
- 高レベルコード: アクセスフラグ / GPUマスク / 遷移情報 を操作
- プラットフォームRHI: D3D12_RESOURCE_STATES / VkImageLayout / MTLResourceUsage に変換

---

## 設計パターンまとめ

| パターン | 用途 |
|---------|------|
| **ビットマスクセット** | GPUマスク、アクセスフラグ — 複数同時状態のコンパクト表現 |
| **スプリット操作** | Begin/End バリア — 独立ワークを挟んでレイテンシ隠蔽 |
| **遅延実行** | コマンド記録→サブミッション→GPU実行 — API呼び出しバッチ化 |
| **参照カウント+遅延削除** | AddRef/Release + MarkForDelete — 非同期GPU使用を尊重 |
| **リングバッファ** | フレーム毎リソースプール — GPUが参照中でないことを保証しつつ再利用 |
| **ファクトリ+ストラテジ** | バリア実装の実行時選択 — ケイパビリティに基づく最適化 |
| **イベントストリーム+シンク** | プロファイリングデータの生産者-消費者 — キャプチャと分析の分離 |
| **スコープドRAIIガード** | パイプラインモード、バインディング — スコープ終了時の自動復元 |

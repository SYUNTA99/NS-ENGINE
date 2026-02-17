# RHI 設計仕様 — デバッグと診断

---

## 1. 実行トレースシステム（GPUブレッドクラム）

### 意図（Intent）
GPUがクラッシュ/ハングした際に、どのGPUコマンドが原因かを特定する。
副次的用途として性能プロファイリング、テレメトリ収集にも使用する。

### アーキテクチャ

```
階層的イベント追跡:
  ┌─ Frame
  │  ├─ SceneRendering
  │  │  ├─ ShadowPass
  │  │  │  ├─ CascadedShadow0
  │  │  │  └─ CascadedShadow1
  │  │  ├─ BasePass
  │  │  └─ Lighting
  │  └─ PostProcess
  │     ├─ Bloom
  │     └─ ToneMap
  └─ Present

各ノード:
  - 一意ID（アトミックカウンタ、センチネル検出用高ビット）
  - 静的名前（識別用）+ 動的パラメータ（フォーマット済み文字列）
  - 親リンク（階層構造形成）
  - パイプライン毎に独立追跡
```

### ビルド構成レベル

| レベル | 機能 | 用途 |
|--------|------|------|
| FULL | 完全トレース + プロファイリング + テレメトリ | Debug/Development |
| MINIMAL | 統計のみ、詳細トレースなし | 最適化ビルド（統計付き） |
| DISABLED | ゼロオーバーヘッド | Shipping |

### クラッシュ診断フロー

```
Device Removed 検出:
  1. ハードウェアからマーカー状態を照会
     - MarkerIn: GPU がワークを開始した位置
     - MarkerOut: GPU がワークを完了した位置

  2. マーカー比較（パイプライン毎）:
     Out >= In → ワーク完了
     Out <  In → ワーク実行中（★ クラッシュ箇所）
     Out == 0  → ワーク未開始

  3. アクティブノードからルートまでツリーを遡行
     → "SceneRendering/ShadowPass/CascadedShadow1" のようなフルパスを取得

  4. クラッシュコンテキストに書き出し（構造化データ）
```

### 記録フロー

```
CPU スレッド:
  MACRO → ノード割り当て → 静的名前 + フォーマット引数格納
  → コンテキストに登録 → 親にリンク → ID 割り当て
  → BeginTrace(Node*) → コマンドキューに投入

GPUコマンド変換:
  RHI バックエンド → マーカー書き込み (MarkerIn) → GPUタイムスタンプ
  → GPUワーク実行
  → RHI バックエンド → マーカー書き込み (MarkerOut) → GPUタイムスタンプ
```

### 要件（Requirements）
- 階層的イベント追跡（親子関係でツリー構造形成）
- パイプライン毎の追跡（Graphics/Compute/Transfer キューを独立追跡）
- 一意ID生成（アトミックカウンタ）
- メモリ効率的格納（プール化アロケータ、固定サイズバッファ）
- 遅延文字列フォーマット（ホットパスでの割り当て回避）

### 不変条件（Invariants）
- ノードIDは単調増加（スレッドセーフなアトミックインクリメント）
- Begin/End呼び出しはパイプライン毎にバランス
- アロケータは trivially-destructible 型のみ使用（手動クリーンアップ不要）
- 最大トレース名長を強制（128文字、無制限メモリ防止）
- トレースはパイプライン毎のDAGを形成（明示的親リンク付き）

### 設計判断（Design Decisions）

| 判断 | 理由 |
|------|------|
| 遅延文字列フォーマット | ホットパスで割り当てコストを回避。値キャプチャは型安全テンプレート |
| パイプライン毎の追跡 | GPUキューは独立・非同期に実行。コンピュートキューのクラッシュを独立診断 |
| 階層構造 | フルコンテキスト提供。"Downsample" ではなく "PostProcess/Bloom/Downsample" |
| 固定バッファサイズ | 予測可能なメモリ使用量。キャッシュフレンドリー。最大コスト事前把握 |
| アトミックID | ロック不要の高速パス。CPU/GPUイベント相関用の決定的順序 |
| MINIMALモード | Shipping ビルドに統計は必要だがフルトレースは不要 |

---

## 2. ベンダークラッシュ解析統合

### 意図（Intent）
基本マーカーを超える詳細なクラッシュ診断を取得する。
シェーダー逆アセンブリ、フォルトアドレス、リソース状態等。

### 初期化フロー

```
デバイス作成前:
  1. SDK セットアップ（デバイス作成をラップ必要）
  2. 機能フラグネゴシエーション（マーカー/コールスタック/リソース追跡を独立切替）
  3. コールバック登録（クラッシュダンプ、シェーダー検索、遅延関連付け）
```

### シェーダー登録ライフサイクル

```
シェーダーコンパイル:
  1. バイトコード生成 → ハッシュ計算（例: xxHash64）
  2. SDK::RegisterShader(binary, size, debug_name) → 格納

シェーダーバインド:
  3. PSO 作成 → SDK がシェーダーハッシュをパイプラインに関連付け

シェーダー削除:
  4. 遅延削除をキュー → GPU フェンス待機
  5. SDK::DeregisterShader(hash) → レジストリから除去
```

### クラッシュ処理フロー

```
Device Removed:
  1. SDK コールバック発火 → OnGPUCrash()
  2. SDK がダンプを処理:
     - シェーダーハッシュ解決 → 逆アセンブリ
     - マーカー解決 → ブレッドクラム文字列
     - ページフォルト解析 → フォルトアドレス
  3. Array<CrashResult> を返却:
     - OutputLog（人間可読テキスト）
     - DumpPath（オフラインツール用バイナリダンプ）
     - GPUFaultAddress（オプション、ページフォルト時）
  4. エンジンクラッシュコンテキストに書き出し
```

### 要件（Requirements）
- デバイス作成前のセットアップ（SDKがデバイス作成をラップ）
- シェーダーバイナリのSDK登録（ハッシュ計算、参照格納）
- 遅延シェーダー登録解除（GPU完了待機後）
- マーカー文字列のブレッドクラムからの抽出

### 不変条件（Invariants）
- シェーダーハッシュは同一バイナリ間で安定（決定的ハッシュ関数）
- 登録解除はGPUアイドル後にのみキュー（use-after-free 防止）
- マーカーテキスト長はブレッドクラム制限と同一
- クラッシュコールバックは device-removed スレッドで実行（シングルスレッド、ブロッキング）

### 設計判断（Design Decisions）

| 判断 | 理由 |
|------|------|
| マーカーの文字列コピー | クラッシュコールバックのタイミングは予測不能。ブレッドクラムアロケータが既に破棄されている可能性 |
| 複数クラッシュ結果 | マルチGPU/マルチキューで独立した障害が発生しうる |
| 遅延シェーダー関連付け | クラッシュ時にシェーダーロードが不完全な場合がある |
| 遅延シェーダー登録解除 | GPUが実行中にシェーダーバイナリを参照している可能性 |
| 汎用初期化パターン | `InitializeDevice(TFunctionRef<uint32(flags)>)` — プラットフォームコードがAPI変換を処理 |

---

## 3. API検証レイヤー

### 意図（Intent）
GPUエラーが発生する前に不正なRHI API使用を検出する。
パラメータ検証、状態追跡エラー等。

### ラッパーアーキテクチャ

```
┌─────────────────────────────────────────┐
│ アプリケーション                          │
│   RHICmdList.Draw(Args)                  │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│ 検証レイヤー (ValidationRHI)              │
│                                         │
│ 1. 状態チェック:                         │
│    - PSO バインド済み?                   │
│    - インデックスバッファ必要?            │
│    - 全シェーダーバインド済み?            │
│ 2. パラメータ検証:                       │
│    - VertexCount > 0                    │
│    - InstanceCount > 0                  │
│    - StartVertex は範囲内               │
│ 3. 追跡更新:                            │
│    - 描画操作を記録                      │
│    - UAVアクセス確認（重複チェック）      │
│ 4. デリゲート:                           │
│    - InnerRHI->Draw(Args)               │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│ プラットフォーム RHI (D3D12 / Vulkan等)   │
└─────────────────────────────────────────┘
```

### 検証カテゴリ（`ERHIValidationCategory`）

| カテゴリ | ビット | チェック内容 |
|---------|--------|------------|
| ResourceState | 1 << 0 | リソース状態、遷移、バリア |
| ResourceBinding | 1 << 1 | バインディング整合性、null チェック |
| CommandList | 1 << 2 | コマンドリスト状態、記録/実行順序 |
| Shader | 1 << 3 | シェーダーバインディング、互換性 |
| Pipeline | 1 << 4 | PSO完全性、インデックスバッファ存在 |
| Descriptor | 1 << 5 | ディスクリプタヒープ、テーブル |
| Memory | 1 << 6 | メモリ割り当て、リソースライフタイム |
| Synchronization | 1 << 7 | フェンス、バリア、キュー同期 |
| SwapChain | 1 << 8 | スワップチェーン状態、Present |
| Performance | 1 << 9 | 冗長バリア、不要な状態変更 |

### リソース状態追跡

```
Tracker（パイプライン毎）:
  CurrentList: Array<Operation>（保留中の操作）

Operation 種類:
  - BeginTransition, EndTransition
  - SetTrackedAccess, Assert
  - Draw, Dispatch
  - BeginBreadcrumb, EndBreadcrumb

Finalize 時:
  1. 全操作を処理
  2. 状態遷移を検証
  3. ハザードチェック（RAW, WAR, WAW）
  4. 最終操作リストを返却
```

### エラー報告

| 機能 | 説明 |
|------|------|
| 重複排除 | ハッシュベース、各一意エラーを1回のみ報告 |
| 構成可能冗長度 | ログ / 警告 / 致命エラー |
| スタックトレース | オプション、構成可能 |
| 構造化メッセージ | リソース名、状態情報を含む |

### 要件（Requirements）
- ラッパーパターン（実RHIの全仮想メソッドをオーバーライド）
- コンテキスト毎の状態追跡（並列コンテキストは独立）
- サブリソース粒度の追跡（per-mip, per-slice, per-plane）

### 不変条件（Invariants）
- 検証状態はGPU状態をミラー（検証通過 → GPUは成功すべき）
- リソーストラッカーはパイプライン固有（Graphics vs AsyncCompute 独立）
- 全操作がキューに記録（リプレイ/解析可能）
- サブリソース粒度（Mip/スライス/プレーン毎の追跡）

### 設計判断（Design Decisions）

| 判断 | 理由 |
|------|------|
| ラッパーパターン | プラットフォームRHIコード修正不要。有効/無効が容易 |
| コンテキスト毎の状態 | 並列コンテキストは独立。GPU実行モデルに一致 |
| 操作キュー | バッチ検証で効率化。クロス操作解析が可能 |
| サブリソース粒度 | 異なるMip/スライスが異なる状態を同時に持てる |
| エラー重複排除 | 毎フレーム同一エラーでログ氾濫を防止 |
| 静的検証メソッド | コンテキスト不要のチェック。release でインライン化→ゼロオーバーヘッド |

---

## 4. リソース命名とデバッグマーカー

### 意図（Intent）
デバッガ上の無名GPUリソースを識別可能にする。
"0x7fff12345678" の代わりに "SceneColorBuffer" として表示する。

### リソース命名

```
リソース作成:
  TextureCreateInfo {
    debugName = "SceneColor"
  }

RHI 内部:
  1. CreateTexture(Info)
  2. プラットフォームリソース割り当て
  3. debugName 存在時:
     - プラットフォームAPI呼び出し (SetName / vkDebugMarkerSetObjectName)
     - リソースオブジェクトに名前格納
```

### デバッグマーカー

```
レンダリングコード:
  SCOPED_DRAW_EVENT(RHICmdList, DrawBasePass)
  {
    // 描画コマンド
  }

変換:
  Begin:
    RHICmdList.PushEvent("DrawBasePass", Color)
    → RHI バックエンド → PushMarker API 呼び出し

  End (RAII デストラクタ):
    RHICmdList.PopEvent()
    → RHI バックエンド → PopMarker API 呼び出し
```

### キャプチャツールでの表示

```
RenderDoc / PIX / NSight:
  Frame
  └─ DrawBasePass
     ├─ DrawOpaque
     └─ DrawTranslucent
```

### 統合ポイント

| ツール | 用途 |
|--------|------|
| グラフィクスデバッガ (RenderDoc, PIX, RGP) | GPU コマンドキャプチャと解析 |
| 検証レイヤー | エラーメッセージにリソース名を含む |
| クラッシュダンプ | リソーステーブルでの名前表示 |
| プロファイラ | GPU イベント階層表示 |

### 不変条件（Invariants）
- 名前は UTF-8/UTF-16（プラットフォーム依存、RHIが変換を処理）
- マーカーの Begin/End はバランス（ミスマッチは検証エラー）
- マーカーはコンテキスト毎（各コマンドバッファが独立スタック）
- カラー値は RGBA（プラットフォームが無視する場合あり、クラッシュはしない）

### 設計判断（Design Decisions）

| 判断 | 理由 |
|------|------|
| ブレッドクラムと分離 | 異なるライフタイム、異なる対象、異なるプラットフォームサポート |
| RAII ラッパー | Begin/End のバランスを保証。例外安全 |
| リソースに名前格納 | 検証レイヤーとクラッシュダンプが必要とする |
| オプションカラー | 一部ツールは無視するがパラメータは受け入れ必要 |

---

## 5. 統計とカウンタ

### 意図（Intent）
詳細プロファイリングのオーバーヘッドなしに、リソース使用量と性能を測定する。

### 統計構造体

実装では汎用カウンタシステムではなく、用途別の構造体を使用する。
`DECLARE_CYCLE_STAT` / `SCOPE_CYCLE_COUNTER` マクロは使用しない。

| 構造体 | 内容 |
|--------|------|
| `RHIDrawCallStats` | 描画コール数（Draw, DrawIndexed, Indirect, Dispatch, Mesh, Rays） |
| `RHIStateChangeStats` | PSO変更、ルートシグネチャ変更、RT変更、ビューポート変更等 |
| `RHIBindingStats` | VB/IB/CB/SRV/UAV/サンプラー/ディスクリプタテーブルバインド |
| `RHIBarrierStats` | テクスチャ/バッファ/UAV/エイリアシングバリア数、冗長バリア数 |
| `RHIMemoryOpStats` | バッファ/テクスチャコピー回数、転送バイト数 |
| `RHICommandListStats` | 上記全ての統合 + コマンド数、レンダーパス数 |

### 統計収集と表示

```
収集（コマンドリスト単位）:
  IRHICommandListStatsCollector::BeginCollecting()
  // コマンド記録 → 各操作でカウンタをインクリメント
  IRHICommandListStatsCollector::EndCollecting()
  → RHICommandListStats を返却

フレーム集計:
  RHIFrameStatsTracker::BeginFrame()
  → 各コマンドリストの統計を AddCommandListStats() で加算
  RHIFrameStatsTracker::EndFrame()
  → RHIFrameStats に集計（CPU記録時間、GPU実行時間含む）

表示:
  RHIPrintFrameStats(stats)         ← コンソール/ログ出力
  RHIExportStatsToCSV(stats, ...)   ← CSV出力
  RHIDrawStatsImGui(tracker)        ← ImGuiウィンドウ
```

### CSV エクスポート

```
ProfileGPU CSV:
  各フレーム:
    全アクティブカウンタをサンプリング
    行書き出し: Frame#, Counter1, Counter2, ...
  ファイル出力: Saved/Profiling/CSV/
```

### 要件（Requirements）
- 複数のカウンタ種類（サイクル、メモリ、アキュムレータ、セット）
- グループ毎の有効/無効制御
- コンソール表示とCSVエクスポート
- コンパイル時除去（Shipping ビルドでゼロオーバーヘッド）

### 不変条件（Invariants）
- INC/DEC ペアはバランス（非ゼロ最終値 = メモリリーク検出）
- サイクルカウンタはスレッド固有（クロススレッドタイミングなし）
- 統計は Shipping でコンパイルアウト（ゼロオーバーヘッド）
- グループメンバーシップは静的（起動時に登録）

### 設計判断（Design Decisions）

| 判断 | 理由 |
|------|------|
| コンパイル時グループ | グループ無効時にマクロ全体が空に。ホットパスの if チェック不要 |
| スレッドローカルサイクルカウンタ | 同期オーバーヘッドなし。照会時にスレッド間集約 |
| INC/DEC（Set ではなく） | 割り当てライフタイム追跡。リーク検出（非ゼロ最終値） |
| ブレッドクラムと分離 | 異なる粒度、異なるオーバーヘッド、異なる用途 |

---

## 6. CVar ベース構成

### 意図（Intent）
再コンパイルなしにデバッグ機能のランタイム制御を提供する。

### CVar 種類

| 型 | 用途 | 例 |
|----|------|-----|
| bool | 機能トグル | r.GPUCrashDebugging.Breadcrumbs |
| int32 | 列挙型/数値閾値 | r.ProfileGPU.Sort |
| float | 時間/パーセンテージ値 | r.GPUHitchThreshold |
| string | パターン/パス | r.ProfileGPU.Pattern |

### 優先度解決

```
優先度（高→低）:
  1. コマンドラインフラグ (-gpucrashdebugging)
  2. コード内 SetConsoleVariable
  3. 設定ファイル (DefaultEngine.ini)
  4. DECLARE でのデフォルト値

起動時:
  デフォルト値 (0)
    → INI ロード (1)
    → コマンドライン解析 → オーバーライド
    → コールバック発火 → 機能初期化
```

### ReadOnly CVar

```
特性:
  - 起動時に1回設定
  - 変更にはリスタートが必要
  - フレーム中の不整合を防止

用途:
  - デバイスレベル機能（デバイス作成後に変更不可）
  - 検証レイヤー（初期化時に有効化必須）
```

### 階層的 CVar

```
r.GPUCrashDebugging              ← マスタートグル
  r.GPUCrashDebugging.Breadcrumbs  ← サブ機能
    r.GPUCrashDebugging.Aftermath.Markers ← ベンダー固有

マスタートグル OFF → 全サブ機能 OFF（単一スイッチ）
```

### 不変条件（Invariants）
- ReadOnly CVar は RHI 初期化後に変更されない
- CVar ポインタは有効のまま維持（静的ライフタイム）
- 変更コールバックは冪等でなければならない（複数回発火の可能性）

---

## 7. 条件コンパイル

### 意図（Intent）
無効化されたデバッグ機能のオーバーヘッドをゼロにする。
Shipping ビルドにはデバッグコードが一切含まれない。

### マクロレベル

```
3段階制御:

NS_RHI_BREADCRUMBS_FULL    = (Debug || Development)
NS_RHI_BREADCRUMBS_MINIMAL = (!FULL && NS_PROFILE_GPU)

NS_RHI_VALIDATION_ENABLED = (Debug || Development)
```

### 階層的依存関係

```
DISABLED ⊂ MINIMAL ⊂ FULL

FULL   → 全機能、全チェック（Development向け）
MINIMAL → 統計のみ、高速（テスト向け）
DISABLED → ゼロコスト、最大性能（Shipping向け）
```

### 仮想メソッド最適化

```
#if NS_RHI_VALIDATION_ENABLED
  virtual void ValidateState() = 0;
#endif

Shipping では vtable エントリなし → バイナリサイズ削減
```

### 不変条件（Invariants）
- マクロのネストが正しい（MINIMAL は WITH を暗示、FULL は MINIMAL を暗示）
- 無効化された機能はコードにコンパイルされない（オプティマイザがデッドブランチを除去）
- パブリックヘッダーに #ifdef なし（API 契約を保持）

### 設計判断（Design Decisions）

| 判断 | 理由 |
|------|------|
| 3レベル | 開発（全機能）/ テスト（統計のみ）/ Shipping（ゼロコスト） |
| ランタイムトグルではない | 分岐ミス予測/キャッシュ汚染のオーバーヘッド。vtable変更不可 |
| マクロネスト | 明示的依存関係。コンパイラが強制 |

---

## 横断的設計パターン

| パターン | 適用箇所 | 説明 |
|---------|---------|------|
| **遅延高コスト操作** | ブレッドクラム、統計、ベンダーSDK | ホットパスは高速、エラー/表示パスは低速許容 |
| **階層的メタデータ** | ブレッドクラム（ツリー）、統計（グループ）、CVar（マスター/サブ） | エンジン構造をミラー。フィルタリング可能。コンテキスト提供 |
| **多段コンパイル** | Full/Minimal/Disabled | ビルド構成にマッチ。不要時はゼロオーバーヘッド |
| **スレッドセーフ・ロックフリー** | ブレッドクラムID（アトミック）、マーカー（コンテキスト毎）、統計（スレッドローカル） | レンダリングは高度にマルチスレッド。ロックは性能を殺す |
| **型安全遅延フォーマット** | ブレッドクラムの TValue テンプレート | ライフタイム問題防止。ランタイムコストなしの安全性 |
| **ラッパー/プロキシ** | 検証（RHIをラップ）、ベンダーSDK（デバイス作成をラップ） | 関心事の分離。有効/無効が容易 |

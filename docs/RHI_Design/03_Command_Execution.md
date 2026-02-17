# RHI 設計仕様 — コマンド実行

---

## 1. コマンド記録モデル

### 意図（Intent）
描画コマンドを記録時に即実行せず、バッファに蓄積して一括実行する。
記録スレッドと実行スレッドを分離し、マルチスレッド並列性を確保する。

### データフロー（Data Flow）
```
Render Thread (記録):          RHI Thread (実行):
┌───────────────────────┐     ┌───────────────────────┐
│ SetPSO(pso)    → 記録 │     │ 記録済みコマンドを     │
│ SetVB(buffer)  → 記録 │ ──▶│ 順次実行して           │
│ Draw(100)      → 記録 │     │ ネイティブAPIに送信    │
│ Transition()   → 記録 │     └───────────────────────┘
└───────────────────────┘
```

### 要件（Requirements）
- コマンドはリニアアロケータで高速に確保すること（malloc/new禁止）
- アロケータはフレーム毎にリセットして再利用すること
- コマンドリスト間のメモリ競合がないこと（スレッドローカル）
- 記録されたコマンドの実行順序は記録順を保証すること

### 不変条件（Invariants）
- コマンド記録中にGPU副作用は発生しない
- アロケータリセットはGPUが全コマンドを実行完了した後にのみ行う

---

## 2. コマンドコンテキスト階層

### 意図（Intent）
GPU操作の種類に応じたインターフェース階層を提供する。
Compute操作はGraphics操作のサブセットであるため、継承関係で表現する。

```
┌──────────────────────────────────────────────────────┐
│  IRHICommandContextBase                              │
│  ├─ ライフサイクル (Begin / Finish / Reset)           │
│  ├─ リソース遷移（バリア）                            │
│  ├─ バッファ/テクスチャコピー                         │
│  ├─ ステージングコピー (CopyToStagingBuffer)          │
│  ├─ MSAA解決 (ResolveTexture)                        │
│  └─ デバッグイベント / ブレッドクラム                  │
└────────────────┬─────────────────────────────────────┘
                 │ extends
                 ▼
┌──────────────────────────────────────────────────────┐
│  IRHIComputeContext (= Base + コンピュート操作)       │
│  ├─ コンピュートディスパッチ                          │
│  ├─ UAVクリア / 読み書き                              │
│  ├─ シェーダーパラメータ設定（コンピュート）           │
│  ├─ ディスクリプタヒープ設定                          │
│  └─ クエリ開始/終了 / タイムスタンプ                  │
└────────────────┬─────────────────────────────────────┘
                 │ extends
                 ▼
┌──────────────────────────────────────────────────────┐
│  IRHICommandContext (= Compute + 描画操作)            │
│  ├─ 描画コマンド (Draw, DrawIndexed, DrawIndirect)    │
│  ├─ レンダーパス開始/終了                             │
│  ├─ ビューポート/シザー設定                           │
│  ├─ 頂点ストリームバインド                            │
│  ├─ メッシュシェーダー (DispatchMesh)                 │
│  └─ レイトレーシング                                  │
└──────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│  IRHIUploadContext （独立、Base を継承）               │
│  ├─ データアップロード（CPU → GPU転送）               │
│  ├─ ステージングバッファ転送                          │
│  └─ Graphics/Computeと並行動作可能                    │
└──────────────────────────────────────────────────────┘
```

### 不変条件（Invariants）
- コンテキスト取得はスレッドセーフ
- コマンド記録はシングルスレッド（1コンテキスト = 1スレッド）
- ファイナライズ後のコンテキストに記録してはならない

---

## 3. レンダーパス

### 意図（Intent）
描画操作のスコープを定義し、レンダーターゲットのロード/ストアアクションを
指定することでタイルベースGPU（モバイル等）の最適化を可能にする。

### ロード/ストアアクション

```
ロードアクション (ERHILoadAction, レンダーパス開始時):
  Load     — 前の内容を保持（メモリから読み込み）
  Clear    — 指定色/深度でクリア（タイルキャッシュ最適化）
  DontCare — 初期内容は不定（GPUは気にしない）
  NoAccess — このパスでは読み書きしない

ストアアクション (ERHIStoreAction, レンダーパス終了時):
  Store           — メモリに書き戻す
  DontCare        — タイルキャッシュを破棄（メモリに書き戻さない）
  Resolve         — MSAAをシングルサンプルにリゾルブ
  StoreAndResolve — MSAA結果を保存し、同時にリゾルブ
  NoAccess        — このパスでは書き込まない
```

### ロード/ストアの組み合わせ例

| 組み合わせ | 用途 |
|-----------|------|
| Clear + Store | 通常のレンダーターゲット（クリアして描画結果を保存） |
| Clear + DontCare | 深度プリパス（描画後に破棄。後でSRVとして読む場合はStoreが必要） |
| Load + Store | 追加描画（前パスの結果に上書き） |
| DontCare + DontCare | テスト用（何も保存しない） |
| Clear + Resolve | MSAA描画→リゾルブ |
| Clear + StoreAndResolve | MSAA描画→保存+リゾルブ |

### 深度/ステンシルの独立制御

深度とステンシルのロード/ストアを独立に指定できる:
- 深度のみ書き込み、ステンシルは保持
- ステンシルのみクリア、深度はロード
- 帯域幅の最適化に直結

### レンダーパスライフサイクル
```
BeginRenderPass(RHIRenderPassDesc):
  1. リソースバリア挿入 (SRV → RTV, SRV → DSV)
  2. レンダーターゲット設定
  3. ロードアクション実行 (Clear / Load)
  4. bInsideRenderPass = true

[描画コマンド]

EndRenderPass():
  1. MSAA解決 (Resolve指定時)
  2. ストアアクション実行 (Store / NoAction)
  3. bInsideRenderPass = false
```

### サブパス（モバイル最適化）

実装では `RHISubpassDesc` / `RHISubpassDependency` 構造体を使用する（Vulkan スタイル）。
事前定義ヒント列挙型ではなく、入出力アタッチメントと依存関係を明示的に記述する。

```
RHISubpassDesc {
  inputAttachments[]      — 入力アタッチメント（前サブパスの出力を読み取り）
  colorAttachments[]      — カラー出力アタッチメント
  resolveAttachments[]    — MSAA解決先アタッチメント
  preserveAttachments[]   — 書き込まないが内容を保持するアタッチメント
  depthStencilAttachment  — デプスステンシルアタッチメント（-1で未使用）
}

RHISubpassDependency {
  srcSubpass, dstSubpass          — 依存元/先サブパスインデックス
  srcStageMask, dstStageMask      — パイプラインステージ（ERHIPipelineStageFlags）
  srcAccessMask, dstAccessMask    — アクセスマスク
}
```

典型的な用途:
- タイルシェーダーで深度を同一パス内で読み取り
- モバイルDeferredシェーディング（GBufferをサブパスで参照）
- カスタムMSAA解決

### 不変条件（Invariants）
- レンダーパスはネストできない（End前に次のBeginを呼んではならない）
- 描画コマンドはレンダーパス内でのみ発行可能
- コンピュートディスパッチはレンダーパス外でのみ発行可能
- PSO のRT構成はレンダーパスのRT構成と一致していること

---

## 4. 描画コマンド分類

### 意図（Intent）
さまざまな描画パターン（シンプル描画〜GPU駆動描画）を
統一的なインターフェースで提供する。

### 分類

| コマンド | 引数 | 用途 |
|---------|------|------|
| Draw | 頂点数, インスタンス数 | プロシージャルジオメトリ |
| DrawIndexed | IndexBuffer, 頂点オフセット, インデックス数, インスタンス数 | 一般的なメッシュ描画 |
| DrawIndirect | ArgumentBuffer, オフセット | GPU駆動描画 |
| DrawIndexedIndirect | ArgumentBuffer, オフセット | GPU駆動メッシュ描画 |
| MultiDrawIndirectCount | ArgumentBuffer, CountBuffer, maxDrawCount, argsStride | GPU駆動バッチ描画 |
| DispatchMesh | groupCountX, groupCountY, groupCountZ | メッシュシェーダーパイプライン |
| DispatchMeshIndirect | ArgumentBuffer, オフセット | GPU駆動メッシュシェーダー |
| DispatchMeshIndirectCount | ArgumentBuffer, CountBuffer, maxDispatchCount | カウント付きGPU駆動メッシュシェーダー |

### GPU駆動描画（Indirect）のデータフロー

```
Compute Pass:
  1. カリング・LOD選択をGPU上で実行
  2. 描画パラメータをArgumentBufferに書き込み
  3. 描画数をCountBufferに書き込み

Graphics Pass:
  4. MultiDrawIndirectCount(ArgumentBuffer, CountBuffer, maxDrawCount, argsStride)
  5. GPUがArgumentBufferから描画パラメータを読み取り実行

→ CPU関与なしで可視オブジェクトのみ描画
```

### Indirect 引数バッファ構造

```
RHIDrawArguments (16 bytes):
  { vertexCountPerInstance, instanceCount,
    startVertexLocation, startInstanceLocation }

RHIDrawIndexedArguments (20 bytes):
  { indexCountPerInstance, instanceCount,
    startIndexLocation, baseVertexLocation, startInstanceLocation }

RHIDispatchMeshArguments (12 bytes):
  { threadGroupCountX, threadGroupCountY, threadGroupCountZ }
```

### 不変条件（Invariants）
- 描画コマンドはレンダーパス内でのみ発行可能
- PSO がバインドされていない状態で描画してはならない
- Indirect 引数バッファは DrawIndirect フラグ付きで作成されていること

---

## 5. コピーコマンド

### 意図（Intent）
GPU上でのリソース間データ転送を提供する。

### コピー種類

| コマンド | 対象 | 説明 |
|---------|------|------|
| CopyBufferRegion | Buffer → Buffer | 指定範囲のバイトコピー |
| CopyTexture | Texture → Texture | リージョン・スライス・Mip指定可能 |
| CopyToStagingBuffer | Resource → IRHIStagingBuffer | GPU → CPU リードバック（専用ステージング型を使用） |
| ResolveTexture | MSAA → NonMSAA | マルチサンプル解決 |

### テクスチャコピー情報
```
RHITextureCopyDesc {
  srcTexture       // ソーステクスチャ
  srcMipLevel      // ソースMipレベル（1レベル指定）
  srcArraySlice    // ソース配列スライス（1スライス指定）
  srcOffset        // Offset3D ソース内オフセット
  dstTexture       // デスティネーションテクスチャ
  dstMipLevel      // デストMipレベル（1レベル指定）
  dstArraySlice    // デスト配列スライス（1スライス指定）
  dstOffset        // Offset3D デスト内オフセット
  extent           // Extent3D コピー範囲 ({0,0,0} = ソースMIP全体)
}
```

※ 実装では1コール = 単一Mip+単一スライスのコピー。複数Mip/スライスをコピーする場合はループで呼び出す。

### 不変条件（Invariants）
- コピーはレンダーパス外でのみ実行可能
- ソースとデストは同一リソースであってはならない（オーバーラップ禁止）
- ソースは CopySrc 状態、デストは CopyDest 状態であること

---

## 6. ステートバインディング

### 意図（Intent）
描画に必要な動的状態（ビューポート、頂点バッファ等）を設定する。
PSO に含まれない動的に変化する状態を対象とする。

### 動的状態一覧

| 状態 | 説明 | 変更頻度 |
|------|------|---------|
| Viewport | 描画領域（X, Y, Width, Height, MinDepth, MaxDepth） | パス毎 |
| ScissorRect | クリッピング矩形 | パス毎 |
| StencilRef | ステンシル参照値 | マテリアル毎 |
| BlendFactor | ブレンド係数 | マテリアル毎 |
| VertexBuffer[N] | 頂点ストリーム（最大16本） | メッシュ毎 |
| IndexBuffer | インデックスバッファ | メッシュ毎 |
| ShaderParameters | シェーダー定数 | ドロー毎 |

### シェーダーパラメータ設定

実装では `SetBatchedShaderParameters` は使用せず、D3D12スタイルのルートパラメータバインディングを採用:

```
ルート定数:
  SetComputeRoot32BitConstants(rootIndex, num32BitValues, data, destOffset)
  SetGraphicsRoot32BitConstants(rootIndex, num32BitValues, data, destOffset)

ルートディスクリプタ (CBV/SRV/UAV):
  SetComputeRootConstantBufferView(rootIndex, bufferAddress)
  SetComputeRootShaderResourceView(rootIndex, bufferAddress)
  SetComputeRootUnorderedAccessView(rootIndex, bufferAddress)
  （Graphics版も同様に提供）

ディスクリプタテーブル:
  SetComputeRootDescriptorTable(rootIndex, baseDescriptor)
  SetGraphicsRootDescriptorTable(rootIndex, baseDescriptor)
```

### 不変条件（Invariants）
- ビューポートは描画前に必ず設定されていること
- 頂点バッファのレイアウトはPSOの頂点宣言と一致していること

---

## 7. クエリシステム

### 意図（Intent）
GPUの計測データ（オクルージョン、タイムスタンプ等）をCPU側に取得する。

### クエリ種類

| 種類 (ERHIQueryType) | 結果 | 用途 |
|------|------|------|
| Occlusion | 深度テストを通過したサンプル数 | オクルージョンカリング |
| BinaryOcclusion | 1ピクセルでも可視なら true | 高速オクルージョン判定 |
| Timestamp | GPU時刻（マイクロ秒） | パフォーマンス計測 |
| PipelineStatistics | 全パイプラインステージの統計値 | プロファイリング |
| StreamOutputStatistics | ストリームアウトの書き込み/必要プリミティブ数 | ストリームアウト監視 |
| StreamOutputOverflow | ストリームアウトバッファオーバーフロー検出 | バッファ溢れ検出 |
| Predication | 条件付きレンダリング用 | GPU駆動の条件分岐 |

### オクルージョンクエリのワークフロー
```
Frame N:
  BeginQuery(occlusionQuery)
  DrawBoundingBox()
  EndQuery(occlusionQuery)

Frame N+2 (2フレーム遅延):
  bool ready = GetQueryResult(occlusionQuery, &sampleCount, bWait=false)
  if (ready && sampleCount > 0) {
      // オブジェクトは可視 → 描画する
  }
```

### タイムスタンプクエリのワークフロー
```
EndQuery(startTimestamp)   // GPU時刻を記録
[GPU作業]
EndQuery(endTimestamp)     // GPU時刻を記録

// 結果取得
GetQueryResult(startTimestamp, &startUs, bWait=true)
GetQueryResult(endTimestamp, &endUs, bWait=true)
double durationMs = (endUs - startUs) / 1000.0
```

### クエリプール

- 単一クエリ作成のオーバーヘッドが大きいため、バッチ割り当て
- プールから取得 → 使用 → 返却 のパターン
- バックエンド側でドライバ効率化が可能

### 不変条件（Invariants）
- bWait=false（ノンブロッキング）でGPU完了前に呼び出した場合、falseを返す
- bWait=true はレンダースレッドからのみ呼び出すこと（RHIスレッドではデッドロックの可能性）
- Occlusionクエリの結果はサンプル数であり、ピクセル数ではない（MSAAで異なる）

---

## 8. コマンド実行パイプライン

### 意図（Intent）
記録されたコマンドのGPU送信から完了までの流れを定義する。

### データフロー（Data Flow）
```
Worker/Render Thread          RHI Thread              GPU
       │                         │                     │
 GetCommandContext()             │                     │
       │                         │                     │
 Record Commands                 │                     │
       │                         │                     │
 FinalizeContext()               │                     │
       │──── PlatformCmdList ──▶│                     │
       │                         │                     │
       │                  SubmitCommandLists()         │
       │                         │──── API Submit ──▶│
       │                         │                     │ Execute
       │                         │                     │
       │                         │          ◀── Fence Signal ──│
       │                         │                     │
       │                  ProcessDeleteQueue()          │
       │                         │                     │
```

### ペイロードフェーズ（コマンド内部の論理順序）

```
1. Wait    — 同期バリアの待機
2. Execute — メインコマンド実行
3. Signal  — 完了シグナル

※バックエンドはこの順序で内部的にコマンドをグループ化する
```

### 並列コマンド記録

```
Thread 0: ├─ context0: DrawChunk(0) ─┤
Thread 1: ├─ context1: DrawChunk(1) ─┤
Thread 2: ├─ context2: DrawChunk(2) ─┤
                    │
                    ▼
          FinalizeContext([ctx0, ctx1, ctx2])
                    │
                    ▼
          SubmitCommandLists([cmdList0, cmdList1, cmdList2])
                    │
                    ▼
          GPU: Execute cmdList0 → cmdList1 → cmdList2 (順序保証)
```

### Bypass パターン（即時実行）

```
通常パス（遅延実行）:
  cmdList.Draw(...)     → コマンドバッファに記録
  ...
  Submit()              → まとめてGPU送信

Bypassパス（即時実行 — IRHIImmediateContext）:
  immediateContext.CopyBuffer(...)  → 即座にバックエンドAPI呼び出し
  immediateContext.Flush()          → GPU送信＆完了待ち
  // 用途: 初期化、デバッグ、GPU readbackの即時完了待ち
  // 制約: RHIスレッドからのみ使用可能、並列記録不可
```

### 不変条件（Invariants）
- Submit の順序がGPU実行順序を決定する
- 並列記録されたコマンドリストのマージ順序は呼び出し側が制御する
- Bypass モードではコマンドバッファリングのオーバーヘッドはゼロ

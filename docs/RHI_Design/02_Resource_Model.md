# RHI 設計仕様 — リソースモデル

---

## 1. リソース階層

### 意図（Intent）
GPUが扱う全オブジェクト（バッファ、テクスチャ、シェーダー等）に共通の
ライフタイム管理と型識別の基盤を提供する。

### 階層構成
```
Resource (参照カウント基底)
  │
  ├─ Buffer
  ├─ Texture
  │
  ├─ Shader (EShaderFrequency で種別を識別)
  │   └─ VS, PS, GS, MS, AS, CS, RayGen, Miss, HitGroup, Callable, WorkGraph
  │
  ├─ Pipeline State Objects
  │   ├─ GraphicsPSO
  │   ├─ ComputePSO
  │   └─ RayTracingPSO
  │
  ├─ Sampler (イミュータブル)
  │
  └─ View (リソースの代替解釈)
      ├─ ShaderResourceView (SRV) — 読み取り専用
      ├─ UnorderedAccessView (UAV) — 読み書き
      ├─ RenderTargetView (RTV) — カラーレンダーターゲット
      ├─ DepthStencilView (DSV) — 深度ステンシル
      └─ ConstantBufferView (CBV) — 定数バッファ
```

### 不変条件（Invariants）
- 全 Resource は参照カウントを持ち、カウントゼロで遅延削除キューに入る
- リソース型は作成時に確定し、以降変更されない
- バッファとテクスチャがビュー（SRV/UAV）を作成できる

---

## 2. バッファ

### 意図（Intent）
GPU上の線形メモリ領域を抽象化し、頂点データ・インデックスデータ・
定数データ・構造化データ等の多様な用途に対応する。

### 要件（Requirements）
- サイズ（バイト数）とストライド（要素サイズ）を持つこと
- 用途フラグの組み合わせで複数の用途に同時対応できること
- Map/Unmap でCPUアクセスを提供すること

### 用途フラグ設計

| フラグ | 用途 | 説明 |
|-------|------|------|
| VertexBuffer | 頂点パイプライン入力 | 頂点属性データ格納 |
| IndexBuffer | インデックスパイプライン入力 | 頂点インデックス格納 |
| StructuredBuffer | シェーダーアクセス | 構造化データ（ストライド付き） |
| ByteAddressBuffer | シェーダーアクセス | バイト単位ランダムアクセス |
| ConstantBuffer | 定数データ | 読み取り専用、小サイズ高速 |
| IndirectArgs | GPU駆動描画 | Indirect描画の引数バッファ |
| ShaderResource | SRV作成可能 | シェーダーから読み取り |
| UnorderedAccess | UAV作成可能 | シェーダーから読み書き |
| CPUReadable | CPU読み取り可能 | Map(Read) で読み取りアクセス |
| CPUWritable | CPU書き込み可能 | Map(Write) で書き込みアクセス |
| AccelerationStructure | レイトレーシング | BLAS/TLAS構造 |

### ライフタイムモデル

| モデル | 更新頻度 | メモリ配置 |
|-------|---------|----------|
| Static | 一度だけ書き込み | DEFAULT (VRAM) |
| Dynamic | 低頻度更新 | DEFAULT + Upload staging |
| Volatile | 毎フレーム更新 | UPLOAD (CPU直接書き込み) |

### CPUアクセスパターン

```
DEFAULT リソース (VRAM):
  Map(WriteOnly)
    ├─ ステージングバッファ (UPLOAD) を割り当て
    └─ ステージングバッファのポインタを返す
  [CPUが書き込み]
  Unmap()
    ├─ CopyBufferRegion (UPLOAD → DEFAULT) を発行
    └─ ステージングバッファを遅延解放

CPU Accessible リソース (UPLOAD):
  Map(WriteOnly)
    └─ GPUメモリを直接マップ
  [CPUが書き込み]
  Unmap()
    └─ アンマップ（コピー不要）

ERHIMapMode は5モードを提供:
  Read, Write, ReadWrite, WriteDiscard, WriteNoOverwrite
```

### 不変条件（Invariants）
- Map中のバッファに対してGPU操作を発行してはならない
- ReadOnlyでMap中にCPU書き込みしてはならない
- Static バッファは初期化後にMapしてはならない

---

## 3. テクスチャ

### 意図（Intent）
2D/3D/Cube等の多次元画像データをGPU上で管理し、
サンプリング・レンダーターゲット・UAVアクセス等の多様な用途に対応する。

### テクスチャ次元

```
Texture1D:     Width × MipLevels
Texture2D:     Width × Height × MipLevels
Texture2DArray: Width × Height × ArraySize × MipLevels
Texture3D:     Width × Height × Depth × MipLevels （ボリューム）
TextureCube:   Width × Width × 6面 × MipLevels
TextureCubeArray: Width × Width × (6 × ArraySize) × MipLevels
```

### テクスチャフラグ設計

| カテゴリ | フラグ例 | 説明 |
|---------|---------|------|
| 用途 | RenderTargetable, DepthStencilTargetable, ShaderResource, UAV | レンダー先か読み取り元か |
| レイアウト | NoTiling, SRGB | メモリ配置とカラースペース |
| アクセス | CPUWritable, CPUReadback, Shared | CPU/プロセス間アクセス |
| 最適化 | FastVRAM, NoFastClear, AtomicCompatible | パフォーマンスヒント |
| プラットフォーム | Memoryless (モバイル), Foveation | プラットフォーム固有 |

### テクスチャ作成パターン（Builder）

```cpp
RHITextureDesc desc = CreateTexture2DDesc(1024, 1024, PF_R8G8B8A8)
    .SetMipLevels(10)
    .SetUsage(ERHITextureUsage::RenderTarget | ERHITextureUsage::ShaderResource)
    .SetInitialState(ResourceState::RenderTarget)
    .SetClearValue(ClearColor::Black)
    .SetDebugName("SceneColor");

TextureRef texture = RHI->CreateTexture(desc);
```

### サブリソースモデル

```
Texture2DArray (4 Mips, 3 Slices):

  Slice 0:  [Mip0] [Mip1] [Mip2] [Mip3]
  Slice 1:  [Mip0] [Mip1] [Mip2] [Mip3]
  Slice 2:  [Mip0] [Mip1] [Mip2] [Mip3]

SubresourceIndex = MipSlice + (ArraySlice × MipLevels)

バリアはサブリソース単位で指定可能:
  - 特定のMipレベルのみ遷移
  - 特定のArrayスライスのみ遷移
  - Depth/Stencilプレーンを独立に遷移
```

### クリア値の最適化

- テクスチャ作成時にデフォルトクリア値を指定可能
- タイルベースGPU（モバイル等）はこの値でFastClear最適化を行う
- 一般的な値: 深度=0.0 or 1.0、カラー=黒(0,0,0,1)

### 不変条件（Invariants）
- テクスチャの次元・フォーマット・Mip数は作成後変更不可
- RenderTargetableフラグなしのテクスチャをレンダーターゲットにバインドしてはならない
- MSAAテクスチャは Resolve 操作を経てから SRV として読み取ること

---

## 4. ビューシステム

### 意図（Intent）
同一リソースの異なる解釈（読み取り専用、読み書き可能、レンダー先等）を
ビューオブジェクトとして提供し、柔軟なリソース参照を可能にする。

### ビュータイプ

| ビュー | アクセス | 対象 | 用途 |
|--------|---------|------|------|
| SRV | 読み取り専用 | Buffer, Texture | テクスチャサンプリング、構造体読み取り |
| UAV | 読み書き | Buffer, Texture | コンピュート出力、アトミック操作 |
| RTV | 書き込み | Texture | カラーレンダーターゲット |
| DSV | 深度テスト | Texture (深度) | 深度ステンシル操作 |

### リソースとビューの関係
```
┌─────────────┐
│  Texture    │───┬──▶ SRV (シェーダーで読み取り)
│  (2048x2048 │   │      └─ Mip 0-4, Array 0-5
│   8 Mips    │   │
│   6 Array)  │   ├──▶ UAV (コンピュートで読み書き)
│             │   │      └─ Mip 0 only
│             │   │
│             │   ├──▶ RTV (レンダーターゲットとして)
│             │   │      └─ Mip 0, Array 0
│             │   │
│             │   └──▶ RTV (別スライスへ)
└─────────────┘          └─ Mip 0, Array 1

※1つのリソースから複数のビューを作成可能
※ビューごとに見える範囲（Mip、Array Slice等）を指定
```

### フォーマット再解釈

- Typeless リソースとして作成 → 異なる型のビューを作成可能
  - 例: `R24G8_TYPELESS` → DSVとして `D24_UNORM_S8_UINT`、SRVとして `R24_UNORM_X8_TYPELESS`
- sRGB再解釈: 同一リソースに対しリニアSRVとsRGB SRVの両方を作成
- エイリアス可能フォーマット配列で互換性を宣言

### 不変条件（Invariants）
- ビューは参照先リソースより長生きしてはならない（リソースがビューを参照カウント管理）
- 同時に書き込みビュー（UAV/RTV）と読み取りビュー（SRV）をバインドしてはならない（バリアが必要）
- ビューの範囲外のサブリソースにアクセスした場合は未定義動作

---

## 5. ピクセルフォーマット抽象化

### 意図（Intent）
プラットフォーム非依存のフォーマット識別子を定義し、
各バックエンドでのネイティブフォーマットへのマッピングを提供する。

### 設計パターン: 静的ルックアップテーブル

```
抽象フォーマットID → ネイティブフォーマット（O(1)変換）

  FormatMap[PF_R8G8B8A8]     → DXGI_FORMAT_R8G8B8A8_UNORM (D3D12)
                              → VK_FORMAT_R8G8B8A8_UNORM (Vulkan)
                              → MTLPixelFormatRGBA8Unorm (Metal)
```

### フォーマットケイパビリティクエリ

各フォーマット×用途の組み合わせでサポートの有無を問い合わせ:

| ケイパビリティ | 説明 |
|-------------|------|
| ShaderSample | テクスチャサンプリング可能 |
| RenderTarget | レンダーターゲット使用可能 |
| UAV | Unordered Access View 可能 |
| DepthStencil | 深度ステンシル使用可能 |
| MSAA | マルチサンプル可能（サンプル数別） |

### 圧縮フォーマット

| ファミリー | ブロックサイズ | 対応API |
|-----------|:------------:|---------|
| BC1-BC5 | 4×4 (8-16B) | D3D12, Vulkan |
| BC6H-BC7 | 4×4 (16B) | D3D12, Vulkan |
| ASTC | 可変 (4×4〜12×12) | Vulkan, Metal |
| ETC2 | 4×4 | Vulkan (Android) |

### 不変条件（Invariants）
- フォーマットマッピングテーブルは初期化後に変更されない
- サポートされないフォーマットの使用はリソース作成時にエラーとなること

---

## 6. リソース状態モデル

### 意図（Intent）
リソースの現在の使用状態（レンダーターゲット、シェーダーリソース等）を追跡し、
必要なキャッシュフラッシュやレイアウト遷移をバリアとして表現する。

### 状態の分類

```
読み取り専用状態（複数同時設定可能）:
  CPURead, IndirectArgs, VertexOrIndexBuffer,
  NonPixelShaderResource, PixelShaderResource, CopySource, ResolveSource,
  RaytracingAccelerationStructure, ShadingRateSource

書き込み専用状態（排他的）:
  RenderTarget, CopyDest, ResolveDest, DepthWrite

特殊:
  DepthRead (DepthWriteと組み合わせ可能)
  UnorderedAccess (読み書き両方)
  Common (ワイルドカード — 完全キャッシュフラッシュ)
```

### 状態合成ルール

```
OK:  PixelShaderResource | NonPixelShaderResource  (複数読み取り)
OK:  DepthRead | DepthWrite                        (深度読み書き同時)
NG:  PixelShaderResource | RenderTarget            (読み取り + 書き込み)
NG:  CPURead | CopyDest                            (読み取り + 書き込み)
```

### 不変条件（Invariants）
- 読み取り専用状態と書き込み専用状態は同時に設定できない（バリアが必要）
- 同一リソースの異なるサブリソースは異なる状態を持てる
- Common 状態からの遷移は完全キャッシュフラッシュを伴う

---

## 7. リソース遷移（バリア）

### 意図（Intent）
リソースの使用状態を変更する際に、GPUのキャッシュ整合性と
メモリレイアウト変換を保証する。

### 遷移情報構造
```
RHITransitionBarrier {
  Resource*          resource       // 対象リソース
  ERHIResourceState  stateBefore    // 遷移前の状態
  ERHIResourceState  stateAfter     // 遷移後の状態
  uint32             subresource    // 対象サブリソース (kAllSubresources で全体)
  ERHIBarrierFlags   flags          // バリアフラグ
}
```

### バッチ遷移

```cpp
// 複数のバリアを一括発行（GPUパイプラインストールを最小化）
RHITransitionBarrier transitions[] = {
    {colorTarget, RenderTarget, PixelShaderResource},
    {depthTarget, DepthWrite, PixelShaderResource},
    {computeOutput, UnorderedAccess, PixelShaderResource}
};
cmdList.Transition(transitions);  // 1回の発行で3つの遷移
```

### スプリットバリア

```
通常のバリア:
  ├─ Transition ─┤  ← GPU待機
  ▼              ▼
  開始          完了 → 使用開始

スプリットバリア:
  ├─ BeginTransition ─────────────────┤
  │       他の無関係な作業             │  ← GPU待機なし
  ├─ EndTransition ───────────────────┤ → 使用開始
```

### パイプライン間遷移

```
Graphics → AsyncCompute:
  Graphics側: BeginTransition (フォーク)
  AsyncCompute側: EndTransition (アクワイア)

AsyncCompute → Graphics:
  AsyncCompute側: BeginTransition
  Graphics側: EndTransition (ジョイン)
```

### 不変条件（Invariants）
- 遷移前の状態がリソースの実際の状態と一致していること
- Begin した遷移は必ず End すること（対になっていること）
- 同一バッチ内の遷移は相互依存しないこと

---

## 8. Transient リソース（フレーム一時リソース）

### 意図（Intent）
フレーム内でのみ使用するリソースのメモリをエイリアシング（共有）し、
GPUメモリ消費を大幅に削減する。

### ライフサイクル
```
1. Acquire  — Transientアロケータからリソースを取得
2. Use      — 描画・コンピュートで使用
3. Discard  — 使用完了を宣言（メモリ再利用可能に）

重ならない期間のリソースは同一メモリを共有:

時間 →
┌────────────┐
│ Shadow Map │  Acquire T0, Discard T2
└────────────┘
              ┌────────────┐
              │ G-Buffer A │  Acquire T2, Discard T4
              └────────────┘
                            ┌────────────┐
                            │ G-Buffer B │  Acquire T4, Discard T6
                            └────────────┘

上記3つは同一メモリ領域を共有可能
```

### 要件（Requirements）
- アロケータはリソースの生存期間を追跡すること
- 生存期間が重ならないリソース同士は同一メモリを共有できること
- ビュー（SRV/UAV）はキャッシュして再生成を避けること
- パイプライン間同期（Graphics ↔ AsyncCompute）のフェンスを管理すること

### 不変条件（Invariants）
- Transient リソースはフレーム境界を跨いで使用してはならない
- 生存期間が重なるリソースはエイリアシングしてはならない
- Discard 後のリソースアクセスは未定義動作

---

## 9. Sparse/Reserved リソース

### 意図（Intent）
仮想アドレス空間を先に予約し、物理メモリを必要に応じてコミットする。
仮想テクスチャやストリーミングに使用する。

### 設計パターン

```
1. リソース作成（ReservedResource フラグ）
   → 仮想アドレス空間のみ割り当て（物理メモリなし）

2. コミット操作
   → 64KB タイルリング単位で物理メモリをバインド

3. 使用
   → コミット済みの範囲のみアクセス可能

4. デコミット
   → 物理メモリを解放（仮想アドレスは保持）
```

### 不変条件（Invariants）
- コミット単位は 65536バイト（64KB）固定
- 未コミット領域へのGPUアクセスは未定義動作
- 新規コミット領域の内容は不定（読み取り前に書き込み必要）
- SRV/UAV は物理メモリのコミット/デコミットで再作成不要

---

## 10. メモリ割り当て戦略

### 意図（Intent）
リソースの用途とサイズに応じた最適なメモリ割り当て戦略を選択し、
フラグメンテーションと割り当てオーバーヘッドのバランスを取る。

### 戦略パターン

| 戦略 | 対象 | アルゴリズム | トレードオフ |
|------|------|------------|------------|
| プールアロケータ | 小バッファ（定数、頂点） | フリーリスト | 高速割り当て、サイズ固定 |
| バディアロケータ | 中サイズリソース | 2の冪分割 | 内部フラグメンテーション |
| リニアアロケータ | 一時データ（Upload） | バンプポインタ | 最速、フレーム単位リセット |
| デフラグアロケータ | 長寿命リソース | コンパクション | 最高効率、コスト大 |

### デフラグメンテーション設計

```
デフラグ前:
  [A][---空き---][B][--空き--][C][D][-空き-]

デフラグ後:
  [A][B][C][D][--------空き--------]

手順:
  1. 移動可能なリソースを特定
  2. コピーコマンドで移動先に転送
  3. 参照を更新
  4. 古い領域を解放
```

### 不変条件（Invariants）
- リニアアロケータのリセットはフレーム境界でのみ行う
- デフラグ中のリソースは移動完了まで使用可能であること（古い位置を保持）

---

## 11. リソースコレクション（バインドレスグループ）

### 意図（Intent）
複数のリソース（テクスチャ、SRV、サンプラー）をグループ化し、
バインドレスディスクリプタヒープ上のハンドルとして効率的にアクセスする。

### 要件（Requirements）
- テクスチャ、SRV、サンプラーをメンバーとして追加できること
- `GetBindlessHandle()` でディスクリプタヒープのインデックスを取得できること
- 個別メンバーの動的更新が可能であること（コレクション全体の再作成不要）

### 設計判断（Design Decisions）
- コレクションは少数のリソースをまとめるバインドレスの「テーブル」
- シェーダーはインデックスでアクセス（スロットベースバインディング不要）
- メンバー更新は `UpdateMember()` / `UpdateMembers()` で行う

> **参考: UE5実装**
> UE5では `FRHIResourceCollection` クラスが提供され、
> テクスチャ、SRV、サンプラーのグループ化を行う。

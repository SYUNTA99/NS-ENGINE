# RHI 設計仕様 — パイプライン状態

---

## 1. シェーダーモデル

### 意図（Intent）
GPUで実行するプログラム（シェーダー）を抽象化し、
バイトコードハンドルとリソースバインディングメタデータとして管理する。

### シェーダー種類

| 種類 | パイプライン | 用途 |
|------|-----------|------|
| Vertex | Graphics | 頂点変換 |
| Hull | Graphics | テッセレーション制御 |
| Domain | Graphics | テッセレーション評価 |
| Pixel | Graphics | ピクセル色決定 |
| Geometry | Graphics | プリミティブ増減（レガシー） |
| Mesh | Graphics | 頂点生成（VS+GS置き換え） |
| Amplification | Graphics | メッシュシェーダー前段 |
| Compute | Compute | 汎用GPU計算 |
| RayGen | RayTracing | レイ発射元 |
| RayMiss | RayTracing | レイ未ヒット時処理 |
| RayClosestHit | RayTracing | 最近接ヒット時処理 |
| RayAnyHit | RayTracing | 任意ヒット判定 |
| RayIntersection | RayTracing | カスタム交差判定 |
| RayCallable | RayTracing | レイトレ内汎用呼び出し |
| WorkGraphRoot | WorkGraph | ワークグラフルート |
| WorkGraphComputeNode | WorkGraph | ワークグラフコンピュートノード |

### シェーダーの構成要素

```
Shader {
  bytecodeHash    // 128-bit ハッシュ（RHIShaderHash、同一性判定用）
  frequency       // シェーダー種類（上記テーブル）
  bytecode        // コンパイル済みバイトコード
}

リソースバインディングは RHIShaderParameterMap / RHIShaderReflection で管理される。
```

### リソースバインディングメタデータ

```
ResourceTable {
  srvMap[]        // SRVバインディング位置 → ユニフォームバッファオフセット
  uavMap[]        // UAVバインディング位置 → オフセット
  samplerMap[]    // サンプラーバインディング位置 → オフセット
  textureMap[]    // テクスチャバインディング位置 → オフセット
}

シェーダーコンパイル時に生成され、ランタイムでリソース解決に使用
```

### 不変条件（Invariants）
- シェーダーは作成後イミュータブル（バイトコード変更不可）
- 同一バイトコードハッシュのシェーダーは同一の動作を保証

---

## 2. 頂点入力

### 意図（Intent）
頂点バッファのメモリレイアウトをシェーダーの入力属性にマッピングする。

### 入力要素記述子

```
RHIInputElementDesc {
  semanticName        // セマンティクス名 ("POSITION", "TEXCOORD", etc.)
  semanticIndex       // セマンティクスインデックス (同名が複数ある場合)
  format              // データ形式 (ERHIPixelFormat)
  inputSlot           // 入力スロット番号 (0-15)
  alignedByteOffset   // スロット内バイトオフセット
  inputClass          // PerVertex / PerInstance
  instanceDataStepRate // インスタンスデータのステップレート (PerInstance時)
}
```

### 複数ストリーム設計

```
Stream 0: [Position(12B)][Normal(12B)]  stride=24
Stream 1: [UV0(8B)][UV1(8B)]           stride=16
Stream 2: [InstanceTransform(48B)]      inputClass=PerInstance, instanceDataStepRate=1

各ストリームは独立した頂点バッファから供給される
→ 部分的なデータ更新が効率的（UV変更時にPositionの再送信不要）
```

### 不変条件（Invariants）
- 入力レイアウトは伝統的パイプライン（VS+PS）でのみ使用
- メッシュシェーダー / ワークグラフでは入力レイアウトは null
- 入力レイアウトはPSOの一部として焼き付けられる

---

## 3. 固定機能状態オブジェクト

### 意図（Intent）
描画中に変更されないGPU状態を不変オブジェクトとして事前作成し、
ドライバー最適化とキャッシュを可能にする。

### 設計原則
- 全状態オブジェクトは作成後イミュータブル
- ポインタ比較で同一性判定可能（キャッシュ効率化）
- 初期化構造体から作成し、`GetInitializer()` で構成を取得可能

### サンプラー状態

| パラメータ | 値 | 説明 |
|-----------|---|------|
| Filter | Point, Linear, Anisotropic (ERHIFilter) | テクスチャフィルタリング方式。Bilinear/Trilinear は min/mag/mip フィルタ設定の組み合わせで構成 |
| AddressU/V/W | Wrap, Clamp, Mirror, Border | UV範囲外の処理 |
| MipBias | float | Mipレベルバイアス |
| MaxAnisotropy | 1-16 | 異方性フィルタリング比 |
| ComparisonFunc | Never, Less, Equal, ... | シャドウ比較関数 |
| BorderColor | ERHIBorderColor (TransparentBlack, OpaqueBlack, OpaqueWhite) + optional float customBorderColor[4] | Border アドレスモード時の色 |

### ラスタライザ状態

| パラメータ | 値 | 説明 |
|-----------|---|------|
| FillMode | Solid, Wireframe, Point | 描画モード |
| CullMode | None, Front, Back (ERHICullMode) | カリングモード |
| frontFace | ERHIFrontFace (CW, CCW) | 前面の巻き方向 |
| DepthBias | float | 深度バイアス（シャドウアクネ防止） |
| SlopeScaleDepthBias | float | 勾配依存深度バイアス |
| depthBiasClamp | float | 深度バイアスの最大値クランプ |
| depthClipEnable | bool | 深度クリッピング有効 |
| multisampleEnable | bool | マルチサンプリング許可 |
| conservativeRaster | ERHIConservativeRasterMode | コンサバティブラスタライゼーション |

### 深度ステンシル状態

| パラメータ | 値 | 説明 |
|-----------|---|------|
| bEnableDepthWrite | bool | 深度書き込み有効 |
| DepthTest | CompareFunc | 深度テスト関数 |
| FrontFace/BackFace | StencilOp×3 + CompareFunc | ステンシル操作 |
| StencilReadMask | 0x00-0xFF | ステンシル読み取りマスク |
| StencilWriteMask | 0x00-0xFF | ステンシル書き込みマスク |

### ブレンド状態

```
Per-RT ブレンド設定 (最大8 RT):
  ColorBlendOp   : Add, Subtract, Min, Max, RevSubtract
  AlphaBlendOp   : (同上)
  srcBlend       : Zero, One, SrcColor, SrcAlpha, DstColor, DstAlpha, ...
  dstBlend       : (同上)
  AlphaSrcBlend  : (同上)
  AlphaDestBlend : (同上)
  ColorWriteMask : R | G | B | A

グローバル設定:
  independentBlendEnable       : RTごとに異なるブレンド設定を使うか
  alphaToCoverageEnable        : MSAA時のアルファカバレッジ
```

### 不変条件（Invariants）
- 状態オブジェクトは作成後に変更不可（スレッドセーフに共有可能）
- 同一初期化パラメータから作成された状態オブジェクトはキャッシュで同一インスタンスを返す

---

## 4. Graphics PSO 構成

### 意図（Intent）
描画に必要な全ての不変状態を単一のオブジェクト（Pipeline State Object）に
焼き付け、GPUドライバーの事前コンパイルと最適化を可能にする。

### PSO の構成要素

```
GraphicsPSO {
  // シェーダー
  vertexShader        // or meshShader or workGraphShader
  hullShader          // optional (テッセレーション制御)
  domainShader        // optional (テッセレーション評価)
  pixelShader         // optional
  geometryShader      // optional (レガシー)

  // 頂点入力
  inputLayout         // VS使用時のみ (RHIInputElementDesc の配列)

  // 固定機能状態
  blendState
  rasterizerState     // conservativeRasterization を含む
  depthStencilState

  // レンダーターゲット構成
  renderTargetFormats  // RHIRenderTargetFormats (最大8 RT + sampleCount)
  depthStencilFormat
  primitiveTopologyType // TriangleList, TriangleStrip, etc.
}
```

> **注意:** `amplificationShader` は MeshPSO に属する。`conservativeRasterization` はラスタライザ状態内で設定する。

### なぜPSOはイミュータブルか

| 理由 | 説明 |
|------|------|
| GPU事前コンパイル | ドライバーがPSO作成時にGPUマイクロコードを生成 |
| 状態検証 | 作成時に全組み合わせの整合性を検証 |
| ポインタ比較 | 同一PSO判定がO(1)（マテリアルソート高速化） |
| スレッドセーフ | イミュータブルなので同期不要 |

### PSO 同一性判定

```
等価比較に含む:
  シェーダーポインタ、状態オブジェクトポインタ、
  プリミティブタイプ、全RT/DS構成

等価比較に含まない（ヒントのみ）:
  プリキャッシュフラグ、ファイルキャッシュ起源フラグ
```

---

## 5. Compute PSO

### 意図（Intent）
コンピュートパイプラインは Graphics PSO より大幅にシンプル。
シェーダー1つのみで構成される。

```
ComputePSO {
  computeShader    // コンピュートシェーダーのみ
}
```

### 設計判断（Design Decisions）
- 頂点宣言、ブレンド、ラスタライザ、深度ステンシル、RT構成は不要
- 等価比較はコンピュートシェーダーポインタのみ
- Graphics PSO と同じキャッシュ機構を共有

---

## 6. PSO キャッシュ戦略

### 意図（Intent）
PSO作成はGPUドライバーのコンパイルを伴う高コスト操作であるため、
多層キャッシュで重複作成を排除し、ロード時間を短縮する。

### 3層キャッシュ

```
Level 1: ランタイムメモリキャッシュ（最速）
  ┌─────────────────────────────────────────────────┐
  │ PSOInitializer → ポインタベースルックアップ       │
  │ ヒット → キャッシュ済みPSOを返す                  │
  └─────────────────────────────────────────────────┘
                    │ ミス
                    ▼
Level 2: ディスクキャッシュ（永続的）
  ┌─────────────────────────────────────────────────┐
  │ PSO ハッシュ(uint64) → シリアライズ済みバイナリ   │
  │ ヒット → デシリアライズしてLevel 1に格納           │
  └─────────────────────────────────────────────────┘
                    │ ミス
                    ▼
Level 3: バックエンドネイティブ作成（最遅）
  ┌─────────────────────────────────────────────────┐
  │ シェーダーバイトコード + 状態 → ドライバーコンパイル│
  │ 作成後 Level 1 に格納                            │
  └─────────────────────────────────────────────────┘
```

### プリキャッシュ（事前コンパイル）

```
ロード時:
  1. レベルが使用するマテリアルからPSO候補を列挙
  2. バックグラウンドスレッドで非同期コンパイル
  3. 描画時にはキャッシュヒット → ヒッチなし

フォールバック:
  非同期コンパイル完了前に描画が必要な場合:
  → フォールバックPSO（簡略化された状態）を使用
  → 完了後にスワップ
```

### LRU エビクション

- メモリ制限に達した場合、最も使用されていないPSOを破棄
- 再度必要になった場合は Level 2 (ディスク) から復元
- キャッシュサイズは設定可能

### 不変条件（Invariants）
- 同一の初期化パラメータからは常に同一のPSOが返されること
- キャッシュからの取得はスレッドセーフであること
- 非同期コンパイル中のPSOアクセスはフォールバックを返すこと

---

## 7. シェーダーバインディングレイアウト

### 意図（Intent）
シェーダーがアクセスするリソース（テクスチャ、バッファ、サンプラー）の
配置情報を定義し、バックエンドがディスクリプタセットを構築できるようにする。

### バインディングの種類

| 種類 | 説明 | 更新頻度 |
|------|------|---------|
| UniformBuffer | 定数データ（行列、色等） | フレーム/ドロー毎 |
| SRV (Texture) | テクスチャサンプリング | マテリアル毎 |
| SRV (Buffer) | 構造化バッファ読み取り | シーン毎 |
| UAV | 読み書きバッファ/テクスチャ | ディスパッチ毎 |
| Sampler | テクスチャフィルタリング設定 | マテリアル毎 |

### バッチパラメータ設計

```
効率的なパラメータ設定:
  全シェーダーパラメータを1つの構造体にまとめて一括転送

  BatchedShaderParameters {
    uniformBuffers[N]   // ユニフォームバッファ配列
    srvs[M]             // SRV配列
    samplers[K]         // サンプラー配列
    uavs[L]             // UAV配列
  }

  SetBatchedShaderParameters(shader, params)
  → 1回のAPI呼び出しで全バインディング更新
```

### 不変条件（Invariants）
- シェーダーのリソーステーブルとバインドされたリソースの型が一致していること
- UAV バインディングはコンピュートPSO またはピクセルシェーダー（限定的）でのみ有効

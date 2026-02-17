# RHI 設計仕様 — コアアーキテクチャ

---

## 1. レイヤーモデル

### 意図（Intent）
エンジンの描画コードをグラフィックスAPI（D3D12, Vulkan, Metal等）から完全に分離し、
プラットフォーム非依存なレンダリングパイプラインを実現する。

### データフロー（Data Flow）
```
┌─────────────────────────────────────────┐
│         ゲーム / エンジンコード           │
│    （マテリアル、メッシュ、ライティング等）   │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│              レンダラー                   │
│     （Deferred/Forward、シャドウ等）       │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│          RHI 抽象化レイヤー              │  ← 本ドキュメントの対象
│    （プラットフォーム非依存のAPI）         │
└─────────────────────────────────────────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐
   │ D3D12   │ │ Vulkan  │ │  Metal  │  ...
   │ Backend │ │ Backend │ │ Backend │
   └─────────┘ └─────────┘ └─────────┘
        │           │           │
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐
   │DirectX12│ │ Vulkan  │ │  Metal  │
   │  Driver │ │ Driver  │ │ Driver  │
   └─────────┘ └─────────┘ └─────────┘
```

### 要件（Requirements）
- レンダラーはバックエンド固有のAPIを一切参照しないこと
- バックエンド追加がレンダラーの修正を必要としないこと
- 開発時: バックエンド選択がランタイムで切替可能であること（ディスパッチテーブル方式）
- 出荷時: コンパイル時にバックエンドが確定し、関数呼び出しがインライン化可能であること

### 不変条件（Invariants）
- RHI層より上のコードにネイティブAPI型（ID3D12Device*, VkDevice等）が漏洩しない
- 同一フレーム内でバックエンドを切り替えることはない
- ディスパッチテーブルのエントリは初期化後 null でないこと

---

## 2. モジュール構成

### 意図（Intent）
RHIをコアモジュール（インターフェース定義）と複数のバックエンドモジュール（実装）に分離し、
バックエンドの追加・削除をビルド設定の変更のみで行えるようにする。

### 構成パターン
```
Engine/Source/Runtime/
├── RHI/                    # コアモジュール（インターフェース定義のみ）
│   ├── Public/             # 公開ヘッダー（全バックエンド共通）
│   └── Private/            # 共通実装（コマンドリスト基底等）
│
├── RHICore/                # 共通ユーティリティ（フォーマット変換等）
│
├── D3D12RHI/               # DirectX 12 バックエンド
├── VulkanRHI/              # Vulkan バックエンド
├── MetalRHI/               # Metal バックエンド
├── D3D11RHI/               # DirectX 11 バックエンド（レガシー）
└── OpenGLDrv/              # OpenGL バックエンド（レガシー）
```

### 責務（Responsibilities）

| モジュール | 責務 |
|-----------|------|
| RHI (コア) | インターフェース定義、型定義、共通ロジック |
| RHICore | フォーマットテーブル、ユーティリティ関数 |
| バックエンド | ネイティブAPI呼び出し、メモリ管理、コマンド変換 |

### 設計判断（Design Decisions）
- バックエンドは**ディスパッチテーブル（関数ポインタテーブル）**で切り替える
  - 理由: 仮想関数のvtableオーバーヘッドを回避
  - 理由: 出荷時にはコンパイル時選択で関数ポインタ自体も除去可能
- **開発モード**: 全バックエンドを1バイナリに含み、起動時にテーブルを差し替えてランタイム切替
- **出荷モード**: プラットフォーム毎に別バイナリをビルドし、直接関数呼び出し（インライン化可能）
- プラットフォーム固有のSDK依存はバックエンドソースに閉じ込める
- RHICore は薄いユーティリティ層として分離する
  - 理由: バックエンド間で共有するロジック（ピクセルフォーマットテーブル等）の重複を防ぐ

---

## 3. バックエンドインターフェース契約

### 意図（Intent）
全バックエンドが実装すべきAPIをディスパッチテーブル（関数ポインタテーブル）で定義し、
仮想関数のvtableオーバーヘッドなしにバックエンドを差し替え可能にする。

### ディスパッチテーブル設計

> **実装との対応:**
> ディスパッチテーブル（`RHIDispatchTable`）は **コマンドコンテキスト上の操作**（Draw, Dispatch, バリア、コピー等）の
> 関数ポインタのみを含む。リソース生成（CreateBuffer, CreateTexture等）やライフサイクル管理（Init, Shutdown等）は
> `IDynamicRHI` クラスの仮想メソッドとして提供される。
> グローバルテーブル変数名は `GRHIDispatchTable`。

```cpp
// コマンドコンテキスト操作のディスパッチテーブル（抜粋）
struct RHIDispatchTable {
    // Base: プロパティ
    IRHIDevice* (*GetDevice)(IRHICommandContextBase* ctx);
    GPUMask (*GetGPUMask)(IRHICommandContextBase* ctx);
    ERHIQueueType (*GetQueueType)(IRHICommandContextBase* ctx);

    // Base: ライフサイクル
    void (*Begin)(IRHICommandContextBase* ctx, IRHICommandAllocator* allocator);
    IRHICommandList* (*Finish)(IRHICommandContextBase* ctx);
    void (*Reset)(IRHICommandContextBase* ctx);
    bool (*IsRecording)(IRHICommandContextBase* ctx);

    // Base: リソースバリア
    void (*TransitionResource)(IRHICommandContextBase* ctx, IRHIResource*, ERHIAccess, ERHIAccess);
    void (*UAVBarrier)(IRHICommandContextBase* ctx, IRHIResource*);
    void (*AliasingBarrier)(IRHICommandContextBase* ctx, IRHIResource*, IRHIResource*);
    void (*FlushBarriers)(IRHICommandContextBase* ctx);

    // Base: コピー（バッファ、テクスチャ、ステージング）
    void (*CopyBuffer)(IRHICommandContextBase* ctx, IRHIBuffer* dst, IRHIBuffer* src);
    void (*CopyTexture)(IRHICommandContextBase* ctx, IRHITexture* dst, IRHITexture* src);
    // ... 他コピー操作

    // Compute: ディスパッチ
    void (*Dispatch)(IRHIComputeContext* ctx, uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ);
    void (*DispatchIndirect)(IRHIComputeContext* ctx, IRHIBuffer* argsBuffer, uint64 argsOffset);

    // Compute: クエリ（IRHIQueryHeap* + queryIndex パターン）
    void (*BeginQuery)(IRHIComputeContext* ctx, IRHIQueryHeap* queryHeap, uint32 queryIndex);
    void (*EndQuery)(IRHIComputeContext* ctx, IRHIQueryHeap* queryHeap, uint32 queryIndex);
    bool (*GetQueryResult)(IRHIComputeContext* ctx, IRHIQueryHeap* queryHeap,
                           uint32 queryIndex, uint64* outResult, bool bWait);

    // Graphics: 描画
    void (*Draw)(IRHICommandContext* ctx, uint32 vertexCount, uint32 instanceCount, ...);
    void (*DrawIndexed)(IRHICommandContext* ctx, uint32 indexCount, uint32 instanceCount, ...);

    // Graphics: レンダーパス
    void (*BeginRenderPass)(IRHICommandContext* ctx, const RHIRenderPassDesc* desc);
    void (*EndRenderPass)(IRHICommandContext* ctx);

    // Graphics: ビューポート・シザー
    void (*SetViewports)(IRHICommandContext* ctx, uint32 count, const RHIViewport* viewports);
    void (*SetScissorRects)(IRHICommandContext* ctx, uint32 count, const RHIRect* rects);

    // Upload: データ転送
    void (*UploadBuffer)(IRHIUploadContext* ctx, IRHIBuffer* dst, uint64 dstOffset, const void* srcData, uint64 srcSize);
    void (*UploadTexture)(IRHIUploadContext* ctx, IRHITexture* dst, uint32 dstMip, uint32 dstSlice, ...);

    // 検証メソッド
    bool IsValid() const;                   // 必須エントリの null チェック
    bool HasMeshShaderSupport() const;       // オプショナル: メッシュシェーダー
    bool HasRayTracingSupport() const;       // オプショナル: レイトレーシング
    bool HasWorkGraphSupport() const;        // オプショナル: ワークグラフ
    bool HasVariableRateShadingSupport() const; // オプショナル: VRS
};

// リソースファクトリ・ライフサイクルは IDynamicRHI の仮想メソッド:
class IDynamicRHI {
    virtual TRefCountPtr<IRHIBuffer> CreateBuffer(const RHIBufferDesc&) = 0;
    virtual TRefCountPtr<IRHITexture> CreateTexture(const RHITextureDesc&) = 0;
    virtual TRefCountPtr<IRHIShader> CreateShader(const RHIShaderDesc&) = 0;
    virtual TRefCountPtr<IRHISwapChain> CreateSwapChain(const RHISwapChainDesc&, void* windowHandle) = 0;
    virtual TRefCountPtr<IRHIQueryHeap> CreateQueryHeap(const RHIQueryHeapDesc&) = 0;
    virtual TRefCountPtr<IRHIFence> CreateFence(uint64 initialValue) = 0;
    virtual void PopulateDispatchTable(RHIDispatchTable& table) = 0;
    // ...
};
```

> **注意: クエリAPI** — `CreateQuery` は存在しない。クエリヒープを `IDynamicRHI::CreateQueryHeap()` で作成し、
> `IRHIQueryHeap*` + `uint32 queryIndex` の組でクエリを操作する。
>
> **注意: Viewport概念の不在** — 実装ではViewport（CreateViewport / ResizeViewport / GetBackBuffer）は使用しない。
> 代わりに `IDynamicRHI::CreateSwapChain()` で `IRHISwapChain` を作成し、スワップチェーン経由で
> バックバッファを管理する。

### テーブル初期化パターン

```cpp
// バックエンドは IDynamicRHI::PopulateDispatchTable() を実装
// テーブルをコピーで返すのではなく、参照渡しで設定する
class D3D12DynamicRHI : public IDynamicRHI {
    void PopulateDispatchTable(RHIDispatchTable& table) override {
        table.Draw = &D3D12_Draw;
        table.DrawIndexed = &D3D12_DrawIndexed;
        table.Dispatch = &D3D12_Dispatch;
        table.TransitionResource = &D3D12_TransitionResource;
        // ... 全エントリを設定
    }
};

// グローバルテーブル（開発モード: ランタイム切替）
extern RHIDispatchTable GRHIDispatchTable;  // グローバルインスタンス

// 初期化フロー:
//   1. IDynamicRHIModule を使い IDynamicRHI* を作成
//   2. IDynamicRHI::Init() でデバイス/キュー初期化
//   3. IDynamicRHI::PopulateDispatchTable(GRHIDispatchTable) でテーブル設定
//   4. GRHIDispatchTable.IsValid() で検証

// ERHIInterfaceType: バックエンド種別
// 実装では以下の値を持つ:
//   Hidden  — テスト用・内部実装
//   Null    — ヘッドレス（Null実装）
//   D3D11   — DirectX 11
//   D3D12   — DirectX 12
//   Vulkan  — Vulkan
//   Metal   — Metal (macOS/iOS)

// レンダラーからの呼び出し（コマンドコンテキスト操作）
// NS_RHI_DISPATCH マクロ経由:
//   開発ビルド: GRHIDispatchTable.Draw(ctx, ...)
//   出荷ビルド: D3D12_Draw(ctx, ...) に展開（インライン化可能）

// リソース作成は IDynamicRHI 経由:
auto buf = GetDynamicRHI()->CreateBuffer(desc);
auto tex = GetDynamicRHI()->CreateTexture(desc);
```

### 出荷モード: コンパイル時バックエンド選択

```cpp
// NS_RHI_DISPATCH マクロが出荷/開発で切り替わる:

#if NS_BUILD_SHIPPING && defined(NS_RHI_STATIC_BACKEND)
    // 出荷ビルド: バックエンド関数を直接呼び出し（インライン化可能）
    // 例: NS_RHI_DISPATCH(Draw, ctx, ...) → D3D12_Draw(ctx, ...)
    #define NS_RHI_DISPATCH(func, ...) \
        ::NS::RHI::NS_RHI_EXPAND(NS_RHI_STATIC_BACKEND, func)(__VA_ARGS__)
#else
    // 開発ビルド: グローバルテーブル経由の間接呼び出し
    // 例: NS_RHI_DISPATCH(Draw, ctx, ...) → GRHIDispatchTable.Draw(ctx, ...)
    #define NS_RHI_DISPATCH(func, ...) \
        ::NS::RHI::GRHIDispatchTable.func(__VA_ARGS__)
#endif

// ビルド成果物:
//   Game_D3D12.exe  — D3D12 関数が直接インライン化
//   Game_Vulkan.exe — Vulkan 関数が直接インライン化
```

### ディスパッチテーブル必須エントリ（~90関数ポインタ）

> **注意:** このテーブルはコマンドコンテキスト操作のみ。
> リソースファクトリ（CreateBuffer等）やライフサイクル（Init, Shutdown）は `IDynamicRHI` の仮想メソッド。

| カテゴリ | エントリ例 | 責務 |
|---------|----------|------|
| Base: プロパティ | GetDevice, GetGPUMask, GetQueueType | コンテキスト情報取得 |
| Base: ライフサイクル | Begin, Finish, Reset, IsRecording | コマンド記録制御 |
| Base: バリア | TransitionResource, UAVBarrier, FlushBarriers | リソース状態遷移 |
| Base: コピー | CopyBuffer, CopyTexture, CopyBufferToTexture | データ転送 |
| Base: デバッグ | BeginDebugEvent, EndDebugEvent, InsertBreadcrumb | 診断・プロファイリング |
| Immediate | Flush, GetNativeContext | 即時実行コンテキスト |
| Compute: ディスパッチ | Dispatch, DispatchIndirect, DispatchIndirectMulti | コンピュート実行 |
| Compute: バインド | SetComputePipelineState, SetComputeRootSignature | パイプライン設定 |
| Compute: クエリ | BeginQuery, EndQuery, GetQueryResult, WriteTimestamp | GPU計測 |
| Graphics: 描画 | Draw, DrawIndexed, DrawIndirect, MultiDrawIndirect | 描画コマンド |
| Graphics: レンダーパス | BeginRenderPass, EndRenderPass, NextSubpass | レンダーパス管理 |
| Graphics: ビューポート | SetViewports, SetScissorRects | 出力領域設定 |
| Graphics: バインド | SetGraphicsPipelineState, SetRenderTargets | グラフィックス設定 |
| Upload | UploadBuffer, UploadTexture, CopyStagingToTexture | CPU→GPU転送 |

### オプショナルエントリ（~55関数ポインタ、null許容）

対応しないバックエンドは null を設定。呼び出し側でケイパビリティチェック後に呼ぶ:
- メッシュシェーダー / アンプリフィケーションシェーダー
- レイトレーシング（AS構築、SBT管理、ディスパッチ）
- 仮想テクスチャ
- GPUブレッドクラム
- ワークグラフ

```cpp
// オプショナル呼び出しパターン
auto support = GetDynamicRHI()->GetFeatureSupport(ERHIFeature::RayTracing);
if (IsFeatureSupported(support) && GRHIDispatchTable.HasRayTracingSupport()) {
    ctx->BuildRaytracingAccelerationStructure(blasDesc);
}
```

### 不変条件（Invariants）
- 必須エントリが null のバックエンドは `Init()` で失敗すること（起動時検証）
- オプショナルエントリは null 可能。呼び出し前にケイパビリティチェックを行うこと
- `GetName()` は一意なバックエンド識別文字列を返すこと
- テーブルは `Init()` 完了後に変更されないこと

### Vulkan 自体がこの設計

```
Vulkan API の関数取得:
  vkGetInstanceProcAddr("vkCreateDevice") → 関数ポインタ
  vkGetDeviceProcAddr("vkCmdDraw")       → 関数ポインタ

同じ思想:
  - ランタイムでドライバーから関数ポインタを取得
  - 呼び出しはテーブル経由
  - ローダー層が間接呼び出しを1段挟む
```

---

## 4. 初期化シーケンス

### 意図（Intent）
バックエンドの選択とディスパッチテーブルの初期化を段階的に行い、
失敗時のフォールバックやコマンドライン指定による上書きを可能にする。

### データフロー（Data Flow）

```
開発モード（ランタイム切替）:
  PlatformCreateDynamicRHI()
         │
         ▼
  IDynamicRHIModule::IsSupported()   ← 各バックエンドモジュールに問い合わせ
         │
         ▼
  IDynamicRHIModule::CreateRHI()     ← IDynamicRHI* インスタンス作成
         │
         ▼
  IDynamicRHI::Init()                ← デバイス/キュー作成
         │
         ▼
  IDynamicRHI::PopulateDispatchTable(GRHIDispatchTable) ← テーブル設定
         │
         ▼
  GRHIDispatchTable.IsValid()        ← 必須エントリの null チェック
         │
         ▼
  IDynamicRHI::PostInit()            ← 追加初期化、ケイパビリティ確定

出荷モード（コンパイル時選択）:
  // GRHIDispatchTable は不使用。NS_RHI_DISPATCH マクロが直接関数呼び出しに展開
  // 全コンテキスト操作が #define で直結（LTOでインライン化可能）
```

### テーブル検証

```cpp
// 実装では RHIDispatchTable のメンバメソッドで検証する

// 必須エントリの一括検証（全 null チェック）
bool ok = GRHIDispatchTable.IsValid();
// → GetDevice, Begin, Finish, TransitionResource, CopyBuffer, Draw,
//    DrawIndexed, Dispatch, SetViewports, BeginRenderPass, UploadBuffer,
//    ... 約90個の必須関数ポインタを全て検証

// オプショナルエントリの個別検証（ケイパビリティ依存）
bool meshOk = GRHIDispatchTable.HasMeshShaderSupport();
//  → SetMeshPipelineState, DispatchMesh, DispatchMeshIndirect,
//    DispatchMeshIndirectCount が全て非null

bool rtOk = GRHIDispatchTable.HasRayTracingSupport();
//  → BuildRaytracingAccelerationStructure, CopyRaytracingAccelerationStructure,
//    SetRaytracingPipelineState, DispatchRays が全て非null

bool wgOk = GRHIDispatchTable.HasWorkGraphSupport();
//  → SetWorkGraphPipeline, DispatchGraph, InitializeWorkGraphBackingMemory

bool vrsOk = GRHIDispatchTable.HasVariableRateShadingSupport();
//  → SetShadingRate, SetShadingRateImage
```

### バックエンド選択の優先度

```
1. コマンドライン指定  (-d3d12, -vulkan, -metal)
2. ユーザー設定        (設定ファイル)
3. プロジェクトデフォルト (プロジェクト設定ファイル)
4. 自動選択            (利用可能なバックエンドを順番に試行)
5. 機能レベルフォールバック (SM6.x → SM6.0 → SM5)
```

### 要件（Requirements）
- `IsSupported()` はGPUハードウェアとドライバーの両方を検証すること
- テーブル設定後に `GRHIDispatchTable.IsValid()` で全必須エントリの存在を確認すること
- `Init()` が失敗した場合、次の候補バックエンドでテーブルを差し替えてリトライすること
- 初期化完了後にケイパビリティフラグが確定し、以降変更されないこと

### 不変条件（Invariants）
- `Init()` はレンダースレッド起動**前**に呼ばれること
- ディスパッチテーブルは `Init()` 完了後に変更されないこと
- 必須エントリが1つでも null ならバックエンド初期化は失敗とすること

---

## 5. ハードウェア階層モデル

### 意図（Intent）
物理GPUからコマンドキューまでの階層を抽象化し、
シングルGPUからマルチGPU（SLI/CrossFire）まで統一的に扱えるようにする。

```
┌─────────────────────────────────────────────────────────────┐
│                     RHI Backend                             │
│                  （RHI最上位オブジェクト）                     │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  Adapter (物理GPU)                   │   │
│  │            （1つの物理GPU/GPU群を表現）                │   │
│  │                                                      │   │
│  │  ┌──────────────────┐  ┌──────────────────┐        │   │
│  │  │     Device       │  │     Device       │        │   │
│  │  │  （GPUノード0）   │  │  （GPUノード1）   │        │   │
│  │  │                  │  │   ※SLI/CFX時    │        │   │
│  │  │  ┌────────────┐ │  │                  │        │   │
│  │  │  │   Queue    │ │  │                  │        │   │
│  │  │  │ (Graphics) │ │  │                  │        │   │
│  │  │  └────────────┘ │  │                  │        │   │
│  │  │  ┌────────────┐ │  │                  │        │   │
│  │  │  │   Queue    │ │  │                  │        │   │
│  │  │  │ (Compute)  │ │  │                  │        │   │
│  │  │  └────────────┘ │  │                  │        │   │
│  │  │  ┌────────────┐ │  │                  │        │   │
│  │  │  │   Queue    │ │  │                  │        │   │
│  │  │  │  (Copy)    │ │  │                  │        │   │
│  │  │  └────────────┘ │  │                  │        │   │
│  │  └──────────────────┘  └──────────────────┘        │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │               Adapter (別の物理GPU)                  │   │
│  │          （例：内蔵GPU + 外付けGPU構成時）             │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 責務（Responsibilities）

| 概念 | 責務 | RHI APIでの位置 |
|------|------|----------------|
| Adapter (`IRHIAdapter`) | GPU列挙、ケイパビリティ報告 | 公開インターフェース（`IDynamicRHI::GetAdapter()` 経由） |
| Device (`IRHIDevice`) | リソース作成、メモリ管理、ディスクリプタ割り当て、キュー管理 | 公開インターフェース（`IDynamicRHI::GetDefaultDevice()` 経由） |
| Queue (`IRHIQueue`) | コマンド送信、GPU完了追跡、フェンス操作 | 公開インターフェース（`IRHIDevice::GetQueue()` 経由） |

### 設計判断（Design Decisions）
- Adapter/Device/Queue は **公開インターフェース**（`IRHIAdapter`, `IRHIDevice`, `IRHIQueue`）として RHI API に露出する
  - `IDynamicRHI` から `GetAdapter()`, `GetDefaultDevice()` でアクセス可能
  - `IRHIDevice` から `GetQueue(ERHIQueueType)`, `GetGraphicsQueue()`, `GetComputeQueue()` でキューにアクセス可能
  - バックエンド固有の実装はこれらのインターフェースを継承して提供する
- マルチGPUはビットマスク（GPUMask）で抽象化する
  - 理由: レンダラーは「どのGPUか」ではなく「どのGPU群に対して」コマンドを発行するだけ

### 不変条件（Invariants）
- シングルGPU環境ではGPUMaskは常にビット0のみセット
- マルチGPU環境では全Deviceが同一のAdapter内に属する

---

## 6. コマンドコンテキスト階層

### 意図（Intent）
GPUに送信するコマンドの記録インターフェースを、4レベルの継承階層で定義する。

```
┌──────────────────────────────────────────────────────┐
│  IRHICommandContextBase （基底）                     │
│  ├─ ライフサイクル（Begin, Finish, Reset）           │
│  ├─ プロパティ（GetDevice, GetGPUMask, GetQueueType）│
│  ├─ リソースバリア（TransitionResource, UAVBarrier） │
│  ├─ コピー操作（Buffer, Texture, Staging, MSAA解決） │
│  └─ デバッグ（BeginDebugEvent, InsertBreadcrumb等）  │
└──────┬──────────────────┬────────────────────────────┘
       │                  │
       ▼                  ▼
┌────────────────┐ ┌──────────────────────────────────┐
│IRHIImmediate-  │ │  IRHIComputeContext              │
│Context         │ │  （IRHICommandContextBaseを継承） │
│（Baseを継承）  │ │  ├─ コンピュートディスパッチ      │
│ ├─ Flush()     │ │  ├─ UAVクリア                    │
│ └─ GetNative() │ │  ├─ ルートシグネチャ/定数設定    │
└────────────────┘ │  ├─ ディスクリプタヒープ管理      │
                   │  └─ クエリ（BeginQuery, EndQuery） │
                   └──────────┬───────────────────────┘
                              │ 継承
                              ▼
              ┌──────────────────────────────────────────────┐
              │  IRHICommandContext （IRHIComputeContextを継承）│
              │  ├─ 全コンピュート操作（継承）                │
              │  ├─ 描画操作（Draw, DrawIndexed, Indirect等） │
              │  ├─ レンダーパス開始/終了                     │
              │  ├─ ビューポート/シザー設定                   │
              │  ├─ 頂点/インデックスバッファバインド          │
              │  ├─ メッシュシェーダー（DispatchMesh等）      │
              │  ├─ レイトレーシング（DispatchRays等）        │
              │  ├─ ワークグラフ（DispatchGraph等）           │
              │  └─ Variable Rate Shading                    │
              └──────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│  IRHIUploadContext （IRHICommandContextBaseを継承）   │
│  ├─ UploadBuffer（CPU→GPU バッファ転送）             │
│  ├─ UploadTexture（CPU→GPU テクスチャ転送）          │
│  ├─ CopyStagingToTexture / CopyStagingToBuffer      │
│  └─ Graphics/Computeと並行動作可能（コピーキュー）    │
└──────────────────────────────────────────────────────┘
```

### 要件（Requirements）
- IRHICommandContext は IRHIComputeContext の全操作を含むこと（is-a 関係）
- IRHIComputeContext は IRHICommandContextBase の全操作を含むこと
- IRHIUploadContext は IRHICommandContextBase を継承するが、Compute/Graphicsとは独立
- コンテキスト取得はスレッドセーフであること
- コンテキストへのコマンド記録はシングルスレッドであること（1コンテキスト = 1スレッド）
- コンテキストのファイナライズ（記録完了）はスレッドセーフであること

### 不変条件（Invariants）
- ファイナライズ後のコンテキストにコマンドを記録してはならない
- 描画操作はレンダーパス内でのみ呼び出せる
- コンピュートディスパッチはレンダーパス外でのみ呼び出せる

---

## 7. コマンドライフサイクル

### 意図（Intent）
コマンドの記録から実行完了までの明確なライフサイクルを定義し、
各段階での責務と遷移条件を明確にする。

```
    ┌─────────────┐
    │   Obtain    │  ← GetCommandContext() でコンテキスト取得
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Record    │  ← コマンドを記録（Open状態）
    │             │     SetPSO, Draw, Dispatch, Transition 等
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Finalize   │  ← FinalizeContext() → PlatformCommandList
    └──────┬──────┘    （コンテキストは使用不可に）
           │
           ▼
    ┌─────────────┐
    │   Submit    │  ← SubmitCommandLists() でGPUキューに送信
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Execute   │  ← GPU実行（ドライバー制御下）
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Complete  │  ← GPU実行完了（フェンスシグナル）
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Release   │  ← プールに返却（再利用）
    └─────────────┘
```

### コマンド実行のスレッド分担

```
Worker/Render Thread:
  1. GetCommandContext(Pipeline, GPUMask)
     → ComputeContext* or GraphicsContext*

  2. コマンド記録
     SetPSO(), SetShaderParams(), Draw(), Dispatch()...

  3. FinalizeContext(contexts[], uploadContext)
     → PlatformCommandList[]（提出準備完了）

RHI Thread:
  4. SubmitCommandLists(commandLists[], uploadContext)
     → GPU実行開始（順序保証）
```

### 不変条件（Invariants）
- Submit の順序はコマンドリストの実行順序を決定する
- Finalize は Submit より前に呼ばれなければならない
- 同一コンテキストの Record → Finalize は同一スレッドで行うこと

---

## 8. パイプラインタイプ

### 意図（Intent）
GPUの異なる処理パイプラインを分類し、
並列実行可能なキューの組み合わせを明確にする。

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         パイプラインタイプ                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   Graphics Pipeline                    AsyncCompute Pipeline            │
│   ┌─────────────────────┐             ┌─────────────────────┐          │
│   │ ・頂点シェーダー      │             │ ・コンピュートシェーダー│          │
│   │ ・ピクセルシェーダー   │             │ ・非同期実行           │          │
│   │ ・ジオメトリシェーダー │             │ ・Graphicsと並列動作  │          │
│   │ ・テッセレーション    │             └─────────────────────┘          │
│   │ ・メッシュシェーダー   │                                             │
│   │ ・ラスタライズ        │                                             │
│   │ ・コンピュートも可能  │                                             │
│   └─────────────────────┘                                             │
│                                                                         │
│   Graphics キューはコンピュートも実行可能                                │
│   AsyncCompute キューはコンピュートのみ実行可能                          │
│   両キューは並列実行可能（明示的同期が必要）                              │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 責務（Responsibilities）
- **Graphics Pipeline**: 描画全般 + コンピュート
- **AsyncCompute Pipeline**: コンピュートのみ（描画と同時並行可能）

---

## 9. GPUメモリモデル

### 意図（Intent）
GPUメモリの種類とアクセスパターンを分類し、
リソース配置戦略の設計指針を提供する。

### メモリタイプ

```
┌────────────────────────────────────────────────────────┐
│                    DEFAULT (VRAM)                       │
│  ・GPUローカルメモリ（最速）                             │
│  ・テクスチャ、レンダーターゲット、頻繁に使うバッファ      │
│  ・CPUから直接アクセス不可（Stagingバッファ経由）          │
├────────────────────────────────────────────────────────┤
│                    UPLOAD (CPU → GPU)                   │
│  ・CPUで書き込み、GPUで読み取り                          │
│  ・頂点/インデックスバッファのアップロード                │
│  ・定数バッファ（ユニフォームバッファ）                   │
│  ・Write-Combined メモリ（CPU読み取りは非常に遅い）       │
├────────────────────────────────────────────────────────┤
│                    READBACK (GPU → CPU)                 │
│  ・GPUで書き込み、CPUで読み取り                          │
│  ・スクリーンショット、オクルージョンクエリ結果           │
│  ・GPUからCPUへのデータ転送用                            │
└────────────────────────────────────────────────────────┘
```

### リソース配置戦略

```
1. Committed Resource（専用割り当て）
   ┌──────────────────────────────┐
   │    リソース専用のメモリ領域     │  ← 大きなリソース、レンダーターゲット
   └──────────────────────────────┘

2. Placed Resource（ヒープ上に配置）
   ┌────────────────────────────────────────────────────┐
   │                      Heap                          │
   │  ┌─────────┐ ┌─────────┐ ┌─────────┐             │
   │  │Resource1│ │Resource2│ │Resource3│  ...         │  ← メモリ効率重視
   │  └─────────┘ └─────────┘ └─────────┘             │
   └────────────────────────────────────────────────────┘

3. Sub-allocation（サブアロケーション）
   ┌────────────────────────────────────────────────────┐
   │                   大きなバッファ                     │
   │  ┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐            │
   │  │ A  ││ B  ││ C  ││ D  ││ E  ││ F  │  ...       │  ← 小さなバッファ群
   │  └────┘└────┘└────┘└────┘└────┘└────┘            │
   └────────────────────────────────────────────────────┘

4. Reserved Resource（予約リソース）
   ・物理メモリバッキングなしで仮想アドレスを予約
   ・必要に応じて物理メモリをコミット（スパース/タイルド）
```

### 設計判断（Design Decisions）

| 戦略 | 適用場面 | トレードオフ |
|------|---------|------------|
| Committed | レンダーターゲット、大テクスチャ | メモリ効率低、割り当て高速 |
| Placed | 中サイズリソース群 | メモリ効率高、管理複雑 |
| Sub-allocation | 定数バッファ、小バッファ | 最高効率、フラグメンテーション |
| Reserved | 仮想テクスチャ、ストリーミング | 柔軟、API制約あり |

---

## 10. ケイパビリティシステム

### 意図（Intent）
バックエンドとハードウェアの組み合わせで利用可能な機能を
実行時にクエリできる仕組みを提供する。

### 要件（Requirements）
- 全ケイパビリティフラグは `Init()` 完了後に確定すること
- ブール型フラグ（サポート有無）とティア型（機能レベル）の2種類を提供すること
- フラグ値はスレッドセーフに読み取り可能であること

> **実装での対応:**
> 実装では `ERHIFeature` enum と `IDynamicRHI::GetFeatureSupport(ERHIFeature)` メソッドで
> ケイパビリティをクエリする。戻り値は `ERHIFeatureSupport`（Unsupported / RuntimeDependent / RuntimeGuaranteed）。
>
> ```cpp
> enum class ERHIFeature : uint32 {
>     WaveOperations, RayTracing, MeshShaders, VariableRateShading,
>     Bindless, WorkGraphs, EnhancedBarriers, AtomicInt64,
>     // ... 約25項目
>     Count
> };
>
> // 使用例:
> auto support = GetDynamicRHI()->GetFeatureSupport(ERHIFeature::RayTracing);
> if (IsFeatureSupported(support)) { /* レイトレーシング有効 */ }
> ```

### カテゴリ分類

| カテゴリ | 例 | 用途 |
|---------|---|------|
| Supports* | レイトレーシング、メッシュシェーダー、バインドレス | 機能の有無 |
| Needs* | ステンシルクリア分離、UAVフォーマット互換フラグ | プラットフォーム回避策 |
| Requires* | 最小アライメント、テクスチャ制限 | ハードウェア制約 |
| Tier | レイトレーシングティア、VRSティア | 段階的機能レベル |

### 設計パターン: プレビューオーバーライド

```
エディタ等のツールで、実際のハードウェアと異なるケイパビリティを
シミュレートする必要がある場面:

┌────────────────────────────────────────────┐
│ CapabilityFlag                             │
│   ├─ actualValue  (ハードウェアの実際の値)  │
│   ├─ previewValue (エディタでの仮の値)      │
│   └─ isPreview    (プレビューモードか)      │
│                                            │
│ GetValue():                                │
│   return isPreview ? previewValue : actualValue │
└────────────────────────────────────────────┘
```

---

## 11. 機能レベル

### 意図（Intent）
GPUのケイパビリティを段階的なレベルに分類し、
レンダラーが「このレベル以上」で使える機能セットを簡潔に判定できるようにする。

### レベル定義

| レベル | 対応API | 主な機能 |
|-------|--------|---------|
| SM5   | D3D11 FL11, Vulkan 1.0, Metal 2 | デスクトップ標準 |
| SM6.0 | D3D12 SM6.0, Vulkan 1.1 | Wave Intrinsics |
| SM6.1 | D3D12 SM6.1, Vulkan 1.1 | SV_ViewID, Barycentric |
| SM6.2 | D3D12 SM6.2, Vulkan 1.1 | Float16, Denorm制御 |
| SM6.3 | D3D12 SM6.3, Vulkan 1.2 | DirectX Raytracing (DXR) |
| SM6.4 | D3D12 SM6.4, Vulkan 1.2 | VRS (Tier 2)、整数ドット積 |
| SM6.5 | D3D12 SM6.5, Vulkan 1.2 | メッシュシェーダー、サンプラーフィードバック |
| SM6.6 | D3D12 SM6.6, Vulkan 1.3 | 動的リソース、Atomic Int64、Wave計算 |
| SM6.7 | D3D12 SM6.7, Vulkan 1.3 | Wave Matrix、Relaxed Precision |

### 設計判断（Design Decisions）
- 機能レベルは**上位互換**: SM6.x は SM5 および SM6.0〜SM6.(x-1) の全機能を含む
- 個別のケイパビリティフラグと併用する（SM6.5でも全GPUがワークグラフを持つとは限らない）

---

## 12. 新規バックエンド実装ロードマップ

### 意図（Intent）
新しいグラフィックスAPI向けバックエンドを追加する際の、
段階的な実装順序と最小動作要件を明確にする。

```
Phase 1: 基盤構築
─────────────────
  1. モジュール作成
     ・BackendModule: IsSupported() / CreateRHI()
     ・RHIBackend: Init() / Shutdown() / GetName()
  2. 階層構造の実装
     ・Adapter (GPU列挙)
     ・Device (デバイス作成)
     ・Queue (コマンドキュー)

Phase 2: リソース作成
─────────────────────
  3. メモリ管理
     ・ヒープ / アロケーター
  4. リソース実装
     ・Buffer / Texture / View (SRV, UAV)

Phase 3: パイプライン
─────────────────────
  5. シェーダー
     ・各シェーダータイプ (VS, PS, CS, ...)
  6. パイプラインステート
     ・Graphics PSO / Compute PSO / PSO キャッシュ

Phase 4: コマンド実行
─────────────────────
  7. コマンドコンテキスト
     ・ComputeContext / GraphicsContext 全メソッド
     ・FinalizeContext / SubmitCommandLists
  8. 同期機構
     ・Fence (Create/Poll/Wait)
     ・Transition (Begin/End)

Phase 5: プレゼンテーション
───────────────────────────
  9. スワップチェーン
     ・CreateSwapChain（IDynamicRHI / IRHIDevice経由）
     ・GetBackBuffer / Present / Resize
```

### 要件（Requirements）
- Phase 1-5 を順に実装することで段階的に画面出力まで到達できること
- Phase 2 完了で単色クリアが表示できること（最初のマイルストーン）
- Phase 4 完了で三角形が描画できること（基本動作確認）
- Phase 5 完了で画面にPresent できること（最低限の実用レベル）

---

## 13. 同期メカニズム概要

### 意図（Intent）
CPU-GPU間、GPU内（キュー間）の同期プリミティブを定義する。
（詳細は `05_Synchronization.md` を参照）

### フェンス

```
CPU
───┬────────────────────────────────────────────▶
   │
   │ Submit CmdList1    Submit CmdList2    Wait(2)
   │      │                  │                │
   │      ▼                  ▼                ▼
GPU│   ┌──────┐          ┌──────┐
───┼───│CmdL1 │──────────│CmdL2 │──Signal(2)──▶
   │   └──────┘          └──────┘          │
   │                                       │
   │                              Fence Value: 1 → 2
```

### リソース状態遷移（バリア）

```
┌─────────────────┐     Barrier      ┌─────────────────┐
│  RenderTarget   │ ──────────────▶ │ ShaderResource  │
│   (書き込み中)   │                  │   (読み取り)     │
└─────────────────┘                  └─────────────────┘
```

### 同期パターン

| パターン | 用途 | メカニズム |
|---------|------|----------|
| フレーム同期 | スワップチェーン、リソース削除 | フレームフェンス |
| リードバック | GPU→CPU データ取得 | ステージングバッファ + フェンス + ポーリング |
| 非同期コンピュート | Graphics と Compute の並列実行 | クロスキューフェンス |

> **参考: UE5実装**
> UE5では `FRHIGPUFence` (Clear/Poll/Wait)、`RHICreateTransition` +
> `RHIBeginTransitions` / `RHIEndTransitions` で同期を行う。

---

## 14. マルチGPU設計

### 意図（Intent）
複数のGPUを協調動作させるための抽象化を提供する。

### 要件（Requirements）
- GPUマスク（ビットマスク）でリソースの所属GPUを指定できること
- GPU間のリソース転送APIを提供すること
- シングルGPU環境で追加のオーバーヘッドがないこと

### 不変条件（Invariants）
- シングルGPU環境ではGPUマスクは常にビット0のみ（1）
- リソース転送は明示的なAPIでのみ行われる（暗黙の同期なし）

```
Multi-GPU:
  GPU0: ┌─ Render Left Eye ─┐─── Transfer ──▶ GPU0 Present
  GPU1: ┌─ Render Right Eye ─┐─── Transfer ──▶
```

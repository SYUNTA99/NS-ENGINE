# D3D12RHI Part 12: ビューポート・シェーダー・レンダーターゲット

## 1. ビューポート管理

### 1.1 アーキテクチャ概要

`FD3D12Viewport`はDXGIスワップチェーンの管理、バックバッファの割り当て、
HDRカラースペース制御、フルスクリーン遷移、およびPresent操作を担当する。

```
┌──────────────────────────────────────────────────────┐
│                    FD3D12DynamicRHI                    │
│  RHICreateViewport() / RHIResizeViewport()            │
│  RHITick() → ConditionalResetSwapChain()              │
│  RHIAdvanceFrameForGetViewportBackBuffer()            │
│  RHIGetViewportBackBuffer()                           │
└─────────┬────────────────────────────────────────────┘
          │
    ┌─────▼──────────────────────────────────────────────┐
    │     FD3D12Viewport (FRHIViewport, FD3D12AdapterChild)│
    │                                                      │
    │  IDXGISwapChain1..4      スワップチェーンインターフェース│
    │  TArray<FBackBufferData>  バックバッファ配列            │
    │  DummyBackBuffer_RenderThread  プロキシテクスチャ      │
    │  FrameSyncPoints         フレームペーシング同期点      │
    │  CustomPresent           VR/XRプラグインフック         │
    │  ColorSpace              DXGI_COLOR_SPACE_TYPE         │
    └──────────────────────────────────────────────────────┘
```

### 1.2 FD3D12Viewport

```
class FD3D12Viewport : public FRHIViewport, public FD3D12AdapterChild
{
    // ウィンドウ
    const HWND WindowHandle;
    uint32 SizeX, SizeY;
    EPixelFormat PixelFormat;
    bool bIsFullscreen, bFullscreenLost, bIsValid;
    bool bAllowTearing;           // DXGI_FEATURE_PRESENT_ALLOW_TEARING
    bool bNeedSwapChain;          // false = -RenderOffScreen時

    // スワップチェーン
    TRefCountPtr<IDXGISwapChain1> SwapChain1;
    TRefCountPtr<IDXGISwapChain2> SwapChain2;  // フレームレイテンシ制御
    TRefCountPtr<IDXGISwapChain3> SwapChain3;  // mGPU対応
    TRefCountPtr<IDXGISwapChain4> SwapChain4;  // HDR対応

    // バックバッファ
    TArray<FBackBufferData, TInlineAllocator<3>> BackBuffers;
    TRefCountPtr<FD3D12Texture> DummyBackBuffer_RenderThread; // プロキシ

    // カラースペース (D3D12RHI_USE_DXGI_COLOR_SPACE)
    DXGI_COLOR_SPACE_TYPE ColorSpace;  // デフォルト: sRGB

    // フレーム同期
    TArray<FD3D12SyncPointRef> FrameSyncPoints;

    // カスタムPresent (VR/XR)
    FCustomPresentRHIRef CustomPresent;

    // mGPU
    int32 BackbufferMultiGPUBinding = 0; // INDEX_NONE=サイクル、それ以外=固定GPUインデックス
    uint32 CheckedPresentFailureCounter; // 一時的エラーリトライ（最大5回）

    // 静的メンバー
    static FCriticalSection DXGIBackBufferLock; // DXGI競合ワークアラウンド
};
```

**FBackBufferData:**

| フィールド | 条件 | 説明 |
|-----------|------|------|
| `Texture` | 常時 | メインバックバッファ FD3D12Texture |
| `TextureSDR` | `D3D12RHI_USE_SDR_BACKBUFFER` | ゲームDVR/放送用SDRコピー |
| `UAV` | `D3D12RHI_SUPPORTS_UAV_BACKBUFFER` | コンピュートアクセス用UAV |
| `GPUIndex` | `WITH_MGPU` | このバッファを所有するGPU |

### 1.3 プラットフォーム条件フラグ

| マクロ | Windows値 | 説明 |
|--------|-----------|------|
| `D3D12RHI_DEFAULT_NUM_BACKBUFFER` | 3 | トリプルバッファリング |
| `D3D12_VIEWPORT_EXPOSES_SWAP_CHAIN` | 1 | スワップチェーン直接アクセス可 |
| `D3D12RHI_USE_DUMMY_BACKBUFFER` | 1 | レンダースレッド用プロキシテクスチャ使用 |
| `D3D12RHI_USE_SDR_BACKBUFFER` | 0 | SDRバックバッファ（Windows: 無効） |
| `D3D12RHI_SUPPORTS_UAV_BACKBUFFER` | 0 | UAVバックバッファ（Windows: 無効） |
| `D3D12RHI_USE_DXGI_COLOR_SPACE` | 1 | カラースペース管理有効 |

### 1.4 初期化フロー

```
FD3D12Viewport::Init():
  ┌─────────────────────────────────────────────────────┐
  │ 1. レンダリングフラッシュ + GPUアイドル待機            │
  │    (リサイクルHWNDの安全性保証)                       │
  ├─────────────────────────────────────────────────────┤
  │ 2. IDXGISwapchainProviderモジュラーフィーチャー検索    │
  │    (カスタムスワップチェーンプロバイダ対応)             │
  ├─────────────────────────────────────────────────────┤
  │ 3. DXGI機能チェック                                   │
  │    IDXGIFactory5::CheckFeatureSupport(ALLOW_TEARING)  │
  │    Factory2->IsWindowedStereoEnabled() (Quad-Buffer)  │
  ├─────────────────────────────────────────────────────┤
  │ 4. スワップチェーン作成                               │
  │    IDXGIFactory2::CreateSwapChainForHwnd()            │
  │    SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD         │
  │    Scaling = DXGI_SCALING_NONE                        │
  │    Flags = ALLOW_MODE_SWITCH (+ ALLOW_TEARING)        │
  │    Usage = RENDER_TARGET_OUTPUT | SHADER_INPUT         │
  ├─────────────────────────────────────────────────────┤
  │ 5. SwapChain2/3/4 QueryInterface                     │
  │    MakeWindowAssociation(NO_WINDOW_CHANGES)           │
  ├─────────────────────────────────────────────────────┤
  │ 6. Resize() → バックバッファ作成                      │
  │ 7. PostMessageW(WM_PAINT) → 初回ウィンドウ再描画      │
  └─────────────────────────────────────────────────────┘
```

### 1.5 バックバッファ作成

`GetSwapChainSurface()`がバックバッファテクスチャのファクトリ関数として機能する。

```
3つのリソース取得パス:
  1. SwapChain->GetBuffer(Index) ← 通常パス
  2. BackBufferResourceOverride  ← オーバーライド
  3. CreateCommittedResource     ← ウィンドウレス/PixelStreaming

FD3D12Texture作成:
  Adapter->CreateLinkedObject<FD3D12Texture>() (mGPUリンクオブジェクト)

  所有GPU: FD3D12Resource(SwapChainリソース) → AsStandAlone
  他GPU:   SafeCreateTexture2D (互換テクスチャ)

ビュー作成:
  RTV: 2つ (Quad-Buffer Stereo: Left/Right) または 1つ (通常)
  SRV: バックバッファのシェーダー読み取り用
  UAV: (D3D12RHI_SUPPORTS_UAV_BACKBUFFER有効時のみ)

テクスチャフラグ: RenderTargetable | Presentable | ResolveTargetable
```

**DummyBackBufferパターン:**
`FD3D12BackBufferReferenceTexture2D`は実際のD3D12リソースを持たないプロキシテクスチャで、
アクセス時にRHIスレッドの現在バックバッファに動的に解決する。
レンダースレッドとRHIスレッド間のバックバッファインデックス同期問題を解消する。

### 1.6 Present フロー

```
FD3D12CommandContextBase::RHIEndDrawingViewport():
  │
  ▼ (bPresent == true)
FD3D12Viewport::Present():
  │
  ├── IsPresentAllowed() チェック
  │
  ├── 各GPU: バックバッファ取得 → リソースバリアフラッシュ
  │          → コマンドフラッシュ (WaitForSubmission)
  │
  ├── SyncInterval = bLockToVsync ? RHIGetSyncInterval() : 0
  │
  └── PresentChecked():
        ├── フルスクリーン状態検証 (bFullscreenLost検出)
        ├── CustomPresent->Present() (登録済みの場合)
        ├── PresentInternal(SyncInterval):
        │     ├── DXGI_PRESENT_ALLOW_TEARING (VSync無効+ウィンドウ+ティアリング許可)
        │     ├── FThreadHeartBeat::PresentFrame()
        │     ├── SwapChain1->Present(SyncInterval, Flags)
        │     └── GRHIPresentCounter更新
        ├── 一時的エラーリトライ (最大5回)
        └── CustomPresent->PostPresent()

フレームペーシング:
  WaitForFrameEventCompletion() → 前フレームの全SyncPoint待機
  IssueFrameEvent() → 現フレームのSyncPointシグナル
  制御: r.FinishCurrentFrame (Wait-then-Issue vs Issue-then-Wait)
```

### 1.7 Resize フロー

```
FD3D12Viewport::Resize():
  1. レンダリングフラッシュ + GPUアイドル
  2. 全GPUのコマンドコンテキストステートクリア
  3. CustomPresent通知 (OnBackBufferResize)
  4. UAVバックバッファ解放 (+ 遅延削除フラッシュ)
  5. 全バックバッファテクスチャ解放 (Main + SDR)
  6. Presentキュークリア
  7. 最終フラッシュ + アイドル待機

  8. サイズ変更: SwapChain1->ResizeTarget() (フルスクリーン時)
  9. フルスクリーン変更: ConditionalResetSwapChain(true)

  10. HDRメタデータ照会 (HDRGetMetaData)

  11. ResizeInternal():
      mGPU (GNumExplicitGPUsForRendering > 1):
        各バックバッファにGPU割り当て (サイクル or 固定)
        SwapChain3->ResizeBuffers1() (GPU単位キュー+ノードマスク)
        GPU別にGetSwapChainSurface()

      シングルGPU:
        SwapChain1->ResizeBuffers()
        GetSwapChainSurface() (Device 0)

      SwapChain3->GetCurrentBackBufferIndex() で初期インデックス取得
      DummyBackBuffer作成

  12. HDR有効化 or シャットダウン
```

### 1.8 HDRカラースペース管理

```
EnsureColorSpace(Gamut, OutputFormat):

  ┌──────────────────────────────┬──────────────────────────────┐
  │ UE OutputFormat              │ DXGI_COLOR_SPACE_TYPE         │
  ├──────────────────────────────┼──────────────────────────────┤
  │ SDR_sRGB / SDR_Rec709        │ RGB_FULL_G22_NONE_P709       │
  │ (Rec2020 Gamut時)            │ RGB_FULL_G22_NONE_P2020      │
  │ HDR_ACES_1000nit_ST2084      │ RGB_FULL_G2084_NONE_P2020    │
  │ HDR_ACES_2000nit_ST2084      │ RGB_FULL_G2084_NONE_P2020    │
  │ HDR_ACES_1000nit_ScRGB       │ RGB_FULL_G10_NONE_P709 (リニア)│
  │ HDR_ACES_2000nit_ScRGB       │ RGB_FULL_G10_NONE_P709 (リニア)│
  └──────────────────────────────┴──────────────────────────────┘

  1. SwapChain4->CheckColorSpaceSupport() でサポート検証
  2. SwapChain4->SetColorSpace1() で適用

HDR検出:
  CurrentOutputSupportsHDR():
    SwapChain4->GetContainingOutput() → IDXGIOutput6
    IDXGIOutput6::GetDesc1() → DXGI_OUTPUT_DESC1
    HDR = (ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
```

### 1.9 スレッドモデル

```
3スレッドがビューポートと相互作用:

  ゲームスレッド:
    RHICreateViewport, RHIResizeViewport
    RHITick → ConditionalResetSwapChain

  レンダースレッド:
    AdvanceExpectedBackBufferIndex
    GetBackBuffer_RenderThread → DummyBackBufferプロキシ経由
    RHIAdvanceFrameForGetViewportBackBuffer

  RHIスレッド:
    Present, GetBackBuffer_RHIThread
    SetBackBufferIndex_RHIThread (実バックバッファポインタ更新)

同期:
  ExpectedBackBufferIndexLock (FCriticalSection)
  DummyBackBuffer → 動的解決でインデックス不整合を回避
```

---

## 2. シェーダー階層

### 2.1 FD3D12ShaderData（共通ミックスイン）

全シェーダータイプが多重継承で使用する共通データ。

```
class FD3D12ShaderData  // 基底クラスなし（ミックスイン）
{
    TArray<uint8> Code;                    // 生シェーダーバイトコード
    FShaderCodePackedResourceCounts ResourceCounts; // リソースバインディングカウント
    uint32 ShaderBindingLayoutHash;        // 静的バインディングレイアウトハッシュ

    // 条件付きフィールド
    TArray<FShaderCodeVendorExtension> VendorExtensions; // D3D12RHI_NEEDS_VENDOR_EXTENSIONS
    EShaderCodeFeatures Features;                        // D3D12RHI_NEEDS_SHADER_FEATURE_CHECKS

    // メソッド
    D3D12_SHADER_BYTECODE GetShaderBytecode();
    ShaderBytecodeHash GetBytecodeHash();         // DXBCヘッダー offset+4 の128bitチェックサム
    bool UsesDiagnosticBuffer();
    bool UsesGlobalUniformBuffer();
    bool UsesBindlessResources();
    bool UsesBindlessSamplers();
    bool UsesRootConstants();
};
```

### 2.2 シェーダータイプ階層

全シェーダーは二重継承パターン:
`FRHIXxxShader`（RHI抽象インターフェース）+ `FD3D12ShaderData`（D3D12固有データ）

```
FD3D12ShaderData (mixin)
    │
    ├── FD3D12VertexShader         (FRHIVertexShader)         SF_Vertex
    ├── FD3D12MeshShader           (FRHIMeshShader)           SF_Mesh
    ├── FD3D12AmplificationShader  (FRHIAmplificationShader)  SF_Amplification
    ├── FD3D12GeometryShader       (FRHIGeometryShader)       SF_Geometry
    ├── FD3D12PixelShader          (FRHIPixelShader)          SF_Pixel
    │                              [+ EntryPoint (D3D12_RHI_WORKGRAPHS_GRAPHICS)]
    ├── FD3D12ComputeShader        (FRHIComputeShader)        SF_Compute
    │                              [+ FD3D12RootSignature* RootSignature]
    ├── FD3D12WorkGraphShader      (FRHIWorkGraphShader)      (動的)
    │                              [+ RootSignature, EntryPoint]
    └── FD3D12RayTracingShader     (FRHIRayTracingShader)     (動的)
                                   [+ LocalRootSignature, EntryPoint,
                                    AnyHitEntryPoint, IntersectionEntryPoint,
                                    bPrecompiledPSO]
```

**ルートシグネチャの保持:**
- Compute / WorkGraph / RayTracing: シェーダーオブジェクトが直接保持
- Graphics (VS/MS/AS/GS/PS): PSOから取得（シェーダーには持たない）

**EntryPointの保持（メモリ最適化）:**
テンプレート特殊化`CanReadEntryPoint()`でエントリポイント文字列の格納を制御。
必要なタイプのみ: WorkGraphShader、PixelShader（`D3D12_RHI_WORKGRAPHS_GRAPHICS`時）、
RayTracingShader。

### 2.3 FD3D12VertexDeclaration

```
class FD3D12VertexDeclaration : public FRHIVertexDeclaration
{
    FD3D12VertexElements VertexElements;
    //  = TArray<D3D12_INPUT_ELEMENT_DESC, TFixedAllocator<MaxVertexElementCount>>
    //  ヒープ割り当てを回避する固定サイズ配列

    TStaticArray<uint16, MaxVertexElementCount> StreamStrides;
    uint32 Hash;           // ストライド含むハッシュ
    uint32 HashNoStrides;  // PSOプリキャッシュ用（ストライド除外）

    bool GetInitializer(FVertexDeclarationElementList& Init);  // D3D12→UE逆変換
    uint32 GetPrecachePSOHash();                     // HashNoStridesを返す
};
```

**D3D12_INPUT_ELEMENT_DESC型特性:**
```cpp
template <> struct TTypeTraits<D3D12_INPUT_ELEMENT_DESC>
    { enum { IsBytewiseComparable = true }; };
// → memcmpベースの高速比較を有効化
```

### 2.4 シェーダー作成パイプライン

```
RHICreateXxxShader(Code):
  │
  ▼
InitStandardShader<TShaderType>() / InitStandardShaderWithCustomSerialization<>():
  │
  ├── SRT（Shader Resource Table）デシリアライズ
  ├── カスタムLambda（型固有初期化、例: RT EntryPoint読み取り）
  ├── InitShaderCommon():
  │     ├── ReadShaderOptionalData():
  │     │     ├── FShaderCodePackedResourceCounts (必須)
  │     │     ├── デバッグデータ (RHI_INCLUDE_SHADER_DEBUG_DATA)
  │     │     ├── VendorExtensions (D3D12RHI_NEEDS_VENDOR_EXTENSIONS)
  │     │     ├── FeatureFlags (D3D12RHI_NEEDS_SHADER_FEATURE_CHECKS)
  │     │     ├── ShaderBindingLayoutHash
  │     │     └── EntryPoint (CanReadEntryPoint()がtrueの型のみ)
  │     ├── ValidateShaderIsUsable():
  │     │     ├── Mesh/Amplification → GRHISupportsMeshShadersTier0
  │     │     ├── WaveOps → GRHISupportsWaveOperations (RT除外)
  │     │     ├── Bindless → GRHIGlobals.bSupportsBindless
  │     │     ├── StencilRef → GRHISupportsStencilRefFromPixelShader
  │     │     └── Barycentrics → GRHIGlobals.SupportsBarycentricsSemantic
  │     └── Code配列にネイティブバイトコードをコピー
  │
  ├── Static Uniform Bufferスロット初期化
  └── 失敗時: AddRef+Release (クリーンアップ) → nullptr

特殊ケース:
  ComputeShader: GetAdapter().GetRootSignature() でRS作成
  WorkGraphShader: GetAdapter().GetRootSignature() でRS作成
  RayTracingShader: EntryPoint/AnyHit/Intersection読み取り
                    GetLocalRootSignature() でローカルRS作成 (RayGen除く)
  ShaderBundle: Device(0)で作成
```

### 2.5 ShaderBytecodeHash

```
struct ShaderBytecodeHash
{
    uint64 Hash[2];  // 128bit
};

// DXBCヘッダーのオフセット+4から16バイトを直接抽出
// （組み込みチェックサム利用で完全ハッシュ計算を回避）
```

---

## 3. レンダーターゲット操作

### 3.1 MSAA Resolve

```
FD3D12CommandContext::ResolveTexture(Info):

  ┌─────────────────────────┐     ┌──────────────────────────┐
  │  Depth/Stencil           │     │  Color                    │
  │  (PF_DepthStencil)       │     │  (その他フォーマット)      │
  │                          │     │                            │
  │  シェーダーベース解決     │     │  ネイティブD3D12解決       │
  │  ResolveTextureUsingShader│     │  ResolveSubresource()     │
  │  <FResolveDepthPS>       │     │  (リソースバリア管理付き)  │
  │                          │     │                            │
  │  ピクセルシェーダーで     │     │  typeless→UNORM変換       │
  │  深度をサンプリング+描画  │     │  ConvertTypelessToUnorm() │
  └─────────────────────────┘     └──────────────────────────┘
```

**シェーダーベース解決の詳細:**

```
ResolveTextureUsingShader<TPixelShader>():
  1. 現在のビューポート保存 (StateCache)
  2. FGraphicsPipelineStateInitializer設定:
     BlendState: 無効
     RasterizerState: ソリッド、カリングなし
     DepthStencilState: 深度パスはCF_Always+書き込み有効
  3. ソーステクスチャのSRV/UAVバインディングクリア
  4. 全面解決時: クリア+ディスカード最適化
  5. DepthStencil: ClearDepthStencilView → DSVバインド
     Color: ClearRenderTargetView → RTVバインド
  6. FResolveVS + TPixelShader (GMaxRHIFeatureLevel)
  7. GFilterVertexDeclaration, PT_TriangleStrip, 2三角形描画
  8. ソースSRVクリア、ビューポート復元
```

### 3.2 サーフェスデータ読み戻し

全読み戻し操作は同じパターンに従う:

```
リードバックバッファ作成 (D3D12_HEAP_TYPE_READBACK)
    │
    ▼ CD3DX12_TEXTURE_COPY_LOCATION設定
    │
    ▼ リソースバリア: Source → CopySrc
    │
    ▼ CopyTextureRegionChecked() (GPU→リードバックコピー)
    │
    ▼ リソースバリア: Source → 元の状態
    │
    ▼ SubmitAndBlockUntilGPUIdle() (同期GPUストール)
    │
    ▼ ID3D12Resource::Map() → データ読み取り → Unmap()
```

**読み戻しバリエーション:**

| 関数 | 対象 | 出力形式 |
|------|------|---------|
| `ReadSurfaceDataNoMSAARaw()` | 非MSAAテクスチャ | 生バイト配列 |
| `ReadSurfaceDataMSAARaw()` | MSAAテクスチャ | サンプル単位インターリーブ |
| `RHIReadSurfaceData()` | 汎用2D | `FColor` / `FLinearColor` |
| `RHIReadSurfaceFloatData()` | Float16テクスチャ | `FFloat16Color` |
| `RHIRead3DSurfaceFloatData()` | 3Dテクスチャ | `FFloat16Color` |

### 3.3 MSAAサンプル単位読み戻し

```
ReadSurfaceDataMSAARaw():
  1. 非MSAA中間テクスチャ作成 (ALLOW_RENDER_TARGET)
  2. リードバックステージングバッファ作成

  3. 各サンプル (i = 0..NumSamples-1):
     a. ResolveTextureUsingShader<FResolveSingleSamplePS>(サンプルi)
        → 単一サンプルを非MSAAテクスチャに解決
     b. 非MSAA → CopySrc バリア遷移
     c. ステージングにコピー
     d. SubmitAndBlockUntilGPUIdle()
     e. Map → サンプル単位インターリーブ読み取り → Unmap

  出力レイアウト:
  [pixel0_sample0, pixel0_sample1, ..., pixel1_sample0, pixel1_sample1, ...]
```

### 3.4 3Dテクスチャ読み戻し

```
RHIRead3DSurfaceFloatData():
  サポートフォーマット:
    PF_FloatRGBA (RGBA16F) → 直接memcpy
    PF_R16F → FFloat16Colorに変換 (Alpha=1.0)
    PF_R32_FLOAT → FFloat16Colorに変換 (Alpha=1.0)

  深度スライスのストライド: DepthBytesAligned
    = Align(W * BytesPerPixel, FD3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * H
```

### 3.5 ステージングサーフェス

```
RHIMapStagingSurface(Texture, Fence, OutData, OutWidth, OutHeight, GPUIndex):
  1. Fence指定時: Wait()
  2. ID3D12Resource::Map(0, &ReadRange)  // ReadRange = {0, Width}
  3. OutWidth = RowPitch / BytesPerPixel
  4. OutHeight = テクスチャ高さ

RHIUnmapStagingSurface(Texture, GPUIndex):
  1. ID3D12Resource::Unmap(0, nullptr)
```

### 3.6 フォーマット変換ユーティリティ

```cpp
// Typelessフォーマット → UNORM変換 (ResolveSubresource用)
ConvertTypelessToUnorm(DXGI_FORMAT):
  R8G8B8A8_TYPELESS → R8G8B8A8_UNORM
  B8G8R8A8_TYPELESS → B8G8R8A8_UNORM

// Planarフォーマットのプレーン分離 (Depth-Stencil)
GetPlaneFormat(DXGI_FORMAT, PlaneSlice):
  R32G8X24_TYPELESS / R24G8_TYPELESS:
    Plane 0 → R32_TYPELESS (深度)
    Plane 1 → R8_TYPELESS  (ステンシル)
```

---

## 4. CVar一覧

| CVar | 型 | デフォルト | 説明 |
|------|-----|-----------|------|
| `D3D12.LogViewportEvents` | int32 | 0 | ビューポートイベントのログ出力（非Shipping） |
| `r.D3D12.NumViewportBuffers` | int32 | 3 | スワップチェーンバックバッファ数（読み取り専用） |
| `r.D3D12.UseAllowTearing` | int32 | 1 | DXGIフリップモードのティアリング許可（読み取り専用） |

---

## 5. DXGI APIインターフェース

### 5.1 スワップチェーン操作

| API | 用途 |
|-----|------|
| `IDXGIFactory2::CreateSwapChainForHwnd()` | スワップチェーン作成 |
| `IDXGIFactory2::MakeWindowAssociation()` | DXGIのウィンドウ変更防止 |
| `IDXGIFactory5::CheckFeatureSupport(ALLOW_TEARING)` | ティアリングサポート検出 |
| `IDXGISwapChain1::Present()` | フレーム表示 |
| `IDXGISwapChain1::ResizeBuffers()` | シングルGPUバッファリサイズ |
| `IDXGISwapChain1::GetFullscreenState()` / `SetFullscreenState()` | フルスクリーン管理 |
| `IDXGISwapChain1::GetFrameStatistics()` | フリップトラッキング |
| `IDXGISwapChain3::ResizeBuffers1()` | mGPU対応リサイズ（GPU別キュー） |
| `IDXGISwapChain3::GetCurrentBackBufferIndex()` | フリップモデルインデックス |
| `IDXGISwapChain4::CheckColorSpaceSupport()` | HDRカラースペースサポート |
| `IDXGISwapChain4::SetColorSpace1()` | カラースペース設定 |
| `IDXGIOutput6::GetDesc1()` | HDR出力ディスクリプタ照会 |

### 5.2 プレゼンテーションモデル

```
DXGI_SWAP_EFFECT_FLIP_DISCARD (排他使用)
  │
  ├── VSync有効: Present(SyncInterval, 0)
  │
  └── VSync無効 + ウィンドウ + ティアリング許可:
       Present(0, DXGI_PRESENT_ALLOW_TEARING)
       → 可変リフレッシュレート (VRR) ディスプレイ対応
```

---

## 6. 実装上の注意事項

### 6.1 DXGIバグワークアラウンド

`DXGIBackBufferLock` (static FCriticalSection) は
`MakeWindowAssociation` と バックバッファ解放の間の既知のDXGI競合条件を回避する。

### 6.2 一時的Presentエラーのリトライ

`PresentChecked()`はモード切り替え中の`E_INVALIDARG`/`DXGI_ERROR_INVALID_CALL`を
一時的エラーとして扱い、最大5回リトライする。5回連続失敗で致命的エラーとなる。

### 6.3 フルスクリーン復旧

`ConditionalResetSwapChain()`は毎フレーム`RHITick()`から呼ばれ、
`bFullscreenLost`検出時にウィンドウモードにフォールバックする。
`DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`は次ティックまで待機する。

### 6.4 MSAA読み戻しの最適化余地

ソースコード内コメント:
```
// Can be optimized by doing all subsamples into a large enough rendertarget in one pass
```
現在は各サンプルを個別にResolve→Copy→ReadbackしてGPUストールが発生する。

### 6.5 RHICreateBoundShaderState

D3D12ではレガシーBound Shader State作成はサポートされず、
`RHICreateBoundShaderState()`は`checkNoEntry()`でスタブ化されている。
PSO ベースのパイプラインバインディングが使用される。

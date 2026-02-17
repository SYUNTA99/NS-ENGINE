# Unreal Engine 5.7 RHI アーキテクチャ解説

## 目次

1. [概要](#1-概要)
2. [全体アーキテクチャ](#2-全体アーキテクチャ)
3. [モジュール初期化とプラットフォーム選択](#3-モジュール初期化とプラットフォーム選択)
4. [階層構造](#4-階層構造)
5. [コアインターフェース](#5-コアインターフェース)
6. [リソースシステム](#6-リソースシステム)
7. [コマンド実行フロー](#7-コマンド実行フロー)
8. [メモリ管理](#8-メモリ管理)
9. [同期メカニズム](#9-同期メカニズム)
10. [プラットフォーム実装一覧](#10-プラットフォーム実装一覧)
11. [新規RHI作成ガイド](#11-新規rhi作成ガイド)

---

## 1. 概要

### RHIとは

**RHI（Rendering Hardware Interface）** は、Unreal Engineのレンダリングシステムとグラフィックスハードウェアの間に位置する抽象化レイヤーです。

```
┌─────────────────────────────────────────┐
│         ゲーム / エンジンコード           │
│    （マテリアル、メッシュ、ライティング等）   │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│              Renderer                    │
│     （Deferred/Forward、シャドウ等）       │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│           RHI 抽象化レイヤー              │  ← ここ
│    （プラットフォーム非依存のAPI）          │
└─────────────────────────────────────────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐
   │ D3D12   │ │ Vulkan  │ │  Metal  │  ...
   │   RHI   │ │   RHI   │ │   RHI   │
   └─────────┘ └─────────┘ └─────────┘
        │           │           │
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐
   │DirectX12│ │ Vulkan  │ │  Metal  │
   │  Driver │ │ Driver  │ │ Driver  │
   └─────────┘ └─────────┘ └─────────┘
```

### RHIの役割

| 役割 | 説明 |
|------|------|
| **抽象化** | 異なるグラフィックスAPIの差異を吸収し、統一されたインターフェースを提供 |
| **移植性** | 同一のレンダリングコードが複数プラットフォームで動作可能 |
| **最適化** | 各プラットフォーム固有の最適化をRHI層で実装可能 |
| **拡張性** | 新しいグラフィックスAPIのサポートを追加しやすい設計 |

---

## 2. 全体アーキテクチャ

### モジュール構成

```
Engine/Source/Runtime/
├── RHI/                    # コアRHIモジュール（インターフェース定義）
│   ├── Public/             # 公開ヘッダー
│   └── Private/            # 実装
│
├── RHICore/                # 共通ユーティリティ
│
├── D3D12RHI/               # DirectX 12 実装
├── VulkanRHI/              # Vulkan 実装
├── Apple/MetalRHI/         # Metal 実装
├── Windows/D3D11RHI/       # DirectX 11 実装
└── OpenGLDrv/              # OpenGL 実装
```

### 主要ファイルの役割

| ファイル | 役割 |
|----------|------|
| `RHI.h` | RHI全体の定義、グローバル関数、ヘルパー |
| `DynamicRHI.h` | DynamicRHI / IDynamicRHIModule インターフェース定義 |
| `RHIContext.h` | IRHIComputeContext / IRHICommandContext コマンド記録インターフェース |
| `RHIResources.h` | RHIResource / RHIViewableResource / RHIView 等の基底クラス |
| `RHIDefinitions.h` | RHIResourceType, BufferUsageFlags, TextureCreateFlags 等の列挙型 |
| `RHIPipeline.h` | RHIPipeline 列挙型、RHIPipelineArray ユーティリティ |
| `RHICommandList.h` | コマンドリスト（描画コマンドのキュー） |

### RHIモジュール依存関係 (RHI.Build.cs)

```
RHI モジュールの依存関係:
  PrivateDependencyModuleNames:
    ・Core
    ・TraceLog
    ・ApplicationCore
    ・Cbor
    ・BuildSettings

  DynamicallyLoadedModuleNames (動的ロード):
    ・NullDrv              （常時）
    ・D3D11RHI             （Windowsのみ）
    ・D3D12RHI             （Windowsのみ）
    ・VulkanRHI            （Windows + Unix）
    ・OpenGLDrv            （Windows + Linux, サーバー以外）

  PublicDefinitions:
    ・WITH_MGPU = true     （Windows Desktopのみ）
```

---

## 3. モジュール初期化とプラットフォーム選択

### IDynamicRHIModule

RHIバックエンドモジュールは `IDynamicRHIModule` インターフェースを実装します。

```
┌─────────────────────────────────────────────────────────────┐
│               IDynamicRHIModule : IModuleInterface           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  virtual bool IsSupported() = 0;                             │
│    → 現在のシステムでこのRHIが使用可能か判定                   │
│                                                              │
│  virtual bool IsSupported(RHIFeatureLevel::Type) { ... }    │
│    → 指定フィーチャーレベルでのサポート判定                    │
│                                                              │
│  virtual DynamicRHI* CreateRHI(                             │
│      RHIFeatureLevel::Type RequestedFeatureLevel            │
│          = RHIFeatureLevel::Num) = 0;                       │
│    → DynamicRHI インスタンスを生成                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### RHI初期化フロー

```
    PlatformCreateDynamicRHI()
           │
           ▼
    IDynamicRHIModule::IsSupported()    ← 各RHIモジュールを問い合わせ
           │
           ▼
    IDynamicRHIModule::CreateRHI()      ← FDynamicRHIインスタンス作成
           │
           ▼
    GDynamicRHI = 作成されたインスタンス  ← グローバル変数にセット
           │
           ▼
    DynamicRHI::Init()                 ← RHI初期化（純粋仮想）
           │
           ▼
    DynamicRHI::PostInit()             ← 初期化後処理（仮想、レンダースレッド起動前）
           │
           ▼
    DynamicRHI::InitPixelFormatInfo()  ← ピクセルフォーマット情報の初期化
```

### プラットフォーム選択ロジック

RHIモジュールは動的にロードされ（`DynamicallyLoadedModuleNames`）、プラットフォームごとに利用可能なRHIが異なります。

| プラットフォーム | 利用可能なRHI |
|-----------------|--------------|
| Windows Desktop | D3D12RHI, D3D11RHI, VulkanRHI, OpenGLDrv, NullDrv |
| Linux           | VulkanRHI, OpenGLDrv, NullDrv |
| Android         | VulkanRHI (Build.csではなくUEBuildAndroid.csで追加), NullDrv |
| iOS / macOS     | MetalRHI (プラットフォーム固有Build.csで設定), NullDrv |
| Xbox            | D3D12RHI (プラットフォーム固有), NullDrv |
| Dedicated Server | NullDrv のみ |

---

## 4. 階層構造

### RHI -> Adapter -> Device -> Queue

RHIは以下の階層構造で物理ハードウェアを抽象化しています。

```
┌─────────────────────────────────────────────────────────────┐
│                        DynamicRHI                          │
│                    （RHI最上位オブジェクト）                   │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                    Adapter (GPU)                     │   │
│  │              （1つの物理GPU/GPU群を表現）              │   │
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
│  │                 Adapter (別のGPU)                    │   │
│  │            （例：内蔵GPU + 外付けGPU構成時）           │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 各階層の説明

#### DynamicRHI（RHI本体）

- **役割**: RHI全体を管理する最上位オブジェクト
- **責務**:
  - アダプターの列挙と選択
  - リソース作成の統括
  - フレーム管理
- **グローバル変数**: `GDynamicRHI`

#### Adapter（アダプター）

- **役割**: 1つの物理GPU（またはSLI/CrossFireでリンクされたGPU群）を表現
- **責務**:
  - GPUの機能・能力情報の保持
  - 共有リソース（パイプラインステートキャッシュ等）の管理
  - 複数デバイスノードの管理
- **具体例**: NVIDIA RTX 4090、AMD RX 7900 XT等

#### Device（デバイス）

- **役割**: Adapter内の1つのGPUノードを表現
- **責務**:
  - コマンドキューの所有
  - メモリアロケーターの管理
  - デスクリプタヒープの管理
- **備考**: 通常は1 Adapter = 1 Device。SLI時は1 Adapter = 複数 Device

#### Queue（キュー）

- **役割**: GPUへのコマンド送信パイプライン
- **種類**:
  - **Direct (Graphics)**: 描画 + コンピュート
  - **Async Compute**: 非同期コンピュート専用
  - **Copy**: データ転送専用
- **責務**: コマンドリストの実行、フェンス管理

### 構成パターン例

| 構成 | Adapter数 | Device数/Adapter | 説明 |
|------|-----------|------------------|------|
| シングルGPU | 1 | 1 | 最も一般的 |
| SLI/CrossFire | 1 | 2+ | 同種GPUのリンク |
| 内蔵+外付け | 2 | 各1 | ノートPC等 |
| マルチGPU独立 | 複数 | 各1 | ワークステーション |

---

## 5. コアインターフェース

### DynamicRHI

全プラットフォームRHIが実装する**メインインターフェース**です。

```
┌──────────────────────────────────────────────────────────────────┐
│                          DynamicRHI                              │
├──────────────────────────────────────────────────────────────────┤
│ 【初期化/終了】                                                   │
│   Init()                   - RHI初期化（純粋仮想）                │
│   PostInit()               - 初期化後処理（レンダースレッド起動前）│
│   Shutdown()               - シャットダウン（純粋仮想）           │
│   GetName() -> const TCHAR*- RHI名取得（"D3D12", "Vulkan"等）    │
│   GetInterfaceType()       - RHIInterfaceType を返す             │
├──────────────────────────────────────────────────────────────────┤
│ 【ステート作成】（全て純粋仮想、スレッドセーフ）                    │
│   RHICreateSamplerState(const SamplerStateInitializerRHI&)      │
│   RHICreateRasterizerState(const RasterizerStateInitializerRHI&)│
│   RHICreateDepthStencilState(const DepthStencilStateInitializerRHI&)│
│   RHICreateBlendState(const BlendStateInitializerRHI&)          │
│   RHICreateVertexDeclaration(const VertexDeclarationElementList&)│
├──────────────────────────────────────────────────────────────────┤
│ 【シェーダー作成】（ArrayView<const uint8> Code, const SHAHash&）│
│   RHICreateVertexShader()          - 頂点シェーダー（純粋仮想）   │
│   RHICreatePixelShader()           - ピクセルシェーダー（純粋仮想）│
│   RHICreateComputeShader()         - コンピュートシェーダー       │
│   RHICreateGeometryShader()        - ジオメトリシェーダー         │
│   RHICreateMeshShader()            - メッシュシェーダー           │
│   RHICreateAmplificationShader()   - アンプリフィケーションシェーダー │
│   RHICreateWorkGraphShader()       - ワークグラフシェーダー       │
├──────────────────────────────────────────────────────────────────┤
│ 【パイプライン作成】                                              │
│   RHICreateGraphicsPipelineState(                                │
│       const GraphicsPipelineStateInitializer&)                  │
│   RHICreateComputePipelineState(                                 │
│       const ComputePipelineStateInitializer&)                   │
│   RHICreateWorkGraphPipelineState(                               │
│       const WorkGraphPipelineStateInitializer&)                 │
│   RHICreateBoundShaderState(RHIVertexDeclaration*,              │
│       RHIVertexShader*, RHIPixelShader*, RHIGeometryShader*)  │
├──────────────────────────────────────────────────────────────────┤
│ 【リソース作成】                                                  │
│   RHICreateBufferInitializer(RHICommandListBase&,               │
│       const RHIBufferCreateDesc&) -> RHIBufferInitializer      │
│   RHICreateTextureInitializer(RHICommandListBase&,              │
│       const RHITextureCreateDesc&) -> RHITextureInitializer    │
│   RHICreateUniformBuffer(const void*, const RHIUniformBufferLayout*,│
│       UniformBufferUsage, UniformBufferValidation)             │
│   RHIUpdateUniformBuffer(RHICommandListBase&,                   │
│       RHIUniformBuffer*, const void*)                           │
│   RHICreateShaderResourceView(RHICommandListBase&,              │
│       RHIViewableResource*, RHIViewDesc const&)                │
│   RHICreateUnorderedAccessView(RHICommandListBase&,             │
│       RHIViewableResource*, RHIViewDesc const&)                │
│   RHIReplaceResources(RHICommandListBase&,                      │
│       Array<RHIResourceReplaceInfo>&&)                         │
├──────────────────────────────────────────────────────────────────┤
│ 【Viewport】                                                     │
│   RHICreateViewport(void* WindowHandle, uint32 SizeX,            │
│       uint32 SizeY, bool bIsFullscreen, PixelFormat)            │
│   RHIResizeViewport(RHIViewport*, uint32, uint32, bool)         │
│   RHIGetViewportBackBuffer(RHIViewport*) -> TextureRHIRef      │
│   RHIAdvanceFrameForGetViewportBackBuffer(RHIViewport*, bool)   │
├──────────────────────────────────────────────────────────────────┤
│ 【コマンド実行】                                                  │
│   RHIGetDefaultContext() -> IRHICommandContext*                   │
│   RHIGetCommandContext(RHIPipeline, RHIGPUMask)                │
│       -> IRHIComputeContext*                                     │
│   RHIFinalizeContext(RHIFinalizeContextArgs&&,                  │
│       RHIPipelineArray<IRHIPlatformCommandList*>&)              │
│   RHISubmitCommandLists(RHISubmitCommandListsArgs&&)            │
│   RHIProcessDeleteQueue()  - リソース削除キュー処理              │
├──────────────────────────────────────────────────────────────────┤
│ 【フレーム管理】                                                  │
│   RHIEndFrame(const RHIEndFrameArgs& Args)                      │
│       RHIEndFrameArgs:                                          │
│         uint32 FrameNumber;                                      │
│         [WITH_RHI_BREADCRUMBS] GPUBreadcrumbs;                   │
│         [STATS] Optional<int64> StatsFrame;                     │
│   RHITick(float DeltaTime)                                       │
│   RHIBlockUntilGPUIdle()                                         │
│   RHIFlushResources()                                            │
├──────────────────────────────────────────────────────────────────┤
│ 【テクスチャ操作】                                                │
│   RHICalcTexturePlatformSize(RHITextureDesc const&, uint32)     │
│       -> RHICalcTextureSizeResult { uint64 Size; uint32 Align; }│
│   RHIGetTextureMemoryStats(TextureMemoryStats&)                 │
│   RHIAsyncCreateTexture2D(...)                                   │
│   RHIAsyncReallocateTexture2D(...)                               │
│   RHILockTexture(RHICommandListImmediate&,                      │
│       const RHILockTextureArgs&) -> RHILockTextureResult       │
│   RHIUnlockTexture(RHICommandListImmediate&,                    │
│       const RHILockTextureArgs&)                                │
│   RHIUpdateTexture2D(RHICommandListBase&, RHITexture*, ...)    │
│   RHIUpdateTexture3D(RHICommandListBase&, RHITexture*, ...)    │
│   RHIComputeMemorySize(RHITexture*) -> uint32                   │
├──────────────────────────────────────────────────────────────────┤
│ 【読み取り】                                                      │
│   RHIReadSurfaceData(RHITexture*, IntRect, Array<Color>&,    │
│       ReadSurfaceDataFlags)                                     │
│   RHIMapStagingSurface(...) / RHIUnmapStagingSurface(...)        │
│   RHIReadSurfaceFloatData(...)                                   │
│   RHIRead3DSurfaceFloatData(...)                                 │
├──────────────────────────────────────────────────────────────────┤
│ 【同期・クエリ】                                                  │
│   RHICreateGPUFence(const Name&) -> GPUFenceRHIRef             │
│   RHICreateTransition(...) / RHIReleaseTransition(...)           │
│   RHICreateRenderQuery(RenderQueryType) -> RenderQueryRHIRef   │
│   RHIGetRenderQueryResult(RHIRenderQuery*, uint64&, bool, uint32)│
├──────────────────────────────────────────────────────────────────┤
│ 【ネイティブアクセス】                                            │
│   RHIGetNativeDevice() -> void*                                  │
│   RHIGetNativeInstance() -> void*                                │
│   RHIGetAvailableResolutions(ScreenResolutionArray&, bool)      │
│   RHIGetSupportedResolution(uint32& Width, uint32& Height)       │
└──────────────────────────────────────────────────────────────────┘
```

### RHIFinalizeContextArgs / RHISubmitCommandListsArgs

```
struct RHIFinalizeContextArgs
{
    Array<IRHIComputeContext*> Contexts;    // ファイナライズ対象のコンテキスト群
    IRHIUploadContext* UploadContext;        // アップロードコンテキスト
};

struct RHISubmitCommandListsArgs
{
    Array<IRHIPlatformCommandList*> CommandLists;  // 送信するコマンドリスト群
    IRHIUploadContext* UploadContext;                // アップロードコンテキスト
};
```

### RHIContextArray / IRHIUploadContext

**RHIContextArray** (`RHIResources.h`) は `RHIPipelineArray<IRHIComputeContext*>` を継承した
型エイリアスで、並列レンダリング時に複数パイプラインのコンテキストをまとめて保持します。
`RHIParallelRenderPassInfo` のメンバとして使用され、`RHIBeginParallelRenderPass()` に渡されます。

**IRHIUploadContext** (`RHIContext.h`) はデータアップロード専用の軽量コンテキストです。
現在は空のインターフェースとして定義されており、`RHIFinalizeContextArgs` と
`RHISubmitCommandListsArgs` の両方に含まれます。
`DynamicRHI::RHIGetUploadContext()` で取得（デフォルト: nullptr）。

### IRHIComputeContext / IRHICommandContext（コマンドコンテキスト）

GPUコマンドを記録するインターフェースです。

```
┌──────────────────────────────────────────────────────────────────┐
│                     IRHIComputeContext                            │
│                   （コンピュート共通機能）                          │
├──────────────────────────────────────────────────────────────────┤
│ 【パイプライン情報】                                              │
│   GetPipeline() -> RHIPipeline  (デフォルト: AsyncCompute)      │
├──────────────────────────────────────────────────────────────────┤
│ 【パイプライン設定】                                              │
│   RHISetComputePipelineState(RHIComputePipelineState*)          │
├──────────────────────────────────────────────────────────────────┤
│ 【ディスパッチ】                                                  │
│   RHIDispatchComputeShader(uint32 X, uint32 Y, uint32 Z)        │
│   RHIDispatchIndirectComputeShader(RHIBuffer*, uint32 Offset)   │
├──────────────────────────────────────────────────────────────────┤
│ 【レイトレーシング】                                              │
│   RHIRayTraceDispatch(RHIRayTracingPipelineState*,              │
│       RHIRayTracingShader*, RHIShaderBindingTable*,            │
│       const RayTracingShaderBindings&, uint32 W, uint32 H)     │
│   RHIRayTraceDispatchIndirect(...)                               │
├──────────────────────────────────────────────────────────────────┤
│ 【リソース遷移】                                                  │
│   RHIBeginTransitions(ArrayView<const RHITransition*>)         │
│   RHIEndTransitions(ArrayView<const RHITransition*>)           │
├──────────────────────────────────────────────────────────────────┤
│ 【UAVクリア】                                                    │
│   RHIClearUAVFloat(RHIUnorderedAccessView*, const Vector4f&)   │
│   RHIClearUAVUint(RHIUnorderedAccessView*, const UintVector4&) │
├──────────────────────────────────────────────────────────────────┤
│ 【UAVオーバーラップ制御】                                         │
│   RHIBeginUAVOverlap() / RHIEndUAVOverlap()                     │
│   RHIBeginUAVOverlap(ConstArrayView<RHIUnorderedAccessView*>) │
│   RHIEndUAVOverlap(ConstArrayView<RHIUnorderedAccessView*>)   │
├──────────────────────────────────────────────────────────────────┤
│ 【シェーダーパラメータ】                                          │
│   RHISetShaderParameters(RHIComputeShader*,                     │
│       ConstArrayView<uint8> InParametersData,                   │
│       ConstArrayView<RHIShaderParameter> InParameters,         │
│       ConstArrayView<RHIShaderParameterResource> InResources,  │
│       ConstArrayView<RHIShaderParameterResource> InBindless)   │
│   RHISetStaticUniformBuffers(const UniformBufferStaticBindings&)│
│   RHISetStaticUniformBuffer(UniformBufferStaticSlot,            │
│       RHIUniformBuffer*)                                        │
├──────────────────────────────────────────────────────────────────┤
│ 【GPU間転送 (WITH_MGPU)】                                        │
│   RHITransferResources(ConstArrayView<TransferResourceParams>) │
│   RHISetGPUMask(RHIGPUMask) / RHIGetGPUMask()                  │
├──────────────────────────────────────────────────────────────────┤
│ 【StagingBuffer / Fence】                                        │
│   RHICopyToStagingBuffer(RHIBuffer*, RHIStagingBuffer*,        │
│       uint32 Offset, uint32 NumBytes)                            │
│   RHIWriteGPUFence(RHIGPUFence*)                                │
├──────────────────────────────────────────────────────────────────┤
│ 【レイトレーシングAS構築】                                        │
│   RHIBuildAccelerationStructures(                                │
│       ConstArrayView<RayTracingGeometryBuildParams>,           │
│       const RHIBufferRange&)                                    │
│   RHIBuildAccelerationStructures(                                │
│       ConstArrayView<RayTracingSceneBuildParams>)              │
│   RHIBindAccelerationStructureMemory(RHIRayTracingScene*,       │
│       RHIBuffer*, uint32 BufferOffset)                          │
└──────────────────────────────────────────────────────────────────┘
                          │
                          │ 継承
                          ▼
┌──────────────────────────────────────────────────────────────────┐
│                     IRHICommandContext                            │
│                 （グラフィックス固有機能を追加）                    │
├──────────────────────────────────────────────────────────────────┤
│ 【パイプライン情報】                                              │
│   GetPipeline() -> RHIPipeline::Graphics  (オーバーライド)      │
├──────────────────────────────────────────────────────────────────┤
│ 【パイプライン設定】                                              │
│   RHISetGraphicsPipelineState(RHIGraphicsPipelineState*,        │
│       uint32 StencilRef, bool bApplyAdditionalState)             │
├──────────────────────────────────────────────────────────────────┤
│ 【レンダーパス】                                                  │
│   RHIBeginRenderPass(const RHIRenderPassInfo&, const TCHAR*)    │
│   RHIEndRenderPass()                                             │
│   RHINextSubpass()                                               │
│   RHIEndDrawingViewport(RHIViewport*, bool bPresent,            │
│       bool bLockToVsync)                                         │
├──────────────────────────────────────────────────────────────────┤
│ 【ビューポート/シザー】                                           │
│   RHISetViewport(float MinX, float MinY, float MinZ,             │
│       float MaxX, float MaxY, float MaxZ)                        │
│   RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY,     │
│       uint32 MaxX, uint32 MaxY)                                  │
│   RHISetMultipleViewports(uint32 Count, const ViewportBounds*)  │
├──────────────────────────────────────────────────────────────────┤
│ 【頂点入力】                                                     │
│   RHISetStreamSource(uint32 StreamIndex, RHIBuffer*, uint32 Offset)│
├──────────────────────────────────────────────────────────────────┤
│ 【描画コマンド】                                                  │
│   RHIDrawPrimitive(uint32 BaseVertexIndex, uint32 NumPrimitives, │
│       uint32 NumInstances)                                       │
│   RHIDrawIndexedPrimitive(RHIBuffer* IndexBuffer,               │
│       int32 BaseVertexIndex, uint32 FirstInstance,               │
│       uint32 NumVertices, uint32 StartIndex,                     │
│       uint32 NumPrimitives, uint32 NumInstances)                 │
│   RHIDrawPrimitiveIndirect(RHIBuffer* ArgumentBuffer,           │
│       uint32 ArgumentOffset)                                     │
│   RHIDrawIndexedIndirect(RHIBuffer* IndexBuffer,                │
│       RHIBuffer* ArgumentsBuffer, int32 DrawArgumentsIndex,     │
│       uint32 NumInstances)                                       │
│   RHIDrawIndexedPrimitiveIndirect(RHIBuffer* IndexBuffer,       │
│       RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset)         │
│   RHIMultiDrawIndexedPrimitiveIndirect(RHIBuffer* IndexBuffer,  │
│       RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset,         │
│       RHIBuffer* CountBuffer, uint32 CountBuffeOffset,          │
│       uint32 MaxDrawArguments)                                   │
├──────────────────────────────────────────────────────────────────┤
│ 【メッシュシェーダーディスパッチ】                                 │
│   RHIDispatchMeshShader(uint32 X, uint32 Y, uint32 Z)            │
│   RHIDispatchIndirectMeshShader(RHIBuffer*, uint32 Offset)      │
├──────────────────────────────────────────────────────────────────┤
│ 【コピー操作】                                                   │
│   RHICopyTexture(RHITexture* Src, RHITexture* Dst,             │
│       const RHICopyTextureInfo&)                                │
│   RHICopyBufferRegion(RHIBuffer* Dst, uint64 DstOffset,         │
│       RHIBuffer* Src, uint64 SrcOffset, uint64 NumBytes)        │
├──────────────────────────────────────────────────────────────────┤
│ 【追加機能】                                                     │
│   RHISetDepthBounds(float MinDepth, float MaxDepth)              │
│   RHISetShadingRate(VRSShadingRate, VRSRateCombiner)           │
│   RHISetStencilRef(uint32) / RHISetBlendFactor(LinearColor&)    │
│   RHIBeginRenderQuery(RHIRenderQuery*)                          │
│   RHIEndRenderQuery(RHIRenderQuery*)                            │
│   RHIResummarizeHTile(RHITexture*)                              │
└──────────────────────────────────────────────────────────────────┘
                          │
                          │ 継承（レガシー用）
                          ▼
┌──────────────────────────────────────────────────────────────────┐
│              IRHICommandContextPSOFallback                        │
│          （PSO非対応プラットフォーム向けフォールバック）             │
├──────────────────────────────────────────────────────────────────┤
│   RHISetBoundShaderState(RHIBoundShaderState*)                  │
│   ...                                                            │
└──────────────────────────────────────────────────────────────────┘
```

### IRHIPlatformCommandList

ファイナライズ済みコマンドリストの基底クラスです。プラットフォームRHIが実装を提供します。

```
class IRHIPlatformCommandList
{
protected:
    IRHIPlatformCommandList() = default;    // 派生型のみ使用可能
    // コピー不可、ムーブ可能

public:
#if WITH_RHI_BREADCRUMBS
    RHIBreadcrumbAllocatorArray BreadcrumbAllocators;
    RHIBreadcrumbRange BreadcrumbRange;
#endif
};
```

---

## 6. リソースシステム

### リソース階層（完全版）

```
                              RHIResource
                         （全リソースの基底クラス）
                         RHIResourceType で種別管理
                         参照カウント（アトミック）
                                  │
        ┌─────────┬───────────────┼─────────────┬──────────────┐
        │         │               │             │              │
        ▼         ▼               ▼             ▼              ▼
 RHIViewable  RHIView     ステート系      シェーダー系     その他
  Resource   （ビュー基底）   (下記参照)     (下記参照)     (下記参照)
        │         │
  ┌─────┴────┐    ├── RHIShaderResourceView (SRV)
  │          │    └── RHIUnorderedAccessView (UAV)
  ▼          ▼
RHITexture RHIBuffer
```

### RHIResourceType（リソース種別一覧）

ソースコード `RHIDefinitions.h` の `RHIResourceType` に基づく全種別:

```
┌──────────────────────────────────────────────────────────────────┐
│                     RHIResourceType 一覧                        │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  【ステート】                                                    │
│    RRT_SamplerState          → RHISamplerState                  │
│    RRT_RasterizerState       → RHIRasterizerState               │
│    RRT_DepthStencilState     → RHIDepthStencilState             │
│    RRT_BlendState            → RHIBlendState                    │
│    RRT_VertexDeclaration     → RHIVertexDeclaration             │
│                                                                  │
│  【シェーダー】                                                  │
│    RRT_VertexShader          → RHIVertexShader                  │
│    RRT_MeshShader            → RHIMeshShader                    │
│    RRT_AmplificationShader   → RHIAmplificationShader           │
│    RRT_PixelShader           → RHIPixelShader                   │
│    RRT_GeometryShader        → RHIGeometryShader                │
│    RRT_RayTracingShader      → RHIRayTracingShader              │
│    RRT_ComputeShader         → RHIComputeShader                 │
│                                                                  │
│  【パイプラインステート】                                        │
│    RRT_GraphicsPipelineState → RHIGraphicsPipelineState         │
│    RRT_ComputePipelineState  → RHIComputePipelineState          │
│    RRT_RayTracingPipelineState → RHIRayTracingPipelineState     │
│    RRT_BoundShaderState      → RHIBoundShaderState              │
│                                                                  │
│  【バッファ・ユニフォーム】                                       │
│    RRT_UniformBufferLayout                                       │
│    RRT_UniformBuffer         → RHIUniformBuffer                 │
│    RRT_Buffer                → RHIBuffer                        │
│                                                                  │
│  【テクスチャ】                                                  │
│    RRT_Texture               → RHITexture                       │
│    RRT_TextureReference      → RHITextureReference              │
│    (RRT_Texture2D, RRT_Texture2DArray, RRT_Texture3D,            │
│     RRT_TextureCube は 5.7 で deprecated)                        │
│                                                                  │
│  【ビュー】                                                      │
│    RRT_UnorderedAccessView   → RHIUnorderedAccessView           │
│    RRT_ShaderResourceView    → RHIShaderResourceView            │
│                                                                  │
│  【クエリ・フェンス】                                            │
│    RRT_TimestampCalibrationQuery                                 │
│    RRT_GPUFence              → RHIGPUFence                      │
│    RRT_RenderQuery           → RHIRenderQuery                   │
│    RRT_RenderQueryPool       → RHIRenderQueryPool               │
│                                                                  │
│  【レイトレーシング】                                            │
│    RRT_RayTracingAccelerationStructure                            │
│        → RHIRayTracingAccelerationStructure                     │
│            → RHIRayTracingGeometry                              │
│            → RHIRayTracingScene                                 │
│    RRT_RayTracingShaderBindingTable → RHIShaderBindingTable     │
│                                                                  │
│  【ワークグラフ】                                                │
│    RRT_WorkGraphShader       → RHIWorkGraphShader               │
│    RRT_WorkGraphPipelineState → RHIWorkGraphPipelineState       │
│                                                                  │
│  【その他】                                                      │
│    RRT_None                  → 未割り当て/無効値                 │
│    RRT_Viewport              → RHIViewport                      │
│    RRT_StagingBuffer         → RHIStagingBuffer                 │
│    RRT_CustomPresent                                             │
│    RRT_ShaderLibrary                                             │
│    RRT_ShaderBundle          → RHIShaderBundle                  │
│    RRT_StreamSourceSlot      → ストリームソーススロット           │
│    RRT_ResourceCollection    → RHIResourceCollection            │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### シェーダー継承階層

```
                    RHIResource
                        │
                    RHIShaderData  ← シェーダーリソーステーブル等
                        │
                    RHIShader     ← RHIResource + RHIShaderData の多重継承
                   (RHI共通基底)
                        │
           ┌────────────┼──────────────────┬──────────────────┐
           │            │                  │                  │
    RHIGraphicsShader  RHIComputeShader  RHIRayTracingShader  RHIWorkGraphShader
           │                               │
     ┌─────┼──────┬───────────┬────────┐   ├── RHIRayGenShader
     │     │      │           │        │   ├── RHIRayMissShader
     │     │      │           │        │   ├── RHIRayCallableShader
     │     │      │           │        │   └── RHIRayHitGroupShader
     │     │      │           │        │
  RHIVertex  RHIPixel  RHIMesh  RHIGeometry  RHIAmplification
   Shader     Shader     Shader     Shader        Shader
```

※ RHIGraphicsShader の5つの子クラス（RHIVertexShader, RHIPixelShader,
  RHIMeshShader, RHIGeometryShader, RHIAmplificationShader）は全て
  RHIGraphicsShader の**直接の子**（兄弟関係）である
※ RHIWorkGraphShader は RHIShader を直接継承（RHIGraphicsShader ではない）
  └─ RHIWorkGraphRootShader, RHIWorkGraphComputeNodeShader が派生

### RHIView（ビュー基底クラス）

UE5.7では `RHIView` がSRVとUAVの共通基底クラスです。

```
┌──────────────────────────────────────────────────────────────────┐
│                         RHIView                                 │
│                    （RHIResource を継承）                         │
├──────────────────────────────────────────────────────────────────┤
│ コンストラクタ:                                                   │
│   RHIView(RHIResourceType, RHIViewableResource*, RHIViewDesc)│
│                                                                  │
│ 【メソッド】                                                     │
│   GetResource() -> RHIViewableResource*                         │
│   GetBuffer()   -> RHIBuffer*     ← ViewDescがバッファの時      │
│   GetTexture()  -> RHITexture*    ← ViewDescがテクスチャの時    │
│   IsBuffer()  -> bool                                            │
│   IsTexture() -> bool                                            │
│   GetDesc()   -> const RHIViewDesc&                             │
│   GetBindlessHandle() -> RHIDescriptorHandle                    │
│                                                                  │
│ 【メンバ】                                                       │
│   RefCountPtr<RHIViewableResource> Resource;  (private)        │
│   RHIViewDesc const ViewDesc;                  (protected)      │
│                                                                  │
│ 【RHIViewDesc::ViewType】                                      │
│   BufferSRV   / BufferUAV                                        │
│   TextureSRV  / TextureUAV                                       │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  派生クラス:                                                     │
│    RHIShaderResourceView (SRV)  ← RRT_ShaderResourceView       │
│    RHIUnorderedAccessView (UAV) ← RRT_UnorderedAccessView       │
│                                                                  │
│  ※RTV/DSVはRHIクラスではなく、RHIRenderTargetView /             │
│   RHIDepthRenderTargetView として構造体で表現される               │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### リソース種別

#### テクスチャ (RHITexture)

```
┌─────────────────────────────────────────────────────────────┐
│                       RHITexture                           │
│              （RHIViewableResource を継承）                  │
├─────────────────────────────────────────────────────────────┤
│ 【種類】（RHITextureDesc::Dimension で区別）               │
│   ・Texture2D       - 2Dテクスチャ                          │
│   ・Texture2DArray  - 2Dテクスチャ配列                       │
│   ・Texture3D       - 3Dテクスチャ（ボリューム）              │
│   ・TextureCube     - キューブマップ                         │
│   ・TextureCubeArray- キューブマップ配列                     │
├─────────────────────────────────────────────────────────────┤
│ 【プロパティ】（RHITextureDesc 経由）                       │
│   ・Extent         - サイズ (Width, Height, Depth)          │
│   ・Format         - ピクセルフォーマット (PixelFormat)     │
│   ・NumMips        - ミップマップ数                          │
│   ・NumSamples     - MSAAサンプル数                         │
│   ・Flags          - 作成フラグ (TextureCreateFlags)       │
├─────────────────────────────────────────────────────────────┤
│ 【主要メソッド】                                            │
│   GetDesc() -> const RHITextureDesc&                       │
│   GetNativeResource() -> void*                              │
│   GetNativeShaderResourceView() -> void*                    │
│   GetTextureReference() -> RHITextureReference*            │
│   GetDefaultBindlessHandle() -> RHIDescriptorHandle        │
├─────────────────────────────────────────────────────────────┤
│ 【関連ビュー】（RHIView 経由で作成）                       │
│   ・RHIShaderResourceView (SRV)  - シェーダー読み取り用     │
│   ・RHIUnorderedAccessView (UAV) - シェーダー読み書き用     │
│   ・RHIRenderTargetView (RTV)    - レンダーターゲット用     │
│     （構造体、FRHIResource非継承）                            │
│   ・RHIDepthRenderTargetView (DSV) - 深度ステンシル用       │
│     （構造体、FRHIResource非継承）                            │
└─────────────────────────────────────────────────────────────┘
```

#### バッファ (RHIBuffer)

```
┌──────────────────────────────────────────────────────────────────┐
│                         RHIBuffer                                │
│                （RHIViewableResource を継承）                     │
├──────────────────────────────────────────────────────────────────┤
│ 【用途フラグ (BufferUsageFlags)】ビットフラグで組み合わせ使用    │
│                                                                  │
│   ライフタイム制御:                                               │
│     Static              (1 << 0)  - 一度だけ書き込み             │
│     Dynamic             (1 << 1)  - 時折更新、GPU読/CPU書         │
│     Volatile            (1 << 2)  - 毎フレーム更新必須           │
│                                                                  │
│   アクセス制御:                                                   │
│     UnorderedAccess     (1 << 3)  - UAVアクセス可能              │
│     ByteAddressBuffer   (1 << 4)  - バイトアドレスバッファ       │
│     SourceCopy          (1 << 5)  - GPUコピー元                  │
│     DrawIndirect        (1 << 7)  - 間接描画/ディスパッチ引数     │
│     ShaderResource      (1 << 8)  - SRVアクセス可能              │
│     KeepCPUAccessible   (1 << 9)  - CPU直接アクセス可能          │
│     FastVRAM            (1 << 10) - 高速VRAM配置ヒント           │
│     Shared              (1 << 12) - 外部RHI/プロセス共有         │
│                                                                  │
│   バッファ種別:                                                   │
│     VertexBuffer        (1 << 14) - 頂点バッファ                 │
│     IndexBuffer         (1 << 15) - インデックスバッファ          │
│     StructuredBuffer    (1 << 16) - 構造化バッファ               │
│     UniformBuffer       (1 << 21) - ユニフォームバッファ          │
│                                                                  │
│   レイトレーシング:                                               │
│     AccelerationStructure (1<<13) - RT用AS                       │
│     RayTracingScratch   (1<<19|UnorderedAccess)- RTスクラッチ    │
│                                                                  │
│   マルチGPU:                                                     │
│     MultiGPUAllocate    (1 << 17) - GPU毎に独立メモリ確保        │
│     MultiGPUGraphIgnore (1 << 18) - GPU間転送スキップ            │
│                                                                  │
│   その他:                                                        │
│     NNE                 (1 << 11) - DirectML用                   │
│     NullResource        (1 << 20) - ストリーミング用プレースホルダ│
│     ReservedResource    (1 << 22) - Reserved/Sparse/Virtual      │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│ 【プロパティ】（RHIBufferDesc 経由）                             │
│   GetSize()    -> uint32  - バッファサイズ（バイト）              │
│   GetStride()  -> uint32  - 要素ストライド                       │
│   GetUsage()   -> BufferUsageFlags - 使用フラグ                 │
│   GetDesc()    -> const RHIBufferDesc&                          │
└──────────────────────────────────────────────────────────────────┘
```

### ビュー（View）システム

ビューは、リソースの特定の「見方」を定義します。

```
┌──────────────────────────────────────────────────────────────────────────┐
│                           リソースとビューの関係                           │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌─────────────┐                                                        │
│   │  Texture    │───┬──▶ SRV (シェーダーで読み取り)                       │
│   │  (2048x2048 │   │      └─ Mip 0-4, Array 0-5                        │
│   │   8 Mips    │   │                                                    │
│   │   6 Array)  │   ├──▶ UAV (コンピュートで読み書き)                     │
│   │             │   │      └─ Mip 0 only                                │
│   │             │   │                                                    │
│   │             │   ├──▶ RTV (レンダーターゲットとして)                    │
│   │             │   │      └─ Mip 0, Array 0                            │
│   │             │   │                                                    │
│   │             │   └──▶ RTV (別スライスへ)                              │
│   └─────────────┘          └─ Mip 0, Array 1                            │
│                                                                          │
│   ※1つのリソースから複数のビューを作成可能                                │
│   ※ビューごとに見える範囲（Mip、Array Slice等）を指定                     │
│                                                                          │
│   RHIView (RHIResource 継承)                                           │
│     ├─ RHIShaderResourceView    ← DynamicRHI::RHICreateShaderResourceView()  │
│     └─ RHIUnorderedAccessView   ← DynamicRHI::RHICreateUnorderedAccessView() │
│                                                                          │
│   RHIRenderTargetView          ← 構造体（FRHIResource非継承）           │
│   RHIDepthRenderTargetView     ← 構造体（FRHIResource非継承）           │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

| ビュー種別 | RHIクラス | 用途 |
|-----------|-----------|------|
| ShaderResourceView (SRV) | RHIShaderResourceView (FRHIView継承) | シェーダーからの読み取り専用アクセス |
| UnorderedAccessView (UAV) | RHIUnorderedAccessView (FRHIView継承) | シェーダーからの読み書きアクセス |
| RenderTargetView (RTV) | RHIRenderTargetView (構造体) | レンダーターゲットとしての書き込み |
| DepthStencilView (DSV) | RHIDepthRenderTargetView (構造体) | 深度/ステンシルバッファとしての使用 |

---

## 7. コマンド実行フロー

### 基本フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        コマンド実行の流れ                                │
└─────────────────────────────────────────────────────────────────────────┘

    ゲームスレッド              レンダースレッド                 RHIスレッド
         │                          │                              │
         │   ┌──────────────────────┤                              │
         │   │ 1. コンテキスト取得    │                              │
         │   │    RHIGetCommandContext(RHIPipeline, RHIGPUMask)  │
         │   └──────────────────────┤                              │
         │                          │                              │
         │   ┌──────────────────────┤                              │
         │   │ 2. コマンド記録        │                              │
         │   │    RHISetGraphicsPipelineState()                    │
         │   │    RHISetShaderParameters()                         │
         │   │    RHIDrawIndexedPrimitive()                        │
         │   │    ...                │                              │
         │   └──────────────────────┤                              │
         │                          │                              │
         │   ┌──────────────────────┤                              │
         │   │ 3. コンテキストファイナライズ                        │
         │   │    RHIFinalizeContext(RHIFinalizeContextArgs&&,    │
         │   │        RHIPipelineArray<IRHIPlatformCommandList*>&)│
         │   │    → IRHIPlatformCommandList を出力                  │
         │   └──────────────────────┤                              │
         │                          │                              │
         │                          │      ┌───────────────────────┤
         │                          │      │ 4. GPUへ送信           │
         │                          │      │    RHISubmitCommandLists(
         │                          │      │      RHISubmitCommandListsArgs&&)
         │                          │      │    → ドライバへ送信     │
         │                          │      └───────────────────────┤
         │                          │                              │
         ▼                          ▼                              ▼
```

### パイプライン種別

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          RHIPipeline                                    │
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
│   IRHIComputeContext::GetPipeline()                                     │
│     Graphics: RHIPipeline::Graphics                                    │
│     Compute:  RHIPipeline::AsyncCompute                                │
│                                                                         │
│   ※通常の描画はGraphics、GPGPUやポストプロセスはAsyncComputeも利用可能    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### コマンドリストのライフサイクル

```
    ┌─────────────┐
    │   Obtain    │  ← RHIGetCommandContext() でコンテキスト取得
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Record    │  ← コマンドを記録（Open状態）
    │             │     RHISetGraphicsPipelineState, RHIDraw... 等
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │  Finalize   │  ← RHIFinalizeContext() → IRHIPlatformCommandList
    └──────┬──────┘
           │
           ▼
    ┌─────────────┐
    │   Submit    │  ← RHISubmitCommandLists() でGPUキューに送信
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

---

## 8. メモリ管理

### メモリタイプ

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         GPUメモリタイプ                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │                    DEFAULT (VRAM)                              │   │
│   │  ・GPUローカルメモリ（高速）                                     │   │
│   │  ・テクスチャ、レンダーターゲット、頻繁に使うバッファ              │   │
│   │  ・CPUから直接アクセス不可（Stagingバッファ経由でコピー）          │   │
│   └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │                    UPLOAD (CPU → GPU)                          │   │
│   │  ・CPUで書き込み、GPUで読み取り                                  │   │
│   │  ・頂点/インデックスバッファのアップロード                        │   │
│   │  ・定数バッファ（ユニフォームバッファ）                           │   │
│   │  ・Write-Combined メモリ（CPU読み取りは非常に遅い）               │   │
│   └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │                    READBACK (GPU → CPU)                        │   │
│   │  ・GPUで書き込み、CPUで読み取り                                  │   │
│   │  ・スクリーンショット、オクルージョンクエリ結果                   │   │
│   │  ・GPUからCPUへのデータ転送用                                    │   │
│   │  ・RHIStagingBuffer を使用して非ブロッキング読み戻し            │   │
│   └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### アロケーター構成

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        メモリアロケーター                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   YourDevice                                                           │
│       │                                                                 │
│       ├─── DefaultBufferAllocator                                       │
│       │        └─ 汎用バッファ用                                        │
│       │           ・サブアロケーション（プール）                          │
│       │           ・大きいリソースは専用Committed Resource               │
│       │                                                                 │
│       ├─── UploadHeapAllocator                                          │
│       │        └─ CPU→GPU転送用                                         │
│       │           ・リングバッファ方式                                   │
│       │           ・フレームごとにリセット                               │
│       │                                                                 │
│       ├─── TextureAllocator                                             │
│       │        └─ テクスチャ用                                          │
│       │           ・Placed Resource（ヒープ上に配置）                    │
│       │           ・アラインメント考慮                                   │
│       │                                                                 │
│       └─── TransientAllocator                                           │
│                └─ 一時リソース用                                        │
│                   ・フレーム内でのみ有効                                 │
│                   ・メモリエイリアシング                                 │
│                   ・IRHITransientResourceAllocator インターフェース       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### リソース配置戦略

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       リソース配置の種類                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   1. Committed Resource（専用割り当て）                                  │
│      ┌──────────────────────────────┐                                  │
│      │    リソース専用のメモリ領域     │                                  │
│      │    ・大きなリソース向け         │                                  │
│      │    ・レンダーターゲット等       │                                  │
│      └──────────────────────────────┘                                  │
│                                                                         │
│   2. Placed Resource（ヒープ上に配置）                                   │
│      ┌────────────────────────────────────────────────────────┐        │
│      │                      Heap                              │        │
│      │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐     │        │
│      │  │Resource1│ │Resource2│ │Resource3│ │   ...   │     │        │
│      │  └─────────┘ └─────────┘ └─────────┘ └─────────┘     │        │
│      │  ・複数リソースで1つのヒープを共有                       │        │
│      │  ・メモリ効率が良い                                    │        │
│      └────────────────────────────────────────────────────────┘        │
│                                                                         │
│   3. Sub-allocation（サブアロケーション）                                │
│      ┌────────────────────────────────────────────────────────┐        │
│      │                   大きなバッファ                        │        │
│      │  ┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐         │        │
│      │  │ A  ││ B  ││ C  ││ D  ││ E  ││ F  ││ G  │  ...    │        │
│      │  └────┘└────┘└────┘└────┘└────┘└────┘└────┘         │        │
│      │  ・小さなバッファを大きなバッファ内に配置               │        │
│      │  ・定数バッファ等で使用                                │        │
│      └────────────────────────────────────────────────────────┘        │
│                                                                         │
│   4. Reserved Resource（予約リソース、実験的機能）                       │
│      ・BufferUsageFlags::ReservedResource (1 << 22)                   │
│      ・物理メモリバッキングなしで作成（tiled/sparse/virtual）           │
│      ・必要に応じて物理メモリをコミット                                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 9. 同期メカニズム

### フェンス（Fence）

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          フェンスの概念                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   CPU                                                                   │
│   ───┬────────────────────────────────────────────────────────────▶    │
│      │                                                                  │
│      │ Submit CmdList1    Submit CmdList2    Wait(Fence, 2)            │
│      │      │                  │                   │                    │
│      │      ▼                  ▼                   ▼                    │
│   GPU│   ┌──────┐          ┌──────┐                                    │
│   ───┼───│CmdL1 │──────────│CmdL2 │──Signal(2)─────▶                   │
│      │   └──────┘          └──────┘          │                         │
│      │                                       │                         │
│      │                              Fence Value: 1 → 2                 │
│      │                                                                  │
│   ・Signal: GPUがフェンス値をインクリメント                              │
│   ・Wait: CPU/GPUが特定のフェンス値に達するまで待機                      │
│                                                                         │
│   RHIGPUFence (RHIResource 継承):                                     │
│     ・Clear() - フェンスリセット                                        │
│     ・Poll() -> bool - GPU完了確認（ノンブロッキング）                   │
│     ・Poll(RHIGPUMask) -> bool - 特定GPU群の完了確認                   │
│     ・Wait(RHICommandListImmediate&, RHIGPUMask) - ブロッキング待機  │
│                                                                         │
│   注意: Poll()をループで呼び出してブロックしてはならない。               │
│   一部のRHI実装ではRHIスレッドの進行が必要なためデッドロックする。       │
│   ブロック待機には Wait() を使用すること（レンダースレッドからのみ）。    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### リソース状態遷移（Barrier）

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        リソース状態遷移                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   リソースは用途に応じて「状態」を持ち、状態遷移にはバリアが必要           │
│                                                                         │
│   ┌─────────────────┐     Barrier      ┌─────────────────┐            │
│   │  RenderTarget   │ ──────────────▶ │ ShaderResource  │            │
│   │   (書き込み中)   │                  │   (読み取り)     │            │
│   └─────────────────┘                  └─────────────────┘            │
│                                                                         │
│   【RHIでの遷移API】                                                    │
│   ・DynamicRHI::RHICreateTransition() - トランジション作成            │
│   ・DynamicRHI::RHIReleaseTransition() - トランジション解放           │
│   ・IRHIComputeContext::RHIBeginTransitions() - 遷移開始               │
│   ・IRHIComputeContext::RHIEndTransitions() - 遷移完了                 │
│                                                                         │
│   【主な状態 (RHIAccess)】ビットフラグ (uint32)                        │
│   読み取り系:                                                           │
│   ・Unknown (= 0)        - 前状態が不明（全キャッシュフラッシュ）        │
│   ・CPURead              - CPU読み取り                                   │
│   ・Present              - 画面表示用                                    │
│   ・IndirectArgs         - 間接描画引数                                  │
│   ・VertexOrIndexBuffer  - 頂点/インデックスバッファ                     │
│   ・SRVCompute           - コンピュートシェーダーSRV読み取り             │
│   ・SRVGraphicsPixel     - ピクセルシェーダーSRV読み取り                 │
│   ・SRVGraphicsNonPixel  - 非ピクセルシェーダーSRV読み取り               │
│   ・CopySrc              - コピー元                                      │
│   ・ResolveSrc           - リゾルブ元                                    │
│   ・DSVRead              - 深度ステンシル読み取り                        │
│   読み書き系:                                                           │
│   ・AVCompute           - コンピュートUAV読み書き                       │
│   ・AVGraphics          - グラフィックスUAV読み書き                     │
│   ・RTV                  - レンダーターゲット書き込み                    │
│   ・CopyDest             - コピー先                                      │
│   ・ResolveDst           - リゾルブ先                                    │
│   ・DSVWrite             - 深度ステンシル書き込み                        │
│   レイトレーシング:                                                     │
│   ・BVHRead / BVHWrite   - AS入力/出力                                   │
│   その他:                                                               │
│   ・Discard              - 一時リソース解放                              │
│   ・ShadingRateSource    - VRS ソース                                    │
│   合成マスク:                                                           │
│   ・SRVGraphics = SRVGraphicsPixel | SRVGraphicsNonPixel                │
│   ・SRVMask = SRVCompute | SRVGraphics                                  │
│   ・AVMask = AVCompute | AVGraphics                                   │
│                                                                         │
│   【バリアの目的】                                                       │
│   ・キャッシュのフラッシュ/無効化                                        │
│   ・メモリ可視性の保証                                                   │
│   ・圧縮状態の解決（一部ハードウェア）                                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### GPU-CPU同期パターン

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      同期パターン例                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   【パターン1: フレーム同期】                                             │
│                                                                         │
│   Frame N        Frame N+1      Frame N+2                               │
│   ┌─────────┐   ┌─────────┐   ┌─────────┐                             │
│   │Render   │   │Render   │   │Render   │   ← GPU                     │
│   └────┬────┘   └────┬────┘   └────┬────┘                             │
│        │Signal       │Signal       │Signal                              │
│        ▼             ▼             ▼                                    │
│   ┌─────────┐   ┌─────────┐   ┌─────────┐                             │
│   │Wait&    │   │Wait&    │   │Wait&    │   ← CPU                     │
│   │Update   │   │Update   │   │Update   │                              │
│   └─────────┘   └─────────┘   └─────────┘                             │
│                                                                         │
│   【パターン2: リードバック（FRHIStagingBuffer使用）】                    │
│                                                                         │
│   GPU: Render ─▶ RHICopyToStagingBuffer() ─▶ RHIWriteGPUFence()      │
│                                                    │                    │
│   CPU: ────────────────────── RHIGPUFence::Poll() │                   │
│                                   │                                     │
│                                   ▼ (signaled)                          │
│                    RHILockStagingBuffer() → Read Data                   │
│                    RHIUnlockStagingBuffer()                             │
│                                                                         │
│   【パターン3: 非同期コンピュート】                                       │
│                                                                         │
│   Graphics Queue: ──Render──┬──Render──                                │
│                             │                                           │
│   Compute Queue:  ───────Compute───────                                │
│                             │                                           │
│                        Sync Point                                       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 10. プラットフォーム実装一覧

### 実装比較

| 項目 | D3D12RHI | VulkanRHI | MetalRHI | D3D11RHI | OpenGLDrv |
|------|----------|-----------|----------|----------|-----------|
| **対象API** | DirectX 12 | Vulkan | Metal | DirectX 11 | OpenGL/ES |
| **プラットフォーム** | Windows, Xbox | Windows, Linux, Android | iOS, macOS | Windows (Legacy) | Android, Linux |
| **レイトレーシング** | ○ | ○ | ○ | × | × |
| **メッシュシェーダー** | ○ | ○ | ○ | × | × |
| **Bindless** | ○ | ○ | ○ | △ | × |
| **マルチGPU** | ○ | △ (\*1) | × | × | × |

\*1: WITH_MGPU は Windows Desktop のみ有効（RHI.Build.cs）。VulkanRHI は Linux/Android でも
動作するが、mGPU サポートは Windows に限定される。

### ディレクトリ構造

```
Engine/Source/Runtime/
│
├── D3D12RHI/
│   ├── Public/
│   │   ├── D3D12RHI.h              # 公開定義
│   │   ├── ID3D12DynamicRHI.h      # 動的インターフェース
│   │   └── D3D12ShaderResources.h  # シェーダーリソース
│   └── Private/
│       ├── D3D12Adapter.h/cpp      # アダプター
│       ├── D3D12Device.h/cpp       # デバイス
│       ├── D3D12Queue.h            # キュー
│       ├── D3D12CommandContext.h/cpp # コマンドコンテキスト
│       ├── D3D12Resources.h/cpp    # リソース基底
│       ├── D3D12Texture.h/cpp      # テクスチャ
│       ├── D3D12Buffer.cpp         # バッファ
│       ├── D3D12Shader.h/cpp       # シェーダー
│       ├── D3D12PipelineState.h/cpp # PSO
│       ├── D3D12View.h/cpp         # ビュー (SRV/UAV/RTV/DSV)
│       ├── D3D12Viewport.h/cpp     # ビューポート
│       ├── D3D12Descriptors.h/cpp  # デスクリプタ管理
│       ├── D3D12Allocation.h/cpp   # メモリアロケーション
│       ├── D3D12RayTracing.h/cpp   # レイトレーシング
│       └── ...
│
├── VulkanRHI/
│   └── (同様の構造)
│
├── Apple/MetalRHI/
│   └── (同様の構造)
│
└── ...
```

---

## 11. 新規RHI作成ガイド

### 必要なコンポーネント一覧

新しいRHIバックエンドを作成する際に実装が必要なコンポーネントです。

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    新規RHI 必須コンポーネント                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【レベル1: 最小構成（静止画が表示できる）】                               │
│  ────────────────────────────────────────                               │
│  □ モジュール定義 (Build.cs + IDynamicRHIModule実装)                    │
│  □ YourDynamicRHI (FDynamicRHI継承)                                    │
│  □ YourAdapter                                                         │
│  □ YourDevice                                                          │
│  □ YourQueue (最低1つ: Graphics)                                       │
│  □ YourCommandContext (IRHICommandContext実装)                         │
│  □ YourPlatformCommandList (IRHIPlatformCommandList実装)               │
│  □ YourTexture (FRHITexture継承)                                       │
│  □ YourBuffer (FRHIBuffer継承)                                         │
│  □ YourVertexShader / YourPixelShader                                │
│  □ YourGraphicsPipelineState (FRHIGraphicsPipelineState継承)           │
│  □ YourViewport (FRHIViewport継承、スワップチェーン)                    │
│  □ 基本ステート (Sampler, Rasterizer, DepthStencil, Blend)              │
│  □ 基本ビュー (RHIShaderResourceView)                                 │
│                                                                         │
│  【レベル2: 基本機能（通常のレンダリング）】                               │
│  ────────────────────────────────────────                               │
│  □ YourComputeShader + ComputePipelineState                            │
│  □ RHIUnorderedAccessView (UAV)                                        │
│  □ RHIUniformBuffer                                                    │
│  □ RHIGPUFence (フェンス/同期機構)                                     │
│  □ リソース状態遷移 (RHICreateTransition / Begin/EndTransitions)        │
│  □ RHIStagingBuffer (リードバック)                                     │
│  □ RHIRenderQuery (オクルージョンクエリ等)                              │
│                                                                         │
│  【レベル3: 高度な機能】                                                  │
│  ────────────────────────────────────────                               │
│  □ AsyncComputeキュー (IRHIComputeContext)                              │
│  □ ジオメトリシェーダー (RHIGeometryShader)                             │
│  □ メッシュシェーダー / アンプリフィケーションシェーダー                  │
│  □ レイトレーシング (RHIRayTracingGeometry, RHIRayTracingScene,       │
│        RHIRayTracingPipelineState, RHIShaderBindingTable)             │
│  □ Bindlessリソース                                                     │
│  □ マルチGPUサポート (WITH_MGPU, RHITransferResources)                  │
│  □ VRS (Variable Rate Shading, RHISetShadingRate)                      │
│  □ ワークグラフ (RHIWorkGraphShader, RHIWorkGraphPipelineState)       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 実装の流れ

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      RHI実装のロードマップ                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   Phase 1: 基盤構築                                                      │
│   ─────────────────                                                     │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │ 1. モジュール作成                                             │      │
│   │    ・Build.cs でモジュール定義                                │      │
│   │    ・IDynamicRHIModule 実装                                  │      │
│   │      - IsSupported() → ハードウェア/ドライバ検出             │      │
│   │      - CreateRHI() → YourDynamicRHI インスタンス生成        │      │
│   │    ・RHI.Build.cs の DynamicallyLoadedModuleNames に登録     │      │
│   │                                                              │      │
│   │ 2. 階層構造の実装                                             │      │
│   │    ・YourDynamicRHI (Init/Shutdown/GetName 等)              │      │
│   │    ・YourAdapter (GPU列挙)                                  │      │
│   │    ・YourDevice (デバイス作成)                               │      │
│   │    ・YourQueue (コマンドキュー)                              │      │
│   └─────────────────────────────────────────────────────────────┘      │
│                              │                                          │
│                              ▼                                          │
│   Phase 2: リソース作成                                                  │
│   ─────────────────────                                                 │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │ 3. メモリ管理                                                 │      │
│   │    ・ヒープ/アロケーター                                      │      │
│   │    ・リソースロケーション                                     │      │
│   │                                                              │      │
│   │ 4. リソース実装                                               │      │
│   │    ・バッファ (RHIBuffer 継承)                               │      │
│   │    ・テクスチャ (RHITexture 継承)                            │      │
│   │    ・ビュー (RHIShaderResourceView, RHIUnorderedAccessView)│      │
│   └─────────────────────────────────────────────────────────────┘      │
│                              │                                          │
│                              ▼                                          │
│   Phase 3: パイプライン                                                  │
│   ─────────────────────                                                 │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │ 5. シェーダー                                                 │      │
│   │    ・各シェーダータイプ (VS, PS, CS, GS, MS, AS, ...)         │      │
│   │    ・シェーダーバイトコード処理                               │      │
│   │    ・全て (ArrayView<const uint8> Code, const SHAHash&)    │      │
│   │                                                              │      │
│   │ 6. パイプラインステート                                       │      │
│   │    ・RHIGraphicsPipelineState                               │      │
│   │    ・RHIComputePipelineState                                │      │
│   │    ・PSOキャッシュ                                            │      │
│   └─────────────────────────────────────────────────────────────┘      │
│                              │                                          │
│                              ▼                                          │
│   Phase 4: コマンド実行                                                  │
│   ─────────────────────                                                 │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │ 7. コマンドコンテキスト                                        │      │
│   │    ・IRHICommandContext 全メソッド実装                        │      │
│   │    ・IRHIPlatformCommandList 実装                            │      │
│   │    ・RHIFinalizeContext / RHISubmitCommandLists 実装          │      │
│   │                                                              │      │
│   │ 8. 同期機構                                                   │      │
│   │    ・RHIGPUFence (Clear/Poll/Wait)                          │      │
│   │    ・RHICreateTransition / Begin/EndTransitions              │      │
│   └─────────────────────────────────────────────────────────────┘      │
│                              │                                          │
│                              ▼                                          │
│   Phase 5: プレゼンテーション                                            │
│   ─────────────────────────                                             │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │ 9. ビューポート/スワップチェーン                               │      │
│   │    ・RHIViewport 継承                                       │      │
│   │    ・RHICreateViewport / RHIResizeViewport                   │      │
│   │    ・RHIGetViewportBackBuffer                                │      │
│   │    ・RHIAdvanceFrameForGetViewportBackBuffer                 │      │
│   │    ・RHIEndDrawingViewport (Present処理)                     │      │
│   └─────────────────────────────────────────────────────────────┘      │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### DynamicRHI 純粋仮想メソッド一覧

以下は最低限実装が必要な純粋仮想関数（= 0）です。

```
┌──────────────────────────────────────────────────────────────────────────┐
│                   DynamicRHI 純粋仮想メソッド（= 0）                     │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  【初期化/終了】                                                         │
│  ・void Init()                                                           │
│  ・void Shutdown()                                                       │
│  ・const TCHAR* GetName()                                                │
│                                                                          │
│  【ステート作成】                                                        │
│  ・RHICreateSamplerState(const SamplerStateInitializerRHI&)             │
│  ・RHICreateRasterizerState(const RasterizerStateInitializerRHI&)       │
│  ・RHICreateDepthStencilState(const DepthStencilStateInitializerRHI&)   │
│  ・RHICreateBlendState(const BlendStateInitializerRHI&)                 │
│  ・RHICreateVertexDeclaration(const VertexDeclarationElementList&)      │
│                                                                          │
│  【シェーダー作成】(全てTArrayView<const uint8> Code, const SHAHash& Hash)│
│  ・RHICreateVertexShader()                                               │
│  ・RHICreatePixelShader()                                                │
│  ・RHICreateGeometryShader()                                             │
│  ・RHICreateComputeShader()                                              │
│  ※ RHICreateMeshShader, RHICreateAmplificationShader,                   │
│    RHICreateWorkGraphShader は仮想（デフォルト: nullref 返却）           │
│                                                                          │
│  【パイプライン】                                                        │
│  ・RHICreateBoundShaderState(RHIVertexDeclaration*,                     │
│      RHIVertexShader*, RHIPixelShader*, RHIGeometryShader*)           │
│  ・RHICreateGraphicsPipelineState(const GraphicsPipelineStateInitializer&)│
│  ・RHICreateComputePipelineState(const ComputePipelineStateInitializer&) │
│                                                                          │
│  【リソース】                                                            │
│  ・RHICreateUniformBuffer(const void*, const RHIUniformBufferLayout*,   │
│      UniformBufferUsage, UniformBufferValidation)                      │
│  ・RHIUpdateUniformBuffer(RHICommandListBase&, RHIUniformBuffer*,      │
│      const void*)                                                        │
│  ・RHICreateBufferInitializer(RHICommandListBase&,                      │
│      const RHIBufferCreateDesc&) -> RHIBufferInitializer               │
│  ・RHICreateTextureInitializer(RHICommandListBase&,                     │
│      const RHITextureCreateDesc&) -> RHITextureInitializer             │
│  ・RHICreateShaderResourceView(RHICommandListBase&,                     │
│      RHIViewableResource*, RHIViewDesc const&)                         │
│  ・RHICreateUnorderedAccessView(RHICommandListBase&,                    │
│      RHIViewableResource*, RHIViewDesc const&)                         │
│  ・RHIReplaceResources(RHICommandListBase&,                             │
│      Array<RHIResourceReplaceInfo>&&)                                  │
│                                                                          │
│  【テクスチャ操作】                                                      │
│  ・RHICalcTexturePlatformSize(RHITextureDesc const&, uint32)            │
│      -> RHICalcTextureSizeResult                                        │
│  ・RHIGetTextureMemoryStats(TextureMemoryStats&)                        │
│  ・RHIGetTextureMemoryVisualizeData(Color*, int32, int32, int32, int32) │
│  ・RHIAsyncCreateTexture2D(uint32, uint32, uint8, uint32,                │
│      TextureCreateFlags, RHIAccess, void**, uint32, const TCHAR*,      │
│      GraphEventRef&) -> TextureRHIRef                                  │
│  ・RHIAsyncReallocateTexture2D(...)                                      │
│  ・RHILockTexture(RHICommandListImmediate&, const RHILockTextureArgs&) │
│  ・RHIUnlockTexture(RHICommandListImmediate&, const RHILockTextureArgs&)│
│  ・RHIUpdateTexture2D(RHICommandListBase&, RHITexture*, uint32,        │
│      const UpdateTextureRegion2D&, uint32, const uint8*)                │
│  ・RHIUpdateTexture3D(RHICommandListBase&, RHITexture*, uint32,        │
│      const UpdateTextureRegion3D&, uint32, uint32, const uint8*)        │
│  ・RHIComputeMemorySize(RHITexture*) -> uint32                          │
│                                                                          │
│  【読み取り】                                                            │
│  ・RHIReadSurfaceData(RHITexture*, IntRect, Array<Color>&,           │
│      ReadSurfaceDataFlags)                                              │
│  ・RHIMapStagingSurface(RHITexture*, RHIGPUFence*, void*&, int32&,     │
│      int32&, uint32 GPUIndex = 0)                                        │
│  ・RHIUnmapStagingSurface(RHITexture*, uint32 GPUIndex = 0)             │
│  ・RHIReadSurfaceFloatData(RHITexture*, IntRect, Array<Float16Color>&,│
│      CubeFace, int32, int32)                                            │
│  ・RHIRead3DSurfaceFloatData(RHITexture*, IntRect, IntPoint,          │
│      Array<Float16Color>&)                                             │
│                                                                          │
│  【クエリ】                                                              │
│  ・RHICreateRenderQuery(RenderQueryType) -> RenderQueryRHIRef          │
│  ・RHIGetRenderQueryResult(RHIRenderQuery*, uint64&, bool,              │
│      uint32 GPUIndex = INDEX_NONE) -> bool                               │
│                                                                          │
│  【ビューポート】                                                        │
│  ・RHICreateViewport(void*, uint32, uint32, bool, PixelFormat)          │
│  ・RHIResizeViewport(RHIViewport*, uint32, uint32, bool)                │
│  ・RHIGetViewportBackBuffer(RHIViewport*) -> TextureRHIRef             │
│  ・RHIAdvanceFrameForGetViewportBackBuffer(RHIViewport*, bool)          │
│                                                                          │
│  【フレーム管理】                                                        │
│  ・RHIEndFrame(const RHIEndFrameArgs& Args)                             │
│  ・RHITick(float DeltaTime)                                              │
│  ・RHIBlockUntilGPUIdle()                                                │
│  ・RHIFlushResources()                                                   │
│                                                                          │
│  【コマンド実行】                                                        │
│  ・RHIGetDefaultContext() -> IRHICommandContext*                          │
│  ・RHIGetCommandContext(RHIPipeline, RHIGPUMask)                       │
│      -> IRHIComputeContext*                                              │
│  ・RHIFinalizeContext(RHIFinalizeContextArgs&&,                         │
│      RHIPipelineArray<IRHIPlatformCommandList*>&)                       │
│  ・RHISubmitCommandLists(RHISubmitCommandListsArgs&&)                   │
│                                                                          │
│  【同期】                                                                │
│  ・RHICreateGPUFence(const Name&) -> GPUFenceRHIRef                    │
│                                                                          │
│  【その他】                                                              │
│  ・RHIGetAvailableResolutions(ScreenResolutionArray&, bool) -> bool     │
│  ・RHIGetSupportedResolution(uint32&, uint32&)                           │
│  ・RHIGetNativeDevice() -> void*                                         │
│  ・RHIGetNativeInstance() -> void*                                       │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

### デバッグのヒント

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       デバッグ・検証のヒント                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【1. バリデーションレイヤーの活用】                                      │
│     ・D3D12: D3D12 Debug Layer                                          │
│     ・Vulkan: Validation Layers                                         │
│     ・Metal: Metal Validation                                           │
│     → APIミスの早期検出                                                 │
│                                                                         │
│  【2. RHI Validation (ENABLE_RHI_VALIDATION)】                          │
│     ・UE内蔵のRHIバリデーション層                                       │
│     ・IRHIComputeContext::Tracker でリソース状態追跡                     │
│     ・RHIViewableResource, RHIView のバリデーション                    │
│                                                                         │
│  【3. GPUキャプチャツール】                                               │
│     ・RenderDoc                                                         │
│     ・PIX (Windows)                                                     │
│     ・Nsight (NVIDIA)                                                   │
│     ・Xcode GPU Frame Capture (Apple)                                   │
│     → フレーム解析、リソース状態確認                                    │
│                                                                         │
│  【4. GPU Breadcrumbs】                                                  │
│     ・WITH_RHI_BREADCRUMBS マクロ有効化                                  │
│     ・IRHIPlatformCommandList に RHIBreadcrumbRange を格納             │
│     ・RHIEndFrameArgs::GPUBreadcrumbs で収集                           │
│     ・クラッシュ時のGPU実行位置特定                                      │
│                                                                         │
│  【5. リソース追跡 (RHI_ENABLE_RESOURCE_INFO)】                         │
│     ・Shippingビルドでは常に無効                                        │
│     ・Testビルドではデフォルト無効だが                                   │
│       bTrackRHIResourceInfoForTest=true で有効化可能                    │
│     ・それ以外のビルド（Development等）では有効（Build.csで制御）        │
│     ・RHIResource::GetResourceInfo() でリソース情報取得                 │
│     ・RHIResource::BeginTrackingResource / EndTrackingResource         │
│                                                                         │
│  【6. 段階的実装】                                                       │
│     ・クリアカラーの表示から開始                                         │
│     ・三角形1枚の描画                                                   │
│     ・テクスチャ付き描画                                                 │
│     ・複雑なシーンへ                                                     │
│                                                                         │
│  【7. 既存RHIとの比較】                                                  │
│     ・D3D12RHIを参考実装として活用                                       │
│     ・同じ入力で同じ出力が得られるか確認                                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 参考資料

### 主要ソースファイル

| ファイル | パス | 説明 |
|----------|------|------|
| DynamicRHI.h | `RHI/Public/` | DynamicRHI / IDynamicRHIModule 定義 |
| RHIContext.h | `RHI/Public/` | IRHIComputeContext / IRHICommandContext 定義 |
| RHIResources.h | `RHI/Public/` | RHIResource / RHIView / 全リソースクラス |
| RHIDefinitions.h | `RHI/Public/` | RHIResourceType / BufferUsageFlags 等の定義 |
| RHI.Build.cs | `RHI/` | モジュール依存関係・プラットフォーム設定 |
| D3D12Adapter.h | `D3D12RHI/Private/` | Adapter実装例 |
| D3D12Device.h | `D3D12RHI/Private/` | Device実装例 |
| D3D12CommandContext.h | `D3D12RHI/Private/` | CommandContext実装例 |

### 関連ドキュメント

- [Unreal Engine Documentation - RHI](https://docs.unrealengine.com/)
- [DirectX 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/)
- [Vulkan Specification](https://www.khronos.org/registry/vulkan/)
- [Metal Programming Guide](https://developer.apple.com/metal/)

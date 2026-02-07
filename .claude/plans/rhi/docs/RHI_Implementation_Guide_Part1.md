# RHI 実装ガイド Part 1: 基盤とリソース

## 目次

1. [モジュール構成](#1-モジュール構成)
2. [FDynamicRHI 実装詳細](#2-fdynamicrhi-実装詳細)
3. [Adapter 実装詳細](#3-adapter-実装詳細)
4. [Device 実装詳細](#4-device-実装詳細)
5. [Queue 実装詳細](#5-queue-実装詳細)

---

## 1. モジュール構成

### 1.1 ディレクトリ構造

```
YourRHI/
│
├── YourRHI.Build.cs              ← モジュール定義
│
├── Public/                        ← 公開ヘッダー
│   ├── YourRHI.h                  ← 公開定数・マクロ
│   ├── IYourDynamicRHI.h          ← プラットフォーム固有インターフェース
│   ├── YourThirdParty.h           ← サードパーティヘッダーのインクルード
│   └── YourShaderResources.h      ← シェーダーリソース定義
│
└── Private/                       ← 非公開実装
    ├── YourRHIPrivate.h           ← 内部共通ヘッダー
    ├── YourRHIModule.cpp          ← モジュール登録
    │
    ├── YourDynamicRHI.h/cpp       ← FDynamicRHI実装
    ├── YourAdapter.h/cpp          ← Adapter
    ├── YourDevice.h/cpp           ← Device
    ├── YourQueue.h/cpp            ← Queue
    │
    ├── YourCommandContext.h/cpp   ← コマンドコンテキスト
    ├── YourCommandList.h/cpp      ← コマンドリスト
    ├── YourCommandAllocator.h/cpp ← コマンドアロケーター
    │
    ├── YourResources.h/cpp        ← リソース基底
    ├── YourBuffer.h/cpp           ← バッファ
    ├── YourTexture.h/cpp          ← テクスチャ
    │
    ├── YourShader.h/cpp           ← シェーダー
    ├── YourPipelineState.h/cpp    ← パイプラインステート
    ├── YourState.h/cpp            ← レンダーステート
    │
    ├── YourView.h/cpp             ← ビュー (SRV/UAV/RTV/DSV)
    ├── YourViewport.h/cpp         ← ビューポート
    │
    ├── YourDescriptors.h/cpp      ← デスクリプタ管理
    ├── YourAllocation.h/cpp       ← メモリアロケーション
    ├── YourFence.h/cpp            ← フェンス/同期
    └── YourQuery.h/cpp            ← クエリ
```

### 1.2 モジュール登録

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     モジュール登録の流れ                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  エンジン起動                                                            │
│       │                                                                 │
│       ▼                                                                 │
│  ┌─────────────────────────────────────┐                               │
│  │ PlatformCreateDynamicRHI() 呼び出し │                               │
│  │ (RHI選択ロジック)                    │                               │
│  └─────────────────┬───────────────────┘                               │
│                    │                                                    │
│                    ▼                                                    │
│  ┌─────────────────────────────────────┐                               │
│  │ IDynamicRHIModule::IsSupported()    │                               │
│  │ ・ドライバー存在チェック             │                               │
│  │ ・APIバージョンチェック              │                               │
│  │ ・ハードウェア要件チェック           │                               │
│  └─────────────────┬───────────────────┘                               │
│                    │ true                                               │
│                    ▼                                                    │
│  ┌─────────────────────────────────────┐                               │
│  │ IDynamicRHIModule::CreateRHI()      │                               │
│  │ ・アダプター列挙                     │                               │
│  │ ・最適アダプター選択                 │                               │
│  │ ・FYourDynamicRHI インスタンス作成   │                               │
│  └─────────────────┬───────────────────┘                               │
│                    │                                                    │
│                    ▼                                                    │
│  ┌─────────────────────────────────────┐                               │
│  │ GDynamicRHI = 作成したRHI           │                               │
│  │ GDynamicRHI->Init() 呼び出し        │                               │
│  └─────────────────────────────────────┘                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.3 Build.cs で指定すべき依存関係

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        必須依存モジュール                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【PublicDependencyModuleNames】                                        │
│  ・Core         - 基本型、コンテナ、メモリ管理                          │
│  ・RHI          - RHIインターフェース定義                               │
│                                                                         │
│  【PrivateDependencyModuleNames】                                       │
│  ・CoreUObject  - UObject システム                                      │
│  ・Engine       - エンジンコア                                          │
│  ・RHICore      - RHI共通ユーティリティ                                 │
│  ・RenderCore   - レンダリング基盤                                      │
│                                                                         │
│  【サードパーティ】                                                      │
│  ・グラフィックスAPI SDK (Vulkan SDK, Metal Framework 等)               │
│  ・ベンダー拡張 (NVIDIA, AMD, Intel 等)                                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. FDynamicRHI 実装詳細

### 2.1 クラス継承関係

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      FDynamicRHI 継承関係                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│                      FDynamicRHI                                        │
│                    (RHI/Public/DynamicRHI.h)                            │
│                     │                                                   │
│                     │ 継承                                              │
│                     ▼                                                   │
│              IYourDynamicRHI                                            │
│            (Public/IYourDynamicRHI.h)                                   │
│             ・ERHIInterfaceType 指定                                    │
│             ・プラットフォーム固有API宣言                                │
│                     │                                                   │
│                     │ 継承                                              │
│                     ▼                                                   │
│              FYourDynamicRHI                                            │
│           (Private/YourDynamicRHI.h)                                    │
│             ・全メソッドの実装                                           │
│             ・アダプター/デバイス管理                                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 FYourDynamicRHI の責務

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    FYourDynamicRHI の責務一覧                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【ライフサイクル管理】                                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・Init()        : アダプター初期化、デバイス作成、キュー作成      │   │
│  │ ・PostInit()    : 追加初期化（ピクセルフォーマット情報等）         │   │
│  │ ・Shutdown()    : 全リソース解放、デバイス破棄                    │   │
│  │ ・RHITick()     : 毎フレーム更新（メモリ統計、タイムアウト処理）   │   │
│  │ ・RHIEndFrame() : フレーム終了処理（リソース解放、統計収集）       │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【リソースファクトリ】                                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ステート作成:                                                     │   │
│  │ ・Sampler, Rasterizer, DepthStencil, Blend                       │   │
│  │                                                                   │   │
│  │ シェーダー作成:                                                   │   │
│  │ ・Vertex, Pixel, Compute, Geometry, Mesh, Amplification          │   │
│  │                                                                   │   │
│  │ パイプライン作成:                                                 │   │
│  │ ・GraphicsPipelineState, ComputePipelineState                    │   │
│  │                                                                   │   │
│  │ リソース作成:                                                     │   │
│  │ ・Buffer, Texture, UniformBuffer, SRV, UAV                       │   │
│  │                                                                   │   │
│  │ その他:                                                           │   │
│  │ ・Viewport, GPUFence, RenderQuery, Transition                    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【コマンド実行管理】                                                    │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・RHIGetDefaultContext()    : デフォルトコンテキスト取得          │   │
│  │ ・RHIGetCommandContext()    : パイプライン別コンテキスト取得      │   │
│  │ ・RHIFinalizeContext()      : コンテキスト終了、コマンドリスト化  │   │
│  │ ・RHISubmitCommandLists()   : GPUへコマンド送信                   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【アダプター/デバイス管理】                                             │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・複数アダプターの保持                                            │   │
│  │ ・現在使用中のアダプター/デバイスへのアクセス提供                 │   │
│  │ ・マルチGPU構成時のデバイス選択                                   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【スレッド管理】                                                        │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・サブミッションスレッド（コマンドリスト送信専用）                 │   │
│  │ ・インタラプトスレッド（GPU完了通知処理）                          │   │
│  │ ・遅延削除キュー管理                                              │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.3 メンバー変数

```
┌─────────────────────────────────────────────────────────────────────────┐
│                  FYourDynamicRHI 主要メンバー変数                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【アダプター管理】                                                      │
│  ・ChosenAdapters : TArray<TSharedPtr<FYourAdapter>>                    │
│    └─ 使用可能な全アダプターのリスト                                    │
│                                                                         │
│  【スレッド】                                                            │
│  ・SubmissionThread : FYourThread*                                      │
│    └─ コマンドリスト送信専用スレッド                                    │
│  ・InterruptThread : FYourThread*                                       │
│    └─ GPU完了イベント処理スレッド                                       │
│                                                                         │
│  【ペンディングキュー】                                                  │
│  ・PendingPayloadsForSubmission : TQueue<TArray<FYourPayload*>*>        │
│    └─ 送信待ちペイロード                                                │
│  ・ObjectsToDelete : TArray<FYourDeferredDeleteObject>                  │
│    └─ 遅延削除オブジェクト                                              │
│                                                                         │
│  【クリティカルセクション】                                              │
│  ・SubmissionCS : FCriticalSection                                      │
│  ・InterruptCS : FCriticalSection                                       │
│  ・ObjectsToDeleteCS : FCriticalSection                                 │
│                                                                         │
│  【タイミング】                                                          │
│  ・CurrentTimingPerQueue : FYourTimingArray                             │
│    └─ キューごとのGPUタイミング情報                                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.4 初期化シーケンス

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      Init() の処理フロー                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Init()                                                                 │
│    │                                                                    │
│    ├─1─▶ グローバル変数初期化                                           │
│    │      ・GRHIGlobals の設定                                          │
│    │      ・GMaxRHIFeatureLevel                                         │
│    │      ・GMaxRHIShaderPlatform                                       │
│    │                                                                    │
│    ├─2─▶ 各アダプターの初期化                                           │
│    │      for each Adapter:                                             │
│    │        ├─ Adapter->InitializeDevices()                             │
│    │        │    └─ 各デバイスノードの作成                               │
│    │        ├─ Adapter->InitializeRayTracing() (対応時)                 │
│    │        └─ Adapter->InitializeBindless() (対応時)                   │
│    │                                                                    │
│    ├─3─▶ デフォルトコンテキスト作成                                      │
│    │      for each Device:                                              │
│    │        └─ CreateCommandContext(Device, Direct, true)               │
│    │                                                                    │
│    ├─4─▶ サブミッションパイプ初期化                                      │
│    │      ・InitializeSubmissionPipe()                                  │
│    │      ・スレッド起動                                                 │
│    │                                                                    │
│    └─5─▶ 機能フラグ設定                                                 │
│           ・GRHISupportsAsyncTextureCreation                            │
│           ・GRHISupportsRayTracing                                      │
│           ・その他サポート機能フラグ                                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Adapter 実装詳細

### 3.1 Adapter の責務

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      FYourAdapter の責務                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【ハードウェア情報管理】                                                 │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ AdapterDesc (アダプター記述情報):                                 │   │
│  │ ・AdapterIndex      : システム内でのGPUインデックス              │   │
│  │ ・DeviceName        : GPU名 ("NVIDIA GeForce RTX 4090"等)        │   │
│  │ ・VendorId          : ベンダーID (0x10DE=NVIDIA等)               │   │
│  │ ・DeviceId          : デバイスID                                  │   │
│  │ ・DedicatedVideoMemory : 専用VRAM容量                            │   │
│  │ ・SharedSystemMemory   : 共有システムメモリ                       │   │
│  │ ・NumDeviceNodes    : GPUノード数 (SLI時は2以上)                 │   │
│  │                                                                   │   │
│  │ 機能レベル:                                                       │   │
│  │ ・MaxSupportedFeatureLevel : 対応Feature Level                   │   │
│  │ ・MaxSupportedShaderModel  : 対応Shader Model                    │   │
│  │ ・ResourceBindingTier      : リソースバインディング能力           │   │
│  │ ・ResourceHeapTier         : ヒープ能力                           │   │
│  │                                                                   │   │
│  │ 能力フラグ:                                                       │   │
│  │ ・bSupportsRayTracing      : レイトレーシング対応                 │   │
│  │ ・bSupportsMeshShaders     : メッシュシェーダー対応               │   │
│  │ ・bSupportsBindless        : Bindless対応                        │   │
│  │ ・bSupportsWaveOps         : Wave演算対応                         │   │
│  │ ・bUnifiedMemory           : 統合メモリ (APU等)                   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【デバイスノード管理】                                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・Devices配列の所有                                               │   │
│  │ ・GetDevice(GPUIndex) でアクセス提供                              │   │
│  │ ・デバイスの初期化/破棄                                           │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【共有リソース管理】                                                    │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ パイプラインステートキャッシュ:                                   │   │
│  │ ・PipelineStateCache : FYourPipelineStateCache                   │   │
│  │ ・複数デバイス間でPSOを共有（SLI時）                              │   │
│  │                                                                   │   │
│  │ ルートシグネチャ/パイプラインレイアウト:                          │   │
│  │ ・RootSignatureManager : FYourRootSignatureManager               │   │
│  │ ・シェーダーバインディング構成の管理                              │   │
│  │                                                                   │   │
│  │ コマンドシグネチャ:                                               │   │
│  │ ・DrawIndirectCommandSignature                                    │   │
│  │ ・DrawIndexedIndirectCommandSignature                             │   │
│  │ ・DispatchIndirectCommandSignature                                │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【ファクトリ/ネイティブオブジェクト】                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・NativeFactory   : API固有のファクトリ (DXGI Factory等)         │   │
│  │ ・NativeAdapter   : ネイティブアダプターオブジェクト              │   │
│  │ ・RootDevice      : ルートデバイス (デバイス作成用)               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Adapter のメンバー構成

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    FYourAdapter メンバー構成                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FYourAdapter                                                           │
│  │                                                                      │
│  ├── Desc : FYourAdapterDesc                                            │
│  │    └── アダプター情報（名前、VRAM、機能レベル等）                     │
│  │                                                                      │
│  ├── Devices : TArray<FYourDevice*>                                     │
│  │    └── デバイスノード配列 [通常1つ、SLI時は複数]                     │
│  │                                                                      │
│  ├── Viewports : TArray<FYourViewport*>                                 │
│  │    └── このアダプターに属するビューポート                             │
│  │                                                                      │
│  ├── PipelineStateCache : FYourPipelineStateCache                       │
│  │    └── PSO キャッシュ                                                │
│  │                                                                      │
│  ├── RootSignatureManager : FYourRootSignatureManager                   │
│  │    └── ルートシグネチャ管理                                          │
│  │                                                                      │
│  ├── FrameFence : FYourManualFence                                      │
│  │    └── フレーム同期用フェンス                                        │
│  │                                                                      │
│  ├── UploadHeapAllocator : TArray<FYourUploadHeapAllocator*>            │
│  │    └── デバイスごとのアップロードヒープ                               │
│  │                                                                      │
│  └── [Native Objects]                                                   │
│       ├── NativeFactory                                                 │
│       ├── NativeAdapter                                                 │
│       └── RootDevice / RootDevice1 / ... (バージョン別)                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.3 デバイス初期化フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    InitializeDevices() の流れ                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  InitializeDevices()                                                    │
│    │                                                                    │
│    ├─1─▶ ネイティブデバイス作成                                         │
│    │      ・API固有のデバイス作成関数呼び出し                            │
│    │      ・デバッグレイヤー有効化（デバッグビルド時）                    │
│    │                                                                    │
│    ├─2─▶ 機能レベル確認                                                 │
│    │      ・サポートするFeature Level取得                               │
│    │      ・Shader Model確認                                            │
│    │      ・オプション機能の確認                                        │
│    │                                                                    │
│    ├─3─▶ デバイスノード作成                                             │
│    │      for i = 0 to NumDeviceNodes-1:                                │
│    │        ├─ FYourDevice* Device = new FYourDevice(GPUMask, this)     │
│    │        ├─ Device->SetupAfterDeviceCreation()                       │
│    │        └─ Devices.Add(Device)                                      │
│    │                                                                    │
│    ├─4─▶ 共有リソース初期化                                             │
│    │      ・コマンドシグネチャ作成                                       │
│    │      ・ルートシグネチャマネージャー初期化                           │
│    │      ・PSOキャッシュ初期化                                         │
│    │                                                                    │
│    └─5─▶ アップロードヒープ初期化                                        │
│           for each Device:                                              │
│             └─ UploadHeapAllocator作成                                  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Device 実装詳細

### 4.1 Device の責務

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       FYourDevice の責務                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【キュー管理】                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ キュー種別:                                                       │   │
│  │ ・Direct (Graphics)  : 描画 + コンピュート                       │   │
│  │ ・Async (Compute)    : 非同期コンピュート専用                    │   │
│  │ ・Copy               : データ転送専用                            │   │
│  │                                                                   │   │
│  │ 責務:                                                             │   │
│  │ ・キューの作成と所有                                              │   │
│  │ ・キューへのアクセス提供 GetQueue(QueueType)                     │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【コンテキスト管理】                                                    │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ デフォルトコンテキスト:                                           │   │
│  │ ・ImmediateContext : 即時実行用（レンダースレッド専用）           │   │
│  │                                                                   │   │
│  │ コンテキストプール:                                               │   │
│  │ ・ObtainContext(QueueType) : プールから取得                      │   │
│  │ ・ReleaseContext(Context)  : プールに返却                        │   │
│  │ ・並列コマンドリスト記録に対応                                    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【コマンドリスト/アロケーター管理】                                     │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ コマンドアロケーター:                                             │   │
│  │ ・ObtainCommandAllocator(QueueType)                              │   │
│  │ ・ReleaseCommandAllocator(Allocator)                             │   │
│  │ ・GPU完了後に再利用可能                                           │   │
│  │                                                                   │   │
│  │ コマンドリスト:                                                   │   │
│  │ ・ObtainCommandList(Allocator, ...)                              │   │
│  │ ・ReleaseCommandList(CommandList)                                │   │
│  │ ・Close後に再利用可能                                             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【デスクリプタ管理】                                                    │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ デスクリプタヒープマネージャー:                                   │   │
│  │ ・DescriptorHeapManager     : ヒープ全体の管理                   │   │
│  │ ・OnlineDescriptorManager   : シェーダー可視デスクリプタ         │   │
│  │ ・OfflineDescriptorManager  : CPU専用デスクリプタ                │   │
│  │                                                                   │   │
│  │ Bindless (対応時):                                                │   │
│  │ ・BindlessDescriptorManager : Bindlessリソース管理               │   │
│  │ ・BindlessDescriptorAllocator                                    │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【メモリ管理】                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ アロケーター:                                                     │   │
│  │ ・DefaultBufferAllocator  : 汎用バッファ用                       │   │
│  │ ・DefaultFastAllocator    : 高速一時アロケーション               │   │
│  │ ・TextureAllocator        : テクスチャ用                         │   │
│  │                                                                   │   │
│  │ Residency (メモリ常駐管理):                                       │   │
│  │ ・ResidencyManager        : VRAMオーバーコミット対応             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【サンプラー管理】                                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・GlobalSamplerHeap : グローバルサンプラーヒープ                  │   │
│  │ ・CreateSampler()   : サンプラー作成                             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【レイトレーシング (対応時)】                                           │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・RayTracingPipelineCache                                        │   │
│  │ ・RayTracingCompactionRequestHandler                             │   │
│  │ ・GetRaytracingAccelerationStructurePrebuildInfo()               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Device のメンバー構成

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     FYourDevice メンバー構成                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FYourDevice                                                            │
│  │                                                                      │
│  ├── ParentAdapter : FYourAdapter*                                      │
│  │    └── 親アダプターへの参照                                          │
│  │                                                                      │
│  ├── GPUMask : FRHIGPUMask                                              │
│  │    └── このデバイスのGPUマスク                                       │
│  │                                                                      │
│  ├── NativeDevice : YourNativeDevice*                                   │
│  │    └── ネイティブデバイスオブジェクト                                 │
│  │                                                                      │
│  ├── Queues : TStaticArray<FYourQueue, QueueTypeCount>                  │
│  │    ├── [Direct]  : グラフィックスキュー                              │
│  │    ├── [Async]   : 非同期コンピュートキュー                          │
│  │    └── [Copy]    : コピーキュー                                      │
│  │                                                                      │
│  ├── ImmediateContext : FYourCommandContext*                            │
│  │    └── デフォルト（即時）コンテキスト                                 │
│  │                                                                      │
│  ├── [オブジェクトプール]                                                │
│  │    ├── ContextPool : TObjectPool<FYourContextCommon>                 │
│  │    ├── AllocatorPool : TObjectPool<FYourCommandAllocator>            │
│  │    └── CommandListPool : TObjectPool<FYourCommandList>               │
│  │                                                                      │
│  ├── [デスクリプタ管理]                                                  │
│  │    ├── DescriptorHeapManager : FYourDescriptorHeapManager            │
│  │    ├── OnlineDescriptorManager : FYourOnlineDescriptorManager        │
│  │    ├── OfflineDescriptorManagers : TArray<FYourOfflineDescMgr>       │
│  │    └── GlobalSamplerHeap : FYourGlobalSamplerHeap                    │
│  │                                                                      │
│  ├── [メモリ管理]                                                        │
│  │    ├── DefaultBufferAllocator : FYourDefaultBufferAllocator          │
│  │    ├── DefaultFastAllocator : FYourFastAllocator                     │
│  │    ├── TextureAllocator : FYourTextureAllocatorPool                  │
│  │    └── ResidencyManager : FYourResidencyManager                      │
│  │                                                                      │
│  ├── DefaultViews : FYourDefaultViews                                   │
│  │    └── デフォルトのNull SRV/UAV等                                    │
│  │                                                                      │
│  └── [プロパティキャッシュ]                                              │
│       ├── ConstantBufferPageProperties                                  │
│       └── ResourceAllocationInfoCache                                   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.3 デバイス初期化フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│                SetupAfterDeviceCreation() の流れ                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  SetupAfterDeviceCreation()                                             │
│    │                                                                    │
│    ├─1─▶ キュー作成                                                     │
│    │      for each QueueType:                                           │
│    │        └─ Queues[QueueType] = FYourQueue(this, QueueType, Index)   │
│    │                                                                    │
│    ├─2─▶ デスクリプタ管理初期化                                          │
│    │      ├─ DescriptorHeapManager.Initialize()                         │
│    │      ├─ OnlineDescriptorManager.Initialize()                       │
│    │      ├─ for each HeapType:                                         │
│    │      │    └─ OfflineDescriptorManagers[Type].Initialize()          │
│    │      └─ GlobalSamplerHeap.Initialize()                             │
│    │                                                                    │
│    ├─3─▶ メモリアロケーター初期化                                        │
│    │      ├─ DefaultBufferAllocator.Initialize()                        │
│    │      ├─ DefaultFastAllocator.Initialize()                          │
│    │      ├─ TextureAllocator.Initialize()                              │
│    │      └─ ResidencyManager.Initialize()                              │
│    │                                                                    │
│    ├─4─▶ デフォルトビュー作成                                            │
│    │      ├─ NullSRV作成                                                │
│    │      ├─ NullUAV作成                                                │
│    │      └─ NullCBV作成                                                │
│    │                                                                    │
│    └─5─▶ その他初期化                                                    │
│           ├─ タイムスタンプ周波数取得                                    │
│           ├─ プロパティキャッシュ初期化                                  │
│           └─ レイトレーシング初期化（対応時）                            │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 5. Queue 実装詳細

### 5.1 Queue の責務

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        FYourQueue の責務                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【コマンドキュー管理】                                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・ネイティブコマンドキューの所有                                  │   │
│  │ ・キュータイプに応じた機能制限の管理                              │   │
│  │   - Direct: 全機能                                               │   │
│  │   - Async: コンピュートのみ                                      │   │
│  │   - Copy: コピー操作のみ                                         │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【フェンス管理】                                                        │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・Fence : このキュー専用のフェンス                                │   │
│  │ ・コマンドリスト実行後にシグナル                                  │   │
│  │ ・GPU完了待機に使用                                               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【ペイロード管理】                                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ペンディングキュー:                                               │   │
│  │ ・PendingSubmission : 送信待ちペイロード                          │   │
│  │ ・PendingInterrupt  : 完了処理待ちペイロード                      │   │
│  │                                                                   │   │
│  │ バッチ処理:                                                       │   │
│  │ ・PayloadToSubmit   : 現在処理中のペイロード                      │   │
│  │ ・NumCommandListsInBatch : バッチ内コマンドリスト数               │   │
│  │ ・BatchedObjects    : バッチ化されたオブジェクト                  │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【オブジェクトプール】                                                   │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・Contexts   : コンテキストプール                                 │   │
│  │ ・Allocators : コマンドアロケータープール                         │   │
│  │ ・Lists      : コマンドリストプール                               │   │
│  │                                                                   │   │
│  │ ※GPU完了後に再利用可能になる                                     │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  【タイミング/診断】                                                     │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ ・Timing           : GPUタイミング情報                           │   │
│  │ ・DiagnosticBuffer : GPU診断バッファ（クラッシュ解析用）         │   │
│  │ ・BarrierTimestamps: バリアタイムスタンプクエリ                   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Queue のメンバー構成

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      FYourQueue メンバー構成                             │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FYourQueue                                                             │
│  │                                                                      │
│  ├── Device : FYourDevice* const                                        │
│  │    └── 親デバイス                                                    │
│  │                                                                      │
│  ├── QueueType : EYourQueueType const                                   │
│  │    └── Direct / Async / Copy                                         │
│  │                                                                      │
│  ├── QueueIndex : int32 const                                           │
│  │    └── キューインデックス                                            │
│  │                                                                      │
│  ├── NativeQueue : YourNativeQueue*                                     │
│  │    └── ネイティブコマンドキュー                                      │
│  │                                                                      │
│  ├── Fence : FYourFence                                                 │
│  │    └── このキュー専用フェンス                                        │
│  │                                                                      │
│  ├── [ペンディングキュー]                                                │
│  │    ├── PendingSubmission : TMpscQueue<FYourPayload*>                 │
│  │    │    └── マルチプロデューサ・シングルコンシューマ                  │
│  │    └── PendingInterrupt : TSpscQueue<FYourPayload*>                  │
│  │         └── シングルプロデューサ・シングルコンシューマ                │
│  │                                                                      │
│  ├── [現在の処理状態]                                                    │
│  │    ├── PayloadToSubmit : FYourPayload*                               │
│  │    ├── BarrierAllocator : FYourCommandAllocator*                     │
│  │    ├── BarrierTimestamps : FYourQueryAllocator                       │
│  │    ├── NumCommandListsInBatch : uint32                               │
│  │    └── BatchedObjects : FYourBatchedPayloadObjects                   │
│  │                                                                      │
│  ├── ObjectPool                                                         │
│  │    ├── Contexts : TObjectPool<FYourContextCommon>                    │
│  │    ├── Allocators : TObjectPool<FYourCommandAllocator>               │
│  │    └── Lists : TObjectPool<FYourCommandList>                         │
│  │                                                                      │
│  ├── Timing : FYourTiming*                                              │
│  │    └── GPUタイミング情報                                             │
│  │                                                                      │
│  ├── DiagnosticBuffer : TUniquePtr<FYourDiagnosticBuffer>               │
│  │    └── GPU診断バッファ                                               │
│  │                                                                      │
│  └── bSupportsTileMapping : bool                                        │
│       └── タイルマッピングサポートフラグ                                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.3 コマンド実行フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     キューでのコマンド実行フロー                         │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  レンダースレッド                サブミッションスレッド                   │
│       │                              │                                  │
│       │                              │                                  │
│  ┌────┴────┐                         │                                  │
│  │コマンド記録│                         │                                  │
│  │Context   │                         │                                  │
│  └────┬────┘                         │                                  │
│       │                              │                                  │
│       │ Finalize()                   │                                  │
│       ▼                              │                                  │
│  ┌─────────────────┐                 │                                  │
│  │ Payload作成      │                 │                                  │
│  │ ・CommandLists   │                 │                                  │
│  │ ・SyncPoints     │                 │                                  │
│  │ ・Queries        │                 │                                  │
│  └────────┬────────┘                 │                                  │
│           │                          │                                  │
│           │ PendingSubmission.Enqueue()                                 │
│           ▼                          │                                  │
│  ┌─────────────────────────────────┐│                                  │
│  │      PendingSubmission Queue    ││                                  │
│  └─────────────────────────────────┘│                                  │
│                                      │ Dequeue()                        │
│                                      ▼                                  │
│                              ┌───────────────┐                          │
│                              │FinalizePayload│                          │
│                              │・バリア挿入    │                          │
│                              │・バッチ化      │                          │
│                              └───────┬───────┘                          │
│                                      │                                  │
│                                      ▼                                  │
│                              ┌───────────────┐                          │
│                              │ExecuteCommand │                          │
│                              │Lists          │                          │
│                              │・GPU送信      │                          │
│                              │・フェンスシグナル                         │
│                              └───────┬───────┘                          │
│                                      │                                  │
│                                      │ PendingInterrupt.Enqueue()       │
│                                      ▼                                  │
│                              ┌─────────────────────────────────┐        │
│                              │      PendingInterrupt Queue     │        │
│                              └─────────────────────────────────┘        │
│                                                                         │
│                              インタラプトスレッド                        │
│                                      │ Dequeue()                        │
│                                      ▼                                  │
│                              ┌───────────────┐                          │
│                              │完了処理        │                          │
│                              │・リソース解放  │                          │
│                              │・プールへ返却  │                          │
│                              │・コールバック  │                          │
│                              └───────────────┘                          │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.4 キュータイプ別の機能

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      キュータイプ別機能対応表                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  機能                          │ Direct │ Async  │ Copy   │            │
│  ─────────────────────────────┼────────┼────────┼────────┤            │
│  グラフィックス描画             │   ○    │   ×    │   ×    │            │
│  コンピュートディスパッチ       │   ○    │   ○    │   ×    │            │
│  コピー操作                    │   ○    │   ○    │   ○    │            │
│  レイトレーシング              │   ○    │   ○    │   ×    │            │
│  タイムスタンプクエリ          │   ○    │   ○    │   △    │            │
│  パイプライン統計              │   ○    │   ○    │   ×    │            │
│  オクルージョンクエリ          │   ○    │   ×    │   ×    │            │
│                                                                         │
│  ○ = 完全サポート                                                       │
│  △ = 一部ハードウェアでサポート                                         │
│  × = 非サポート                                                         │
│                                                                         │
│  【使い分け指針】                                                        │
│  ・Direct  : メインの描画処理                                           │
│  ・Async   : 描画と並行して実行したいコンピュート処理                   │
│  ・Copy    : テクスチャアップロード、リードバック等                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 次のパートの内容

Part 2 では以下を解説します：
- コマンドコンテキスト実装詳細
- コマンドリスト/アロケーター
- リソース (Buffer, Texture) 実装
- ビュー (SRV, UAV, RTV, DSV) 実装

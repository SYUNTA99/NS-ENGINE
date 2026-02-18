# D3D12RHI 実装ガイド Part 1: アーキテクチャ概要

## 目次

1. [モジュール構成と依存関係](#1-モジュール構成と依存関係)
2. [クラス階層](#2-クラス階層)
3. [マルチGPUオブジェクトモデル](#3-マルチgpuオブジェクトモデル)
4. [ID3D12DynamicRHI パブリックAPI](#4-id3d12dynamicrhi-パブリックapi)
5. [FD3D12DynamicRHI 実装クラス](#5-fd3d12dynamicrhi-実装クラス)
6. [モジュール初期化シーケンス](#6-モジュール初期化シーケンス)
7. [コンパイル時定義とフィーチャーフラグ](#7-コンパイル時定義とフィーチャーフラグ)
8. [CVar概要テーブル](#8-cvar概要テーブル)

---

## 1. モジュール構成と依存関係

### 1.1 D3D12RHI.Build.cs 依存関係

```
┌─────────────────────────────────────────────────────────────────────────┐
│           D3D12RHI.Build.cs 依存関係（実際のソースより）                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  [SupportedPlatformGroups("Microsoft")]                                │
│  ※ Microsoft プラットフォームグループのみ対応                           │
│                                                                         │
│  【PublicDependencyModuleNames】                                        │
│  ・Core         - 基本型、コンテナ、メモリ管理                          │
│  ・RHI          - RHI インターフェース定義                              │
│                                                                         │
│  【PrivateDependencyModuleNames】                                       │
│  ・CoreUObject   - UObject システム                                    │
│  ・Engine        - エンジン基盤                                        │
│  ・RHICore       - RHI 共通ユーティリティ                              │
│  ・RenderCore    - レンダリング基盤                                    │
│  ・TraceLog      - トレースログ                                        │
│                                                                         │
│  【PrivateIncludePathModuleNames】                                      │
│  ・Shaders       - シェーダーインクルードパス                           │
│                                                                         │
│  【サードパーティ依存（Static）】                                        │
│  ・AMD_AGS         - AMD GPU Services ライブラリ                       │
│  ・DX12            - DirectX 12 SDK                                    │
│  ・IntelExtensionsFramework - Intel 拡張フレームワーク                  │
│  ・NVAftermath     - NVIDIA Aftermath (GPU クラッシュダンプ)           │
│  ・NVAPI           - NVIDIA API (ベンダー拡張)                         │
│                                                                         │
│  【Windows プラットフォーム固有】                                        │
│  ・HeadMountedDisplay - HMD サポート (PrivateDependency)               │
│  ・D3D12RHI_PLATFORM_HAS_CUSTOM_INTERFACE=0                            │
│  ・WinPixEventRuntime - PIX プロファイリング                            │
│    (bPixProfilingEnabled && (非Shipping||bAllowProfileGPUInShipping)   │
│     && (非Test||bAllowProfileGPUInTest) 時)                           │
│                                                                         │
│  【フラグ】                                                              │
│  ・bAllowConfidentialPlatformDefines = true                             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 ディレクトリ構造

```
D3D12RHI/
│
├── D3D12RHI.Build.cs                    ← モジュール定義
│
├── Internal/
│   └── D3D12RayTracingResources.h       ← RT リソース定義（モジュール間共有）
│
├── Public/                              ← 公開ヘッダー
│   ├── D3D12RHI.h                       ← 公開定数 (MAX_SRVS, MAX_CBS 等)
│   ├── ID3D12DynamicRHI.h               ← D3D12 固有 RHI インターフェース
│   ├── D3D12RootSignatureDefinitions.h   ← Root Signature 定数定義
│   ├── D3D12ShaderResources.h           ← シェーダーリソース定義
│   ├── D3D12ThirdParty.h                ← D3D12/DXGI SDK ヘッダー集約
│   └── Windows/
│       └── WindowsD3D12ThirdParty.h     ← Windows 版サードパーティ
│
└── Private/                             ← 非公開実装
    ├── D3D12RHIPrivate.h                ← 内部共通ヘッダー (全Private含む)
    ├── D3D12RHICommon.h                 ← 共通型定義
    ├── D3D12RHIDefinitions.h            ← コンパイル時定義
    ├── D3D12Access.h                    ← ED3D12Access アクセスフラグ
    │
    ├── D3D12Adapter.h/.cpp              ← FD3D12Adapter
    ├── D3D12Device.h/.cpp               ← FD3D12Device
    ├── D3D12Queue.h                     ← FD3D12Queue
    ├── D3D12Submission.h/.cpp           ← FD3D12Payload / FD3D12SyncPoint
    │
    ├── D3D12CommandContext.h/.cpp        ← FD3D12CommandContext
    ├── D3D12CommandList.h/.cpp           ← FD3D12CommandList
    ├── D3D12Commands.cpp                ← RHI コマンド実装
    │
    ├── D3D12Resources.h/.cpp            ← FD3D12Resource / FD3D12Heap
    ├── D3D12Buffer.cpp                  ← バッファ作成
    ├── D3D12Texture.h/.cpp              ← FD3D12Texture
    ├── D3D12View.h/.cpp                 ← SRV/UAV/RTV/DSV ビュー
    │
    ├── D3D12Allocation.h/.cpp           ← Buddy/FreeList アロケータ
    ├── D3D12PoolAllocator.h/.cpp        ← プールアロケータ
    ├── D3D12TransientResourceAllocator.h/.cpp
    │
    ├── ID3D12Barriers.h                 ← バリア抽象インターフェース
    ├── D3D12BarriersFactory.h           ← バリアファクトリ
    ├── D3D12EnhancedBarriers.h/.cpp     ← Enhanced Barriers 実装
    ├── D3D12LegacyBarriers.h/.cpp       ← Legacy Barriers 実装
    │
    ├── D3D12Descriptors.h/.cpp          ← ディスクリプタヒープ管理
    ├── D3D12DescriptorCache.h/.cpp      ← オンラインディスクリプタキャッシュ
    ├── D3D12BindlessDescriptors.h/.cpp  ← Bindless レンダリング
    ├── D3D12ExplicitDescriptorCache.h/.cpp ← RT 用ディスクリプタ
    │
    ├── D3D12PipelineState.h/.cpp        ← PSO 管理
    ├── D3D12RootSignature.h/.cpp        ← Root Signature
    ├── D3D12StateCachePrivate.h         ← ステートキャッシュ
    ├── D3D12StateCache.cpp
    ├── D3D12DiskCache.h                 ← PSO ディスクキャッシュ
    │
    ├── D3D12Shader.h                    ← FD3D12*Shader
    ├── D3D12Shaders.cpp
    ├── D3D12State.h                     ← サンプラー/ラスタライザー/ブレンド
    ├── D3D12ConstantBuffer.h            ← 定数バッファ管理
    │
    ├── D3D12Viewport.h/.cpp             ← スワップチェーン管理
    ├── D3D12Query.h/.cpp                ← GPU クエリ
    ├── D3D12GPUProfiler.h/.cpp          ← GPU プロファイラー
    │
    ├── D3D12RayTracing.h/.cpp           ← レイトレーシング (6419行)
    ├── D3D12RayTracingDebug.h/.cpp      ← RT デバッグ
    ├── D3D12WorkGraph.h/.cpp            ← Work Graph
    │
    ├── D3D12Residency.h                 ← レジデンシー管理
    ├── D3D12NvidiaExtensions.h/.cpp      ← NVIDIA 拡張
    ├── D3D12IntelExtensions.h           ← Intel 拡張
    ├── D3D12AmdExtensions.h             ← AMD 拡張
    ├── D3D12Util.h/.cpp                 ← ユーティリティ
    ├── D3D12Stats.h                     ← 統計宣言
    │
    ├── D3D12RHI.cpp                     ← メインモジュール実装
    ├── D3D12RenderTarget.cpp            ← レンダーターゲット操作
    │
    ├── D3D12ResourceCollection.h        ← Bindless リソースコレクション
    ├── D3D12TextureReference.h          ← テクスチャ参照
    ├── D3D12DirectCommandListManager.h  ← コマンドリスト管理
    │
    └── Windows/                         ← Windows プラットフォーム固有
        ├── WindowsD3D12Adapter.h        ← Windows Adapter 拡張
        ├── WindowsD3D12Device.cpp       ← Init(), FindAdapter()
        ├── WindowsD3D12RHIDefinitions.h ← Windows 固有定義
        ├── WindowsD3D12PipelineState.h/.cpp
        ├── WindowsD3D12BarriersFactory.h
        ├── WindowsD3D12BindlessDescriptors.h
        ├── WindowsD3D12DiskCache.h
        ├── WindowsD3D12Submission.h
        └── WindowsD3D12Viewport.cpp
```

### 1.3 リソースバインディング定数 (D3D12RHI.h)

```
┌─────────────────────────────────────────────────────────────────────────┐
│              リソースバインディング上限定数                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  #define MAX_SRVS      64    ← SRV スロット上限 (64bit マスク制約)     │
│  #define MAX_SAMPLERS   32    ← サンプラースロット上限                  │
│  #define MAX_UAVS       16    ← UAV スロット上限                       │
│  #define MAX_CBS        16    ← 定数バッファスロット上限                │
│                                                                         │
│  ※ Resource Binding Tier 1/2 の制約は考慮不要:                        │
│    1) PC ではシェーダーコンパイラがプロファイル制限を強制               │
│    2) グローバルルートシグネチャ使用プラットフォームは                   │
│       全て Tier 3 をサポート                                           │
│                                                                         │
│  【スロットマスク型定義】(D3D12RHICommon.h)                             │
│  ・CBVSlotMask   = uint16     (MAX_CBS <= 16)                          │
│  ・SRVSlotMask   = uint64     (MAX_SRVS > 32 の場合)                  │
│  ・SamplerSlotMask = uint32   (MAX_SAMPLERS <= 32)                     │
│  ・UAVSlotMask   = uint16     (MAX_UAVS <= 16)                        │
│                                                                         │
│  ・GRootCBVSlotMask = (1 << MAX_ROOT_CBVS) - 1                        │
│    Root Descriptor 用 CBV スロットマスク                                │
│  ・GDescriptorTableCBVSlotMask = 全CBV & ~RootCBV                      │
│    Descriptor Table 用 CBV スロットマスク                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. クラス階層

### 2.1 DynamicRHI 継承チェーン

```
┌─────────────────────────────────────────────────────────────────────────┐
│              D3D12RHI クラス階層                                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FDynamicRHI                      (RHI/Public/DynamicRHI.h)            │
│    │                                                                    │
│    ▼                                                                    │
│  ID3D12DynamicRHI                 (D3D12RHI/Public/ID3D12DynamicRHI.h) │
│    │ ・GetInterfaceType() → ERHIInterfaceType::D3D12                   │
│    │ ・D3D12 固有パブリック API 宣言                                    │
│    │                                                                    │
│    ▼  (D3D12RHI_PLATFORM_HAS_CUSTOM_INTERFACE == 0 の場合)             │
│  ID3D12PlatformDynamicRHI = ID3D12DynamicRHI  (typedef)                │
│    │                                                                    │
│    │  (D3D12RHI_PLATFORM_HAS_CUSTOM_INTERFACE == 1 の場合)             │
│    │  ID3D12PlatformDynamicRHI   ← 別途定義 (コンソール等)             │
│    │                                                                    │
│    ▼                                                                    │
│  FD3D12DynamicRHI                 (D3D12RHI/Private/D3D12RHIPrivate.h) │
│    ・SingleD3DRHI (staticシングルトン)                                   │
│    ・ChosenAdapters (TArray<TSharedPtr<FD3D12Adapter>>)                │
│    ・SubmissionThread / InterruptThread                                 │
│    ・全 RHI 仮想関数の実装                                              │
│    ・ベンダー拡張コンテキスト (AGS, NVAPI, Intel)                       │
│                                                                         │
│  FD3D12DynamicRHIModule           (D3D12RHI/Private/D3D12RHIPrivate.h) │
│    ・IDynamicRHIModule 実装                                             │
│    ・FindAdapter() で GPU 列挙                                          │
│    ・CreateRHI() で FD3D12DynamicRHI 生成                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 オブジェクト所有権モデル

```
┌─────────────────────────────────────────────────────────────────────────┐
│              D3D12RHI オブジェクト所有権階層                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12DynamicRHI (シングルトン)                                        │
│    │                                                                    │
│    ├── FD3D12Adapter (1つ以上、TSharedPtr)                             │
│    │     │  ・ID3D12Device* (RootDevice 〜 RootDevice12)               │
│    │     │  ・IDXGIFactory2* 〜 IDXGIFactory7*                         │
│    │     │  ・FD3D12RootSignatureManager                                │
│    │     │  ・FD3D12PipelineStateCache                                  │
│    │     │  ・FD3D12FenceCorePool                                       │
│    │     │                                                              │
│    │     ├── FD3D12Device (ノード数分、LDA時は複数)                     │
│    │     │     │  ・FD3D12Queue (Graphics/Compute/Copy)                │
│    │     │     │  ・FD3D12CommandContext (デフォルト+非同期)             │
│    │     │     │  ・FD3D12DescriptorHeapManager                        │
│    │     │     │  ・FD3D12BindlessDescriptorManager                    │
│    │     │     │  ・FD3D12SamplerManager                               │
│    │     │     │  ・FD3D12QueryHeapManager                             │
│    │     │     │  ・Allocators (Buddy/Pool/Transient)                  │
│    │     │     │                                                        │
│    │     │     └── FD3D12Queue                                         │
│    │     │           ・ID3D12CommandQueue*                              │
│    │     │           ・FD3D12Fence                                      │
│    │     │           ・BarrierTimestamp                                  │
│    │     │                                                              │
│    │     └── FD3D12CommandContextRedirector (MGPU ルーティング)         │
│    │                                                                    │
│    ├── SubmissionThread (FD3D12Thread)                                  │
│    ├── InterruptThread  (FD3D12Thread)                                 │
│    └── ObjectsToDelete  (遅延削除キュー)                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.3 親子関係ミックスインクラス

```
┌─────────────────────────────────────────────────────────────────────────┐
│              親子関係ミックスイン (D3D12RHICommon.h)                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12AdapterChild                                                    │
│    ・ParentAdapter (FD3D12Adapter*)                                    │
│    ・GetParentAdapter() / SetParentAdapter()                           │
│    ・Adapter レベルの共有リソースにアクセスするオブジェクト用            │
│                                                                         │
│  FD3D12DeviceChild                                                     │
│    ・Parent (FD3D12Device*)                                            │
│    ・GetParentDevice() / GetParentDevice_Unsafe()                      │
│    ・特定の GPU ノードに属するオブジェクト用                             │
│                                                                         │
│  ※ 多くの D3D12 オブジェクトはこれらを多重継承して所有関係を表現        │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. マルチGPUオブジェクトモデル

### 3.1 GPU マスクとオブジェクト階層

```
┌─────────────────────────────────────────────────────────────────────────┐
│              マルチ GPU オブジェクトモデル (D3D12RHICommon.h)            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12GPUObject (基底)                                                │
│    ・GPUMask       (FRHIGPUMask) — 所属 GPU ノード                    │
│    ・VisibilityMask (FRHIGPUMask) — アクセス可能な GPU ノード          │
│    ※ WITH_MGPU=0 時は常に GPU0 を返す constexpr                       │
│    │                                                                    │
│    ├── FD3D12SingleNodeGPUObject                                       │
│    │     ・GPUIndex (uint32) — 単一ノード用ショートカット              │
│    │     ・GPUMask == VisibilityMask (自ノードのみ)                    │
│    │                                                                    │
│    └── FD3D12MultiNodeGPUObject                                        │
│          ・NodeMask ⊃ VisibilityMask の交差チェック付き               │
│          ・複数ノードからの可視性を持つオブジェクト用                    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 FD3D12LinkedAdapterObject テンプレート

```
┌─────────────────────────────────────────────────────────────────────────┐
│       FD3D12LinkedAdapterObject<ObjectType> (D3D12RHICommon.h)         │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  マルチ GPU (LDA) 環境で GPU ごとのオブジェクトを連結管理するテンプレート│
│                                                                         │
│  【内部構造】(WITH_MGPU 有効時)                                        │
│  ・FLinkedObjects                                                      │
│    ├── GPUMask (FRHIGPUMask)                                          │
│    └── Objects[MAX_NUM_GPUS] (TStaticArray<ObjectType*>)              │
│                                                                         │
│  【主要メソッド】                                                        │
│  ・CreateLinkedObjects<ReturnType>(GPUMask, pfnGetParam, pfnCore)      │
│    → GPU ごとにオブジェクトを生成し、LinkedObjects 配列で相互参照      │
│    → 先頭オブジェクトは AddRef しない (循環参照防止)                   │
│    → 全オブジェクトに LinkedObjects 配列のコピーを伝播                 │
│                                                                         │
│  ・GetLinkedObject(GPUIndex) → 指定 GPU のオブジェクトを取得           │
│  ・GetFirstLinkedObject()    → 先頭 (代表) オブジェクト               │
│  ・IsHeadLink()              → 自身が先頭リンクか判定                  │
│  ・GetLinkedObjectsGPUMask() → 全リンク対象の GPU マスク              │
│                                                                         │
│  【FLinkedObjectIterator】                                              │
│  ・リンクされた全 GPU オブジェクトを順次走査するイテレータ              │
│  ・begin() / end() でレンジベース for 対応                             │
│                                                                         │
│  【TD3D12DualLinkedObjectIterator<T0, T1>】                            │
│  ・2つの LinkedAdapterObject を同時走査                                │
│  ・GPU マスク一致を前提条件としてチェック                               │
│  ・リソースビュー更新やデータコピーに使用                               │
│                                                                         │
│  【デストラクタ】                                                        │
│  ・先頭リンクオブジェクトのみが他リンクオブジェクトを Release           │
│  ・DoNotDeferDelete 対策として蓄積→一括解放                            │
│                                                                         │
│  【SGPU 時 (WITH_MGPU=0)】                                            │
│  ・LinkedObjects 構造体自体が存在しない                                 │
│  ・GetLinkedObject(0) → const_cast(this)                              │
│  ・GetLinkedObjectsGPUMask() → GPU0 固定                              │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.3 ResourceCast パターン

```
┌─────────────────────────────────────────────────────────────────────────┐
│              ResourceCast (D3D12RHIPrivate.h)                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  template<typename TRHIType>                                           │
│  struct TD3D12ResourceTraits { };                                      │
│  ※ 各リソース型で特殊化され、TConcreteType を定義                     │
│                                                                         │
│  FD3D12DynamicRHI::ResourceCast<TRHIType>(Resource)                    │
│    → static_cast<TD3D12ResourceTraits<TRHIType>::TConcreteType*>      │
│                                                                         │
│  FD3D12DynamicRHI::ResourceCast<TRHIType>(Resource, GPUIndex)          │
│    → ResourceCast → GetLinkedObject(GPUIndex)                         │
│    → マルチ GPU 時に特定ノードのオブジェクトを取得                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 4. ID3D12DynamicRHI パブリックAPI

### 4.1 インターフェース定義 (ID3D12DynamicRHI.h)

```
┌─────────────────────────────────────────────────────────────────────────┐
│  ID3D12DynamicRHI : public FDynamicRHI                                 │
│  (D3D12RHI/Public/ID3D12DynamicRHI.h)                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  GetInterfaceType() → ERHIInterfaceType::D3D12 (final)                │
│                                                                         │
│  ※ 外部プラグインが D3D12 ネイティブリソースにアクセスするための       │
│    公開インターフェース。エンジン外部コードが利用可能。                 │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

| カテゴリ | メソッド | 説明 |
|---------|---------|------|
| **アダプタ情報** | `RHIGetAdapterDescs()` | TArray<FD3D12MinimalAdapterDesc> (DXGI_ADAPTER_DESC + ノード数) を返す |
| | `RHIIsPixEnabled()` | PIX プロファイリング有効か |
| **ネイティブオブジェクト** | `RHIGetCommandQueue()` | ID3D12CommandQueue* を取得 |
| | `RHIGetDevice(InIndex)` | ID3D12Device* を取得 (ノードインデックス指定) |
| | `RHIGetDeviceNodeMask(InIndex)` | デバイスノードマスクを取得 |
| | `RHIGetGraphicsCommandList(CmdList, DeviceIndex)` | 実行中の ID3D12GraphicsCommandList* |
| | `RHIGetSwapChainFormat(InFormat)` | DXGI_FORMAT へのマッピング |
| **テクスチャ相互運用** | `RHICreateTexture2DFromResource(...)` | 既存 ID3D12Resource からテクスチャ作成 |
| | `RHICreateTexture2DArrayFromResource(...)` | 同上 (2D配列) |
| | `RHICreateTextureCubeFromResource(...)` | 同上 (キューブ) |
| **リソース情報** | `RHIGetResource(Buffer/Texture)` | ID3D12Resource* を取得 |
| | `RHIGetResourceDeviceIndex(Buffer/Texture)` | リソースの GPU ノードインデックス |
| | `RHIGetResourceMemorySize(Buffer/Texture)` | リソースメモリサイズ (bytes) |
| | `RHIIsResourcePlaced(Buffer/Texture)` | Placed allocation かどうか |
| **レンダーターゲット** | `RHIGetRenderTargetView(Texture, Mip, Slice)` | D3D12_CPU_DESCRIPTOR_HANDLE |
| **外部コンピュート** | `RHIFinishExternalComputeWork(...)` | 外部コマンドリスト完了通知 |
| **リソース遷移** | `RHITransitionResource(...)` | ⚠️ Deprecated (5.6)。ERHIAccess ベース推奨 |
| **手動フェンス** | `RHISignalManualFence(Fence, Value)` | ID3D12Fence をシグナル |
| | `RHIWaitManualFence(Fence, Value)` | ID3D12Fence を待機 |
| **キュー実行** | `RHIRunOnQueue(QueueType, CodeToRun, bWait)` | 指定キューでコード実行 |
| **バリア** | `RHIFlushResourceBarriers(CmdList, GPUIndex)` | リソースバリアをフラッシュ |
| **レジデンシー** | `RHIUpdateResourceResidency(CmdList, GPUIndex, Resource)` | レジデンシーハンドル追加 |
| **エラー検証** | `RHIVerifyResult(Device, Result, Code, File, Line)` | HRESULT 検証・エラーレポート |

### 4.2 ヘルパー関数

```cpp
// D3D12 RHI が有効か判定
inline bool IsRHID3D12()
{
    return GDynamicRHI != nullptr
        && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::D3D12;
}

// ID3D12DynamicRHI ポインタ取得
inline ID3D12DynamicRHI* GetID3D12DynamicRHI()
{
    check(GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::D3D12);
    return GetDynamicRHI<ID3D12DynamicRHI>();
}

// プラットフォーム固有インターフェース取得
inline ID3D12PlatformDynamicRHI* GetID3D12PlatformDynamicRHI()
// ※ Windows では ID3D12PlatformDynamicRHI = ID3D12DynamicRHI (typedef)
```

### 4.3 ED3D12RHIRunOnQueueType

```
┌─────────────────────────────────────────────────────────────────────────┐
│  enum class ED3D12RHIRunOnQueueType                                    │
├─────────────────────────────────────────────────────────────────────────┤
│  Graphics = 0    ← グラフィックスキューで実行                          │
│  Copy            ← コピーキューで実行                                  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 5. FD3D12DynamicRHI 実装クラス

### 5.1 主要メンバ変数

| メンバ | 型 | 説明 |
|--------|-----|------|
| `SingleD3DRHI` | `static FD3D12DynamicRHI*` | シングルトンインスタンス |
| `ChosenAdapters` | `TArray<TSharedPtr<FD3D12Adapter>>` | 選択されたアダプタ群 |
| `SubmissionThread` | `FD3D12Thread*` | GPU サブミッションスレッド |
| `InterruptThread` | `FD3D12Thread*` | GPU 完了割り込みスレッド |
| `PendingPayloadsForSubmission` | `TQueue<...*, EQueueMode::Mpsc>` | MPSC ペイロードキュー |
| `ObjectsToDelete` | `TArray<FD3D12DeferredDeleteObject>` | 遅延削除キュー |
| `FeatureLevel` | `D3D_FEATURE_LEVEL` | デバイスの Feature Level |
| `AmdAgsContext` | `AGSContext*` | AMD AGS コンテキスト |
| `IntelExtensionContext` | `INTCExtensionContext*` | Intel 拡張コンテキスト |
| `bPixEventEnabled` | `bool` | PIX イベント有効フラグ |
| `bDriverCacheAwarePSOPrecaching` | `bool` | PSO ドライバキャッシュ考慮 |
| `ZeroBuffer` | `void*` | ゼロ初期化バッファ (テクスチャストリーミング用) |
| `ZeroBufferSize` | `uint32` | ゼロバッファサイズ (デフォルト 4MB) |
| `FlipEvent` | `HANDLE` | フレームフリップイベント |
| `EopTask` | `FGraphEventRef` | End-of-Pipe タスクグラフイベント |

### 5.2 遅延削除タイプ (FD3D12DeferredDeleteObject)

```
┌─────────────────────────────────────────────────────────────────────────┐
│        遅延削除オブジェクトタイプ (EType)                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  RHIObject          - RHI リソース (最終 Release, RefCount==1 検証)    │
│  Heap               - D3D12 ヒープ (追加参照あり得る)                  │
│  DescriptorHeap     - ディスクリプタヒープ (ImmediateFreeHeap)         │
│  D3DObject          - 生 ID3D12Object (Release)                        │
│  BindlessDescriptor - Bindless ディスクリプタハンドル (ImmediateFree)  │
│  BindlessDescriptorHeap - Bindless ヒープ (Recycle)                   │
│  CPUAllocation      - CPU メモリ (FMemory::Free)                      │
│  DescriptorBlock    - ディスクリプタブロック (Recycle)                  │
│  VirtualAllocation  - 仮想テクスチャメモリ (DestroyVirtualTexture)     │
│  Func               - カスタムクリーンアップ関数                        │
│  TextureStagingBuffer - テクスチャステージングバッファ (ReuseStagingBuffer) │
│                                                                         │
│  ※ 全オブジェクトは EnqueueEndOfPipeTask() 経由で                     │
│    GPU 全キューの完了後に実際の破棄が実行される                         │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 6. モジュール初期化シーケンス

### 6.1 起動フロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│              D3D12RHI 初期化シーケンス                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. FD3D12DynamicRHIModule::StartupModule()                            │
│     └→ モジュールロード                                                │
│                                                                         │
│  2. FD3D12DynamicRHIModule::IsSupported()                              │
│     └→ D3D12 対応チェック (Feature Level SM5 以上)                    │
│                                                                         │
│  3. FD3D12DynamicRHIModule::CreateRHI()                                │
│     ├→ FindAdapter()                                                   │
│     │   ├→ DXGI Factory 作成                                          │
│     │   ├→ GPU 列挙 (EnumAdapters, DXGI_GPU_PREFERENCE)               │
│     │   ├→ Feature Level / Shader Model 検出                          │
│     │   ├→ Resource Binding Tier / Heap Tier 検出                     │
│     │   ├→ Wave Ops / Atomic64 サポート検出                           │
│     │   └→ ChosenAdapters に選択結果を格納                             │
│     │                                                                   │
│     └→ new FD3D12DynamicRHI(ChosenAdapters, bPixEnabled)              │
│                                                                         │
│  4. FD3D12DynamicRHI::FD3D12DynamicRHI() [コンストラクタ]              │
│     ├→ SingleD3DRHI = this (シングルトン登録)                          │
│     ├→ FeatureLevel 確認 (>= D3D_FEATURE_LEVEL_11_0)                  │
│     ├→ ZeroBuffer 割り当て (4MB, Windows のみ)                        │
│     ├→ GPixelFormats[] 初期化 (PF_* → DXGI_FORMAT マッピング)        │
│     ├→ GRHISupports* フラグ設定                                       │
│     │   ・GRHIGlobals.SupportsMultiDrawIndirect = true                │
│     │   ・GRHISupportsMultithreading = true                            │
│     │   ・GRHISupportsMultithreadedResources = true                    │
│     │   ・GRHISupportsAsyncGetRenderQueryResult = true                 │
│     │   ・GSupportsSeparateRenderTargetBlendState = true               │
│     │   ・GRHISupportsMSAADepthSampleAccess = true                     │
│     │   ・GRHISupportsArrayIndexFromAnyShader = true                   │
│     │   ・GRHISupportsRHIThread = true                                 │
│     │   ・GRHISupportsParallelRHIExecute = true                        │
│     │   ・GRHISupportsParallelRenderPasses = true                      │
│     │   ・GRHISupportsRawViewsForAnyBuffer = true                      │
│     │   ・GSupportsTimestampRenderQueries = true                       │
│     │   ・GSupportsParallelOcclusionQueries = true                     │
│     │   ・GRHISupportsRayTracingAsyncBuildAccelerationStructure = true │
│     │   ・GRHISupportsPipelineFileCache = PLATFORM_WINDOWS            │
│     │   ・GRHISupportsPSOPrecaching = PLATFORM_WINDOWS                │
│     │   ・GRHISupportsMapWriteNoOverwrite = true                       │
│     │   ・GRHISupportsFrameCyclesBubblesRemoval = true                 │
│     │   ・GRHISupportsGPUTimestampBubblesRemoval = true                │
│     │   ・GRHISupportsRHIOnTaskThread = true                           │
│     │   ・GRHIGlobals.NeedsShaderUnbinds = true                      │
│     │   ・GRHIGlobals.SupportsVertexShaderUAVs = true                 │
│     │   ・GRHIGlobals.NeedsExtraTransitions = true                   │
│     └→ テクスチャ上限設定                                              │
│         ・GMaxTextureDimensions = 16384 (D3D12_REQ_TEXTURE2D_*)       │
│         ・GMaxCubeTextureDimensions = 16384                           │
│         ・GMaxTextureArrayLayers = 2048                                │
│                                                                         │
│  5. FD3D12DynamicRHI::Init() [WindowsD3D12Device.cpp]                  │
│     ├→ PIX GPU Capturer DLL ロード (オプション)                       │
│     ├→ SetupD3D12Debug() — デバッグレイヤー設定                       │
│     ├→ ClearPSODriverCache() (コマンドライン指定時)                    │
│     ├→ AMD AGS 初期化 (AMD デバイス時)                                │
│     ├→ Adapter->InitializeDevices() [全アダプタ]                       │
│     │   ├→ D3D12 デバイス作成                                         │
│     │   ├→ FD3D12Device 生成 (ノードごと)                              │
│     │   └→ FD3D12Queue 生成 (Graphics/Compute/Copy)                   │
│     ├→ D3D12 ランタイムバックグラウンドスレッド無効化 (オプション)     │
│     ├→ NVIDIA NVAPI 初期化 (SER, ClusterOps, Atomic64)               │
│     ├→ Intel Extensions 初期化                                        │
│     ├→ Enhanced Barriers サポート検出                                  │
│     ├→ Ray Tracing サポート検出                                       │
│     ├→ Bindless Rendering サポート検出                                │
│     ├→ Mesh Shader / Work Graph サポート検出                          │
│     └→ InitializeSubmissionPipe()                                      │
│         ├→ SubmissionThread 起動                                       │
│         └→ InterruptThread 起動                                        │
│                                                                         │
│  6. FD3D12DynamicRHI::PostInit()                                       │
│     ├→ Adapter->InitializeExplicitDescriptorHeap()                     │
│     └→ Adapter->InitializeRayTracing() (RT サポート時)                 │
│                                                                         │
│  7. GIsRHIInitialized = true                                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.2 シャットダウンフロー

```
┌─────────────────────────────────────────────────────────────────────────┐
│              D3D12RHI シャットダウンシーケンス                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  FD3D12DynamicRHI::Shutdown()                                          │
│    ├→ GIsRHIInitialized = false                                        │
│    ├→ AMD AGS / Intel Extensions クリーンアップ                        │
│    ├→ FRenderResource::ReleaseRHIForAllResources()                     │
│    ├→ Adapter->CleanupResources() [全アダプタ]                         │
│    ├→ Adapter->BlockUntilIdle()   [全アダプタ]                         │
│    ├→ ImmediateFlush (RHIThread + Resources)                           │
│    ├→ RHIShutdownFlipTracking()                                        │
│    ├→ ShutdownSubmissionPipe()                                         │
│    │   └→ SubmissionThread / InterruptThread 停止                      │
│    ├→ ChosenAdapters.Empty() — デバイス/キュー/コンテキスト全削除      │
│    ├→ ZeroBuffer 解放                                                  │
│    └→ PIX DLL アンロード                                               │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 7. コンパイル時定義とフィーチャーフラグ

### 7.1 D3D12RHIDefinitions.h

| 定義 | デフォルト値 | 説明 |
|------|-------------|------|
| `DX_MAX_MSAA_COUNT` | `8` | MSAA 最大カウント |
| `RESIDENCY_PIPELINE_DEPTH` | `6` | レジデンシーパイプライン深度 |
| `DEBUG_RESOURCE_STATES` | `0` | リソースステートデバッグ |
| `D3D12RHI_IDLE_AFTER_EVERY_GPU_EVENT` | `0` | 毎 GPU イベント後にアイドル (デバッグ) |
| `D3D12_RHI_RAYTRACING` | `RHI_RAYTRACING` | レイトレーシング有効 |
| `D3D12_RHI_WORKGRAPHS` | `D3D12_MAX_COMMANDLIST_INTERFACE >= 10` | Work Graph 有効 |
| `D3D12_RHI_WORKGRAPHS_GRAPHICS` | `0` | Graphics Work Graph (無効) |
| `D3D12RHI_SUPPORTS_ENHANCED_BARRIERS` | `D3D12_MAX_DEVICE_INTERFACE >= 11 && ALLOW` | Enhanced Barriers |
| `D3D12RHI_SUPPORTS_LEGACY_BARRIERS` | `D3D12RHI_ALLOW_LEGACY_BARRIERS` | Legacy Barriers |
| `D3D12RHI_SUPPORTS_UNCOMPRESSED_UAV` | `D3D12_MAX_DEVICE_INTERFACE >= 12` | 非圧縮 UAV |
| `MAX_ROOT_CBVS` | `MAX_CBS (16)` | Root CBV 最大数 |
| `TRACK_RESOURCE_ALLOCATIONS` | `PLATFORM_WINDOWS && !UE_BUILD_SHIPPING && !UE_BUILD_TEST` | アロケーション追跡 |

### 7.2 WindowsD3D12RHIDefinitions.h (Windows固有)

| 定義 | 値 | 説明 |
|------|-----|------|
| `D3D12RHI_DEFAULT_NUM_BACKBUFFER` | `3` | デフォルトバックバッファ数 |
| `D3D12RHI_PLATFORM_COPY_COMMAND_LIST_TYPE` | `D3D12_COMMAND_LIST_TYPE_COPY` | コピーキューのコマンドリスト型 |
| `FD3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER` | `D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER` | サンプラーヒープ型 |
| `D3D12RHI_PLATFORM_USES_TIMESTAMP_QUERIES` | `1` | タイムスタンプクエリ使用 |
| `D3D12RHI_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER` | `D3D12_RESOURCE_FLAG_NONE` | Indirect バッファフラグ |
| `D3D12RHI_NEEDS_VENDOR_EXTENSIONS` | `1` | ベンダー拡張必要 |
| `D3D12RHI_NEEDS_SHADER_FEATURE_CHECKS` | `1` | シェーダーフィーチャーチェック |
| `USE_STATIC_ROOT_SIGNATURE` | `0` | 静的 Root Signature 使用 |
| `D3D12RHI_USE_CONSTANT_BUFFER_VIEWS` | `0` | CBV 使用 (Root Descriptor 優先) |
| `D3D12RHI_USE_DUMMY_BACKBUFFER` | `1` | ダミーバックバッファ使用 |
| `D3D12RHI_USE_DXGI_COLOR_SPACE` | `1` | DXGI カラースペース使用 |
| `D3D12RHI_ALLOW_ENHANCED_BARRIERS` | `1` | Enhanced Barriers 許可 |
| `D3D12RHI_ALLOW_LEGACY_BARRIERS` | `1` | Legacy Barriers 許可 |

### 7.3 バッファプールサイズ

```
┌─────────────────────────────────────────────────────────────────────────┐
│              バッファプールサイズ定義 (D3D12RHIDefinitions.h)            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  【レイトレーシング有効時】                                               │
│  ・DEFAULT_BUFFER_POOL_MAX_ALLOC_SIZE    = 64 MB                       │
│  ・DEFAULT_BUFFER_POOL_DEFAULT_POOL_SIZE = 16 MB                       │
│  ※ CreateCommittedResource() 呼び出し削減のため大きめ                  │
│                                                                         │
│  【レイトレーシング無効時】                                               │
│  ・DEFAULT_BUFFER_POOL_MAX_ALLOC_SIZE    = 64 KB                       │
│  ・DEFAULT_BUFFER_POOL_DEFAULT_POOL_SIZE =  8 MB                       │
│  ※ D3D12 バッファは 64KB アラインメント                                │
│                                                                         │
│  【共通】                                                                │
│  ・READBACK_BUFFER_POOL_MAX_ALLOC_SIZE    = 64 KB                     │
│  ・READBACK_BUFFER_POOL_DEFAULT_POOL_SIZE =  4 MB                     │
│  ・TEXTURE_POOL_SIZE                      =  8 MB                     │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 7.4 ED3D12Access (D3D12Access.h)

```
┌─────────────────────────────────────────────────────────────────────────┐
│              ED3D12Access — D3D12 リソースアクセスフラグ                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ERHIAccess からの直接マッピング:                                       │
│  ・Unknown, CPURead, Present, IndirectArgs                             │
│  ・VertexOrIndexBuffer, SRVCompute, SRVGraphicsPixel                   │
│  ・SRVGraphicsNonPixel, CopySrc, ResolveSrc, DSVRead                  │
│  ・UAVCompute, UAVGraphics, RTV, CopyDest                             │
│  ・ResolveDst, DSVWrite, BVHRead, BVHWrite                            │
│  ・Discard, ShadingRateSource                                          │
│                                                                         │
│  D3D12 固有拡張:                                                       │
│  ・Common      = RHIAccessLast << 1  (D3D12 共通ステート)              │
│  ・GenericRead = RHIAccessLast << 2  (全読み取りステート統合)           │
│                                                                         │
│  複合マスク:                                                            │
│  ・SRVMask, UAVMask, ReadOnlyMask, WritableMask 等                    │
│  ・ReadOnlyExclusiveMask に GenericRead を含む (BVHRead を除外)        │
│                                                                         │
│  検証関数:                                                              │
│  ・IsValidAccess() — 読み取り専用と書き込みの同時指定を検出             │
│  ・Common は単独でのみ使用可                                            │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 8. CVar概要テーブル

### 8.1 D3D12RHI.cpp 内で定義される CVar

| CVar 名 | 型 | デフォルト | 説明 |
|---------|-----|----------|------|
| `d3d12.BindResourceLabels` | int32 | `1` | D3D12 リソースへのデバッグ名バインド有効化 |
| `r.D3D12.Depth24Bit` | int32 | `0` | 0: 32bit float 深度バッファ, 1: 24bit fixed point (ReadOnly) |
| `D3D12.ZeroBufferSizeInMB` | int32 | `4` | テクスチャストリーミング用ゼロバッファサイズ (MB) (ReadOnly) |
| `r.PSOPrecache.D3D12.DriverCacheAware` | bool | `true` | PSO プリキャッシュでドライバキャッシュ類似性を考慮 (ReadOnly) |

### 8.2 コマンドラインフラグ

| フラグ | 説明 |
|--------|------|
| `-warp` | WARP ソフトウェアアダプタ使用 |
| `-AllowSoftwareRendering` | ソフトウェアレンダリングフォールバック許可 |
| `-nod3dasync` | 非同期リソース作成を無効化 |
| `-d3dcompat` / `-d3d12compat` | 新しい D3D12 機能の使用を抑制 |
| `-attachPIX` | PIX GPU キャプチャ DLL を自動ロード |
| `-clearPSODriverCache` | PSO ドライバキャッシュをクリア |

### 8.3 Windows プラットフォーム固有定数 (D3D12RHI.h)

| 定義 | 値 | 説明 |
|------|-----|------|
| `D3D12RHI_PLATFORM_HAS_UNIFIED_MEMORY` | `0` | UMA (Unified Memory Architecture) なし |
| `ENABLE_RESIDENCY_MANAGEMENT` | `1` | レジデンシー管理有効 |
| `PIPELINE_STATE_FILE_LOCATION` | `FPaths::ProjectSavedDir()` | PSO キャッシュファイル保存先 |
| `USE_PIX` | `WITH_PIX_EVENT_RUNTIME` | PIX イベント使用 |
| `FD3D12_TEXTURE_DATA_PITCH_ALIGNMENT` | `D3D12_TEXTURE_DATA_PITCH_ALIGNMENT` | テクスチャデータピッチアラインメント |

---

## 補足: TD3D12ObjectPool

```
┌─────────────────────────────────────────────────────────────────────────┐
│  TD3D12ObjectPool<TObjectType> (D3D12RHICommon.h)                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  TLockFreePointerListUnordered を継承したオブジェクトプール。           │
│                                                                         │
│  ・ロックフリーなポインタリスト (PLATFORM_CACHE_LINE_SIZE アライン)    │
│  ・デストラクタで残存オブジェクトを自動削除 (CleanupResources)         │
│  ・コマンドアロケータ等の再利用可能オブジェクトのプーリングに使用       │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 補足: FScopedResourceBarrier (D3D12RHIPrivate.h)

```
┌─────────────────────────────────────────────────────────────────────────┐
│  FScopedResourceBarrier                                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  RAII スタイルのリソースバリアヘルパー。                                 │
│                                                                         │
│  ・コンストラクタで Before → Desired のバリアを発行                    │
│  ・デストラクタで Desired → Before の逆バリアを発行 (状態復元)        │
│  ・追跡/非追跡リソース両対応                                            │
│  ・Before が Unknown の場合、リソースのデフォルトアクセスを使用         │
│  ・RequiresResourceStateTracking() == false の場合でも動作              │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 補足: ログカテゴリ

```
DECLARE_LOG_CATEGORY_EXTERN(LogD3D12RHI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogD3D12GapRecorder, Log, All);
```

## 補足: 統計グループ (D3D12Stats.h)

| グループ名 | 説明 |
|-----------|------|
| `STATGROUP_D3D12RHI` | 一般的な D3D12RHI 統計 |
| `STATGROUP_D3D12Memory` | メモリ統計 |
| `STATGROUP_D3D12MemoryDetails` | メモリ詳細統計 |
| `STATGROUP_D3D12Resources` | リソース統計 (RT, UAV, Buffer, Texture) |
| `STATGROUP_D3D12Bindless` | Bindless ディスクリプタ統計 |
| `STATGROUP_D3D12BufferDetails` | バッファ詳細統計 |
| `STATGROUP_D3D12PipelineState` | PSO 統計 |
| `STATGROUP_D3D12DescriptorHeap` | GPU Visible ディスクリプタヒープ統計 |

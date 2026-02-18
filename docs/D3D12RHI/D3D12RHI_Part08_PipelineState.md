# D3D12RHI Part 8: パイプラインステートとルートシグネチャ

## 目次
1. [概要](#1-概要)
2. [コンパイル時フラグとキャッシュ構成](#2-コンパイル時フラグとキャッシュ構成)
3. [ルートシグネチャ定義 (D3D12RootSignatureDefinitions.h)](#3-ルートシグネチャ定義)
4. [FD3D12RootSignatureDesc — ルートシグネチャ記述構築](#4-fd3d12rootsignaturedesc)
5. [FD3D12RootSignature — ルートシグネチャオブジェクト](#5-fd3d12rootsignature)
6. [FD3D12RootSignatureManager — ルートシグネチャ管理](#6-fd3d12rootsignaturemanager)
7. [PSO記述構造体](#7-pso記述構造体)
8. [PSO Stream アーキテクチャ](#8-pso-stream-アーキテクチャ)
9. [FD3D12PipelineState と非同期PSO生成](#9-fd3d12pipelinestate-と非同期pso生成)
10. [FD3D12PipelineStateCacheBase — Low-Level キャッシュ](#10-fd3d12pipelinestatecachebase)
11. [FD3D12PipelineStateCache — ディスクキャッシュとPipeline Library](#11-fd3d12pipelinestatecache)
12. [FD3D12StateCache — ステートキャッシュ](#12-fd3d12statecache)
13. [CVar一覧](#13-cvar一覧)

---

## 1. 概要

D3D12RHI のパイプラインステート管理は、D3D12 の**不変PSO (Pipeline State Object)** モデルに
基づき、ルートシグネチャとPSOの2層で構成される。

```
+------------------------------------------------------------------+
|                     FD3D12DynamicRHI                              |
+------------------------------------------------------------------+
|  FD3D12Adapter                                                   |
|  +------------------------------------------------------------+  |
|  | FD3D12RootSignatureManager                                  |  |
|  |   QBSS → FD3D12RootSignature マップ (FCriticalSection保護)  |  |
|  +------------------------------------------------------------+  |
|  | FD3D12PipelineStateCache (Windows固有)                      |  |
|  |   ┌─ Low-Level Graphics Cache (CombinedHash → PSO)         |  |
|  |   ├─ Low-Level Compute Cache  (CombinedHash → PSO)         |  |
|  |   ├─ DiskCaches[2] (Graphics/Compute)                      |  |
|  |   ├─ DiskBinaryCache (ドライバ最適化ブロブ)                 |  |
|  |   └─ ID3D12PipelineLibrary (ドライバ管理PSO辞書)            |  |
|  +------------------------------------------------------------+  |
|  FD3D12Device                                                    |
|  +------------------------------------------------------------+  |
|  | FD3D12CommandContext                                         |  |
|  |   └─ FD3D12StateCache                                       |  |
|  |       ├─ Graphics / Compute / Common ステート               |  |
|  |       ├─ ダーティビット追跡 (bNeedSet*)                     |  |
|  |       └─ FD3D12DescriptorCache                              |  |
|  +------------------------------------------------------------+  |
+------------------------------------------------------------------+
```

**設計原則:**
- **High-Level PSO Cache**: `D3D12RHI_USE_HIGH_LEVEL_PSO_CACHE = 0` でデフォルト無効
- **D3D12_USE_DERIVED_PSO**: `0` でデフォルト無効 (Derived PSO最適化は非使用)
- ルートシグネチャは `FD3D12QuantizedBoundShaderState (QBSS)` から一意に決定・重複排除
- PSO生成は `FAsyncTask<FD3D12PipelineStateWorker>` による非同期生成をサポート（現在は同期のみ使用）

---

## 2. コンパイル時フラグとキャッシュ構成

```
┌─────────────────────────────────────────┬───────┬─────────────────────────────────────────┐
│ フラグ                                  │デフォ │ 説明                                    │
│                                         │ルト   │                                         │
├─────────────────────────────────────────┼───────┼─────────────────────────────────────────┤
│ D3D12RHI_USE_HIGH_LEVEL_PSO_CACHE       │   0   │ Initializer→PSO マップ (無効)            │
│ D3D12_USE_DERIVED_PSO                   │   0   │ Derived PSO最適化 (無効)                 │
│ D3D12_USE_DERIVED_PSO_SHADER_EXPORTS    │   0   │ ShaderExport付きDerived PSO              │
│ D3D12RHI_USE_ASYNC_PRELOAD              │   0   │ 非同期PSOプリロード (バグにより無効)      │
│ D3D12RHI_USE_D3DDISASSEMBLE             │   1   │ PSO障害時シェーダーダンプ                 │
│ D3D12_STATE_CACHE_RUNTIME_TOGGLE        │   0   │ TOGGLESTATECACHE コンソールコマンド       │
│ D3D12RHI_NEEDS_VENDOR_EXTENSIONS        │ 環境  │ ベンダー拡張PSO生成パス                  │
│ D3D12RHI_USE_CONSTANT_BUFFER_VIEWS      │ 環境  │ CBVディスクリプタテーブル使用             │
│ PLATFORM_SUPPORTS_MESH_SHADERS          │ 環境  │ Mesh/Amplification シェーダー対応         │
└─────────────────────────────────────────┴───────┴─────────────────────────────────────────┘
```

### PSO キャッシュ階層

```
  RHICreateGraphicsPipelineState()
          │
          ▼
  ┌─ High-Level Cache ─┐   D3D12RHI_USE_HIGH_LEVEL_PSO_CACHE = 0
  │ (Initializer→PSO)  │   → 現在無効、スキップ
  └────────┬───────────┘
           ▼
  ┌─ Low-Level Cache ──┐   CombinedHash → FD3D12PipelineState*
  │ FindInLowLevelCache│   TMap (FRWLock 保護)
  │ Hit → AddRef       │
  │ Miss → Create+Add  │
  └────────┬───────────┘
           ▼
  ┌─ OnPSOCreated ─────┐
  │ Create(同期)        │   FORT-243931: 非同期は信頼性問題で無効
  │ AddToDiskCache      │   シリアライズ保存
  └────────────────────┘
```

---

## 3. ルートシグネチャ定義

**ファイル:** `D3D12RootSignatureDefinitions.h` (Public/)

`D3D12ShaderUtils` 名前空間に、ルートシグネチャの静的構築ロジックが集約されている。

### 3.1 ディスクリプタ範囲フラグ定数

```
┌───────────────────────┬─────────────────────────────────────────────────────┐
│ リソース種別          │ デフォルト D3D12_DESCRIPTOR_RANGE_FLAGS             │
├───────────────────────┼─────────────────────────────────────────────────────┤
│ SRV                   │ DATA_STATIC_WHILE_SET_AT_EXECUTE │                 │
│                       │ DESCRIPTORS_VOLATILE                               │
│ CBV                   │ DATA_STATIC_WHILE_SET_AT_EXECUTE │                 │
│                       │ DESCRIPTORS_VOLATILE                               │
│ UAV                   │ DATA_VOLATILE │ DESCRIPTORS_VOLATILE               │
│ Sampler               │ DESCRIPTORS_VOLATILE                               │
├───────────────────────┼─────────────────────────────────────────────────────┤
│ Root CBV              │ DATA_STATIC (アップロードヒープから常にセット)       │
└───────────────────────┴─────────────────────────────────────────────────────┘
```

**注:** `DESCRIPTORS_VOLATILE` は、ディスクリプタテーブル内の全エントリを初期化しない
UE5の運用に対応するため必須。

### 3.2 FRootSignatureCreator — Strategy パターン

```
  FRootSignatureCreator (抽象基底)
      │ Flags: EShaderBindingLayoutFlags
      │ RegisterSpace: uint32
      │
      ├─ FBinaryRootSignatureCreator ─── D3D12 API 用バイナリ記述生成
      │    RootFlags, Parameters[], DescriptorRanges[], PendingTables[]
      │    Finalize() → D3D12_VERSIONED_ROOT_SIGNATURE_DESC
      │
      └─ FTextRootSignatureCreator ──── DXC コンパイル用テキスト生成
           RootFlags: FString, Table: FString
           GenerateString() → HLSL root signature 文字列
```

### 3.3 標準ルートシグネチャ構成

**Graphics ルートシグネチャ (CreateGfxRootSignature):**
```
  [0] UAV Root Descriptor  ─── u0, space=UE_HLSL_SPACE_DIAGNOSTIC (診断バッファ)
  [1] Root Constants (opt)  ── 4 × uint32, space=UE_HLSL_SPACE_SHADER_ROOT_CONSTANTS
  ─── 以下はディスクリプタテーブル ───
  [2] PS SRV table (t0..MAX_SRVS)
  [3] PS CBV table (b0..MAX_CBS)
  [4] PS Sampler table (s0..MAX_SAMPLERS)
  [5] VS SRV table
  [6] VS CBV table
  [7] VS Sampler table
  [8] GS SRV table
  [9] GS CBV table
  [10] GS Sampler table
  [11] MS SRV table  (AllowMeshShaders時)
  [12] MS CBV table
  [13] MS Sampler table
  [14] AS SRV table
  [15] AS CBV table
  [16] AS Sampler table
  [17] PS UAV table (u0..MAX_UAVS)
  [18] VS UAV table
```

**Compute ルートシグネチャ (CreateComputeRootSignature):**
```
  [0] UAV Root Descriptor (診断バッファ)
  [1] Root Constants (opt)
  [2] ALL SRV table
  [3] ALL CBV table
  [4] ALL Sampler table
  [5] ALL UAV table
```

**Bindless 時の変更:**
- `BindlessResources` → SRV/UAV テーブルをスキップ + `CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED` フラグ
- `BindlessSamplers` → Sampler テーブルをスキップ + `SAMPLER_HEAP_DIRECTLY_INDEXED` フラグ

### 3.4 レイトレーシングルートシグネチャ

```
  ┌─ Local Root Signature (ヒットグループ) ──────────────────────┐
  │ Bindless:                                                     │
  │   [0] Root Constants ─── FD3D12HitGroupSystemParameters       │
  │       (8 × uint32 = 32 bytes)                                 │
  │       ・IndexBufferConfig, IndexBufferOffset                   │
  │       ・FirstPrimitive, HitGroupUserData                       │
  │       ・BindlessIndexBuffer, BindlessVertexBuffer + padding    │
  │                                                                │
  │ Non-Bindless:                                                  │
  │   [0] Root Constants ─── FHitGroupSystemRootConstants          │
  │       (4 × uint32 = 16 bytes)                                 │
  │   [1] SRV Root Descriptor ─── IndexBuffer (raw buffer)        │
  │   [2] SRV Root Descriptor ─── VertexBuffer (raw buffer)       │
  │                                                                │
  │ + 通常の SRV/CBV/Sampler/UAV テーブル                          │
  │ FLAG: D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE           │
  └───────────────────────────────────────────────────────────────┘

  ┌─ Global Root Signature ──────────────────────────────────────┐
  │ ShaderBindingLayout 使用時: 静的UB用 CBV Root Descriptors     │
  │ Bindless時: MAX_CBS個の CBV Root Descriptors                  │
  │ 非Bindless時: 標準 SRV/CBV/Sampler テーブル + UAV テーブル   │
  │ FLAG: FD3D12_ROOT_SIGNATURE_FLAG_GLOBAL_ROOT_SIGNATURE        │
  └───────────────────────────────────────────────────────────────┘
```

---

## 4. FD3D12RootSignatureDesc

**ファイル:** `D3D12RootSignature.cpp`

`FD3D12QuantizedBoundShaderState (QBSS)` からルートパラメータレイアウトを構築する。

### 4.1 ルートパラメータコスト (DWORD単位)

```
┌──────────────────────────┬──────────┬──────────────────────────────────┐
│ パラメータ種別           │ コスト   │ 備考                             │
├──────────────────────────┼──────────┼──────────────────────────────────┤
│ Descriptor Table (Global)│ 1 DWORD  │ GPU ポインタ                     │
│ Descriptor Table (Local) │ 2 DWORD  │ ローカルRS用 (文書化されていない)│
│ Root Constant            │ 1 DWORD  │ 1定数 = 1 DWORD                  │
│ Root Descriptor (CBV等)  │ 2 DWORD  │ 64-bit GPU 仮想アドレス          │
├──────────────────────────┼──────────┼──────────────────────────────────┤
│ 最大ルートパラメータ数   │ 32       │ MaxRootParameters                │
│ 最大ルートシグネチャサイズ│ 64 DWORD │ RootParametersSize 制限          │
│ 性能警告閾値             │ 12 DWORD │ Raster RS で超過時 Verbose ログ  │
└──────────────────────────┴──────────┴──────────────────────────────────┘
```

### 4.2 パラメータ配置順序

```
  優先順位 (外側ループ: パラメータ種別):
    1. D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE
    2. D3D12_ROOT_PARAMETER_TYPE_CBV

  優先順位 (内側ループ: シェーダー可視性):
    1. SV_Pixel        (PS 最優先)
    2. SV_Vertex
    3. SV_Geometry
    4. SV_Mesh          (PLATFORM_SUPPORTS_MESH_SHADERS 時)
    5. SV_Amplification
    6. SV_All
```

各ステージについて、QBSS の `RegisterCounts` に基づき:
- **SRV > 0** → SRV ディスクリプタテーブル追加
- **CBV > MAX_ROOT_CBVS** → 超過分を CBV ディスクリプタテーブルに配置
- **Sampler > 0** → Sampler ディスクリプタテーブル追加
- **UAV > 0** → UAV ディスクリプタテーブル追加
- **CBV ≤ MAX_ROOT_CBVS** → Root CBV として配置 (コスト: 2 DWORD/個)

### 4.3 Resource Binding Tier によるフラグ調整

```
┌──────────────┬─────────────────────────────────────┬──────────────────────┐
│ Tier         │ DESCRIPTORS_VOLATILE                │ 影響                 │
├──────────────┼─────────────────────────────────────┼──────────────────────┤
│ Tier 1       │ SRV: なし, CBV: なし, Sampler: なし, │ 全テーブル要 full set│
│              │ UAV: DATA_VOLATILEのみ              │                      │
│ Tier 2       │ SRV: あり, Sampler: あり,           │ SRV/Sampler partial  │
│              │ CBV: なし, UAV: DATA_VOLATILEのみ   │ OK                   │
│ Tier 3       │ 全種: DATA_VOLATILE+DESCRIPTORS_    │ 全 partial OK        │
│              │ VOLATILE                            │                      │
└──────────────┴─────────────────────────────────────┴──────────────────────┘
```

### 4.4 静的サンプラー

```
  space=1000 に6個の静的サンプラーを配置 (D3DCommon.ush と対応):
  ┌──────┬──────────────────────────────┬──────────────────────────────┐
  │ Reg  │ Filter                       │ AddressMode                  │
  ├──────┼──────────────────────────────┼──────────────────────────────┤
  │ s0   │ MIN_MAG_MIP_POINT            │ WRAP                         │
  │ s1   │ MIN_MAG_MIP_POINT            │ CLAMP                        │
  │ s2   │ MIN_MAG_LINEAR_MIP_POINT     │ WRAP                         │
  │ s3   │ MIN_MAG_LINEAR_MIP_POINT     │ CLAMP                        │
  │ s4   │ MIN_MAG_MIP_LINEAR           │ WRAP                         │
  │ s5   │ MIN_MAG_MIP_LINEAR           │ CLAMP                        │
  └──────┴──────────────────────────────┴──────────────────────────────┘
  ※ Tier 1 では静的サンプラー使用不可 (16サンプラー制限)
  ※ Local Root Signature では静的サンプラー不使用 (Global RS と重複回避)
```

### 4.5 追加パラメータ

構築の後半で、条件に応じて追加パラメータを配置:

| 条件 | パラメータ | 詳細 |
|------|-----------|------|
| `bNeedsAgsIntrinsicsSpace` | UAV Root Descriptor | AMD AGS intrinsics space |
| `ShaderBindingLayout > 0` | CBV Root Descriptor × N | 静的シェーダーバインディング |
| `bUseRootConstants` | 32bit Constants × 4 | UERootConstants |
| NVAPI + RT Global | UAV Descriptor Table | NV shader extension slot |
| `bUseDiagnosticBuffer` | UAV Root Descriptor | シェーダー診断バッファ |

---

## 5. FD3D12RootSignature

**ファイル:** `D3D12RootSignature.h`, `D3D12RootSignature.cpp`

### 5.1 ERootParameterKeys

```
  enum ERootParameterKeys
  {
    // PS: 4キー
    PS_SRVs, PS_CBVs, PS_RootCBVs, PS_Samplers,
    // VS: 5キー (UAV含む)
    VS_SRVs, VS_CBVs, VS_RootCBVs, VS_Samplers, VS_UAVs,
    // GS: 4キー
    GS_SRVs, GS_CBVs, GS_RootCBVs, GS_Samplers,
    // MS: 4キー
    MS_SRVs, MS_CBVs, MS_RootCBVs, MS_Samplers,
    // AS: 4キー
    AS_SRVs, AS_CBVs, AS_RootCBVs, AS_Samplers,
    // ALL: 5キー (UAV含む)
    ALL_SRVs, ALL_CBVs, ALL_RootCBVs, ALL_Samplers, ALL_UAVs,
    RPK_RootParameterKeyCount  // = 26
  };
  // ※ PS_UAVsは存在しない。PS UAVはALL_UAVsにマッピングされる
```

### 5.2 クラス構造

```
  FD3D12RootSignature : FD3D12AdapterChild
  ┌──────────────────────────────────────────────────────────┐
  │ ID3D12RootSignature*                                     │
  │ ID3DBlob* RootSignatureBlob                              │
  │                                                          │
  │ BindSlotMap[RPK_RootParameterKeyCount]                   │
  │   → ルートパラメータインデックスマップ                   │
  │                                                          │
  │ BindSlotOffsetsInDWORDs[MaxRootParameters]               │
  │   → 各パラメータのDWORDオフセット                        │
  │                                                          │
  │ TotalRootSignatureSizeInDWORDs                           │
  │                                                          │
  │ Stage[SF_NumStandardFrequencies]                         │
  │   bVisible, MaxSRVCount, MaxUAVCount, MaxCBVCount,       │
  │   MaxSamplerCount, CBVRegisterMask                       │
  │                                                          │
  │ bHasUAVs, bHasSRVs, bHasCBVs, bHasRootCBs, bHasSamplers│
  │ bUsesDynamicResources, bUsesDynamicSamplers (Bindless)   │
  │                                                          │
  │ RootConstantsSlot: int8                                  │
  │ StaticShaderBindingSlot: int8                            │
  │ StaticShaderBindingCount: int8                           │
  │ DiagnosticBufferSlot: int8                               │
  └──────────────────────────────────────────────────────────┘
```

### 5.3 Init フロー

```
  FD3D12RootSignature::Init(QBSS)
    │
    ├─ FD3D12RootSignatureDesc(QBSS, BindingTier) ─── パラメータ配置
    │
    ├─ Init(VersionedDesc, RootSignatureType)
    │   ├─ D3DX12SerializeVersionedRootSignature() ─── ブロブ生成
    │   ├─ ID3D12Device::CreateRootSignature()
    │   └─ AnalyzeSignature() → InternalAnalyzeSignature<>()
    │       ├─ 各パラメータを走査
    │       ├─ BindSlotMap[] に RPK → ルートパラメータ番号を格納
    │       ├─ Stage[] に MaxSRV/UAV/CBV/SamplerCount を設定
    │       ├─ BindSlotOffsetsInDWORDs[] にDWORDオフセットを計算
    │       └─ TotalRootSignatureSizeInDWORDs を集計
    │
    └─ 特殊スロット情報を FD3D12RootSignatureDesc から継承
        RootConstantsSlot, StaticShaderBindingSlot, DiagnosticBufferSlot
```

### 5.4 静的ルートシグネチャ初期化

Shader Binding Layout 非使用時に `D3D12ShaderUtils` の静的構築関数を使用:

| メソッド | 用途 |
|---------|------|
| `InitStaticGraphicsRootSignature()` | Gfx RS を FBinaryRootSignatureCreator で構築 |
| `InitStaticComputeRootSignatureDesc()` | Compute RS を同様に構築 |
| `InitStaticRayTracingGlobalRootSignatureDesc()` | RT Global RS |
| `InitStaticRayTracingLocalRootSignatureDesc()` | RT Local RS |

各初期化後、`Creator.Parameters` を走査して `RootConstantsSlot` と `DiagnosticBufferSlot` を検出。

### 5.5 ERootSignatureType と Binding Space

```
┌──────────────────────────┬───────────────────────────────────────┐
│ ERootSignatureType       │ Binding Space                         │
├──────────────────────────┼───────────────────────────────────────┤
│ RS_Raster                │ UE_HLSL_SPACE_DEFAULT (0)             │
│ RS_RayTracingGlobal      │ UE_HLSL_SPACE_RAY_TRACING_GLOBAL      │
│ RS_RayTracingLocal       │ UE_HLSL_SPACE_RAY_TRACING_LOCAL       │
│ RS_WorkGraphGlobal       │ UE_HLSL_SPACE_WORK_GRAPH_GLOBAL       │
│ RS_WorkGraphLocalCompute │ UE_HLSL_SPACE_WORK_GRAPH_LOCAL        │
│ RS_WorkGraphLocalRaster  │ Mesh: WORK_GRAPH_LOCAL                │
│                          │ Pixel: WORK_GRAPH_LOCAL_PIXEL         │
└──────────────────────────┴───────────────────────────────────────┘
```

---

## 6. FD3D12RootSignatureManager

**ファイル:** `D3D12RootSignature.h`, `D3D12RootSignature.cpp`

```
  FD3D12RootSignatureManager : FD3D12AdapterChild
  ┌──────────────────────────────────────────────────────────┐
  │ TMap<FD3D12QuantizedBoundShaderState, FD3D12RootSignature*>│
  │ FCriticalSection CS ─── スレッドセーフ (BSS生成は並列)   │
  │                                                          │
  │ GetRootSignature(QBSS)                                   │
  │   → Map検索、ヒットなしで CreateRootSignature()          │
  │                                                          │
  │ GetQuantizedBoundShaderState(RS*)                         │
  │   → 逆引き (ディスクキャッシュ保存時に使用)              │
  │                                                          │
  │ Destroy()                                                │
  │   → 全 RootSignature を delete                           │
  └──────────────────────────────────────────────────────────┘
```

QBSS は `FShaderRegisterCounts` (SRV/CBV/UAV/Sampler の各カウント) を
各可視性ステージごとに保持し、これがルートシグネチャの一意キーとなる。

---

## 7. PSO記述構造体

**ファイル:** `D3D12PipelineState.h`

### 7.1 FD3D12_GRAPHICS_PIPELINE_STATE_DESC

```
  ┌──────────────────────────────────────────────────────────┐
  │ ID3D12RootSignature* pRootSignature                      │
  │ D3D12_SHADER_BYTECODE VS, MS, AS, PS, GS                │
  │ D3D12_BLEND_DESC BlendState                              │
  │ uint32 SampleMask                                        │
  │ D3D12_RASTERIZER_DESC RasterizerState                    │
  │ D3D12_DEPTH_STENCIL_DESC1 DepthStencilState              │
  │ D3D12_INPUT_LAYOUT_DESC InputLayout                      │
  │ D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue       │
  │ D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType       │
  │ D3D12_RT_FORMAT_ARRAY RTFormatArray                      │
  │ DXGI_FORMAT DSVFormat                                    │
  │ DXGI_SAMPLE_DESC SampleDesc                              │
  │ uint32 NodeMask                                          │
  │ D3D12_CACHED_PIPELINE_STATE CachedPSO                    │
  │ D3D12_PIPELINE_STATE_FLAGS Flags                         │
  │                                                          │
  │ PipelineStateStream() → FD3D12_GRAPHICS_PIPELINE_STATE_STREAM │
  │ MeshPipelineStateStream() → FD3D12_MESH_PIPELINE_STATE_STREAM │
  │ UsesMeshShaders() → Desc.MS.BytecodeLength > 0           │
  └──────────────────────────────────────────────────────────┘
```

### 7.2 Low-Level 記述ラッパー

```
  FD3D12LowLevelGraphicsPipelineStateDesc
  ┌─────────────────────────────────────────┐
  │ FD3D12_GRAPHICS_PIPELINE_STATE_DESC Desc│
  │ const FD3D12RootSignature* pRootSignature│  (UE側参照)
  │ ShaderBytecodeHash VSHash, MSHash, ...  │
  │ uint32 InputLayoutHash (ストライド除外) │
  │ uint64 CombinedHash ── CityHash64       │
  │ bool bFromPSOFileCache                  │
  │ VendorExtensions* (各シェーダー)        │
  └─────────────────────────────────────────┘

  FD3D12ComputePipelineStateDesc
  ┌─────────────────────────────────────────┐
  │ FD3D12_COMPUTE_PIPELINE_STATE_DESC Desc │
  │ const FD3D12RootSignature* pRootSignature│
  │ ShaderBytecodeHash CSHash               │
  │ uint64 CombinedHash                     │
  │ VendorExtensions*                       │
  └─────────────────────────────────────────┘
```

### 7.3 CombinedHash 計算

`FD3D12PipelineStateCacheBase::HashPSODesc()` で CityHash64 を使用:

**Graphics PSO Hash に含まれるフィールド:**
- シェーダーバイトコードハッシュ (VS/MS/AS/GS/PS)
- InputLayoutHash
- BlendState (AlphaToCoverage, IndependentBlend, 各RT BlendDesc)
- SampleMask, RasterizerState, DepthStencilState (`!D3D12_USE_DERIVED_PSO` 時)
- IBStripCutValue, PrimitiveTopologyType
- DSVFormat, SampleDesc, NodeMask, Flags
- RT Format + BlendDesc (NumRenderTargets分のみ)

**Compute PSO Hash:**
- CSHash, NodeMask, Flags のみ

---

## 8. PSO Stream アーキテクチャ

**ファイル:** `WindowsD3D12PipelineState.h`

D3D12 の Pipeline State Stream API (`ID3D12Device2::CreatePipelineState`) を使用。

### 8.1 ストリーム構造体

```
  FD3D12_GRAPHICS_PIPELINE_STATE_STREAM
  ┌──────────────────────────────────────────┐
  │ CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK  │
  │ CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE│
  │ CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT│
  │ CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE│
  │ CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY│
  │ CD3DX12_PIPELINE_STATE_STREAM_VS         │
  │ CD3DX12_PIPELINE_STATE_STREAM_GS         │
  │ CD3DX12_PIPELINE_STATE_STREAM_HS         │  (未使用だが定義)
  │ CD3DX12_PIPELINE_STATE_STREAM_DS         │  (未使用だが定義)
  │ CD3DX12_PIPELINE_STATE_STREAM_PS         │
  │ CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC │
  │ CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1│
  │ CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT│
  │ CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER │
  │ CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS│
  │ CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC│
  │ CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK│
  │ CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO │
  └──────────────────────────────────────────┘

  FD3D12_MESH_PIPELINE_STATE_STREAM ─── InputLayout なし、VS/GS → MS/AS
  FD3D12_COMPUTE_PIPELINE_STATE_STREAM ─── CS + NodeMask + CachedPSO のみ
```

### 8.2 PSO 生成パイプライン

```
  CreateGraphicsPipelineState()
    │
    ├─ HasVendorExtensions?
    │   ├─ Yes → CreatePipelineStateWithExtensions()
    │   │         各拡張ベンダーに応じた前処理
    │   │         ├─ NVIDIA: NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread()
    │   │         ├─ AMD AGS: パススルー
    │   │         └─ Intel: パススルー
    │   │
    │   └─ No → CreatePipelineStateWrapper()
    │
    └─ CreatePipelineStateWrapper()
        ├─ FastHashName(CombinedHash) → wchar_t[17] PSO名
        ├─ UsesMeshShaders()?
        │   ├─ Yes → MeshPipelineStateStream()
        │   └─ No  → PipelineStateStream()
        │
        └─ CreatePipelineStateFromStream()
            ├─ PipelineLibrary 有?
            │   ├─ Library->LoadPipeline(Name, ...) ── ライブラリ検索
            │   │   ├─ E_INVALIDARG → Device->CreatePipelineState() + Library->StorePipeline()
            │   │   └─ Success → 再利用
            │   └─ No Library
            │       └─ Device->CreatePipelineState()
            │
            └─ 失敗時: DumpGraphicsPSO() / DumpComputePSO()
                       シェーダーアセンブリダンプ (d3dcompiler_47.dll)
```

---

## 9. FD3D12PipelineState と非同期PSO生成

### 9.1 FD3D12PipelineState

```
  FD3D12PipelineState : FD3D12AdapterChild, FD3D12MultiNodeGPUObject
  ┌──────────────────────────────────────────────────────────┐
  │ TRefCountPtr<ID3D12PipelineState> PipelineState          │
  │ FAsyncTask<FD3D12PipelineStateWorker>* Worker            │
  │ PSOInitState InitState {Uninitialized, Initialized,      │
  │                         CreationFailed}                  │
  │ FRWLock GetPipelineStateMutex                            │
  │                                                          │
  │ Create(GraphicsPipelineCreationArgs) ─── 同期生成        │
  │ Create(ComputePipelineCreationArgs)  ─── 同期生成        │
  │ CreateAsync(...)  ─── FAsyncTask でバックグラウンド生成  │
  │                                                          │
  │ GetPipelineState() / InternalGetPipelineState()          │
  │   FRWScopeLock(Write)                                    │
  │   Worker完了待ち → PipelineState取得 → Worker削除        │
  │   Worker無しでUninitialized → Busy-wait (ログ警告付き)   │
  │                                                          │
  │ UsePSORefCounting() ─── static                           │
  │   D3D12_USE_DERIVED_PSO=0 かつ                           │
  │   (PSOPrecache有効 ∧ !KeepPrecached) ∨ !KeepUsed        │
  └──────────────────────────────────────────────────────────┘
```

### 9.2 FD3D12PipelineStateWorker

```
  FD3D12PipelineStateWorker : FNonAbandonableTask
  ┌──────────────────────────────────────────────┐
  │ union CreationArgs                           │
  │   ComputePipelineCreationArgs_POD*           │
  │   GraphicsPipelineCreationArgs_POD*          │
  │ bool bIsGraphics                             │
  │ TRefCountPtr<ID3D12PipelineState> PSO        │
  │                                              │
  │ DoWork():                                    │
  │   bIsGraphics?                               │
  │     CreateGraphicsPipelineState(PSO, ...)    │
  │     : CreateComputePipelineState(PSO, ...)   │
  │   CreationArgs->Destroy() + delete           │
  └──────────────────────────────────────────────┘
```

**現在の制限:** `FORT-243931` により、非同期PSO生成は信頼性問題で無効化。
`OnPSOCreated()` は常に `bAsync = false` で同期生成を実行。

### 9.3 RHIレベル PSO ラッパー

```
  FD3D12GraphicsPipelineState : FRHIGraphicsPipelineState, FD3D12PipelineStateCommonData
  ┌──────────────────────────────────────────────────────────┐
  │ FGraphicsPipelineStateInitializer PipelineStateInitializer│
  │ TStaticArray<uint16> StreamStrides                       │
  │ bool bShaderNeedsGlobalConstantBuffer[SF_Num...]         │
  │ SortKey (GRHISupportsPipelineStateSortKey)               │
  │                                                          │
  │ ~dtor: RefCounting有効時                                  │
  │   PipelineState->Release()                               │
  │   RefCount==1 → PSOCache.RemoveFromLowLevelCache()       │
  │   (Precache PSO のメモリ回収)                             │
  └──────────────────────────────────────────────────────────┘

  FD3D12ComputePipelineState : FRHIComputePipelineState, FD3D12PipelineStateCommonData
  ┌──────────────────────────────────────────────────────────┐
  │ bool bShaderNeedsGlobalConstantBuffer                    │
  │ bool bPSOPrecache                                        │
  │                                                          │
  │ ~dtor: 同様の RefCount ベース自動削除                     │
  └──────────────────────────────────────────────────────────┘
```

---

## 10. FD3D12PipelineStateCacheBase

**ファイル:** `D3D12PipelineState.h`, `D3D12PipelineState.cpp`

### 10.1 Low-Level キャッシュ構造

```
  FD3D12PipelineStateCacheBase : FD3D12AdapterChild
  ┌──────────────────────────────────────────────────────────┐
  │ Graphics Low-Level Cache:                                │
  │   TPipelineCache<FD3D12LowLevelGraphicsPipelineStateDesc,│
  │                  FD3D12PipelineState*>                    │
  │   FRWLock LowLevelGraphicsPipelineStateCacheMutex        │
  │                                                          │
  │ Compute Low-Level Cache:                                 │
  │   TPipelineCache<FD3D12ComputePipelineStateDesc,          │
  │                  FD3D12PipelineState*>                    │
  │   FRWLock ComputePipelineStateCacheMutex                 │
  │                                                          │
  │ High-Level Cache (USE_HIGH_LEVEL = 0 で無効):            │
  │   InitializerToGraphicsPipelineMap                       │
  │   ComputeShaderToComputePipelineMap                      │
  │                                                          │
  │ DiskCaches[PSO_CACHE_GRAPHICS / PSO_CACHE_COMPUTE]       │
  └──────────────────────────────────────────────────────────┘
```

### 10.2 キャッシュ操作フロー

```
  FindInLoadedCache(Initializer, RootSignature)
    │ LowLevelDesc = GetLowLevelGraphicsPipelineStateDesc(Initializer, RS)
    │ LowLevelDesc.CombinedHash = HashPSODesc(LowLevelDesc)
    │
    ├─ FindInLowLevelCache(LowLevelDesc)
    │   FRWScopeLock(Read)
    │   LowLevelGraphicsPipelineStateCache.Find(LowLevelDesc)
    │   ├─ Hit: AddRef(), return
    │   └─ Miss: return nullptr
    │
    └─ (Miss時) CreateAndAdd(Initializer, RS, LowLevelDesc)
        │ CreateAndAddToLowLevelCache(LowLevelDesc)
        │   FRWScopeLock(Write)
        │   ├─ 再チェック (ダブルチェックパターン)
        │   ├─ new FD3D12PipelineState → Map に追加
        │   ├─ AddRef() × 2 (キャッシュ用 + 呼び出し側用)
        │   └─ PostCreateCallback → OnPSOCreated()
        │
        └─ new FD3D12GraphicsPipelineState(Initializer, RS, PipelineState)
```

### 10.3 RefCount ベースの PSO 回収

`UsePSORefCounting()` が true の場合:
- Low-Level キャッシュに AddRef
- FD3D12GraphicsPipelineState に AddRef
- デストラクタで Release → RefCount == 1 なら Low-Level キャッシュからも削除
- Precache PSO (`bPSOPrecache`) と使用済み PSO の両方を対象

```
  ~FD3D12GraphicsPipelineState()
    │ PipelineState->Release()
    │ RefCount == 1? (キャッシュ参照のみ残存)
    │   ├─ bRemovePrecachedPSO || bRemoveUsedPSO
    │   │   → PSOCache.RemoveFromLowLevelCache()
    │   │     FRWScopeLock(Write)
    │   │     PipelineState->Release()
    │   │     RefCount == 0? → LowLevelCache.Remove()
    │   │     RefCount > 0?  → AddRef() (他スレッドが同時要求)
    │   └─ else: キャッシュに残留
    └─ BSS/BlendState/RasterizerState/DepthStencilState Release()
```

---

## 11. FD3D12PipelineStateCache

**ファイル:** `WindowsD3D12PipelineState.h`, `WindowsD3D12PipelineState.cpp`

Windows プラットフォーム固有の PSO ディスクキャッシュ実装。

### 11.1 ディスクキャッシュアーキテクチャ

```
  FD3D12PipelineStateCache : FD3D12PipelineStateCacheBase
  ┌──────────────────────────────────────────────────────────┐
  │ FDiskCacheInterface DiskCaches[2]                        │
  │   [PSO_CACHE_GRAPHICS] ─── グラフィクス PSO 記述         │
  │   [PSO_CACHE_COMPUTE]  ─── コンピュート PSO 記述         │
  │                                                          │
  │ FDiskCacheInterface DiskBinaryCache                      │
  │   ─── ドライバ最適化バイナリブロブ (PipelineLibrary代替)  │
  │                                                          │
  │ TRefCountPtr<ID3D12PipelineLibrary> PipelineLibrary      │
  │   ─── ID3D12Device1::CreatePipelineLibrary()             │
  │   ─── PSO名でLoad/Store → ドライバ側キャッシュ          │
  │                                                          │
  │ bool bUseAPILibaries                                     │
  └──────────────────────────────────────────────────────────┘
```

### 11.2 Init フロー

```
  Init(GraphicsCacheFileName, ComputeCacheFileName, DriverBlobFileName)
    │
    ├─ CVarPipelineStateDiskCache (D3D12.PSO.DiskCache = 0 デフォルト)
    │   → DiskCaches[].Init() ─── デフォルト無効
    │
    ├─ CVarDriverOptimizedPipelineStateDiskCache (D3D12.PSO.DriverOptimizedDiskCache = 0)
    │   → DiskBinaryCache.Init()
    │   → bUseAPILibaries = 有効?
    │
    └─ ID3D12Device1::CreatePipelineLibrary()
        ├─ 既存ブロブあり → ブロブからLibrary復元
        ├─ DXGI_ERROR_UNSUPPORTED → ドライバ非対応
        ├─ FAILED (VERSION_MISMATCH等) → キャッシュクリア後再作成
        └─ Success → PipelineLibrary にセット
```

### 11.3 AddToDiskCache

PSO生成成功時に記述をディスクキャッシュに追記:

```
  AddToDiskCache(GraphicsDesc, PipelineState)
    ├─ Desc構造体をAppend
    ├─ RootSignature ブロブ長 + QBSS をAppend
    ├─ InputLayout 要素 (SemanticName文字列含む) をAppend
    ├─ 各シェーダーバイトコード (VS/MS/AS/PS/GS) をAppend
    ├─ WriteOutShaderBlob()
    │   ├─ UseCachedBlobs()? → GetCachedBlob() で DiskBinaryCache にAppend
    │   └─ else → サイズ0をAppend
    └─ DiskCache.Flush(NumPSOs + 1)
```

### 11.4 RebuildFromDiskCache

起動時にディスクキャッシュからPSOを復元:

```
  RebuildFromDiskCache()
    │ 各 Graphics PSO:
    │   SetPointerAndAdvanceFilePosition() で各フィールド復元
    │   RootSignatureManager->GetRootSignature(QBSS) で RS 再取得
    │   NodeMask チェック (LDA マスク一致のみ復元)
    │   AddToLowLevelCache() + Create() (D3D12RHI_USE_ASYNC_PRELOAD=0)
    │
    │ 各 Compute PSO:
    │   同様のフロー
    │
    └─ Close() 時: PipelineLibrary をシリアライズしてディスク保存
```

---

## 12. FD3D12StateCache

**ファイル:** `D3D12StateCachePrivate.h`, `D3D12StateCache.cpp`

コマンドコンテキストごとのGPUステート追跡システム。

### 12.1 ステート構造

```
  FD3D12StateCache : FD3D12DeviceChild, FD3D12SingleNodeGPUObject
  ┌──────────────────────────────────────────────────────────┐
  │ FD3D12CommandContext& CmdContext                          │
  │ D3D12_RESOURCE_BINDING_TIER ResourceBindingTier          │
  │ FD3D12DescriptorCache DescriptorCache                    │
  │                                                          │
  │ PipelineState:                                           │
  │   .Graphics:                                             │
  │     CurrentPipelineStateObject (FD3D12GraphicsPipelineState*)│
  │     bNeedSetRootSignature                                │
  │     CurrentReferenceStencil, CurrentBlendFactor[4]       │
  │     CurrentViewport[], CurrentScissorRects[]             │
  │     VBCache, IBCache                                     │
  │     CurrentPrimitiveType, CurrentPrimitiveTopology       │
  │     RenderTargetArray[], CurrentDepthStencilTarget       │
  │     MinDepth, MaxDepth                                   │
  │     DrawShadingRate, Combiners[], ShadingRateImage       │
  │     StreamStrides                                        │
  │                                                          │
  │   .Compute:                                              │
  │     CurrentPipelineStateObject (FD3D12ComputePipelineState*)│
  │     bNeedSetRootSignature                                │
  │     ComputeBudget                                        │
  │                                                          │
  │   .Common:                                               │
  │     SRVCache, CBVCache, UAVCache, SamplerCache           │
  │     CurrentPipelineStateObject (ID3D12PipelineState*)    │
  │     bNeedSetPSO, bNeedSetRootConstants                   │
  │     ShaderRootConstants (FUint32Vector4)                  │
  │     CurrentShaderSRV/CBV/UAV/SamplerCounts[]             │
  │     QueuedBindlessSRVs[], QueuedBindlessUAVs[]           │
  │                                                          │
  │ ダーティビット:                                          │
  │   bNeedSetVB, bNeedSetRTs, bNeedSetViewports             │
  │   bNeedSetScissorRects, bNeedSetPrimitiveTopology        │
  │   bNeedSetBlendFactor, bNeedSetStencilRef                │
  │   bNeedSetDepthBounds, bNeedSetShadingRate               │
  │   bNeedSetShadingRateImage                               │
  └──────────────────────────────────────────────────────────┘
```

### 12.2 リソースキャッシュテンプレート

```
  FD3D12ResourceCache<SlotMask>  ─── テンプレート基底
    DirtySlotMask[SF_NumStandardFrequencies]
    CleanSlot(), DirtySlot(), IsSlotDirty()
    DirtyGraphics(), DirtyCompute(), DirtyAll()

  ┌─ FD3D12ShaderResourceViewCache ──┐  SRVSlotMask
  │   Views[Stage][MAX_SRVS]         │
  │   Resources[Stage][MAX_SRVS]     │
  │   BoundMask[], MaxBoundIndex[]   │
  └──────────────────────────────────┘

  ┌─ FD3D12UnorderedAccessViewCache ─┐  UAVSlotMask
  │   Views[Stage][MAX_UAVS]         │
  │   Resources[Stage][MAX_UAVS]     │
  │   StartSlot[]                    │
  └──────────────────────────────────┘

  ┌─ FD3D12ConstantBufferCache ──────┐  CBVSlotMask
  │   CurrentGPUVirtualAddress[][]   │  (Root CBV用)
  │   Resources[][]                  │
  │   CBHandles[][] (D3D12RHI_USE_CONSTANT_BUFFER_VIEWS時) │
  └──────────────────────────────────┘

  ┌─ FD3D12SamplerStateCache ────────┐  SamplerSlotMask
  │   States[Stage][MAX_SAMPLERS]    │
  └──────────────────────────────────┘
```

### 12.3 ApplyState フロー

`ApplyState()` は Draw/Dispatch 直前に呼ばれ、全ダーティステートをコマンドリストに反映。

```
  ApplyState(HardwarePipe, PipelineType)
    │
    ├─ CmdContext.OpenIfNotAlready()
    │
    ├─ PSOCommonData = GetGraphics/ComputePipelineState()
    │
    ├─ Bindless判定:
    │   UsesDynamicResources? → bBindlessDescriptorHeaps
    │   RayTracing Configuration? → FD3D12ExplicitDescriptorCache
    │
    ├─ InternalSetDescriptorHeaps(bBindless)
    │   → DescriptorCache.SetDescriptorHeaps()
    │
    ├─ InternalSetRootSignature(PipelineType, RS, bForce)
    │   ├─ Compute: SetComputeRootSignature() → DirtyCompute() 全キャッシュ
    │   └─ Graphics: SetGraphicsRootSignature() → DirtyGraphics() 全キャッシュ
    │
    ├─ InternalSetPipelineState(PSO)
    │   Current != Pending → SetPipelineState()
    │
    ├─ BindDiagnosticBuffer (RootSignature変更時)
    │
    ├─ Graphics 固有ステート適用 (ダーティビットチェック):
    │   SetVertexBuffers, RSSetViewports, RSSetScissorRects
    │   IASetPrimitiveTopology, OMSetBlendFactor, OMSetStencilRef
    │   SetRenderTargets, SetDepthBounds, SetShadingRate
    │
    ├─ ステージ範囲: Graphics=[0..SF_Compute), Compute=[SF_Compute..SF_Num)
    │
    ├─ ApplySamplerTables(RS, Start, End)
    │   ├─ UsingGlobalSamplerHeap? → FD3D12UniqueSamplerTable 検索
    │   │   Hit → Set*RootDescriptorTable() (コピー不要)
    │   │   Miss → SwitchToContextLocalSamplerHeap()
    │   └─ LocalSamplerHeap: BuildSamplerTable() + SetSamplerTable()
    │
    ├─ bApplyResourceTables?
    │   └─ ApplyResourceTables(RS, Start, End)
    │       ├─ ダーティスロット計算 (Tier に応じた full/partial)
    │       ├─ CanReserveSlots? → RollOver (ヒープ切り替え) if needed
    │       ├─ ReserveSlots(NumViews)
    │       ├─ BuildUAVTable + SetUAVTable
    │       ├─ BuildSRVTable + SetSRVTable
    │       └─ SetConstantBufferViews (D3D12RHI_USE_CONSTANT_BUFFER_VIEWS時)
    │
    ├─ bShaderUsesBindlessResources?
    │   └─ ApplyBindlessResources(RS, Start, End)
    │       PrepareBindlessViews (SRV/UAV キュー処理)
    │
    ├─ ApplyConstants(RS, Start, End)
    │   └─ SetRootConstantBuffers() (Root CBV の GPU仮想アドレスセット)
    │
    ├─ Root Constants (4 × uint32):
    │   Set*Root32BitConstants(RootConstantsSlot, 4, ...)
    │
    └─ FlushResourceBarriers()
```

### 12.4 DirtyStateForNewCommandList

新しいコマンドリスト開始時に、コマンドリストのデフォルトと異なるステートのみをダーティマーク:

```
  DirtyStateForNewCommandList()
    ├─ 常にダーティ: PSO, RootSignature(両方), RootConstants, PrimitiveTopology
    ├─ VB: BoundVBMask > 0 の場合のみ
    ├─ IB: クリア (DrawIndexedで再設定必須)
    ├─ RT: レンダーターゲット有の場合のみ
    ├─ Viewport/ScissorRects: カウント > 0 の場合のみ
    ├─ BlendFactor: デフォルト値と異なる場合のみ
    ├─ StencilRef: デフォルト値と異なる場合のみ
    ├─ DepthBounds: 0.0-1.0 と異なる場合のみ
    ├─ ShadingRate: GRHISupports* の場合
    └─ SRV/UAV/CBV/Sampler: 全ステージ DirtyAll()
```

### 12.5 レイトレーシング遷移

```
  TransitionComputeState(PipelineType)
    LastComputePipelineType != PipelineType?
      → bNeedSetPSO = true
      → bNeedSetRootConstants = true
      → Compute.bNeedSetRootSignature = true
```

Compute ↔ RayTracing 間の遷移時にルートシグネチャとPSOを再設定。

---

## 13. CVar一覧

```
┌─────────────────────────────────────────────┬───────┬──────────┬──────────────────────────────────┐
│ CVar                                        │デフォ │ フラグ   │ 説明                             │
│                                             │ルト   │          │                                  │
├─────────────────────────────────────────────┼───────┼──────────┼──────────────────────────────────┤
│ D3D12.PSO.StallWarningThresholdInMs         │ 100.0 │ ReadOnly │ PSO生成ストール警告閾値(ms)       │
│ D3D12.PSOPrecache.KeepLowLevel              │   0   │ ReadOnly │ Precache PSO ブロブ保持           │
│ D3D12.PSO.KeepUsedPSOsInLowLevelCache       │   0   │ ReadOnly │ 使用済みPSO ブロブ保持            │
│ D3D12.PSO.DiskCache                         │   0   │ ReadOnly │ PSOディスクキャッシュ有効化       │
│ D3D12.PSO.DriverOptimizedDiskCache          │   0   │ ReadOnly │ ドライバ最適化PSOキャッシュ       │
├─────────────────────────────────────────────┼───────┼──────────┼──────────────────────────────────┤
│ D3D12.GlobalResourceDescriptorHeapSize      │ 1000K │ ReadOnly │ グローバルリソースヒープサイズ    │
│ D3D12.GlobalSamplerDescriptorHeapSize       │  2048 │ ReadOnly │ グローバルサンプラーヒープサイズ  │
│ D3D12.LocalViewHeapSize                     │  500K │ ReadOnly │ ローカルビューヒープ (~16MB VRAM) │
│ D3D12.GlobalSamplerHeapSize                 │  2048 │ ReadOnly │ グローバルサンプラーヒープ        │
│ D3D12.OnlineDescriptorHeapSize              │  500K │ ReadOnly │ オンラインディスクリプタヒープ    │
│ D3D12.OnlineDescriptorHeapBlockSize         │  2000 │ ReadOnly │ オンラインヒープブロックサイズ    │
│ D3D12.BindlessOnlineDescriptorHeapSize      │  500K │ ReadOnly │ Bindlessオンラインヒープサイズ    │
│ D3D12.BindlessOnlineDescriptorHeapBlockSize │  2000 │ ReadOnly │ Bindlessオンラインブロックサイズ  │
└─────────────────────────────────────────────┴───────┴──────────┴──────────────────────────────────┘
```

---

## ソースファイル参照

| ファイル | 内容 |
|---------|------|
| `D3D12PipelineState.h` | PSO記述構造体、FD3D12PipelineState、キャッシュベース |
| `D3D12PipelineState.cpp` | ハッシュ計算、Worker、キャッシュ操作、RefCount管理 |
| `D3D12RootSignature.h` | ERootParameterKeys、FD3D12RootSignature、Manager |
| `D3D12RootSignature.cpp` | RS記述構築、解析、静的サンプラー、QBSS→RS変換 |
| `Public/D3D12RootSignatureDefinitions.h` | 静的RS構築、Creator パターン、HLSL空間定義 |
| `D3D12StateCachePrivate.h` | FD3D12StateCache、リソースキャッシュテンプレート |
| `D3D12StateCache.cpp` | ステート適用、ディスクリプタバインディング、ダーティ追跡 |
| `Windows/WindowsD3D12PipelineState.h` | PSO Stream構造体、FD3D12PipelineStateCache |
| `Windows/WindowsD3D12PipelineState.cpp` | PSO生成、ディスクキャッシュ、PipelineLibrary、ベンダー拡張 |

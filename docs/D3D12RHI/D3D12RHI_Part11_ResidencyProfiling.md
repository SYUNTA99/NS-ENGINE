# D3D12RHI Part 11: メモリレジデンシー・GPUプロファイリング・ベンダー拡張・エラーハンドリング

## 1. メモリレジデンシー管理

### 1.1 アーキテクチャ概要

D3D12RHIは Microsoft の `D3DX12Residency` ライブラリを薄いラッパー層で統合し、
GPUメモリのレジデンシー（常駐管理）を実装する。

```
┌──────────────────────────────────────┐
│         FD3D12Resource               │
│   FD3D12ResidencyHandle Handle ──────┼──┐
└──────────────────────────────────────┘  │
                                          ▼
┌──────────────────────────────────────────────────────┐
│  D3DX12Residency ライブラリ                            │
│                                                        │
│  FD3D12ResidencyManager (= ResidencyManager)           │
│    ├── MakeResident / Evict 判定                       │
│    └── IDXGIAdapter3 メモリ予算監視                     │
│                                                        │
│  FD3D12ResidencySet (= ResidencySet)                   │
│    ├── Open() → Insert() × N → Close()                 │
│    └── コマンドリスト単位の使用リソース追跡              │
│                                                        │
│  FD3D12ResidencyHandle (= ManagedObject)               │
│    └── リソースごとの常駐状態追跡                       │
└──────────────────────────────────────────────────────┘
```

### 1.2 コンパイル時フラグ

| フラグ | デフォルト | 説明 |
|--------|-----------|------|
| `D3D12_PLATFORM_NEEDS_RESIDENCY_MANAGEMENT` | 1 | プラットフォームレベルのオプトアウト。0でスタブクラス |
| `ENABLE_RESIDENCY_MANAGEMENT` | (プラットフォーム依存) | フィーチャーレベルのランタイム制御 |

**ランタイムガード:** `GEnableResidencyManagement` グローバルbool
（コンパイル時有効でもランタイムで無効化可能）

### 1.3 FD3D12ResidencyHandle

```
struct FD3D12ResidencyHandle : D3DX12Residency::ManagedObject
{
    // DO_CHECKビルドのみ
    FD3D12GPUObject* GPUObject;   // デバッグ用逆ポインタ
};
```

各D3D12リソース（`ID3D12Pageable`）がこのハンドルを保持する。

### 1.4 インラインラッパーAPI

全関数はコンパイル時ガード（`#if ENABLE_RESIDENCY_MANAGEMENT`）+
ランタイムガード（`if (GEnableResidencyManagement)`）の二重チェックを行う。

| 関数 | パラメータ | 説明 |
|------|-----------|------|
| `Initialize()` | Handle, ID3D12Pageable*, ObjectSize, GPUObject | 管理対象オブジェクト初期化 |
| `IsInitialized()` | Handle | 登録済みかチェック（2オーバーロード） |
| `BeginTrackingObject()` | Manager, Handle | エビクション/プロモーション追跡開始 |
| `EndTrackingObject()` | Manager, Handle | 追跡終了 |
| `InitializeResidencyManager()` | Manager, Device, GPUIndex, IDXGIAdapter3, MaxLatency | マネージャー初期化 |
| `DestroyResidencyManager()` | Manager | マネージャー破棄 |
| `CreateResidencySet()` | Manager | ResidencySet割り当て（無効時nullptr） |
| `DestroyResidencySet()` | Manager, Set | ResidencySet解放 |
| `Open()` / `Close()` | Set | 変更セッション開始/終了 |
| `Insert()` | Set, Handle | 使用リソース登録（`check(IsInitialized)`) |

### 1.5 使用パターン

```
// コマンドリスト記録時
Open(ResidencySet)
  → 各リソースバインド時: Insert(Set, Resource.ResidencyHandle)
Close(ResidencySet)

// コマンドリストサブミット時
ResidencyManager が Set 内の全リソースを MakeResident()
→ 未使用リソースは LRU に基づき Evict()
```

**QueryHeapの注意:** レジデンシー追跡コードは存在するが、
「リソース使用追跡の欠落に対する一時的ワークアラウンド」として
`&& 0` でコメントアウトされている。

---

## 2. クエリシステム

### 2.1 クエリタイプ体系

```cpp
enum class ED3D12QueryType
{
    None,
    PipelineStats,              // パイプライン統計
    TimestampMicroseconds,      // マイクロ秒変換済みタイムスタンプ
    TimestampRaw,               // 生GPUタイムスタンプ（ティック）
    ProfilerTimestampTOP,       // (RHI_NEW_GPU_PROFILER) Top-of-pipe CPU時刻
    ProfilerTimestampBOP,       // (RHI_NEW_GPU_PROFILER) Bottom-of-pipe CPU時刻
    CommandListBegin,           // (Legacy) コマンドリスト開始マーカー
    CommandListEnd,             // (Legacy) コマンドリスト終了マーカー
    IdleBegin,                  // (Legacy) アイドル開始マーカー
    IdleEnd,                    // (Legacy) アイドル終了マーカー
    Occlusion,                  // オクルージョンクエリ
};

enum class ED3D12QueryPosition
{
    TopOfPipe,      // 未来のコマンドリスト作業の前に書き込み
    BottomOfPipe,   // 過去の全コマンドリスト作業の後に書き込み
};
```

### 2.2 クラス階層

```
FD3D12QueryHeap (FD3D12SingleNodeGPUObject)
│  TRefCountPtr<ID3D12QueryHeap>  D3DQueryHeap
│  TRefCountPtr<FD3D12Resource>   ResultBuffer (READBACK, 永続マップ)
│  uint8 const*             ResultPtr    (マップ済みポインタ)
│  FD3D12ResidencyHandle    ResidencyHandle
│  static MaxHeapSize = 65536 (64KB, 1ページ分)
│
├── FD3D12QueryLocation (個別クエリ結果位置)
│     Heap*, Index, Type, void* Target
│     CopyResultTo() → ResultPtr + Index * GetResultSize()
│
├── FD3D12QueryRange (ヒープ内の使用範囲)
│     Start, End
│     IsFull(Heap) → End >= Heap->NumQueries
│
└── FD3D12QueryAllocator (バンプアロケータ)
      Allocate() → 次スロット割り当て、ヒープ枯渇時にプールから取得
      CloseAndReset() → GPU解決用の範囲マップを出力
```

### 2.3 FD3D12QueryHeap

```
┌────────────────────────────────────────────────────┐
│  FD3D12QueryHeap                                    │
│                                                      │
│  MaxHeapSize = 65536 bytes (1ページ)                 │
│  NumQueries = MaxHeapSize / GetResultSize()          │
│                                                      │
│  GetResultSize():                                    │
│    Timestamp/Occlusion → sizeof(uint64) = 8         │
│    → NumQueries = 8192                              │
│    PipelineStats → sizeof(D3D12_QUERY_DATA_PIPELINE_│
│                    STATISTICS) = 88                   │
│    → NumQueries = 744                               │
│                                                      │
│  カスタム参照カウント:                               │
│    Release() → refs==0 → Device->ReleaseQueryHeap() │
│    (deleteではなくプールに返却)                       │
└────────────────────────────────────────────────────┘
```

**D3D12 QueryHeapタイプマッピング:**

| D3D12_QUERY_HEAP_TYPE | 用途 |
|------------------------|------|
| `OCCLUSION` | オクルージョンクエリ |
| `TIMESTAMP` | Directキュータイムスタンプ |
| `COPY_QUEUE_TIMESTAMP` | Copyキュータイムスタンプ |
| `PIPELINE_STATISTICS` | パイプライン統計 |

**リードバックバッファ:**
- `D3D12_HEAP_TYPE_READBACK`、クロスGPU可視マスク付き
- 永続マップ（D3D12仕様でREADBACKヒープはマップ解除不要）
- サイズ: `GetResultSize() * NumQueries`

### 2.4 FD3D12QueryAllocator

バンプアロケータパターンでクエリスロットを線形割り当てする。

```
Allocate(Type, Target):
  if CurrentHeap == null || CurrentRange is full:
    CurrentHeap = Device->ObtainQueryHeap()  // プールから取得
    Heaps.Add(CurrentHeap, new Range)
  return QueryLocation(CurrentHeap, CurrentRange->End++, Type, Target)

CloseAndReset(OutRanges):
  全ヒープの使用範囲を OutRanges に出力
  満杯ヒープ: 全リセット
  部分使用ヒープ: Start = End で再利用準備
```

### 2.5 FD3D12RenderQuery

```
class FD3D12RenderQuery : public FRHIRenderQuery,
                          public FD3D12DeviceChild,
                          public FD3D12LinkedAdapterObject<FD3D12RenderQuery>
{
    ERenderQueryType const Type;
    FD3D12SyncPointRef SyncPoint;       // 結果準備完了シグナル
    uint64* Result;                     // ヒープ割り当て（遅延削除で保護）
    FD3D12QueryLocation ActiveLocation; // アクティブオクルージョンクエリ位置
};
```

**遅延削除:** `Result`ポインタは`FD3D12DynamicRHI::DeferredDelete(EType::CPUAllocation)`で
保護され、割り込みスレッドからの非同期アクセス後に安全に解放される。

### 2.6 RHIクエリインターフェース

**バッチ最適化:**

```
RHIBeginRenderQueryBatch_TopOfPipe():
  GPU単位に1つのFD3D12SyncPointを作成
  → バッチ内の全クエリで共有（フェンスオーバーヘッド削減）

  GInsertOuterOcclusionQuery有効時:
    オクルージョンバッチ全体を外側クエリで囲む
    （GPUアーキテクチャによるパフォーマンス改善）
```

**結果取得:**

```cpp
RHIGetRenderQueryResult(Query, bWait):
  // 高速パス: シングルGPU → GPU 0 のみ
  // マルチGPU: 全リンクオブジェクトから最初の有効SyncPointを検索
  if (bWait) SyncPoint->Wait();
  return *Query->Result;
```

---

## 3. GPUプロファイラー（Legacy）

`RHI_NEW_GPU_PROFILER == 0` 時のレガシーGPUプロファイリング階層。

### 3.1 クラス階層

```
FD3D12GPUProfiler (FGPUProfiler, FD3D12DeviceChild)
│  per-device GPU プロファイリングオーケストレータ
│
├── FD3D12EventNodeFrame (FGPUProfilerEventNodeFrame, FD3D12DeviceChild)
│   │  1フレーム分のプロファイリングデータ
│   │  FD3D12BufferedGPUTiming RootEventTiming (フレーム全体タイミング)
│   │
│   └── FD3D12EventNode (FGPUProfilerEventNode, FD3D12DeviceChild)
│         1つのDraw Eventタイミング
│         FD3D12BufferedGPUTiming Timing
│
└── FD3D12BufferedGPUTiming (FGPUTiming, FD3D12DeviceChild)
      GPU タイムスタンプクエリの発行と結果取得
      Begin/End: {uint64 Result, FD3D12SyncPointRef SyncPoint}
```

### 3.2 FD3D12BufferedGPUTiming

```
StartTiming():
  1. StablePowerState CVar変更チェック
     → ID3D12Device::SetStablePowerState(true/false)
     → 成功: タイミング周波数を再読み取り
     → 失敗（SDKLayersなし）: CVar を 0 にリセット
  2. InsertTimestamp(ED3D12Units::Raw, &Begin.Result)
  3. Begin.SyncPoint = CmdContext.GetContextSyncPoint()

EndTiming():
  1. InsertTimestamp(ED3D12Units::Raw, &End.Result)
  2. End.SyncPoint = CmdContext.GetContextSyncPoint()

GetTiming() → uint64:
  1. End.SyncPoint->Wait()
  2. Begin.SyncPoint->Wait()
  3. return (End >= Begin) ? (End - Begin) : 0  // ラップアラウンド対策

CalibrateTimers(Adapter):
  全GPU: Device->GetTimestampFrequency(Direct)
         Device->GetCalibrationTimestamp(Direct)
```

### 3.3 FD3D12GPUProfiler

```
BeginFrame():
  1. GTriggerGPUProfile / GTriggerGPUHitchProfile ラッチ
  2. 相互排他: ヒッチプロファイリング中は通常プロファイリング無効
  3. DrawEvent発行状態の保存/復元
  4. ヒッチデバウンス: 報告後5フレームクールダウン
  5. 新FD3D12EventNodeFrame作成、フレームタイミング開始

EndFrame():
  通常プロファイリング:
    GPU 0のみでイベントツリーダンプ
    スクリーンショット (ShouldSaveScreenshotAfterProfilingGPU())

  ヒッチプロファイリング:
    HitchHistorySize = 4 フレーム (リングバッファ)
    GPUHitchDebounce = 5 フレーム
    閾値: RHIConfig::GetGPUHitchThreshold()
    ヒッチ検出時: 履歴全フレーム + 現在フレームダンプ
```

---

## 4. エラーハンドリングパイプライン

### 4.1 HRESULT検証マクロ

| マクロ | 説明 |
|--------|------|
| `VERIFYD3D12RESULT(x)` | HRESULT検証、nullptr Device |
| `VERIFYD3D12RESULT_EX(x, Device)` | HRESULT検証、Device指定 |
| `VERIFYD3D12RESULT_LAMBDA(x, Device, Lambda)` | 失敗時にLambdaで追加メッセージ |
| `VERIFYD3D12CREATETEXTURERESULT(x, Desc, Device)` | テクスチャ作成用（リソースDescを含む） |

### 4.2 エスカレーションパイプライン

```
VERIFYD3D12RESULT マクロ
    │
    ▼
D3D12RHI::VerifyD3D12Result(HRESULT, Code, File, Line, Device, Message)
D3D12RHI::VerifyD3D12CreateTextureResult(HRESULT, Code, File, Line, Desc, Device)
    │
    ▼
FD3D12DynamicRHI::HandleFailedD3D12Result(D3DResult, Device, bCreatingTextures, Message)
    │
    ├── E_OUTOFMEMORY ──────────────────────────────────────┐
    │                                                         ▼
    │                              TerminateOnOutOfMemory(Device, Result, bCreatingTextures)
    │                                GPUOutOfMemoryDelegate ブロードキャスト
    │                                ローカライズ済みメッセージボックス表示
    │                                LogMemoryStats() → RequestExit(true)
    │
    ├── DXGI_ERROR_DEVICE_REMOVED ─┐
    ├── DXGI_ERROR_DEVICE_HUNG ────┤
    ├── DXGI_ERROR_DEVICE_RESET ───┤
    │                              ▼
    │                    TerminateOnGPUCrash()
    │                      OutputGPUCrashReport(TextBuilder)
    │                        ├── GIsCriticalError = true, GIsGPUCrashed = true
    │                        ├── GetDeviceRemovedReason() ログ (per-device)
    │                        ├── シェーダー診断メッセージ/アサート ログ
    │                        ├── DumpActiveBreadcrumbs() (WITH_RHI_BREADCRUMBS)
    │                        ├── NV Aftermath / Intel GPU Crash Dump 呼び出し
    │                        ├── DRED ログ (1.2 → 1.1 フォールバック)
    │                        ├── LogPageFaultData() → 16MB半径内リソース検索
    │                        └── LogMemoryStats()
    │                      UGameEngine::OnGPUCrash() 通知
    │                      メッセージボックス表示 (非Shipping)
    │                      RequestExit(true)
    │
    └── その他 ── Fatal ログ + RequestExit(true)
```

**スレッドセーフティ:** `GD3DCallFailedCS` (static `FCriticalSection`) で
エラーログのシングルスレッド化を保証。一度ロックされると解放されず、
レース中のスレッドからのログスパムを防止する。

**割り込みスレッド特別処理:** 割り込みスレッド上でのデバイス除去エラーは
RHIブレッドクラムとアクティブペイロードへのアクセスがあるため、
ロックをバイパスして直接クラッシュ処理を実行する。

### 4.3 DRED (Device Removed Extended Data)

```
┌──────────────────────────────────────────────────────────┐
│  DRED ブレッドクラムシステム                               │
│                                                            │
│  FDred_1_1: ID3D12DeviceRemovedExtendedData               │
│    → GetAutoBreadcrumbsOutput()                           │
│    → D3D12_AUTO_BREADCRUMB_NODE チェーン                  │
│                                                            │
│  FDred_1_2: ID3D12DeviceRemovedExtendedData1              │
│    → GetAutoBreadcrumbsOutput1()                          │
│    → D3D12_AUTO_BREADCRUMB_NODE1 (コンテキスト文字列付き)  │
│                                                            │
│  ブレッドクラムオペレーション名 (49種):                     │
│    SetMarker, BeginEvent, EndEvent, DrawInstanced,        │
│    DrawIndexedInstanced, ExecuteIndirect, Dispatch,       │
│    CopyBufferRegion, CopyTextureRegion, CopyResource,     │
│    ResolveSubresource, ClearRenderTargetView,             │
│    ClearUnorderedAccessView, ClearDepthStencilView,       │
│    ResourceBarrier, ...                                    │
│    BuildRaytracingAccelerationStructure, DispatchRays,    │
│    DispatchMesh, DispatchGraph, SetProgram, Barrier,      │
│    ...                                                     │
│                                                            │
│  LogDREDData<FDred_T>():                                  │
│    最後100完了 + 次20オペレーション/コマンドリストをログ    │
│    ページフォルト: アクティブ + 最近解放された割り当て       │
│    クラッシュコンテキスト: RHI.DREDHasBreadcrumbData       │
│                            RHI.DREDHasPageFaultData        │
│                                                            │
│  DRED割り当てタイプ名 (29種):                              │
│    CommandQueue, CommandAllocator, PipelineState,          │
│    CommandList, Fence, DescriptorHeap, Heap,              │
│    QueryHeap, CommandSignature, PipelineLibrary,          │
│    VideoDecoder, VideoProcessor, Resource, Pass, ...      │
└──────────────────────────────────────────────────────────┘
```

### 4.4 ページフォルト診断

```cpp
// D3D12RHI::LogPageFaultData(Adapter, Device, GPU_VA)
//
// 1. フレームフェンス値ログ（キャッシュ、実際、次）
// 2. フォルトアドレス ± 16MB 半径内のアクティブリソース検索
//    FindResourcesNearGPUAddress() → 最大100件
// 3. フォルトアドレスを含むヒープ検索
//    FindHeapsContainingGPUAddress()
// 4. 過去100フレーム内の解放済み割り当て検索
//    FindReleasedAllocationData()
// 5. 各結果: GPU VA、サイズ、名前、トランジェントフラグ
```

---

## 5. ベンダー拡張

### 5.1 初期化順序

```
FD3D12Adapter::InitializeDevices():
  ┌─────────────────────────────────────────────────────┐
  │ 1. NV_AFTERMATH:                                     │
  │    InitializeBeforeDeviceCreation()                  │
  │    SetLateShaderAssociateCallback()                  │
  │                                                       │
  │ 2. INTEL_EXTENSIONS:                                  │
  │    一時デバイス作成 → CreateIntelExtensionsContext()   │
  │    EnableIntelAtomic64Support()                       │
  │    一時デバイス破棄                                   │
  │                                                       │
  │ 3. WITH_AMD_AGS:                                      │
  │    agsDriverExtensionsDX12_CreateDevice()             │
  │    (D3D12CreateDevice()の代替)                        │
  │    → AMDSupportedExtensionFlags取得                   │
  │                                                       │
  │ 4. フォールバック: 標準 D3D12CreateDevice()             │
  │                                                       │
  │ 5. NV_AFTERMATH (デバイス生成後):                      │
  │    InitializeDevice(RootDevice)                       │
  └─────────────────────────────────────────────────────┘

FD3D12DynamicRHI::Init():
  ┌─────────────────────────────────────────────────────┐
  │ 6. AMD AGS: agsInitialize() (デバイス前)              │
  │                                                       │
  │ 7. 各Adapter: InitializeDevices()                    │
  │                                                       │
  │ 8. AMD 機能検出:                                      │
  │    Pre-RDNA → GRHIMinimumWaveSize = 64               │
  │    RGP マーカーサポート検出                            │
  │    intrinsics19 → 64bit atomics                       │
  │    rayHitToken → GRHISupportsRayTracingAMDHitToken    │
  │    agsCheckDriverVersion() (RT最小バージョン)          │
  │                                                       │
  │ 9. NVAPI:                                             │
  │    NvAPI_Initialize()                                 │
  │    NV_EXTN_OP_UINT64_ATOMIC → 64bit atomics           │
  │    NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD → SER         │
  │    GetRaytracingCaps(CLUSTER_OPERATIONS)               │
  │    SetCreatePipelineStateOptions()                    │
  │    NvAPI_SYS_GetDriverAndBranchVersion()              │
  │                                                       │
  │ 10. Intel:                                            │
  │     IntelExtensionContext永続コンテキスト              │
  │     EnableIntelAppDiscovery()                          │
  └─────────────────────────────────────────────────────┘
```

### 5.2 NVIDIA Aftermath

```
名前空間: UE::RHICore::Nvidia::Aftermath::D3D12

┌──────────────────────────────────────────────────────┐
│  GFSDK Aftermath SDK 統合                              │
│                                                        │
│  InitializeDevice(ID3D12Device*):                      │
│    GFSDK_Aftermath_DX12_Initialize()                   │
│                                                        │
│  RegisterCommandList(ID3D12CommandList*):               │
│    GFSDK_Aftermath_DX12_CreateContextHandle()          │
│    → FCommandList (不透明ハンドル)                      │
│                                                        │
│  UnregisterCommandList(FCommandList):                  │
│    GFSDK_Aftermath_ReleaseContextHandle()              │
│                                                        │
│  RegisterResource(ID3D12Resource*):                    │
│    GFSDK_Aftermath_DX12_RegisterResource()             │
│                                                        │
│  UnregisterResource(FResource):                        │
│    GFSDK_Aftermath_DX12_UnregisterResource()           │
│                                                        │
│  BeginBreadcrumb / EndBreadcrumb:                      │
│    GFSDK_Aftermath_SetEventMarker() (WITH_RHI_BREADCRUMBS)│
└──────────────────────────────────────────────────────┘
```

**遅延シェーダーアソシエーション** (`CreateShaderAssociations`):

```
1. PipelineStateCache::GetPipelineStates() で全パイプライン収集 (1秒タイムアウト)
2. FrameLimit フレーム未使用パイプラインをスキップ (IsPipelineUnused)
3. TSet<FRHIShader*> で重複排除

4. パイプラインタイプ別シェーダー抽出:
   RRT_GraphicsPipelineState → VS, GS, AS, MS, PS
   RRT_ComputePipelineState  → CS
   RRT_RayTracingPipelineState → RayGen, Callable, HitGroup, Miss
   RRT_WorkGraphPipelineState  → WorkGraph shaders

5. ParallelFor() で並列登録
   RegisterShader<T>() → RegisterShaderBinary(Code, Size, Name)

6. TimeLimitSeconds タイムアウト制御
```

### 5.3 AMD AGS

```
┌──────────────────────────────────────────────────────┐
│  AMD GPU Services (AGS) SDK                            │
│                                                        │
│  FD3D12DynamicRHI メンバー:                            │
│    AGSContext* AmdAgsContext (ベアポインタ)              │
│    uint32 AmdSupportedExtensionFlags                   │
│                                                        │
│  初期化:                                               │
│    agsInitialize(VERSION, nullptr, &Context, &GpuInfo) │
│    agsDriverExtensionsDX12_CreateDevice()              │
│    → エンジン名/バージョン、アプリ名をドライバに登録    │
│    → シェーダーIntrinsics UAV: slot 0,                 │
│      space AGS_DX12_SHADER_INSTRINSICS_SPACE_ID        │
│    → AGSDX12ReturnedParams.extensionsSupported         │
│                                                        │
│  RGP マーカー (D3D12.EmitRgpFrameMarkers):              │
│    agsDriverExtensionsDX12_PushMarker(Context, CmdList, Name)│
│    agsDriverExtensionsDX12_PopMarker(Context, CmdList)  │
│                                                        │
│  ルートシグネチャ:                                      │
│    bNeedsAgsIntrinsicsSpace = true 時                   │
│    UAV(0, AGS_DX12_SHADER_INSTRINSICS_SPACE_ID,        │
│        DATA_VOLATILE, ALL) をルートパラメータに追加     │
│                                                        │
│  シャットダウン:                                        │
│    agsDeInitialize(AmdAgsContext)                       │
│                                                        │
│  機能フラグ:                                            │
│    intrinsics19 → 64bit typed atomics                  │
│    userMarkers  → RGPマーカーサポート                   │
│    rayHitToken  → RT AMDヒットトークン                  │
│    Pre-RDNA検出 → WaveSize = 64 固定                   │
└──────────────────────────────────────────────────────┘
```

### 5.4 Intel Extensions

```
┌──────────────────────────────────────────────────────┐
│  Intel Graphics Driver Extension (INTC) SDK            │
│                                                        │
│  サポートバージョンテーブル:                            │
│    {4, 8, 0}  → Typed 64bit Atomics エミュレーション    │
│                  (Nanite on Arc ACM/DG2 必須)           │
│    {4, 14, 0} → Intel GPU Crash Dumps                  │
│                  (CVarIntelCrashDumps制御)               │
│                                                        │
│  CreateIntelExtensionsContext(Device, Info, DeviceId)   │
│  DestroyIntelExtensionsContext(Context)                 │
│  EnableIntelAtomic64Support(Context, Info)              │
│  EnableIntelAppDiscovery(DeviceId)                      │
│                                                        │
│  GPU Crash Dumps (UE::RHICore::Intel::GPUCrashDumps):  │
│    RegisterCommandList(ID3D12CommandList*):             │
│      INTC_D3D12_GetCommandListHandle()                 │
│                                                        │
│    BeginBreadcrumb(CmdList, BreadcrumbNode):            │
│      INTC_D3D12_SetEventMarker(INTC_EVENT_MARKER_BEGIN)│
│                                                        │
│    EndBreadcrumb(CmdList, BreadcrumbNode):              │
│      INTC_D3D12_SetEventMarker(INTC_EVENT_MARKER_END)  │
│                                                        │
│  グローバル:                                            │
│    GDX12INTCAtomicUInt64Emulation (64bit atomic有効フラグ)│
└──────────────────────────────────────────────────────┘
```

### 5.5 ブレッドクラム統合パターン

全3ベンダーが`FD3D12CommandContext::RHIBeginBreadcrumbGPU()`/`RHIEndBreadcrumbGPU()`にフックする:

| ベンダー | Begin | End |
|----------|-------|-----|
| AMD | `agsDriverExtensionsDX12_PushMarker()` | `PopMarker()` |
| NVIDIA | `GFSDK_Aftermath_SetEventMarker()` (Begin) | `SetEventMarker()` (親ブレッドクラム復元) |
| Intel | `INTC_D3D12_SetEventMarker(BEGIN)` | `INTC_D3D12_SetEventMarker(END)` |

**リソース追跡の差異:**
- NVIDIA: コマンドリスト + リソース両方を登録/解除
- Intel: コマンドリストのみ登録
- AMD: デバイス/パイプラインレベルのみ（リソース単位追跡なし）

---

## 6. ユーティリティ関数

### 6.1 D3D12Util.h 主要API

**ヒーププロパティ照会:**

| 関数 | 戻り値 | 説明 |
|------|--------|------|
| `IsCPUWritable(HeapType, CustomProps)` | bool | UPLOAD or CUSTOM(WRITE_COMBINE/WRITE_BACK) |
| `IsGPUOnly(HeapType, CustomProps)` | bool | DEFAULT or CUSTOM(NOT_AVAILABLE) |
| `IsCPUAccessible(HeapType, CustomProps)` | bool | `!IsGPUOnly()` |
| `DetermineInitialBufferD3D12Access(HeapType, CustomProps)` | ED3D12Access | DEFAULT/UPLOAD→GenericRead, READBACK→CopyDest |

**タイル計算:**

| 関数 | 説明 |
|------|------|
| `GetTilesNeeded(W, H, D, TileShape)` | 指定次元に必要なタイル数を計算 |
| `Get4KTileShape(Shape, Format, PixelFormat, Dim, SampleCount)` | 4KBタイルシェイプ計算（BC圧縮、MSAA対応） |

**プリミティブトポロジー変換:**

| 関数 | 入力 | 出力 |
|------|------|------|
| `TranslatePrimitiveTopologyType()` | `EPrimitiveTopologyType` | `D3D12_PRIMITIVE_TOPOLOGY_TYPE` |
| `TranslatePrimitiveType()` | `EPrimitiveType` | `D3D_PRIMITIVE_TOPOLOGY` |
| `D3D12PrimitiveTypeToTopologyType()` | `D3D_PRIMITIVE_TOPOLOGY` | `D3D12_PRIMITIVE_TOPOLOGY_TYPE` |

### 6.2 デバッグ命名

```cpp
SetD3D12ObjectName(ID3D12Object*, Name);   // ID3D12Object::SetName()
SetD3D12ResourceName(...);                 // 4オーバーロード (Resource, Location, Buffer, Texture)
GetD312ObjectName(ID3D12Object*);          // GetPrivateData(WKPDID_D3DDebugObjectNameW)
                                           // ※ "D312" は UE5ソースコードのタイポ
```

**命名条件:** `RHI_USE_RESOURCE_DEBUG_NAME`が有効で、リソースがステート追跡必要または
スタンドアロン割り当ての場合のみ名前設定する。

### 6.3 ルートシグネチャ選択

```
FD3D12QuantizedBoundShaderState
  ├── FShaderRegisterCounts[SV_ShaderVisibilityCount]
  │     {SamplerCount, ConstantBufferCount, ShaderResourceCount, UnorderedAccessCount} (uint8)
  │
  ├── ERootSignatureType (RS_Raster / RS_RayTracing* / RS_WorkGraph*)
  │
  └── ビットフィールドフラグ:
        bAllowIAInputLayout
        bNeedsAgsIntrinsicsSpace
        bUseDiagnosticBuffer
        bUseDirectlyIndexedResourceHeap
        bUseDirectlyIndexedSamplerHeap
        bUseRootConstants

量子化ルール (InitShaderRegisterCounts):
  Tier 1: サンプラー・SRV・UAV → 2の累乗に切り上げ、CBV → MAX_ROOT_CBVS(16)超過時のみ2の累乗
  Tier 2: サンプラー・SRV → 最大値、UAV → 2の累乗、CBV → MAX_ROOT_CBVS超過時のみ2の累乗
  Tier 3+: サンプラー・SRV・UAV → 最大値、CBV → MAX_ROOT_CBVS超過時のみ最大値
  ※ CBV ≤ MAX_ROOT_CBVS(=16)時はルートディスクリプタ使用のため正確なカウントを保持

GetRootSignature() オーバーロード:
  1. Graphics BSS → Raster RS
  2. FD3D12ComputeShader → Compute RS
  3. ERHIShaderBundleMode → Bundle RS (WithConstants)
  4. FD3D12WorkGraphShader → WorkGraphGlobal / WorkGraphLocalCompute
  5. Global WorkGraph RS → 最大レジスタカウント
  6. WorkGraph Graphics RS → RS_WorkGraphLocalRaster (MS + PS)
  7. Global RT RS → RS_RayTracingGlobal
  8. Local RT RS → RS_RayTracingLocal (正確なカウント、SBTサイズ最小化)
```

### 6.4 リソースステートアサーション

```cpp
// ASSERT_RESOURCE_STATES = 0 (デフォルト無効)
AssertResourceState(CommandList, Resource, State, Subresource)
  → ID3D12DebugCommandList::AssertResourceState()
  → デバッグレイヤー必須
```

---

## 7. 統計 (Stats)

D3D12Util.cppに約112の`DEFINE_STAT()`宣言が存在する。

### 7.1 主要カテゴリ

| カテゴリ | 統計例 |
|---------|--------|
| **タイミング** | Present, CustomPresent, ExecuteCommandList, WaitForFence, ApplyState |
| **カウント** | CommandAllocators, CommandLists, QueryHeaps, PSOs, ExecutedCommandLists/Batches |
| **メモリ(総量)** | TotalMemory, RenderTargets, UAVTextures, Textures, UAVBuffers, Buffers |
| **スタンドアロン割り当て** | RT/UAVTexture/Texture/UAVBuffer/Buffer (バイト数+個数) |
| **テクスチャアロケータ** | Allocated, Unused, Count |
| **バッファプール** | MemoryAllocated/Used/Free, AlignmentWaste, PageCount, Fragmentation |
| **アップロードプール** | MemoryAllocated/Used/Free, AlignmentWaste, PageCount |
| **ディスクリプタヒープ** | View/Sampler Online (Count, Memory), ReusableSamplerTables/Descriptors |
| **グローバルViewヒープ** | Free/Reserved/Used/Wasted Descriptors, BlockAllocations |
| **Bindless** | Allocated/Active/InUseByGPU/Versioned, DescriptorsInit/Updated/Copied, GPUMemory |
| **ShaderBundle** | DispatchCount |

---

## 8. CVar一覧

| CVar | 型 | デフォルト | 説明 |
|------|-----|-----------|------|
| `D3D12.StablePowerState` | int32 | 0 | GPUタイミング精度向上のためのStablePowerState (0=OFF, 1=プロファイリング中, 2=起動時) |
| `D3D12.InsertOuterOcclusionQuery` | int32 | 0 | オクルージョンバッチの外側ダミークエリ（一部GPUアーキテクチャでパフォーマンス改善） |
| `D3D12.EmitRgpFrameMarkers` | int32 | 0 | AMD RGPプロファイラフレームマーカー有効化 |
| `r.GPUCrashOnOutOfMemory` | (参照) | - | OOM時にGPUクラッシュレポートをトリガーするか |
| `r.D3D12.DXR.MinimumDriverVersionNVIDIA` | int32 | 0 | RT用NVIDIA最小ドライバーバージョン |
| `r.D3D12.DXR.MinimumDriverVersionAMD` | int32 | 0 | RT用AMD最小ドライバーバージョン |
| `r.DisableEngineAndAppRegistration` | int32 | (extern) | ベンダードライバへのエンジン/アプリ名登録を無効化 |

---

## 9. 実装上の注意事項

### 9.1 タイポ

- `GetD312ObjectName()` — 正しくは`GetD3D12ObjectName`だが、これはUE5の安定APIタイポ
- `bUseAPILibaries` (WindowsD3D12PipelineState.h:157) — 正しくは`Libraries`

### 9.2 プラットフォーム依存

- `D3D12_PLATFORM_NEEDS_RESIDENCY_MANAGEMENT`: 非Windows環境では0に設定可能
- `D3D12RHI_PLATFORM_USES_TIMESTAMP_QUERIES`: 0の場合、タイムスタンプヒープ作成をスキップ
- `D3D12RHI_GPU_CRASH_LOG_VERBOSITY`: `PLATFORM_WINDOWS || !UE_BUILD_SHIPPING`時は`Error`（全WindowsビルドおよびWindows以外の非Shipping）、それ以外は`Fatal`

### 9.3 QueryHeapレジデンシー

QueryHeapのレジデンシー追跡は`&& 0`でコメントアウトされている:
```cpp
// Temporary workaround for missing resource usage tracking for query heap
```
これは`FD3D12ResidencySet::Insert()`がQueryHeapに対して正しく呼ばれない問題の
ワークアラウンドである。

### 9.4 マルチGPU制約

- GPUプロファイラ: GPU 0のみでイベントツリーダンプ
  （エディタではマルチGPU結果は有用でないため）
- レンダクエリ結果: マルチGPUでは全リンクオブジェクトから最初の有効SyncPointを検索

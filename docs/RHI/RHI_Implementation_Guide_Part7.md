# RHI 実装ガイド Part 7: デバッグ・診断機能

このドキュメントでは、RHI実装における重要なデバッグ・診断機能を解説します：

- GPU Breadcrumbs システム
- NVIDIA Aftermath 統合
- Intel GPU Crash Dumps
- RHI Validation Layer

---

## 1. GPU Breadcrumbs システム

### 1.1 概要

GPU Breadcrumbsは、GPUコマンドの実行履歴を追跡し、クラッシュ時の診断を支援するシステムです。

```
┌─────────────────────────────────────────────────────────────────┐
│                GPU Breadcrumbs システム                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  目的:                                                           │
│  ├── GPUハング/クラッシュ時の原因特定                            │
│  ├── Unreal Insights への GPU イベント出力                       │
│  ├── profilegpu コマンドのサポート                               │
│  └── GPU 統計情報の収集                                          │
│                                                                  │
│  ビルド設定:                                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // RHIBreadcrumbs.h                                     │    │
│  │                                                          │    │
│  │ // フル機能 (Debug/Development/ProfileGPU)              │    │
│  │ #define WITH_RHI_BREADCRUMBS_FULL                       │    │
│  │                                                          │    │
│  │ // 最小限 (GPU統計のみ)                                 │    │
│  │ #define WITH_RHI_BREADCRUMBS_MINIMAL                    │    │
│  │                                                          │    │
│  │ // CPU トレース出力                                      │    │
│  │ #define RHI_BREADCRUMBS_EMIT_CPU                        │    │
│  │                                                          │    │
│  │ // ソース位置情報                                        │    │
│  │ #define RHI_BREADCRUMBS_EMIT_LOCATION                   │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 主要なクラス

```
┌─────────────────────────────────────────────────────────────────┐
│              Breadcrumb クラス階層                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  FRHIBreadcrumbNode:                                             │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // ブレッドクラムノード - GPU コマンドのマーカー        │    │
│  │ struct FRHIBreadcrumbNode                                │    │
│  │ {                                                        │    │
│  │     FRHIBreadcrumbNode* Parent;      // 親ノード        │    │
│  │     FRHIBreadcrumbAllocator* Allocator;                 │    │
│  │     FRHIBreadcrumbData const& Data;  // 名前、位置等    │    │
│  │     uint32 const ID;                 // 一意のID        │    │
│  │                                                          │    │
│  │     // 仮想メソッド                                      │    │
│  │     virtual void TraceBeginGPU(...) const = 0;          │    │
│  │     virtual void TraceEndGPU(...) const = 0;            │    │
│  │     virtual TCHAR const* GetTCHAR(FBuffer&) const = 0;  │    │
│  │                                                          │    │
│  │     // ユーティリティ                                    │    │
│  │     FString GetFullPath() const;     // フルパス取得    │    │
│  │     void WriteCrashData(...) const;  // クラッシュ出力  │    │
│  │ };                                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  FRHIBreadcrumbData:                                             │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // ブレッドクラムのメタデータ                           │    │
│  │ class FRHIBreadcrumbData                                 │    │
│  │ {                                                        │    │
│  │     TCHAR const* const StaticName;   // 静的名前        │    │
│  │     ANSICHAR const* File;            // ソースファイル  │    │
│  │     uint32 Line;                     // 行番号          │    │
│  │     FRHIBreadcrumbData_Stats Stats;  // GPU統計ID       │    │
│  │ };                                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  FRHIBreadcrumbAllocator:                                        │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // ブレッドクラムのメモリアロケーター                   │    │
│  │ class FRHIBreadcrumbAllocator                            │    │
│  │ {                                                        │    │
│  │     FMemStackBase Inner;             // メモリスタック  │    │
│  │     FRHIBreadcrumbAllocatorArray Parents;               │    │
│  │                                                          │    │
│  │     template <typename T, typename... TArgs>            │    │
│  │     T* Alloc(TArgs&&... Args);       // アロケート      │    │
│  │                                                          │    │
│  │     FRHIBreadcrumbNode* AllocBreadcrumb(...);           │    │
│  │ };                                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 使用方法

```
┌─────────────────────────────────────────────────────────────────┐
│              Breadcrumb マクロの使用                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  基本マクロ:                                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // スコープ付きブレッドクラム                           │    │
│  │ RHI_BREADCRUMB_EVENT(RHICmdList, "MyPass");             │    │
│  │                                                          │    │
│  │ // 条件付き                                              │    │
│  │ RHI_BREADCRUMB_EVENT_CONDITIONAL(                       │    │
│  │     RHICmdList, bCondition, "ConditionalPass");         │    │
│  │                                                          │    │
│  │ // GPU統計付き                                           │    │
│  │ RHI_BREADCRUMB_EVENT_STAT(                              │    │
│  │     RHICmdList, StatName, "PassWithStat");              │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  フォーマット付きマクロ:                                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // printf形式でフォーマット                             │    │
│  │ RHI_BREADCRUMB_EVENT_F(RHICmdList,                      │    │
│  │     "MyPass",           // 静的名前                     │    │
│  │     "View=%d",          // フォーマット文字列           │    │
│  │     ViewIndex);         // 引数                         │    │
│  │                                                          │    │
│  │ // 名前付きフィールド (Insights出力用)                  │    │
│  │ RHI_BREADCRUMB_EVENT_F(RHICmdList,                      │    │
│  │     "DrawMesh",                                          │    │
│  │     "Mesh=%s LOD=%d",                                   │    │
│  │     RHI_BREADCRUMB_FIELD("MeshName", MeshName),         │    │
│  │     RHI_BREADCRUMB_FIELD("LODIndex", LODIndex));        │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  コンテキストAPI:                                                │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // RHI コンテキストでの使用                             │    │
│  │ void IRHIComputeContext::RHIBeginBreadcrumbGPU(         │    │
│  │     FRHIBreadcrumbNode* Breadcrumb);                    │    │
│  │                                                          │    │
│  │ void IRHIComputeContext::RHIEndBreadcrumbGPU(           │    │
│  │     FRHIBreadcrumbNode* Breadcrumb);                    │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 1.4 クラッシュ時の診断

```
┌─────────────────────────────────────────────────────────────────┐
│              クラッシュ診断フロー                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  GPUクラッシュ発生時:                                            │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ 1. Device Removed 検出                                  │    │
│  │        │                                                │    │
│  │        ▼                                                │    │
│  │ 2. FRHIBreadcrumbState::DumpActiveBreadcrumbs()        │    │
│  │    ├── 各パイプライン (Graphics/AsyncCompute)         │    │
│  │    ├── 各GPUデバイス                                    │    │
│  │    └── アクティブなブレッドクラムを出力                │    │
│  │        │                                                │    │
│  │        ▼                                                │    │
│  │ 3. FRHIBreadcrumbNode::WriteCrashData()                │    │
│  │    └── クラッシュコンテキストに書き込み               │    │
│  │        │                                                │    │
│  │        ▼                                                │    │
│  │ 4. ログ出力例:                                          │    │
│  │    LogRHI: Error: GPU Crash Breadcrumbs:               │    │
│  │    LogRHI: Error:   Scene/                              │    │
│  │    LogRHI: Error:     BasePass/                         │    │
│  │    LogRHI: Error:       DrawMesh [Mesh=SM_Rock LOD=0]  │    │
│  │    LogRHI: Error:       ← Last executed                │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. NVIDIA Aftermath 統合

### 2.1 概要

NVIDIA Aftermathは、GPUクラッシュの詳細な診断情報を収集するNVIDIA提供のSDKです。

```
┌─────────────────────────────────────────────────────────────────┐
│               NVIDIA Aftermath 概要                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  有効化条件:                                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ #ifndef NV_AFTERMATH                                    │    │
│  │ #define NV_AFTERMATH 0                                  │    │
│  │ #endif                                                  │    │
│  │                                                          │    │
│  │ // プラットフォームRHIでの有効化                        │    │
│  │ // D3D12RHI の場合、NVIDIAドライバーとAftermathSDK必要  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  機能:                                                           │
│  ├── GPUクラッシュダンプの収集                                  │
│  ├── シェーダーバイナリの登録/逆参照                            │
│  ├── ブレッドクラムマーカーとの統合                              │
│  └── フォールトアドレスの報告                                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 API

```
┌─────────────────────────────────────────────────────────────────┐
│               Aftermath API                                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  namespace UE::RHICore::Nvidia::Aftermath:                       │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                                                          │    │
│  │  // 状態チェック                                        │    │
│  │  bool IsEnabled();                                       │    │
│  │  bool IsShaderRegistrationEnabled();                    │    │
│  │  bool AreMarkersEnabled();                              │    │
│  │                                                          │    │
│  │  // 初期化 (プラットフォームRHIから呼び出し)            │    │
│  │  void InitializeBeforeDeviceCreation(                   │    │
│  │      bool bResolveMarkers = false);                     │    │
│  │                                                          │    │
│  │  bool InitializeDevice(                                 │    │
│  │      TFunctionRef<uint32(uint32 Flags)> InitCallback);  │    │
│  │                                                          │    │
│  │  // シェーダー登録                                       │    │
│  │  using FShaderHash = uint64;                            │    │
│  │  static constexpr FShaderHash InvalidShaderHash = MAX;  │    │
│  │                                                          │    │
│  │  FShaderHash RegisterShaderBinary(                      │    │
│  │      const void* Binary,                                │    │
│  │      uint32 ByteSize,                                   │    │
│  │      const FStringView& DebugName);                     │    │
│  │                                                          │    │
│  │  void DeregisterShaderBinary(FShaderHash Hash);         │    │
│  │                                                          │    │
│  │  // クラッシュ処理                                       │    │
│  │  struct FCrashResult {                                  │    │
│  │      FString OutputLog;                                 │    │
│  │      TOptional<FString> DumpPath;                       │    │
│  │      TOptional<uint64> GPUFaultAddress;                 │    │
│  │  };                                                      │    │
│  │                                                          │    │
│  │  bool OnGPUCrash(TArray<FCrashResult>& OutResults);     │    │
│  │                                                          │    │
│  │  // 遅延シェーダー関連付けコールバック                  │    │
│  │  void SetLateShaderAssociateCallback(                   │    │
│  │      FAssociateShadersFunc Callback);                   │    │
│  │                                                          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  FMarker クラス (ブレッドクラム統合用):                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ class FMarker                                           │    │
│  │ {                                                        │    │
│  │ public:                                                  │    │
│  │     FMarker(FRHIBreadcrumbNode* Breadcrumb);            │    │
│  │                                                          │    │
│  │     operator bool() const;                              │    │
│  │     void* GetPtr() const;                               │    │
│  │     uint32 GetSize() const;                             │    │
│  │ };                                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 プラットフォームRHIでの実装

```
┌─────────────────────────────────────────────────────────────────┐
│           Aftermath 実装例 (D3D12RHI)                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  デバイス作成時:                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // デバイス作成前                                       │    │
│  │ UE::RHICore::Nvidia::Aftermath::                        │    │
│  │     InitializeBeforeDeviceCreation();                   │    │
│  │                                                          │    │
│  │ // D3D12デバイス作成                                    │    │
│  │ D3D12CreateDevice(...);                                 │    │
│  │                                                          │    │
│  │ // Aftermath デバイス初期化                             │    │
│  │ UE::RHICore::Nvidia::Aftermath::InitializeDevice(       │    │
│  │     [&](uint32 Flags) -> uint32 {                       │    │
│  │         return GFSDK_Aftermath_DX12_Initialize(         │    │
│  │             Device, Flags);                             │    │
│  │     });                                                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  シェーダー作成時:                                               │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // シェーダーバイナリを登録                             │    │
│  │ auto Hash = UE::RHICore::Nvidia::Aftermath::            │    │
│  │     RegisterShaderBinary(                               │    │
│  │         ShaderBytecode.GetData(),                       │    │
│  │         ShaderBytecode.GetSize(),                       │    │
│  │         ShaderName);                                    │    │
│  │                                                          │    │
│  │ // シェーダー破棄時に登録解除                           │    │
│  │ UE::RHICore::Nvidia::Aftermath::                        │    │
│  │     DeregisterShaderBinary(StoredHash);                 │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  クラッシュ検出時:                                               │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ HRESULT hr = Device->GetDeviceRemovedReason();          │    │
│  │ if (FAILED(hr))                                         │    │
│  │ {                                                        │    │
│  │     TArray<UE::RHICore::Nvidia::Aftermath::FCrashResult>│    │
│  │         Results;                                        │    │
│  │                                                          │    │
│  │     if (UE::RHICore::Nvidia::Aftermath::OnGPUCrash(     │    │
│  │             Results))                                   │    │
│  │     {                                                    │    │
│  │         for (auto& Result : Results)                    │    │
│  │         {                                                │    │
│  │             UE_LOG(LogD3D12RHI, Error,                  │    │
│  │                 TEXT("%s"), *Result.OutputLog);         │    │
│  │                                                          │    │
│  │             if (Result.GPUFaultAddress)                 │    │
│  │             {                                            │    │
│  │                 UE_LOG(LogD3D12RHI, Error,              │    │
│  │                     TEXT("Fault Address: 0x%llX"),      │    │
│  │                     *Result.GPUFaultAddress);           │    │
│  │             }                                            │    │
│  │         }                                                │    │
│  │     }                                                    │    │
│  │ }                                                        │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Intel GPU Crash Dumps

### 3.1 概要

Intel GPU Crash Dumpsは、IntelのGPUでのクラッシュ診断をサポートします。

```
┌─────────────────────────────────────────────────────────────────┐
│            Intel GPU Crash Dumps                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  namespace UE::RHICore::Intel::GPUCrashDumps:                    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                                                          │    │
│  │  // Aftermath と同様のAPI構造                           │    │
│  │  bool IsEnabled();                                       │    │
│  │                                                          │    │
│  │  void InitializeBeforeDeviceCreation();                 │    │
│  │  bool InitializeDevice(...);                            │    │
│  │                                                          │    │
│  │  // ブレッドクラム統合                                   │    │
│  │  // デバイスID に基づく有効化                           │    │
│  │                                                          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  有効化条件:                                                     │
│  ├── Intel Arc/Xe グラフィックス                                │
│  ├── 対応ドライバーバージョン                                    │
│  └── WITH_RHI_BREADCRUMBS 有効                                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. RHI Validation Layer

### 4.1 概要

RHI Validation Layerは、RHIの使用における誤りを検出するデバッグ機能です。

```
┌─────────────────────────────────────────────────────────────────┐
│              RHI Validation Layer                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  有効化:                                                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ #if ENABLE_RHI_VALIDATION                               │    │
│  │                                                          │    │
│  │ // コマンドライン引数                                   │    │
│  │ -rhivalidation                                           │    │
│  │                                                          │    │
│  │ // または設定変数                                        │    │
│  │ r.RHI.EnableValidation=1                                │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  検証内容:                                                       │
│  ├── リソース状態の正確性                                        │
│  ├── パラメータの有効性                                          │
│  ├── バリアの正確性                                              │
│  ├── スレッドセーフティ                                          │
│  ├── リソースアクセスパターン                                    │
│  └── コマンドリスト使用法                                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 FValidationRHI クラス

```
┌─────────────────────────────────────────────────────────────────┐
│              FValidationRHI                                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  クラス構造:                                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // 実際のRHIをラップするバリデーションレイヤー          │    │
│  │ class FValidationRHI : public FDynamicRHI               │    │
│  │ {                                                        │    │
│  │ public:                                                  │    │
│  │     FValidationRHI(FDynamicRHI* InRHI);                 │    │
│  │                                                          │    │
│  │     // 全 FDynamicRHI メソッドをオーバーライド          │    │
│  │     // パラメータを検証後、内部 RHI に委譲              │    │
│  │                                                          │    │
│  │     FDynamicRHI* GetNonValidationRHI();                 │    │
│  │                                                          │    │
│  │ private:                                                 │    │
│  │     FDynamicRHI* RHI;  // ラップされた実際のRHI         │    │
│  │ };                                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  検証マクロ:                                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ #define RHI_VALIDATION_CHECK(Expression, Message)       │    │
│  │     do {                                                 │    │
│  │         if (UNLIKELY(!(Expression))) {                  │    │
│  │             FValidationRHI::ReportValidationFailure(    │    │
│  │                 Message);                               │    │
│  │         }                                                │    │
│  │     } while(0)                                          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.3 検証例

```
┌─────────────────────────────────────────────────────────────────┐
│              検証の具体例                                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  スレッドグループ数の検証:                                       │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ static void ValidateThreadGroupCount(                   │    │
│  │     uint32 X, uint32 Y, uint32 Z)                       │    │
│  │ {                                                        │    │
│  │     RHI_VALIDATION_CHECK(                               │    │
│  │         X <= GRHIMaxDispatchThreadGroupsPerDimension.X, │    │
│  │         FString::Printf(                                │    │
│  │             TEXT("ThreadGroupCountX invalid: %u"),      │    │
│  │             X));                                        │    │
│  │     // Y, Z も同様...                                   │    │
│  │ }                                                        │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  間接引数バッファの検証:                                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ static void ValidateIndirectArgsBuffer(                 │    │
│  │     FRHIBuffer* Buffer,                                 │    │
│  │     uint32 Offset,                                      │    │
│  │     uint32 Size,                                        │    │
│  │     uint32 BoundarySize)                                │    │
│  │ {                                                        │    │
│  │     // VertexBuffer または ByteAddressBuffer 必須       │    │
│  │     RHI_VALIDATION_CHECK(                               │    │
│  │         EnumHasAnyFlags(Buffer->GetUsage(),             │    │
│  │             EBufferUsageFlags::VertexBuffer |           │    │
│  │             EBufferUsageFlags::ByteAddressBuffer),      │    │
│  │         TEXT("..."));                                   │    │
│  │                                                          │    │
│  │     // DrawIndirect フラグ必須                          │    │
│  │     RHI_VALIDATION_CHECK(                               │    │
│  │         EnumHasAnyFlags(Buffer->GetUsage(),             │    │
│  │             EBufferUsageFlags::DrawIndirect),           │    │
│  │         TEXT("..."));                                   │    │
│  │                                                          │    │
│  │     // オフセットは 4 の倍数                            │    │
│  │     RHI_VALIDATION_CHECK(                               │    │
│  │         (Offset % 4) == 0,                              │    │
│  │         TEXT("..."));                                   │    │
│  │                                                          │    │
│  │     // バッファサイズ内に収まる                         │    │
│  │     RHI_VALIDATION_CHECK(                               │    │
│  │         (Offset + Size) <= Buffer->GetSize(),           │    │
│  │         TEXT("..."));                                   │    │
│  │ }                                                        │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  ロックモードの検証:                                             │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ RHI_VALIDATION_CHECK(                                   │    │
│  │     LockMode != RLM_WriteOnly_NoOverwrite ||            │    │
│  │         GRHISupportsMapWriteNoOverwrite,                │    │
│  │     TEXT("RLM_WriteOnly_NoOverwrite not supported"));   │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. デバッグ機能の組み合わせ

```
┌─────────────────────────────────────────────────────────────────┐
│           デバッグ機能の推奨構成                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  開発時 (Development Build):                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ┌──────────────────┬────────────────────────────────┐  │    │
│  │ │ 機能              │ 設定                          │  │    │
│  │ ├──────────────────┼────────────────────────────────┤  │    │
│  │ │ RHI Validation   │ 有効 (-rhivalidation)         │  │    │
│  │ │ GPU Breadcrumbs  │ WITH_RHI_BREADCRUMBS_FULL     │  │    │
│  │ │ NVIDIA Aftermath │ NV_AFTERMATH=1 (NVIDIA GPU)    │  │    │
│  │ │ D3D12 Debug Layer│ 有効 (-d3ddebug)              │  │    │
│  │ │ GPU Validation   │ 有効 (-gpuvalidation)         │  │    │
│  │ └──────────────────┴────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  テスト時 (Test Build):                                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ┌──────────────────┬────────────────────────────────┐  │    │
│  │ │ 機能              │ 設定                          │  │    │
│  │ ├──────────────────┼────────────────────────────────┤  │    │
│  │ │ RHI Validation   │ 無効                           │  │    │
│  │ │ GPU Breadcrumbs  │ WITH_RHI_BREADCRUMBS_MINIMAL   │  │    │
│  │ │ NVIDIA Aftermath │ NV_AFTERMATH=1                  │  │    │
│  │ │ D3D12 Debug Layer│ 無効                           │  │    │
│  │ └──────────────────┴────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  出荷時 (Shipping Build):                                        │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ┌──────────────────┬────────────────────────────────┐  │    │
│  │ │ 機能              │ 設定                          │  │    │
│  │ ├──────────────────┼────────────────────────────────┤  │    │
│  │ │ RHI Validation   │ 無効                           │  │    │
│  │ │ GPU Breadcrumbs  │ 無効                           │  │    │
│  │ │ NVIDIA Aftermath │ 無効                           │  │    │
│  │ │ D3D12 Debug Layer│ 無効                           │  │    │
│  │ └──────────────────┴────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## まとめ

| 機能 | 目的 | 使用場面 |
|------|------|----------|
| GPU Breadcrumbs | GPUコマンド追跡 | クラッシュ診断、プロファイリング |
| NVIDIA Aftermath | 詳細なクラッシュ情報 | NVIDIAハードウェアでのデバッグ |
| Intel GPU Crash Dumps | Intelクラッシュ診断 | Intelハードウェアでのデバッグ |
| RHI Validation | API使用法の検証 | 開発時のバグ検出 |

これらの機能を適切に組み合わせることで、GPU関連の問題を効率的に診断・解決できます。

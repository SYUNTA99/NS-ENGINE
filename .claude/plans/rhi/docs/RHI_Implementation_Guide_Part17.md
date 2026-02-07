# RHI Implementation Guide - Part 17: 内部実行エンジンと高度な同期システム

## 概要

このドキュメントでは、RHIの内部実行エンジン、マルチGPU転送、PSOプリキャッシュ、
新GPUプロファイラー、およびその他の高度な内部システムについて解説します。

---

## 1. コマンドリスト実行エンジン

### 1.1 FRHICommandListExecutor

コマンドリストの実行を制御する内部クラス。

```cpp
class FRHICommandListExecutor
{
public:
    // コマンドリストの実行
    void ExecuteList(FRHICommandListBase& CmdList);

    // 即時コマンドリストの取得
    FRHICommandListImmediate& GetImmediateCommandList();

    // RHIスレッド待機
    void WaitForRHIThreadCompletion();

    // ポーリング（RHIスレッド進捗確認）
    bool PollRHIThread();
};
```

### 1.2 ERHIThreadMode

RHIスレッドの動作モード。

```cpp
enum class ERHIThreadMode
{
    // 単一スレッド（RHIスレッドなし）
    None,

    // 専用RHIスレッド
    DedicatedThread,

    // タスクシステム利用
    Tasks
};
```

### 1.3 内部スレッドフラグ

```cpp
// RHIが別スレッドで実行中か（内部使用のみ）
extern bool GIsRunningRHIInSeparateThread_InternalUseOnly;

// 専用RHIスレッドで実行中か
extern bool GIsRunningRHIInDedicatedThread_InternalUseOnly;

// タスクスレッドで実行中か
extern bool GIsRunningRHIInTaskThread_InternalUseOnly;
```

### 1.4 レンダースレッドアイドル追跡

```cpp
enum class ERenderThreadIdleTypes
{
    WaitingForAllOtherSleep,  // 他スレッド待機
    WaitingForGPUQuery,       // GPUクエリ待機
    WaitingForGPUPresent,     // Present待機
    Num
};

// アイドルサイクルカウント（グローバル配列）
extern uint32 GRenderThreadIdle[ERenderThreadIdleTypes::Num];
```

---

## 2. マルチGPU転送システム

### 2.1 FTransferResourceFenceData

GPU間転送用のフェンスデータ。

```cpp
struct FTransferResourceFenceData
{
    // 最大8GPUまでサポート
    static constexpr uint32 MaxGPUs = 8;

    // GPU毎のフェンス値
    uint64 FenceValues[MaxGPUs] = {};

    // 転送元GPU
    uint32 SrcGPUIndex = 0;

    // 転送先GPUマスク
    FRHIGPUMask DstGPUMask;
};
```

### 2.2 FCrossGPUTransferFence

GPU間の同期ポイント管理。

```cpp
class FCrossGPUTransferFence
{
public:
    // フェンス作成
    void Create(FRHICommandListBase& RHICmdList,
                uint32 SrcGPUIndex,
                FRHIGPUMask DstGPUMask);

    // フェンス待機
    void Wait(FRHICommandListBase& RHICmdList,
              uint32 GPUIndex);

    // 完了確認
    bool IsFenceComplete(uint32 GPUIndex) const;
};
```

**使用例**:
```cpp
void TransferTextureAcrossGPUs(FRHICommandListImmediate& RHICmdList,
                               FRHITexture* Texture,
                               uint32 SrcGPU,
                               uint32 DstGPU)
{
    // GPU0からGPU1への転送
    FCrossGPUTransferFence TransferFence;

    // GPU0でフェンス作成
    {
        FScopedGPUMask Scope(RHICmdList, FRHIGPUMask::FromIndex(SrcGPU));
        TransferFence.Create(RHICmdList, SrcGPU, FRHIGPUMask::FromIndex(DstGPU));
    }

    // GPU1でフェンス待機
    {
        FScopedGPUMask Scope(RHICmdList, FRHIGPUMask::FromIndex(DstGPU));
        TransferFence.Wait(RHICmdList, DstGPU);

        // テクスチャ使用可能
    }
}
```

---

## 3. PSOプリキャッシュシステム

### 3.1 FPSOPrecacheRequestID

PSO事前キャッシュリクエストID。

```cpp
struct FPSOPrecacheRequestID
{
    // 2ビット: タイプ
    // 30ビット: リクエストID
    uint32 Value;

    enum class EType : uint32
    {
        Graphics = 0,
        Compute = 1,
        RayTracing = 2,
    };

    EType GetType() const;
    uint32 GetRequestID() const;
    bool IsValid() const;
};
```

### 3.2 FPSOPrecacheRequestResult

非同期コンパイル結果。

```cpp
struct FPSOPrecacheRequestResult
{
    enum class EStatus
    {
        Unknown,
        Pending,
        Active,      // コンパイル中
        Complete,
        Failed,
        Cancelled
    };

    EStatus Status = EStatus::Unknown;

    // コンパイル完了イベント
    FGraphEventRef CompletionEvent;

    // 結果のPSO
    FRHIGraphicsPipelineState* GraphicsPSO = nullptr;
    FRHIComputePipelineState* ComputePSO = nullptr;
};
```

### 3.3 FPSORuntimeCreationStats

PSO作成時間統計。

```cpp
struct FPSORuntimeCreationStats
{
    // 作成時間（マイクロ秒）
    uint64 CreationTimeMicroseconds = 0;

    // ドライバキャッシュヒット推測
    bool bLikelyDriverCacheHit = false;

    // 推測根拠（作成時間が閾値以下）
    static constexpr uint64 DriverCacheHitThreshold = 1000; // 1ms
};
```

**PSOプリキャッシュの使用**:
```cpp
void PrecompilePSOs(TArrayView<FGraphicsPipelineStateInitializer> Initializers)
{
    for (const auto& Init : Initializers)
    {
        FPSOPrecacheRequestID RequestID = PSOPrecacher->RequestPrecache(Init);

        if (RequestID.IsValid())
        {
            // 非同期コンパイル開始
            UE_LOG(LogRHI, Log, TEXT("PSO precache started: %u"),
                   RequestID.GetRequestID());
        }
    }
}

void CheckPSOCompletions()
{
    for (const auto& RequestID : PendingRequests)
    {
        FPSOPrecacheRequestResult Result = PSOPrecacher->GetResult(RequestID);

        if (Result.Status == FPSOPrecacheRequestResult::EStatus::Complete)
        {
            // コンパイル完了
            FPSORuntimeCreationStats Stats = PSOPrecacher->GetStats(RequestID);

            if (Stats.bLikelyDriverCacheHit)
            {
                UE_LOG(LogRHI, Verbose, TEXT("Driver cache hit detected"));
            }
        }
    }
}
```

---

## 4. 新GPUプロファイラー

### 4.1 UE::RHI::GPUProfiler 名前空間

新しいGPUプロファイリングシステム。

```cpp
namespace UE::RHI::GPUProfiler
{
    // キューの種類
    enum class FQueue
    {
        Graphics,
        Compute,
        Copy,
        SwapChain
    };

    // フレーム境界イベント
    struct FFrameBoundary
    {
        uint64 FrameNumber;
        uint64 CPUTimestamp;
        uint64 GPUTimestamp;
    };

    // プロファイリングイベント
    struct FEvent
    {
        FName Name;
        FQueue Queue;
        uint64 BeginTimestamp;
        uint64 EndTimestamp;

        // 相対計測用
        uint64 GetDurationNanoseconds() const;
    };
}
```

### 4.2 GPUプロファイラーの使用

```cpp
#if RHI_NEW_GPU_PROFILER
void ProfileGPUWork(FRHICommandList& RHICmdList)
{
    using namespace UE::RHI::GPUProfiler;

    // イベント開始
    FEvent Event;
    Event.Name = TEXT("MyGPUWork");
    Event.Queue = FQueue::Graphics;

    BeginEvent(RHICmdList, Event);

    // GPU作業
    // ...

    EndEvent(RHICmdList, Event);

    // 結果取得（数フレーム後）
    if (Event.EndTimestamp > Event.BeginTimestamp)
    {
        uint64 DurationNs = Event.GetDurationNanoseconds();
        UE_LOG(LogRHI, Log, TEXT("GPU Work: %llu ns"), DurationNs);
    }
}
#endif
```

---

## 5. 描画統計カテゴリ

### 5.1 FRHIDrawStatsCategory

描画コールのカテゴリ別カウント。

```cpp
// 最大カテゴリ数
constexpr uint32 MAX_DRAWCALL_CATEGORY = 31;

struct FRHIDrawStatsCategory
{
    // カテゴリ名
    FName Name;

    // 描画コール数
    uint32 DrawCount = 0;

    // プリミティブ数
    uint32 PrimitiveCount = 0;

    // 頂点数
    uint32 VertexCount = 0;
};

struct FRHIDrawStats
{
    // カテゴリ別統計
    FRHIDrawStatsCategory Categories[MAX_DRAWCALL_CATEGORY];

    // 使用中カテゴリ数
    uint32 NumCategories = 0;

    // 合計
    uint32 TotalDrawCalls = 0;
    uint32 TotalPrimitives = 0;
};
```

---

## 6. バウンドシェーダーステートキャッシュ

### 6.1 FBoundShaderStateKey

シェーダーコンビネーションのキー。

```cpp
struct FBoundShaderStateKey
{
    FRHIVertexDeclaration* VertexDeclaration;
    FRHIVertexShader* VertexShader;
    FRHIPixelShader* PixelShader;
    FRHIGeometryShader* GeometryShader;

    // メッシュシェーダーパイプライン
    FRHIMeshShader* MeshShader;
    FRHIAmplificationShader* AmplificationShader;

    // ハッシュ計算
    uint32 GetHash() const;

    // 比較
    bool operator==(const FBoundShaderStateKey& Other) const;
};
```

### 6.2 FCachedBoundShaderStateLink_Threadsafe

スレッドセーフなキャッシュリンク。

```cpp
class FCachedBoundShaderStateLink_Threadsafe
{
public:
    // キャッシュ検索
    FRHIBoundShaderState* Find(const FBoundShaderStateKey& Key) const;

    // キャッシュ追加
    void Add(const FBoundShaderStateKey& Key, FRHIBoundShaderState* State);

    // キャッシュ削除
    void Remove(const FBoundShaderStateKey& Key);

private:
    mutable FRWLock Lock;
    TMap<FBoundShaderStateKey, FRHIBoundShaderState*> Cache;
};
```

---

## 7. データドリブンシェーダープラットフォーム情報

### 7.1 高度なケイパビリティフラグ

```cpp
// レイトレーシング拡張
bool bSupportsRayTracingIndirectInstanceData;    // TLAS間接インスタンス
bool bSupportsRayTracingClusterOps;              // Naniteクラスター高速化
bool bSupportsShaderExecutionReordering;         // NVIDIA SER

// メッシュシェーダーティア
bool bSupportsMeshShadersTier0;  // 基本メッシュシェーダー
bool bSupportsMeshShadersTier1;  // 高度機能（プリミティブ出力等）

// コンピュートスレッド数（最大1023）
uint32 NumberOfComputeThreads : 10;
```

### 7.2 TRHIGlobal<T>

エディタプレビュー対応のグローバル値。

```cpp
template<typename T>
class TRHIGlobal
{
public:
    T Get() const
    {
#if WITH_EDITOR
        return bUsePreviewOverride ? PreviewValue : RuntimeValue;
#else
        return RuntimeValue;
#endif
    }

    void Set(T Value) { RuntimeValue = Value; }

#if WITH_EDITOR
    void SetPreviewOverride(T Value)
    {
        PreviewValue = Value;
        bUsePreviewOverride = true;
    }

    void ClearPreviewOverride()
    {
        bUsePreviewOverride = false;
    }
#endif

private:
    T RuntimeValue;
#if WITH_EDITOR
    T PreviewValue;
    bool bUsePreviewOverride = false;
#endif
};
```

---

## 8. バリデーション層詳細

### 8.1 FValidationRHI

RHI検証ラッパー。

```cpp
class FValidationRHI : public FDynamicRHI
{
public:
    FValidationRHI(FDynamicRHI* InRHI);

    // すべてのRHI呼び出しをラップして検証
    virtual FTextureRHIRef RHICreateTexture(
        const FRHITextureCreateDesc& CreateDesc) override;

    // ...

private:
    FDynamicRHI* WrappedRHI;

    // 検証状態
    void ValidateResourceState(FRHIResource* Resource, ERHIAccess ExpectedAccess);
    void ValidateShaderBindings(FRHIShader* Shader);
};
```

### 8.2 RHI_VALIDATION_CHECK マクロ

遅延評価検証マクロ。

```cpp
#if ENABLE_RHI_VALIDATION
    #define RHI_VALIDATION_CHECK(Condition, Message, ...) \
        do { \
            if (!(Condition)) { \
                FValidationRHI::ReportError(FString::Printf(Message, ##__VA_ARGS__)); \
            } \
        } while(0)
#else
    #define RHI_VALIDATION_CHECK(Condition, Message, ...) do {} while(0)
#endif
```

### 8.3 検証フラグ

```cpp
// バッファコピー元検証
extern bool GRHIValidateBufferSourceCopy;

// リソース状態検証の詳細レベル
extern int32 GRHIValidationLevel;
```

---

## 9. RHIリソースユーティリティ

### 9.1 UE::RHIResourceUtils 名前空間

```cpp
namespace UE::RHIResourceUtils
{
    // 配列からバッファ作成
    template<typename T>
    FBufferRHIRef CreateBufferWithArray(
        FRHICommandListBase& RHICmdList,
        EBufferUsageFlags Usage,
        TConstArrayView<T> Data,
        const TCHAR* DebugName = nullptr);

    // 頂点バッファ作成
    template<typename T>
    FBufferRHIRef CreateVertexBufferFromArray(
        FRHICommandListBase& RHICmdList,
        TConstArrayView<T> Vertices,
        const TCHAR* DebugName = nullptr);

    // インデックスバッファ作成
    template<typename T>
    FBufferRHIRef CreateIndexBufferFromArray(
        FRHICommandListBase& RHICmdList,
        TConstArrayView<T> Indices,
        const TCHAR* DebugName = nullptr);
}
```

**使用例**:
```cpp
void CreateMeshBuffers(FRHICommandListBase& RHICmdList)
{
    TArray<FVector3f> Vertices = { ... };
    TArray<uint32> Indices = { ... };

    FBufferRHIRef VB = UE::RHIResourceUtils::CreateVertexBufferFromArray(
        RHICmdList, Vertices, TEXT("MyMeshVB"));

    FBufferRHIRef IB = UE::RHIResourceUtils::CreateIndexBufferFromArray(
        RHICmdList, Indices, TEXT("MyMeshIB"));
}
```

---

## 10. 内部定数とアライメント

### 10.1 シェーダーパラメータアライメント

```cpp
// 構造体アライメント（16バイト境界）
constexpr uint32 SHADER_PARAMETER_STRUCT_ALIGNMENT = 16;

// 配列要素アライメント（16バイト境界）
constexpr uint32 SHADER_PARAMETER_ARRAY_ELEMENT_ALIGNMENT = 16;

// ポインタアライメント（64ビット）
constexpr uint32 SHADER_PARAMETER_POINTER_ALIGNMENT = 8;
```

### 10.2 インダイレクト引数境界

```cpp
// プラットフォーム依存のインダイレクト引数境界サイズ
#ifndef PLATFORM_DISPATCH_INDIRECT_ARGUMENT_BOUNDARY_SIZE
    #define PLATFORM_DISPATCH_INDIRECT_ARGUMENT_BOUNDARY_SIZE 256
#endif
```

### 10.3 デバッグ名フラグ

```cpp
// リソースデバッグ名の有効化
#ifndef RHI_USE_RESOURCE_DEBUG_NAME
    #define RHI_USE_RESOURCE_DEBUG_NAME 1
#endif

// 同期ポイントデバッグ名の有効化
#ifndef RHI_USE_SYNC_POINT_DEBUG_NAME
    #define RHI_USE_SYNC_POINT_DEBUG_NAME 1
#endif
```

---

## 11. 入力レイテンシ追跡

### 11.1 GInputLatencyTime

入力からフリップまでのレイテンシ計測。

```cpp
// グローバル入力レイテンシ（マイクロ秒）
extern uint64 GInputLatencyTime;

// 使用例
void MeasureInputLatency()
{
    uint64 InputTime = GetInputTimestamp();
    uint64 FlipTime = GetLastFlipTimestamp();

    GInputLatencyTime = FlipTime - InputTime;

    // VR等での低レイテンシ確認
    if (GInputLatencyTime > MaxAcceptableLatency)
    {
        UE_LOG(LogRHI, Warning, TEXT("High input latency: %llu us"),
               GInputLatencyTime);
    }
}
```

---

## 12. GPUベンダー情報

### 12.1 FRHIGlobals::FGpuInfo

```cpp
struct FGpuInfo
{
    // ベンダーID
    uint32 VendorId;

    // デバイスID
    uint32 DeviceId;

    // AMD Pre-GCNアーキテクチャ判定（レガシー対応）
    bool IsAMDPreGCNArchitecture;

    // ベンダー判定ヘルパー
    bool IsNVIDIA() const { return VendorId == 0x10DE; }
    bool IsAMD() const { return VendorId == 0x1002; }
    bool IsIntel() const { return VendorId == 0x8086; }
    bool IsQualcomm() const { return VendorId == 0x5143; }
    bool IsApple() const { return VendorId == 0x106B; }
};
```

---

## まとめ

Part 17では、RHIの内部実行エンジンと高度な同期システムを文書化しました：

1. **コマンドリスト実行エンジン** - FRHICommandListExecutor、スレッドモード
2. **マルチGPU転送** - FCrossGPUTransferFence、FTransferResourceFenceData
3. **PSOプリキャッシュ** - 非同期コンパイル、ドライバキャッシュ推測
4. **新GPUプロファイラー** - UE::RHI::GPUProfiler名前空間
5. **描画統計カテゴリ** - FRHIDrawStatsCategory
6. **バウンドシェーダーキャッシュ** - スレッドセーフキャッシュ
7. **データドリブンプラットフォーム情報** - TRHIGlobal、高度なフラグ
8. **バリデーション層** - FValidationRHI、RHI_VALIDATION_CHECK
9. **リソースユーティリティ** - UE::RHIResourceUtils
10. **内部定数** - アライメント、デバッグ名フラグ

これでRHIの内部実装詳細がほぼ完全に文書化されました。

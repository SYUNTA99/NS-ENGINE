# UE5 RHI 実装ガイド Part 12: 内部システムとユーティリティ

## 目次
1. [GPU デフラグアロケータ](#1-gpu-デフラグアロケータ)
2. [Pipeline File Cache](#2-pipeline-file-cache)
3. [GPU Readback](#3-gpu-readback)
4. [Diagnostic Buffer](#4-diagnostic-buffer)
5. [Descriptor Allocator](#5-descriptor-allocator)
6. [GPU Profiler (新システム)](#6-gpu-profiler-新システム)
7. [Resource Replace](#7-resource-replace)
8. [Lock Tracker](#8-lock-tracker)
9. [Shader Binding Layout](#9-shader-binding-layout)
10. [PSO LRU Cache](#10-pso-lru-cache)

---

## 1. GPU デフラグアロケータ

### 1.1 概要

`FGPUDefragAllocator` は、GPU メモリのフラグメンテーションを解消するためのベストフィットアロケータです。

```cpp
// GPUDefragAllocator.h

class FGPUDefragAllocator
{
public:
    // メモリ要素タイプ
    enum EMemoryElementType
    {
        MET_Allocated,   // 割り当て済み
        MET_Free,        // 空き
        MET_Locked,      // ロック中
        MET_Relocating,  // 再配置中
        MET_Resizing,    // サイズ変更中
        MET_Resized,     // サイズ変更済み
    };

    // 設定
    struct FSettings
    {
        int32 MaxDefragRelocations = 128 * 1024;  // 最大再配置バイト数
        int32 MaxDefragDownShift = 32 * 1024;     // 最大ダウンシフトバイト数
        int32 OverlappedBandwidthScale = 1;       // オーバーラップ帯域スケール
    };

    // 再配置統計
    struct FRelocationStats
    {
        int64 NumBytesRelocated;    // 再配置バイト数
        int64 NumBytesDownShifted;  // ダウンシフトバイト数
        int64 LargestHoleSize;      // 最大空き領域サイズ
        int32 NumRelocations;       // 再配置回数
        int32 NumHoles;             // 空き領域数
        int32 NumLockedChunks;      // ロックチャンク数
    };
};
```

### 1.2 主要 API

```cpp
// 初期化
void Initialize(uint8* InMemoryBase, int64 InMemorySize, int32 InAllocationAlignment);

// 割り当て
void* Allocate(int64 AllocationSize, int32 Alignment, TStatId InStat, bool bAllowFailure);

// 解放
void Free(void* Pointer);

// ロック/アンロック
void Lock(const void* Pointer);
void Unlock(const void* Pointer);

// デフラグメンテーション
void DefragmentMemory(FRelocationStats& Stats);         // 完全デフラグ
int32 Tick(FRelocationStats& Stats, bool bPanicDefrag); // 部分デフラグ（毎フレーム）

// 統計
void GetMemoryStats(int64& OutAllocatedMemorySize,
                    int64& OutAvailableMemorySize,
                    int64& OutPendingMemoryAdjustment,
                    int64& OutPaddingWasteSize);
```

### 1.3 メモリチャンク

```cpp
class FMemoryChunk
{
    uint8*          Base;           // チャンクベースアドレス
    int64           Size;           // チャンクサイズ
    bool            bIsAvailable;   // 使用可能フラグ
    int32           LockCount;      // ロックカウント
    uint32          SyncIndex;      // 同期インデックス
    void*           UserPayload;    // ユーザーペイロード
    FMemoryChunk*   PreviousChunk;  // 前のチャンク
    FMemoryChunk*   NextChunk;      // 次のチャンク
};
```

### 1.4 プラットフォーム実装

```cpp
// プラットフォーム固有のメモリ再配置
virtual void PlatformRelocate(void* Dest, const void* Source, int64 Size, void* UserPayload) = 0;

// フェンス挿入
virtual uint64 PlatformInsertFence() = 0;

// フェンスブロック
virtual void PlatformBlockOnFence(uint64 Fence) = 0;

// 再配置可能かチェック
virtual bool PlatformCanRelocate(const void* Source, void* UserPayload) const = 0;
```

---

## 2. Pipeline File Cache

### 2.1 概要

`FPipelineFileCacheManager` は、PSO（Pipeline State Object）のキャッシングと事前コンパイルを管理します。

```cpp
// PipelineFileCache.h

class FPipelineFileCacheManager
{
public:
    // 保存モード
    enum class SaveMode : uint32
    {
        Incremental = 0,   // インクリメンタル（高速）
        BoundPSOsOnly = 1, // バインドされた PSO のみ
    };

    // PSO 順序
    enum class PSOOrder : uint32
    {
        Default = 0,          // デフォルト順序
        FirstToLatestUsed = 1, // 最初に使用された順
        MostToLeastUsed = 2   // 最も使用された順
    };
};
```

### 2.2 PSO ディスクリプタ

```cpp
struct FPipelineCacheFileFormatPSO
{
    enum class DescriptorType : uint32
    {
        Compute = 0,
        Graphics = 1,
        RayTracing = 2,
    };

    struct ComputeDescriptor
    {
        FSHAHash ComputeShader;
    };

    struct GraphicsDescriptor
    {
        FSHAHash VertexShader;
        FSHAHash FragmentShader;
        FSHAHash GeometryShader;
        FSHAHash MeshShader;
        FSHAHash AmplificationShader;

        FVertexDeclarationElementList VertexDescriptor;
        FBlendStateInitializerRHI BlendState;
        FPipelineFileCacheRasterizerState RasterizerState;
        FDepthStencilStateInitializerRHI DepthStencilState;

        EPixelFormat RenderTargetFormats[MaxSimultaneousRenderTargets];
        uint32 RenderTargetsActive;
        uint32 MSAASamples;
        // ...
    };

    struct FPipelineFileCacheRayTracingDesc
    {
        FSHAHash ShaderHash;
        EShaderFrequency Frequency;
        FRHIShaderBindingLayout ShaderBindingLayout;
    };
};
```

### 2.3 主要 API

```cpp
// 初期化/シャットダウン
static void Initialize(uint32 GameVersion);
static void Shutdown();

// ファイルキャッシュ操作
static bool OpenPipelineFileCache(const FString& Key, const FString& CacheName,
                                   EShaderPlatform Platform, FGuid& OutGameFileGuid);
static bool SavePipelineFileCache(SaveMode Mode);
static void CloseUserPipelineFileCache();

// PSO キャッシング
static void CacheGraphicsPSO(uint32 RunTimeHash,
                              FGraphicsPipelineStateInitializer const& Initializer,
                              bool bWasPSOPrecached,
                              FPipelineStateStats** OutStats = nullptr);
static void CacheComputePSO(uint32 RunTimeHash,
                             FRHIComputeShader const* Initializer,
                             bool bWasPSOPrecached,
                             FPipelineStateStats** OutStats = nullptr);
static void CacheRayTracingPSO(const FRayTracingPipelineStateInitializer& Initializer,
                                ERayTracingPipelineCacheFlags Flags);

// ゲーム使用マスク
static uint64 SetGameUsageMaskWithComparison(uint64 GameUsageMask,
                                              FPSOMaskComparisonFn InComparisonFnPtr);
```

### 2.4 PSO 統計

```cpp
struct FPipelineStateStats
{
    int64 FirstFrameUsed;   // 最初に使用されたフレーム
    int64 LastFrameUsed;    // 最後に使用されたフレーム
    uint64 CreateCount;     // 作成回数
    int64 TotalBindCount;   // 総バインド回数
    uint32 PSOHash;         // PSO ハッシュ
};
```

---

## 3. GPU Readback

### 3.1 概要

`FRHIGPUMemoryReadback` は、GPU からの非同期メモリ読み取りを管理します。

```cpp
// RHIGPUReadback.h

class FRHIGPUMemoryReadback
{
public:
    FRHIGPUMemoryReadback(FName RequestName);

    // 準備完了チェック
    bool IsReady();
    bool IsReady(FRHIGPUMask GPUMask);

    // 同期待機
    void Wait(FRHICommandListImmediate& RHICmdList, FRHIGPUMask GPUMask) const;

    // コピー要求
    virtual void EnqueueCopy(FRHICommandList& RHICmdList,
                              FRHIBuffer* SourceBuffer, uint32 NumBytes = 0);
    virtual void EnqueueCopy(FRHICommandList& RHICmdList,
                              FRHITexture* SourceTexture,
                              const FIntVector& SourcePosition,
                              uint32 SourceSlice, const FIntVector& Size);

    // データアクセス
    virtual void* Lock(uint32 NumBytes) = 0;
    virtual void Unlock() = 0;

protected:
    FGPUFenceRHIRef Fence;
    FRHIGPUMask LastCopyGPUMask;
    uint32 LastLockGPUIndex;
};
```

### 3.2 バッファ Readback

```cpp
class FRHIGPUBufferReadback final : public FRHIGPUMemoryReadback
{
public:
    FRHIGPUBufferReadback(FName RequestName);

    void EnqueueCopy(FRHICommandList& RHICmdList,
                      FRHIBuffer* SourceBuffer, uint32 NumBytes = 0) override;
    void* Lock(uint32 NumBytes) override;
    void Unlock() override;
    uint64 GetGPUSizeBytes() const;

private:
    FStagingBufferRHIRef DestinationStagingBuffers[MAX_NUM_GPUS];
};
```

### 3.3 テクスチャ Readback

```cpp
class FRHIGPUTextureReadback final : public FRHIGPUMemoryReadback
{
public:
    FRHIGPUTextureReadback(FName RequestName);

    void EnqueueCopy(FRHICommandList& RHICmdList,
                      FRHITexture* SourceTexture,
                      const FIntVector& SourcePosition,
                      uint32 SourceSlice, const FIntVector& Size) override;

    void* Lock(int32& OutRowPitchInPixels, int32* OutBufferHeight = nullptr);
    void Unlock() override;
    uint64 GetGPUSizeBytes() const;

    FTextureRHIRef DestinationStagingTextures[MAX_NUM_GPUS];
};
```

### 3.4 使用例

```cpp
// Readback 作成
FRHIGPUBufferReadback* Readback = new FRHIGPUBufferReadback(TEXT("MyReadback"));

// コピー要求
Readback->EnqueueCopy(RHICmdList, SourceBuffer, SizeInBytes);

// 後のフレームで結果を取得
if (Readback->IsReady())
{
    void* Data = Readback->Lock(SizeInBytes);
    // データを処理...
    Readback->Unlock();
}
```

---

## 4. Diagnostic Buffer

### 4.1 概要

`FRHIDiagnosticBuffer` は、GPU からの診断メッセージ（デバッグログ、シェーダーアサート等）を収集します。

```cpp
// RHIDiagnosticBuffer.h

class FRHIDiagnosticBuffer
{
public:
    // シェーダーからのメッセージレーン
    struct FLane
    {
        uint32 Counter;
        uint32 MessageID;
        union
        {
            int32  AsInt[4];
            uint32 AsUint[4];
            float  AsFloat[4];
        } Payload;
    };

    struct FQueue
    {
        static constexpr uint32 MaxLanes = 64;  // UEDiagnosticMaxLanes
        FLane Lanes[MaxLanes];

    #if WITH_RHI_BREADCRUMBS
        uint32 MarkerIn;   // GPU Breadcrumb マーカー入力
        uint32 MarkerOut;  // GPU Breadcrumb マーカー出力
    #endif
    };

    static constexpr uint32 SizeInBytes = sizeof(FQueue);

    // 永続マップされた診断バッファデータ
    FQueue* Data = nullptr;

    // シェーダー診断メッセージ取得
    FString GetShaderDiagnosticMessages(uint32 DeviceIndex,
                                         uint32 QueueIndex,
                                         const TCHAR* QueueName);
};
```

### 4.2 シェーダー側 (HLSL)

```hlsl
// UEDiagnosticBuffer レイアウト
struct UEDiagnosticLane
{
    uint Counter;
    uint MessageID;
    uint4 Payload;  // int4, uint4, float4 として解釈可能
};

#define UEDiagnosticMaxLanes 64

RWStructuredBuffer<UEDiagnosticLane> DiagnosticBuffer;
```

---

## 5. Descriptor Allocator

### 5.1 概要

`FRHIDescriptorAllocator` は、ディスクリプタヒープからのディスクリプタ割り当てを管理します。

```cpp
// RHIDescriptorAllocator.h

struct FRHIDescriptorAllocation
{
    uint32 StartIndex = 0;
    uint32 Count = 0;
};

class FRHIDescriptorAllocator
{
public:
    FRHIDescriptorAllocator(uint32 InNumDescriptors, TConstArrayView<TStatId> InStats);

    // 割り当て/解放
    TOptional<FRHIDescriptorAllocation> Allocate(uint32 NumDescriptors);
    void Free(FRHIDescriptorAllocation Allocation);

    FRHIDescriptorHandle Allocate(ERHIDescriptorType InType);
    void Free(FRHIDescriptorHandle InHandle);

    // リサイズして割り当て
    TOptional<FRHIDescriptorAllocation> ResizeGrowAndAllocate(uint32 NewCapacity,
                                                               uint32 NumAllocations);

    // 割り当て範囲取得
    bool GetAllocatedRange(FRHIDescriptorAllocatorRange& OutRange);

    uint32 GetCapacity() const;
};
```

### 5.2 ヒープディスクリプタアロケータ

```cpp
class FRHIHeapDescriptorAllocator : protected FRHIDescriptorAllocator
{
public:
    FRHIHeapDescriptorAllocator(ERHIDescriptorTypeMask InTypeMask,
                                 uint32 InDescriptorCount,
                                 TConstArrayView<TStatId> InStats);

    FRHIDescriptorHandle Allocate(ERHIDescriptorType InType);
    void Free(FRHIDescriptorHandle InHandle);

    ERHIDescriptorTypeMask GetTypeMask() const;
    bool HandlesAllocation(ERHIDescriptorType InType) const;
};
```

### 5.3 オフセット付きアロケータ

```cpp
class FRHIOffsetHeapDescriptorAllocator : protected FRHIHeapDescriptorAllocator
{
public:
    FRHIOffsetHeapDescriptorAllocator(ERHIDescriptorTypeMask InTypeMask,
                                       uint32 InDescriptorCount,
                                       uint32 InHeapOffset,
                                       TConstArrayView<TStatId> InStats);

    FRHIDescriptorHandle Allocate(ERHIDescriptorType InType);
    void Free(FRHIDescriptorHandle InHandle);

private:
    const uint32 HeapOffset;
};
```

---

## 6. GPU Profiler (新システム)

### 6.1 概要

`RHI_NEW_GPU_PROFILER` は、階層的な GPU プロファイリングの新システムです。

```cpp
// GPUProfiler.h

#if RHI_NEW_GPU_PROFILER
namespace UE::RHI::GPUProfiler
{
    struct FQueue
    {
        enum class EType : uint8
        {
            Graphics,
            Compute,
            Copy,
            SwapChain
        };

        EType Type;
        uint8 GPU;
        uint8 Index;
    };
};
#endif
```

### 6.2 イベントタイプ

```cpp
struct FEvent
{
    // フレーム境界
    struct FFrameBoundary
    {
        uint64 CPUTimestamp;
        uint32 FrameNumber;
    #if WITH_RHI_BREADCRUMBS
        FRHIBreadcrumbNode* const Breadcrumb;
    #endif
    };

    // フレーム時間（GPU タイミング非サポートプラットフォーム用）
    struct FFrameTime
    {
        uint64 TotalGPUTime;
    };

#if WITH_RHI_BREADCRUMBS
    // Breadcrumb 開始/終了
    struct FBeginBreadcrumb
    {
        FRHIBreadcrumbNode* const Breadcrumb;
        uint64 GPUTimestampTOP;
    };
    struct FEndBreadcrumb
    {
        FRHIBreadcrumbNode* const Breadcrumb;
        uint64 GPUTimestampBOP;
    };
#endif

    // 作業開始/終了
    struct FBeginWork
    {
        uint64 CPUTimestamp;
        uint64 GPUTimestampTOP;
    };
    struct FEndWork
    {
        uint64 GPUTimestampBOP;
    };

    // 統計
    struct FStats
    {
        uint32 NumDraws;
        uint32 NumDispatches;
        uint32 NumPrimitives;
        uint32 NumVertices;
    };
};
```

### 6.3 タイムスタンプ

- **TOP (Top of Pipe)**: GPU コマンドプロセッサが作業開始前に書き込む
- **BOP (Bottom of Pipe)**: GPU が作業完了後に書き込む
- すべてのタイムスタンプは `FPlatformTime::Cycles64()` を基準

---

## 7. Resource Replace

### 7.1 概要

`FRHIResourceReplaceBatcher` は、リソースの非同期置換をバッチ処理します。

```cpp
// RHIResourceReplace.h

class FRHIResourceReplaceInfo
{
public:
    template <typename TType>
    struct TPair
    {
        TType* Dst;
        TType* Src;
    };

    typedef TPair<FRHIBuffer>             FBuffer;
    typedef TPair<FRHIRayTracingGeometry> FRTGeometry;

    enum class EType : uint8
    {
        Buffer,
        RTGeometry,
    };

    EType GetType() const;
    FBuffer const& GetBuffer() const;
    FRTGeometry const& GetRTGeometry() const;
};

class FRHIResourceReplaceBatcher
{
    FRHICommandListBase& RHICmdList;
    TArray<FRHIResourceReplaceInfo> Infos;

public:
    FRHIResourceReplaceBatcher(FRHICommandListBase& RHICmdList,
                                uint32 InitialCapacity = 0);
    ~FRHIResourceReplaceBatcher();

    template <typename... TArgs>
    void EnqueueReplace(TArgs&&... Args);
};
```

### 7.2 使用例

```cpp
// リソース置換バッチ作成
FRHIResourceReplaceBatcher Batcher(RHICmdList, 10);

// バッファ置換をキュー
Batcher.EnqueueReplace(DstBuffer, SrcBuffer);

// RT ジオメトリ置換をキュー
Batcher.EnqueueReplace(DstRTGeometry, SrcRTGeometry);

// デストラクタで一括実行
```

---

## 8. Lock Tracker

### 8.1 概要

`FRHILockTracker` は、バッファとテクスチャのロック状態を追跡します。

```cpp
// RHILockTracker.h

struct FRHILockTracker
{
    // テクスチャロックパラメータ
    struct FTextureLockParams
    {
        FRHILockedTextureDesc Desc;
        EResourceLockMode LockMode = RLM_WriteOnly;
        void* Data = nullptr;
        bool bDirectLock = false;
    };

    // バッファロックパラメータ
    struct FLockParams
    {
        void* RHIBuffer;
        void* Buffer;
        uint32 BufferSize;
        uint32 Offset;
        EResourceLockMode LockMode;
        bool bDirectLock;
        bool bCreateLock;
    };

    // アンロックフェンスパラメータ
    struct FUnlockFenceParams
    {
        void* RHIBuffer;
        FGraphEventRef UnlockEvent;
    };

    // 未処理のロック
    TArray<FTextureLockParams, TInlineAllocator<16>> OutstandingTextureLocks;
    TArray<FLockParams, TInlineAllocator<16>> OutstandingLocks;
    TArray<FUnlockFenceParams, TInlineAllocator<16>> OutstandingUnlocks;

    // ロック/アンロック
    void Lock(const FRHILockTextureArgs& InArguments, void* InData, bool bInDirectBufferWrite);
    FTextureLockParams Unlock(const FRHILockedTextureDesc& InDesc);

    void Lock(void* RHIBuffer, void* Buffer, uint32 Offset, uint32 SizeRHI,
              EResourceLockMode LockMode, bool bInDirectBufferWrite = false,
              bool bInCreateLock = false);
    FLockParams Unlock(void* RHIBuffer, uint32 Offset = 0);

    // アンロックフェンス管理
    template<class TIndexOrVertexBufferPointer>
    void AddUnlockFence(TIndexOrVertexBufferPointer* Buffer,
                         FRHICommandListImmediate& RHICmdList,
                         const FLockParams& LockParms);
    void WaitForUnlock(void* RHIBuffer);
    void FlushCompleteUnlocks();
};

// グローバルインスタンス
extern RHI_API FRHILockTracker GRHILockTracker;
```

---

## 9. Shader Binding Layout

### 9.1 概要

`FRHIShaderBindingLayout` は、シェーダーコンパイル時にバインディングレイアウトを定義します。

```cpp
// RHIShaderBindingLayout.h

enum class EShaderBindingLayoutFlags : uint8
{
    None = 0,
    AllowMeshShaders = 1 << 0,
    InputAssembler = 1 << 1,
    BindlessResources = 1 << 2,
    BindlessSamplers = 1 << 3,
    RootConstants = 1 << 4,
    ShaderBindingLayoutUsed = 1 << 5,
};

struct FRHIUniformBufferShaderBindingLayout
{
    FString LayoutName;
    uint32 RegisterSpace : 4;
    uint32 CBVResourceIndex : 6;
    uint32 BaseSRVResourceIndex : 8;
    uint32 BaseUAVResourceIndex : 8;
    uint32 BaseSamplerResourceIndex : 6;
};

class FRHIShaderBindingLayout
{
public:
    enum { MaxUniformBufferEntries = 8 };

    FRHIShaderBindingLayout(EShaderBindingLayoutFlags InFlags,
                             TConstArrayView<FRHIUniformBufferShaderBindingLayout> InEntries);

    uint32 GetHash() const;
    EShaderBindingLayoutFlags GetFlags() const;
    uint32 GetNumUniformBufferEntries() const;
    const FRHIUniformBufferShaderBindingLayout& GetUniformBufferEntry(uint32 Index) const;
    const FRHIUniformBufferShaderBindingLayout* FindEntry(const FString& LayoutName) const;

private:
    uint32 Hash = 0;
    EShaderBindingLayoutFlags Flags;
    uint32 NumUniformBufferEntries = 0;
    TStaticArray<FRHIUniformBufferShaderBindingLayout, MaxUniformBufferEntries> UniformBufferEntries;
};
```

### 9.2 利点

- 同じ `FRHIShaderBindingLayout` を使用する複数シェーダーは Uniform Buffer を一度だけバインド
- シェーダーリフレクション最適化
- PSO 間でのバインディング共有

---

## 10. PSO LRU Cache

### 10.1 概要

`TPsoLruCache` は、PSO の LRU（Least Recently Used）キャッシュを実装します。

```cpp
// PsoLruCache.h

template<typename KeyType, typename ValueType>
class TPsoLruCache
{
public:
    TPsoLruCache(int32 InMaxNumElements);

    // 追加（最も最近使用としてマーク）
    FSetElementId Add(const KeyType& Key, const ValueType& Value);

    // 検索
    bool Contains(const KeyType& Key) const;
    const ValueType* Find(const KeyType& Key) const;
    const ValueType* FindAndTouch(const KeyType& Key);  // 使用としてマーク

    // 削除
    void Remove(const KeyType& Key);
    ValueType RemoveLeastRecent();
    ValueType RemoveMostRecent();

    // 統計
    int32 Max() const;
    int32 Num() const;

    // Predicate ベース操作
    template<typename Predicate>
    bool ContainsByPredicate(Predicate Pred) const;
    template<typename Predicate>
    TArray<ValueType> FilterByPredicate(Predicate Pred) const;
    template<typename Predicate>
    int32 RemoveByPredicate(Predicate Pred);
};
```

### 10.2 使用例

```cpp
// PSO キャッシュ作成（最大 1000 エントリ）
TPsoLruCache<uint32, FGraphicsPipelineStateRHIRef> PSOCache(1000);

// PSO 追加
PSOCache.Add(PSOHash, NewPSO);

// PSO 検索（使用としてマーク）
const FGraphicsPipelineStateRHIRef* CachedPSO = PSOCache.FindAndTouch(PSOHash);

// キャッシュが満杯の場合、最も古いものを削除
if (PSOCache.Num() >= PSOCache.Max())
{
    FGraphicsPipelineStateRHIRef EvictedPSO = PSOCache.RemoveLeastRecent();
}
```

---

## 11. ベストプラクティス

### 11.1 GPU Defrag

1. **毎フレーム Tick** - `Tick()` を毎フレーム呼び出し
2. **パニックデフラグ** - メモリ不足時のみ使用
3. **ロック最小化** - 再配置をブロックするロックを最小限に

### 11.2 Pipeline Cache

1. **事前コンパイル** - ロード画面で PSO を事前コンパイル
2. **ゲームマスク** - 品質設定に基づくフィルタリング
3. **インクリメンタル保存** - 高速保存モードを優先

### 11.3 GPU Readback

1. **フェンス確認** - `IsReady()` をポーリングして待機を避ける
2. **ダブルバッファ** - 継続的な readback にはダブルバッファ使用
3. **サイズ最小化** - 必要なデータのみをコピー

---

## 12. 関連ドキュメント

- [Part 3: シェーダーと PSO](RHI_Implementation_Guide_Part3.md) - PSO 基本
- [Part 7: デバッグ・診断](RHI_Implementation_Guide_Part7.md) - GPU Breadcrumbs
- [Part 10: 高度なリソース管理](RHI_Implementation_Guide_Part10.md) - メモリ管理

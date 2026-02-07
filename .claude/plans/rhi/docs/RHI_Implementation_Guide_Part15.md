# RHI Implementation Guide - Part 15: コマンドリストユーティリティと高度なパラメータシステム

## 概要

このドキュメントでは、RHIコマンドリストのスコープユーティリティ、バッチシェーダーパラメータシステム、
クエリプール、およびリソースリファレンスシステムについて解説します。

---

## 1. コマンドリストスコープユーティリティ

### 1.1 FRHICommandListScopedFence

**場所**: `RHICommandList.h:422`

複数のリソース更新をバッチ化し、スコープ終了時に単一のフェンスを発行するRAIIクラス。

```cpp
class FRHICommandListScopedFence
{
    friend class FRHICommandListBase;
    FRHICommandListBase& RHICmdList;
    FRHICommandListScopedFence* Previous;
    bool bFenceRequested = false;

public:
    FRHICommandListScopedFence(FRHICommandListBase& RHICmdList);
    ~FRHICommandListScopedFence();
};
```

**用途**:
- 複数のリソースロック/アンロック操作をまとめる
- N個のフェンス発行を1個に削減
- RHIThreadFence のオーバーヘッド軽減

**使用例**:
```cpp
void BatchUpdateResources(FRHICommandListBase& RHICmdList)
{
    // スコープ開始 - 内部でフェンスリクエストを追跡
    FRHICommandListScopedFence ScopedFence(RHICmdList);

    // これらの操作は個別にフェンスを発行しない
    Buffer1->Lock(...);
    Buffer1->Unlock();
    Buffer2->Lock(...);
    Buffer2->Unlock();

    // スコープ終了時に単一のフェンスが発行される
}
```

### 1.2 FRHICommandListScopedPipelineGuard

**場所**: `RHICommandList.h:434`

パイプライン状態の設定を保護するガードクラス。

```cpp
class FRHICommandListScopedPipelineGuard
{
    FRHICommandListBase& RHICmdList;
    bool bPipelineSet = false;

public:
    FRHICommandListScopedPipelineGuard(FRHICommandListBase& RHICmdList);
    ~FRHICommandListScopedPipelineGuard();
};
```

**用途**:
- パイプライン状態の一貫性保証
- 描画コマンド前の状態検証
- デバッグビルドでの状態追跡

### 1.3 FRHICommandListScopedAllowExtraTransitions

**場所**: `RHICommandList.h:444`

追加のリソース遷移を一時的に許可するスコープクラス。

```cpp
class FRHICommandListScopedAllowExtraTransitions
{
    FRHICommandListBase& RHICmdList;
    bool bAllowExtraTransitions = false;

public:
    FRHICommandListScopedAllowExtraTransitions(
        FRHICommandListBase& RHICmdList,
        bool bAllowExtraTransitions);
    ~FRHICommandListScopedAllowExtraTransitions();
};
```

**用途**:
- 通常は禁止されている追加遷移の一時許可
- レガシーコードとの互換性維持
- プラットフォーム固有の回避策

---

## 2. バッチシェーダーパラメータシステム

### 2.1 システム概要

RHIは効率的なシェーダーパラメータ設定のためのバッチングシステムを提供します。

**関連クラス**:
- `FRHIShaderParameter` - スカラーパラメータ
- `FRHIShaderParameterResource` - リソースパラメータ
- `FRHIBatchedShaderParameters` - パラメータコレクション
- `FRHIBatchedShaderParametersAllocator` - メモリアロケータ
- `FRHIBatchedShaderUnbinds` - アンバインドコレクション

### 2.2 FRHIShaderParameter

**場所**: `RHIShaderParameters.h:15`

スカラーシェーダーパラメータのコンパクト表現。

```cpp
struct FRHIShaderParameter
{
    FRHIShaderParameter(uint16 InBufferIndex, uint16 InBaseIndex,
                        uint16 InByteOffset, uint16 InByteSize)
        : BufferIndex(InBufferIndex)
        , BaseIndex(InBaseIndex)
        , ByteOffset(InByteOffset)
        , ByteSize(InByteSize)
    {}

    uint16 BufferIndex;   // バッファインデックス
    uint16 BaseIndex;     // ベースインデックス
    uint16 ByteOffset;    // データブロブ内のオフセット
    uint16 ByteSize;      // データサイズ
};
```

### 2.3 FRHIShaderParameterResource

**場所**: `RHIShaderParameters.h:30`

リソースパラメータ（テクスチャ、SRV、UAV、サンプラー等）の表現。

```cpp
struct FRHIShaderParameterResource
{
    enum class EType : uint8
    {
        Texture,
        ResourceView,
        UnorderedAccessView,
        Sampler,
        UniformBuffer,
        ResourceCollection,  // バインドレス用
    };

    FRHIResource* Resource = nullptr;
    uint16        Index = 0;
    EType         Type = EType::Texture;

    // 型別コンストラクタ
    FRHIShaderParameterResource(FRHITexture* InTexture, uint16 InIndex);
    FRHIShaderParameterResource(FRHIShaderResourceView* InView, uint16 InIndex);
    FRHIShaderParameterResource(FRHIUnorderedAccessView* InUAV, uint16 InIndex);
    FRHIShaderParameterResource(FRHISamplerState* InSampler, uint16 InIndex);
    FRHIShaderParameterResource(FRHIUniformBuffer* InUBO, uint16 InIndex);
    FRHIShaderParameterResource(FRHIResourceCollection* InCollection, uint16 InIndex);
};
```

### 2.4 FRHIBatchedShaderParametersAllocator

**場所**: `RHIShaderParameters.h:95`

パラメータデータ用の効率的なメモリアロケータ。

```cpp
class FRHIBatchedShaderParametersAllocator
{
public:
    FRHIBatchedShaderParametersAllocator* Next;
    FRHICommandListBase& RHICmdList;

private:
    FMemStackBase ParametersData;      // スカラーデータ用
    FMemStackBase Parameters;          // パラメータメタデータ用
    FMemStackBase ResourceParameters;  // リソースパラメータ用
    FMemStackBase BindlessParameters;  // バインドレスパラメータ用

    // ページサイズオプション
    enum class ERHIBatchedShaderParameterAllocatorPageSize
    {
        Small,  // 小さいバッチ向け
        Large   // 大きいバッチ向け
    };
};
```

**特徴**:
- ページベースのメモリ管理
- 自動配列拡張
- 排他的アタッチ検証（デバッグ時）

### 2.5 FRHIBatchedShaderParameters

**場所**: `RHIShaderParameters.h:240`

パラメータをバッチで設定するためのコレクション。

```cpp
struct FRHIBatchedShaderParameters
{
    FRHIBatchedShaderParametersAllocator& Allocator;
    TArrayView<uint8> ParametersData;
    TArrayView<FRHIShaderParameter> Parameters;
    TArrayView<FRHIShaderParameterResource> ResourceParameters;
    TArrayView<FRHIShaderParameterResource> BindlessParameters;

    // パラメータ設定API
    void SetShaderParameter(uint32 BufferIndex, uint32 BaseIndex,
                           uint32 NumBytes, const void* NewValue);
    void SetShaderUniformBuffer(uint32 Index, FRHIUniformBuffer* UBO);
    void SetShaderTexture(uint32 Index, FRHITexture* Texture);
    void SetShaderResourceViewParameter(uint32 Index, FRHIShaderResourceView* SRV);
    void SetShaderSampler(uint32 Index, FRHISamplerState* State);
    void SetUAVParameter(uint32 Index, FRHIUnorderedAccessView* UAV);
    void SetResourceCollection(uint32 Index, FRHIResourceCollection* Collection);

    // バインドレスAPI
    void SetBindlessTexture(uint32 Index, FRHITexture* Texture);
    void SetBindlessResourceView(uint32 Index, FRHIShaderResourceView* SRV);
    void SetBindlessSampler(uint32 Index, FRHISamplerState* State);
    void SetBindlessUAV(uint32 Index, FRHIUnorderedAccessView* UAV);
    void SetBindlessResourceCollection(uint32 Index, FRHIResourceCollection* RC);

    // 状態管理
    bool HasParameters() const;
    void Finish();  // 完了マーク
    void Reset();   // リセット
};
```

**使用例**:
```cpp
void SetShaderParameters(FRHIComputeCommandList& RHICmdList,
                        FRHIComputeShader* Shader)
{
    FRHIBatchedShaderParametersAllocator Allocator(
        nullptr, RHICmdList,
        ERHIBatchedShaderParameterAllocatorPageSize::Small);

    FRHIBatchedShaderParameters BatchParams(Allocator);

    // パラメータ設定
    float TimeValue = 0.5f;
    BatchParams.SetShaderParameter(0, 0, sizeof(float), &TimeValue);
    BatchParams.SetShaderTexture(0, MyTexture);
    BatchParams.SetShaderSampler(0, MySampler);

    // 完了
    BatchParams.Finish();

    // RHIに適用
    RHICmdList.SetBatchedShaderParameters(Shader, BatchParams);
}
```

### 2.6 FRHIBatchedShaderUnbinds

**場所**: `RHIShaderParameters.h:373`

シェーダーリソースのアンバインドをバッチで処理。

```cpp
struct FRHIBatchedShaderUnbinds
{
    TArray<FRHIShaderParameterUnbind> Unbinds;

    bool HasParameters() const;
    void Reset();
    void UnsetSRV(uint32 Index);
    void UnsetUAV(uint32 Index);
};
```

---

## 3. Shader Bundle ディスパッチ

### 3.1 FRHIShaderBundleComputeDispatch

**場所**: `RHIShaderParameters.h:397`

コンピュートシェーダーバンドルのディスパッチ情報。

```cpp
struct FRHIShaderBundleComputeDispatch
{
    uint32 RecordIndex = ~uint32(0u);
    class FComputePipelineState* PipelineState = nullptr;
    FRHIComputeShader* Shader = nullptr;
    FRHIWorkGraphShader* WorkGraphShader = nullptr;
    FRHIComputePipelineState* RHIPipeline = nullptr;
    TOptional<FRHIBatchedShaderParameters> Parameters;
    FUint32Vector4 Constants;

    inline bool IsValid() const { return RecordIndex != ~uint32(0u); }
};
```

### 3.2 FRHIShaderBundleGraphicsState

**場所**: `RHIShaderParameters.h:413`

グラフィックスシェーダーバンドルの状態情報。

```cpp
struct FRHIShaderBundleGraphicsState
{
    FIntRect ViewRect;
    float DepthMin = 0.0f;
    float DepthMax = 1.0f;
    float BlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    EPrimitiveType PrimitiveType = PT_TriangleList;
    uint8 StencilRef = 0;
};
```

### 3.3 FRHIShaderBundleGraphicsDispatch

**場所**: `RHIShaderParameters.h:427`

グラフィックスシェーダーバンドルのディスパッチ情報。

```cpp
struct FRHIShaderBundleGraphicsDispatch
{
    uint32 RecordIndex = ~uint32(0u);
    class FGraphicsPipelineState* PipelineState = nullptr;
    FRHIGraphicsPipelineState* RHIPipeline = nullptr;
    FGraphicsPipelineStateInitializer PipelineInitializer;

    // メッシュ/頂点シェーダー用パラメータ
    TOptional<FRHIBatchedShaderParameters> Parameters_MSVS;
    // ピクセルシェーダー用パラメータ
    TOptional<FRHIBatchedShaderParameters> Parameters_PS;

    FUint32Vector4 Constants;

    inline bool IsValid() const { return RecordIndex != ~uint32(0u); }
};
```

---

## 4. タイムスタンプとクエリシステム

### 4.1 FRHITimestampCalibrationQuery

**場所**: `RHIResources.h:2377`

GPU/CPUタイムスタンプのキャリブレーション用クエリ。

```cpp
#if (RHI_NEW_GPU_PROFILER == 0)
class FRHITimestampCalibrationQuery : public FRHIResource
{
public:
    FRHITimestampCalibrationQuery()
        : FRHIResource(RRT_TimestampCalibrationQuery) {}

    // GPU毎のタイムスタンプ値（マイクロ秒）
    uint64 GPUMicroseconds[MAX_NUM_GPUS] = {};
    uint64 CPUMicroseconds[MAX_NUM_GPUS] = {};
};
#endif
```

**用途**:
- GPU/CPUタイミング同期
- プロファイリングの精度向上
- フレームタイミング解析

### 4.2 FRHIRenderQueryPool

**場所**: `RHIResources.h:2479`

レンダークエリのプール管理システム。

```cpp
class FRHIRenderQueryPool : public FRHIResource
{
public:
    FRHIRenderQueryPool() : FRHIResource(RRT_RenderQueryPool) {}
    virtual ~FRHIRenderQueryPool() {}

    // クエリ取得
    virtual FRHIPooledRenderQuery AllocateQuery() = 0;

private:
    friend class FRHIPooledRenderQuery;
    // クエリ返却
    virtual void ReleaseQuery(TRefCountPtr<FRHIRenderQuery>&& Query) = 0;
};
```

### 4.3 FRHIPooledRenderQuery

**場所**: `RHIResources.h:2451`

プールから取得したレンダークエリのRAIIラッパー。

```cpp
class FRHIPooledRenderQuery
{
    TRefCountPtr<FRHIRenderQuery> Query;
    FRHIRenderQueryPool* QueryPool = nullptr;

public:
    FRHIPooledRenderQuery() = default;
    FRHIPooledRenderQuery(FRHIRenderQueryPool* InQueryPool,
                         TRefCountPtr<FRHIRenderQuery>&& InQuery);
    ~FRHIPooledRenderQuery();

    // コピー禁止、ムーブのみ許可
    FRHIPooledRenderQuery(const FRHIPooledRenderQuery&) = delete;
    FRHIPooledRenderQuery(FRHIPooledRenderQuery&&) = default;

    bool IsValid() const { return Query.IsValid(); }
    FRHIRenderQuery* GetQuery() const { return Query; }
    void ReleaseQuery();
};
```

**使用例**:
```cpp
void ProfileGPUWork(FRHIRenderQueryPool* Pool, FRHICommandList& RHICmdList)
{
    // プールからクエリ取得
    FRHIPooledRenderQuery BeginQuery = Pool->AllocateQuery();
    FRHIPooledRenderQuery EndQuery = Pool->AllocateQuery();

    // タイムスタンプ記録
    RHICmdList.EndRenderQuery(BeginQuery.GetQuery());

    // ... GPU作業 ...

    RHICmdList.EndRenderQuery(EndQuery.GetQuery());

    // スコープ終了時に自動返却
}
```

---

## 5. テクスチャリファレンスシステム

### 5.1 FRHITextureReference

**場所**: `RHITextureReference.h:7`

テクスチャへの間接参照を提供するクラス。実行時にターゲットテクスチャを変更可能。

```cpp
class FRHITextureReference : public FRHITexture
{
public:
    FRHITextureReference() = delete;
    FRHITextureReference(FRHITexture* InReferencedTexture);

#if PLATFORM_SUPPORTS_BINDLESS_RENDERING
    FRHITextureReference(FRHITexture* InReferencedTexture,
                        FRHIDescriptorHandle InBindlessHandle);
#endif

    ~FRHITextureReference();

    // 参照されているテクスチャを取得
    inline FRHITexture* GetReferencedTexture() const
    {
        return ReferencedTexture.GetReference();
    }

    // デフォルトテクスチャ（黒テクスチャ）
    static inline FRHITexture* GetDefaultTexture()
    {
        return DefaultTexture;
    }

#if PLATFORM_SUPPORTS_BINDLESS_RENDERING
    FRHIDescriptorHandle GetBindlessHandle() const { return BindlessHandle; }
    bool IsBindless() const { return GetBindlessHandle().IsValid(); }
#endif

protected:
    friend class FDynamicRHI;

    // FDynamicRHI::RHIUpdateTextureReference からのみ呼び出し
    void SetReferencedTexture(FRHITexture* InTexture)
    {
        ReferencedTexture = InTexture;
    }

    TRefCountPtr<FRHITexture> ReferencedTexture;

#if PLATFORM_SUPPORTS_BINDLESS_RENDERING
    const FRHIDescriptorHandle BindlessHandle;
#endif

    // FBlackTextureWithSRV によって設定される
    static TRefCountPtr<FRHITexture> DefaultTexture;
};
```

**用途**:
- ストリーミングテクスチャの実行時切り替え
- LODテクスチャの動的変更
- プレースホルダーテクスチャからのスムーズな遷移

**使用例**:
```cpp
// テクスチャリファレンス作成（初期状態はプレースホルダー）
FRHITextureReference* TexRef = RHI->CreateTextureReference(PlaceholderTexture);

// マテリアルに設定
Material->SetTextureParameterValue("Albedo", TexRef);

// 後でストリーミング完了時にテクスチャを更新
// （シェーダーの再バインドなしで反映される）
RHI->UpdateTextureReference(TexRef, StreamedTexture);
```

---

## 6. イミュータブルサンプラー

### 6.1 FImmutableSamplerState

**場所**: `RHIImmutableSamplerState.h:16`

PSO（パイプライン状態オブジェクト）に組み込まれるイミュータブルサンプラー。

```cpp
enum { MaxImmutableSamplers = 2 };

struct FImmutableSamplerState
{
    using TImmutableSamplers = TStaticArray<FRHISamplerState*, MaxImmutableSamplers>;

    FImmutableSamplerState()
        : ImmutableSamplers(InPlace, nullptr)
    {}

    void Reset()
    {
        for (uint32 Index = 0; Index < MaxImmutableSamplers; ++Index)
        {
            ImmutableSamplers[Index] = nullptr;
        }
    }

    bool operator==(const FImmutableSamplerState& rhs) const
    {
        return ImmutableSamplers == rhs.ImmutableSamplers;
    }

    TImmutableSamplers ImmutableSamplers;
};
```

**特徴**:
- 最大2個のサンプラーを保持
- PSO作成時に固定
- シェーダーからの動的変更不可
- パフォーマンス最適化に寄与

**用途**:
- シャドウマップサンプリング
- 標準的なテクスチャフィルタリング
- 描画コール間で変化しないサンプラー

---

## 7. リソースリプレースシステム

### 7.1 FRHIResourceReplaceInfo

**場所**: `RHIResourceReplace.h:11`

リソースのホットスワップ情報を格納。

```cpp
class FRHIResourceReplaceInfo
{
public:
    template <typename TType>
    struct TPair
    {
        TType* Dst;  // 置換先
        TType* Src;  // 置換元
    };

    typedef TPair<FRHIBuffer>             FBuffer;
    typedef TPair<FRHIRayTracingGeometry> FRTGeometry;

    typedef TVariant<FBuffer, FRTGeometry> TStorage;

    enum class EType : uint8
    {
        Buffer     = TStorage::IndexOfType<FBuffer>(),
        RTGeometry = TStorage::IndexOfType<FRTGeometry>(),
    };

    EType GetType() const;
    FBuffer const& GetBuffer() const;
    FRTGeometry const& GetRTGeometry() const;
};
```

### 7.2 FRHIResourceReplaceBatcher

**場所**: `RHIResourceReplace.h:54`

複数のリソース置換をバッチ処理。

```cpp
class FRHIResourceReplaceBatcher
{
    FRHICommandListBase& RHICmdList;
    TArray<FRHIResourceReplaceInfo> Infos;

public:
    FRHIResourceReplaceBatcher(FRHICommandListBase& RHICmdList,
                               uint32 InitialCapacity = 0);
    ~FRHIResourceReplaceBatcher();  // デストラクタでバッチ実行

    template <typename... TArgs>
    void EnqueueReplace(TArgs&&... Args)
    {
        Infos.Emplace(Forward<TArgs>(Args)...);
    }
};
```

**使用例**:
```cpp
void ReplaceStreamedResources(FRHICommandListBase& RHICmdList)
{
    FRHIResourceReplaceBatcher Batcher(RHICmdList, 3);

    // バッファ置換をエンキュー
    Batcher.EnqueueReplace(OldBuffer, NewBuffer);
    Batcher.EnqueueReplace(OldBuffer2, NewBuffer2);

    // レイトレーシングジオメトリ置換
    Batcher.EnqueueReplace(OldRTGeometry, NewRTGeometry);

    // スコープ終了時にバッチ実行
}
```

---

## 8. ビューキャッシュシステム

### 8.1 FRHITextureViewCache

**場所**: `RHIResources.h:5784`

テクスチャビュー（SRV/UAV）のキャッシュ。

```cpp
class FRHITextureViewCache
{
    // キャッシュされたビューの管理
    // ビュー作成コストの削減
};
```

### 8.2 FRHIBufferViewCache

**場所**: `RHIResources.h:5805`

バッファビュー（SRV/UAV）のキャッシュ。

```cpp
class FRHIBufferViewCache
{
    // キャッシュされたバッファビューの管理
    // 同一パラメータでの再作成を回避
};
```

---

## 9. イベントとデリゲート

### 9.1 FRHIPanicEvent

**場所**: `RHI.h:606`

RHIパニック時に呼び出されるデリゲート。

```cpp
DECLARE_DELEGATE_OneParam(FRHIPanicEvent, const FName&);
```

**用途**:
- 致命的RHIエラーの通知
- クラッシュレポート生成
- 緊急シャットダウン処理

### 9.2 FPipelineStateLoggedEvent

**場所**: `PipelineFileCache.h:370`

PSO記録時のイベント。

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FPipelineStateLoggedEvent,
                                    const FPipelineCacheFileFormatPSO&);
```

**用途**:
- PSOキャッシュの更新通知
- シェーダーコンパイル統計
- ウォームアップ進捗追跡

---

## 10. GPU フレームタイム履歴

### 10.1 FRHIGPUFrameTimeHistory

**場所**: `GPUProfiler.h:938`

GPUフレームタイムの履歴を追跡するクラス。

```cpp
class FRHIGPUFrameTimeHistory
{
    // フレームタイムの履歴バッファ
    // 移動平均計算
    // 統計情報提供
};
```

**用途**:
- パフォーマンス分析
- フレームレート安定性監視
- ボトルネック検出

---

## 11. ベストプラクティス

### 11.1 バッチパラメータ設定

```cpp
// 推奨: バッチ化されたパラメータ設定
void SetupShaderParametersBatched(FRHIComputeCommandList& RHICmdList)
{
    FRHIBatchedShaderParametersAllocator Allocator(
        nullptr, RHICmdList,
        ERHIBatchedShaderParameterAllocatorPageSize::Large);

    FRHIBatchedShaderParameters Params(Allocator);

    // 複数のパラメータを一括設定
    Params.SetShaderTexture(0, Texture0);
    Params.SetShaderTexture(1, Texture1);
    Params.SetShaderSampler(0, Sampler);
    Params.SetUAVParameter(0, OutputUAV);

    Params.Finish();
    RHICmdList.SetBatchedShaderParameters(Shader, Params);
}

// 非推奨: 個別のパラメータ設定
void SetupShaderParametersIndividual(FRHIComputeCommandList& RHICmdList)
{
    // 複数のRHIコマンドが発行される
    RHICmdList.SetShaderTexture(Shader, 0, Texture0);
    RHICmdList.SetShaderTexture(Shader, 1, Texture1);
    RHICmdList.SetShaderSampler(Shader, 0, Sampler);
    RHICmdList.SetShaderUAV(Shader, 0, OutputUAV);
}
```

### 11.2 フェンスのバッチ化

```cpp
// 推奨: スコープフェンスでバッチ化
void UpdateMultipleResources(FRHICommandListBase& RHICmdList)
{
    FRHICommandListScopedFence ScopedFence(RHICmdList);

    for (int i = 0; i < NumResources; ++i)
    {
        Resources[i]->Update();  // 個別フェンスは発行されない
    }
    // 1回のフェンスで完了
}

// 非推奨: 個別フェンス
void UpdateMultipleResourcesIndividual(FRHICommandListBase& RHICmdList)
{
    for (int i = 0; i < NumResources; ++i)
    {
        Resources[i]->Update();  // 毎回フェンス発行
    }
    // N回のフェンス = パフォーマンス低下
}
```

### 11.3 クエリプールの活用

```cpp
// 推奨: プールからクエリ取得
void ProfileWithPool(FRHIRenderQueryPool* Pool)
{
    // プールから取得（再利用される）
    FRHIPooledRenderQuery Query = Pool->AllocateQuery();
    // ... 使用 ...
    // 自動返却
}

// 非推奨: 毎回新規作成
void ProfileWithoutPool(FRHICommandList& RHICmdList)
{
    // 毎回新規作成（高コスト）
    FRHIRenderQuery* Query = RHI->CreateRenderQuery(RQT_AbsoluteTime);
    // ... 使用 ...
    // 明示的な解放が必要
}
```

---

## まとめ

Part 15では以下の機能を文書化しました：

1. **コマンドリストスコープユーティリティ** - フェンス、パイプライン、遷移のRAII管理
2. **バッチシェーダーパラメータシステム** - 効率的なパラメータ設定
3. **Shader Bundleディスパッチ** - コンピュート/グラフィックスバンドル
4. **タイムスタンプとクエリシステム** - GPUタイミング計測
5. **テクスチャリファレンス** - 間接テクスチャ参照
6. **イミュータブルサンプラー** - PSO固定サンプラー
7. **リソースリプレースシステム** - ホットスワップ対応
8. **ビューキャッシュ** - SRV/UAVキャッシュ
9. **イベント/デリゲート** - パニック/PSOログイベント
10. **GPUフレームタイム履歴** - パフォーマンス追跡

これらの機能を適切に活用することで、RHIレベルでのパフォーマンス最適化が可能になります。

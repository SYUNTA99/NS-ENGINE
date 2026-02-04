# RHI 設計原則 - 詳細ガイド

## 概要

RHI（Render Hardware Interface）は、Unreal Engineのグラフィックス抽象化層です。
本ドキュメントでは、RHIの設計を支える根本的な原則と、それらがどのように実装に
反映されているかを詳細に解説します。

---

## 1. 基本設計哲学

### 1.1 最小公約数ではなく最大公倍数

**原則**: RHIは各プラットフォームの「最小限の共通機能」ではなく、
「すべてのプラットフォームの機能を統合した最大セット」を提供する。

```
┌─────────────────────────────────────────────────────────┐
│                    RHI API Surface                       │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │
│  │ D3D12   │  │ Vulkan  │  │ Metal   │  │ OpenGL  │    │
│  │ 機能    │  │ 機能    │  │ 機能    │  │ 機能    │    │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘    │
│                                                          │
│  最小公約数アプローチ: 全APIで使える機能のみ ✗          │
│  最大公倍数アプローチ: 全APIの機能を統合 ✓              │
└─────────────────────────────────────────────────────────┘
```

**実装例**:
```cpp
// ケイパビリティクエリで機能の有無を確認
if (GRHISupportsRayTracing)
{
    // レイトレーシング使用
}
else
{
    // フォールバックパス
}

// 機能レベルによる分岐
if (GRHIGlobals.RayTracing.Tier >= ERayTracingTier::Tier1_1)
{
    // 高度なRT機能使用
}
```

**理由**:
- ハイエンド機能を諦めない
- プラットフォーム固有の最適化が可能
- 将来の拡張に対応しやすい

### 1.2 ゼロコスト抽象化の追求

**原則**: 抽象化レイヤーのオーバーヘッドを可能な限り排除する。

```cpp
// 悪い例: 仮想関数による間接呼び出し
class IRHICommandContext
{
    virtual void Draw(uint32 VertexCount) = 0;  // vtable lookup
};

// 良い例: CRTP（Curiously Recurring Template Pattern）
template<typename DerivedType>
class TRHICommandContext
{
    void Draw(uint32 VertexCount)
    {
        // コンパイル時にインライン化される
        static_cast<DerivedType*>(this)->DrawImpl(VertexCount);
    }
};
```

**RHIでの実装**:
```cpp
// コマンドは構造体として定義され、仮想関数を使わない
struct FRHICommandDraw
{
    uint32 VertexCountPerInstance;
    uint32 InstanceCount;
    uint32 StartVertexLocation;
    uint32 StartInstanceLocation;

    // 実行関数はstaticで、vtableなし
    static void Execute(FRHICommandListBase& CmdList, const FRHICommandDraw& Cmd);
};
```

### 1.3 明示的制御の優先

**原則**: 暗黙の動作より明示的な指定を優先する。

```cpp
// 悪い例: 暗黙のリソース状態遷移
void DrawMesh(FRHITexture* Texture)
{
    // ドライバーが勝手に状態遷移（非効率、予測不能）
    SetTexture(0, Texture);
    Draw();
}

// 良い例: 明示的なバリア
void DrawMesh(FRHICommandList& RHICmdList, FRHITexture* Texture)
{
    // 明示的な状態遷移
    RHICmdList.Transition(FRHITransitionInfo(
        Texture,
        ERHIAccess::CopyDest,      // 前の状態
        ERHIAccess::SRVGraphics    // 新しい状態
    ));

    RHICmdList.SetTexture(0, Texture);
    RHICmdList.Draw();
}
```

**利点**:
- パフォーマンスの予測可能性
- デバッグの容易さ
- 最適化の余地

---

## 2. リソース管理原則

### 2.1 参照カウントによるライフタイム管理

**原則**: GPUリソースは参照カウントで管理し、安全な破棄を保証する。

```cpp
// TRefCountPtr による自動管理
class FRHIResource
{
private:
    mutable uint32 RefCount = 0;

public:
    uint32 AddRef() const
    {
        return ++RefCount;
    }

    uint32 Release() const
    {
        uint32 NewCount = --RefCount;
        if (NewCount == 0)
        {
            // 遅延削除キューに追加（GPU使用中の可能性）
            EnqueueDeferredDeletion(this);
        }
        return NewCount;
    }
};

// 使用例
void UseTexture()
{
    TRefCountPtr<FRHITexture> Texture = CreateTexture();
    // Texture はスコープ終了時に自動解放
    // ただしGPU使用中なら遅延削除
}
```

### 2.2 遅延削除パターン

**原則**: リソース削除はGPUがリソースを使い終わるまで遅延する。

```
フレーム N:     リソース使用 → Release() 呼び出し
                              ↓
                      遅延削除キューに追加
                              ↓
フレーム N+1:   GPUがフレームN完了を待機
                              ↓
フレーム N+2:   実際のリソース解放
```

```cpp
// 遅延削除の実装概念
class FDeferredDeletionQueue
{
    struct FPendingDelete
    {
        FRHIResource* Resource;
        uint64 FenceValue;  // この値を超えたら削除可能
    };

    TArray<FPendingDelete> PendingDeletes;

    void Enqueue(FRHIResource* Resource)
    {
        PendingDeletes.Add({Resource, GetCurrentFenceValue()});
    }

    void ProcessDeletions()
    {
        uint64 CompletedFence = GetCompletedFenceValue();

        for (int32 i = PendingDeletes.Num() - 1; i >= 0; --i)
        {
            if (PendingDeletes[i].FenceValue <= CompletedFence)
            {
                delete PendingDeletes[i].Resource;
                PendingDeletes.RemoveAtSwap(i);
            }
        }
    }
};
```

### 2.3 Transient（一時的）リソースの最適化

**原則**: フレーム内でのみ使用するリソースはメモリを共有（エイリアシング）する。

```
フレーム内のリソースタイムライン:

時間 →
┌────────────┐
│ Shadow Map │  使用期間: T0-T2
└────────────┘
              ┌────────────┐
              │ G-Buffer A │  使用期間: T2-T4
              └────────────┘
                            ┌────────────┐
                            │ G-Buffer B │  使用期間: T4-T6
                            └────────────┘

メモリ共有:
┌─────────────────────────────────────────┐
│           同一メモリ領域                 │
│  Shadow Map → G-Buffer A → G-Buffer B   │
└─────────────────────────────────────────┘
```

```cpp
// Transient リソースの使用
void RenderFrame(FRHICommandList& RHICmdList)
{
    // Transient テクスチャ取得（メモリ再利用）
    FRHITransientTexture* ShadowMap = TransientAllocator->CreateTexture(
        ShadowMapDesc, TEXT("ShadowMap"));

    // シャドウマップ描画
    RenderShadows(RHICmdList, ShadowMap);

    // 使用完了宣言 → メモリ再利用可能に
    TransientAllocator->DeallocateMemory(ShadowMap);

    // 同じメモリを別のテクスチャとして再利用
    FRHITransientTexture* GBuffer = TransientAllocator->CreateTexture(
        GBufferDesc, TEXT("GBuffer"));

    // ...
}
```

---

## 3. コマンドパターン

### 3.1 遅延実行モデル

**原則**: コマンドは記録時に実行されず、後で一括実行される。

```
記録フェーズ（ゲームスレッド/レンダースレッド）:
┌─────────────────────────────────────┐
│ CommandList.SetPipeline(PSO)        │ → メモリに記録
│ CommandList.SetVertexBuffer(VB)     │ → メモリに記録
│ CommandList.Draw(100)               │ → メモリに記録
└─────────────────────────────────────┘

実行フェーズ（RHIスレッド）:
┌─────────────────────────────────────┐
│ 記録されたコマンドを順次実行        │
│ → GPU に送信                        │
└─────────────────────────────────────┘
```

```cpp
// コマンド記録（メモリ確保のみ）
void RecordCommands(FRHICommandList& RHICmdList)
{
    // これらは即座にGPUに送信されない
    RHICmdList.SetGraphicsPipelineState(PSO);
    RHICmdList.SetVertexBuffer(0, VertexBuffer);
    RHICmdList.DrawPrimitive(0, TriangleCount, 1);
}

// 実行（RHIスレッドまたはタスク）
void ExecuteCommands(FRHICommandListImmediate& RHICmdList)
{
    // ここでGPUにコマンドを送信
    RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
}
```

### 3.2 コマンドメモリ管理

**原則**: コマンドはリニアアロケータで高速に確保する。

```cpp
// リニアアロケータ（フレーム単位でリセット）
class FRHICommandListMemory
{
    uint8* CurrentPage;
    uint8* PageEnd;
    TArray<uint8*> Pages;

public:
    void* Alloc(size_t Size, size_t Alignment)
    {
        // アライメント調整
        uint8* Aligned = Align(CurrentPage, Alignment);

        if (Aligned + Size > PageEnd)
        {
            // 新しいページ確保
            AllocateNewPage();
            Aligned = CurrentPage;
        }

        CurrentPage = Aligned + Size;
        return Aligned;
    }

    void Reset()
    {
        // フレーム終了時：ページを再利用
        CurrentPage = Pages[0];
        PageEnd = CurrentPage + PageSize;
    }
};

// コマンド確保マクロ
#define ALLOC_COMMAND(CommandType) \
    new (CmdList.AllocCommand(sizeof(CommandType), alignof(CommandType))) CommandType
```

### 3.3 Bypass パターン

**原則**: 即時実行が必要な場合は遅延キューをバイパスできる。

```cpp
// 通常パス: 遅延実行
void DrawDeferred(FRHICommandList& RHICmdList)
{
    RHICmdList.Draw(...);  // キューに記録
}

// バイパスパス: 即時実行
void DrawImmediate(FRHICommandListImmediate& RHICmdList)
{
    // 即時フラッシュしてから実行
    RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);

    // コンテキスト直接操作
    IRHICommandContext* Context = RHICmdList.GetContext();
    Context->RHIDraw(...);  // 即座にGPU送信
}
```

---

## 4. スレッディングモデル

### 4.1 スレッド分離原則

**原則**: ゲームスレッド、レンダースレッド、RHIスレッドを明確に分離する。

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ Game Thread │───→│Render Thread│───→│ RHI Thread  │───→ GPU
└─────────────┘    └─────────────┘    └─────────────┘
     │                   │                   │
     │ シーン更新        │ 可視性計算        │ APIコール
     │ 入力処理          │ 描画コマンド生成  │ リソース管理
     │ AI/物理           │ カリング          │ GPU同期
     └───────────────────┴───────────────────┘
            それぞれ独立して並列実行
```

### 4.2 コマンドリストの並列記録

**原則**: 複数のコマンドリストを並列に記録し、後でマージする。

```cpp
// 並列コマンドリスト記録
void RenderSceneParallel(FRHICommandListImmediate& RHICmdList)
{
    TArray<FRHICommandList*> ParallelLists;

    // 並列タスク起動
    ParallelFor(NumChunks, [&](int32 ChunkIndex)
    {
        // 各タスクで独立したコマンドリストに記録
        FRHICommandList* CmdList = new FRHICommandList(
            FRHIGPUMask::All());

        RenderChunk(ChunkIndex, *CmdList);

        CmdList->FinishRecording();
        ParallelLists[ChunkIndex] = CmdList;
    });

    // メインリストにマージ
    for (FRHICommandList* CmdList : ParallelLists)
    {
        RHICmdList.QueueAsyncCommandListSubmit(CmdList);
    }
}
```

### 4.3 パイプライン並列性

**原則**: Graphics と AsyncCompute パイプラインは並列実行できる。

```
GPU実行タイムライン:

Graphics Queue:  ├─Draw A─┼─Draw B─┼─Draw C─┤
                                   ↓ 同期点
AsyncCompute:           ├─Compute X─┼─Compute Y─┤

時間 →

両キューは独立して進行し、明示的な同期点でのみ待機
```

```cpp
void RenderWithAsyncCompute(FRHICommandListImmediate& RHICmdList)
{
    // グラフィックス作業開始
    BeginShadowRendering(RHICmdList);

    // 非同期コンピュート開始（並列実行）
    FRHIAsyncComputeCommandListImmediate& AsyncCmdList =
        FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();

    AsyncCmdList.BeginAsyncCompute();
    DispatchCompute(AsyncCmdList);
    AsyncCmdList.EndAsyncCompute();

    // グラフィックス続行
    EndShadowRendering(RHICmdList);

    // 同期（コンピュート結果が必要な場合）
    RHICmdList.WaitForAsyncCompute();
}
```

---

## 5. 状態管理原則

### 5.1 明示的状態追跡

**原則**: リソース状態はRHI側で明示的に追跡し、必要な遷移を自動挿入しない。

```cpp
// 状態追跡構造体
struct FRHITrackedResource
{
    ERHIAccess CurrentAccess = ERHIAccess::Unknown;
    ERHIPipeline CurrentPipeline = ERHIPipeline::Graphics;
    uint32 SubresourceStates[MaxMipLevels * MaxArraySlices];
};

// 明示的な遷移要求
void TransitionResource(FRHICommandList& RHICmdList,
                       FRHITexture* Texture,
                       ERHIAccess OldAccess,
                       ERHIAccess NewAccess)
{
    // 遷移が必要か検証
    if (OldAccess != NewAccess)
    {
        RHICmdList.Transition(FRHITransitionInfo(
            Texture, OldAccess, NewAccess));
    }
}
```

### 5.2 不変状態の焼き付け

**原則**: 変更されない状態はPSOに焼き付けてオーバーヘッドを削減する。

```cpp
// パイプライン状態オブジェクト（不変）
struct FGraphicsPipelineStateInitializer
{
    // シェーダー（不変）
    FRHIVertexShader* VertexShader;
    FRHIPixelShader* PixelShader;

    // ラスタライザ状態（不変）
    FRHIRasterizerState* RasterizerState;

    // ブレンド状態（不変）
    FRHIBlendState* BlendState;

    // 深度ステンシル状態（不変）
    FRHIDepthStencilState* DepthStencilState;

    // これらは作成後変更不可 → GPU最適化可能
};

// 動的状態は別途設定
void SetDynamicState(FRHICommandList& RHICmdList)
{
    // ビューポート（動的）
    RHICmdList.SetViewport(0, 0, Width, Height, 0, 1);

    // シザー（動的）
    RHICmdList.SetScissorRect(true, 0, 0, Width, Height);

    // ステンシル参照値（動的）
    RHICmdList.SetStencilRef(StencilRef);
}
```

### 5.3 バインディングモデル

**原則**: リソースバインディングは軽量で頻繁な変更に対応する。

```
従来のバインディング:
┌─────────────────────────────────────┐
│ Slot 0: Texture A                   │
│ Slot 1: Texture B                   │
│ Slot 2: Buffer C                    │
│ ...                                 │
└─────────────────────────────────────┘
変更のたびにディスクリプタ更新

バインドレスバインディング:
┌─────────────────────────────────────┐
│ Global Descriptor Heap              │
│ [0] [1] [2] [3] [4] [5] [6] ...    │
│  ↑                                  │
│ インデックスでアクセス              │
└─────────────────────────────────────┘
シェーダーがインデックスで直接参照
```

```cpp
// バインドレスの使用
void DrawBindless(FRHICommandList& RHICmdList)
{
    // ディスクリプタハンドルを取得
    FRHIDescriptorHandle TextureHandle = Texture->GetDefaultBindlessHandle();

    // シェーダーにインデックスを渡す
    ShaderParams.TextureIndex = TextureHandle.GetIndex();
    RHICmdList.SetBatchedShaderParameters(Shader, ShaderParams);

    // 描画（バインディング変更なし）
    RHICmdList.Draw(...);
}
```

---

## 6. 同期プリミティブ

### 6.1 フェンスの階層

**原則**: 同期の粒度に応じた複数レベルのフェンスを提供する。

```
┌─────────────────────────────────────────────────────┐
│ Level 1: フレームフェンス                           │
│   フレーム全体の完了を待機                          │
│   用途: スワップチェーン、リソース削除              │
├─────────────────────────────────────────────────────┤
│ Level 2: パイプラインフェンス                       │
│   特定パイプラインの完了を待機                      │
│   用途: Graphics ↔ AsyncCompute 同期               │
├─────────────────────────────────────────────────────┤
│ Level 3: コマンドリストフェンス                     │
│   特定コマンドリストの完了を待機                    │
│   用途: CPU読み取り、マップ操作                     │
├─────────────────────────────────────────────────────┤
│ Level 4: メモリバリア                               │
│   特定リソースのキャッシュフラッシュ                │
│   用途: UAV ↔ SRV 遷移                             │
└─────────────────────────────────────────────────────┘
```

### 6.2 バリアバッチング

**原則**: 複数のバリアをバッチ化して効率を上げる。

```cpp
// 悪い例: 個別バリア
void TransitionsIndividual(FRHICommandList& RHICmdList)
{
    RHICmdList.Transition(FRHITransitionInfo(Texture1, ...));  // バリア1
    RHICmdList.Transition(FRHITransitionInfo(Texture2, ...));  // バリア2
    RHICmdList.Transition(FRHITransitionInfo(Buffer1, ...));   // バリア3
    // 3回のバリア発行
}

// 良い例: バッチバリア
void TransitionsBatched(FRHICommandList& RHICmdList)
{
    TArray<FRHITransitionInfo> Transitions;
    Transitions.Add(FRHITransitionInfo(Texture1, ...));
    Transitions.Add(FRHITransitionInfo(Texture2, ...));
    Transitions.Add(FRHITransitionInfo(Buffer1, ...));

    RHICmdList.Transition(Transitions);  // 1回のバリア発行
}
```

### 6.3 スプリットバリア

**原則**: バリアの開始と終了を分離して、間に他の作業を挿入できる。

```cpp
void UseSplitBarrier(FRHICommandList& RHICmdList)
{
    // バリア開始（遷移を開始するが完了を待たない）
    RHICmdList.BeginTransitions({
        FRHITransitionInfo(Texture, ERHIAccess::SRVGraphics, ERHIAccess::UAVCompute)
    });

    // バリア進行中に他の作業
    DoUnrelatedWork(RHICmdList);

    // バリア終了（遷移完了を待機）
    RHICmdList.EndTransitions();

    // テクスチャがUAVとして使用可能
    DispatchCompute(RHICmdList, Texture);
}
```

---

## 7. エラー処理原則

### 7.1 フェイルファスト

**原則**: 無効な状態は早期に検出してクラッシュさせる。

```cpp
// デバッグビルドでの検証
void SetTexture(uint32 Slot, FRHITexture* Texture)
{
#if DO_CHECK
    checkf(Texture != nullptr, TEXT("Null texture at slot %d"), Slot);
    checkf(Slot < MaxTextureSlots, TEXT("Invalid slot %d"), Slot);
    checkf(Texture->GetDesc().Flags & TexCreate_ShaderResource,
           TEXT("Texture not created with SRV flag"));
#endif

    // 実装
}
```

### 7.2 検証レイヤー

**原則**: デバッグ時は詳細な検証を行い、リリース時はスキップする。

```cpp
#if ENABLE_RHI_VALIDATION
class FValidationRHI : public FDynamicRHI
{
    void Draw(uint32 VertexCount, uint32 InstanceCount) override
    {
        // 状態検証
        ValidatePipelineState();
        ValidateResourceBindings();
        ValidateVertexBuffers();

        // 実際の描画
        WrappedRHI->Draw(VertexCount, InstanceCount);

        // 事後検証
        ValidateResourceStates();
    }

    void ValidatePipelineState()
    {
        RHI_VALIDATION_CHECK(CurrentPSO != nullptr,
            TEXT("No pipeline state bound"));
        RHI_VALIDATION_CHECK(CurrentPSO->IsValid(),
            TEXT("Invalid pipeline state"));
    }
};
#endif
```

### 7.3 GPU クラッシュ診断

**原則**: GPUクラッシュ時に原因特定のための情報を提供する。

```cpp
// Breadcrumb（パンくず）の埋め込み
void RenderPass(FRHICommandList& RHICmdList, const TCHAR* PassName)
{
    // パス開始マーカー
    RHICmdList.PushBreadcrumb(PassName);

    // 描画処理
    DrawObjects(RHICmdList);

    // パス終了マーカー
    RHICmdList.PopBreadcrumb();
}

// クラッシュ時の情報
void OnGPUCrash()
{
    // 最後に完了したBreadcrumbを報告
    UE_LOG(LogRHI, Error, TEXT("GPU Crash detected"));
    UE_LOG(LogRHI, Error, TEXT("Last completed: %s"),
           *Breadcrumbs.GetLastCompleted());
    UE_LOG(LogRHI, Error, TEXT("In progress: %s"),
           *Breadcrumbs.GetInProgress());
}
```

---

## 8. 拡張性原則

### 8.1 プラットフォーム拡張ポイント

**原則**: プラットフォーム固有機能を追加できる拡張ポイントを設ける。

```cpp
// 基底クラスで拡張ポイント定義
class FDynamicRHI
{
public:
    // 標準API
    virtual void RHIDraw(...) = 0;

    // 拡張API（オプション）
    virtual void* RHIGetNativeDevice() { return nullptr; }
    virtual void* RHIGetNativeCommandQueue() { return nullptr; }

    // プラットフォーム固有拡張
    virtual bool RHISupportsExtension(const FName& ExtensionName)
    {
        return false;
    }

    virtual void* RHIGetExtensionInterface(const FName& ExtensionName)
    {
        return nullptr;
    }
};

// D3D12固有拡張
class FD3D12DynamicRHI : public FDynamicRHI
{
    bool RHISupportsExtension(const FName& ExtensionName) override
    {
        if (ExtensionName == "D3D12_EnhancedBarriers")
            return bSupportsEnhancedBarriers;
        if (ExtensionName == "D3D12_WorkGraphs")
            return bSupportsWorkGraphs;
        return false;
    }
};
```

### 8.2 機能フラグによる条件分岐

**原則**: コンパイル時と実行時の両方で機能の有無を判定できる。

```cpp
// コンパイル時フラグ
#if PLATFORM_SUPPORTS_MESH_SHADERS
    // メッシュシェーダー対応コード
#endif

// 実行時フラグ
if (GRHISupportsMeshShaders)
{
    // ハードウェアがサポートしている場合
    DrawMeshShaderPipeline();
}
else
{
    // フォールバック
    DrawTraditionalPipeline();
}

// 機能ティア
switch (GRHIGlobals.RayTracing.Tier)
{
case ERayTracingTier::NotSupported:
    // RTなし
    break;
case ERayTracingTier::Tier1_0:
    // 基本RT
    break;
case ERayTracingTier::Tier1_1:
    // 高度なRT
    break;
}
```

---

## 9. パフォーマンス原則

### 9.1 CPUオーバーヘッド最小化

**原則**: API呼び出し回数を最小化する。

```cpp
// 悪い例: 多数のAPI呼び出し
void DrawManyObjects_Slow(FRHICommandList& RHICmdList)
{
    for (const FMesh& Mesh : Meshes)
    {
        RHICmdList.SetVertexBuffer(0, Mesh.VB);      // N回
        RHICmdList.SetIndexBuffer(Mesh.IB);          // N回
        RHICmdList.DrawIndexed(...);                  // N回
    }
    // 合計: 3N API呼び出し
}

// 良い例: インスタンシング
void DrawManyObjects_Fast(FRHICommandList& RHICmdList)
{
    RHICmdList.SetVertexBuffer(0, MergedVB);         // 1回
    RHICmdList.SetIndexBuffer(MergedIB);             // 1回
    RHICmdList.DrawIndexedInstanced(..., NumMeshes); // 1回
    // 合計: 3 API呼び出し
}

// さらに良い例: Indirect描画
void DrawManyObjects_Fastest(FRHICommandList& RHICmdList)
{
    RHICmdList.SetVertexBuffer(0, MergedVB);
    RHICmdList.SetIndexBuffer(MergedIB);
    RHICmdList.DrawIndexedIndirect(IndirectArgsBuffer, NumDraws);
    // GPUが描画パラメータを決定
}
```

### 9.2 GPUアイドル時間の排除

**原則**: GPUが常に作業を持つようにスケジューリングする。

```
悪い例（GPUアイドルあり）:
CPU: ├─Record─┼─Wait─┼─Record─┼─Wait─┤
GPU:          ├─Execute─┤    ├─Execute─┤
                    ↑ アイドル

良い例（パイプライン化）:
CPU: ├─Frame N─┼─Frame N+1─┼─Frame N+2─┤
GPU:     ├─Frame N-1─┼─Frame N─┼─Frame N+1─┤
     常にGPUに作業がある
```

### 9.3 メモリ帯域幅の最適化

**原則**: データレイアウトとアクセスパターンを最適化する。

```cpp
// キャッシュフレンドリーなデータレイアウト
struct FVertexOptimized
{
    // 頻繁にアクセスするデータを先頭に
    FVector3f Position;     // 12 bytes
    FVector3f Normal;       // 12 bytes
    // 8バイトアライメントで合計32バイト（キャッシュライン半分）

    FVector2f UV0;          // 8 bytes
    FVector2f UV1;          // 8 bytes
    // 合計48バイト → パディングで64バイト（1キャッシュライン）
};

// 圧縮フォーマットの使用
struct FVertexCompressed
{
    FVector3f Position;     // 12 bytes
    uint32 PackedNormal;    // 4 bytes (10:10:10:2)
    uint16 UV0[2];          // 4 bytes (half)
    // 合計20バイト（38%削減）
};
```

---

## 10. 設計パターンまとめ

### 10.1 主要パターン一覧

| パターン | 用途 | RHIでの適用 |
|----------|------|-------------|
| **コマンド** | 遅延実行 | FRHICommand* |
| **ファクトリ** | リソース作成 | FDynamicRHI::RHICreate* |
| **ストラテジー** | プラットフォーム切り替え | FD3D12DynamicRHI等 |
| **フライウェイト** | リソース共有 | PSO キャッシュ |
| **RAII** | ライフタイム管理 | TRefCountPtr |
| **プール** | 再利用 | FRHIRenderQueryPool |
| **バッチ** | 効率化 | FRHIBatchedShaderParameters |

### 10.2 設計原則チェックリスト

新機能追加時のチェックリスト:

- [ ] ゼロコスト抽象化か？（vtable回避）
- [ ] 明示的制御か？（暗黙の動作なし）
- [ ] スレッドセーフか？
- [ ] 遅延削除に対応しているか？
- [ ] バッチ化可能か？
- [ ] 検証レイヤーでカバーされているか？
- [ ] プラットフォーム拡張可能か？
- [ ] パフォーマンス計測可能か？

---

## 結論

RHIの設計原則は以下の核心的な考え方に基づいています：

1. **パフォーマンス第一**: 抽象化のコストを最小化
2. **明示的制御**: 予測可能な動作
3. **拡張可能性**: プラットフォーム固有機能のサポート
4. **安全性**: 検証レイヤーとデバッグ支援
5. **並列性**: マルチスレッドとマルチパイプライン

これらの原則を理解することで、RHIを効果的に使用し、
高品質なグラフィックスコードを記述できます。

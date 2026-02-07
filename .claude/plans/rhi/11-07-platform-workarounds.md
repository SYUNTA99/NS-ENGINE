# 11-07: プラットフォーム回避策フラグ

## 目的

プラットフォームやドライバの固有のバグ・制限を回避するためのフラグシステムを定義する。

## 参照ドキュメント

- .claude/plans/rhi/docs/RHI_Implementation_Guide_Part11.md (1. プラットフォーム回避策フラグ)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIPlatformWorkarounds.h`

既存ファイル修正:
- `Source/Engine/RHI/Public/RHIGlobals.h` (フラグ追加)

## 回避策フラグの概念

```
プラットフォーム/ドライバのバグや制限
┌──────────────────────────────────────────────────────────────│
│ 一部のGPU/ドライバで特定の操作が正しく動作しない。         │
│ 例: 暗黙の状態遷移、特定フォーマットのサポート等）           │
└──────────────────────────────────────────────────────────────│
           │
           ▼
┌──────────────────────────────────────────────────────────────│
│ GRHINeed* フラグで回避策を有効化                           │
│ RHI初期化時にプラットフォーム/ドライバを検出して設定     │
└──────────────────────────────────────────────────────────────│
           │
           ▼
┌──────────────────────────────────────────────────────────────│
│ エンジンコードでフラグをチェックし、必要に応じて回避策適用  │
│ if (GRHINeedsExtraTransitions) { /* 追加遷移を発行 */ }     │
└──────────────────────────────────────────────────────────────│
```

## TODO

### 回避策フラグ構造体

```cpp
/// プラットフォーム回避策フラグ
struct RHIPlatformWorkarounds {
    //=========================================================================
    // リソース状態の遷移関連
    //=========================================================================

    /// COPYSRC/COPYDEST状態への追加遷移が必要。
    /// 一部RHIで暗黙の遷移がサポートされない
    bool needsExtraTransitions = false;

    /// Transient ResourceのDiscard状態追跡が必要。
    bool needsTransientDiscardStateTracking = false;

    /// AsyncCompute→GraphicsのDiscard遷移回避
    bool needsTransientDiscardOnGraphicsWorkaround = false;

    /// 非ピクセルシェーダーSRVの手動遷移が必要。
    /// 頂点/ジオメトリシェーダーからのSRVアクセスで暗黙の遷移が発生しない
    bool needsSRVGraphicsNonPixelWorkaround = false;

    //=========================================================================
    // リソース削除関連
    //=========================================================================

    /// リソース削除に追加の遅延が必要。
    /// 参照カウントでもGPU使用中の可能性がある
    bool needsExtraDeletionLatency = false;

    /// ストリーミングテクスチャの削除遅延を強制無効化
    bool forceNoDeletionLatencyForStreamingTextures = false;

    //=========================================================================
    // シェーダー関連
    //=========================================================================

    /// 明示的なシェーダーアンバインドが必要。
    /// シェーダー切り替え時に前のシェーダーが残留する
    bool needsShaderUnbinds = false;

    //=========================================================================
    // レンダリング関連
    //=========================================================================

    /// カスケードシャドウマップのアトラス化回避
    /// 一部GPUで深度比較が不正確になる
    bool needsUnatlasedCSMDepthsWorkaround = false;

    //=========================================================================
    // フォーマット関連
    //=========================================================================

    /// 特定フォーマットのエミュレーションが必要。
    bool needsBC4FormatEmulation = false;
    bool needsBC5FormatEmulation = false;

    //=========================================================================
    // 同期関連
    //=========================================================================

    /// フェンス値の追加マージンが必要。
    bool needsFenceValuePadding = false;

    /// コマンドリスト間の追加同期が必要。
    bool needsExplicitCommandListSync = false;
};
```

### グローバルアクセス

```cpp
/// グローバル回避策フラグ
extern RHIPlatformWorkarounds GRHIPlatformWorkarounds;

/// マクロアクセス (互換性のため)
#define GRHINeedsExtraTransitions \
    GRHIPlatformWorkarounds.needsExtraTransitions

#define GRHINeedsTransientDiscardStateTracking \
    GRHIPlatformWorkarounds.needsTransientDiscardStateTracking

#define GRHINeedsTransientDiscardOnGraphicsWorkaround \
    GRHIPlatformWorkarounds.needsTransientDiscardOnGraphicsWorkaround

#define GRHINeedsSRVGraphicsNonPixelWorkaround \
    GRHIPlatformWorkarounds.needsSRVGraphicsNonPixelWorkaround

#define GRHINeedsExtraDeletionLatency \
    GRHIPlatformWorkarounds.needsExtraDeletionLatency

#define GRHIForceNoDeletionLatencyForStreamingTextures \
    GRHIPlatformWorkarounds.forceNoDeletionLatencyForStreamingTextures

#define GRHINeedsShaderUnbinds \
    GRHIPlatformWorkarounds.needsShaderUnbinds

#define GRHINeedsUnatlasedCSMDepthsWorkaround \
    GRHIPlatformWorkarounds.needsUnatlasedCSMDepthsWorkaround
```

### 回避策フラグ詳細

#### needsExtraTransitions

```cpp
// 問題： 一部RHIでCOPYSRC/COPYDEST状態への暗黙の遷移がサポートされない
// 回避策： コピー操作前後に明示的な遷移を挿入

void CopyTexture(IRHITexture* src, IRHITexture* dst)
{
    if (GRHINeedsExtraTransitions) {
        // 明示的な遷移
        context->Transition(src, ERHIAccess::CopySource);
        context->Transition(dst, ERHIAccess::CopyDest);
    }

    context->CopyTexture(src, dst);

    if (GRHINeedsExtraTransitions) {
        // 元の状態に戻す
        context->Transition(src, ERHIAccess::SRVGraphics);
        context->Transition(dst, ERHIAccess::SRVGraphics);
    }
}
```

#### needsTransientDiscardOnGraphicsWorkaround

```cpp
// 問題： AsyncCompute→All→Graphicsパイプラインの
//       Discard遷移が正しく動作しない
// 回避策： エイリアシングを無効化

bool CanAliasResources(
    const FRHITransientAllocationFences& discard,
    const FRHITransientAllocationFences& acquire)
{
    if (GRHINeedsTransientDiscardOnGraphicsWorkaround &&
        discard.GetPipelines() == ERHIPipeline::All &&
        acquire.GetPipelines() == ERHIPipeline::AsyncCompute)
    {
        // エイリアシング不可
        return false;
    }
    return true;
}
```

#### needsSRVGraphicsNonPixelWorkaround

```cpp
// 問題： 頂点/ジオメトリシェーダーからのSRVアクセスで
//       暗黙の状態遷移が発生しない
// 回避策： 手動でSRVGraphicsNonPixelに遷移

void BindVertexShaderSRV(IRHITexture* texture)
{
    if (GRHINeedsSRVGraphicsNonPixelWorkaround) {
        context->Transition(texture, ERHIAccess::SRVGraphicsNonPixel);
    }
    context->SetSRV(0, texture);
}
```

#### needsExtraDeletionLatency

```cpp
// 問題： 参照カウントが0になってもリソースがまだGPU使用中
// 回避策： 削除を複数フレーム遅延

void DeferredDelete(IRHIResource* resource)
{
    uint32 extraFrames = GRHINeedsExtraDeletionLatency ? 3 : 0;
    uint64 deletionFrame = currentFrame + maxFramesInFlight + extraFrames;

    m_deferredDeleteQueue.Add({resource, deletionFrame});
}
```

### プラットフォーム初期化

```cpp
/// プラットフォーム固有の回避策フラグを設定
void InitializePlatformWorkarounds(ERHIBackend backend)
{
    auto& w = GRHIPlatformWorkarounds;

    switch (backend) {
    case ERHIBackend::D3D12:
        // D3D12固有の回避策
        w.needsTransientDiscardStateTracking = true;
        break;

    case ERHIBackend::Vulkan:
        // Vulkan固有の回避策
        w.needsExtraTransitions = true;
        break;

    case ERHIBackend::Metal:
        // Metal固有の回避策
        w.needsExtraTransitions = true;
        break;
    }

    // ドライバの/GPU固有の検出
    DetectDriverWorkarounds(w);
}

/// ドライバの固有の回避策検出
void DetectDriverWorkarounds(RHIPlatformWorkarounds& w)
{
    // NVIDIA特定ドライバのバージョンの問題
    if (IsNVIDIA() && GetDriverVersion() < MinSafeVersion) {
        w.needsFenceValuePadding = true;
    }

    // AMD特定GPUの問題
    if (IsAMD() && IsAffectedGPU()) {
        w.needsUnatlasedCSMDepthsWorkaround = true;
    }

    // Intel iGPUの問題
    if (IsIntel() && IsIntegratedGPU()) {
        w.needsExtraDeletionLatency = true;
    }
}
```

### 使用パターン

```cpp
// フラグをチェックしてから処理
void ProcessTransientResource(FTransientResource* resource)
{
    // Discard状態追跡が必要な場合
    if (GRHINeedsTransientDiscardStateTracking) {
        TrackDiscardState(resource);
    }

    // Graphics回避策が必要な場合
    if (GRHINeedsTransientDiscardOnGraphicsWorkaround &&
        !resource->IsSinglePipeline())
    {
        resource->bDiscardOnGraphicsWorkaround = true;
    }
}

// シェーダーアンバインドが必要な場合
void SetShader(IRHIShader* shader)
{
    if (GRHINeedsShaderUnbinds && m_currentShader != nullptr) {
        UnbindShader(m_currentShader);
    }
    m_currentShader = shader;
    // ...
}
```

## 検証方法

- 各フラグの正確な検出
- 回避策適用後の動作確認
- パフォーマンスへの影響測定
- 回帰テスト（新ドライバでの確認）

## 備考

回避策フラグは最後の手段として使用する。
可能であれば、根本的な問題をAPIレベルで解決することを優先する。

新しいドライバのリリースで問題が修正された場合、
バージョンチェックを追加して回避策を無効化できるようにする。

ドキュメントには以下を記載:
- 問題の症状
- 影響するプラットフォーム/ドライバ。
- 回避策の内容
- 根本原因（わかっている場合）

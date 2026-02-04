# UE5 RHI 実装ガイド Part 11: プラットフォーム最適化と回避策

## 目次
1. [プラットフォーム回避策フラグ (GRHINeed*)](#1-プラットフォーム回避策フラグ-grhineed)
2. [GPU Crash Trigger System](#2-gpu-crash-trigger-system)
3. [Variable Rate Shading (VRS) 詳細](#3-variable-rate-shading-vrs-詳細)
4. [モバイル/フレームバッファ固有機能](#4-モバイルフレームバッファ固有機能)
5. [Shader Execution Reordering (SER)](#5-shader-execution-reordering-ser)
6. [高度なレイトレーシング拡張](#6-高度なレイトレーシング拡張)

---

## 1. プラットフォーム回避策フラグ (GRHINeed*)

### 1.1 概要

`GRHINeed*` フラグは、特定のプラットフォームやドライバーのバグや制限を回避するための回避策を有効化するフラグです。

### 1.2 フラグ一覧

| フラグ | 説明 | 影響プラットフォーム |
|--------|------|---------------------|
| `NeedsUnatlasedCSMDepthsWorkaround` | カスケードシャドウマップのアトラス化による深度アーティファクト回避 | 一部 GPU |
| `NeedsTransientDiscardStateTracking` | Discard 状態遷移時の状態追跡が必要 | D3D12, Vulkan |
| `NeedsTransientDiscardOnGraphicsWorkaround` | Async Compute から Graphics への Discard 遷移回避 | D3D12 |
| `NeedsSRVGraphicsNonPixelWorkaround` | 非ピクセルシェーダー SRV の手動遷移が必要 | 一部ドライバー |
| `NeedsExtraTransitions` | COPYSRC/COPYDEST 状態の追加遷移が必要 | Metal |
| `NeedsExtraDeletionLatency` | リソース削除に追加の遅延が必要 | 一部プラットフォーム |
| `NeedsShaderUnbinds` | SetShaderUnbinds 呼び出しが必須 | OpenGL |

### 1.3 詳細説明

#### NeedsUnatlasedCSMDepthsWorkaround

```cpp
// RHIGlobals.h
bool NeedsUnatlasedCSMDepthsWorkaround = false;

// マクロアクセス
#define GRHINeedsUnatlasedCSMDepthsWorkaround GRHIGlobals.NeedsUnatlasedCSMDepthsWorkaround
```

**問題:** 一部の GPU でカスケードシャドウマップをアトラス化すると、深度比較で不正確なサンプリングが発生。

**回避策:** CSM を個別のテクスチャに分離してサンプリング。

#### NeedsTransientDiscardStateTracking

```cpp
// 一時リソースの Discard 状態を追跡
bool NeedsTransientDiscardStateTracking = false;
```

**問題:** Discard と Acquire 間の状態遷移が不正確になる可能性。

**回避策:** 明示的な状態追跡を有効化。

#### NeedsTransientDiscardOnGraphicsWorkaround

```cpp
// Async Compute パイプラインでの Discard 制限
bool NeedsTransientDiscardOnGraphicsWorkaround = false;
```

**問題:** Async Compute から All パイプラインへの Discard 遷移が正しく動作しない。

**回避策:**
```cpp
// FRHITransientAllocationFences::Contains() で使用
if (GRHIGlobals.NeedsTransientDiscardOnGraphicsWorkaround &&
    Discard.GetPipelines() == ERHIPipeline::All &&
    Acquire.GetPipelines() == ERHIPipeline::AsyncCompute &&
    Acquire.GraphicsForkJoin.Min < Discard.GraphicsForkJoin.Max)
{
    // エイリアシング不可
    return true;
}
```

#### NeedsSRVGraphicsNonPixelWorkaround

```cpp
bool NeedsSRVGraphicsNonPixelWorkaround = false;

#define GRHINeedsSRVGraphicsNonPixelWorkaround GRHIGlobals.NeedsSRVGraphicsNonPixelWorkaround
```

**問題:** 頂点/ジオメトリシェーダーからの SRV アクセスで暗黙の遷移が発生しない。

**回避策:** 手動で `ERHIAccess::SRVGraphicsNonPixel` への遷移を発行。

#### NeedsExtraTransitions

```cpp
bool NeedsExtraTransitions = false;
```

**問題:** 一部の RHI で COPYSRC/COPYDEST 状態への暗黙遷移がサポートされない。

**回避策:** コピー操作前後に明示的な遷移を挿入。

#### NeedsExtraDeletionLatency

```cpp
bool NeedsExtraDeletionLatency = false;

#define GRHINeedsExtraDeletionLatency GRHIGlobals.NeedsExtraDeletionLatency
```

**問題:** 参照カウントが 0 になってもリソースがまだ GPU で使用中の可能性。

**回避策:** 削除を複数フレーム遅延。

```cpp
// ストリーミングテクスチャの最適化
bool ForceNoDeletionLatencyForStreamingTextures = false;

#define GRHIForceNoDeletionLatencyForStreamingTextures \
    GRHIGlobals.ForceNoDeletionLatencyForStreamingTextures
```

#### NeedsShaderUnbinds

```cpp
bool NeedsShaderUnbinds = false;
```

**問題:** シェーダー切り替え時に前のシェーダーが残留する RHI がある。

**回避策:** 明示的なシェーダーアンバインドコマンドを発行。

### 1.4 使用パターン

```cpp
// 回避策を考慮したコード例
void DoTransientResourceTransition(FRHITransientResource* Resource,
                                    const FRHITransientAllocationFences& Fences)
{
    // Discard on Graphics workaround
    if (GRHIGlobals.NeedsTransientDiscardOnGraphicsWorkaround &&
        !Fences.IsSinglePipeline())
    {
        Resource->bDiscardOnGraphicsWorkaround = true;
    }

    // State tracking workaround
    if (GRHIGlobals.NeedsTransientDiscardStateTracking)
    {
        TrackDiscardState(Resource, Fences);
    }
}
```

---

## 2. GPU Crash Trigger System

### 2.1 概要

意図的に GPU クラッシュを発生させるデバッグ機能です。GPU デバッグツール（Aftermath、RenderDoc 等）のテストや、クラッシュ復旧フローの検証に使用します。

### 2.2 ERequestedGPUCrash 列挙

```cpp
// RHIDefinitions.h
enum class ERequestedGPUCrash : uint8
{
    None = 0,

    // クラッシュタイプ
    Type_Hang            = 1 << 0,  // GPU ハング（無限ループ）
    Type_PageFault       = 1 << 1,  // ページフォルト（無効メモリアクセス）
    Type_PlatformBreak   = 1 << 2,  // プラットフォーム固有ブレーク
    Type_Assert          = 1 << 3,  // GPU アサーション
    Type_CmdListCorruption = 1 << 4,  // コマンドリスト破損

    // ターゲットキュー
    Queue_Direct  = 1 << 5,  // グラフィックス/ダイレクトキュー
    Queue_Compute = 1 << 6,  // コンピュートキュー
};

ENUM_CLASS_FLAGS(ERequestedGPUCrash);
```

### 2.3 トリガー API

```cpp
// RHIGlobals.h
struct FRHIGlobals
{
    // 次フレームで GPU クラッシュをトリガー
    ERequestedGPUCrash TriggerGPUCrash = ERequestedGPUCrash::None;

    // GPU プロファイルをトリガー
    bool TriggerGPUProfile = false;

    // GPU ヒッチプロファイルをトリガー
    bool TriggerGPUHitchProfile = false;
};

// マクロアクセス
#define GTriggerGPUCrash GRHIGlobals.TriggerGPUCrash
```

### 2.4 使用例

```cpp
// グラフィックスキューでハングをトリガー
GTriggerGPUCrash = ERequestedGPUCrash::Type_Hang | ERequestedGPUCrash::Queue_Direct;

// コンピュートキューでページフォルトをトリガー
GTriggerGPUCrash = ERequestedGPUCrash::Type_PageFault | ERequestedGPUCrash::Queue_Compute;

// コマンドリスト破損をシミュレート
GTriggerGPUCrash = ERequestedGPUCrash::Type_CmdListCorruption | ERequestedGPUCrash::Queue_Direct;
```

### 2.5 クラッシュタイプ詳細

| タイプ | 説明 | 典型的な原因 |
|--------|------|-------------|
| `Type_Hang` | GPU が応答しなくなる | 無限ループ、デッドロック |
| `Type_PageFault` | 無効なメモリアクセス | NULL ポインタ、解放済みメモリ |
| `Type_PlatformBreak` | プラットフォーム固有のブレーク | デバッガーブレーク |
| `Type_Assert` | GPU 側アサーション | シェーダーアサート |
| `Type_CmdListCorruption` | コマンドバッファ破損 | 無効なコマンド、パラメータ |

### 2.6 デバッグとの連携

```cpp
// NVIDIA Aftermath との連携
#if WITH_NVIDIA_AFTERMATH
void OnGPUCrashTriggered()
{
    // Aftermath がクラッシュダンプを収集
    GFSDK_Aftermath_GetCrashDumpStatus(&status);
}
#endif

// Intel GPU Crash Dumps との連携
#if WITH_INTEL_GPU_CRASH_DUMPS
void OnGPUCrashTriggered()
{
    // Intel ツールがダンプを収集
    CollectIntelGPUCrashDump();
}
#endif
```

---

## 3. Variable Rate Shading (VRS) 詳細

### 3.1 概要

Variable Rate Shading は、画面の領域ごとに異なるシェーディングレートを適用する機能です。重要でない領域の処理を削減してパフォーマンスを向上させます。

### 3.2 シェーディングレート列挙

```cpp
// RHIDefinitions.h

// 軸ごとのシェーディングレート
enum EVRSAxisShadingRate : uint8
{
    VRSASR_1X = 0x0,  // フルレート
    VRSASR_2X = 0x1,  // 1/2 レート
    VRSASR_4X = 0x2,  // 1/4 レート
};

// 2D シェーディングレート（X軸 x Y軸）
enum EVRSShadingRate : uint8
{
    VRSSR_1x1 = (VRSASR_1X << 2) + VRSASR_1X,  // 1x1 (フルレート)
    VRSSR_1x2 = (VRSASR_1X << 2) + VRSASR_2X,  // 1x2
    VRSSR_2x1 = (VRSASR_2X << 2) + VRSASR_1X,  // 2x1
    VRSSR_2x2 = (VRSASR_2X << 2) + VRSASR_2X,  // 2x2
    VRSSR_2x4 = (VRSASR_2X << 2) + VRSASR_4X,  // 2x4
    VRSSR_4x2 = (VRSASR_4X << 2) + VRSASR_2X,  // 4x2
    VRSSR_4x4 = (VRSASR_4X << 2) + VRSASR_4X,  // 4x4

    VRSSR_Last = VRSSR_4x4
};
```

### 3.3 レートコンバイナー

```cpp
// 複数のシェーディングレートソースを組み合わせる方法
enum EVRSRateCombiner : uint8
{
    VRSRB_Passthrough,  // 入力をそのまま出力
    VRSRB_Override,     // 入力を上書き
    VRSRB_Min,          // 最小値（最高品質）を選択
    VRSRB_Max,          // 最大値（最低品質）を選択
    VRSRB_Sum,          // 合計（品質を下げる）
};
```

### 3.4 イメージデータタイプ

```cpp
enum EVRSImageDataType : uint8
{
    // VRS イメージ非サポート
    VRSImage_NotSupported,

    // パレットベース（離散値）
    // 例: 0=1x1, 1=2x2, 2=4x4
    VRSImage_Palette,

    // 浮動小数点ベース（連続値）
    // 例: 1.0f=フルレート, 0.5f=1/2, 0.25f=1/4
    VRSImage_Fractional,
};
```

### 3.5 ケイパビリティフラグ

```cpp
// RHIGlobals.h
struct FVariableRateShading
{
    // パイプライン VRS サポート（描画単位）
    bool SupportsPipeline = false;

    // 大サイズ VRS サポート（2x4, 4x2, 4x4）
    bool SupportsLargerSizes = false;

    // イメージベース VRS サポート
    bool SupportsAttachment = false;

    // 複雑なコンバイナー操作サポート
    bool SupportsComplexCombinerOps = false;

    // アレイテクスチャとしての VRS アタッチメント
    bool SupportsAttachmentArrayTextures = false;

    // VRS イメージのタイルサイズ
    int32 ImageTileMaxWidth = 0;
    int32 ImageTileMaxHeight = 0;
    int32 ImageTileMinWidth = 0;
    int32 ImageTileMinHeight = 0;

    // イメージデータタイプ
    EVRSImageDataType ImageDataType = VRSImage_NotSupported;

    // VRS イメージフォーマット
    EPixelFormat ImageFormat = PF_Unknown;

    // 遅延更新サポート
    bool SupportsLateUpdate = false;
};
```

### 3.6 マクロアクセス

```cpp
#define GRHISupportsPipelineVariableRateShading \
    GRHIGlobals.VariableRateShading.SupportsPipeline

#define GRHISupportsLargerVariableRateShadingSizes \
    GRHIGlobals.VariableRateShading.SupportsLargerSizes

#define GRHISupportsAttachmentVariableRateShading \
    GRHIGlobals.VariableRateShading.SupportsAttachment

#define GRHISupportsComplexVariableRateShadingCombinerOps \
    GRHIGlobals.VariableRateShading.SupportsComplexCombinerOps

#define GRHIVariableRateShadingImageTileMaxWidth \
    GRHIGlobals.VariableRateShading.ImageTileMaxWidth

#define GRHIVariableRateShadingImageTileMaxHeight \
    GRHIGlobals.VariableRateShading.ImageTileMaxHeight

#define GRHIVariableRateShadingImageDataType \
    GRHIGlobals.VariableRateShading.ImageDataType

#define GRHIVariableRateShadingImageFormat \
    GRHIGlobals.VariableRateShading.ImageFormat
```

### 3.7 使用例

```cpp
// パイプライン VRS の設定
void SetShadingRate(FRHICommandList& RHICmdList)
{
    if (GRHISupportsPipelineVariableRateShading)
    {
        // 2x2 シェーディングレートを設定
        RHICmdList.SetShadingRate(
            EVRSShadingRate::VRSSR_2x2,
            EVRSRateCombiner::VRSRB_Passthrough
        );
    }
}

// VRS イメージの作成
FTextureRHIRef CreateVRSImage(uint32 Width, uint32 Height)
{
    if (!GRHISupportsAttachmentVariableRateShading)
    {
        return nullptr;
    }

    // タイルサイズに基づいてイメージサイズを計算
    uint32 TileWidth = GRHIVariableRateShadingImageTileMaxWidth;
    uint32 TileHeight = GRHIVariableRateShadingImageTileMaxHeight;

    uint32 ImageWidth = FMath::DivideAndRoundUp(Width, TileWidth);
    uint32 ImageHeight = FMath::DivideAndRoundUp(Height, TileHeight);

    FRHITextureCreateInfo CreateInfo;
    CreateInfo.Dimension = ETextureDimension::Texture2D;
    CreateInfo.Format = GRHIVariableRateShadingImageFormat;
    CreateInfo.Extent = FIntPoint(ImageWidth, ImageHeight);
    CreateInfo.Flags = TexCreate_UAV | TexCreate_ShaderResource;

    return RHICreateTexture(CreateInfo);
}

// レンダーパスでの VRS 使用
FRHIRenderPassInfo RenderPassInfo;
RenderPassInfo.ShadingRateTexture = VRSImage;
RenderPassInfo.ShadingRateTextureCombiner = EVRSRateCombiner::VRSRB_Max;
```

### 3.8 プラットフォームサポート

| プラットフォーム | Pipeline VRS | Image VRS | Large Sizes |
|-----------------|--------------|-----------|-------------|
| D3D12 Tier 1 | ✅ | ❌ | ❌ |
| D3D12 Tier 2 | ✅ | ✅ | ✅ |
| Vulkan (NVIDIA) | ✅ | ✅ | ✅ |
| Vulkan (AMD) | ✅ | ✅ | 部分的 |
| Metal | ❌ | ❌ | ❌ |

---

## 4. モバイル/フレームバッファ固有機能

### 4.1 フレームバッファフェッチ

```cpp
// RHIGlobals.h

// 単一 RT のフレームバッファフェッチ
bool SupportsShaderFramebufferFetch = false;

// MRT のフレームバッファフェッチ
bool SupportsShaderMRTFramebufferFetch = false;

// プログラマブルブレンディング
bool SupportsShaderFramebufferFetchProgrammableBlending = false;

// 深度/ステンシルフェッチ
bool SupportsShaderDepthStencilFetch = false;
```

### 4.2 ピクセルローカルストレージ

```cpp
// タイルベース GPU のオンチップメモリ活用
bool SupportsPixelLocalStorage = false;
```

### 4.3 モバイルマルチビュー

```cpp
// ステレオレンダリングの最適化
bool SupportsMobileMultiView = false;
```

### 4.4 メモリレステクスチャ

```cpp
// TexCreate_Memoryless
// TBDR GPU のタイルメモリのみを使用
// システムメモリにバッキングストアなし
// 一時レンダーターゲットに最適

FRHITextureCreateInfo CreateInfo;
CreateInfo.Flags = TexCreate_RenderTargetable | TexCreate_Memoryless;
```

---

## 5. Shader Execution Reordering (SER)

### 5.1 概要

NVIDIA の Shader Execution Reordering は、レイトレーシングシェーダーの実行順序を最適化する機能です。

### 5.2 サポートフラグ

```cpp
// RHIGlobals.h
bool SupportsShaderExecutionReordering = false;
```

### 5.3 効果

- レイの発散（divergence）を削減
- キャッシュ効率を向上
- レイトレーシングパフォーマンスを大幅に改善

### 5.4 制限

- NVIDIA RTX GPU のみ
- 特定のドライバーバージョン以上が必要
- シェーダーモデル 6.6 以上

---

## 6. 高度なレイトレーシング拡張

### 6.1 クラスター操作

```cpp
// Nanite レイトレーシング加速
bool SupportsClusterOps = false;

// クラスター AS アライメント
uint32 ClusterAccelerationStructureAlignment;
```

### 6.2 AMD Hit Token

```cpp
// AMD 固有のカスタムヒット処理
bool SupportsAMDHitToken = false;
```

### 6.3 インラインレイトレーシング

```cpp
// コンピュートシェーダーからの RT
bool SupportsInlineRayTracing = false;

// インライン RT に SBT が必要か
bool RequiresInlineRayTracingSBT = false;
```

### 6.4 その他の RT 拡張

```cpp
struct FRayTracing
{
    // インライン化されたコールバック
    bool SupportsInlinedCallbacks = false;

    // 永続 SBT
    bool SupportsPersistentSBTs = false;

    // シェーダーレコード内のルースパラメータ
    bool SupportsLooseParamsInShaderRecord = false;

    // AS 圧縮
    bool SupportsAccelerationStructureCompaction = false;

    // AS シリアライズ
    bool SupportsSerializeAccelerationStructure = false;

    // 分離ヒットグループ貢献バッファ
    bool RequiresSeparateHitGroupContributionsBuffer = false;

    // AS アライメント
    uint32 AccelerationStructureAlignment;
};
```

---

## 7. ベストプラクティス

### 7.1 回避策フラグの使用

1. **常にフラグを確認** - ハードコードされた回避策を避ける
2. **プラットフォーム分岐** - `#if PLATFORM_*` より実行時フラグを優先
3. **ドキュメント化** - 回避策を適用する理由をコメント

### 7.2 VRS の使用

1. **段階的品質** - エッジで急激なレート変化を避ける
2. **フォビエーション** - VR で視線追跡と組み合わせ
3. **アダプティブ** - 動的シーンでレートを調整

### 7.3 GPU クラッシュテスト

1. **開発ビルドのみ** - シップビルドでは無効化
2. **復旧テスト** - クラッシュ後の復旧フローを確認
3. **ログ収集** - Aftermath/Intel ダンプと連携

---

## 8. 関連ドキュメント

- [Part 7: デバッグ・診断機能](RHI_Implementation_Guide_Part7.md) - GPU Breadcrumbs, Aftermath
- [Part 9: ケイパビリティフラグ](RHI_Implementation_Guide_Part9.md) - 全サポートフラグ一覧
- [Part 10: 高度なリソース管理](RHI_Implementation_Guide_Part10.md) - Transient Resources

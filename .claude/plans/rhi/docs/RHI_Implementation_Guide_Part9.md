# RHI 実装ガイド Part 9: ケイパビリティフラグ完全リスト

このドキュメントでは、RHI のすべてのケイパビリティフラグ (GRHISupports*) を解説します。

---

## 1. ケイパビリティフラグの概要

```
┌─────────────────────────────────────────────────────────────────┐
│            ケイパビリティフラグとは                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  目的:                                                           │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ RHI機能のランタイムサポート状況を示すグローバル変数     │    │
│  │                                                          │    │
│  │ // 使用例                                                │    │
│  │ if (GRHISupportsRayTracing)                             │    │
│  │ {                                                        │    │
│  │     // レイトレーシング機能を使用                       │    │
│  │ }                                                        │    │
│  │ else                                                     │    │
│  │ {                                                        │    │
│  │     // フォールバック処理                               │    │
│  │ }                                                        │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  定義場所: RHIGlobals.h                                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ // マクロとして定義され、GRHIGlobals 構造体にマップ     │    │
│  │ #define GRHISupportsXXX GRHIGlobals.SupportsXXX         │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  初期化: 各プラットフォームRHIの Init() で設定                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. 描画・ディスパッチ関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsDrawIndirect` | インダイレクト描画のサポート | ✓ | ✓ | ✓ |
| `GRHISupportsMultiDrawIndirect` | マルチドローインダイレクト | ✓ | ✓ | ✓ |
| `GRHISupportsBaseVertexIndex` | ベース頂点インデックス | ✓ | ✓ | ✓ |
| `GRHISupportsFirstInstance` | 最初のインスタンスID指定 | ✓ | ✓ | ✓ |

---

## 3. シェーダー関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsPixelShaderUAVs` | ピクセルシェーダーからのUAV書き込み | ✓ | ✓ | ✓ |
| `GRHISupportsWaveOperations` | Wave/Subgroup操作 (SM6.0) | ✓ | ✓ | ✓ |
| `GRHISupportsAtomicUInt64` | 64ビットアトミック操作 | ✓ | ✓ | △ |
| `GRHISupportsDX12AtomicUInt64` | D3D12 64ビットアトミック | ✓ | - | - |
| `GRHISupportsArrayIndexFromAnyShader` | 任意シェーダーからの配列インデックス | ✓ | ✓ | ✓ |
| `GRHISupportsStencilRefFromPixelShader` | PSからのステンシル参照出力 | ✓ | ✓ | ✓ |
| `GRHISupportsPrimitiveShaders` | プリミティブシェーダー | △ | △ | × |
| `GRHISupportsShaderRootConstants` | シェーダールート定数 | ✓ | ✓ | ✓ |
| `GRHISupportsShaderTimestamp` | シェーダー内タイムスタンプ | ✓ | ✓ | △ |
| `GRHISupportsLazyShaderCodeLoading` | 遅延シェーダーコードロード | ✓ | ✓ | ✓ |

---

## 4. メッシュシェーダー

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsMeshShadersTier0` | メッシュシェーダー Tier 0 (AS) | ✓ | ✓ | △ |
| `GRHISupportsMeshShadersTier1` | メッシュシェーダー Tier 1 (フル) | ✓ | ✓ | △ |

```
┌─────────────────────────────────────────────────────────────────┐
│           メッシュシェーダー Tier 説明                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Tier 0 (Amplification Shaders):                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ├── 増幅シェーダー (Task Shader) のみ                   │    │
│  │ └── メッシュシェーダーの基本サポート                    │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  Tier 1 (Full Mesh Shaders):                                     │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ├── フルメッシュシェーダーパイプライン                  │    │
│  │ ├── クリップ距離のサポート                              │    │
│  │ └── 高度な機能                                          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. レイトレーシング

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsRayTracing` | レイトレーシング基本サポート | ✓ | ✓ | △ |
| `GRHISupportsRayTracingShaders` | RTシェーダー (フルパイプライン) | ✓ | ✓ | △ |
| `GRHISupportsRayTracingPSOAdditions` | RT PSO追加サポート | ✓ | ✓ | × |
| `GRHISupportsRayTracingDispatchIndirect` | RTインダイレクトディスパッチ | ✓ | ✓ | × |
| `GRHISupportsRayTracingAsyncBuildAccelerationStructure` | 非同期BLAS構築 | ✓ | △ | × |
| `GRHISupportsRayTracingAMDHitToken` | AMD Hit Token | × | × | × |
| `GRHISupportsInlineRayTracing` | インラインレイトレーシング | ✓ | ✓ | △ |

---

## 6. Variable Rate Shading (VRS)

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsPipelineVariableRateShading` | パイプラインVRS | ✓ | ✓ | × |
| `GRHISupportsLargerVariableRateShadingSizes` | 大きいVRSタイルサイズ | ✓ | ✓ | × |
| `GRHISupportsAttachmentVariableRateShading` | アタッチメントVRS | ✓ | ✓ | × |
| `GRHISupportsComplexVariableRateShadingCombinerOps` | 複合VRSコンバイナー | ✓ | △ | × |
| `GRHISupportsVariableRateShadingAttachmentArrayTextures` | VRS配列テクスチャ | ✓ | △ | × |
| `GRHISupportsLateVariableRateShadingUpdate` | 遅延VRS更新 | ✓ | △ | × |

---

## 7. ワークグラフ

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsShaderWorkGraphsTier1` | ワークグラフ Tier 1 | ✓ | × | × |

---

## 8. シェーダーバンドル

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsShaderBundleDispatch` | シェーダーバンドルディスパッチ | ✓ | △ | △ |
| `GRHISupportsShaderBundleWorkGraphDispatch` | バンドル+ワークグラフ | ✓ | × | × |
| `GRHISupportsShaderBundleParallel` | 並列バンドルディスパッチ | ✓ | △ | △ |

---

## 9. テクスチャ・バッファ関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsTextureViews` | テクスチャビュー | ✓ | ✓ | ✓ |
| `GRHISupportsTextureStreaming` | テクスチャストリーミング | ✓ | ✓ | ✓ |
| `GRHISupportsUpdateFromBufferTexture` | バッファからテクスチャ更新 | ✓ | ✓ | ✓ |
| `GRHISupportsRWTextureBuffers` | RWテクスチャバッファ | ✓ | ✓ | ✓ |
| `GRHISupportsRawViewsForAnyBuffer` | 任意バッファのRawビュー | ✓ | ✓ | △ |
| `GRHISupportsBindingTexArrayPerSlice` | テクスチャ配列スライス単位バインド | ✓ | ✓ | △ |

---

## 10. メモリ・リソース関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsDirectGPUMemoryLock` | 直接GPUメモリロック | △ | △ | △ |
| `GRHISupportsMapWriteNoOverwrite` | WriteNoOverwriteマップ | ✓ | ✓ | ✓ |
| `GRHISupportsEfficientUploadOnResourceCreation` | リソース作成時効率的アップロード | ✓ | ✓ | ✓ |

---

## 11. 深度・ステンシル関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsDepthUAV` | 深度UAV | ✓ | ✓ | △ |
| `GRHISupportsSeparateDepthStencilCopyAccess` | 深度/ステンシル個別コピー | ✓ | ✓ | ✓ |
| `GRHISupportsDepthStencilResolve` | 深度/ステンシルリゾルブ | ✓ | ✓ | ✓ |

---

## 12. MSAA 関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsMSAADepthSampleAccess` | MSAA深度サンプルアクセス | ✓ | ✓ | △ |
| `GRHISupportsMSAAShaderResolve` | シェーダーでのMSAAリゾルブ | ✓ | ✓ | ✓ |

---

## 13. AMD 固有機能

| フラグ | 説明 | AMD | NVIDIA | Intel |
|--------|------|-----|--------|-------|
| `GRHISupportsExplicitHTile` | 明示的HTileアクセス | ✓ | × | × |
| `GRHISupportsExplicitFMask` | 明示的FMaskアクセス | ✓ | × | × |
| `GRHISupportsResummarizeHTile` | HTile再サマリ | ✓ | × | × |

---

## 14. スレッディング関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsMultithreading` | マルチスレッドサポート | ✓ | ✓ | ✓ |
| `GRHISupportsMultithreadedShaderCreation` | MTシェーダー作成 | ✓ | ✓ | ✓ |
| `GRHISupportsMultithreadedResources` | MTリソース作成 | ✓ | ✓ | ✓ |
| `GRHISupportsRHIThread` | 専用RHIスレッド | ✓ | ✓ | ✓ |
| `GRHISupportsRHIOnTaskThread` | タスクスレッドでRHI実行 | ✓ | ✓ | ✓ |
| `GRHISupportsParallelRHIExecute` | 並列RHI実行 | ✓ | ✓ | ✓ |
| `GRHISupportsParallelRenderPasses` | 並列レンダーパス | ✓ | ✓ | ✓ |

---

## 15. クエリ関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsExactOcclusionQueries` | 正確なオクルージョンクエリ | ✓ | ✓ | △ |
| `GRHISupportsAsyncGetRenderQueryResult` | 非同期クエリ結果取得 | ✓ | ✓ | ✓ |

---

## 16. パイプライン・PSO 関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsPipelineStateSortKey` | PSOソートキー | ✓ | ✓ | ✓ |
| `GRHISupportsPipelineFileCache` | PSOファイルキャッシュ | ✓ | ✓ | ✓ |
| `GRHISupportsPSOPrecaching` | PSOプリキャッシング | ✓ | ✓ | ✓ |
| `GRHISupportsAsyncPipelinePrecompile` | 非同期パイプラインコンパイル | ✓ | ✓ | ✓ |

---

## 17. トポロジー関連

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsQuadTopology` | Quadトポロジー | ✓ | × | × |
| `GRHISupportsRectTopology` | Rectトポロジー | ✓ | × | × |

---

## 18. レンダリング機能

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsDynamicResolution` | 動的解像度 | ✓ | ✓ | ✓ |
| `GRHISupportsHDROutput` | HDR出力 | ✓ | ✓ | ✓ |
| `GRHISupportsBackBufferWithCustomDepthStencil` | カスタム深度ステンシル付きバックバッファ | ✓ | ✓ | ✓ |
| `GRHISupportsUAVFormatAliasing` | UAVフォーマットエイリアシング | ✓ | ✓ | △ |
| `GRHISupportsRasterOrderViews` | Raster Order Views (ROV) | ✓ | △ | × |
| `GRHISupportsConservativeRasterization` | 保守的ラスタライゼーション | ✓ | ✓ | × |
| `GRHISupportsLossyFramebufferCompression` | 損失ありフレームバッファ圧縮 | △ | △ | ✓ |

---

## 19. プロファイリング・タイミング

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsGPUTimestampBubblesRemoval` | GPUタイムスタンプバブル除去 | ✓ | ✓ | △ |
| `GRHISupportsFrameCyclesBubblesRemoval` | フレームサイクルバブル除去 | ✓ | ✓ | △ |
| `GRHISupportsGPUUsage` | GPU使用率取得 | ✓ | △ | △ |

---

## 20. テクスチャ作成

| フラグ | 説明 | D3D12 | Vulkan | Metal |
|--------|------|-------|--------|-------|
| `GRHISupportsAsyncTextureCreation` | 非同期テクスチャ作成 | ✓ | ✓ | ✓ |

---

## 21. プラットフォーム別サポート表

```
┌─────────────────────────────────────────────────────────────────┐
│              プラットフォーム別サポートマトリクス                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  凡例:                                                           │
│  ✓ = 完全サポート                                                │
│  △ = 部分サポート / ハードウェア依存                            │
│  × = 非サポート                                                  │
│  - = 該当なし                                                    │
│                                                                  │
│  ┌───────────────────────────┬───────┬────────┬───────┬──────┐  │
│  │ カテゴリ                  │ D3D12 │ Vulkan │ Metal │ GL   │  │
│  ├───────────────────────────┼───────┼────────┼───────┼──────┤  │
│  │ 基本描画                  │ ✓     │ ✓      │ ✓     │ ✓    │  │
│  │ インダイレクト描画        │ ✓     │ ✓      │ ✓     │ △    │  │
│  │ メッシュシェーダー        │ ✓     │ ✓      │ △     │ ×    │  │
│  │ レイトレーシング          │ ✓     │ ✓      │ △     │ ×    │  │
│  │ Variable Rate Shading    │ ✓     │ ✓      │ ×     │ ×    │  │
│  │ ワークグラフ              │ ✓     │ ×      │ ×     │ ×    │  │
│  │ シェーダーバンドル        │ ✓     │ △      │ △     │ ×    │  │
│  │ マルチスレッド            │ ✓     │ ✓      │ ✓     │ △    │  │
│  │ PSO キャッシング          │ ✓     │ ✓      │ ✓     │ △    │  │
│  │ HDR出力                   │ ✓     │ ✓      │ ✓     │ ×    │  │
│  └───────────────────────────┴───────┴────────┴───────┴──────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 22. 使用パターン

### 22.1 機能チェックパターン

```cpp
// 単一機能チェック
if (GRHISupportsRayTracing)
{
    // レイトレーシングを使用
}

// 複合機能チェック
if (GRHISupportsRayTracing && GRHISupportsRayTracingShaders)
{
    // フルレイトレーシングパイプラインを使用
}
else if (GRHISupportsInlineRayTracing)
{
    // インラインレイトレーシングにフォールバック
}
else
{
    // ラスタライゼーションにフォールバック
}
```

### 22.2 DataDrivenShaderPlatformInfo との組み合わせ

```cpp
// シェーダープラットフォーム情報と組み合わせ
if (RHISupportsRayTracing(ShaderPlatform) && GRHISupportsRayTracing)
{
    // コンパイル時 + ランタイム両方でサポート確認
}
```

### 22.3 CVar との組み合わせ

```cpp
// CVar と機能フラグの組み合わせ
static bool IsRayTracingEnabled()
{
    static IConsoleVariable* CVarRayTracing =
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing"));

    return GRHISupportsRayTracing &&
           CVarRayTracing &&
           CVarRayTracing->GetInt() != 0;
}
```

---

## 23. 新規RHI実装時のチェックリスト

```
┌─────────────────────────────────────────────────────────────────┐
│           新規RHI実装時の機能フラグ設定                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  必須 (全RHIで設定):                                             │
│  □ GRHISupportsDrawIndirect                                     │
│  □ GRHISupportsMultithreading                                   │
│  □ GRHISupportsTextureViews                                     │
│  □ GRHISupportsPipelineFileCache                                │
│  □ GRHISupportsRHIThread                                        │
│                                                                  │
│  推奨 (モダンRHI):                                               │
│  □ GRHISupportsMultiDrawIndirect                                │
│  □ GRHISupportsWaveOperations                                   │
│  □ GRHISupportsPSOPrecaching                                    │
│  □ GRHISupportsAsyncPipelinePrecompile                          │
│  □ GRHISupportsParallelRHIExecute                               │
│                                                                  │
│  オプション (機能依存):                                          │
│  □ GRHISupportsRayTracing (RTハードウェア必須)                  │
│  □ GRHISupportsMeshShadersTier0/1 (MSハードウェア必須)          │
│  □ GRHISupportsPipelineVariableRateShading (VRSハードウェア)    │
│  □ GRHISupportsShaderWorkGraphsTier1 (D3D12のみ)                │
│                                                                  │
│  ベンダー固有:                                                   │
│  □ GRHISupportsExplicitHTile (AMD)                              │
│  □ GRHISupportsExplicitFMask (AMD)                              │
│  □ GRHISupportsResummarizeHTile (AMD)                           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## まとめ

RHI ケイパビリティフラグは、クロスプラットフォーム対応において重要な役割を果たします。

- **常に機能フラグをチェック**: 機能使用前に必ずサポート確認
- **フォールバックパスを用意**: 非対応時の代替処理を実装
- **組み合わせに注意**: 関連する複数のフラグを確認
- **ランタイムチェック**: シェーダープラットフォーム情報だけでなくランタイムフラグも確認

これらのフラグを適切に使用することで、様々なハードウェアで最適なレンダリングパスを選択できます。

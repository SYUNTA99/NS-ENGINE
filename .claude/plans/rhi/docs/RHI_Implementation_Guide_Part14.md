# RHI Implementation Guide Part 14: 詳細プラットフォーム実装とアロケータ

## 概要

このドキュメントは UE5.7 RHI の D3D12/Vulkan プラットフォーム固有の詳細実装、メモリアロケータシステム、および高度な内部機能を網羅します。Part 13 で紹介した CVar に加え、さらに詳細な設定オプションを記載します。

---

## 1. D3D12RHI メモリアロケータシステム

### 1.1 アロケータタイプ一覧

| アロケータ | 用途 | ファイル |
|-----------|------|----------|
| `FD3D12BuddyAllocator` | バディ割り当て戦略 | D3D12Allocation.h |
| `FD3D12MultiBuddyAllocator` | 複数バディアロケータ | D3D12Allocation.h |
| `FD3D12BucketAllocator` | バケット基盤割り当て | D3D12Allocation.h |
| `FD3D12FastAllocator` | 高速リニアアロケータ | D3D12Allocation.h |
| `FD3D12FastConstantAllocator` | 高速定数バッファ割り当て | D3D12Allocation.h |
| `FD3D12SegListAllocator` | セグメントリストアロケータ | D3D12Allocation.h |
| `FD3D12TextureAllocator` | テクスチャ専用アロケータ | D3D12Allocation.h |
| `FD3D12DefaultBufferAllocator` | デフォルトバッファ割り当て | D3D12Allocation.h |
| `FD3D12UploadHeapAllocator` | アップロードヒープ管理 | D3D12Allocation.h |
| `FD3D12TextureAllocatorPool` | テクスチャアロケータプーリング | D3D12Allocation.h |

### 1.2 割り当て戦略

```cpp
enum class EResourceAllocationStrategy
{
    kPlacedResource,      // Placed Resource (推奨)
    kManualSubAllocation  // 手動サブアロケーション
};
```

### 1.3 プールサイズ設定

| プール | デフォルトサイズ | 用途 |
|--------|-----------------|------|
| Readback Buffer Pool | 最大64KB、合計4MB | GPU→CPU読み戻し |
| Texture Pool | 8MB | テクスチャ割り当て |
| Resource Sub-allocation | 64KB境界 | リソースアライメント |
| Residency Pipeline | 最大6パケット | レジデンシー管理 |

---

## 2. D3D12RHI 追加 CVar

### 2.1 レイトレーシング詳細設定

| CVar | 説明 |
|------|------|
| `r.D3D12.RayTracing.AllowCompaction` | AS コンパクション有効化 |
| `r.D3D12.RayTracing.CacheShaderRecords` | SBT キャッシュ |
| `r.D3D12.RayTracing.DebugForceBuildMode` | デバッグ用ビルドモード強制 |
| `r.D3D12.RayTracing.SpecializeStateObjects` | 状態オブジェクト特殊化 |
| `r.D3D12.RayTracing.MaxBatchedCompaction` | バッチコンパクション最大数 |
| `r.D3D12.RayTracing.Compaction.MinPrimitiveCount` | コンパクション最小プリミティブ数 |
| `r.D3D12.RayTracing.GPUValidation` | GPU側RT検証 |
| `r.D3D12.RayTracingViewDescriptorHeapSize` | RTビューヒープサイズ |

### 2.2 DRED (Device Removed Extended Data)

| CVar | 説明 |
|------|------|
| `r.D3D12.DRED` | DRED 有効化 |
| `r.D3D12.LightweightDRED` | 軽量DRED |
| `r.D3D12.EnableD3DDebug` | D3Dデバッグレイヤー |

### 2.3 サブミッション制御

| CVar | 説明 |
|------|------|
| `r.D3D12.SubmissionTimeout` | サブミッションタイムアウト |
| `r.D3D12.Submission.MaxExecuteBatchSize.Direct` | グラフィックスキューバッチサイズ |
| `r.D3D12.Submission.MaxExecuteBatchSize.Copy` | コピーキューバッチサイズ |
| `r.D3D12.Submission.MaxExecuteBatchSize.Async` | 非同期コンピュートバッチサイズ |

### 2.4 ドライバー要件

| CVar | 説明 |
|------|------|
| `r.D3D12.DXR.MinimumWindowsBuildVersion` | DXR最小Windowsビルド |
| `r.D3D12.DXR.MinimumDriverVersionNVIDIA` | NVIDIA最小ドライバーバージョン |
| `r.D3D12.DXR.MinimumDriverVersionAMD` | AMD最小ドライバーバージョン |
| `r.D3D12.ExperimentalShaderModels` | 実験的SM (SM6.6+) |

### 2.5 PSO・パイプライン

| CVar | 説明 |
|------|------|
| `r.D3D12.PSO.DiskCache` | PSOディスクキャッシュ |
| `r.D3D12.PSO.DriverOptimizedDiskCache` | ドライバー最適化キャッシュ |
| `r.D3D12.NumViewportBuffers` | ビューポートバッファ数 |

### 2.6 デバッグ・開発

| CVar | 説明 |
|------|------|
| `r.D3D12.DevDisableD3DRuntimeBackgroundThreads` | D3Dランタイムスレッド無効化 |
| `r.D3D12.AutoAttachPIX` | PIX自動アタッチ |
| `r.D3D12.GPUTimeFromTimestamps` | タイムスタンプからGPU時間取得 |

---

## 3. VulkanRHI 詳細 CVar (追加分)

### 3.1 拡張機能有効化

| CVar | 説明 |
|------|------|
| `r.Vulkan.AllowConcurrentBuffer` | 同時バッファ操作 |
| `r.Vulkan.AllowConcurrentImage` | 同時イメージ操作 |
| `r.Vulkan.AllowPSOPrecaching` | PSO プリキャッシュ |
| `r.Vulkan.AllowUniformUpload` | UBO アップロード最適化 |

### 3.2 バインドレス詳細設定

| CVar | デフォルト | 説明 |
|------|-----------|------|
| `r.Vulkan.Bindless.BlockSize` | - | ディスクリプタブロックサイズ |
| `r.Vulkan.Bindless.MaxResourceAccelerationStructureCount` | - | 最大AS数 |
| `r.Vulkan.Bindless.MaxResourceSampledImageCount` | - | 最大サンプルイメージ数 |
| `r.Vulkan.Bindless.MaxResourceStorageBufferCount` | - | 最大ストレージバッファ数 |
| `r.Vulkan.Bindless.MaxResourceStorageImageCount` | - | 最大ストレージイメージ数 |
| `r.Vulkan.Bindless.MaxResourceStorageTexelBufferCount` | - | 最大ストレージTexel数 |
| `r.Vulkan.Bindless.MaxResourceUniformBufferCount` | - | 最大UBO数 |
| `r.Vulkan.Bindless.MaxResourceUniformTexelBufferCount` | - | 最大UBO Texel数 |

### 3.3 Chunked PSOキャッシュ

| CVar | 説明 |
|------|------|
| `r.Vulkan.ChunkedPSOCache.ChunkEvictTime` | チャンク退避時間 |
| `r.Vulkan.ChunkedPSOCache.MaxSingleCachePSOCount` | 単一キャッシュ最大PSO数 |
| `r.Vulkan.ChunkedPSOCache.MaxTotalCacheSizeMb` | 総キャッシュサイズ上限(MB) |
| `r.Vulkan.ChunkedPSOCache.TargetResidentCacheSizeMb` | ターゲットレジデントサイズ(MB) |
| `r.Vulkan.UseChunkedPSOCache` | Chunkedキャッシュ使用 |
| `r.Vulkan.MemoryMapChunkedPSOCache` | メモリマップキャッシュ |

### 3.4 メモリデフラグ

| CVar | 説明 |
|------|------|
| `r.Vulkan.EnableDefrag` | デフラグ有効化 |
| `r.Vulkan.DefragPaused` | デフラグ一時停止 |
| `r.Vulkan.DefragAutoPause` | 自動一時停止 |
| `r.Vulkan.DefragSizeFraction` | サイズ閾値割合 |
| `r.Vulkan.DefragSizeFactor` | スケーリング係数 |
| `r.Vulkan.DefragAgeDelay` | 年齢遅延 |
| `r.Vulkan.DefragOnceDebug` | 単発デフラグ(デバッグ) |
| `r.Vulkan.EvictOnePageDebug` | 単一ページ退避(デバッグ) |
| `r.Vulkan.EvictionLimitPercentage` | 退避上限パーセンテージ |
| `r.Vulkan.EvictionLimitPercentageRenableLimit` | 退避再有効化閾値 |

### 3.5 メモリ管理詳細

| CVar | 説明 |
|------|------|
| `r.Vulkan.BudgetScale` | メモリ予算スケール |
| `r.Vulkan.FakeMemoryLimit` | 仮想メモリ制限(テスト用) |
| `r.Vulkan.MemoryFallbackToHost` | ホストメモリフォールバック |
| `r.Vulkan.MemoryBacktrace` | アロケーションバックトレース |
| `r.Vulkan.EnableDedicatedImageMemory` | 専用イメージメモリ |
| `r.Vulkan.SingleAllocationPerResource` | リソース毎単一アロケーション |
| `r.Vulkan.SparseImageAllocSizeMB` | スパースイメージサイズ(MB) |
| `r.Vulkan.SparseImageMergeBindings` | スパースイメージバインディング統合 |

### 3.6 同期・サブミッション

| CVar | 説明 |
|------|------|
| `r.Vulkan.Submission.AllowTimelineSemaphores` | タイムラインセマフォ |
| `r.Vulkan.Submission.MergePayloads` | ペイロード統合最適化 |
| `r.Vulkan.Submission.UseInterruptThread` | 割り込みスレッド使用 |
| `r.Vulkan.Submission.UseSubmissionThread` | サブミッションスレッド使用 |
| `r.Vulkan.WaitForIdleOnSubmit` | サブミット時GPU待機 |
| `r.Vulkan.RequiresWaitingForFrameCompletionEvent` | フレーム完了待機 |

### 3.7 テクスチャ・レンダリング

| CVar | 説明 |
|------|------|
| `r.Vulkan.DepthStencilForceStorageBit` | 深度ステンシルStorage強制 |
| `r.Vulkan.UseASTCDecodeMode` | ASTCデコードモード |
| `r.Vulkan.SupportsBCTextureFormats` | BCテクスチャフォーマット |
| `r.Vulkan.InputAttachmentShaderRead` | 入力アタッチメント読み取り |
| `r.Vulkan.AlwaysWriteDS` | 常時深度ステンシル書き込み |
| `r.Vulkan.PreTransform` | サーフェスプリトランスフォーム |
| `r.Vulkan.ForceCoherentOperations` | コヒーレント操作強制 |

### 3.8 スワップチェーン

| CVar | 説明 |
|------|------|
| `r.Vulkan.KeepSwapChain` | スワップチェーン維持 |
| `r.Vulkan.SwapChainIgnoreExtraImages` | 追加イメージ無視 |

### 3.9 デバッグ詳細

| CVar | 説明 |
|------|------|
| `r.Vulkan.DebugBarrier` | デバッグバリア挿入 |
| `r.Vulkan.DebugVsync` | Vsyncデバッグ |
| `r.Vulkan.UniqueValidationMessages` | 固有検証メッセージフィルタ |
| `r.Vulkan.CompressSPIRV` | SPIR-V圧縮 |
| `r.Vulkan.FlushLog` | ログフラッシュ |
| `r.Vulkan.DumpMemory` | メモリダンプ |
| `r.Vulkan.DumpMemoryFull` | 完全メモリダンプ |
| `r.Vulkan.DumpStagingMemory` | ステージングメモリダンプ |
| `r.Vulkan.DumpPSOLRU` | PSO LRUダンプ |
| `r.Vulkan.TrimPSOLRU` | PSO LRUトリム |
| `r.Vulkan.LogEvictStatus` | 退避状態ログ |
| `r.Vulkan.ReleaseShaderModuleWhenEvictingPSO` | PSO退避時シェーダーモジュール解放 |

### 3.10 フレームペーシング

| CVar | 説明 |
|------|------|
| `r.Vulkan.CPURHIThreadFramePacer` | RHIスレッドフレームペーサー |
| `r.Vulkan.CPURenderthreadFramePacer` | レンダースレッドフレームペーサー |
| `r.Vulkan.ExtensionFramePacer` | 拡張フレームペーサー |
| `r.Vulkan.ForcePacingWithoutVSync` | Vsyncなしペーシング強制 |
| `r.Vulkan.RHIThread` | RHIスレッド制御 |
| `r.Vulkan.CpuWaitForFence` | CPU側フェンス待機 |

### 3.11 バッファ操作

| CVar | 説明 |
|------|------|
| `r.Vulkan.FlushOnMapStaging` | ステージングマップ時フラッシュ |
| `r.Vulkan.IgnoreCPUReads` | CPU読み取り無視最適化 |
| `r.Vulkan.UseBufferBinning` | バッファビニング |
| `r.Vulkan.NumFramesToWaitForResourceDelete` | リソース削除待機フレーム数 |

### 3.12 クエリ・プロファイリング

| CVar | 説明 |
|------|------|
| `r.Vulkan.TimestampQueryStage` | タイムスタンプクエリステージ |
| `r.Vulkan.TimingQueryPoolSize` | クエリプールサイズ |
| `r.Vulkan.QueryPoolDeletionDelay` | クエリプール削除遅延 |

### 3.13 その他

| CVar | 説明 |
|------|------|
| `r.Vulkan.AMDCompatibilityMode` | AMD互換モード |
| `r.Vulkan.Aftermath.ExtendedLifetimeFrames` | Aftermath拡張ライフタイム |
| `r.Vulkan.RebuildPipelineCache` | パイプラインキャッシュ再構築 |
| `r.Vulkan.SavePipelineCache` | パイプラインキャッシュ保存 |
| `r.Vulkan.SaveValidationCache` | 検証キャッシュ保存 |
| `r.Vulkan.EnableTransientResourceAllocator` | 一時リソースアロケータ |
| `r.Vulkan.EnablePSOFileCacheWhenPrecachingActive` | プリキャッシュ中PSOファイルキャッシュ |

---

## 4. D3D12RHI 内部クラス

### 4.1 ディスクリプタ管理

| クラス | 用途 |
|--------|------|
| `FD3D12ExplicitDescriptorCache` | 明示的ディスクリプタヒープキャッシュ |
| `FD3D12DescriptorCache` | 自動ディスクリプタキャッシュ |
| `FD3D12BindlessDescriptorAllocator` | バインドレスディスクリプタ割り当て |
| `FD3D12BindlessSamplerManager` | バインドレスサンプラー管理 |

### 4.2 バリアシステム

| クラス | 用途 |
|--------|------|
| `FD3D12EnhancedBarriersForAdapter` | Enhanced Barriers (DX12.1+) |
| `FD3D12LegacyBarriers` | レガシーリソース状態バリア |

```cpp
enum class ED3D12BarrierImplementationType
{
    Legacy,    // レガシーバリア
    Enhanced   // Enhanced Barriers (DX12.1+)
};
```

### 4.3 レイトレーシングコンポーネント

| クラス | 用途 |
|--------|------|
| `FD3D12RayTracingPipelineState` | RT PSO |
| `FD3D12RayTracingGeometry` | AS ジオメトリ |
| `FD3D12RayTracingShaderLibrary` | RTシェーダーライブラリ |
| `FD3D12ShaderIdentifier` | RTシェーダー識別子 |
| `ID3D12RayTracingGeometryUpdateListener` | RTジオメトリリスナー |

### 4.4 Work Graphs

```cpp
// Work Graphs サポート (新機能)
class FD3D12WorkGraphPipelineState;

// 機能定義
#define D3D12_RHI_WORKGRAPHS 1
```

### 4.5 レジデンシー管理

```cpp
class FD3D12ResidencyHandle;

// プラットフォーム機能フラグ
#define D3D12_PLATFORM_NEEDS_RESIDENCY_MANAGEMENT 1
```

---

## 5. VulkanRHI 内部クラス

### 5.1 メモリ管理

| クラス | 用途 |
|--------|------|
| `FVulkanMemory` | コアメモリ管理システム |
| `FVulkanEvictable` | 退避可能リソースインターフェース |
| `FVulkanAllocation` | メモリアロケーションハンドル |

```cpp
enum class EVulkanAllocationType
{
    // アロケーションタイプ定義
};

enum class EVulkanAllocationMetaType
{
    // メタデータタイプ定義
};
```

### 5.2 バインドレス

| クラス | 用途 |
|--------|------|
| `FVulkanBindlessDescriptorManager` | バインドレスディスクリプタ管理 |

```cpp
namespace VulkanBindless
{
    constexpr int NumBindlessSets = ...;  // バインドレスセット数
    constexpr int MaxNumSets = ...;       // 最大セット数
}
```

### 5.3 拡張フレームワーク

| クラス | 用途 |
|--------|------|
| `FVulkanExtensionBase` | 拡張基底クラス |
| `FVulkanDeviceExtension` | デバイスレベル拡張 |
| `FVulkanInstanceExtension` | インスタンスレベル拡張 |

```cpp
enum class EExtensionActivation
{
    AutoActivate,      // 自動有効化
    ManuallyActivate   // 手動有効化
};
```

---

## 6. 内部デバッグ定義

### 6.1 D3D12 デバッグ定義

```cpp
// D3D12RHIDefinitions.h
#define DEBUG_RESOURCE_STATES 0
#define DEBUG_FRAME_TIMING 0
#define DEBUG_EXECUTE_COMMAND_LISTS 0
#define DEBUG_RHI_EXECUTE_COMMAND_LIST 0
#define EXECUTE_DEBUG_COMMAND_LISTS 0
```

### 6.2 Vulkan 設定定義

```cpp
// VulkanConfiguration.h
#define VULKAN_HAS_DEBUGGING_ENABLED
#define VULKAN_VALIDATION_DEFAULT_VALUE 0  // 0-5レベル
#define VULKAN_ENABLE_DUMP_LAYER
#define VULKAN_ENABLE_IMAGE_TRACKING_LAYER
#define VULKAN_ENABLE_BUFFER_TRACKING_LAYER
#define VULKAN_ENABLE_TRACKING_LAYER
#define VULKAN_COMMANDWRAPPERS_ENABLE
#define VULKAN_OBJECT_TRACKING
#define VULKAN_MEMORY_TRACK
```

---

## 7. D3D12 列挙型

### 7.1 アクセス・パイプライン制御

```cpp
enum class ED3D12Access
{
    // D3D12リソースアクセス状態
};

enum class ED3D12PipelineType
{
    Graphics,
    Compute
};

enum class ED3D12QueueType
{
    Direct,   // グラフィックス
    Compute,  // コンピュート
    Copy      // コピー
};
```

### 7.2 状態管理

```cpp
enum class ED3D12ResourceStateMode
{
    // リソース状態モード
};

enum class ED3D12ViewType
{
    // ビュータイプカテゴリ
};

enum class ED3D12QueryType
{
    // クエリタイプ
};

enum class ED3D12QueryPosition
{
    Start,
    End
};
```

### 7.3 ディスクリプタ管理

```cpp
enum class ED3D12DescriptorHeapFlags
{
    // ヒープ設定フラグ
};

enum class ED3D12SetDescriptorHeapsFlags
{
    // ディスクリプタヒープ設定フラグ
};
```

### 7.4 同期

```cpp
enum class ED3D12SyncPointType
{
    // 同期ポイントタイプ
};

enum class ED3D12FlushFlags
{
    WaitForCompletion  // 完了待機
};
```

---

## 8. パフォーマンス設定ガイドライン

### 8.1 D3D12 最適化設定

```ini
; 高パフォーマンス設定
[D3D12RHI]
r.D3D12.AllowAsyncCompute=1
r.D3D12.PreferredBarrierImplementation=1  ; Enhanced Barriers
r.D3D12.PSO.DiskCache=1
r.D3D12.ResidencyManagement=1

; レイトレーシング最適化
r.D3D12.RayTracing.AllowCompaction=1
r.D3D12.RayTracing.CacheShaderRecords=1
```

### 8.2 Vulkan 最適化設定

```ini
; 高パフォーマンス設定
[VulkanRHI]
r.Vulkan.AllowAsyncCompute=1
r.Vulkan.AllowDynamicRendering=1
r.Vulkan.AllowSynchronization2=1
r.Vulkan.EnableLRU=1
r.Vulkan.UseChunkedPSOCache=1

; メモリ最適化
r.Vulkan.EnableDefrag=1
r.Vulkan.EnableDedicatedImageMemory=1
```

### 8.3 デバッグ設定

```ini
; D3D12 デバッグ
[D3D12RHI]
r.D3D12.DRED=1
r.D3D12.EnableD3DDebug=1

; Vulkan デバッグ
[VulkanRHI]
r.Vulkan.EnableValidation=3  ; 高レベル検証
r.Vulkan.GPUValidation=1
r.Vulkan.DebugMarkers=1
```

---

## 9. 関連ファイル一覧

### 9.1 D3D12RHI

| ファイル | 内容 |
|----------|------|
| `D3D12Adapter.h/cpp` | ドライバー機能、拡張 |
| `D3D12Device.h/cpp` | デバイス初期化 |
| `D3D12Allocation.h/cpp` | アロケータ実装 (8種以上) |
| `D3D12DescriptorCache.h` | ディスクリプタ管理 |
| `D3D12BindlessDescriptors.h` | バインドレスレンダリング |
| `D3D12EnhancedBarriers.h/cpp` | DX12.1+ バリアシステム |
| `D3D12RayTracing.h/cpp` | レイトレーシング実装 |
| `D3D12Residency.h` | メモリレジデンシー管理 |
| `D3D12Submission.h/cpp` | コマンドサブミッション |
| `D3D12WorkGraph.h/cpp` | Work Graphs サポート |
| `D3D12Stats.h` | パフォーマンス統計 |

### 9.2 VulkanRHI

| ファイル | 内容 |
|----------|------|
| `VulkanConfiguration.h` | 40+ 設定定義 |
| `VulkanExtensions.h` | 拡張フレームワーク |
| `VulkanMemory.h` | メモリ割り当てシステム |
| `VulkanBindlessDescriptorManager.h` | バインドレスサポート |
| `VulkanChunkedPipelineCache.h` | PSOキャッシュ |
| `VulkanRayTracing.h` | レイトレーシング |
| `VulkanTransientResourceAllocator.h` | 一時リソース |

---

## 10. 統計サマリー

| カテゴリ | 項目数 |
|----------|--------|
| D3D12 CVar (追加) | 35+ |
| Vulkan CVar (追加) | 70+ |
| D3D12 アロケータ | 10種 |
| D3D12 列挙型 | 15+ |
| 内部クラス | 50+ |

---

## 更新履歴

| 日付 | 内容 |
|------|------|
| 2026-02-04 | 初版作成 - 詳細プラットフォーム実装とアロケータ |

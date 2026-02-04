# RHI Implementation Guide Part 13: 統計・コンソール変数・プラットフォーム設定

## 概要

このドキュメントは UE5.7 RHI の統計グループ (Stats)、コンソール変数 (CVars)、およびプラットフォーム固有の設定オプションを網羅的に記載します。

---

## 1. 統計グループ (Stats Groups)

### 1.1 コアRHI統計

| 統計グループ | 表示名 | 用途 |
|-------------|--------|------|
| `STATGROUP_RHICMDLIST` | RHICmdList | RHIコマンドリスト統計 |
| `STATGROUP_RHI_COMMANDS` | RHICommands | 個別RHIコマンド統計 |
| `STATGROUP_PipelineStateCache` | ShaderPipelineCache | PSOキャッシュ統計 |
| `STATGROUP_TexturePool` | TexturePool | テクスチャプール・デフラグ統計 |
| `STATGROUP_D3DMemory` | D3D Video Memory | D3Dビデオメモリ統計 |
| `STATGROUP_RHITransientMemory` | RHI: Transient Memory | 一時リソースメモリ統計 |

### 1.2 D3D12RHI統計

| 統計グループ | 表示名 | 用途 |
|-------------|--------|------|
| `STATGROUP_D3D12RHI` | D3D12RHI | D3D12全般統計 |
| `STATGROUP_D3D12Memory` | D3D12RHI: Memory | メモリ使用統計 |
| `STATGROUP_D3D12MemoryDetails` | D3D12RHI: Memory Details | 詳細メモリ統計 |
| `STATGROUP_D3D12Resources` | D3D12RHI: Resources | リソース統計 |
| `STATGROUP_D3D12Bindless` | D3D12RHI: Bindless | バインドレス統計 |
| `STATGROUP_D3D12BufferDetails` | D3D12RHI: Buffer Details | バッファ詳細統計 |
| `STATGROUP_D3D12PipelineState` | D3D12RHI: Pipeline State (PSO) | PSO統計 |
| `STATGROUP_D3D12DescriptorHeap` | D3D12RHI: Descriptor Heap | ディスクリプタヒープ統計 |
| `STATGROUP_D3D12RHIPipeline` | D3D12RHIPipeline | パイプライン統計 |
| `STATGROUP_D3D12RayTracing` | D3D12RHI: Ray Tracing | レイトレーシング統計 |

### 1.3 VulkanRHI統計

| 統計グループ | 表示名 | 用途 |
|-------------|--------|------|
| `STATGROUP_VulkanRHI` | Vulkan RHI | Vulkan全般統計 |
| `STATGROUP_VulkanMemory` | Vulkan Memory | メモリ統計 |
| `STATGROUP_VulkanMemoryRaw` | Vulkan Memory Raw | 生メモリ統計（名前順ソート） |
| `STATGROUP_VulkanPSO` | Vulkan PSO | PSO統計 |
| `STATGROUP_VulkanBindless` | Vulkan Bindless | バインドレス統計 |
| `STATGROUP_VulkanGraphicPipelineLibs` | Vulkan Graphic Pipeline Libraries | パイプラインライブラリ統計 |
| `STATGROUP_VulkanRayTracing` | Vulkan: Ray Tracing | レイトレーシング統計 |

### 1.4 統計の使用方法

```cpp
// 統計グループの有効化（コンソール）
stat RHICMDLIST
stat D3D12RHI
stat VulkanRHI

// コード内でのサイクル統計
DECLARE_CYCLE_STAT(TEXT("RHI Thread Time"), STAT_RHIThreadTime, STATGROUP_RHICMDLIST);
SCOPE_CYCLE_COUNTER(STAT_RHIThreadTime);

// メモリ統計
DECLARE_MEMORY_STAT(TEXT("Buffer Memory"), STAT_BufferMemory, STATGROUP_D3D12Memory);
INC_MEMORY_STAT_BY(STAT_BufferMemory, AllocationSize);
DEC_MEMORY_STAT_BY(STAT_BufferMemory, AllocationSize);
```

---

## 2. GPUプロファイラ CVar

### 2.1 基本プロファイル設定

| CVar | タイプ | デフォルト | 説明 |
|------|--------|-----------|------|
| `r.ProfileGPU.Sort` | int32 | - | ソート方法 (0=なし, 1=時間順) |
| `r.ProfileGPU.Root` | FString | - | プロファイルルートノード名 |
| `r.ProfileGPU.ThresholdPercent` | float | - | 表示する最小パーセンテージ閾値 |
| `r.ProfileGPU.UnicodeOutput` | bool | - | Unicode出力の有効化 |
| `r.ProfileGPU.ShowLeafEvents` | bool | - | リーフイベントの表示 |
| `r.ProfileGPU.ShowHeader` | bool | - | ヘッダーの表示 |
| `r.ProfileGPU.ShowEmptyQueues` | bool | - | 空キューの表示 |
| `r.ProfileGPU.ShowStats` | bool | - | 統計の表示 |
| `r.ProfileGPU.ShowPercentColumn` | bool | - | パーセント列の表示 |
| `r.ProfileGPU.ShowInclusive` | bool | - | 包含時間の表示 |
| `r.ProfileGPU.ShowExclusive` | bool | - | 排他時間の表示 |
| `r.ProfileGPU.ShowUI` | bool | - | UIでの表示 |

### 2.2 GPU CSV・ヒストグラム

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.GPUCsvStatsEnabled` | int32 | GPU CSV統計の有効化 |
| `r.ProfileGPUPattern` | FString | プロファイルパターンフィルタ |
| `r.ProfileShowEventHistogram` | int32 | イベントヒストグラムの表示 |
| `r.ProfileGPUTransitions` | int32 | 遷移のプロファイル |
| `r.ProfilePrintAssetSummary` | int32 | アセットサマリーの出力 |
| `r.ProfileAssetSummaryCallOuts` | FString | アセットサマリーコールアウト |

### 2.3 GPUクラッシュデータ収集

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.GPUCrashDebugging` | int32 | GPUクラッシュデバッグ有効化 |
| `r.GPUCrashDebugging.Breadcrumbs` | int32 | Breadcrumbs収集の有効化 |
| `r.GPUCrashDataCollectionEnable` | int32 | クラッシュデータ収集の有効化 |
| `r.GPUCrashDataDepth` | int32 | クラッシュデータの深度 |
| `r.GPUCrashOnOutOfMemory` | int32 | OOM時にGPUクラッシュ |
| `r.SaveScreenshotAfterProfilingGPU` | int32 | プロファイル後スクリーンショット保存 |
| `r.GPUHitchThreshold` | float | GPUヒッチ閾値（秒） |

---

## 3. PSOキャッシュ CVar

### 3.1 非同期コンパイル

| CVar | タイプ | デフォルト | 説明 |
|------|--------|-----------|------|
| `r.AsyncPipelineCompile` | int32 | 1 | 非同期PSOコンパイル有効化 |
| `r.CreatePSOsOnRHIThread` | int32 | - | RHIスレッドでPSO作成 |
| `r.EnablePSOAsyncCacheConsolidation` | int32 | - | 非同期キャッシュ統合 |

### 3.2 キャッシュ管理

| CVar | タイプ | デフォルト | 説明 |
|------|--------|-----------|------|
| `r.PSOEvictionTime` | int32 | 60 | PSO退避時間（秒） |
| `r.PSORuntimeCreationHitchThreshold` | int32 | - | ランタイムPSO作成ヒッチ閾値 |
| `r.RTPSOCacheSize` | int32 | - | レイトレーシングPSOキャッシュサイズ |
| `r.PSOPrecaching` | int32 | - | PSOプリキャッシュ有効化 |

### 3.3 Shader Pipeline Cache

```cpp
// ファイルキャッシュの場所
// {ProjectDir}/Saved/PipelineCaches/{Platform}/

// 使用例
FShaderPipelineCache::OpenPipelineFileCache(Platform);
FShaderPipelineCache::SavePipelineFileCache(SaveMode);
FShaderPipelineCache::ClosePipelineFileCache();
```

---

## 4. D3D12RHI固有 CVar

### 4.1 メモリ・アロケーション

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.AllowPoolAllocateIndirectArgBuffers` | int32 | 間接引数バッファのプール割り当て |
| `D3D12.ReadOnlyTextureAllocatorMinPoolSize` | int32 | 読み取り専用テクスチャ最小プールサイズ |
| `D3D12.ReadOnlyTextureAllocatorMinNumToPool` | int32 | プールする最小テクスチャ数 |
| `D3D12.ReadOnlyTextureAllocatorMaxPoolSize` | int32 | 最大プールサイズ |
| `D3D12.VRAMBufferPoolDefrag` | int32 | VRAMバッファプールデフラグ有効化 |
| `D3D12.VRAMBufferPoolDefragMaxCopySizePerFrame` | int32 | フレームあたり最大コピーサイズ |
| `D3D12.PoolAllocatorReadOnlyTextureVRAMPoolSize` | int32 | 読み取り専用テクスチャVRAMプールサイズ |
| `D3D12.PoolAllocatorRTUAVTextureVRAMPoolSize` | int32 | RT/UAVテクスチャVRAMプールサイズ |

### 4.2 Upload Heap設定

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.UploadHeapSmallBlockMaxAllocationSize` | int32 | 小ブロック最大割り当てサイズ |
| `D3D12.UploadHeapSmallBlockPoolSize` | int32 | 小ブロックプールサイズ |
| `D3D12.UploadHeapBigBlockMaxAllocationSize` | int32 | 大ブロック最大割り当てサイズ |
| `D3D12.UploadHeapBigBlockPoolSize` | int32 | 大ブロックプールサイズ |
| `D3D12.FastConstantAllocatorPageSize` | int32 | 高速定数アロケータページサイズ |

### 4.3 ディスクリプタヒープ

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.BindlessResourceDescriptorHeapSize` | int32 | バインドレスリソースディスクリプタヒープサイズ |
| `D3D12.BindlessResourceDescriptorGarbageCollectLatency` | int32 | GCレイテンシ（フレーム） |
| `D3D12.BindlessSamplerDescriptorHeapSize` | int32 | バインドレスサンプラーヒープサイズ |
| `D3D12.GlobalResourceDescriptorHeapSize` | int32 | グローバルリソースヒープサイズ |
| `D3D12.GlobalSamplerDescriptorHeapSize` | int32 | グローバルサンプラーヒープサイズ |
| `D3D12.LocalViewHeapSize` | int32 | ローカルビューヒープサイズ |
| `D3D12.OnlineDescriptorHeapSize` | int32 | オンラインディスクリプタヒープサイズ |
| `D3D12.OnlineDescriptorHeapBlockSize` | int32 | オンラインヒープブロックサイズ |

### 4.4 デバッグ・診断

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.Debug` | int32 | D3D12デバッグレイヤー有効化 |
| `D3D12.EnableDRED` | int32 | DRED（Device Removed Extended Data）有効化 |
| `D3D12.EnableLightweightDRED` | int32 | 軽量DRED有効化 |
| `D3D12.TrackAllAllocations` | int32 | 全割り当ての追跡 |
| `D3D12.GPUTimeout` | int32 | GPUタイムアウト有効化 |
| `D3D12.ExtraDiagnosticBufferMemory` | int32 | 追加診断バッファメモリ |
| `D3D12.RHIStablePowerState` | int32 | 安定電力状態（プロファイル用） |
| `D3D12.SegListTrackLeaks` | int32 | セグメントリストリーク追跡 |
| `D3D12.FastAllocatorMinPagesToRetain` | int32 | 保持する最小ページ数 |

### 4.5 レジデンシー管理

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.ResidencyManagement` | int32 | レジデンシー管理有効化 |
| `D3D12.TrackedReleasedAllocationFrameRetention` | int32 | 解放アロケーションのフレーム保持数 |
| `D3D12.EvictAllResidentResourcesInBackground` | int32 | バックグラウンドで全リソース退避 |

### 4.6 レイトレーシング (D3D12)

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.RayTracing.DebugForceFastTrace` | int32 | FastTrace強制 |
| `D3D12.RayTracing.ShaderRecordCache` | int32 | シェーダーレコードキャッシュ |
| `D3D12.RayTracing.AllowCompaction` | int32 | ASコンパクション許可 |
| `D3D12.RayTracing.MaxBatchedCompaction` | int32 | バッチコンパクション最大数 |
| `D3D12.RayTracing.CompactionMinPrimitiveCount` | int32 | コンパクション最小プリミティブ数 |
| `D3D12.RayTracing.SpecializeStateObjects` | int32 | 状態オブジェクト特殊化 |
| `D3D12.RayTracing.AllowSpecializedStateObjects` | int32 | 特殊化状態オブジェクト許可 |

### 4.7 バリア・コマンドリスト

| CVar | タイプ | 説明 |
|------|--------|------|
| `D3D12.PreferredBarrierImplementation` | int32 | バリア実装選択（0=Legacy, 1=Enhanced） |
| `D3D12.MaxCommandsPerCommandList` | int32 | コマンドリストあたり最大コマンド数 |
| `D3D12.AllowAsyncCompute` | int32 | 非同期コンピュート許可 |
| `D3D12.EmitRgpFrameMarkers` | int32 | RGPフレームマーカー出力 |
| `D3D12.UnsafeCrossGPUTransfers` | int32 | クロスGPU転送（非安全） |

---

## 5. VulkanRHI固有 CVar

### 5.1 キュー・デバイス設定

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.RHIAllowAsyncCompute` | int32 | 非同期コンピュートキュー許可 |
| `r.Vulkan.RHIAllowTransferQueue` | int32 | 転送キュー許可 |
| `r.Vulkan.AllowConcurrentBuffer` | int32 | 同時バッファアクセス許可 |
| `r.Vulkan.AllowPresentOnComputeQueue` | int32 | コンピュートキューでのPresent許可 |
| `r.Vulkan.RobustBufferAccess` | int32 | 堅牢バッファアクセス有効化 |
| `r.Vulkan.UseD24` | int32 | D24深度フォーマット使用 |
| `r.Vulkan.TempBlockSizeKB` | int32 | 一時ブロックサイズ（KB） |
| `r.Vulkan.UseMinimalSubmits` | int32 | 最小サブミット使用 |
| `r.Vulkan.DelayAcquireBackBuffer` | int32 | バックバッファ取得遅延 |
| `r.Vulkan.DebugMarkers` | int32 | デバッグマーカー有効化 |

### 5.2 Vulkan拡張機能

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.Allow64bitShaderAtomics` | int32 | 64ビットシェーダーアトミック |
| `r.Vulkan.Allow16bitOps` | int32 | 16ビット演算 |
| `r.Vulkan.RayTracing` | int32 | レイトレーシング有効化 |
| `r.Vulkan.AllowHostQueryReset` | int32 | ホストクエリリセット |
| `r.Vulkan.AllowSync2Barriers` | int32 | VK_KHR_synchronization2使用 |
| `r.Vulkan.AllowDynamicRendering` | int32 | 動的レンダリング使用 |
| `r.Vulkan.AllowDynamicStates` | int32 | 動的ステート使用 |
| `r.Vulkan.AllowShaderObject` | int32 | シェーダーオブジェクト使用 |
| `r.Vulkan.AllowGraphicPipelineLibrary` | int32 | グラフィックパイプラインライブラリ |
| `r.Vulkan.AllowHostImageCopy` | int32 | ホストイメージコピー |
| `r.Vulkan.VariableRateShadingFormat` | int32 | VRSフォーマット |

### 5.3 バリア・同期

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.UseMemoryBarrierOpt` | int32 | メモリバリア最適化 |
| `r.Vulkan.MaxBarriersPerBatch` | int32 | バッチあたり最大バリア数 |
| `r.Vulkan.AllowSplitBarriers` | int32 | スプリットバリア許可 |

### 5.4 バインドレスディスクリプタ

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.BindlessMaxSamplerDescriptorCount` | int32 | 最大サンプラー数 |
| `r.Vulkan.BindlessMaxSampledImageCount` | int32 | 最大サンプルイメージ数 |
| `r.Vulkan.BindlessMaxStorageImageCount` | int32 | 最大ストレージイメージ数 |
| `r.Vulkan.BindlessMaxUniformTexelBufferCount` | int32 | 最大UBO Texel Buffer数 |
| `r.Vulkan.BindlessMaxStorageTexelBufferCount` | int32 | 最大SSBO Texel Buffer数 |
| `r.Vulkan.BindlessMaxUniformBufferCount` | int32 | 最大UBO数 |
| `r.Vulkan.BindlessMaxStorageBufferCount` | int32 | 最大SSBO数 |
| `r.Vulkan.BindlessMaxAccelerationStructureCount` | int32 | 最大AS数 |
| `r.Vulkan.BindlessBlockSize` | int32 | バインドレスブロックサイズ |

### 5.5 パイプラインキャッシュ (Vulkan)

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.PipelineDebugForceEvictImmediately` | int32 | 即時退避強制 |
| `r.Vulkan.PipelineLRUCacheEvictBinaryPreloadScreen` | int32 | プリロード画面でバイナリ退避 |
| `r.Vulkan.EnableLRU` | int32 | LRUキャッシュ有効化 |
| `r.Vulkan.PipelineLRUCacheEvictBinary` | int32 | LRUキャッシュバイナリ退避 |
| `r.Vulkan.LRUMaxPipelineSize` | int32 | LRU最大パイプラインサイズ |
| `r.Vulkan.LRUPipelineCapacity` | int32 | LRUパイプライン容量 |
| `r.Vulkan.EnablePipelineCacheLoad` | int32 | パイプラインキャッシュ読み込み |
| `r.Vulkan.PipelineCacheFromShaderPipelineCache` | int32 | SPCからパイプラインキャッシュ作成 |
| `r.Vulkan.EnablePipelineCacheCompression` | int32 | キャッシュ圧縮有効化 |

### 5.6 Chunkedパイプラインキャッシュ

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.UseNewCacheCode` | int32 | 新キャッシュコード使用 |
| `r.Vulkan.MaxSingleCachePSOCount` | int32 | 単一キャッシュ最大PSO数 |
| `r.Vulkan.TargetResidentCacheSize` | int32 | ターゲットレジデントキャッシュサイズ |
| `r.Vulkan.MaxTotalCacheSizeMb` | int32 | 最大総キャッシュサイズ（MB） |
| `r.Vulkan.MemoryMapChunkedPSOCache` | int32 | メモリマップキャッシュ |
| `r.Vulkan.ChunkEvictTime` | int32 | チャンク退避時間 |

### 5.7 バッファ・メモリ

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.ForceStagingBufferOnLock` | int32 | ロック時ステージングバッファ強制 |
| `r.Vulkan.SparseBufferAllocSizeMB` | int32 | スパースバッファ割り当てサイズ（MB） |
| `r.Vulkan.DynamicGlobalUBs` | int32 | 動的グローバルUBO |

### 5.8 検証レイヤー

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Vulkan.Validation` | int32 | Vulkan検証レイヤー有効化 |
| `r.Vulkan.GPUValidation` | int32 | GPU検証有効化 |
| `r.Vulkan.UseProfileCheck` | int32 | プロファイルチェック使用 |
| `r.Vulkan.AllowSparseResidency` | int32 | スパースレジデンシー許可 |

---

## 6. NVIDIA Aftermath CVar

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.GPUCrashDebugging.Aftermath` | int32 | Aftermath有効化 |
| `r.GPUCrashDebugging.Aftermath.Markers` | int32 | マーカー収集 |
| `r.GPUCrashDebugging.Aftermath.Callstack` | int32 | コールスタック収集 |
| `r.GPUCrashDebugging.Aftermath.Resources` | int32 | リソース追跡 |
| `r.GPUCrashDebugging.Aftermath.ShaderErrorReporting` | int32 | シェーダーエラー報告 |
| `r.GPUCrashDebugging.Aftermath.TrackAll` | int32 | 全追跡 |
| `r.GPUCrashDebugging.Aftermath.ShaderRegistration` | int32 | シェーダー登録モード |
| `r.GPUCrashDebugging.Aftermath.DumpShaderDebugInfo` | float | シェーダーデバッグ情報ダンプ |
| `r.GPUCrashDebugging.Aftermath.LateShaderAssociationsTimeLimit` | float | 遅延シェーダー関連付け時間制限 |
| `r.GPUCrashDebugging.Aftermath.LateShaderAssociationsFrameLimit` | int32 | 遅延シェーダー関連付けフレーム制限 |
| `r.GPUCrashDebugging.Aftermath.DumpProcessWaitTime` | float | ダンプ処理待機時間 |
| `r.GPUCrashDebugging.Aftermath.DumpStartWaitTime` | float | ダンプ開始待機時間 |

---

## 7. Intel Crash Dumps CVar

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Intel.CrashDumps` | int32 | Intel クラッシュダンプ有効化 |
| `r.Intel.CrashDumps.Markers` | int32 | マーカー収集 |
| `r.Intel.CrashDumps.Callstack` | int32 | コールスタック収集 |
| `r.Intel.CrashDumps.Resources` | int32 | リソース追跡 |
| `r.Intel.CrashDumps.DumpWaitTime` | float | ダンプ待機時間 |

---

## 8. Transient Resource Allocator CVar

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.RHI.TransientAllocator.ParallelResourceCreation` | int32 | 並列リソース作成 |
| `r.RHI.TransientAllocator.MinimumHeapSize` | int32 | 最小ヒープサイズ |
| `r.RHI.TransientAllocator.BufferCacheSize` | int32 | バッファキャッシュサイズ |
| `r.RHI.TransientAllocator.TextureCacheSize` | int32 | テクスチャキャッシュサイズ |
| `r.RHI.TransientAllocator.GarbageCollectLatency` | int32 | GCレイテンシ（フレーム） |

---

## 9. Pool Allocator CVar

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.RHI.PoolAllocator.DefragSizeFraction` | float | デフラグサイズ割合 |
| `r.RHI.PoolAllocator.DefragMaxPoolsToClear` | int32 | クリアする最大プール数 |
| `r.RHI.PoolAllocator.ValidateLinkList` | int32 | リンクリスト検証 |

---

## 10. GPU Defrag CVar

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.GPUDefrag.DumpRelocationsToTTY` | int32 | リロケーションのTTYダンプ |
| `r.GPUDefrag.EnableTimeLimits` | int32 | 時間制限有効化 |
| `r.GPUDefrag.MaxRelocations` | int32 | 最大リロケーション数 |
| `r.GPUDefrag.AllowOverlappedMoves` | int32 | オーバーラップ移動許可 |

---

## 11. その他の重要CVar

### 11.1 RHI一般

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.RHI.Name` | FString | 使用するRHI名 |
| `r.RHICmdWidth` | int32 | RHIコマンドリスト幅 |
| `r.DisableEngineAndAppRegistration` | int32 | エンジン・アプリ登録無効化 |
| `r.GraphicsAdapter` | int32 | 使用するグラフィックスアダプタインデックス |
| `r.RHI.RenderPass` | int32 | レンダーパスモード |
| `r.EnableBindless` | int32 | バインドレス有効化 |
| `r.WarnOfBadDrivers` | int32 | 不良ドライバ警告 |
| `r.BadDriverWarningIsFatal` | int32 | 不良ドライバ警告を致命的エラーに |

### 11.2 Android固有

| CVar | タイプ | 説明 |
|------|--------|------|
| `r.Android.DisablePSOSchedulingParams` | int32 | PSOスケジューリングパラメータ無効化 |
| `r.Android.MinPriPSOPrecacheAffinity` | int32 | 最小優先度PSOアフィニティ |
| `r.Android.NormalPriPSOPrecacheAffinity` | int32 | 通常優先度PSOアフィニティ |
| `r.Android.MaxPriPSOPrecacheAffinity` | int32 | 最大優先度PSOアフィニティ |
| `r.Android.ExternalCompilerFailureThreshold` | int32 | 外部コンパイラ失敗閾値 |

---

## 12. CVar使用のベストプラクティス

### 12.1 開発時設定

```ini
# DefaultEngine.ini - 開発用

[/Script/Engine.RendererSettings]
r.GPUCrashDebugging=1
r.GPUCrashDebugging.Breadcrumbs=1

[D3D12RHI]
D3D12.Debug=1
D3D12.EnableDRED=1

[VulkanRHI]
r.Vulkan.Validation=1
```

### 12.2 シッピング時最適化

```ini
# DefaultEngine.ini - シッピング用

[/Script/Engine.RendererSettings]
r.AsyncPipelineCompile=1
r.PSOPrecaching=1

[D3D12RHI]
D3D12.ResidencyManagement=1
D3D12.VRAMBufferPoolDefrag=1

[VulkanRHI]
r.Vulkan.EnablePipelineCacheCompression=1
r.Vulkan.EnableLRU=1
```

### 12.3 プロファイリング設定

```cpp
// コンソールコマンドでの使用
ProfileGPU           // GPU プロファイル開始
stat D3D12RHI        // D3D12統計表示
stat VulkanRHI       // Vulkan統計表示
stat RHICMDLIST      // RHIコマンドリスト統計
stat PipelineStateCache // PSOキャッシュ統計

// プロファイル出力カスタマイズ
r.ProfileGPU.Sort=1
r.ProfileGPU.ThresholdPercent=0.5
```

---

## 13. 関連ファイル

| ファイル | 内容 |
|----------|------|
| `RHI/Public/RHIStats.h` | RHI統計定義 |
| `RHI/Private/GPUProfiler.cpp` | GPUプロファイラCVar |
| `RHI/Private/PipelineStateCache.cpp` | PSOキャッシュCVar |
| `RHI/Private/GPUDefragAllocator.cpp` | GPUデフラグCVar |
| `D3D12RHI/Private/D3D12Adapter.cpp` | D3D12アダプタCVar |
| `D3D12RHI/Private/D3D12Stats.h` | D3D12統計定義 |
| `VulkanRHI/Private/VulkanExtensions.cpp` | Vulkan拡張CVar |
| `VulkanRHI/Private/VulkanDevice.cpp` | Vulkanデバイス設定 |
| `RHICore/Private/RHICoreNvidiaAftermath.cpp` | Aftermath CVar |
| `RHICore/Private/RHICoreIntelBreadcrumbs.cpp` | Intel CVar |

---

## 更新履歴

| 日付 | 内容 |
|------|------|
| 2026-02-04 | 初版作成 - Stats/CVars網羅的ドキュメント |

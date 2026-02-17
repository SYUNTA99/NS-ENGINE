# RHI 設計仕様 — 付録A: 型・ケイパビリティリファレンス

---

## 1. ケイパビリティフラグ一覧

### 1.1 描画・ディスパッチ

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsDrawIndirect | true | 間接描画コマンド |
| SupportsMultiDrawIndirect | false | GPU生成の可変サブコマンド数 |
| SupportsBaseVertexIndex | true | 非ゼロベース頂点インデックス |
| SupportsFirstInstance | false | 描画呼び出し時のファーストインスタンスID指定 |

### 1.2 シェーダー操作

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsPixelShaderUAVs | true | ピクセルシェーダーからのUAV書き込み |
| SupportsVertexShaderUAVs | false | 頂点シェーダーからのUAV書き込み |
| SupportsWaveOperations | false | Wave/Subgroup操作（SM6.0） |
| SupportsNative16BitOps | false | 16ビットVALU & リソースロード（SM6.2） |
| SupportsAtomicUInt64 | false | 64ビット符号なしアトミック操作 |
| SupportsArrayIndexFromAnyShader | false | 任意シェーダーステージからのRT配列インデックス |
| SupportsStencilRefFromPixelShader | false | ピクセル毎のステンシル参照出力 |
| SupportsPrimitiveShaders | false | プリミティブシェーダー |
| SupportsShaderRootConstants | false | シェーダールート定数 |
| SupportsShaderTimestamp | false | シェーダー内タイムスタンプ（ベンダー拡張） |
| SupportsBarycentricsSemantic | false | 重心座標セマンティクス |
| SupportsShaderExecutionReordering | false | シェーダー実行リオーダリング (SER) |

### 1.3 メッシュシェーダー

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsMeshShadersTier0 | false | Amplification + 基本メッシュシェーダー |
| SupportsMeshShadersTier1 | false | フルパイプライン + クリップ距離 |

### 1.4 レイトレーシング

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsRayTracing | false | AS構築 + RT専用シェーダー |
| SupportsRayTracingShaders | false | フルRTパイプライン（raygen/miss/hit） |
| SupportsRayTracingPSOAdditions | false | 既存RT PSOへのシェーダー追加 |
| SupportsRayTracingDispatchIndirect | false | 間接RTディスパッチ |
| SupportsAsyncBuildAccelerationStructure | false | 非同期AS構築 |
| SupportsInlineRayTracing | false | コンピュートからのRT |
| SupportsAsyncRayTraceDispatch | false | 非同期レイトレースディスパッチ |
| SupportsClusterOps | false | クラスタ操作 |
| SupportsAccelerationStructureCompaction | false | ASコンパクション |
| SupportsSerializeAccelerationStructure | false | ASファイルシリアライゼーション |

### 1.5 可変レートシェーディング (VRS)

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsPipelineVariableRateShading | false | per-draw VRS |
| SupportsLargerVariableRateShadingSizes | false | 2×4, 4×2, 4×4 サイズ |
| SupportsAttachmentVariableRateShading | false | イメージベースVRS |
| SupportsComplexVariableRateShadingCombiners | false | 複合コンバイナ操作 |
| SupportsLateVariableRateShadingUpdate | false | フレーム中VRS更新 |

### 1.6 ワークグラフ

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsShaderWorkGraphsTier1 | false | ワークグラフ Tier 1.0 |
| SupportsShaderWorkGraphsTier1_1 | false | ワークグラフ Tier 1.1 |

### 1.7 シェーダーバンドル

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsShaderBundleDispatch | false | ネイティブシェーダーバンドルディスパッチ |
| SupportsShaderBundleWorkGraphDispatch | false | ワークグラフバンドルディスパッチ |
| SupportsShaderBundleParallel | false | 並列バンドルディスパッチ + RHI並列変換 |

### 1.8 テクスチャ・バッファ

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsTextureViews | true | テクスチャビュー（データエイリアシング） |
| SupportsTextureStreaming | false | テクスチャストリーミング |
| SupportsRWTextureBuffers | true | 読み書きテクスチャバッファ |
| SupportsRawViewsForAnyBuffer | false | フラグなしのRaw (ByteAddress) ビュー |
| SupportsAsyncTextureCreation | false | 非同期テクスチャ作成 |
| SupportsVolumeTextureRendering | true | ボリュームテクスチャへのレンダリング |
| SupportsTexture3D | true | 3Dテクスチャ初期化 |
| SupportsWideMRT | true | 256ビットMRT |
| SupportsImageExternal | false | 外部イメージ |

### 1.9 メモリ・リソース

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsDirectGPUMemoryMap | false | マップが永続GPUメモリポインタを返す |
| SupportsMapWriteNoOverwrite | false | ERHIMapMode::WriteOnly_NoOverwrite モード |
| SupportsEfficientUploadOnResourceCreation | false | 作成時の効率的アップロード |
| SupportsEfficientAsyncCompute | false | 効率的な非同期コンピュート |
| ReservedResources.Supported | false | 予約（タイル/仮想/スパース）リソース |
| ReservedResources.SupportsVolumeTextures | false | 予約ボリュームテクスチャ |

### 1.10 深度・ステンシル

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsDepthUAV | false | 深度ターゲットUAV |
| SupportsSeparateDepthStencilCopyAccess | true | 深度/ステンシル個別コピーアクセス |
| SupportsDepthStencilResolve | false | 深度/ステンシルMSAAリゾルブ |
| SupportsDepthBoundsTest | false | 深度バウンドテスト |

### 1.11 MSAA

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsMSAADepthSampleAccess | false | MSAAサンプルロード実行 |
| SupportsMSAAShaderResolve | false | カスタムシェーダーMSAAリゾルブ |

### 1.12 スレッディング

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsMultithreading | false | マルチスレッドデバイスコンテキストコマンド |
| SupportsMultithreadedShaderCreation | true | マルチスレッドシェーダー作成 |
| SupportsMultithreadedResources | false | 遅延リストでの並列リソース操作 |
| SupportsRHIThread | false | 専用RHIスレッド |
| SupportsRHIOnTaskThread | false | 任意タスクスレッドでのRHIコマンド |
| SupportsParallelRHIExecute | false | 並列RHIスレッド実行/変換 |
| SupportsParallelRenderPasses | false | 並列レンダーパス |

### 1.13 レンダリング機能

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsDynamicResolution | false | 動的解像度 |
| SupportsHDROutput | false | HDR出力 |
| SupportsBackBufferWithCustomDepthStencil | true | カスタム深度ステンシル付きバックバッファ |
| SupportsUAVFormatAliasing | false | UAVフォーマットエイリアシング |
| SupportsRasterOrderViews | false | ラスターオーダービュー（ROV） |
| SupportsConservativeRasterization | false | 保守的ラスタライゼーション |
| SupportsLossyFramebufferCompression | false | 非可逆フレームバッファ圧縮 |
| SupportsSeparateRenderTargetBlendState | false | per-RT ブレンド状態 |
| SupportsDualSrcBlending | true | デュアルソースブレンディング |

### 1.14 モバイル固有

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsShaderFramebufferFetch | false | モバイルフレームバッファフェッチ |
| SupportsShaderMRTFramebufferFetch | false | MRTフレームバッファフェッチ |
| SupportsPixelLocalStorage | false | モバイルピクセルローカルストレージ |
| SupportsShaderDepthStencilFetch | false | モバイル深度&ステンシルフェッチ |
| SupportsMobileMultiView | false | モバイルマルチビュー |

### 1.15 バインドレス

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| SupportsBindless | false | ダイナミック（バインドレス）リソース |

---

## 2. ワークアラウンドフラグ（RHIPlatformWorkarounds構造体）

### リソース状態遷移関連

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| needsExtraTransitions | false | COPYSRC/COPYDEST状態への追加遷移が必要 |
| needsTransientDiscardStateTracking | false | Transient ResourceのDiscard状態追跡が必要 |
| needsTransientDiscardOnGraphicsWorkaround | false | AsyncCompute→GraphicsのDiscard遷移回避 |
| needsSRVGraphicsNonPixelWorkaround | false | 非ピクセルシェーダーSRVの手動遷移が必要 |

### リソース削除関連

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| needsExtraDeletionLatency | false | リソース削除に追加の遅延が必要 |
| forceNoDeletionLatencyForStreamingTextures | false | ストリーミングテクスチャの削除遅延を強制無効化 |

### シェーダー関連

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| needsShaderUnbinds | false | 明示的なシェーダーアンバインドが必要 |

### レンダリング関連

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| needsUnatlasedCSMDepthsWorkaround | false | カスケードシャドウマップのアトラス化回避 |

### フォーマット関連

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| needsBC4FormatEmulation | false | BC4フォーマットエミュレーションが必要 |
| needsBC5FormatEmulation | false | BC5フォーマットエミュレーションが必要 |

### 同期関連

| フラグ | デフォルト | 説明 |
|--------|----------|------|
| needsFenceValuePadding | false | フェンス値の追加マージンが必要 |
| needsExplicitCommandListSync | false | コマンドリスト間の追加同期が必要 |

---

## 3. 数値リミット

### 3.1 テクスチャリミット

| リミット | デフォルト | 説明 |
|---------|----------|------|
| MAX_TEXTURE_MIP_COUNT | 15 | テクスチャ毎の最大Mipレベル |
| MaxTextureDimensions | 2048 | 2Dテクスチャ最大サイズ |
| MaxVolumeTextureDimensions | 2048 | 3Dテクスチャ最大サイズ |
| MaxCubeTextureDimensions | 2048 | キューブテクスチャ最大サイズ |
| MaxShadowDepthBufferSizeX | 2048 | シャドウ深度バッファ最大X |
| MaxShadowDepthBufferSizeY | 2048 | シャドウ深度バッファ最大Y |
| MaxTextureArrayLayers | 256 | テクスチャ配列最大レイヤー |
| MaxTextureSamplers | 16 | テクスチャサンプラー最大数 |

### 3.2 バッファリミット

| リミット | デフォルト | 説明 |
|---------|----------|------|
| MaxConstantBufferByteSize | 128 MB | 定数バッファ最大バイト |
| MaxComputeSharedMemory | 32 KB | 共有コンピュートメモリ最大 |
| MaxViewDimensionForTypedBuffer | 4G | 型付きバッファ最大要素数 |
| MaxViewSizeBytesForNonTypedBuffer | 4G | 非型付きバッファ最大バイト |
| MaxSimultaneousUAVs | 8 | 同時UAVバインディング最大数 |

### 3.3 ディスパッチリミット

| リミット | デフォルト | 説明 |
|---------|----------|------|
| MaxComputeDispatchDimension | 65535 | ディスパッチ次元毎の最大サイズ |
| MaxWorkGroupInvocations | 1024 | ワークグループ最大呼び出し数 |
| MinimumWaveSize | 0 | SIMD Wave最小レーン数（0=不明） |
| MaximumWaveSize | 0 | SIMD Wave最大レーン数 |

### 3.4 レンダーターゲットリミット

| リミット | 値 | 説明 |
|---------|---|------|
| MaxSimultaneousRenderTargets | 8 | 同時レンダーターゲット最大数 |
| MaxImmutableSamplers | 2 | 不変サンプラー最大数 |
| MaxVertexElementCount (kMaxVertexElements) | 16 | 頂点宣言最大要素数 |

### 3.5 マルチGPU

| リミット | 値 | 説明 |
|---------|---|------|
| MAX_NUM_GPUS | 8 (MGPU有効) / 1 (無効) | 最大GPU数 |

### 3.6 レイトレーシングアラインメント

| リミット | デフォルト | 説明 |
|---------|----------|------|
| AccelerationStructureAlignment | 0 | AS アラインメント要件 |
| ScratchBufferAlignment | 0 | スクラッチバッファアラインメント |
| ShaderTableAlignment | 0 | SBT バッファアラインメント |
| InstanceDescriptorSize | 0 | インスタンスバッファ要素サイズ |

### 3.7 VRS パラメータ

| リミット | デフォルト | 説明 |
|---------|----------|------|
| ImageTileMaxWidth | 0 | VRSイメージタイル最大幅 |
| ImageTileMaxHeight | 0 | VRSイメージタイル最大高 |
| ImageTileMinWidth | 0 | VRSイメージタイル最小幅 |
| ImageTileMinHeight | 0 | VRSイメージタイル最小高 |

### 3.8 スパースリソース

| リミット | 値 | 説明 |
|---------|---|------|
| TileSizeInBytes | 65536 | マッピング可能最小物理メモリ単位 |
| TextureArrayMinimumMipDimension | 256 | テクスチャ配列最小Mip次元 |

---

## 4. 列挙型リファレンス

### 4.1 シェーダー関連

#### EShaderFrequency — シェーダーステージ識別

| 値 | 名前 | パイプライン |
|----|------|------------|
| 0 | Vertex | Graphics |
| 1 | Pixel | Graphics |
| 2 | Geometry | Graphics (レガシー) |
| 3 | Hull | Graphics (テッセレーション制御) |
| 4 | Domain | Graphics (テッセレーション評価) |
| 5 | Compute | Compute |
| 6 | Mesh | Graphics (SM6.5+) |
| 7 | Amplification | Graphics (SM6.5+) |
| 8 | RayGen | RayTracing |
| 9 | RayMiss | RayTracing |
| 10 | RayClosestHit | RayTracing |
| 11 | RayAnyHit | RayTracing |
| 12 | RayIntersection | RayTracing |
| 13 | RayCallable | RayTracing |
| 14 | WorkGraphRoot | WorkGraph (SM6.8+) |
| 15 | WorkGraphComputeNode | WorkGraph (SM6.8+) |

定数: Count=16, NumGraphicsFrequencies=5 (VS/PS/GS/HS/DS)
エイリアス: Fragment=Pixel, Task=Amplification

### 4.2 アクセス状態

#### ERHIAccess — リソースアクセス状態（ビットフラグ, uint32）

Unknown = 0

**CPUアクセス:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 0 | CPURead | CPU読み取り |
| 1 | CPUWrite | CPU書き込み |

**頂点/インデックス/定数:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 2 | VertexBuffer | 頂点バッファ |
| 3 | IndexBuffer | インデックスバッファ |
| 4 | ConstantBuffer | 定数バッファ |

**シェーダーリソース:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 5 | SRVGraphics | グラフィクスSRV |
| 6 | SRVCompute | コンピュートSRV |
| 7 | UAVGraphics | グラフィクスUAV |
| 8 | UAVCompute | コンピュートUAV |

**レンダーターゲット / 深度ステンシル:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 9 | RenderTarget | レンダーターゲット |
| 10 | DepthStencilRead | 深度ステンシル読み取り |
| 11 | DepthStencilWrite | 深度ステンシル書き込み |

**コピー / リゾルブ:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 12 | CopySource | コピーソース |
| 13 | CopyDest | コピーデスティネーション |
| 14 | ResolveSource | リゾルブソース |
| 15 | ResolveDest | リゾルブデスティネーション |

**その他:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 16 | Present | 表示出力 |
| 17 | IndirectArgs | 間接引数 |
| 18 | StreamOutput | ストリーム出力 |

**レイトレーシング / VRS:**

| ビット | 名前 | 説明 |
|-------|------|------|
| 19 | AccelerationStructureRead | 加速構造読み取り |
| 20 | AccelerationStructureBuild | 加速構造ビルド |
| 21 | ShadingRateSource | VRSソース |

**合成マスク:**

| マスク | 構成 |
|--------|------|
| SRVAll | SRVGraphics \| SRVCompute |
| UAVAll | UAVGraphics \| UAVCompute |
| VertexOrIndexBuffer | VertexBuffer \| IndexBuffer |
| ReadOnly | SRVAll \| ConstantBuffer \| VertexOrIndexBuffer \| CopySource \| IndirectArgs \| DepthStencilRead |
| WriteMask | UAVAll \| RenderTarget \| DepthStencilWrite \| CopyDest \| StreamOutput |

### 4.3 パイプライン

#### ERHIPipeline — パイプライン識別（ビットフラグ, uint8）

| 値 | 名前 | 説明 |
|----|------|------|
| 0 | None | なし |
| 1 (1<<0) | Graphics | グラフィクスパイプライン |
| 2 (1<<1) | AsyncCompute | 非同期コンピュートパイプライン |
| 3 (Graphics\|AsyncCompute) | All | 全パイプライン |

#### ERHIThreadMode — RHIスレッディング構成

| 値 | 説明 |
|----|------|
| None | シングルスレッド |
| DedicatedThread | 専用RHIスレッド |
| Tasks | タスクシステム実行 |

### 4.4 描画状態

#### ERHIFilter — テクスチャフィルタリング

| 値 | 説明 |
|----|------|
| Point | 最近傍 |
| Linear | リニア |
| Anisotropic | 異方性 |

#### ERHITextureAddressMode — テクスチャ座標ラッピング

| 値 | 説明 |
|----|------|
| Wrap | 繰り返し |
| Mirror | 反転繰り返し |
| Clamp | 端クランプ |
| Border | 境界色 |
| MirrorOnce | 一度だけ反転 |

#### ERHICompareFunc — 深度/ステンシル比較

| 値 | 説明 |
|----|------|
| Never | 常に失敗 |
| Less | < |
| Equal | == |
| LessEqual | <= |
| Greater | > |
| NotEqual | != |
| GreaterEqual | >= |
| Always | 常に成功 |

#### ERHIFillMode — ポリゴン塗りつぶし

| 値 | 説明 |
|----|------|
| Point | ポイント |
| Wireframe | ワイヤーフレーム |
| Solid | ソリッド |

#### ERHICullMode — 背面カリング

| 値 | 説明 |
|----|------|
| None | カリングなし |
| Front | 前面カリング |
| Back | 背面カリング |

#### ERHIStencilOp — ステンシル操作

| 値 | 説明 |
|----|------|
| Keep | 保持 |
| Zero | ゼロ設定 |
| Replace | 置換 |
| IncrSat | 飽和加算 |
| DecrSat | 飽和減算 |
| Invert | 反転 |
| IncrWrap | ラップ加算 |
| DecrWrap | ラップ減算 |

#### ERHIBlendOp — ブレンド方程式

| 値 | 説明 |
|----|------|
| Add | Src + Dst |
| Subtract | Src - Dst |
| RevSubtract | Dst - Src |
| Min | min(Src, Dst) |
| Max | max(Src, Dst) |

#### ERHIBlendFactor — ブレンド係数

| 値 | 説明 |
|----|------|
| Zero | 0 |
| One | 1 |
| SrcColor | Src.rgb |
| InvSrcColor | 1 - Src.rgb |
| SrcAlpha | Src.a |
| InvSrcAlpha | 1 - Src.a |
| DstColor | Dst.rgb |
| InvDstColor | 1 - Dst.rgb |
| DstAlpha | Dst.a |
| InvDstAlpha | 1 - Dst.a |
| SrcAlphaSaturate | min(Src.a, 1-Dst.a) |
| BlendFactor | 定数ブレンドファクター |
| InvBlendFactor | 1 - 定数 |
| Src1Color | デュアルソース Src1.rgb |
| InvSrc1Color | 1 - Src1.rgb |
| Src1Alpha | デュアルソース Src1.a |
| InvSrc1Alpha | 1 - Src1.a |

#### ERHIColorWriteMask — カラーチャネル書き込み制御（ビットフラグ）

| ビット | 名前 |
|-------|------|
| 1<<0 | Red |
| 1<<1 | Green |
| 1<<2 | Blue |
| 1<<3 | Alpha |
| Red\|Green\|Blue | RGB |
| Red\|Green\|Blue\|Alpha | All |

### 4.5 トポロジ

#### ERHIPrimitiveTopology — 描画トポロジ

| 値 | 説明 |
|----|------|
| PointList | ポイントリスト |
| LineList | ラインリスト |
| LineStrip | ラインストリップ |
| TriangleList | 三角形リスト |
| TriangleStrip | 三角形ストリップ |
| LineListAdj | ライン隣接付きリスト |
| LineStripAdj | ライン隣接付きストリップ |
| TriangleListAdj | 三角形隣接付きリスト |
| TriangleStripAdj | 三角形隣接付きストリップ |
| PatchList | パッチリスト（テッセレーション） |

### 4.6 リソースマップ

#### ERHIMapMode — バッファ/テクスチャマップモード

| 値 | 説明 |
|----|------|
| Read | 読み取り専用 |
| Write | 書き込み専用 |
| ReadWrite | 読み書き両方 |
| WriteDiscard | 書き込み（以前の内容を破棄） |
| WriteNoOverwrite | 書き込み（同期なし - 上書きしない領域） |

### 4.7 頂点要素型

#### EVertexElementType — 頂点属性データ型

| 値 | 説明 |
|----|------|
| Float1/2/3/4 | 32ビット浮動小数点 |
| PackedNormal | パック済み法線 |
| UByte4/4N | 8ビット符号なし (N=正規化) |
| Color | BGRA 8ビットカラー |
| Short2/4/2N/4N | 16ビット符号付き (N=正規化) |
| Half2/4 | 16ビット浮動小数点 |
| UShort2/4/2N/4N | 16ビット符号なし (N=正規化) |
| URGB10A2N | 10:10:10:2 正規化 |
| UInt | 32ビット符号なし整数 |

---

## 5. バッファ使用フラグ — ERHIBufferUsage（ビットフラグ, uint32）

### 基本用途

| ビット | 名前 | 説明 |
|-------|------|------|
| 0 | VertexBuffer | 頂点バッファ |
| 1 | IndexBuffer | インデックスバッファ |
| 2 | ConstantBuffer | 定数バッファ |

### シェーダーリソース

| ビット | 名前 | 説明 |
|-------|------|------|
| 3 | ShaderResource | SRV使用 |
| 4 | UnorderedAccess | UAV作成 |

### 構造化バッファ

| ビット | 名前 | 説明 |
|-------|------|------|
| 5 | StructuredBuffer | 構造化バッファ |
| 6 | ByteAddressBuffer | バイトアドレスバッファ |

### 特殊用途

| ビット | 名前 | 説明 |
|-------|------|------|
| 7 | IndirectArgs | 間接引数バッファ |
| 8 | StreamOutput | ストリーム出力 |
| 9 | AccelerationStructure | RT加速構造 |

### メモリ・アクセスヒント

| ビット | 名前 | 説明 |
|-------|------|------|
| 10 | CPUReadable | CPU読み取り可能 |
| 11 | CPUWritable | CPU書き込み可能 |
| 12 | Dynamic | 動的バッファ（頻繁なCPU更新） |
| 13 | CopySource | コピーソース |
| 14 | CopyDest | コピーデスティネーション |

### 便利な組み合わせ

| マスク | 構成 |
|--------|------|
| DynamicVertexBuffer | VertexBuffer \| Dynamic \| CPUWritable |
| DynamicIndexBuffer | IndexBuffer \| Dynamic \| CPUWritable |
| DynamicConstantBuffer | ConstantBuffer \| Dynamic \| CPUWritable |
| Default | ShaderResource |
| Staging | CPUReadable \| CPUWritable \| CopySource \| CopyDest |

---

## 6. テクスチャ使用フラグ — ERHITextureUsage（ビットフラグ, uint32）

### シェーダーリソース

| ビット | 名前 | 説明 |
|-------|------|------|
| 0 | ShaderResource | シェーダーリソース (SRV) |
| 1 | UnorderedAccess | UAV作成 |

### レンダーターゲット / 深度ステンシル

| ビット | 名前 | 説明 |
|-------|------|------|
| 2 | RenderTarget | レンダーターゲット |
| 3 | DepthStencil | 深度ステンシルターゲット |

### スワップチェーン / 表示

| ビット | 名前 | 説明 |
|-------|------|------|
| 4 | Present | ディスプレイ表示（バックバッファ） |
| 5 | Shared | 外部共有 |

### CPUアクセス

| ビット | 名前 | 説明 |
|-------|------|------|
| 6 | CPUReadable | CPUリードバック |
| 7 | CPUWritable | CPU書き込み可能 |

### 特殊機能

| ビット | 名前 | 説明 |
|-------|------|------|
| 8 | GenerateMips | Mipマップ自動生成 |
| 9 | Virtual | 仮想テクスチャ（物理メモリなし） |
| 10 | Streamable | ストリーミング対象 |
| 11 | ShadingRateSource | VRSソーステクスチャ |
| 12 | Memoryless | タイルメモリのみ（モバイル） |
| 13 | ResolveSource | MSAAリゾルブソース |
| 14 | ResolveDest | MSAAリゾルブデスティネーション |

### 便利な組み合わせ

| マスク | 構成 |
|--------|------|
| Default | ShaderResource |
| RenderTargetShaderResource | RenderTarget \| ShaderResource |
| DepthShaderResource | DepthStencil \| ShaderResource |
| UnorderedShaderResource | UnorderedAccess \| ShaderResource |
| Staging | CPUReadable \| CPUWritable |

---

## 7. ディスクリプタシステム

### ERHIDescriptorHeapType — ヒープタイプ

| 値 | 説明 |
|----|------|
| CBV_SRV_UAV | 定数バッファ/シェーダーリソース/UAV |
| Sampler | サンプラー |
| RTV | レンダーターゲットビュー |
| DSV | デプスステンシルビュー |

### ディスクリプタタイプ

| 値 | 説明 |
|----|------|
| BufferSRV | バッファSRV |
| BufferUAV | バッファUAV |
| TypedBufferSRV | 型付きバッファSRV |
| TypedBufferUAV | 型付きバッファUAV |
| TextureSRV | テクスチャSRV |
| TextureUAV | テクスチャUAV |
| CBV | 定数バッファビュー |
| Sampler | サンプラー |
| AccelerationStructure | 加速構造 |

### バインドレス構成

| 値 | 説明 |
|----|------|
| Disabled | バインドレス無効 |
| RayTracing | RTシェーダーのみ |
| Minimal | RT + マテリアル + オプトインシェーダー |
| All | 全シェーダー |

---

## 8. GPU & デバイス情報

### GPUベンダーID（PCI-SIG値）

| 値 | ベンダー |
|----|---------|
| 0x1002 | AMD |
| 0x10DE | NVIDIA |
| 0x8086 | Intel |
| 0x106B | Apple |
| 0x13B5 | ARM |
| 0x5143 | Qualcomm |
| 0x1010 | ImgTec |
| 0x14E4 | Broadcom |
| 0x1414 | Microsoft |
| 0x144D | Samsung (AMD) |

### ERHIInterfaceType — バックエンドタイプ

| 値 | 説明 |
|----|------|
| Hidden | 非表示（テスト用、内部実装） |
| Null | Null実装（ヘッドレス） |
| D3D11 | DirectX 11 |
| D3D12 | DirectX 12 |
| Vulkan | Vulkan |
| Metal | Metal (macOS/iOS) |

### Z バッファ構成

```
FarPlane  = 0    // 遠方面の深度値
NearPlane = 1    // 近方面の深度値
IsInverted = true  // 反転Z（Far=0, Near=1）

利点: 浮動小数点精度の均一な分布
  → 遠距離でもZファイティングが大幅に減少
```

---

## 9. 設計パターン要約

| パターン | 説明 |
|---------|------|
| **ブーリアン + 数値ペアリング** | 機能存在（bool）とスケール（数値）の組み合わせ |
| **ティアベース機能** | Tier 0/1 で段階的能力を表現し、グレースフルデグラデーション |
| **ワークアラウンド隔離** | Needs* フラグでプラットフォームバグをメインラインから分離 |
| **サブ構造体グループ化** | 関連ケイパビリティをグループ化して明確性を確保 |
| **デフォルト規約** | 高度機能は false/0、基本操作は true がデフォルト |
| **ゼロ = 非サポート** | 数値リミット 0 は機能利用不可を示す |
| **ビットフラグ列挙** | 効率的な複数状態表現とテスト |
| **合成マスク** | 頻繁な複数ビットチェックを簡素化 |

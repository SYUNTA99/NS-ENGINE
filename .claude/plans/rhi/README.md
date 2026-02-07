# RHI (Render Hardware Interface) 実装計画

## 目的

NS-ENGINEにRHI抽象化レイヤーを新規実装する。既存のdx11モジュールは変更せず、新規モジュールとして追加。

## 影響範囲

- 新規: `Source/Engine/RHI/` — 抽象インターフェース
- 新規: `Source/Engine/RHICore/` — メモリ管理・ディスクリプタ基盤
- 新規: `Source/Engine/D3D12RHI/` — D3D12バックエンド実装
- 新規: `Source/Engine/NullRHI/` — テスト用Nullバックエンド
- 既存変更なし
- `premake5.lua` に `RHI`, `RHICore`, `D3D12RHI` プロジェクト定義を追加

## 基盤ドキュメント

| # | 計画 | 内容 |
|---|------|------|
| 0 | [00-01-test-strategy](00-01-test-strategy.md) | テスト戦略・モック・CI |
| 0 | [00-02-error-handling](00-02-error-handling.md) | エラー処理方針・回復戦略 |
| 0 | [00-03-dependency-graph](00-03-dependency-graph.md) | 依存関係・実装順序 |
| 0 | [00-04-performance-budget](00-04-performance-budget.md) | パフォーマンス予算・計測方法 |
| 0 | [00-05-null-rhi](00-05-null-rhi.md) | NullRHIバックエンド（CI/テスト用） |

## サブ計画

### Part 1A: 型定義基盤

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 1 | [01-01-fwd-macros](01-01-fwd-macros.md) | pending | 前方宣言・マクロ |
| 2 | [01-02-types-gpu](01-02-types-gpu.md) | pending | GPU識別型・リソース識別型 |
| 3 | [01-03-types-geometry](01-03-types-geometry.md) | pending | ジオメトリ型 |
| 4 | [01-04-types-descriptor](01-04-types-descriptor.md) | pending | ディスクリプタハンドル・定数 |

### Part 1B: 列挙型

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 5 | [01-05-enums-core](01-05-enums-core.md) | pending | バックエンド・機能レベル・キュー |
| 6 | [01-06-enums-shader](01-06-enums-shader.md) | pending | シェーダー関連 |
| 7 | [01-07-enums-access](01-07-enums-access.md) | pending | アクセス状態・ディスクリプタ |
| 8 | [01-08-enums-buffer](01-08-enums-buffer.md) | pending | バッファ使用フラグ |
| 9 | [01-09-enums-texture](01-09-enums-texture.md) | pending | テクスチャ使用フラグ |
| 10 | [01-10-enums-state](01-10-enums-state.md) | pending | レンダーステート |

### Part 1C: リソース管理基盤

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 11 | [01-11-ref-count-ptr](01-11-ref-count-ptr.md) | pending | TRefCountPtrテンプレート |
| 12 | [01-12-resource-base](01-12-resource-base.md) | pending | IRHIResource基底クラス |
| 13 | [01-13-resource-type](01-13-resource-type.md) | pending | リソースタイプ・型キャスト |
| 14 | [01-14-deferred-delete](01-14-deferred-delete.md) | pending | 遅延削除キュー |

### Part 1D: IDynamicRHI

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 15 | [01-15-dynamic-rhi-core](01-15-dynamic-rhi-core.md) | pending | 基本インターフェース |
| 15b | [01-15b-dynamic-rhi-module](01-15b-dynamic-rhi-module.md) | pending | モジュール発見・選択・インスタンス化 |
| 16 | [01-16-dynamic-rhi-factory](01-16-dynamic-rhi-factory.md) | pending | リソースファクトリ |
| 17 | [01-17-dynamic-rhi-command](01-17-dynamic-rhi-command.md) | pending | コマンド実行管理 |
| 17b | [01-17b-submission-pipeline](01-17b-submission-pipeline.md) | pending | サブミッションパイプライン・スレッドモデル |

### Part 1E: IRHIAdapter

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 18 | [01-18-adapter-desc](01-18-adapter-desc.md) | pending | アダプター記述子 |
| 19 | [01-19-adapter-interface](01-19-adapter-interface.md) | pending | IRHIAdapterインターフェース |

### Part 1F: IRHIDevice

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 20 | [01-20-device-core](01-20-device-core.md) | pending | 基本プロパティ |
| 21 | [01-21-device-queue](01-21-device-queue.md) | pending | キュー管理 |
| 22 | [01-22-device-context](01-22-device-context.md) | pending | コンテキスト管理 |
| 23 | [01-23-device-descriptor](01-23-device-descriptor.md) | pending | ディスクリプタ管理 |
| 24 | [01-24-device-memory](01-24-device-memory.md) | pending | メモリ管理 |

### Part 1G: IRHIQueue

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 25 | [01-25-queue-interface](01-25-queue-interface.md) | pending | IRHIQueueインターフェース |
| 26 | [01-26-queue-fence](01-26-queue-fence.md) | pending | フェンス管理 |

### Part 2: コマンドシステム

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 27 | [02-01-command-context-base](02-01-command-context-base.md) | pending | コンテキスト基底 |
| 28 | [02-02-compute-context](02-02-compute-context.md) | pending | コンピュートコンテキスト |
| 29 | [02-03-graphics-context](02-03-graphics-context.md) | pending | グラフィックスコンテキスト |
| 30 | [02-04-command-allocator](02-04-command-allocator.md) | pending | コマンドアロケーター |
| 31 | [02-05-command-list](02-05-command-list.md) | pending | コマンドリスト |
| 32 | [02-06-work-graph](02-06-work-graph.md) | pending | ワークグラフ |

### Part 3: バッファリソース

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 33 | [03-01-buffer-desc](03-01-buffer-desc.md) | pending | バッファ記述子 |
| 34 | [03-02-buffer-interface](03-02-buffer-interface.md) | pending | IRHIBufferインターフェース |
| 35 | [03-03-buffer-lock](03-03-buffer-lock.md) | pending | ロック/アンロック |
| 36 | [03-04-structured-buffer](03-04-structured-buffer.md) | pending | 構造化バッファ |

### Part 4: テクスチャリソース

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 37 | [04-01-texture-desc](04-01-texture-desc.md) | pending | テクスチャ記述子 |
| 38 | [04-02-texture-interface](04-02-texture-interface.md) | pending | IRHITextureインターフェース |
| 39 | [04-03-texture-subresource](04-03-texture-subresource.md) | pending | サブリソース |
| 40 | [04-04-texture-data](04-04-texture-data.md) | pending | データアップロード |

### Part 5: ビュー

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 41 | [05-01-srv](05-01-srv.md) | pending | シェーダーリソースビュー |
| 42 | [05-02-uav](05-02-uav.md) | pending | アンオーダードアクセスビュー |
| 43 | [05-03-rtv](05-03-rtv.md) | pending | レンダーターゲットビュー |
| 44 | [05-04-dsv](05-04-dsv.md) | pending | デプスステンシルビュー |
| 45 | [05-05-cbv](05-05-cbv.md) | pending | 定数バッファビュー |
| 46 | [05-06-gpu-profiler](05-06-gpu-profiler.md) | pending | GPUプロファイラ |

### Part 6: シェーダー

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 47 | [06-01-shader-bytecode](06-01-shader-bytecode.md) | pending | バイトコード管理 |
| 48 | [06-02-shader-interface](06-02-shader-interface.md) | pending | IRHIShaderインターフェース |
| 49 | [06-03-shader-reflection](06-03-shader-reflection.md) | pending | リフレクション |
| 50 | [06-04-shader-library](06-04-shader-library.md) | pending | シェーダーライブラリ |

### Part 7: パイプラインステート

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 51 | [07-01-blend-state](07-01-blend-state.md) | pending | ブレンドステート |
| 52 | [07-02-rasterizer-state](07-02-rasterizer-state.md) | pending | ラスタライザーステート |
| 53 | [07-03-depth-stencil-state](07-03-depth-stencil-state.md) | pending | デプスステンシルステート |
| 54 | [07-04-input-layout](07-04-input-layout.md) | pending | 入力レイアウト |
| 55 | [07-05-graphics-pso](07-05-graphics-pso.md) | pending | グラフィックスPSO |
| 56 | [07-06-compute-pso](07-06-compute-pso.md) | pending | コンピュートPSO |
| 57 | [07-07-variable-rate-shading](07-07-variable-rate-shading.md) | pending | Variable Rate Shading |

### Part 8: ルートシグネチャ

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 58 | [08-01-root-parameter](08-01-root-parameter.md) | pending | ルートパラメータ |
| 59 | [08-02-descriptor-range](08-02-descriptor-range.md) | pending | ディスクリプタレンジ |
| 60 | [08-03-root-signature](08-03-root-signature.md) | pending | ルートシグネチャ |
| 61 | [08-04-binding-layout](08-04-binding-layout.md) | pending | バインディングレイアウト |

### Part 9: 同期

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 62 | [09-01-fence-interface](09-01-fence-interface.md) | pending | IRHIFenceインターフェース |
| 63 | [09-02-sync-point](09-02-sync-point.md) | pending | 同期ポイント |
| 64 | [09-03-gpu-event](09-03-gpu-event.md) | pending | GPUイベント |

### Part 10: ディスクリプタ管理

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 65 | [10-01-descriptor-heap](10-01-descriptor-heap.md) | pending | ディスクリプタヒープ |
| 66 | [10-02-online-descriptor](10-02-online-descriptor.md) | pending | オンラインディスクリプタ |
| 67 | [10-03-offline-descriptor](10-03-offline-descriptor.md) | pending | オフラインディスクリプタ |
| 68 | [10-04-bindless](10-04-bindless.md) | pending | Bindlessサポート |

### Part 11: メモリ管理

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 69 | [11-01-heap-types](11-01-heap-types.md) | pending | ヒープタイプ |
| 70 | [11-02-buffer-allocator](11-02-buffer-allocator.md) | pending | バッファアロケーター |
| 71 | [11-03-texture-allocator](11-03-texture-allocator.md) | pending | テクスチャアロケーター |
| 72 | [11-04-upload-heap](11-04-upload-heap.md) | pending | アップロードヒープ |
| 73 | [11-05-residency](11-05-residency.md) | pending | メモリ常駐管理 |
| 74 | [11-06-reserved-resources](11-06-reserved-resources.md) | pending | Reserved/Sparseリソース |
| 75 | [11-07-platform-workarounds](11-07-platform-workarounds.md) | pending | プラットフォーム回避策 |

### Part 12: スワップチェーン

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 76 | [12-01-swapchain-desc](12-01-swapchain-desc.md) | pending | スワップチェーン記述 |
| 77 | [12-02-swapchain-interface](12-02-swapchain-interface.md) | pending | IRHISwapChain |
| 78 | [12-03-present](12-03-present.md) | pending | Present操作 |
| 79 | [12-04-hdr](12-04-hdr.md) | pending | HDRサポート |

### Part 13: レンダーパス

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 80 | [13-01-render-pass-desc](13-01-render-pass-desc.md) | pending | レンダーパス記述 |
| 81 | [13-02-load-store-action](13-02-load-store-action.md) | pending | ロード/ストアアクション |
| 82 | [13-03-render-pass](13-03-render-pass.md) | pending | レンダーパス実行 |

### Part 14: クエリ

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 83 | [14-01-query-types](14-01-query-types.md) | pending | クエリタイプ |
| 84 | [14-02-query-pool](14-02-query-pool.md) | pending | クエリプール |
| 85 | [14-03-timestamp](14-03-timestamp.md) | pending | タイムスタンプ |
| 86 | [14-04-occlusion](14-04-occlusion.md) | pending | オクルージョン |

### Part 15: ピクセルフォーマット

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 87 | [15-01-pixel-format-enum](15-01-pixel-format-enum.md) | pending | フォーマット列挙型 |
| 88 | [15-02-format-info](15-02-format-info.md) | pending | フォーマット情報 |
| 89 | [15-03-format-conversion](15-03-format-conversion.md) | pending | フォーマット変換 |

### Part 16: リソース状態

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 90 | [16-01-resource-state](16-01-resource-state.md) | pending | リソース状態 |
| 91 | [16-02-barrier](16-02-barrier.md) | pending | バリア |
| 92 | [16-03-state-tracking](16-03-state-tracking.md) | pending | 状態追跡 |

### Part 17: エラー処理

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 93 | [17-01-rhi-result](17-01-rhi-result.md) | pending | RHI チェックマクロ |
| 94 | [17-02-device-lost](17-02-device-lost.md) | pending | デバイスロスト |
| 95 | [17-03-validation](17-03-validation.md) | pending | 検証レイヤー |
| 96 | [17-04-breadcrumbs](17-04-breadcrumbs.md) | pending | GPU Breadcrumbs |

### Part 18: サンプラー

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 97 | [18-01-sampler-desc](18-01-sampler-desc.md) | pending | サンプラー記述 |
| 98 | [18-02-sampler-interface](18-02-sampler-interface.md) | pending | IRHISampler |

### Part 19: 高度な機能（後回し）

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 99 | [19-01-raytracing-as](19-01-raytracing-as.md) | deferred | 加速構造 |
| 100 | [19-02-raytracing-shader](19-02-raytracing-shader.md) | deferred | RTシェーダー |
| 101 | [19-03-raytracing-pso](19-03-raytracing-pso.md) | deferred | RT PSO |
| 102 | [19-04-multi-gpu](19-04-multi-gpu.md) | deferred | マルチGPU |

### Part 20: GPU Readback & Staging

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 103 | [20-01-staging-buffer](20-01-staging-buffer.md) | pending | ステージングバッファ |
| 104 | [20-02-gpu-readback](20-02-gpu-readback.md) | pending | GPUリードバック |
| 105 | [20-03-async-readback](20-03-async-readback.md) | pending | 非同期リードバック |
| 106 | [20-04-texture-readback](20-04-texture-readback.md) | pending | テクスチャリードバック |

### Part 21: Indirect Rendering

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 107 | [21-01-indirect-arguments](21-01-indirect-arguments.md) | pending | Indirect引数構造体 |
| 108 | [21-02-indirect-draw](21-02-indirect-draw.md) | pending | Indirect Draw |
| 109 | [21-03-indirect-dispatch](21-03-indirect-dispatch.md) | pending | Indirect Dispatch |
| 110 | [21-04-command-signature](21-04-command-signature.md) | pending | コマンドシグネチャ |
| 111 | [21-05-execute-indirect](21-05-execute-indirect.md) | pending | ExecuteIndirect |

### Part 22: メッシュシェーダー

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 112 | [22-01-mesh-shader](22-01-mesh-shader.md) | pending | メッシュシェーダー |
| 113 | [22-02-amplification-shader](22-02-amplification-shader.md) | pending | 増幅シェーダー |
| 114 | [22-03-mesh-pso](22-03-mesh-pso.md) | pending | メッシュPSO |
| 115 | [22-04-mesh-dispatch](22-04-mesh-dispatch.md) | pending | メッシュディスパッチ |

### Part 23: Transient Resources

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 116 | [23-01-transient-allocator](23-01-transient-allocator.md) | pending | Transientアロケーター |
| 117 | [23-02-transient-buffer](23-02-transient-buffer.md) | pending | Transientバッファ |
| 118 | [23-03-transient-texture](23-03-transient-texture.md) | pending | Transientテクスチャ |
| 119 | [23-04-aliasing-barrier](23-04-aliasing-barrier.md) | pending | エイリアシングバリア |

### Part 24: Shader Binding

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 120 | [24-01-uniform-buffer-layout](24-01-uniform-buffer-layout.md) | pending | UniformBufferレイアウト |
| 121 | [24-02-shader-parameter-map](24-02-shader-parameter-map.md) | pending | シェーダーパラメータマップ |
| 122 | [24-03-resource-table](24-03-resource-table.md) | pending | リソーステーブル |
| 123 | [24-04-bound-shader-state](24-04-bound-shader-state.md) | pending | バウンドシェーダーステート |

### Part 25: 統計・デバッグ

| # | 計画 | 状態 | 内容 |
|---|------|------|------|
| 124 | [25-01-command-list-stats](25-01-command-list-stats.md) | pending | コマンドリスト統計 |
| 125 | [25-02-memory-stats](25-02-memory-stats.md) | pending | メモリ統計 |
| 126 | [25-03-pso-cache-stats](25-03-pso-cache-stats.md) | pending | PSOキャッシュ統計 |

## 検証方法

1. コンパイル成功
2. インターフェース定義の整合性
3. 依存関係の正確性

## 設計決定

| 決定 | 理由 |
|------|------|
| `NS::RHI::` 名前空間 | 他モジュールと統一 |
| `IRHI〜` インターフェース | 命名規則準拠 |
| `NS::Result` エラー処理 | 例外禁止 |
| RHIResult型を持たない（UEスタイル） | UE参考。リソース作成はnullptr返却、致命的エラーはコールバック。Result型は過剰 |
| RHICheck.hにチェックマクロを集約 | デバッグビルドでのみ有効、リリースではゼロコスト |
| 仮想関数は使用可、ビルド時設定でvtableオーバーヘッド除去可能 | 開発中はvtableで柔軟に、シッピングビルドではコンパイル時バックエンド選択で静的ディスパッチに切替 |
| 細粒度サブ計画 | 大規模実装の管理に向く |

## コーディング規約: commonマクロの使用

実装時は `common/utility/macros.h` で定義されたマクロを使用すること。
コンパイラ固有の構文を直接書かない。

| 用途 | 使用するマクロ | 直接記述しない |
|------|---------------|---------------|
| コピー禁止 | `NS_DISALLOW_COPY(TypeName)` | `= delete` を手書きしない |
| ムーブ禁止 | `NS_DISALLOW_MOVE(TypeName)` | 同上 |
| コピー・ムーブ両方禁止 | `NS_DISALLOW_COPY_AND_MOVE(TypeName)` | 同上 |
| インライン強制 | `NS_FORCEINLINE` | `__forceinline` / `__attribute__((always_inline))` |
| インライン禁止 | `NS_NOINLINE` | `__declspec(noinline)` / `__attribute__((noinline))` |
| 到達不能コード | `NS_UNREACHABLE()` | `__assume(0)` / `__builtin_unreachable()` |
| 到達不能default | `NS_UNEXPECTED_DEFAULT` | 同上 |
| 非推奨マーク | `NS_DEPRECATED(msg)` | `__declspec(deprecated)` / `__attribute__((deprecated))` |
| 制限付きポインタ | `NS_RESTRICT` | `__restrict` / `__restrict__` |
| パック開始/終了 | `NS_PACK_BEGIN` / `NS_PACK_END` | `#pragma pack(push, 1)` |
| パディング挿入 | `NS_PADDING(name, size)` | `char padding[N]` |
| プラットフォーム判定 | `NS_PLATFORM_WINDOWS` 等 | `#if defined(_WIN32)` |
| コンパイラ判定 | `NS_COMPILER_MSVC` 等 | `#if defined(_MSC_VER)` |
| デバッグビルド判定 | `NS_BUILD_DEBUG` | `#if defined(_DEBUG)` |

**標準C++属性はそのまま使用する（マクロ不要）:**
`[[nodiscard]]`, `[[maybe_unused]]`, `[[noreturn]]`, `[[fallthrough]]`, `[[deprecated(msg)]]`,
`alignas(n)`, `alignof(type)`, `noexcept`, `override`

## ファイル構造

UE5に倣い、RHI関連はモジュール分割。Public/直下にフラット配置（サブディレクトリなし）。
フォルダ名はPascalCase（CLAUDE.md準拠）。

```
Source/Engine/
├── RHI/                             # 抽象インターフェース
│   ├── Public/                      #   公開API（フラット配置）
│   │   ├── RHI.h                    #     統合ヘッダー
│   │   ├── RHIFwd.h                 #     前方宣言 (01-01)
│   │   ├── RHIMacros.h              #     マクロ (01-01)
│   │   ├── RHITypes.h               #     基本型 (01-02, 01-03, 01-04)
│   │   ├── RHIEnums.h               #     列挙型 (01-05〜01-10)
│   │   ├── RHIGlobals.h            #     グローバルRHI変数 (11-07)
│   │   ├── RHIPixelFormat.h         #     ピクセルフォーマット (15-01, 15-02)
│   │   ├── RHIFormatConversion.h    #     フォーマット変換 (15-03)
│   │   ├── RHIRefCountPtr.h         #     TRefCountPtr テンプレート (01-11)
│   │   ├── IRHIResource.h           #     リソース基底クラス (01-12)
│   │   ├── RHIResourceType.h        #     リソースタイプ・型キャスト (01-13)
│   │   ├── RHIDeferredDelete.h      #     遅延削除キュー (01-14)
│   │   ├── IDynamicRHI.h            #     IDynamicRHIインターフェース (01-15, 01-17)
│   │   ├── IDynamicRHIModule.h      #     モジュール発見・選択 (01-15b)
│   │   ├── RHIAdapterDesc.h         #     アダプター記述子 (01-18)
│   │   ├── IRHIAdapter.h            #     IRHIAdapterインターフェース (01-19)
│   │   ├── IRHIDevice.h             #     IRHIDevice (01-20〜01-24)
│   │   ├── IRHIQueue.h              #     IRHIQueueインターフェース (01-25, 01-26)
│   │   ├── IRHICommandContextBase.h #     コマンドコンテキスト基底 (02-01)
│   │   ├── IRHIComputeContext.h     #     コンピュートコンテキスト (02-02)
│   │   ├── IRHICommandContext.h     #     グラフィックスコンテキスト (02-03)
│   │   ├── IRHICommandAllocator.h   #     コマンドアロケーター (02-04)
│   │   ├── IRHICommandList.h        #     コマンドリスト (02-05)
│   │   ├── IRHIWorkGraph.h          #     ワークグラフ (02-06)
│   │   ├── RHIWorkGraphTypes.h     #     ワークグラフ型定義 (02-06)
│   │   ├── IRHIBuffer.h             #     バッファインターフェース (03-01, 03-02, 03-03)
│   │   ├── RHIStructuredBuffer.h    #     構造化バッファ (03-04)
│   │   ├── IRHITexture.h            #     テクスチャインターフェース (04-01〜04-04)
│   │   ├── IRHIViews.h              #     SRV/UAV/RTV/DSV/CBV (05-01〜05-05)
│   │   ├── RHIGPUProfiler.h         #     GPUプロファイラ (05-06)
│   │   ├── IRHIShader.h             #     シェーダーインターフェース (06-01, 06-02)
│   │   ├── RHIShaderReflection.h    #     シェーダーリフレクション (06-03)
│   │   ├── IRHIShaderLibrary.h      #     シェーダーライブラリ (06-04)
│   │   ├── RHIPipelineState.h       #     ブレンド/ラスタライザー/DS/入力レイアウト (07-01〜07-04)
│   │   ├── IRHIPipelineState.h      #     Graphics/Compute PSO (07-05, 07-06)
│   │   ├── RHIVariableRateShading.h #     Variable Rate Shading (07-07)
│   │   ├── IRHIRootSignature.h      #     ルートシグネチャ (08-01〜08-03)
│   │   ├── RHIBindingLayout.h       #     バインディングレイアウト (08-04)
│   │   ├── IRHIFence.h              #     フェンスインターフェース (09-01)
│   │   ├── RHISyncPoint.h           #     同期ポイント (09-02)
│   │   ├── RHIGPUEvent.h            #     GPUイベント (09-03)
│   │   ├── IRHISwapChain.h          #     スワップチェーン (12-01〜12-03)
│   │   ├── RHIHDR.h                 #     HDRサポート (12-04)
│   │   ├── RHIRenderPass.h          #     レンダーパス (13-01〜13-03)
│   │   ├── RHIQuery.h               #     クエリタイプ・プール (14-01, 14-02)
│   │   ├── RHITimestamp.h           #     タイムスタンプ (14-03)
│   │   ├── RHIOcclusion.h           #     オクルージョンクエリ (14-04)
│   │   ├── RHIResourceState.h       #     リソース状態 (16-01)
│   │   ├── RHIBarrier.h             #     バリア (16-02)
│   │   ├── RHIStateTracking.h       #     状態追跡 (16-03)
│   │   ├── RHICheck.h               #     チェックマクロ (17-01)
│   │   ├── RHIDeviceLost.h          #     デバイスロスト (17-02)
│   │   ├── RHIValidation.h          #     検証レイヤー (17-03)
│   │   ├── RHIBreadcrumbs.h         #     GPU Breadcrumbs (17-04)
│   │   ├── IRHISampler.h            #     サンプラー (18-01, 18-02)
│   │   ├── RHIStagingBuffer.h       #     ステージングバッファ (20-01)
│   │   ├── RHIGPUReadback.h         #     GPUリードバック (20-02)
│   │   ├── RHIAsyncReadback.h       #     非同期リードバック (20-03)
│   │   ├── RHITextureReadback.h     #     テクスチャリードバック (20-04)
│   │   ├── RHIIndirectArguments.h   #     Indirect引数構造体 (21-01, 21-02, 21-03)
│   │   ├── RHICommandSignature.h    #     コマンドシグネチャ (21-04, 21-05)
│   │   ├── RHIMeshShader.h          #     メッシュシェーダー型定義 (22-01)
│   │   ├── IRHIMeshShader.h         #     メッシュシェーダーインターフェース (22-01)
│   │   ├── RHIAmplificationShader.h #     増幅シェーダー (22-02)
│   │   ├── RHIMeshPipelineState.h   #     メッシュPSO (22-03)
│   │   ├── RHIMeshDispatch.h        #     メッシュディスパッチ (22-04)
│   │   ├── RHIUniformBufferLayout.h #     UniformBufferレイアウト (24-01)
│   │   ├── RHIShaderParameterMap.h  #     シェーダーパラメータマップ (24-02)
│   │   ├── RHIResourceTable.h       #     リソーステーブル (24-03)
│   │   ├── RHIBoundShaderState.h    #     バウンドシェーダーステート (24-04)
│   │   ├── IRHITransientResourceAllocator.h # Transientアロケーターインターフェース (23-01)
│   │   ├── RHICommandListStats.h    #     コマンドリスト統計 (25-01)
│   │   ├── RHIMemoryStats.h         #     メモリ統計 (25-02)
│   │   └── RHIPSOCacheStats.h       #     PSOキャッシュ統計 (25-03)
│   ├── Internal/                    #   Engine内部共有型
│   │   └── RHISubmission.h          #     サブミッションパイプライン (01-17b)
│   └── Private/                     #   実装
│       ├── RHI.cpp
│       ├── DynamicRHI.cpp
│       ├── RHICommandList.cpp
│       ├── RHIGPUProfiler.cpp       #     (05-06)
│       ├── RHIValidation.cpp        #     (17-03)
│       ├── RHIBreadcrumbs.cpp       #     (17-04)
│       ├── RHISubmissionThread.h    #     サブミッション/インタラプトスレッド (01-17b)
│       ├── RHISubmissionThread.cpp  #     (01-17b)
│       ├── RHIObjectPool.h          #     オブジェクトプール (01-17b)
│       ├── RHIObjectPool.cpp        #     (01-17b)
│       └── Windows/                 #     プラットフォーム別DynamicRHI選択
│           └── WindowsDynamicRHI.cpp
│
├── RHICore/                         # メモリ管理・ディスクリプタ基盤
│   ├── Public/
│   │   ├── RHIDescriptorHeap.h      #     ディスクリプタヒープ (10-01)
│   │   ├── RHIOnlineDescriptor.h    #     オンラインディスクリプタ (10-02)
│   │   ├── RHIOfflineDescriptor.h   #     オフラインディスクリプタ (10-03)
│   │   ├── RHIBindless.h            #     Bindlessサポート (10-04)
│   │   ├── RHIMemoryTypes.h         #     ヒープタイプ (11-01)
│   │   ├── RHIBufferAllocator.h     #     バッファアロケーター (11-02)
│   │   ├── RHITextureAllocator.h    #     テクスチャアロケーター (11-03)
│   │   ├── RHIUploadHeap.h          #     アップロードヒープ (11-04)
│   │   ├── RHIResidency.h           #     メモリ常駐管理 (11-05)
│   │   ├── RHIReservedResource.h    #     Reserved/Sparseリソース (11-06)
│   │   ├── RHIPlatformWorkarounds.h #     プラットフォーム回避策 (11-07)
│   │   ├── RHITransientAllocator.h  #     Transientアロケーター (23-01)
│   │   ├── RHITransientBuffer.h     #     Transientバッファ (23-02)
│   │   ├── RHITransientTexture.h    #     Transientテクスチャ (23-03)
│   │   └── RHIAliasingBarrier.h     #     エイリアシングバリア (23-04)
│   ├── Internal/
│   └── Private/
│
├── D3D12RHI/                        # D3D12バックエンド
│   ├── Public/                      #   薄い公開API
│   │   ├── D3D12RHI.h
│   │   ├── D3D12Resources.h
│   │   └── D3D12State.h
│   ├── Internal/
│   └── Private/                     #   実装本体
│       ├── D3D12Adapter.cpp/.h
│       ├── D3D12Device.cpp/.h
│       ├── D3D12CommandContext.cpp/.h
│       ├── D3D12Buffer.cpp
│       ├── D3D12Texture.cpp/.h
│       ├── D3D12RootSignature.cpp/.h
│       ├── D3D12PipelineState.cpp/.h
│       ├── D3D12DescriptorCache.cpp/.h
│       ├── D3D12Query.cpp/.h
│       ├── D3D12Residency.h
│       └── D3D12RHIPrivate.h
│
└── NullRHI/                         # Nullバックエンド（テスト用）
    ├── Public/
    └── Private/
```

## premake5.lua統合

UE5同様、モジュールごとにproject定義。1モジュール = 1 StaticLib。

```lua
-- RHI: 抽象インターフェース
project "RHI"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    files {
        "Source/Engine/RHI/Public/**.h",
        "Source/Engine/RHI/Internal/**.h",
        "Source/Engine/RHI/Private/**.h",
        "Source/Engine/RHI/Private/**.cpp",
    }
    includedirs {
        "Source/Engine/RHI/Public",
        "Source/Engine/RHI/Internal",
        "Source/Engine/RHI/Private",
        "Source/Common",
    }
    links { "Common" }

-- RHICore: メモリ管理・ディスクリプタ基盤
project "RHICore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    files {
        "Source/Engine/RHICore/Public/**.h",
        "Source/Engine/RHICore/Internal/**.h",
        "Source/Engine/RHICore/Private/**.h",
        "Source/Engine/RHICore/Private/**.cpp",
    }
    includedirs {
        "Source/Engine/RHICore/Public",
        "Source/Engine/RHICore/Internal",
        "Source/Engine/RHICore/Private",
        "Source/Engine/RHI/Public",
        "Source/Common",
    }
    links { "RHI", "Common" }

-- D3D12RHI: D3D12バックエンド
project "D3D12RHI"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    files {
        "Source/Engine/D3D12RHI/Public/**.h",
        "Source/Engine/D3D12RHI/Private/**.h",
        "Source/Engine/D3D12RHI/Private/**.cpp",
    }
    includedirs {
        "Source/Engine/D3D12RHI/Public",
        "Source/Engine/D3D12RHI/Private",
        "Source/Engine/RHI/Public",
        "Source/Engine/RHICore/Public",
        "Source/Common",
    }
    links { "RHI", "RHICore", "Common" }
```

**インクルードパス方針:**
- Gameコード → `RHI/Public/` のみ: `#include "RHI/DynamicRHI.h"`
- Engine他モジュール → `+ Internal/`: `#include "RHI/RHISubmission.h"`
- D3D12RHI内 → `+ RHI/Public/ + RHICore/Public/`: `#include "D3D12RHIPrivate.h"`

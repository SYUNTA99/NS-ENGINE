# 02-01: コマンドコンテキスト基底

## 目的

コマンドコンテキストの基底インターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (1. コマンドコンテキスト

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHICommandContextBase.h`

## TODO

### 1. IRHICommandContextBase: 基本定義

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// コマンドコンテキスト基底
    /// 全てのコマンドコンテキストに共通するインターフェース
    class RHI_API IRHICommandContextBase
    {
    public:
        virtual ~IRHICommandContextBase() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// GPUマスク取得
        virtual GPUMask GetGPUMask() const = 0;

        /// キュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;

        /// パイプラインタイプ取得
        virtual ERHIPipeline GetPipeline() const = 0;
    };
}
```

- [ ] 基本インターフェース定義

### 2. ライフサイクル管理

```cpp
namespace NS::RHI
{
    class IRHICommandContextBase
    {
    public:
        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// コンテキスト開始
        /// @param allocator 使用するコマンドアロケーター
        virtual void Begin(IRHICommandAllocator* allocator) = 0;

        /// コンテキスト終了
        /// @return 記録されたコマンドリスト
        virtual IRHICommandList* Finish() = 0;

        /// コンテキストリセット
        virtual void Reset() = 0;

        /// コマンド記録中か
        virtual bool IsRecording() const = 0;
    };
}
```

- [ ] Begin / Finish / Reset
- [ ] IsRecording

### 3. リソースバリア

```cpp
namespace NS::RHI
{
    class IRHICommandContextBase
    {
    public:
        //=====================================================================
        // リソースバリア
        //=====================================================================

        /// 単一リソースの状態遷移
        /// @param resource 対象リソース
        /// @param stateBefore 遷移前の状態
        /// @param stateAfter 遷移後の状態
        virtual void TransitionResource(
            IRHIResource* resource,
            ERHIAccess stateBefore,
            ERHIAccess stateAfter) = 0;

        /// UAVバリア
        /// @param resource 対象リソース（nullptrで全UAV）。
        virtual void UAVBarrier(IRHIResource* resource = nullptr) = 0;

        /// エイリアシングバリア
        /// @param resourceBefore 使用終了リソース
        /// @param resourceAfter 使用開始リソース
        virtual void AliasingBarrier(
            IRHIResource* resourceBefore,
            IRHIResource* resourceAfter) = 0;

        /// バリアをフラッシュ（遅延バリアの場合）
        virtual void FlushBarriers() = 0;
    };
}
```

- [ ] TransitionResource
- [ ] UAVBarrier / AliasingBarrier
- [ ] FlushBarriers

### 4. コピー操作

```cpp
namespace NS::RHI
{
    class IRHICommandContextBase
    {
    public:
        //=====================================================================
        // バッファコピー
        //=====================================================================

        /// バッファ全体コピー
        virtual void CopyBuffer(
            IRHIBuffer* dst,
            IRHIBuffer* src) = 0;

        /// バッファ部分コピー
        virtual void CopyBufferRegion(
            IRHIBuffer* dst, uint64 dstOffset,
            IRHIBuffer* src, uint64 srcOffset,
            uint64 size) = 0;

        //=====================================================================
        // テクスチャコピー
        //=====================================================================

        /// テクスチャ全体コピー
        virtual void CopyTexture(
            IRHITexture* dst,
            IRHITexture* src) = 0;

        /// テクスチャ部分コピー
        virtual void CopyTextureRegion(
            IRHITexture* dst, uint32 dstMip, uint32 dstSlice,
            Offset3D dstOffset,
            IRHITexture* src, uint32 srcMip, uint32 srcSlice,
            const RHIBox* srcBox = nullptr) = 0;

        //=====================================================================
        // バッファ ↔ テクスチャ
        //=====================================================================

        /// バッファからテクスチャへコピー
        virtual void CopyBufferToTexture(
            IRHITexture* dst, uint32 dstMip, uint32 dstSlice,
            Offset3D dstOffset,
            IRHIBuffer* src, uint64 srcOffset,
            uint32 srcRowPitch, uint32 srcDepthPitch) = 0;

        /// テクスチャからバッファへコピー
        virtual void CopyTextureToBuffer(
            IRHIBuffer* dst, uint64 dstOffset,
            uint32 dstRowPitch, uint32 dstDepthPitch,
            IRHITexture* src, uint32 srcMip, uint32 srcSlice,
            const RHIBox* srcBox = nullptr) = 0;
    };
}
```

- [ ] バッファコピー
- [ ] テクスチャコピー
- [ ] バッファ ↔ テクスチャ

### 5. デバッグ機能

```cpp
namespace NS::RHI
{
    class IRHICommandContextBase
    {
    public:
        //=====================================================================
        // デバッグ
        //=====================================================================

        /// デバッグイベント開始
        virtual void BeginDebugEvent(const char* name, uint32 color = 0) = 0;

        /// デバッグイベント終了
        virtual void EndDebugEvent() = 0;

        /// デバッグマーカー挿入
        virtual void InsertDebugMarker(const char* name, uint32 color = 0) = 0;
    };

    /// デバッグイベントスコープ（RAII）。
    struct RHIDebugEventScope
    {
        IRHICommandContextBase* context;

        RHIDebugEventScope(IRHICommandContextBase* ctx, const char* name,
                           uint32 color = 0)
            : context(ctx)
        {
            if (context) context->BeginDebugEvent(name, color);
        }

        ~RHIDebugEventScope()
        {
            if (context) context->EndDebugEvent();
        }

        NS_DISALLOW_COPY(RHIDebugEventScope);
    };

    #define RHI_DEBUG_EVENT(ctx, name) \
        ::NS::RHI::RHIDebugEventScope NS_MACRO_CONCATENATE(_rhiDebugEvent, __LINE__)(ctx, name)
}
```

- [ ] デバッグイベント
- [ ] RAIIスコープ

## 設計決定: 仮想関数とビルド時最適化

仮想関数ベースのインターフェースを使用する。理由:
1. 初期実装の明快さを優先
2. バックエンドが1つ（D3D12）の段階では仮想関数のオーバーヘッドは軽微
3. シッピングビルドではビルド時バックエンド選択で静的ディスパッチに切替可能

### 最適化パス（将来）
- Phase 1: 仮想関数のまま実装・動作確認
- Phase 2: プロファイリングでボトルネック特定
- Phase 3: ビルド時バックエンド選択で静的ディスパッチに切替、またはホットパスのコマンドバッファ構造体化

### Bypassパターン（即時実行）

通常のコンテキストは遅延実行（記録→後で一括実行）だが、
初期化やリソースロード等で即時GPU実行が必要な場合にBypassモードを使用する。

```cpp
namespace NS::RHI
{
    /// 即時実行コンテキスト
    /// 通常コンテキストと同一インターフェースだが、コマンドを即座にGPUに発行する。
    /// Begin/Finishのペアではなく、各コマンドが即時サブミットされる。
    ///
    /// 使用制約:
    /// - RHIスレッドからのみ使用可能
    /// - 並列記録不可（単一インスタンスのみ）
    /// - パフォーマンスコストが高い（毎コマンドでフラッシュ可能なため）
    /// - 通常の遅延実行パスで代替可能な場合は使用しない
    ///
    /// 使用例:
    /// - デバイス初期化時のリソースセットアップ
    /// - デバッグ用の即時描画
    /// - GPU readbackの即時完了待ち
    class IRHIImmediateContext : public IRHICommandContextBase
    {
    public:
        /// 即時フラッシュ
        /// 記録済みコマンドをGPUに送信し、完了を待つ
        virtual void Flush() = 0;

        /// ネイティブコンテキスト取得（プラットフォーム固有操作用）
        virtual void* GetNativeContext() = 0;
    };
}
```

- [ ] IRHIImmediateContext インターフェース
- [ ] Flush

## 検証方法

- [ ] インターフェース定義の完全性
- [ ] コピー操作の整合性
- [ ] Bypassコンテキストの即時実行動作

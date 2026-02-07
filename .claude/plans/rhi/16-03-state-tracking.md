# 16-03: 状態追跡

## 目的

自動リソース状態追跡機能を定義する。

## 参照ドキュメント

- 16-01-resource-state.md (ERHIResourceState)
- 16-02-barrier.md (RHIBarrierBatch)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIStateTracking.h`

## TODO

### 1. リソース状態トラッカー

```cpp
#pragma once

#include "RHIBarrier.h"

namespace NS::RHI
{
    /// リソース状態トラッカー
    /// コマンドリスト単位でリソース状態を追跡
    class RHI_API RHIResourceStateTracker
    {
    public:
        RHIResourceStateTracker() = default;

        /// 初期化
        bool Initialize(uint32 maxTrackedResources = 4096);

        /// シャットダウン
        void Shutdown();

        /// リセット（コマンドリスト開始時）。
        void Reset();

        //=====================================================================
        // 状態追跡
        //=====================================================================

        /// リソースを追跡開始
        void TrackResource(IRHIResource* resource, ERHIResourceState initialState);

        /// リソース追跡解除
        void UntrackResource(IRHIResource* resource);

        /// 現在の状態取得
        ERHIResourceState GetCurrentState(IRHIResource* resource) const;

        /// サブリソースの現在状態取得
        ERHIResourceState GetSubresourceState(
            IRHIResource* resource,
            uint32 subresource) const;

        //=====================================================================
        // 遷移要求
        //=====================================================================

        /// 遷移要求（必要なら自動でバリア生成）。
        void RequireState(
            IRHIResource* resource,
            ERHIResourceState requiredState,
            uint32 subresource = kAllSubresources);

        /// 遷移バリア取得
        /// @param outBarriers 出力バリア配列
        /// @param maxBarriers 最大バリア数
        /// @return 生成されたバリア数
        uint32 GetPendingBarriers(
            RHITransitionBarrier* outBarriers,
            uint32 maxBarriers);

        /// 保留バリアをクリア
        void ClearPendingBarriers();

        /// 保留バリア数
        uint32 GetPendingBarrierCount() const { return m_pendingBarrierCount; }

        //=====================================================================
        // 情報
        //=====================================================================

        /// 追跡中リソース数
        uint32 GetTrackedResourceCount() const { return m_trackedCount; }

    private:
        struct TrackedResource {
            IRHIResource* resource;
            RHIResourceStateMap stateMap;
        };
        TrackedResource* m_trackedResources = nullptr;
        uint32 m_trackedCount = 0;
        uint32 m_trackedCapacity = 0;

        RHITransitionBarrier* m_pendingBarriers = nullptr;
        uint32 m_pendingBarrierCount = 0;
        uint32 m_pendingBarrierCapacity = 0;

        /// リソース検索
        TrackedResource* FindTrackedResource(IRHIResource* resource);
        const TrackedResource* FindTrackedResource(IRHIResource* resource) const;
    };
}
```

- [ ] RHIResourceStateTracker クラス

### 2. グローバル状態のマネージャー

```cpp
namespace NS::RHI
{
    /// グローバルリソース状態のマネージャー
    /// デバイス全体でのリソース状態を管理
    class RHI_API RHIGlobalResourceStateManager
    {
    public:
        RHIGlobalResourceStateManager() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // リソース登録
        //=====================================================================

        /// リソース登録
        void RegisterResource(
            IRHIResource* resource,
            ERHIResourceState initialState,
            uint32 subresourceCount = 1);

        /// リソース登録解除
        void UnregisterResource(IRHIResource* resource);

        //=====================================================================
        // 状態クエリ
        //=====================================================================

        /// グローバル状態取得
        ERHIResourceState GetGlobalState(IRHIResource* resource) const;

        /// サブリソースのグローバル状態取得
        ERHIResourceState GetSubresourceGlobalState(
            IRHIResource* resource,
            uint32 subresource) const;

        //=====================================================================
        // コマンドリスト連携
        //=====================================================================

        /// コマンドリスト実行前のバリア生。
        /// @param localTracker コマンドリストのローカルトラッカー
        /// @param outBarriers 出力バリア配列
        /// @param maxBarriers 最大バリア数
        /// @return 生成されたバリア数
        uint32 ResolveBarriers(
            const RHIResourceStateTracker& localTracker,
            RHITransitionBarrier* outBarriers,
            uint32 maxBarriers);

        /// コマンドリスト実行後の状態更新
        void CommitLocalStates(const RHIResourceStateTracker& localTracker);

    private:
        IRHIDevice* m_device = nullptr;

        struct GlobalResourceState {
            IRHIResource* resource;
            RHIResourceStateMap stateMap;
        };
        // Hash map: resource -> GlobalResourceState

        mutable void* m_mutex = nullptr;  // スレッドセーフ用
    };
}
```

- [ ] RHIGlobalResourceStateManager クラス

### 3. 自動バリアコンテキスト

```cpp
namespace NS::RHI
{
    /// 自動バリアコンテキスト
    /// 状態追跡付きコマンドコンテキストラッカー
    class RHI_API RHIAutoBarrierContext
    {
    public:
        RHIAutoBarrierContext() = default;

        /// 初期化
        void Initialize(
            IRHICommandContext* context,
            RHIGlobalResourceStateManager* globalManager);

        /// 終了時（未発行バリアをフラッシュ）。
        void Finalize();

        //=====================================================================
        // バリア自動挿入付きリソースアクセス
        //=====================================================================

        /// テクスチャをシェーダーリソースとして使用
        void UseAsShaderResource(IRHITexture* texture, uint32 subresource = kAllSubresources);

        /// テクスチャをレンダーターゲットとして使用
        void UseAsRenderTarget(IRHITexture* texture, uint32 subresource = kAllSubresources);

        /// テクスチャをデプスステンシルとして使用
        void UseAsDepthStencil(IRHITexture* texture, bool write = true);

        /// テクスチャをUAVとして使用
        void UseAsUAV(IRHITexture* texture, uint32 subresource = kAllSubresources);

        /// テクスチャをコピー先として使用
        void UseAsCopyDest(IRHITexture* texture);

        /// テクスチャをコピー元として使用
        void UseAsCopySource(IRHITexture* texture);

        /// バッファを頂点バッファとして使用
        void UseAsVertexBuffer(IRHIBuffer* buffer);

        /// バッファをインデックスバッファとして使用
        void UseAsIndexBuffer(IRHIBuffer* buffer);

        /// バッファを定数バッファとして使用
        void UseAsConstantBuffer(IRHIBuffer* buffer);

        /// バッファをUAVとして使用
        void UseAsUAV(IRHIBuffer* buffer);

        //=====================================================================
        // 明示的バリア
        //=====================================================================

        /// バリアをフラッシュ
        void FlushBarriers();

        /// UAVバリア挿入
        void UAVBarrier(IRHIResource* resource = nullptr);

        //=====================================================================
        // コンテキストアクセス
        //=====================================================================

        /// 基底コンテキスト取得
        IRHICommandContext* GetContext() const { return m_context; }

        /// ローカルトラップー取得
        RHIResourceStateTracker& GetLocalTracker() { return m_localTracker; }

    private:
        IRHICommandContext* m_context = nullptr;
        RHIGlobalResourceStateManager* m_globalManager = nullptr;
        RHIResourceStateTracker m_localTracker;
        RHIBarrierBatch m_pendingBarriers;
    };
}
```

- [ ] RHIAutoBarrierContext クラス

### 4. 状態検証

```cpp
namespace NS::RHI
{
    /// 状態検証（デバッグ用）。
    class RHI_API RHIResourceStateValidator
    {
    public:
        RHIResourceStateValidator() = default;

        /// 検証有効化
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

        //=====================================================================
        // 検証
        //=====================================================================

        /// リソースアクセスの検証
        /// @return エラーがあれのfalse
        bool ValidateAccess(
            IRHIResource* resource,
            ERHIResourceState requiredState,
            ERHIResourceState actualState,
            uint32 subresource = kAllSubresources);

        /// 遷移の検証
        bool ValidateTransition(
            IRHIResource* resource,
            ERHIResourceState before,
            ERHIResourceState after,
            uint32 subresource = kAllSubresources);

        //=====================================================================
        // エラー報告
        //=====================================================================

        /// 最後のエラー取得
        const char* GetLastError() const { return m_lastError; }

        /// エラークリア
        void ClearError() { m_lastError[0] = '\0'; }

    private:
        bool m_enabled = false;
        char m_lastError[512] = {};
    };
}
```

- [ ] RHIResourceStateValidator クラス

## 検証方法

- [ ] ローカル状態追跡
- [ ] グローバル状態管理
- [ ] 自動バリア挿入
- [ ] 状態検証

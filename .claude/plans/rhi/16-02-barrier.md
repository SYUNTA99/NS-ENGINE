# 16-02: バリア

## 目的

リソースバリアの定義と発行機能を定義する。

## 参照ドキュメント

- 16-01-resource-state.md (ERHIResourceState)
- 02-01-command-context-base.md (IRHICommandContext)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIBarrier.h`

## TODO

### 1. バリアタイプ

```cpp
#pragma once

#include "RHIResourceState.h"

namespace NS::RHI
{
    /// バリアタイプ
    enum class ERHIBarrierType : uint8
    {
        /// 遷移バリア
        /// リソース状態を変更
        Transition,

        /// エイリアシングバリア
        /// 同一メモリを使う異なるリソース間
        Aliasing,

        /// UAVバリア
        /// 同一UAVへの読み書き同期
        UAV,
    };

    /// バリアフラグ
    enum class ERHIBarrierFlags : uint32
    {
        None = 0,

        /// スプリットバリア開始
        BeginOnly = 1 << 0,

        /// スプリットバリア終了
        EndOnly = 1 << 1,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIBarrierFlags)
}
```

- [ ] ERHIBarrierType 列挙型
- [ ] ERHIBarrierFlags 列挙型

### 2. 遷移バリア記述

```cpp
namespace NS::RHI
{
    /// 遷移バリア記述
    struct RHI_API RHITransitionBarrier
    {
        /// リソース
        IRHIResource* resource = nullptr;

        /// サブリソースインデックス（AllSubresourcesで全て）。
        uint32 subresource = kAllSubresources;

        /// 遷移前の状態
        ERHIResourceState stateBefore = ERHIResourceState::Common;

        /// 遷移後の状態
        ERHIResourceState stateAfter = ERHIResourceState::Common;

        /// フラグ
        ERHIBarrierFlags flags = ERHIBarrierFlags::None;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// 遷移バリア作成
        static RHITransitionBarrier Create(
            IRHIResource* res,
            ERHIResourceState before,
            ERHIResourceState after,
            uint32 sub = kAllSubresources)
        {
            RHITransitionBarrier barrier;
            barrier.resource = res;
            barrier.subresource = sub;
            barrier.stateBefore = before;
            barrier.stateAfter = after;
            return barrier;
        }

        /// バッファ用
        static RHITransitionBarrier Buffer(
            IRHIBuffer* buffer,
            ERHIResourceState before,
            ERHIResourceState after)
        {
            return Create(buffer, before, after);
        }

        /// テクスチャ用
        static RHITransitionBarrier Texture(
            IRHITexture* texture,
            ERHIResourceState before,
            ERHIResourceState after,
            uint32 subresource = kAllSubresources)
        {
            return Create(texture, before, after, subresource);
        }
    };
}
```

- [ ] RHITransitionBarrier 構造体

### 3. UAVバリア記述

```cpp
namespace NS::RHI
{
    /// UAVバリア記述
    struct RHI_API RHIUAVBarrier
    {
        /// リソース（nullで全UAV）。
        IRHIResource* resource = nullptr;

        /// 全UAVバリア
        static RHIUAVBarrier All() {
            return RHIUAVBarrier{};
        }

        /// 特定リソースのUAVバリア
        static RHIUAVBarrier ForResource(IRHIResource* res) {
            RHIUAVBarrier barrier;
            barrier.resource = res;
            return barrier;
        }
    };

    /// エイリアシングバリア記述
    struct RHI_API RHIAliasingBarrier
    {
        /// 前リソース（nullで未使用）。
        IRHIResource* resourceBefore = nullptr;

        /// 後リソース（nullで未使用）。
        IRHIResource* resourceAfter = nullptr;

        /// 作成
        static RHIAliasingBarrier Create(
            IRHIResource* before,
            IRHIResource* after)
        {
            RHIAliasingBarrier barrier;
            barrier.resourceBefore = before;
            barrier.resourceAfter = after;
            return barrier;
        }
    };
}
```

- [ ] RHIUAVBarrier 構造体
- [ ] RHIAliasingBarrier 構造体

### 4. バリア発行

```cpp
namespace NS::RHI
{
    /// バリア発行（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        /// 遷移バリア発行
        virtual void TransitionBarrier(
            IRHIResource* resource,
            ERHIResourceState stateBefore,
            ERHIResourceState stateAfter,
            uint32 subresource = kAllSubresources) = 0;

        /// 複数遷移バリア発行
        virtual void TransitionBarriers(
            const RHITransitionBarrier* barriers,
            uint32 count) = 0;

        /// UAVバリア発行
        virtual void UAVBarrier(IRHIResource* resource = nullptr) = 0;

        /// 複数UAVバリア発行
        virtual void UAVBarriers(
            const RHIUAVBarrier* barriers,
            uint32 count) = 0;

        /// エイリアシングバリア発行
        virtual void AliasingBarrier(
            IRHIResource* resourceBefore,
            IRHIResource* resourceAfter) = 0;

        /// 複数エイリアシングバリア発行
        virtual void AliasingBarriers(
            const RHIAliasingBarrier* barriers,
            uint32 count) = 0;

        /// バリアフラッシュ
        /// 蓄積されたバリアを即座に発行
        virtual void FlushBarriers() = 0;
    };
}
```

- [ ] TransitionBarrier / TransitionBarriers
- [ ] UAVBarrier / UAVBarriers
- [ ] AliasingBarrier / AliasingBarriers

### 5. バリアバッチング

```cpp
namespace NS::RHI
{
    /// バリアバッチ
    /// 複数のバリアをまとめて発行
    class RHI_API RHIBarrierBatch
    {
    public:
        /// 最大バリア数（コンテキスト付きの場合は自動フラッシュで実質無制限）
        /// コンテキスト未設定の場合のスタック容量上限
        static constexpr uint32 kMaxBarriers = 64;

        /// @param context 自動フラッシュ用コマンドコンテキスト（nullptrで自動フラッシュ無効）
        /// @note コンテキスト未設定時に容量超過するとassert。
        ///       バッチングを使わない直接発行パスではcontext=nullptrも許容する。
        explicit RHIBarrierBatch(IRHICommandContext* context = nullptr)
            : m_context(context) {}

        //=====================================================================
        // バリア追加
        //=====================================================================

        /// 遷移バリア追加
        /// @note 容量超過時、m_contextが設定されていれば自動フラッシュ
        RHIBarrierBatch& AddTransition(
            IRHIResource* resource,
            ERHIResourceState before,
            ERHIResourceState after,
            uint32 subresource = kAllSubresources);

        /// 遷移バリア追加（記述から）。
        /// @note 容量超過時、m_contextが設定されていれば自動フラッシュ
        RHIBarrierBatch& AddTransition(const RHITransitionBarrier& barrier);

        /// UAVバリア追加
        /// @note 容量超過時、m_contextが設定されていれば自動フラッシュ
        RHIBarrierBatch& AddUAV(IRHIResource* resource = nullptr);

        /// エイリアシングバリア追加
        /// @note 容量超過時、m_contextが設定されていれば自動フラッシュ
        RHIBarrierBatch& AddAliasing(
            IRHIResource* before,
            IRHIResource* after);

        //=====================================================================
        // 発行
        //=====================================================================

        /// バリア発行
        void Submit(IRHICommandContext* context);

        /// クリア
        void Clear();

        //=====================================================================
        // 情報
        //=====================================================================

        /// バリア数
        uint32 GetTransitionCount() const { return m_transitionCount; }
        uint32 GetUAVCount() const { return m_uavCount; }
        uint32 GetAliasingCount() const { return m_aliasingCount; }

        /// 空か
        bool IsEmpty() const {
            return m_transitionCount == 0 &&
                   m_uavCount == 0 &&
                   m_aliasingCount == 0;
        }

    private:
        /// 自動フラッシュ用コマンドコンテキスト
        IRHICommandContext* m_context = nullptr;

        RHITransitionBarrier m_transitions[kMaxBarriers];
        uint32 m_transitionCount = 0;

        RHIUAVBarrier m_uavs[kMaxBarriers];
        uint32 m_uavCount = 0;

        RHIAliasingBarrier m_aliasings[kMaxBarriers];
        uint32 m_aliasingCount = 0;

        // --- 自動フラッシュ実装メモ ---
        // 各Add*メソッド内で容量チェック:
        //   if (m_*Count >= kMaxBarriers) {
        //       if (m_context) {
        //           Submit(m_context);
        //           Clear();
        //       } else {
        //           NS_LOG_WARNING("[RHI] Barrier batch overflow without context (count=%u)", m_*Count);
        //           // リリースビルドでは超過を無視（最初のkMaxBarriersのみ発行）
        //           RHI_ASSERT(false, "Barrier batch overflow without context for auto-flush");
        //           return *this;
        //       }
        //   }
    };
}
```

- [ ] RHIBarrierBatch クラス

### 6. スプリットバリア

```cpp
namespace NS::RHI
{
    /// スプリットバリアヘルパー（単一リソース用）
    /// 長時間の遷移を分割して他の処理を並列化
    /// 複数リソースに同時にsplit barrierを発行する場合は
    /// RHISplitBarrierBatchを使用する
    class RHI_API RHISplitBarrier
    {
    public:
        RHISplitBarrier() = default;

        /// 開始
        void Begin(
            IRHICommandContext* context,
            IRHIResource* resource,
            ERHIResourceState before,
            ERHIResourceState after,
            uint32 subresource = kAllSubresources);

        /// 終了
        void End(IRHICommandContext* context);

        /// 有効か
        bool IsActive() const { return m_resource != nullptr; }

    private:
        IRHIResource* m_resource = nullptr;
        ERHIResourceState m_stateBefore = ERHIResourceState::Common;
        ERHIResourceState m_stateAfter = ERHIResourceState::Common;
        uint32 m_subresource = kAllSubresources;
    };

    /// スプリットバリアバッチ（複数リソース同時対応）
    /// 複数リソースに対するBegin/Endを一括管理
    class RHI_API RHISplitBarrierBatch
    {
    public:
        /// 同時に管理可能なスプリットバリア数
        /// 超過時はassert（設計レビューで上限を引き上げるか検討）
        static constexpr uint32 kMaxSplitBarriers = 16;

        /// 開始を追加
        /// @note split barrierはBegin/End間に他の処理を挟む前提のため自動フラッシュ不可。
        ///       kMaxSplitBarriersを超えた場合はassertで即座に検出する。
        void BeginBarrier(
            IRHICommandContext* context,
            IRHIResource* resource,
            ERHIResourceState before,
            ERHIResourceState after,
            uint32 subresource = kAllSubresources);

        /// 全て終了
        void EndAll(IRHICommandContext* context);

        /// アクティブ数
        uint32 GetActiveCount() const { return m_count; }

    private:
        RHISplitBarrier m_barriers[kMaxSplitBarriers];
        uint32 m_count = 0;
    };
}
```

- [ ] RHISplitBarrier クラス
- [ ] RHISplitBarrierBatch クラス

### 7. Enhanced Barriers（Phase 2対応）

D3D12 Agility SDK 1.7+のEnhanced Barriersモデルへの対応準備。

> **実装スケジュール**: レガシーバリア（Section 1-6）の実装完了後、D3D12 Agility SDK 1.7+対応として実装する。

```cpp
namespace NS::RHI
{
    /// Enhanced Barrier記述（D3D12_BARRIER_GROUP相当）
    struct RHI_API RHIEnhancedBarrierDesc
    {
        /// 同期スコープ（前）
        ERHIBarrierSync syncBefore = ERHIBarrierSync::None;

        /// 同期スコープ（後）
        ERHIBarrierSync syncAfter = ERHIBarrierSync::None;

        /// アクセススコープ（前）
        ERHIBarrierAccess accessBefore = ERHIBarrierAccess::NoAccess;

        /// アクセススコープ（後）
        ERHIBarrierAccess accessAfter = ERHIBarrierAccess::NoAccess;

        /// レイアウト（前）- テクスチャのみ
        ERHIBarrierLayout layoutBefore = ERHIBarrierLayout::Undefined;

        /// レイアウト（後）- テクスチャのみ
        ERHIBarrierLayout layoutAfter = ERHIBarrierLayout::Undefined;

        /// 対象リソース
        IRHIResource* resource = nullptr;

        /// サブリソース範囲
        RHISubresourceRange subresources;
    };

    /// Enhanced Barrier同期スコープ
    enum class ERHIBarrierSync : uint32
    {
        None = 0,
        All = ~0u,
        Draw = 1 << 0,
        IndexInput = 1 << 1,
        VertexShading = 1 << 2,
        PixelShading = 1 << 3,
        DepthStencil = 1 << 4,
        RenderTarget = 1 << 5,
        Compute = 1 << 6,
        Raytracing = 1 << 7,
        Copy = 1 << 8,
        Resolve = 1 << 9,
        ExecuteIndirect = 1 << 10,
        AllShading = 1 << 12,
        NonPixelShading = 1 << 13,
        BuildRaytracingAccelerationStructure = 1 << 15,
        CopyRaytracingAccelerationStructure = 1 << 16,
        Split = 0x80000000,  // スプリットバリア
    };
    RHI_ENUM_CLASS_FLAGS(ERHIBarrierSync)

    /// Enhanced Barrierアクセススコープ
    enum class ERHIBarrierAccess : uint32
    {
        NoAccess = 0,
        Common = 0,
        VertexBuffer = 1 << 0,
        ConstantBuffer = 1 << 1,
        IndexBuffer = 1 << 2,
        RenderTarget = 1 << 3,
        UnorderedAccess = 1 << 4,
        DepthStencilWrite = 1 << 5,
        DepthStencilRead = 1 << 6,
        ShaderResource = 1 << 7,
        StreamOutput = 1 << 8,
        IndirectArgument = 1 << 9,
        CopyDest = 1 << 11,
        CopySource = 1 << 12,
        ResolveDest = 1 << 13,
        ResolveSource = 1 << 14,
        RaytracingAccelerationStructureRead = 1 << 15,
        RaytracingAccelerationStructureWrite = 1 << 16,
        ShadingRate = 1 << 17,
    };
    RHI_ENUM_CLASS_FLAGS(ERHIBarrierAccess)

    /// テクスチャレイアウト
    enum class ERHIBarrierLayout : uint8
    {
        Undefined,
        Common,
        Present,
        GenericRead,
        RenderTarget,
        UnorderedAccess,
        DepthStencilWrite,
        DepthStencilRead,
        ShaderResource,
        CopySource,
        CopyDest,
        ResolveSource,
        ResolveDest,
        ShadingRate,
        DirectQueueCommon,
        DirectQueueGenericRead,
        DirectQueueUnorderedAccess,
        DirectQueueShaderResource,
        DirectQueueCopySource,
        DirectQueueCopyDest,
        ComputeQueueCommon,
        ComputeQueueGenericRead,
        ComputeQueueUnorderedAccess,
        ComputeQueueShaderResource,
        ComputeQueueCopySource,
        ComputeQueueCopyDest,
    };
}
```

- [ ] ERHIBarrierSync 列挙型
- [ ] ERHIBarrierAccess 列挙型
- [ ] ERHIBarrierLayout 列挙型
- [ ] RHIEnhancedBarrierDesc 構造体
- [ ] 従来バリアからEnhanced Barrierへの変換ヘルパー

## 検証方法

- [ ] バリア発行の動作
- [ ] バッチングの効果
- [ ] スプリットバリアの動作
- [ ] 不要なバリアの検出

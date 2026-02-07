# 11-05: メモリ常駐管理

## 目的

GPUメモリの常駐（Residency）管理）を定義する。

## 参照ドキュメント

- 11-01-heap-types.md (IRHIHeap)
- 01-12-resource-base.md (IRHIResource)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHICore/Public/RHIResidency.h`

## TODO

### 1. 常駐状態

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// 常駐状態
    enum class ERHIResidencyStatus : uint8
    {
        /// 常駐中
        Resident,

        /// 退避中（VRAMにない）
        Evicted,

        /// 常駐待機中
        PendingMakeResident,

        /// 退避待機中
        PendingEvict,
    };

    /// 常駐優先度
    enum class ERHIResidencyPriority : uint8
    {
        /// 最低（退避候補）
        Minimum,

        /// 作
        Low,

        /// 通常
        Normal,

        /// 高（できるだけ常駐維持）。
        High,

        /// 最高（常に常駐）
        Maximum,
    };
}
```

- [ ] ERHIResidencyStatus 列挙型
- [ ] ERHIResidencyPriority 列挙型

### 2. 常駐可能リソースインターフェース

```cpp
namespace NS::RHI
{
    /// 常駐可能リソース
    /// ヒープやコミットリソースなど
    class RHI_API IRHIResidentResource
    {
    public:
        virtual ~IRHIResidentResource() = default;

        /// 常駐状態取得
        virtual ERHIResidencyStatus GetResidencyStatus() const = 0;

        /// 常駐優先度取得
        virtual ERHIResidencyPriority GetResidencyPriority() const = 0;

        /// 常駐優先度設定
        virtual void SetResidencyPriority(ERHIResidencyPriority priority) = 0;

        /// サイズ取得
        virtual uint64 GetSize() const = 0;

        /// 最終使用フレーム取得
        virtual uint64 GetLastUsedFrame() const = 0;

        /// 最終使用情報設定
        virtual void SetLastUsed(uint64 frame, uint64 fenceValue) = 0;

        /// 最終使用フェンス値取得
        virtual uint64 GetLastUsedFenceValue() const = 0;
    };
}
```

- [ ] IRHIResidentResource インターフェース

### 3. 常駐管理コマンド

```cpp
namespace NS::RHI
{
    /// 常駐操作（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// リソースを常駐させる
        /// @param resources リソース配列
        /// @param count リソース数
        /// @return 成功したか
        virtual bool MakeResident(
            IRHIResidentResource* const* resources,
            uint32 count) = 0;

        /// リソースを退避させめ
        /// @param resources リソース配列
        /// @param count リソース数
        /// @return 成功したか
        virtual bool Evict(
            IRHIResidentResource* const* resources,
            uint32 count) = 0;

        /// 単一リソース常駆
        bool MakeResident(IRHIResidentResource* resource) {
            return MakeResident(&resource, 1);
        }

        /// 単一リソース退避
        bool Evict(IRHIResidentResource* resource) {
            return Evict(&resource, 1);
        }
    };
}
```

- [ ] MakeResident
- [ ] Evict

### 4. 常駐のマネージャー

```cpp
namespace NS::RHI
{
    /// 常駐のマネージャー設定
    struct RHI_API RHIResidencyManagerConfig
    {
        /// 最大VRAM使用量（で自動）
        uint64 maxVideoMemoryUsage = 0;

        /// 退避閾値（この使用率を超えると退避開始）
        float evictionThreshold = 0.9f;

        /// 退避目標（退避時にこの使用率まで下げる）
        float evictionTarget = 0.7f;

        /// 未使用フレーム数での自動退避
        uint32 unusedFramesBeforeEvict = 60;

        /// バックグラウンド常駐操作を有効化
        bool enableBackgroundOperations = true;
    };

    /// 常駐のマネージャー
    /// VRAMの自動管理を行う
    class RHI_API RHIResidencyManager
    {
    public:
        RHIResidencyManager() = default;

        /// 初期化
        /// @param device デバイス
        /// @param config 設定
        /// @param fence GPU完了確認用フェンス
        /// @param queue フェンスシグナル用キュー
        bool Initialize(
            IRHIDevice* device,
            const RHIResidencyManagerConfig& config = {},
            IRHIFence* fence = nullptr,
            IRHIQueue* queue = nullptr);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        /// フレーム開始
        void BeginFrame(uint64 frameNumber);

        /// フレーム終了
        /// 未使用リソースの退避判定
        void EndFrame();

        //=====================================================================
        // リソース登録
        //=====================================================================

        /// リソース登録
        void RegisterResource(IRHIResidentResource* resource);

        /// リソース登録解除
        void UnregisterResource(IRHIResidentResource* resource);

        /// リソース使用マーク
        /// @param resource リソース
        /// @param fenceValue 現在のフェンス値（GPU完了確認用）
        void MarkUsed(IRHIResidentResource* resource, uint64 fenceValue = 0);

        /// 複数リソース使用マーク
        void MarkUsed(IRHIResidentResource* const* resources, uint32 count, uint64 fenceValue = 0);

        //=====================================================================
        // 手動制御
        //=====================================================================

        /// 指定リソースの常駐を保証
        bool EnsureResident(IRHIResidentResource* resource);

        /// 指定リソース群の常駐を保証
        bool EnsureResident(IRHIResidentResource* const* resources, uint32 count);

        /// メモリ圧迫時の退避実行
        /// @note 退避前にm_fence->IsCompleted(res.lastUsedFenceValue)でGPU完了を確認する
        void PerformEviction();

        /// 非同期常駐要求
        /// @param resources リソース配列
        /// @param count リソース数
        /// @param fenceToSignal 完了時にシグナルするフェンス
        /// @param fenceValue シグナル値
        /// @return 要求が受理されたか
        virtual bool EnqueueMakeResident(
            IRHIResidentResource* const* resources,
            uint32 count,
            IRHIFence* fenceToSignal,
            uint64 fenceValue) = 0;

        //=====================================================================
        // 情報
        //=====================================================================

        /// 現在のVRAM使用量
        uint64 GetCurrentVideoMemoryUsage() const { return m_currentUsage; }

        /// 予算
        uint64 GetVideoMemoryBudget() const { return m_budget; }

        /// 使用率
        float GetUsageRatio() const {
            return m_budget > 0 ? static_cast<float>(m_currentUsage) / m_budget : 0.0f;
        }

        /// 常駐リソース数
        uint32 GetResidentResourceCount() const { return m_residentCount; }

        /// 退避リソース数
        uint32 GetEvictedResourceCount() const { return m_evictedCount; }

    private:
        IRHIDevice* m_device = nullptr;
        RHIResidencyManagerConfig m_config;

        uint64 m_budget = 0;
        uint64 m_currentUsage = 0;
        uint64 m_currentFrame = 0;

        IRHIFence* m_fence = nullptr;
        IRHIQueue* m_queue = nullptr;

        struct TrackedResource {
            IRHIResidentResource* resource;
            uint64 lastUsedFrame;
            uint64 lastUsedFenceValue = 0;
            ERHIResidencyStatus status;
        };
        TrackedResource* m_trackedResources = nullptr;
        uint32 m_trackedCount = 0;
        uint32 m_trackedCapacity = 0;

        uint32 m_residentCount = 0;
        uint32 m_evictedCount = 0;

        /// LRU退避候補選択
        void SelectEvictionCandidates(
            TrackedResource** outCandidates,
            uint32& outCount,
            uint64 targetSize);
    };
}
```

- [ ] RHIResidencyManagerConfig 構造体
- [ ] RHIResidencyManager クラス

### 5. ストリーミングサポート

```cpp
namespace NS::RHI
{
    /// ストリーミングレベル
    enum class ERHIStreamingLevel : uint8
    {
        /// 非ロード
        Unloaded,

        /// 最低品質（サムネイル）。
        Thumbnail,

        /// 低品質
        Low,

        /// 中品質
        Medium,

        /// 高品質
        High,

        /// 最高品質（フル解像度）。
        Highest,
    };

    /// ストリーミングリソースインターフェース
    class RHI_API IRHIStreamingResource
    {
    public:
        virtual ~IRHIStreamingResource() = default;

        /// 現在のストリーミングレベル取得
        virtual ERHIStreamingLevel GetCurrentStreamingLevel() const = 0;

        /// 要求されたストリーミングレベル取得
        virtual ERHIStreamingLevel GetRequestedStreamingLevel() const = 0;

        /// ストリーミングレベル要求
        virtual void RequestStreamingLevel(ERHIStreamingLevel level) = 0;

        /// ストリーミング完了しているか
        virtual bool IsStreamingComplete() const = 0;

        /// 各レベルのメモリサイズ取得
        virtual uint64 GetMemorySizeForLevel(ERHIStreamingLevel level) const = 0;
    };

    /// テクスチャストリーミングマネージャー
    class RHI_API RHITextureStreamingManager
    {
    public:
        RHITextureStreamingManager() = default;

        /// 初期化
        bool Initialize(
            IRHIDevice* device,
            RHIResidencyManager* residencyManager,
            uint64 streamingBudget);

        /// シャットダウン
        void Shutdown();

        //=====================================================================
        // フレーム操作
        //=====================================================================

        void BeginFrame();
        void EndFrame();

        //=====================================================================
        // リソース管理
        //=====================================================================

        /// ストリーミングリソース登録
        void RegisterResource(IRHIStreamingResource* resource);

        /// ストリーミングリソース登録解除
        void UnregisterResource(IRHIStreamingResource* resource);

        /// 使用距離更新（優先度計算用）。
        void UpdateResourceDistance(IRHIStreamingResource* resource, float distance);

        //=====================================================================
        // ストリーミング制御
        //=====================================================================

        /// ストリーミング予算設定
        void SetStreamingBudget(uint64 budget);

        /// 強制ロード（カットシーン等）
        void ForceLoad(IRHIStreamingResource* resource, ERHIStreamingLevel level);

        /// 処理優先度計算と更新
        void ProcessStreaming();

    private:
        IRHIDevice* m_device = nullptr;
        RHIResidencyManager* m_residencyManager = nullptr;
        uint64 m_budget = 0;

        struct StreamingEntry {
            IRHIStreamingResource* resource;
            float distance;
            float priority;
        };
        StreamingEntry* m_entries = nullptr;
        uint32 m_entryCount = 0;
        uint32 m_entryCapacity = 0;
    };
}
```

- [ ] ERHIStreamingLevel 列挙型
- [ ] IRHIStreamingResource インターフェース
- [ ] RHITextureStreamingManager クラス

## 検証方法

- [ ] 常駆退避操作
- [ ] 自動退避判定
- [ ] メモリ予算管理
- [ ] ストリーミング動作

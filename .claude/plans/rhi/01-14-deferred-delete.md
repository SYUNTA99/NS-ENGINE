# 01-14: 遅延削除キュー

## 目的

GPU使用完了を待ってからリソースを削除する遅延削除キューを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part3.md (リソースライフサイクル)
- docs/RHI/RHI_Implementation_Guide_Part6.md (スレッドィングモデル)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIDeferredDelete.h`

## 所有権と配置

- **IRHIDevice** が `RHIDeferredDeleteQueue` を1つ所有する
- `IRHIDevice::Shutdown()` 時に `FlushAll()` → キュー破棄
- RHIObjectPoolとの関係: プール可能リソースは `ObjectPool::Return()` を優先、プール不可リソースのみ遅延削除キューへ
- マルチデバイス環境: 各デバイスが独立したキューを持つ

## TODO

### 1. RHIDeferredDeleteEntry: 遅延削除エントリ

```cpp
#pragma once

#include "common/utility/types.h"
#include "RHIFwd.h"
#include <vector>
#include <mutex>

namespace NS::RHI
{
    /// 遅延削除エントリ
    struct RHIDeferredDeleteEntry
    {
        /// 削除対象リソース
        const IRHIResource* resource = nullptr;

        /// 待機するフェンス
        IRHIFence* fence = nullptr;

        /// 待機するフェンス値
        uint64 fenceValue = 0;

        /// フレーム番号（フェンスがない場合のフォールバック）。
        uint64 frameNumber = 0;
    };
}
```

- [ ] RHIDeferredDeleteEntry 構造体

### 2. RHIDeferredDeleteQueue: 基本構造

```cpp
namespace NS::RHI
{
    /// 遅延削除キュー
    /// GPUがリソースを使い終わるまで削除を遅延する
    class RHI_API RHIDeferredDeleteQueue
    {
    public:
        RHIDeferredDeleteQueue();
        ~RHIDeferredDeleteQueue();

        NS_DISALLOW_COPY(RHIDeferredDeleteQueue);

    public:
        //=====================================================================
        // 設定
        //=====================================================================

        /// 最大遅延フレーム数設定
        /// フェンスがない場合のフォールバック待機フレーム数
        void SetMaxDeferredFrames(uint32 frames);

        /// 現在のフレーム番号を設定
        void SetCurrentFrame(uint64 frameNumber);

    public:
        /// メモリプレッシャーハンドラを設定
        void SetMemoryPressureHandler(RHIMemoryPressureHandler* handler);

        /// プレッシャー通知閾値を設定
        void SetPressureThreshold(uint32 threshold);

    private:
        std::vector<RHIDeferredDeleteEntry> m_entries;
        std::mutex m_mutex;
        uint64 m_currentFrame = 0;
        uint32 m_maxDeferredFrames = 3;  // デフォルトフレーム

        /// メモリプレッシャーハンドラ参照（外部から設定）
        RHIMemoryPressureHandler* m_pressureHandler = nullptr;

        /// プレッシャー通知閾値（保留エントリ数）
        uint32 m_pressureThreshold = 256;
    };
}
```

- [ ] 基本クラス構造
- [ ] 設定メソッド

### 3. RHIDeferredDeleteQueue: エンキュー

```cpp
namespace NS::RHI
{
    class RHIDeferredDeleteQueue
    {
    public:
        //=====================================================================
        // エンキュー
        //=====================================================================

        /// フェンス指定でエンキュー
        /// @param resource 削除対象リソース
        /// @param fence 待機するフェンス
        /// @param fenceValue 待機するフェンス値
        void Enqueue(const IRHIResource* resource,
                     IRHIFence* fence,
                     uint64 fenceValue);

        /// フレーム遅延でエンキュー（フェンスなし）
        /// @param resource 削除対象リソース
        /// maxDeferredFrames 後に削除されめ
        void EnqueueFrameDeferred(const IRHIResource* resource);

        /// 即座に削除（同期削除）。
        /// @note GPU同期が完了していることを呼び出しのが保証
        static void DeleteImmediate(const IRHIResource* resource);
    };
}
```

- [ ] Enqueue (フェンス付き)
- [ ] EnqueueFrameDeferred (フレーム遅延)
- [ ] DeleteImmediate (即座削除)

### 4. RHIDeferredDeleteQueue: 処理

```cpp
namespace NS::RHI
{
    class RHIDeferredDeleteQueue
    {
    public:
        //=====================================================================
        // 処理
        //=====================================================================

        /// 完了した削除を処理
        /// @return 削除したリソース数
        /// @note 毎フレーム呼び出す
        uint32 ProcessCompletedDeletions();

        /// 全ての削除を強制実行（シャットダウン時）
        /// @note GPU同期後に呼び出すこと
        void FlushAll();

        /// ペンディング中のエントリ数
        size_t GetPendingCount() const;

        /// ペンディング中のメモリ推定量
        size_t GetPendingMemoryEstimate() const;
    };
}
```

- [ ] ProcessCompletedDeletions
- [ ] FlushAll
- [ ] 統計取得

### 5. RHIDeferredDeleteQueue: 実装詳細

```cpp
namespace NS::RHI
{
    // 実装（Private/RHIDeferredDelete.cpp）。

    void RHIDeferredDeleteQueue::Enqueue(
        const IRHIResource* resource,
        IRHIFence* fence,
        uint64 fenceValue)
    {
        if (!resource) return;

        // 遅延削除マスク
        resource->MarkForDeferredDelete();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.push_back({resource, fence, fenceValue, m_currentFrame});
    }

    uint32 RHIDeferredDeleteQueue::ProcessCompletedDeletions()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        uint32 deletedCount = 0;
        auto it = m_entries.begin();

        while (it != m_entries.end()) {
            bool canDelete = false;

            if (it->fence) {
                // フェンス完了チェック
                canDelete = it->fence->GetCompletedValue() >= it->fenceValue;
            } else {
                // フレーム遅延チェック
                canDelete = (m_currentFrame - it->frameNumber) >= m_maxDeferredFrames;
            }

            if (canDelete) {
                // 実際の削除実行
                it->resource->ExecuteDeferredDelete();
                it = m_entries.erase(it);
                ++deletedCount;
            } else {
                ++it;
            }
        }

        // メモリプレッシャー通知
        if (m_pressureHandler && m_entries.size() > m_pressureThreshold)
        {
            ERHIMemoryPressure level = (m_entries.size() > m_pressureThreshold * 4)
                ? ERHIMemoryPressure::High
                : ERHIMemoryPressure::Medium;
            m_pressureHandler->NotifyPressureChange(level);
        }

        return deletedCount;
    }

    void RHIDeferredDeleteQueue::FlushAll()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& entry : m_entries) {
            entry.resource->ExecuteDeferredDelete();
        }
        m_entries.clear();
    }
}
```

- [ ] 削除条件チェック
- [ ] 実際の削除実行

## 検証方法

- [ ] フェンス完了後削除確認
- [ ] フレーム遅延削除の正確性
- [ ] スレッドセーフティ
- [ ] メモリリーク検出

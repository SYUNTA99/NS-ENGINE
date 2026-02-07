# 02-04: コマンドアロケーター

## 目的

コマンドリストのメモリ管理を行うコマンドアロケーターインターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part2.md (コマンドアロケーター)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHICommandAllocator.h`

## TODO

### 1. IRHICommandAllocator: 基本定義

```cpp
#pragma once

#include "IRHIResource.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// コマンドアロケーター
    /// コマンドリストのメモリを管理するオブジェクト
    class RHI_API IRHICommandAllocator : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(CommandAllocator)

        virtual ~IRHICommandAllocator() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 対応するキュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;
    };
}
```

- [ ] 基本インターフェース定義

### 2. ライフサイクル管理

```cpp
namespace NS::RHI
{
    class IRHICommandAllocator
    {
    public:
        //=====================================================================
        // ライフサイクル
        //=====================================================================

        /// リセット
        /// @note GPU完了後にのみ呼び出し可能
        /// 関連付けられたのコマンドリストのメモリを再利用可能にする
        virtual void Reset() = 0;

        /// GPU使用中か
        /// @note 使用中はリセット不可
        virtual bool IsInUse() const = 0;
    };
}
```

- [ ] Reset
- [ ] IsInUse

### 3. メモリ管理

```cpp
namespace NS::RHI
{
    class IRHICommandAllocator
    {
    public:
        //=====================================================================
        // メモリ情報
        //=====================================================================

        /// 割り当て済みメモリサイズ取得
        virtual uint64 GetAllocatedMemory() const = 0;

        /// 使用中メモリサイズ取得
        virtual uint64 GetUsedMemory() const = 0;

        /// メモリ使用率
        float GetMemoryUsageRatio() const {
            uint64 allocated = GetAllocatedMemory();
            return allocated > 0
                ? static_cast<float>(GetUsedMemory()) / allocated
                : 0.0f;
        }
    };
}
```

- [ ] GetAllocatedMemory / GetUsedMemory
- [ ] メモリ使用率

### 4. 待機フェンス

```cpp
namespace NS::RHI
{
    class IRHICommandAllocator
    {
    public:
        //=====================================================================
        // 待機フェンス
        // アロケーターの再利用可能タイミング追跡用
        //=====================================================================

        /// 待機フェンス設定
        /// @param fence 待機するフェンス
        /// @param value 待機するフェンス値
        virtual void SetWaitFence(IRHIFence* fence, uint64 value) = 0;

        /// 待機フェンス取得
        virtual IRHIFence* GetWaitFence() const = 0;

        /// 待機フェンス値取得
        virtual uint64 GetWaitFenceValue() const = 0;

        /// 待機完了後確認
        bool IsWaitComplete() const {
            IRHIFence* fence = GetWaitFence();
            return fence == nullptr ||
                   fence->GetCompletedValue() >= GetWaitFenceValue();
        }
    };
}
```

- [ ] SetWaitFence / GetWaitFence
- [ ] IsWaitComplete

### 5. コマンドアロケータープール

```cpp
namespace NS::RHI
{
    /// コマンドアロケータープール
    /// アロケーターの再利用管理
    class RHI_API IRHICommandAllocatorPool
    {
    public:
        virtual ~IRHICommandAllocatorPool() = default;

        /// アロケーター取得
        /// @param queueType キュータイプ
        /// @return 利用可能なアロケーター
        virtual IRHICommandAllocator* Obtain(ERHIQueueType queueType) = 0;

        /// アロケーター返却
        /// @param allocator 返却するアロケーター
        /// @param fence 完了時フェンス
        /// @param fenceValue 完了時フェンス値
        virtual void Release(
            IRHICommandAllocator* allocator,
            IRHIFence* fence,
            uint64 fenceValue) = 0;

        /// 完了したアロケーターを再利用可能にする
        /// @return 再利用可能になったアロケーター数
        virtual uint32 ProcessCompletedAllocators() = 0;

        /// プール内のアロケーター数
        virtual uint32 GetPooledCount(ERHIQueueType queueType) const = 0;

        /// 使用中のアロケーター数
        virtual uint32 GetInUseCount(ERHIQueueType queueType) const = 0;
    };
}
```

- [ ] IRHICommandAllocatorPool インターフェース
- [ ] Obtain / Release
- [ ] ProcessCompletedAllocators

## 検証方法

- [ ] ライフサイクル管理の正確性
- [ ] プールの再利用効率

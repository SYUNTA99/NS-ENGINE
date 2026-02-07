# 09-01: フェンスインターフェース

## 目的

GPU同期のためのフェンスインターフェースを定義する。

## 参照ドキュメント

- 01-26-queue-fence.md (キュー同期)
- 01-02-types-gpu.md (基本型

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIFence.h`

## TODO

### 1. フェンス記述

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// フェンス記述
    struct RHI_API RHIFenceDesc
    {
        /// 初期値
        uint64 initialValue = 0;

        /// フラグ
        enum class Flags : uint32 {
            None = 0,
            /// 共有フェンス（プロセス間共有可能）。
            Shared = 1 << 0,
            /// クロスアダプター共有
            CrossAdapter = 1 << 1,
            /// モニター対応
            MonitoredFence = 1 << 2,
        };
        Flags flags = Flags::None;
    };
    RHI_ENUM_CLASS_FLAGS(RHIFenceDesc::Flags)
}
```

- [ ] RHIFenceDesc 構造体

### 2. IRHIFence インターフェース

```cpp
namespace NS::RHI
{
    /// フェンス
    /// GPU-CPU間およびキュー間の同期プリミティブ
    class RHI_API IRHIFence : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(Fence)

        virtual ~IRHIFence() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 現在の完了を取得
        virtual uint64 GetCompletedValue() const = 0;

        /// 最後にシグナルされた値
        virtual uint64 GetLastSignaledValue() const = 0;

        //=====================================================================
        // シグナル・待機。
        //=====================================================================

        /// CPU側でシグナル（テスト用）。
        virtual void Signal(uint64 value) = 0;

        /// CPU側で待つ。
        /// @param value 待機する値
        /// @param timeoutMs タイムアウト（ミリ秒、UINT64_MAXで無限待機）
        /// @return 成功したか（タイムアウトでfalse）。
        virtual bool Wait(uint64 value, uint64 timeoutMs = UINT64_MAX) = 0;

        /// 待機（複数値のいずれか）。
        virtual bool WaitAny(const uint64* values, uint32 count, uint64 timeoutMs = UINT64_MAX) = 0;

        /// 待機（複数値のすべて）。
        virtual bool WaitAll(const uint64* values, uint32 count, uint64 timeoutMs = UINT64_MAX) = 0;

        //=====================================================================
        // イベント
        //=====================================================================

        /// 完了時イベント通知設定
        virtual void SetEventOnCompletion(uint64 value, void* eventHandle) = 0;

        //=====================================================================
        // 共有
        //=====================================================================

        /// 共有ハンドル取得（プロセス間共有用）。
        virtual void* GetSharedHandle() const = 0;

        /// 共有ハンドルからフェンスを開く
        /// IRHIDevice経由で呼び出す

        //=====================================================================
        // ユーティリティ
        //=====================================================================

        /// 指定値が完了しているか
        bool IsCompleted(uint64 value) const {
            return GetCompletedValue() >= value;
        }

        /// 即座にポーリング（待機なし）
        bool Poll(uint64 value) const {
            return IsCompleted(value);
        }
    };

    using RHIFenceRef = TRefCountPtr<IRHIFence>;
}
```

- [ ] IRHIFence インターフェース
- [ ] シグナル/待機API
- [ ] 共有ハンドル

### 3. フェンス作成

```cpp
namespace NS::RHI
{
    /// フェンス作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// フェンス作成
        virtual IRHIFence* CreateFence(
            const RHIFenceDesc& desc,
            const char* debugName = nullptr) = 0;

        /// 初期値指定でフェンス作成
        IRHIFence* CreateFence(
            uint64 initialValue = 0,
            const char* debugName = nullptr)
        {
            RHIFenceDesc desc;
            desc.initialValue = initialValue;
            return CreateFence(desc, debugName);
        }

        /// 共有フェンスを開く
        virtual IRHIFence* OpenSharedFence(
            void* sharedHandle,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] CreateFence
- [ ] OpenSharedFence

### 4. キュー同期操作

```cpp
namespace NS::RHI
{
    /// キュー同期操作（RHIQueueに追加）。
    class IRHIQueue
    {
    public:
        /// フェンスをシグナル
        /// コマンドリスト実行後にシグナル
        virtual void Signal(IRHIFence* fence, uint64 value) = 0;

        /// フェンスを待つ。
        /// 次のコマンド実行前に待つ。
        virtual void Wait(IRHIFence* fence, uint64 value) = 0;

        //=====================================================================
        // 便利メソッド
        //=====================================================================

        /// 次の値でシグナルし、その値を返す
        uint64 SignalNext(IRHIFence* fence) {
            uint64 value = fence->GetLastSignaledValue() + 1;
            Signal(fence, value);
            return value;
        }

        /// 他キューの完了を待つ。
        void WaitForQueue(IRHIQueue* otherQueue, IRHIFence* fence, uint64 value) {
            Wait(fence, value);
        }
    };
}
```

- [ ] Signal/Wait on Queue
- [ ] 便利メソッド

### 5. フェンスヘルパー

```cpp
namespace NS::RHI
{
    /// フェンス値管理ヘルパー。
    class RHI_API RHIFenceValueTracker
    {
    public:
        RHIFenceValueTracker() = default;

        /// 初期化
        void Initialize(IRHIFence* fence) {
            m_fence = fence;
            m_nextValue = fence->GetCompletedValue() + 1;
        }

        /// 次の値取得（インクリメント）
        uint64 GetNextValue() { return m_nextValue++; }

        /// 現在の次の値（インクリメントなし）
        uint64 PeekNextValue() const { return m_nextValue; }

        /// キューでシグナルし、値を返す
        uint64 Signal(IRHIQueue* queue) {
            uint64 value = GetNextValue();
            queue->Signal(m_fence, value);
            return value;
        }

        /// CPU待ち。
        bool WaitCPU(uint64 value, uint64 timeoutMs = UINT64_MAX) {
            return m_fence->Wait(value, timeoutMs);
        }

        /// 最新完了後
        uint64 GetCompletedValue() const {
            return m_fence->GetCompletedValue();
        }

        /// 指定値が完了しているか
        bool IsCompleted(uint64 value) const {
            return m_fence->IsCompleted(value);
        }

        /// フェンス取得
        IRHIFence* GetFence() const { return m_fence; }

    private:
        IRHIFence* m_fence = nullptr;
        uint64 m_nextValue = 1;
    };
}
```

- [ ] RHIFenceValueTracker クラス

## 検証方法

- [ ] フェンス作成/破棄
- [ ] シグナル/待機動作
- [ ] 複数キュー間同期
- [ ] タイムアウト処理

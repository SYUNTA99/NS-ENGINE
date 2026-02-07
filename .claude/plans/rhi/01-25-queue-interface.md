# 01-25: IRHIQueueインターフェース

## 目的

コマンドキューを抽象化するIRHIQueueインターフェースを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (5. Queue実装詳細)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIQueue.h`

## TODO

### 1. IRHIQueue: 基本プロパティ

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// コマンドキューインターフェース
    /// GPUにコマンドを送信するためのキュー
    class RHI_API IRHIQueue
    {
    public:
        virtual ~IRHIQueue() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 親デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// キュータイプ取得
        virtual ERHIQueueType GetQueueType() const = 0;

        /// キューインデックス取得（同タイプ中での番号）。
        virtual uint32 GetQueueIndex() const = 0;

        /// キュー名取得（デバッグ用）。
        const char* GetQueueTypeName() const {
            return NS::RHI::GetQueueTypeName(GetQueueType());
        }
    };
}
```

- [ ] 基本プロパティメソッド

### 2. IRHIQueue: 機能サポート

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // 機能サポートクエリ
        //=====================================================================

        /// グラフィックス操作をサポートするか
        bool SupportsGraphics() const {
            return GetQueueType() == ERHIQueueType::Graphics;
        }

        /// コンピュート操作をサポートするか
        bool SupportsCompute() const {
            return GetQueueType() != ERHIQueueType::Copy;
        }

        /// コピー操作をサポートするか
        bool SupportsCopy() const {
            return true;  // 全キューでサポート
        }

        /// タイムスタンプクエリをサポートするか
        virtual bool SupportsTimestampQueries() const = 0;

        /// タイルマッピングをサポートするか
        virtual bool SupportsTileMapping() const = 0;
    };
}
```

- [ ] 機能サポートクエリ

### 3. IRHIQueue: コマンド実行

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // コマンド実行
        //=====================================================================

        /// コマンドリストを実行
        /// @param commandLists コマンドリストの分
        /// @param count コマンドリスト数
        virtual void ExecuteCommandLists(
            IRHICommandList* const* commandLists,
            uint32 count) = 0;

        /// 単一コマンドリストを実行（便利関数）。
        void ExecuteCommandList(IRHICommandList* commandList) {
            ExecuteCommandLists(&commandList, 1);
        }

        /// コンテキストを直接実行
        /// 内のでFinalizeしてから実行
        virtual void ExecuteContext(IRHICommandContext* context) = 0;
    };
}
```

- [ ] ExecuteCommandLists
- [ ] ExecuteContext

### 4. IRHIQueue: 同期

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // 同期
        //=====================================================================

        /// フェンスシグナル
        /// @param fence シグナルするフェンス
        /// @param value シグナル値
        virtual void Signal(IRHIFence* fence, uint64 value) = 0;

        /// フェンス待ち（CPU側）。
        /// @param fence 待機するフェンス
        /// @param value 待機する値
        virtual void Wait(IRHIFence* fence, uint64 value) = 0;

        /// キューの全コマンド完了後待機（CPU側）。
        virtual void Flush() = 0;
    };
}
```

- [ ] Signal / Wait
- [ ] Flush

### 5. IRHIQueue: タイミング/診断

```cpp
namespace NS::RHI
{
    class IRHIQueue
    {
    public:
        //=====================================================================
        // タイミング
        //=====================================================================

        /// GPUタイムスタンプ取得
        /// @return 現在のGPUタイムスタンプ
        virtual uint64 GetGPUTimestamp() const = 0;

        /// タイムスタンプ周波数取得（キュー固有）
        virtual uint64 GetTimestampFrequency() const = 0;

        //=====================================================================
        // 診断
        //=====================================================================

        /// キュー説明文字の取得（デバッグ用）。
        virtual const char* GetDescription() const = 0;

        /// デバッグマーカー挿入（IX等）
        virtual void InsertDebugMarker(const char* name, uint32 color = 0) = 0;

        /// デバッグイベント開始
        virtual void BeginDebugEvent(const char* name, uint32 color = 0) = 0;

        /// デバッグイベント終了
        virtual void EndDebugEvent() = 0;
    };

    /// デバッグイベントスコープ（RAII）。
    struct RHIQueueDebugEventScope
    {
        NS_DISALLOW_COPY(RHIQueueDebugEventScope);

        IRHIQueue* queue;

        RHIQueueDebugEventScope(IRHIQueue* q, const char* name, uint32 color = 0)
            : queue(q)
        {
            if (queue) queue->BeginDebugEvent(name, color);
        }

        ~RHIQueueDebugEventScope()
        {
            if (queue) queue->EndDebugEvent();
        }
    };

    #define RHI_QUEUE_DEBUG_EVENT(queue, name) \
        ::NS::RHI::RHIQueueDebugEventScope NS_MACRO_CONCATENATE(_rhiQueueEvent, __LINE__)(queue, name)
}
```

- [ ] GPUタイムスタンプ
- [ ] デバッグマーカー/イベント
- [ ] RAIIスコープ

## 検証方法

- [ ] コマンド実行フロー
- [ ] 同期の正確性
- [ ] デバッグ機能の動作

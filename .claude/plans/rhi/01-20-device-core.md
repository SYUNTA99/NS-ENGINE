# 01-20: IRHIDevice基本プロパティ

## 目的

論理デバイスの基本プロパティを定義する。

## 参照ドキュメント

- docs/RHI/RHI_Implementation_Guide_Part1.md (4. Device実装詳細)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHIDevice.h`

## TODO

### 1. IRHIDevice: 基本定義

```cpp
#pragma once

#include "RHIFwd.h"
#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// 論理デバイスインターフェース
    /// GPU機能へのアクセスとリソース管理を提供
    class RHI_API IRHIDevice
    {
    public:
        virtual ~IRHIDevice() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 親アダプター取得
        virtual IRHIAdapter* GetAdapter() const = 0;

        /// このデバイスのGPUマスク取得
        virtual GPUMask GetGPUMask() const = 0;

        /// GPUインデックス取得（マルチGPU時）
        virtual uint32 GetGPUIndex() const = 0;

        /// プライマリデバイスか（GPU0）。
        bool IsPrimaryDevice() const { return GetGPUIndex() == 0; }
    };
}
```

- [ ] 基本インターフェース定義
- [ ] アダプター/GPU識別

### 2. IRHIDevice: デバイス情報

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // デバイス情報
        //=====================================================================

        /// タイムスタンプ周波数取得（Hz）。
        /// GPUタイムスタンプをミリ秒に変換: ms = ticks / (frequency / 1000)
        virtual uint64 GetTimestampFrequency() const = 0;

        /// 現在のメモリバジェットを取得
        virtual RHIMemoryBudget GetMemoryBudget() const = 0;

        /// 定数バッファアライメント取得
        virtual uint32 GetConstantBufferAlignment() const = 0;

        /// テクスチャデータアライメント取得
        virtual uint32 GetTextureDataAlignment() const = 0;

        /// デバイスが有効か
        virtual bool IsValid() const = 0;

        /// デバイスロスト状態か
        virtual bool IsDeviceLost() const = 0;
    };

    /// メモリバジェット情報
    struct RHIMemoryBudget
    {
        /// 使用可能なメモリ量（バジェット）。
        uint64 budget = 0;

        /// 現在の使用量
        uint64 currentUsage = 0;

        /// 利用可能な残り
        uint64 GetAvailable() const {
            return budget > currentUsage ? budget - currentUsage : 0;
        }

        /// 使用率（0.0〜1.0）。
        float GetUsageRatio() const {
            return budget > 0 ? static_cast<float>(currentUsage) / budget : 0.0f;
        }
    };
}
```

- [ ] タイムスタンプ周波数
- [ ] メモリバジェット
- [ ] デバイス状態

### 3. IRHIDevice: デフォルトビュー

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // デフォルトビュー
        // バインドされていいロック用のNullビュー
        //=====================================================================

        /// Null SRV取得
        /// バインドされていいSRVスロットに使用
        virtual IRHIShaderResourceView* GetNullSRV() const = 0;

        /// Null UAV取得
        virtual IRHIUnorderedAccessView* GetNullUAV() const = 0;

        /// Null CBV取得（定数バッファ）。
        virtual IRHIConstantBufferView* GetNullCBV() const = 0;

        /// Null サンプラー取得
        virtual IRHISampler* GetNullSampler() const = 0;
    };
}
```

- [ ] Nullビュー取得メソッド

### 4. IRHIDevice: フォーマットサポート

```cpp
namespace NS::RHI
{
    /// フォーマットサポート情報
    struct RHIFormatSupport
    {
        /// バッファとして使用可能か
        bool buffer = false;

        /// テクスチャとして使用可能か（RV）。
        bool texture = false;

        /// レンダーターゲットとして使用可能か
        bool renderTarget = false;

        /// デプスステンシルとして使用可能か
        bool depthStencil = false;

        /// UAVとして使用可能か
        bool unorderedAccess = false;

        /// MIPマップ自動生成可能か
        bool mipMapGeneration = false;

        /// マルチサンプル可能か
        bool multisample = false;

        /// 最大サンプル数
        ERHISampleCount maxSampleCount = ERHISampleCount::Count1;
    };

    class IRHIDevice
    {
    public:
        //=====================================================================
        // フォーマットサポートクエリ
        //=====================================================================

        /// フォーマットサポート情報取得
        virtual RHIFormatSupport GetFormatSupport(EPixelFormat format) const = 0;

        /// レンダーターゲットとして使用可能か
        bool SupportsRenderTarget(EPixelFormat format) const {
            return GetFormatSupport(format).renderTarget;
        }

        /// デプスステンシルとして使用可能か
        bool SupportsDepthStencil(EPixelFormat format) const {
            return GetFormatSupport(format).depthStencil;
        }

        /// UAVとして使用可能か
        bool SupportsUAV(EPixelFormat format) const {
            return GetFormatSupport(format).unorderedAccess;
        }
    };
}
```

- [ ] RHIFormatSupport 構造体
- [ ] フォーマットサポートクエリ

### 5. IRHIDevice: ユーティリティ

```cpp
namespace NS::RHI
{
    class IRHIDevice
    {
    public:
        //=====================================================================
        // ユーティリティ
        //=====================================================================

        /// GPU同期のコマンド完了待ち。
        /// @note パフォーマンス影響大、デバッグ/シャットダウン時のみ使用
        virtual void WaitIdle() = 0;

        /// 遅延削除キュー取得
        virtual RHIDeferredDeleteQueue* GetDeferredDeleteQueue() = 0;

        /// デバッグ名設定（ネイティブオブジェクトに反映）。
        virtual void SetDebugName(const char* name) = 0;

        /// 現在のフレーム番号取得
        virtual uint64 GetCurrentFrameNumber() const = 0;
    };
}
```

- [ ] WaitIdle
- [ ] 遅延削除キュー取得
- [ ] デバッグ機能

## 検証方法

- [ ] インターフェース定義の完全性
- [ ] プロパティの網羅性
- [ ] Nullビューの有効性

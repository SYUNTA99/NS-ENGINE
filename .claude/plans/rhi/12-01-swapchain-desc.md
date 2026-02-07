# 12-01: スワップチェーン記述

## 目的

スワップチェーンの作成パラメータを定義する。

## 参照ドキュメント

- 15-01-pixel-format-enum.md (ERHIPixelFormat)
- 01-03-types-geometry.md (Extent2D)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHISwapChain.h` (部分

## TODO

### 1. スワップチェーンフラグ

```cpp
#pragma once

#include "RHITypes.h"
#include "RHIEnums.h"

namespace NS::RHI
{
    /// スワップチェーンフラグ
    enum class ERHISwapChainFlags : uint32
    {
        None = 0,

        /// ティアリング許可（VSync無効時）
        AllowTearing = 1 << 0,

        /// フレームレイテンシ待機オブジェクト有効
        FrameLatencyWaitableObject = 1 << 1,

        /// 前のバッファ内容を保持
        AllowModeSwitch = 1 << 2,

        /// ステレオスコピック3D
        Stereo = 1 << 3,

        /// GDI互換
        GDICompatible = 1 << 4,

        /// 制限出力（コピー保護）。
        RestrictedContent = 1 << 5,

        /// 共有リソースドライチ
        SharedResourceDriver = 1 << 6,

        /// フルスクリーン動画再生
        YUVSwapChain = 1 << 7,

        /// HDR出力
        HDR = 1 << 8,
    };
    RHI_ENUM_CLASS_FLAGS(ERHISwapChainFlags)
}
```

- [ ] ERHISwapChainFlags 列挙型

### 2. プレゼントモード

```cpp
namespace NS::RHI
{
    /// プレゼントモード
    enum class ERHIPresentMode : uint8
    {
        /// 即時（ティアリングあり）。
        Immediate,

        /// VSync（垂直同期）。
        VSync,

        /// 可変リフレッシュレート（FreeSync/G-Sync）。
        VariableRefreshRate,

        /// メールボックス（最新フレームのみ保持）。
        Mailbox,

        /// FIFO（キュー方式）
        FIFO,
    };

    /// スケーリングモード
    enum class ERHIScalingMode : uint8
    {
        /// ストレット
        Stretch,

        /// アスペクト比維持
        AspectRatioStretch,

        /// なし（1:1マッピング）。
        None,
    };

    /// アルファモード
    enum class ERHIAlphaMode : uint8
    {
        /// 無視
        Ignore,

        /// 事前乗算済み
        Premultiplied,

        /// 直接
        Straight,
    };
}
```

- [ ] ERHIPresentMode 列挙型
- [ ] ERHIScalingMode 列挙型
- [ ] ERHIAlphaMode 列挙型

### 3. スワップチェーン記述

```cpp
namespace NS::RHI
{
    /// スワップチェーン記述
    struct RHI_API RHISwapChainDesc
    {
        /// ウィンドウハンドル
        void* windowHandle = nullptr;

        /// 幅で自動）
        uint32 width = 0;

        /// 高さ（で自動）
        uint32 height = 0;

        /// バッファフォーマット
        ERHIPixelFormat format = ERHIPixelFormat::R8G8B8A8_UNORM;

        /// バッファ数
        uint32 bufferCount = 2;

        /// プレゼントモード
        ERHIPresentMode presentMode = ERHIPresentMode::VSync;

        /// フラグ
        ERHISwapChainFlags flags = ERHISwapChainFlags::None;

        /// スケーリングモード
        ERHIScalingMode scalingMode = ERHIScalingMode::Stretch;

        /// アルファモード
        ERHIAlphaMode alphaMode = ERHIAlphaMode::Ignore;

        /// サンプル数（MSAA）。
        uint32 sampleCount = 1;

        /// サンプル品質
        uint32 sampleQuality = 0;

        /// フルスクリーン
        bool fullscreen = false;

        /// 出力モニター（nullでプライマリ）。
        void* outputMonitor = nullptr;

        //=====================================================================
        // ビルダー
        //=====================================================================

        /// ウィンドウから作成
        static RHISwapChainDesc ForWindow(
            void* hwnd,
            uint32 w = 0,
            uint32 h = 0,
            uint32 buffers = 2)
        {
            RHISwapChainDesc desc;
            desc.windowHandle = hwnd;
            desc.width = w;
            desc.height = h;
            desc.bufferCount = buffers;
            return desc;
        }

        /// HDR対応
        RHISwapChainDesc& EnableHDR() {
            format = ERHIPixelFormat::R10G10B10A2_UNORM;
            flags = flags | ERHISwapChainFlags::HDR;
            return *this;
        }

        /// ティアリング許可
        RHISwapChainDesc& AllowTearing() {
            flags = flags | ERHISwapChainFlags::AllowTearing;
            presentMode = ERHIPresentMode::Immediate;
            return *this;
        }

        /// トリプルバッファリング
        RHISwapChainDesc& TripleBuffering() {
            bufferCount = 3;
            return *this;
        }
    };
}
```

- [ ] RHISwapChainDesc 構造体
- [ ] ビルダーメソッド

### 4. フルスクリーン記述

```cpp
namespace NS::RHI
{
    /// ディスプレイモード
    struct RHI_API RHIDisplayMode
    {
        /// 解像度
        uint32 width = 0;
        uint32 height = 0;

        /// リフレッシュレート（有理数）。
        uint32 refreshRateNumerator = 60;
        uint32 refreshRateDenominator = 1;

        /// フォーマット
        ERHIPixelFormat format = ERHIPixelFormat::R8G8B8A8_UNORM;

        /// スキャンライン順序）。
        enum class ScanlineOrder : uint8 {
            Unspecified,
            Progressive,
            UpperFieldFirst,
            LowerFieldFirst,
        };
        ScanlineOrder scanlineOrder = ScanlineOrder::Progressive;

        /// スケーリング
        ERHIScalingMode scaling = ERHIScalingMode::Stretch;

        /// リフレッシュレート（Hz）。
        float GetRefreshRateHz() const {
            return static_cast<float>(refreshRateNumerator) / refreshRateDenominator;
        }
    };

    /// フルスクリーン記述
    struct RHI_API RHIFullscreenDesc
    {
        /// ディスプレイモード
        RHIDisplayMode displayMode;

        /// 排他フルスクリーン（疑似フルスクリーンではない
        bool exclusiveFullscreen = false;

        /// 解像度切り替え許可
        bool allowModeSwitch = true;

        /// ウィンドウがフォーカスを失ったときにウィンドウモードに戻るか
        bool autoAltEnter = true;
    };
}
```

- [ ] RHIDisplayMode 構造体
- [ ] RHIFullscreenDesc 構造体

### 5. 出力情報

```cpp
namespace NS::RHI
{
    /// 出力（モニター（情報
    struct RHI_API RHIOutputInfo
    {
        /// 出力名
        char name[256] = {};

        /// デスクトップ座標
        int32 desktopX = 0;
        int32 desktopY = 0;

        /// 解像度
        uint32 width = 0;
        uint32 height = 0;

        /// プライマリモニターか
        bool isPrimary = false;

        /// HDR対応か
        bool supportsHDR = false;

        /// 現在のリフレットュレーチ
        float currentRefreshRate = 60.0f;

        /// 可変リフレッシュレート対応
        bool supportsVariableRefreshRate = false;
    };

    /// 出力情報取得（RHIAdapterに追加）。
    class IRHIAdapter
    {
    public:
        /// 出力数取得
        virtual uint32 GetOutputCount() const = 0;

        /// 出力情報取得
        virtual bool GetOutputInfo(uint32 index, RHIOutputInfo& outInfo) const = 0;

        /// 出力のサポートされるディスプレイモード列挙
        virtual uint32 EnumerateDisplayModes(
            uint32 outputIndex,
            ERHIPixelFormat format,
            RHIDisplayMode* outModes,
            uint32 maxModes) const = 0;

        /// 最適なディスプレイモード取得
        virtual bool FindClosestDisplayMode(
            uint32 outputIndex,
            const RHIDisplayMode& target,
            RHIDisplayMode& outClosest) const = 0;
    };
}
```

- [ ] RHIOutputInfo 構造体
- [ ] IRHIAdapter出力情報メソッド

## 検証方法

- [ ] 記述構造体の完全性
- [ ] ビルダーの動作
- [ ] 出力情報取得
- [ ] ディスプレイモード列挙

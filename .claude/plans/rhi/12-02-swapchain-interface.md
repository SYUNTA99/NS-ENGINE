# 12-02: IRHISwapChain インターフェース

## 目的

スワップチェーンインターフェースを定義する。

## 参照ドキュメント

- 12-01-swapchain-desc.md (RHISwapChainDesc)
- 04-02-texture-interface.md (IRHITexture)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/IRHISwapChain.h` (部分

## TODO

### 1. IRHISwapChain インターフェース

```cpp
namespace NS::RHI
{
    /// スワップチェーン
    class RHI_API IRHISwapChain : public IRHIResource
    {
    public:
        DECLARE_RHI_RESOURCE_TYPE(SwapChain)

        virtual ~IRHISwapChain() = default;

        //=====================================================================
        // 基本プロパティ
        //=====================================================================

        /// 所属デバイス取得
        virtual IRHIDevice* GetDevice() const = 0;

        /// 幅取得
        virtual uint32 GetWidth() const = 0;

        /// 高さ取得
        virtual uint32 GetHeight() const = 0;

        /// フォーマット取得
        virtual ERHIPixelFormat GetFormat() const = 0;

        /// バッファ数取得
        virtual uint32 GetBufferCount() const = 0;

        /// プレゼントモード取得
        virtual ERHIPresentMode GetPresentMode() const = 0;

        /// フラグ取得
        virtual ERHISwapChainFlags GetFlags() const = 0;

        //=====================================================================
        // バックバッファ
        //=====================================================================

        /// 現在のバックバッファインデックス取得
        virtual uint32 GetCurrentBackBufferIndex() const = 0;

        /// バックバッファテクスチャ取得
        virtual IRHITexture* GetBackBuffer(uint32 index) const = 0;

        /// 現在のバックバッファ取得
        IRHITexture* GetCurrentBackBuffer() const {
            return GetBackBuffer(GetCurrentBackBufferIndex());
        }

        /// バックバッファRTV取得
        virtual IRHIRenderTargetView* GetBackBufferRTV(uint32 index) const = 0;

        /// 現在のバックバッファRTV取得
        IRHIRenderTargetView* GetCurrentBackBufferRTV() const {
            return GetBackBufferRTV(GetCurrentBackBufferIndex());
        }

        //=====================================================================
        // フルスクリーン
        //=====================================================================

        /// フルスクリーンか
        virtual bool IsFullscreen() const = 0;

        /// フルスクリーン切り替え
        virtual bool SetFullscreen(bool fullscreen, const RHIFullscreenDesc* desc = nullptr) = 0;

        //=====================================================================
        // 状態
        //=====================================================================

        /// ウィンドウがオクルードされているか（最小化等）
        virtual bool IsOccluded() const = 0;

        /// HDRが有効か
        virtual bool IsHDREnabled() const = 0;

        /// 可変リフレッシュレートが有効か
        virtual bool IsVariableRefreshRateEnabled() const = 0;
    };

    using RHISwapChainRef = TRefCountPtr<IRHISwapChain>;
}
```

- [ ] IRHISwapChain インターフェース
- [ ] バックバッファアクセス
- [ ] フルスクリーン操作

### 2. スワップチェーン作成

```cpp
namespace NS::RHI
{
    /// スワップチェーン作成（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// スワップチェーン作成
        virtual IRHISwapChain* CreateSwapChain(
            const RHISwapChainDesc& desc,
            IRHIQueue* presentQueue,
            const char* debugName = nullptr) = 0;
    };
}
```

- [ ] CreateSwapChain

### 3. リサイズ処理

```cpp
namespace NS::RHI
{
    /// リサイズ記述
    struct RHI_API RHISwapChainResizeDesc
    {
        /// 新しい幅（0で現在のウィンドウサイズ）。
        uint32 width = 0;

        /// 新しい高さ
        uint32 height = 0;

        /// 新しいフォーマット（Unknownで変更なし）
        ERHIPixelFormat format = ERHIPixelFormat::Unknown;

        /// 新しいバッファ数（で変更なし）
        uint32 bufferCount = 0;

        /// 新しいフラグ（で変更なし）
        ERHISwapChainFlags flags = ERHISwapChainFlags::None;
    };

    /// リサイズ操作（RHISwapChainに追加）。
    class IRHISwapChain
    {
    public:
        /// リサイズ
        /// @pre デバッグビルドでは、すべてのバックバッファの参照カウントが
        ///      スワップチェーン内部保持分（1）のみであることをアサートする
        /// @pre GPU が当該スワップチェーンに対するコマンドを完了していること
        virtual bool Resize(const RHISwapChainResizeDesc& desc) = 0;

        /// 簡易リサイズ
        bool Resize(uint32 width, uint32 height) {
            RHISwapChainResizeDesc desc;
            desc.width = width;
            desc.height = height;
            return Resize(desc);
        }
    };
}
```

- [ ] RHISwapChainResizeDesc 構造体
- [ ] Resize操作

### 4. スワップチェーンイベント

```cpp
namespace NS::RHI
{
    /// スワップチェーンイベントタイプ
    enum class ERHISwapChainEvent : uint8
    {
        /// バックバッファが利用可能
        BackBufferAvailable,

        /// サイズ変更が必要。
        ResizeNeeded,

        /// フルスクリーン状態変更
        FullscreenChanged,

        /// HDR状態変更
        HDRChanged,

        /// デバイスロスト
        DeviceLost,
    };

    /// スワップチェーンイベントコールバック
    using RHISwapChainEventCallback = void(*)(
        IRHISwapChain* swapChain,
        ERHISwapChainEvent event,
        void* userData);

    /// イベントコールバック設定（RHISwapChainに追加）。
    class IRHISwapChain
    {
    public:
        /// イベントコールバック設定
        virtual void SetEventCallback(
            RHISwapChainEventCallback callback,
            void* userData = nullptr) = 0;

        /// ウィンドウメッセージ処理（Win32用）。
        /// @return 処理したか
        virtual bool ProcessWindowMessage(
            void* hwnd,
            uint32 message,
            uint64 wParam,
            int64 lParam) = 0;
    };
}
```

- [ ] ERHISwapChainEvent 列挙型
- [ ] RHISwapChainEventCallback
- [ ] ProcessWindowMessage

### 5. 待機オブジェクト

```cpp
namespace NS::RHI
{
    /// 待機オブジェクト操作（RHISwapChainに追加）。
    class IRHISwapChain
    {
    public:
        /// フレームレイテンシ待機オブジェクト取得
        /// ERHISwapChainFlags::FrameLatencyWaitableObjectが必要。
        virtual void* GetFrameLatencyWaitableObject() const = 0;

        /// フレームレイテンシ設定
        /// @param maxLatency 最大許容レイテンシ（フレーム数）。
        virtual void SetMaximumFrameLatency(uint32 maxLatency) = 0;

        /// 現在のフレームレイテンシ取得
        virtual uint32 GetCurrentFrameLatency() const = 0;

        /// フレーム待機（CPU側）。
        /// 次のバックバッファが利用可能になるまで待つ。
        virtual bool WaitForNextFrame(uint64 timeoutMs = UINT64_MAX) = 0;
    };
}
```

- [ ] GetFrameLatencyWaitableObject
- [ ] SetMaximumFrameLatency
- [ ] WaitForNextFrame

## 検証方法

- [ ] スワップチェーン作成/破棄
- [ ] バックバッファアクセス
- [ ] リサイズ処理
- [ ] フルスクリーン切り替え

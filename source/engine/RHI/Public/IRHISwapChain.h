/// @file IRHISwapChain.h
/// @brief スワップチェーンインターフェース
/// @details スワップチェーン記述、バックバッファ管理、Present操作、フルスクリーン、イベントを提供。
/// @see 12-01-swapchain-desc.md, 12-02-swapchain-interface.md, 12-03-present.md
#pragma once

#include "Common/Assert/Assert.h"
#include "IRHIResource.h"
#include "RHIEnums.h"
#include "RHIMacros.h"
#include "RHIPixelFormat.h"
#include "RHIRefCountPtr.h"
#include "RHIResourceType.h"
#include "RHITypes.h"

namespace NS
{
    namespace RHI
    {
        class IRHIDevice;
        class IRHIQueue;
        class IRHITexture;
        class IRHIRenderTargetView;

        //=========================================================================
        // ERHISwapChainFlags (12-01)
        //=========================================================================

        /// スワップチェーンフラグ
        enum class ERHISwapChainFlags : uint32
        {
            None = 0,
            AllowTearing = 1 << 0,               ///< ティアリング許可
            FrameLatencyWaitableObject = 1 << 1, ///< フレームレイテンシ待機オブジェクト
            AllowModeSwitch = 1 << 2,            ///< モード切り替え許可
            Stereo = 1 << 3,                     ///< ステレオスコピック3D
            GDICompatible = 1 << 4,              ///< GDI互換
            RestrictedContent = 1 << 5,          ///< 制限出力
            SharedResourceDriver = 1 << 6,       ///< 共有リソースドライバ
            YUVSwapChain = 1 << 7,               ///< YUVスワップチェーン
            HDR = 1 << 8,                        ///< HDR出力
        };
        RHI_ENUM_CLASS_FLAGS(ERHISwapChainFlags)

        //=========================================================================
        // ERHIPresentMode / ERHIScalingMode / ERHIAlphaMode (12-01)
        //=========================================================================

        /// プレゼントモード
        enum class ERHIPresentMode : uint8
        {
            Immediate,           ///< 即時（ティアリングあり）
            VSync,               ///< 垂直同期
            VariableRefreshRate, ///< FreeSync/G-Sync
            Mailbox,             ///< メールボックス（最新フレームのみ保持）
            FIFO,                ///< キュー方式
        };

        /// スケーリングモード
        enum class ERHIScalingMode : uint8
        {
            Stretch,            ///< ストレッチ
            AspectRatioStretch, ///< アスペクト比維持
            None,               ///< 1:1マッピング
        };

        /// アルファモード
        enum class ERHIAlphaMode : uint8
        {
            Ignore,        ///< 無視
            Premultiplied, ///< 事前乗算済み
            Straight,      ///< 直接
        };

        //=========================================================================
        // RHISwapChainDesc (12-01)
        //=========================================================================

        /// スワップチェーン記述
        struct RHI_API RHISwapChainDesc
        {
            void* windowHandle = nullptr;
            uint32 width = 0;
            uint32 height = 0;
            ERHIPixelFormat format = ERHIPixelFormat::R8G8B8A8_UNORM;
            uint32 bufferCount = 2;
            ERHIPresentMode presentMode = ERHIPresentMode::VSync;
            ERHISwapChainFlags flags = ERHISwapChainFlags::None;
            ERHIScalingMode scalingMode = ERHIScalingMode::Stretch;
            ERHIAlphaMode alphaMode = ERHIAlphaMode::Ignore;
            uint32 sampleCount = 1;
            uint32 sampleQuality = 0;
            bool fullscreen = false;
            void* outputMonitor = nullptr;

            //=====================================================================
            // ビルダー
            //=====================================================================

            static RHISwapChainDesc ForWindow(void* hwnd, uint32 w = 0, uint32 h = 0, uint32 buffers = 2)
            {
                RHISwapChainDesc desc;
                desc.windowHandle = hwnd;
                desc.width = w;
                desc.height = h;
                desc.bufferCount = buffers;
                return desc;
            }

            RHISwapChainDesc& EnableHDR()
            {
                format = ERHIPixelFormat::R10G10B10A2_UNORM;
                flags = flags | ERHISwapChainFlags::HDR;
                return *this;
            }

            RHISwapChainDesc& EnableTearing()
            {
                flags = flags | ERHISwapChainFlags::AllowTearing;
                presentMode = ERHIPresentMode::Immediate;
                return *this;
            }

            RHISwapChainDesc& TripleBuffering()
            {
                bufferCount = 3;
                return *this;
            }
        };

        //=========================================================================
        // RHIDisplayMode / RHIFullscreenDesc (12-01)
        //=========================================================================

        /// ディスプレイモード
        struct RHI_API RHIDisplayMode
        {
            uint32 width = 0;
            uint32 height = 0;
            uint32 refreshRateNumerator = 60;
            uint32 refreshRateDenominator = 1;
            ERHIPixelFormat format = ERHIPixelFormat::R8G8B8A8_UNORM;

            enum class ScanlineOrder : uint8
            {
                Unspecified,
                Progressive,
                UpperFieldFirst,
                LowerFieldFirst,
            };
            ScanlineOrder scanlineOrder = ScanlineOrder::Progressive;

            ERHIScalingMode scaling = ERHIScalingMode::Stretch;

            float GetRefreshRateHz() const { return static_cast<float>(refreshRateNumerator) / refreshRateDenominator; }
        };

        /// フルスクリーン記述
        struct RHI_API RHIFullscreenDesc
        {
            RHIDisplayMode displayMode;
            bool exclusiveFullscreen = false;
            bool allowModeSwitch = true;
            bool autoAltEnter = true;
        };

        //=========================================================================
        // RHIOutputInfo (12-01)
        //=========================================================================

        /// 出力（モニター）情報
        struct RHI_API RHIOutputInfo
        {
            char name[256] = {};
            int32 desktopX = 0;
            int32 desktopY = 0;
            uint32 width = 0;
            uint32 height = 0;
            bool isPrimary = false;
            bool supportsHDR = false;
            float currentRefreshRate = 60.0f;
            bool supportsVariableRefreshRate = false;
        };

        //=========================================================================
        // ERHIPresentFlags / RHIPresentParams (12-03)
        //=========================================================================

        /// Presentフラグ
        enum class ERHIPresentFlags : uint32
        {
            None = 0,
            Test = 1 << 0,              ///< テストのみ
            DoNotWait = 1 << 1,         ///< 垂直ブランク待機しない
            RestartFrame = 1 << 2,      ///< フレーム破棄
            AllowTearing = 1 << 3,      ///< ティアリング許可
            StereoPreferRight = 1 << 4, ///< ステレオ右目優先
            UseDirtyRects = 1 << 5,     ///< ダーティリージョン使用
            UseScrollRect = 1 << 6,     ///< スクロール使用
        };
        RHI_ENUM_CLASS_FLAGS(ERHIPresentFlags)

        /// ダーティリージョン
        struct RHI_API RHIDirtyRect
        {
            int32 left = 0;
            int32 top = 0;
            int32 right = 0;
            int32 bottom = 0;
        };

        /// スクロールリージョン
        struct RHI_API RHIScrollRect
        {
            RHIDirtyRect source;
            int32 offsetX = 0;
            int32 offsetY = 0;
        };

        /// Presentパラメータ
        struct RHI_API RHIPresentParams
        {
            ERHIPresentFlags flags = ERHIPresentFlags::None;
            uint32 syncInterval = 1; ///< 0:即時, 1:VSync, 2-4:複数VSync
            const RHIDirtyRect* dirtyRects = nullptr;
            uint32 dirtyRectCount = 0;
            const RHIScrollRect* scrollRect = nullptr;
        };

        //=========================================================================
        // ERHIPresentResult (12-03)
        //=========================================================================

        /// Present結果
        enum class ERHIPresentResult : uint8
        {
            Success,      ///< 成功
            Occluded,     ///< オクルード（最小化等）
            DeviceReset,  ///< デバイスリセット（リソース再作成必要）
            DeviceLost,   ///< デバイスロスト（デバイス再作成必要）
            FrameSkipped, ///< フレームスキップ
            Timeout,      ///< タイムアウト
            Error,        ///< エラー
        };

        //=========================================================================
        // RHIFrameStatistics (12-03)
        //=========================================================================

        /// フレーム統計
        struct RHI_API RHIFrameStatistics
        {
            uint64 presentCount = 0;
            uint64 presentRefreshCount = 0;
            uint64 syncRefreshCount = 0;
            uint64 syncQPCTime = 0;
            uint64 syncGPUTime = 0;
            uint64 frameNumber = 0;
        };

        //=========================================================================
        // ERHISwapChainEvent (12-02)
        //=========================================================================

        /// スワップチェーンイベントタイプ
        enum class ERHISwapChainEvent : uint8
        {
            BackBufferAvailable, ///< バックバッファ利用可能
            ResizeNeeded,        ///< リサイズ必要
            FullscreenChanged,   ///< フルスクリーン状態変更
            HDRChanged,          ///< HDR状態変更
            DeviceLost,          ///< デバイスロスト
        };

        /// スワップチェーンイベントコールバック
        using RHISwapChainEventCallback = void (*)(IRHISwapChain* swapChain, ERHISwapChainEvent event, void* userData);

        //=========================================================================
        // RHISwapChainResizeDesc (12-02)
        //=========================================================================

        /// リサイズ記述
        struct RHI_API RHISwapChainResizeDesc
        {
            uint32 width = 0;                                  ///< 新しい幅（0で現在サイズ）
            uint32 height = 0;                                 ///< 新しい高さ
            ERHIPixelFormat format = ERHIPixelFormat::Unknown; ///< 新フォーマット（Unknownで変更なし）
            uint32 bufferCount = 0;                            ///< 新バッファ数（0で変更なし）
            ERHISwapChainFlags flags = ERHISwapChainFlags::None;
        };

        //=========================================================================
        // IRHISwapChain (12-01〜12-03)
        //=========================================================================

        /// スワップチェーンインターフェース
        class RHI_API IRHISwapChain : public IRHIResource
        {
        public:
            DECLARE_RHI_RESOURCE_TYPE(SwapChain)

            virtual ~IRHISwapChain() = default;

        protected:
            IRHISwapChain() : IRHIResource(ERHIResourceType::SwapChain) {}

        public:
            //=====================================================================
            // 基本プロパティ (12-02)
            //=====================================================================

            virtual IRHIDevice* GetDevice() const = 0;
            virtual uint32 GetWidth() const = 0;
            virtual uint32 GetHeight() const = 0;
            virtual ERHIPixelFormat GetFormat() const = 0;
            virtual uint32 GetBufferCount() const = 0;
            virtual ERHIPresentMode GetPresentMode() const = 0;
            virtual ERHISwapChainFlags GetFlags() const = 0;

            //=====================================================================
            // バックバッファ (12-02)
            //=====================================================================

            virtual uint32 GetCurrentBackBufferIndex() const = 0;
            virtual IRHITexture* GetBackBuffer(uint32 index) const = 0;
            virtual IRHIRenderTargetView* GetBackBufferRTV(uint32 index) const = 0;

            IRHITexture* GetCurrentBackBuffer() const { return GetBackBuffer(GetCurrentBackBufferIndex()); }

            IRHIRenderTargetView* GetCurrentBackBufferRTV() const
            {
                return GetBackBufferRTV(GetCurrentBackBufferIndex());
            }

            //=====================================================================
            // フルスクリーン (12-02)
            //=====================================================================

            virtual bool IsFullscreen() const = 0;
            virtual bool SetFullscreen(bool fullscreen, const RHIFullscreenDesc* desc = nullptr) = 0;

            //=====================================================================
            // 状態 (12-02)
            //=====================================================================

            virtual bool IsOccluded() const = 0;
            virtual bool IsHDREnabled() const = 0;
            virtual bool IsVariableRefreshRateEnabled() const = 0;

            //=====================================================================
            // リサイズ (12-02)
            //=====================================================================

            /// リサイズ
            /// @pre 全バックバッファの外部参照が解放済みであること
            /// @pre GPU完了済みであること
            virtual bool Resize(const RHISwapChainResizeDesc& desc) = 0;

            bool Resize(uint32 width, uint32 height)
            {
                RHISwapChainResizeDesc desc;
                desc.width = width;
                desc.height = height;
                return Resize(desc);
            }

            //=====================================================================
            // イベント (12-02)
            //=====================================================================

            virtual void SetEventCallback(RHISwapChainEventCallback callback, void* userData = nullptr) = 0;

            /// ウィンドウメッセージ処理（Win32用）
            virtual bool ProcessWindowMessage(void* hwnd, uint32 message, uint64 wParam, int64 lParam) = 0;

            //=====================================================================
            // フレームレイテンシ (12-02)
            //=====================================================================

            virtual void* GetFrameLatencyWaitableObject() const = 0;
            virtual void SetMaximumFrameLatency(uint32 maxLatency) = 0;
            virtual uint32 GetCurrentFrameLatency() const = 0;
            virtual bool WaitForNextFrame(uint64 timeoutMs = UINT64_MAX) = 0;

            //=====================================================================
            // Present (12-03)
            //=====================================================================

            virtual ERHIPresentResult Present(const RHIPresentParams& params = {}) = 0;

            ERHIPresentResult Present(uint32 syncInterval)
            {
                RHIPresentParams params;
                params.syncInterval = syncInterval;
                return Present(params);
            }

            ERHIPresentResult PresentVSync() { return Present(1); }

            ERHIPresentResult PresentImmediate()
            {
                NS_ASSERT(EnumHasAnyFlags(GetFlags(), ERHISwapChainFlags::AllowTearing),
                          "PresentImmediate requires AllowTearing flag");
                RHIPresentParams params;
                params.syncInterval = 0;
                params.flags = ERHIPresentFlags::AllowTearing;
                return Present(params);
            }

            //=====================================================================
            // Present統計 (12-03)
            //=====================================================================

            virtual bool GetFrameStatistics(RHIFrameStatistics& outStats) const = 0;
            virtual uint64 GetLastPresentId() const = 0;
            virtual bool WaitForPresentCompletion(uint64 presentId, uint64 timeoutMs = UINT64_MAX) = 0;

            //=====================================================================
            // 構成変更Present (12-03)
            //=====================================================================

            virtual ERHIPresentResult PresentAndResize(uint32 width,
                                                       uint32 height,
                                                       ERHIPixelFormat format = ERHIPixelFormat::Unknown,
                                                       ERHISwapChainFlags flags = ERHISwapChainFlags::None) = 0;

            //=====================================================================
            // HDR (12-04, 定義はRHIHDR.hに依存)
            //=====================================================================

            /// カラースペース設定 (12-04)
            virtual bool SetColorSpace(uint8 colorSpace) = 0;

            /// カラースペース取得 (12-04)
            virtual uint8 GetColorSpace() const = 0;

            /// HDR有効/無効 (12-04)
            virtual bool SetHDREnabled(bool enabled) = 0;

            /// HDR自動切り替え (12-04)
            virtual void SetHDRAutoSwitch(bool enabled) = 0;

            /// Auto HDRサポート (12-04)
            virtual bool SupportsAutoHDR() const = 0;
        };

        using RHISwapChainRef = TRefCountPtr<IRHISwapChain>;

        //=========================================================================
        // RHIMultiSwapChainPresenter (12-03)
        //=========================================================================

        /// 複数スワップチェーン同期Presenter
        class RHI_API RHIMultiSwapChainPresenter
        {
        public:
            RHIMultiSwapChainPresenter() = default;

            bool Initialize(IRHIDevice* device);
            void Shutdown();

            //=====================================================================
            // スワップチェーン管理
            //=====================================================================

            void AddSwapChain(IRHISwapChain* swapChain);
            void RemoveSwapChain(IRHISwapChain* swapChain);
            void ClearSwapChains();

            //=====================================================================
            // 同期Present
            //=====================================================================

            void PresentAll(uint32 syncInterval = 1);
            void Present(IRHISwapChain* swapChain, uint32 syncInterval = 1);

            uint32 GetSwapChainCount() const { return m_swapChainCount; }

        private:
            IRHIDevice* m_device = nullptr;
            IRHISwapChain** m_swapChains = nullptr;
            uint32 m_swapChainCount = 0;
            uint32 m_swapChainCapacity = 0;
        };

    } // namespace RHI
} // namespace NS

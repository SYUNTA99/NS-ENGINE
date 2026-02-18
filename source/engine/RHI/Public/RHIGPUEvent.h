/// @file RHIGPUEvent.h
/// @brief GPUイベントマーカーとブレッドクラム
/// @details イベントマーカー、ブレッドクラム、デバイスロスト診断、プロファイラ統合を提供。
/// @see 09-03-gpu-event.md
#pragma once

#include "IRHIBuffer.h"
#include "IRHICommandContextBase.h"
#include "RHIMacros.h"
#include "RHIRefCountPtr.h"
#include "RHITypes.h"

namespace NS { namespace RHI {
    //=========================================================================
    // RHIEventColor (09-03)
    //=========================================================================

    /// GPUイベントカラー
    struct RHI_API RHIEventColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        static RHIEventColor Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
        static RHIEventColor Green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
        static RHIEventColor Blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
        static RHIEventColor Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
        static RHIEventColor Cyan() { return {0.0f, 1.0f, 1.0f, 1.0f}; }
        static RHIEventColor Purple() { return {1.0f, 0.0f, 1.0f, 1.0f}; }
        static RHIEventColor Orange() { return {1.0f, 0.5f, 0.0f, 1.0f}; }
        static RHIEventColor White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
        static RHIEventColor Gray() { return {0.5f, 0.5f, 0.5f, 1.0f}; }

        /// 32bit RGBA取得
        uint32 ToRGBA() const
        {
            uint32 ir = static_cast<uint32>(r * 255.0f);
            uint32 ig = static_cast<uint32>(g * 255.0f);
            uint32 ib = static_cast<uint32>(b * 255.0f);
            uint32 ia = static_cast<uint32>(a * 255.0f);
            return (ir << 24) | (ig << 16) | (ib << 8) | ia;
        }
    };

    //=========================================================================
    // RHIScopedEvent (09-03)
    //=========================================================================

    /// スコープイベント（RAII）
    class RHI_API RHIScopedEvent
    {
    public:
        RHIScopedEvent(IRHICommandContextBase* context,
                       const char* name,
                       const RHIEventColor& color = RHIEventColor::White())
            : m_context(context)
        {
            if (m_context)
            {
                m_context->BeginDebugEvent(name, color.ToRGBA());
            }
        }

        ~RHIScopedEvent()
        {
            if (m_context)
            {
                m_context->EndDebugEvent();
            }
        }

        NS_DISALLOW_COPY(RHIScopedEvent);

    private:
        IRHICommandContextBase* m_context;
    };

/// スコープイベントマクロ
#define RHI_SCOPED_EVENT(context, name)                                                                                \
    ::NS::RHI::RHIScopedEvent NS_MACRO_CONCATENATE(_rhiEvent, __LINE__)(context, name)

#define RHI_SCOPED_EVENT_COLOR(context, name, color)                                                                   \
    ::NS::RHI::RHIScopedEvent NS_MACRO_CONCATENATE(_rhiEvent, __LINE__)(context, name, color)

/// フォーマット付きイベント
#define RHI_SCOPED_EVENT_F(context, format, ...)                                                                       \
    char NS_MACRO_CONCATENATE(_eventName, __LINE__)[256];                                                              \
    snprintf(NS_MACRO_CONCATENATE(_eventName, __LINE__),                                                               \
             sizeof(NS_MACRO_CONCATENATE(_eventName, __LINE__)),                                                       \
             format,                                                                                                   \
             __VA_ARGS__);                                                                                             \
    ::NS::RHI::RHIScopedEvent NS_MACRO_CONCATENATE(_rhiEvent, __LINE__)(context,                                       \
                                                                        NS_MACRO_CONCATENATE(_eventName, __LINE__))

    //=========================================================================
    // RHIBreadcrumbEntry / RHIBreadcrumbBuffer (09-03)
    //=========================================================================

    /// ブレッドクラムエントリ
    struct RHI_API RHIBreadcrumbEntry
    {
        uint32 id = 0;                 ///< ID
        const char* message = nullptr; ///< メッセージ（オプション）
        uint64 timestamp = 0;          ///< タイムスタンプ（オプション）
    };

    /// ブレッドクラムバッファ
    /// GPUクラッシュ診断用のバッファ
    class RHI_API RHIBreadcrumbBuffer
    {
    public:
        static constexpr uint32 kMaxEntries = 256;

        RHIBreadcrumbBuffer() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        /// GPUバッファ取得
        IRHIBuffer* GetBuffer() const { return m_buffer.Get(); }

        /// エントリ読み取り（GPUクラッシュ後に呼び出す）
        bool ReadEntries(RHIBreadcrumbEntry* outEntries, uint32& outCount);

        /// 最後に書き込まれたインデックス取得
        uint32 GetLastWrittenIndex() const;

        /// リセット
        void Reset();

    private:
        IRHIDevice* m_device = nullptr;
        RHIBufferRef m_buffer;
        RHIBufferRef m_readbackBuffer;
    };

    //=========================================================================
    // ERHIGPUCrashReason / RHIGPUCrashInfo (09-03)
    //=========================================================================

    /// GPUクラッシュ原因
    enum class ERHIGPUCrashReason : uint8
    {
        Unknown,
        HangTimeout,      ///< タイムアウト
        PageFault,        ///< ページフォールト
        TDRRecovery,      ///< TDR回復
        DriverError,      ///< ドライバエラー
        OutOfMemory,      ///< メモリ不足
        InvalidOperation, ///< 無効な操作
    };

    /// GPUクラッシュ情報
    struct RHI_API RHIGPUCrashInfo
    {
        ERHIGPUCrashReason reason = ERHIGPUCrashReason::Unknown;
        const char* message = nullptr;               ///< 詳細メッセージ
        uint32 lastBreadcrumbId = 0;                 ///< 最後のブレッドクラムID
        const char* lastBreadcrumbMessage = nullptr; ///< 最後のブレッドクラムメッセージ
        uint64 faultAddress = 0;                     ///< フォールトしたGPUアドレス
        const void* additionalData = nullptr;        ///< 追加データ
        uint32 additionalDataSize = 0;
    };

    /// デバイスロストコールバック
    using RHIDeviceLostCallback = void (*)(const RHIGPUCrashInfo& info, void* userData);

    //=========================================================================
    // ERHIProfilerType / RHIProfilerConfig (09-03)
    //=========================================================================

    /// プロファイラタイプ
    enum class ERHIProfilerType : uint8
    {
        None,
        PIX,
        RenderDoc,
        NSight,
        Internal,
    };

    /// プロファイラ設定
    struct RHI_API RHIProfilerConfig
    {
        ERHIProfilerType profilerType = ERHIProfilerType::None;
        const char* capturePath = nullptr; ///< キャプチャパス
        bool autoCapture = false;          ///< 自動キャプチャ
        uint32 captureFrameCount = 1;      ///< キャプチャフレーム数
    };

}} // namespace NS::RHI

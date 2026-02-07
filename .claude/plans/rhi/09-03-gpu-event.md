# 09-03: GPUイベント

## 目的

GPUタイムラインでのイベントマーカーとブレッドクラム機能を定義する。

## 参照ドキュメント

- 02-01-command-context-base.md (IRHICommandContext)
- 09-01-fence-interface.md (IRHIFence)

## 変更対象ファイル

新規作成:
- `Source/Engine/RHI/Public/RHIGPUEvent.h`

## TODO

### 1. GPUイベントマーカー

```cpp
#pragma once

#include "RHITypes.h"

namespace NS::RHI
{
    /// GPUイベントカラー
    struct RHI_API RHIEventColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        /// プリセットカラー
        static RHIEventColor Red()    { return {1.0f, 0.0f, 0.0f, 1.0f}; }
        static RHIEventColor Green()  { return {0.0f, 1.0f, 0.0f, 1.0f}; }
        static RHIEventColor Blue()   { return {0.0f, 0.0f, 1.0f, 1.0f}; }
        static RHIEventColor Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
        static RHIEventColor Cyan()   { return {0.0f, 1.0f, 1.0f, 1.0f}; }
        static RHIEventColor Purple() { return {1.0f, 0.0f, 1.0f, 1.0f}; }
        static RHIEventColor Orange() { return {1.0f, 0.5f, 0.0f, 1.0f}; }
        static RHIEventColor White()  { return {1.0f, 1.0f, 1.0f, 1.0f}; }
        static RHIEventColor Gray()   { return {0.5f, 0.5f, 0.5f, 1.0f}; }

        /// 32bit RGBA取得
        uint32 ToRGBA() const {
            uint32 ir = static_cast<uint32>(r * 255.0f);
            uint32 ig = static_cast<uint32>(g * 255.0f);
            uint32 ib = static_cast<uint32>(b * 255.0f);
            uint32 ia = static_cast<uint32>(a * 255.0f);
            return (ir << 24) | (ig << 16) | (ib << 8) | ia;
        }
    };
}
```

- [ ] RHIEventColor 構造体

### 2. コマンドコンテキストイベント操作

```cpp
namespace NS::RHI
{
    /// GPUイベント操作（RHICommandContextに追加）。
    class IRHICommandContext
    {
    public:
        //=====================================================================
        // イベントマーカー
        //=====================================================================

        /// イベント開始
        /// PIX、RenderDoc等のプロファイラで表示
        virtual void BeginEvent(const char* name, const RHIEventColor& color = RHIEventColor::White()) = 0;

        /// イベント終了
        virtual void EndEvent() = 0;

        /// インスタントマーカー
        /// 単一タイムスタンプでのマーカー
        virtual void SetMarker(const char* name, const RHIEventColor& color = RHIEventColor::White()) = 0;

        //=====================================================================
        // ブレッドクラム
        //=====================================================================

        /// ブレッドクラム挿入
        /// GPUクラッシュ時の診断用
        virtual void InsertBreadcrumb(uint32 id, const char* message = nullptr) = 0;
    };
}
```

- [ ] BeginEvent/EndEvent
- [ ] SetMarker
- [ ] InsertBreadcrumb

### 3. スコープイベントヘルパー。

```cpp
namespace NS::RHI
{
    /// スコープイベント（RAII）。
    class RHI_API RHIScopedEvent
    {
    public:
        RHIScopedEvent(IRHICommandContext* context, const char* name,
                       const RHIEventColor& color = RHIEventColor::White())
            : m_context(context)
        {
            if (m_context) {
                m_context->BeginEvent(name, color);
            }
        }

        ~RHIScopedEvent() {
            if (m_context) {
                m_context->EndEvent();
            }
        }

        NS_DISALLOW_COPY(RHIScopedEvent);
        IRHICommandContext* m_context;
    };

    /// スコープイベントのクロ
    #define RHI_SCOPED_EVENT(context, name) \
        NS::RHI::RHIScopedEvent NS_MACRO_CONCATENATE(_rhiEvent, __LINE__)(context, name)

    #define RHI_SCOPED_EVENT_COLOR(context, name, color) \
        NS::RHI::RHIScopedEvent NS_MACRO_CONCATENATE(_rhiEvent, __LINE__)(context, name, color)

    /// フォーマット付きイベント
    #define RHI_SCOPED_EVENT_F(context, format, ...) \
        char NS_MACRO_CONCATENATE(_eventName, __LINE__)[256]; \
        snprintf(NS_MACRO_CONCATENATE(_eventName, __LINE__), sizeof(NS_MACRO_CONCATENATE(_eventName, __LINE__)), format, __VA_ARGS__); \
        NS::RHI::RHIScopedEvent NS_MACRO_CONCATENATE(_rhiEvent, __LINE__)(context, NS_MACRO_CONCATENATE(_eventName, __LINE__))
}
```

- [ ] RHIScopedEvent クラス
- [ ] マクロ定義

### 4. ブレッドクラムバッファ

```cpp
namespace NS::RHI
{
    /// ブレッドクラムエントリ
    struct RHI_API RHIBreadcrumbEntry
    {
        /// ID
        uint32 id = 0;

        /// メッセージ（オプション）。
        const char* message = nullptr;

        /// タイムスタンプ（オプション）。
        uint64 timestamp = 0;
    };

    /// ブレッドクラムバッファ
    /// GPUクラッシュ診断用のバッファ
    class RHI_API RHIBreadcrumbBuffer
    {
    public:
        /// 最大エントリ数
        static constexpr uint32 kMaxEntries = 256;

        RHIBreadcrumbBuffer() = default;

        /// 初期化
        bool Initialize(IRHIDevice* device);

        /// シャットダウン
        void Shutdown();

        /// GPUバッファ取得
        IRHIBuffer* GetBuffer() const { return m_buffer; }

        /// エントリ読み取り
        /// GPUクラッシュ後に呼び出す
        /// @param outEntries 出力エントリ配列
        /// @param outCount 出力エントリ数
        /// @return 読み取りに成功したか
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
}
```

- [ ] RHIBreadcrumbEntry 構造体
- [ ] RHIBreadcrumbBuffer クラス

### 5. デバイスロスト診断

```cpp
namespace NS::RHI
{
    /// GPUクラッシュ原因
    enum class ERHIGPUCrashReason : uint8
    {
        Unknown,
        HangTimeout,        // タイムアウト
        PageFault,          // ページフォールト
        TDRRecovery,        // TDR回復
        DriverError,        // ドライバのエラー
        OutOfMemory,        // メモリ不足
        InvalidOperation,   // 無効な操作
    };

    /// GPUクラッシュ情報
    struct RHI_API RHIGPUCrashInfo
    {
        /// 原因
        ERHIGPUCrashReason reason = ERHIGPUCrashReason::Unknown;

        /// 詳細メッセージ
        const char* message = nullptr;

        /// 最後のブレッドクラムID
        uint32 lastBreadcrumbId = 0;

        /// 最後のブレッドクラムメッセージ
        const char* lastBreadcrumbMessage = nullptr;

        /// フォールトしたGPUアドレス（ページフォールト時）。
        uint64 faultAddress = 0;

        /// 追加データ
        const void* additionalData = nullptr;
        uint32 additionalDataSize = 0;
    };

    /// デバイスロストコールバック
    using RHIDeviceLostCallback = void(*)(const RHIGPUCrashInfo& info, void* userData);

    /// デバイスロストリスナの設定（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// デバイスロストコールバック設定
        virtual void SetDeviceLostCallback(
            RHIDeviceLostCallback callback,
            void* userData = nullptr) = 0;

        /// GPUクラッシュ情報を取得
        /// デバイスロスト後に呼び出す
        virtual bool GetGPUCrashInfo(RHIGPUCrashInfo& outInfo) = 0;

        /// ブレッドクラムバッファ設定
        virtual void SetBreadcrumbBuffer(RHIBreadcrumbBuffer* buffer) = 0;
    };
}
```

- [ ] ERHIGPUCrashReason 列挙型
- [ ] RHIGPUCrashInfo 構造体
- [ ] デバイスロストコールバック

### 6. プロファイリング統合

```cpp
namespace NS::RHI
{
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
        /// 有効なプロファイラ
        ERHIProfilerType profilerType = ERHIProfilerType::None;

        /// キャプチャパス
        const char* capturePath = nullptr;

        /// 自動キャプチャ
        bool autoCapture = false;

        /// キャプチャフレーム数（AutoCapture時）
        uint32 captureFrameCount = 1;
    };

    /// プロファイラ統合（RHIDeviceに追加）。
    class IRHIDevice
    {
    public:
        /// プロファイラ設定
        virtual void ConfigureProfiler(const RHIProfilerConfig& config) = 0;

        /// キャプチャ開始
        virtual void BeginCapture(const char* captureName = nullptr) = 0;

        /// キャプチャ終了
        virtual void EndCapture() = 0;

        /// キャプチャ中か
        virtual bool IsCapturing() const = 0;

        /// 利用可能なプロファイラ取得
        virtual ERHIProfilerType GetAvailableProfiler() const = 0;
    };
}
```

- [ ] ERHIProfilerType 列挙型
- [ ] RHIProfilerConfig 構造体
- [ ] プロファイラ操作

## 検証方法

- [ ] イベントマーカーの表示
- [ ] ブレッドクラムの記録/読み取り
- [ ] デバイスロスト検出
- [ ] プロファイラ統合

/// @file ApplicationCoreTypes.h
/// @brief ApplicationCoreモジュールの基本型定義
#pragma once

#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace NS
{
    // =========================================================================
    // WindowMode
    // =========================================================================

    /// ウィンドウモード（namespace + Type enum パターン）
    namespace WindowMode
    {
        enum Type : int32_t
        {
            /// 排他的フルスクリーン
            Fullscreen = 0,
            /// ボーダーレスフルスクリーン
            WindowedFullscreen = 1,
            /// 通常ウィンドウ
            Windowed = 2,
            /// モード数
            NumWindowModes = 3
        };

        /// int32 をWindowMode::Typeに変換（範囲外はWindowed）
        inline Type ConvertIntToWindowMode(int32_t inWindowMode)
        {
            if (inWindowMode < 0 || inWindowMode >= NumWindowModes)
            {
                return Windowed;
            }
            return static_cast<Type>(inWindowMode);
        }
    } // namespace WindowMode

    // =========================================================================
    // WindowType
    // =========================================================================

    /// ウィンドウの種別
    enum class WindowType : uint8_t
    {
        /// 標準ウィンドウ
        Normal,
        /// メニューウィンドウ
        Menu,
        /// ツールチップ
        ToolTip,
        /// 通知ウィンドウ
        Notification,
        /// カーソル装飾
        CursorDecorator,
        /// ゲーム専用ウィンドウ
        GameWindow
    };

    // =========================================================================
    // WindowTransparency
    // =========================================================================

    /// ウィンドウ透過モード
    enum class WindowTransparency : uint8_t
    {
        /// 不透明
        None,
        /// ウィンドウ単位透過
        PerWindow,
        /// ピクセル単位透過（ALPHA_BLENDED_WINDOWS必須）
        PerPixel
    };

    // =========================================================================
    // WindowActivationPolicy
    // =========================================================================

    /// ウィンドウのアクティベーションポリシー
    enum class WindowActivationPolicy : uint8_t
    {
        /// アクティベートしない
        Never,
        /// 常にアクティベート
        Always,
        /// 初回表示時のみアクティベート
        FirstShown
    };

    // =========================================================================
    // WindowTitleAlignment / PopUpOrientation
    // =========================================================================

    /// ウィンドウタイトルの配置
    enum class WindowTitleAlignment : uint8_t
    {
        Left,
        Center,
        Right
    };

    /// ポップアップの表示方向
    enum class PopUpOrientation : uint8_t
    {
        Horizontal,
        Vertical
    };

    // =========================================================================
    // WindowActivation
    // =========================================================================

    /// ウィンドウアクティベーション状態
    namespace WindowActivation
    {
        enum Type : uint8_t
        {
            /// キーボード等による通常アクティベート
            Activate,
            /// マウスクリックによるアクティベート
            ActivateByMouse,
            /// 非アクティブ化
            Deactivate
        };
    } // namespace WindowActivation

    // =========================================================================
    // WindowZone
    // =========================================================================

    /// ウィンドウ領域（ヒットテスト結果）
    namespace WindowZone
    {
        enum Type : uint8_t
        {
            NotInWindow = 0,
            TopLeftBorder,
            TopBorder,
            TopRightBorder,
            LeftBorder,
            ClientArea,
            RightBorder,
            BottomLeftBorder,
            BottomBorder,
            BottomRightBorder,
            TitleBar,
            MinimizeButton,
            MaximizeButton,
            CloseButton,
            SysMenu,
            Unspecified
        };
    } // namespace WindowZone

    // =========================================================================
    // WindowDrawAttention
    // =========================================================================

    /// ウィンドウ注目要求タイプ
    enum class WindowDrawAttentionRequestType : uint8_t
    {
        /// ユーザーがアクティベートするまで点滅
        UntilActivated,
        /// 点滅停止
        Stop
    };

    /// ウィンドウ注目要求パラメータ
    struct WindowDrawAttentionParameters
    {
        WindowDrawAttentionRequestType requestType;

        WindowDrawAttentionParameters() : requestType(WindowDrawAttentionRequestType::UntilActivated) {}

        explicit WindowDrawAttentionParameters(WindowDrawAttentionRequestType inRequestType)
            : requestType(inRequestType)
        {}
    };

    // =========================================================================
    // ScreenPhysicalAccuracy
    // =========================================================================

    /// 物理スクリーンサイズの精度
    enum class ScreenPhysicalAccuracy : uint8_t
    {
        /// 物理サイズ不明
        Unknown,
        /// 近似値
        Approximation,
        /// 正確な値
        Truth
    };

    // =========================================================================
    // WindowAction
    // =========================================================================

    /// ウィンドウアクション（MessageHandler::OnWindowAction で使用）
    enum class WindowAction : uint8_t
    {
        ClickedNonClientArea,
        Maximize,
        Restore,
        WindowMenu
    };

    // =========================================================================
    // AccessibleBehavior
    // =========================================================================

    /// アクセシビリティ動作モード
    enum class AccessibleBehavior : uint8_t
    {
        NotAccessible,
        Auto,
        Summary,
        Custom,
        ToolTip
    };

    // =========================================================================
    // AccessibleWidgetType (14-01)
    // =========================================================================

#ifndef WITH_ACCESSIBILITY
#define WITH_ACCESSIBILITY 0
#endif

#if WITH_ACCESSIBILITY
    /// アクセシブルウィジェットタイプ
    enum class AccessibleWidgetType : uint8_t
    {
        Unknown,
        Button,
        CheckBox,
        ComboBox,
        Hyperlink,
        Image,
        Layout,
        ScrollBar,
        Slider,
        Text,
        TextEdit,
        Window,
        List,
        ListItem
    };

    /// アクセシビリティイベント種別
    enum class AccessibleEvent : uint8_t
    {
        FocusChange,
        Activate,
        Notification,
        ParentChanged,
        WidgetRemoved
    };

    /// アクセシブルユーザーインデックス
    using AccessibleUserIndex = int32_t;
#endif // WITH_ACCESSIBILITY

    // =========================================================================
    // InputDeviceConnectionState
    // =========================================================================

    /// 入力デバイス接続状態
    enum class InputDeviceConnectionState : uint8_t
    {
        Connected,
        Disconnected,
        Unknown
    };

    // =========================================================================
    // PlatformRect
    // =========================================================================

    /// プラットフォーム矩形
    struct PlatformRect
    {
        int32_t left   = 0;
        int32_t top    = 0;
        int32_t right  = 0;
        int32_t bottom = 0;

        bool operator==(const PlatformRect& other) const
        {
            return left == other.left && top == other.top
                && right == other.right && bottom == other.bottom;
        }

        bool operator!=(const PlatformRect& other) const
        {
            return !(*this == other);
        }
    };

    // =========================================================================
    // WindowSizeLimits
    // =========================================================================

    /// ウィンドウサイズ制限（Fluent API）
    class WindowSizeLimits
    {
    public:
        WindowSizeLimits& SetMinWidth(std::optional<float> inMinWidth)   { m_minWidth  = inMinWidth;  return *this; }
        WindowSizeLimits& SetMinHeight(std::optional<float> inMinHeight) { m_minHeight = inMinHeight; return *this; }
        WindowSizeLimits& SetMaxWidth(std::optional<float> inMaxWidth)   { m_maxWidth  = inMaxWidth;  return *this; }
        WindowSizeLimits& SetMaxHeight(std::optional<float> inMaxHeight) { m_maxHeight = inMaxHeight; return *this; }

        [[nodiscard]] std::optional<float> GetMinWidth()  const { return m_minWidth; }
        [[nodiscard]] std::optional<float> GetMinHeight() const { return m_minHeight; }
        [[nodiscard]] std::optional<float> GetMaxWidth()  const { return m_maxWidth; }
        [[nodiscard]] std::optional<float> GetMaxHeight() const { return m_maxHeight; }

    private:
        std::optional<float> m_minWidth;
        std::optional<float> m_minHeight;
        std::optional<float> m_maxWidth;
        std::optional<float> m_maxHeight;
    };

    // =========================================================================
    // SharedRef<T>
    // =========================================================================

    /// 非null保証の共有ポインタ
    template<typename T>
    class SharedRef
    {
    public:
        explicit SharedRef(std::shared_ptr<T> inPtr)
            : m_ptr(std::move(inPtr))
        {
            NS_CHECK(m_ptr != nullptr);
        }

        T* operator->() const { return m_ptr.get(); }
        T& operator*()  const { return *m_ptr; }
        [[nodiscard]] T* Get()         const { return m_ptr.get(); }

        [[nodiscard]] std::shared_ptr<T> ToSharedPtr() const { return m_ptr; }

    private:
        std::shared_ptr<T> m_ptr;
    };

    // =========================================================================
    // Vector2D / Vector3D（暫定定義）
    // =========================================================================

#ifndef NS_MATH_VECTOR_H
    /// 2Dベクトル（Mathモジュール実装後に差し替え）
    struct Vector2D
    {
        float x = 0.0F;
        float y = 0.0F;
    };

    /// 3Dベクトル（Mathモジュール実装後に差し替え）
    struct Vector3D
    {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    /// 4Dベクトル（セーフゾーン用）
    struct Vector4
    {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float w = 0.0F;
    };
#endif // NS_MATH_VECTOR_H

} // namespace NS

/// @file InputTypes.h
/// @brief 入力関連の基本型定義
#pragma once

#include <cstdint>

namespace NS
{
    // =========================================================================
    // MouseCursor
    // =========================================================================

    /// マウスカーソル形状
    namespace MouseCursor
    {
        enum Type : uint8_t
        {
            /// 非表示
            None = 0,
            /// 標準矢印
            Default,
            /// テキスト編集Iビーム
            TextEditBeam,
            /// 左右リサイズ
            ResizeLeftRight,
            /// 上下リサイズ
            ResizeUpDown,
            /// 右下リサイズ
            ResizeSouthEast,
            /// 左下リサイズ
            ResizeSouthWest,
            /// 十字移動
            CardinalCross,
            /// 精密十字
            Crosshairs,
            /// リンクハンド
            Hand,
            /// 掴みハンド
            GrabHand,
            /// 掴み中ハンド
            GrabHandClosed,
            /// 禁止マーク
            SlashedCircle,
            /// スポイト
            EyeDropper,
            /// カスタムカーソル
            Custom,
            /// カーソル型の総数
            TotalCursorCount
        };
    } // namespace MouseCursor

    // =========================================================================
    // MouseButtons
    // =========================================================================

    /// マウスボタン
    namespace MouseButtons
    {
        enum Type : uint8_t
        {
            Left = 0,
            Middle,
            Right,
            Thumb01,
            Thumb02,
            /// 無効値
            Invalid
        };
    } // namespace MouseButtons

    // =========================================================================
    // DropEffect
    // =========================================================================

    /// ドラッグ&ドロップ効果
    namespace DropEffect
    {
        enum Type : uint8_t
        {
            /// ドロップ不可
            None = 0,
            /// コピー
            Copy,
            /// 移動
            Move,
            /// リンク作成
            Link
        };
    } // namespace DropEffect

    // =========================================================================
    // GestureEvent
    // =========================================================================

    /// ジェスチャイベントタイプ
    enum class GestureEvent : uint8_t
    {
        Scroll,
        Magnify,
        Swipe,
        Rotate,
        LongPress
    };

    // =========================================================================
    // ScreenSaverAction
    // =========================================================================

    /// スクリーンセーバー制御アクション
    enum class ScreenSaverAction : uint8_t
    {
        Disable,
        Enable
    };

    // =========================================================================
    // INDEX_NONE
    // =========================================================================

    /// 無効なインデックス値
    constexpr int32_t INDEX_NONE = -1;

    // =========================================================================
    // InputDeviceId
    // =========================================================================

    /// 入力デバイスID（強い型ID）
    struct InputDeviceId
    {
        InputDeviceId() : m_id(INDEX_NONE) {}
        explicit InputDeviceId(int32_t InId) : m_id(InId) {}

        bool operator==(const InputDeviceId& Other) const { return m_id == Other.m_id; }
        bool operator!=(const InputDeviceId& Other) const { return m_id != Other.m_id; }

        bool IsValid() const { return m_id != INDEX_NONE; }
        int32_t GetId() const { return m_id; }

        static const InputDeviceId NONE;

    private:
        int32_t m_id;
    };

    inline const InputDeviceId InputDeviceId::NONE{};

    // =========================================================================
    // PlatformUserId
    // =========================================================================

    /// プラットフォームユーザーID（強い型ID）
    struct PlatformUserId
    {
        PlatformUserId() : m_id(INDEX_NONE) {}
        explicit PlatformUserId(int32_t InId) : m_id(InId) {}

        bool operator==(const PlatformUserId& Other) const { return m_id == Other.m_id; }
        bool operator!=(const PlatformUserId& Other) const { return m_id != Other.m_id; }

        bool IsValid() const { return m_id != INDEX_NONE; }
        int32_t GetId() const { return m_id; }

        static const PlatformUserId NONE;

    private:
        int32_t m_id;
    };

    inline const PlatformUserId PlatformUserId::NONE{};

    /// 互換マクロ
    #define PLATFORMUSERID_NONE ::NS::PlatformUserId::NONE

    // =========================================================================
    // PlatformUserSelectorFlags
    // =========================================================================

    /// プラットフォームユーザー選択フラグ
    enum class PlatformUserSelectorFlags : uint8_t
    {
        None = 0,
        RequiresOnlineEnabledProfile = 1 << 0,
        ShowSkipButton               = 1 << 1,
        AllowGuests                  = 1 << 2,
        ShowNewUsersOnly             = 1 << 3,
        /// デフォルト
        Default = ShowSkipButton
    };

} // namespace NS

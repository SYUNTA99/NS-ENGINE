/// @file GenericPlatformSoftwareCursor.h
/// @brief ハードウェアカーソル非対応プラットフォーム用ソフトウェアカーソル
#pragma once

#include "GenericPlatform/ICursor.h"

namespace NS
{
    /// ソフトウェアカーソル実装 (ICursor 継承)
    /// ハードウェアカーソルを使わず、位置・形状をソフトウェアで追跡する。
    /// レンダラーが GetCurrentPosition()/GetCurrentType() を毎フレーム参照して描画に使用。
    class GenericPlatformSoftwareCursor : public ICursor
    {
    public:
        GenericPlatformSoftwareCursor();
        ~GenericPlatformSoftwareCursor() override = default;
        NS_DISALLOW_COPY_AND_MOVE(GenericPlatformSoftwareCursor);

    public:
        // =================================================================
        // ICursor overrides
        // =================================================================
        [[nodiscard]] MouseCursor::Type GetType() const override;
        void SetType(MouseCursor::Type inType) override;
        void GetSize(int32_t& width, int32_t& height) const override;
        void GetPosition(Vector2D& outPosition) const override;
        void SetPosition(int32_t x, int32_t y) override;
        void Show(bool bShow) override;
        void Lock(const PlatformRect* bounds) override;

        // =================================================================
        // レンダリングフック
        // =================================================================

        /// 現在のカーソル位置を取得（レンダラー用）
        [[nodiscard]] const Vector2D& GetCurrentPosition() const { return m_position; }

        /// 現在のカーソル種別を取得（レンダラー用）
        [[nodiscard]] MouseCursor::Type GetCurrentType() const { return m_currentType; }

        /// カーソルが表示中か
        [[nodiscard]] bool IsVisible() const { return m_bShow; }

    private:
        /// 位置を Lock 矩形内にクランプ
        void ClampPosition();

        Vector2D m_position;
        MouseCursor::Type m_currentType = MouseCursor::Default;
        bool m_bShow = true;
        bool m_bLocked = false;
        PlatformRect m_lockBounds;
    };

} // namespace NS

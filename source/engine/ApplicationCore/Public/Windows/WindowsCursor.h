/// @file WindowsCursor.h
/// @brief Windows カーソル実装
#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "GenericPlatform/ICursor.h"

namespace NS
{
    /// Windows 固有カーソル実装
    class WindowsCursor : public ICursor
    {
    public:
        WindowsCursor();
        ~WindowsCursor() override;
        NS_DISALLOW_COPY_AND_MOVE(WindowsCursor);

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

        void SetTypeShape(MouseCursor::Type inCursorType, void* inCursorHandle) override;
        void* CreateCursorFromFile(const std::wstring& inPath, Vector2D inHotSpot) override;
        [[nodiscard]] bool IsCreateCursorFromRGBABufferSupported() const override;
        void* CreateCursorFromRGBABuffer(const uint8_t* inPixels,
                                         int32_t inWidth,
                                         int32_t inHeight,
                                         Vector2D inHotSpot) override;

    private:
        /// IDC_* マッピング初期化
        void InitializeDefaultCursors();

        /// カスタム .cur ファイルの読み込み
        HCURSOR LoadCursorFromFile(const wchar_t* inPath);

        // カーソルハンドル配列
        HCURSOR m_cursorHandles[MouseCursor::TotalCursorCount] = {};
        HCURSOR m_cursorOverrideHandles[MouseCursor::TotalCursorCount] = {};
        bool m_bIsCustomCursor[MouseCursor::TotalCursorCount] = {};

        MouseCursor::Type m_currentType = MouseCursor::Default;
        bool m_bShow = true;
    };

} // namespace NS

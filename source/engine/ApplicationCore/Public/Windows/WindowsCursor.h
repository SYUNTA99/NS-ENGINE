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

        // =================================================================
        // ICursor overrides
        // =================================================================
        MouseCursor::Type GetType() const override;
        void SetType(MouseCursor::Type InType) override;
        void GetSize(int32_t& Width, int32_t& Height) const override;
        void GetPosition(Vector2D& OutPosition) const override;
        void SetPosition(int32_t X, int32_t Y) override;
        void Show(bool bShow) override;
        void Lock(const PlatformRect* Bounds) override;

        void SetTypeShape(MouseCursor::Type InCursorType, void* InCursorHandle) override;
        void* CreateCursorFromFile(const std::wstring& InPath, Vector2D InHotSpot) override;
        bool IsCreateCursorFromRGBABufferSupported() const override;
        void* CreateCursorFromRGBABuffer(const uint8_t* InPixels,
                                         int32_t InWidth,
                                         int32_t InHeight,
                                         Vector2D InHotSpot) override;

    private:
        /// IDC_* マッピング初期化
        void InitializeDefaultCursors();

        /// カスタム .cur ファイルの読み込み
        HCURSOR LoadCursorFromFile(const wchar_t* InPath);

        // カーソルハンドル配列
        HCURSOR m_cursorHandles[MouseCursor::TotalCursorCount] = {};
        HCURSOR m_cursorOverrideHandles[MouseCursor::TotalCursorCount] = {};
        bool m_bIsCustomCursor[MouseCursor::TotalCursorCount] = {};

        MouseCursor::Type m_currentType = MouseCursor::Default;
        bool m_bShow = true;
    };

} // namespace NS

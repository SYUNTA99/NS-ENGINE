/// @file WindowsCursor.cpp
/// @brief WindowsCursor 実装 (07-03 + 07-04)
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Windows/WindowsCursor.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace NS
{
    // =========================================================================
    // コンストラクタ・デストラクタ
    // =========================================================================

    WindowsCursor::WindowsCursor()
    {
        std::memset(m_cursorHandles, 0, sizeof(m_cursorHandles));
        std::memset(m_cursorOverrideHandles, 0, sizeof(m_cursorOverrideHandles));
        std::memset(m_bIsCustomCursor, 0, sizeof(m_bIsCustomCursor));

        InitializeDefaultCursors();
    }

    WindowsCursor::~WindowsCursor()
    {
        // カスタムカーソルのみ DestroyCursor で解放
        for (int32_t i = 0; i < MouseCursor::TotalCursorCount; ++i)
        {
            if (m_bIsCustomCursor[i] && m_cursorHandles[i])
            {
                ::DestroyCursor(m_cursorHandles[i]);
                m_cursorHandles[i] = nullptr;
            }
            if (m_cursorOverrideHandles[i])
            {
                ::DestroyCursor(m_cursorOverrideHandles[i]);
                m_cursorOverrideHandles[i] = nullptr;
            }
        }
    }

    // =========================================================================
    // IDC_* マッピング初期化
    // =========================================================================

    void WindowsCursor::InitializeDefaultCursors()
    {
        // システムカーソルマッピング
        m_cursorHandles[MouseCursor::None] = nullptr;
        m_cursorHandles[MouseCursor::Default] = ::LoadCursor(NULL, IDC_ARROW);
        m_cursorHandles[MouseCursor::TextEditBeam] = ::LoadCursor(NULL, IDC_IBEAM);
        m_cursorHandles[MouseCursor::ResizeLeftRight] = ::LoadCursor(NULL, IDC_SIZEWE);
        m_cursorHandles[MouseCursor::ResizeUpDown] = ::LoadCursor(NULL, IDC_SIZENS);
        m_cursorHandles[MouseCursor::ResizeSouthEast] = ::LoadCursor(NULL, IDC_SIZENWSE);
        m_cursorHandles[MouseCursor::ResizeSouthWest] = ::LoadCursor(NULL, IDC_SIZENESW);
        m_cursorHandles[MouseCursor::CardinalCross] = ::LoadCursor(NULL, IDC_SIZEALL);
        m_cursorHandles[MouseCursor::Crosshairs] = ::LoadCursor(NULL, IDC_CROSS);
        m_cursorHandles[MouseCursor::Hand] = ::LoadCursor(NULL, IDC_HAND);
        m_cursorHandles[MouseCursor::SlashedCircle] = ::LoadCursor(NULL, IDC_NO);

        // カスタムカーソルファイル — カスタム .cur が存在しない場合はフォールバック
        // GrabHand → IDC_HAND フォールバック
        m_cursorHandles[MouseCursor::GrabHand] = ::LoadCursor(NULL, IDC_HAND);
        // GrabHandClosed → IDC_HAND フォールバック
        m_cursorHandles[MouseCursor::GrabHandClosed] = ::LoadCursor(NULL, IDC_HAND);
        // EyeDropper → Crosshairs フォールバック
        m_cursorHandles[MouseCursor::EyeDropper] = ::LoadCursor(NULL, IDC_CROSS);
        // Custom → デフォルト矢印
        m_cursorHandles[MouseCursor::Custom] = ::LoadCursor(NULL, IDC_ARROW);
    }

    HCURSOR WindowsCursor::LoadCursorFromFile(const wchar_t* InPath)
    {
        HCURSOR result = static_cast<HCURSOR>(::LoadImageW(NULL, InPath, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE));
        return result;
    }

    // =========================================================================
    // 型・サイズ
    // =========================================================================

    MouseCursor::Type WindowsCursor::GetType() const
    {
        return m_currentType;
    }

    void WindowsCursor::SetType(MouseCursor::Type InType)
    {
        if (InType >= MouseCursor::TotalCursorCount)
        {
            return;
        }

        m_currentType = InType;

        // オーバーライドを優先
        HCURSOR cursor = m_cursorOverrideHandles[InType];
        if (!cursor)
        {
            cursor = m_cursorHandles[InType];
        }

        if (cursor)
        {
            ::SetCursor(cursor);
        }
    }

    void WindowsCursor::GetSize(int32_t& Width, int32_t& Height) const
    {
        Width = ::GetSystemMetrics(SM_CXCURSOR);
        Height = ::GetSystemMetrics(SM_CYCURSOR);
    }

    // =========================================================================
    // 位置制御
    // =========================================================================

    void WindowsCursor::GetPosition(Vector2D& OutPosition) const
    {
        POINT cursorPos;
        if (::GetCursorPos(&cursorPos))
        {
            OutPosition.X = static_cast<float>(cursorPos.x);
            OutPosition.Y = static_cast<float>(cursorPos.y);
        }
    }

    void WindowsCursor::SetPosition(int32_t X, int32_t Y)
    {
        ::SetCursorPos(X, Y);
    }

    // =========================================================================
    // 表示・ロック
    // =========================================================================

    void WindowsCursor::Show(bool bShow)
    {
        if (bShow)
        {
            // ShowCursor はカウンタベース: 0以上で表示
            while (::ShowCursor(TRUE) < 0)
            {}
        }
        else
        {
            // 0未満で非表示
            while (::ShowCursor(FALSE) >= 0)
            {}
        }
        m_bShow = bShow;
    }

    void WindowsCursor::Lock(const PlatformRect* Bounds)
    {
        if (Bounds)
        {
            RECT clipRect;
            clipRect.left = Bounds->Left;
            clipRect.top = Bounds->Top;
            clipRect.right = Bounds->Right;
            clipRect.bottom = Bounds->Bottom;
            ::ClipCursor(&clipRect);
        }
        else
        {
            // nullptr で制限解除
            ::ClipCursor(nullptr);
        }
    }

    // =========================================================================
    // カーソル形状オーバーライド
    // =========================================================================

    void WindowsCursor::SetTypeShape(MouseCursor::Type InCursorType, void* InCursorHandle)
    {
        if (InCursorType >= MouseCursor::TotalCursorCount)
        {
            return;
        }

        m_cursorOverrideHandles[InCursorType] = static_cast<HCURSOR>(InCursorHandle);

        // 現在表示中のカーソルなら即座に反映
        if (m_currentType == InCursorType)
        {
            SetType(InCursorType);
        }
    }

    // =========================================================================
    // カスタムカーソル作成
    // =========================================================================

    void* WindowsCursor::CreateCursorFromFile(const std::wstring& InPath, Vector2D /*InHotSpot*/)
    {
        // .ani/.cur は LoadImage でホットスポット情報を含むため InHotSpot は無視
        HCURSOR cursor = LoadCursorFromFile(InPath.c_str());
        return static_cast<void*>(cursor);
    }

    bool WindowsCursor::IsCreateCursorFromRGBABufferSupported() const
    {
        return true;
    }

    void* WindowsCursor::CreateCursorFromRGBABuffer(const uint8_t* InPixels,
                                                    int32_t InWidth,
                                                    int32_t InHeight,
                                                    Vector2D InHotSpot)
    {
        if (!InPixels || InWidth <= 0 || InHeight <= 0)
        {
            return nullptr;
        }

        // 寸法上限チェック（整数オーバーフロー防止）
        if (InWidth > 4096 || InHeight > 4096)
        {
            return nullptr;
        }

        // RGBA → BGRA チャンネルスワップ (Win32 DIBセクション要件)
        const size_t pixelCount = static_cast<size_t>(InWidth) * static_cast<size_t>(InHeight);
        std::vector<uint8_t> bgraPixels(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            bgraPixels[i * 4 + 0] = InPixels[i * 4 + 2]; // B ← R
            bgraPixels[i * 4 + 1] = InPixels[i * 4 + 1]; // G ← G
            bgraPixels[i * 4 + 2] = InPixels[i * 4 + 0]; // R ← B
            bgraPixels[i * 4 + 3] = InPixels[i * 4 + 3]; // A ← A
        }

        // カラービットマップ
        HBITMAP hColorBitmap = ::CreateBitmap(InWidth, InHeight, 1, 32, bgraPixels.data());
        if (!hColorBitmap)
        {
            return nullptr;
        }

        // マスクビットマップ (1bpp, 全不透明)
        const int32_t maskRowBytes = ((InWidth + 31) / 32) * 4;
        std::vector<uint8_t> maskBits(maskRowBytes * InHeight, 0x00);
        HBITMAP hMaskBitmap = ::CreateBitmap(InWidth, InHeight, 1, 1, maskBits.data());
        if (!hMaskBitmap)
        {
            ::DeleteObject(hColorBitmap);
            return nullptr;
        }

        // ICONINFO からカーソル生成
        ICONINFO iconInfo = {};
        iconInfo.fIcon = FALSE; // カーソル
        float hx = std::clamp(InHotSpot.X, 0.0f, 1.0f);
        float hy = std::clamp(InHotSpot.Y, 0.0f, 1.0f);
        iconInfo.xHotspot =
            static_cast<DWORD>(std::min(static_cast<long>(InWidth - 1), std::max(0L, std::lround(hx * (InWidth - 1)))));
        iconInfo.yHotspot = static_cast<DWORD>(
            std::min(static_cast<long>(InHeight - 1), std::max(0L, std::lround(hy * (InHeight - 1)))));
        iconInfo.hbmMask = hMaskBitmap;
        iconInfo.hbmColor = hColorBitmap;

        HCURSOR hCursor = static_cast<HCURSOR>(::CreateIconIndirect(&iconInfo));

        // GDI リソース解放
        ::DeleteObject(hColorBitmap);
        ::DeleteObject(hMaskBitmap);

        return static_cast<void*>(hCursor);
    }

} // namespace NS

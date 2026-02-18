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
            if (m_bIsCustomCursor[i] && (m_cursorHandles[i] != nullptr))
            {
                ::DestroyCursor(m_cursorHandles[i]);
                m_cursorHandles[i] = nullptr;
            }
            if (m_cursorOverrideHandles[i] != nullptr)
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
        m_cursorHandles[MouseCursor::Default] = ::LoadCursor(nullptr, IDC_ARROW);
        m_cursorHandles[MouseCursor::TextEditBeam] = ::LoadCursor(nullptr, IDC_IBEAM);
        m_cursorHandles[MouseCursor::ResizeLeftRight] = ::LoadCursor(nullptr, IDC_SIZEWE);
        m_cursorHandles[MouseCursor::ResizeUpDown] = ::LoadCursor(nullptr, IDC_SIZENS);
        m_cursorHandles[MouseCursor::ResizeSouthEast] = ::LoadCursor(nullptr, IDC_SIZENWSE);
        m_cursorHandles[MouseCursor::ResizeSouthWest] = ::LoadCursor(nullptr, IDC_SIZENESW);
        m_cursorHandles[MouseCursor::CardinalCross] = ::LoadCursor(nullptr, IDC_SIZEALL);
        m_cursorHandles[MouseCursor::Crosshairs] = ::LoadCursor(nullptr, IDC_CROSS);
        m_cursorHandles[MouseCursor::Hand] = ::LoadCursor(nullptr, IDC_HAND);
        m_cursorHandles[MouseCursor::SlashedCircle] = ::LoadCursor(nullptr, IDC_NO);

        // カスタムカーソルファイル — カスタム .cur が存在しない場合はフォールバック
        // GrabHand → IDC_HAND フォールバック
        m_cursorHandles[MouseCursor::GrabHand] = ::LoadCursor(nullptr, IDC_HAND);
        // GrabHandClosed → IDC_HAND フォールバック
        m_cursorHandles[MouseCursor::GrabHandClosed] = ::LoadCursor(nullptr, IDC_HAND);
        // EyeDropper → Crosshairs フォールバック
        m_cursorHandles[MouseCursor::EyeDropper] = ::LoadCursor(nullptr, IDC_CROSS);
        // Custom → デフォルト矢印
        m_cursorHandles[MouseCursor::Custom] = ::LoadCursor(nullptr, IDC_ARROW);
    }

    HCURSOR WindowsCursor::LoadCursorFromFile(const wchar_t* inPath)
    {
        auto result = static_cast<HCURSOR>(::LoadImageW(nullptr, inPath, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE));
        return result;
    }

    // =========================================================================
    // 型・サイズ
    // =========================================================================

    MouseCursor::Type WindowsCursor::GetType() const
    {
        return m_currentType;
    }

    void WindowsCursor::SetType(MouseCursor::Type inType)
    {
        if (inType >= MouseCursor::TotalCursorCount)
        {
            return;
        }

        m_currentType = inType;

        // オーバーライドを優先
        HCURSOR cursor = m_cursorOverrideHandles[inType];
        if (cursor == nullptr)
        {
            cursor = m_cursorHandles[inType];
        }

        if (cursor != nullptr)
        {
            ::SetCursor(cursor);
        }
    }

    void WindowsCursor::GetSize(int32_t& width, int32_t& height) const
    {
        width = ::GetSystemMetrics(SM_CXCURSOR);
        height = ::GetSystemMetrics(SM_CYCURSOR);
    }

    // =========================================================================
    // 位置制御
    // =========================================================================

    void WindowsCursor::GetPosition(Vector2D& outPosition) const
    {
        POINT cursorPos;
        if (::GetCursorPos(&cursorPos) != 0)
        {
            outPosition.x = static_cast<float>(cursorPos.x);
            outPosition.y = static_cast<float>(cursorPos.y);
        }
    }

    void WindowsCursor::SetPosition(int32_t x, int32_t y)
    {
        ::SetCursorPos(x, y);
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

    void WindowsCursor::Lock(const PlatformRect* bounds)
    {
        if (bounds != nullptr)
        {
            RECT clipRect;
            clipRect.left = bounds->left;
            clipRect.top = bounds->top;
            clipRect.right = bounds->right;
            clipRect.bottom = bounds->bottom;
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

    void WindowsCursor::SetTypeShape(MouseCursor::Type inCursorType, void* inCursorHandle)
    {
        if (inCursorType >= MouseCursor::TotalCursorCount)
        {
            return;
        }

        m_cursorOverrideHandles[inCursorType] = static_cast<HCURSOR>(inCursorHandle);

        // 現在表示中のカーソルなら即座に反映
        if (m_currentType == inCursorType)
        {
            SetType(inCursorType);
        }
    }

    // =========================================================================
    // カスタムカーソル作成
    // =========================================================================

    void* WindowsCursor::CreateCursorFromFile(const std::wstring& inPath, Vector2D /*InHotSpot*/)
    {
        // .ani/.cur は LoadImage でホットスポット情報を含むため InHotSpot は無視
        HCURSOR cursor = LoadCursorFromFile(inPath.c_str());
        return static_cast<void*>(cursor);
    }

    bool WindowsCursor::IsCreateCursorFromRGBABufferSupported() const
    {
        return true;
    }

    void* WindowsCursor::CreateCursorFromRGBABuffer(const uint8_t* inPixels,
                                                    int32_t inWidth,
                                                    int32_t inHeight,
                                                    Vector2D inHotSpot)
    {
        if ((inPixels == nullptr) || inWidth <= 0 || inHeight <= 0)
        {
            return nullptr;
        }

        // 寸法上限チェック（整数オーバーフロー防止）
        if (inWidth > 4096 || inHeight > 4096)
        {
            return nullptr;
        }

        // RGBA → BGRA チャンネルスワップ (Win32 DIBセクション要件)
        const size_t kPixelCount = static_cast<size_t>(inWidth) * static_cast<size_t>(inHeight);
        std::vector<uint8_t> bgraPixels(kPixelCount * 4);
        for (size_t i = 0; i < kPixelCount; ++i)
        {
            bgraPixels[(i * 4) + 0] = inPixels[(i * 4) + 2]; // B ← R
            bgraPixels[(i * 4) + 1] = inPixels[(i * 4) + 1]; // G ← G
            bgraPixels[(i * 4) + 2] = inPixels[(i * 4) + 0]; // R ← B
            bgraPixels[(i * 4) + 3] = inPixels[(i * 4) + 3]; // A ← A
        }

        // カラービットマップ
        HBITMAP hColorBitmap = ::CreateBitmap(inWidth, inHeight, 1, 32, bgraPixels.data());
        if (hColorBitmap == nullptr)
        {
            return nullptr;
        }

        // マスクビットマップ (1bpp, 全不透明)
        const int32_t kMaskRowBytes = ((inWidth + 31) / 32) * 4;
        std::vector<uint8_t> maskBits(static_cast<size_t>(kMaskRowBytes) * inHeight, 0x00);
        HBITMAP hMaskBitmap = ::CreateBitmap(inWidth, inHeight, 1, 1, maskBits.data());
        if (hMaskBitmap == nullptr)
        {
            ::DeleteObject(hColorBitmap);
            return nullptr;
        }

        // ICONINFO からカーソル生成
        ICONINFO iconInfo = {};
        iconInfo.fIcon = FALSE; // カーソル
        float const hx = std::clamp(inHotSpot.x, 0.0F, 1.0F);
        float const hy = std::clamp(inHotSpot.y, 0.0F, 1.0F);
        iconInfo.xHotspot = static_cast<DWORD>(
            std::min(static_cast<long>(inWidth - 1), std::max(0L, std::lround(hx * static_cast<float>(inWidth - 1)))));
        iconInfo.yHotspot = static_cast<DWORD>(std::min(
            static_cast<long>(inHeight - 1), std::max(0L, std::lround(hy * static_cast<float>(inHeight - 1)))));
        iconInfo.hbmMask = hMaskBitmap;
        iconInfo.hbmColor = hColorBitmap;

        auto hCursor = static_cast<HCURSOR>(::CreateIconIndirect(&iconInfo));

        // GDI リソース解放
        ::DeleteObject(hColorBitmap);
        ::DeleteObject(hMaskBitmap);

        return static_cast<void*>(hCursor);
    }

} // namespace NS

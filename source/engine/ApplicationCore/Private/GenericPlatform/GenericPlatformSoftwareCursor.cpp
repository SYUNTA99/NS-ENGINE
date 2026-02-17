/// @file GenericPlatformSoftwareCursor.cpp
/// @brief GenericPlatformSoftwareCursor 実装
#include "GenericPlatform/GenericPlatformSoftwareCursor.h"

#include <algorithm>

namespace NS
{
    GenericPlatformSoftwareCursor::GenericPlatformSoftwareCursor() : m_position{0.0f, 0.0f}, m_lockBounds{0, 0, 0, 0} {}

    MouseCursor::Type GenericPlatformSoftwareCursor::GetType() const
    {
        return m_currentType;
    }

    void GenericPlatformSoftwareCursor::SetType(MouseCursor::Type InType)
    {
        m_currentType = InType;
    }

    void GenericPlatformSoftwareCursor::GetSize(int32_t& Width, int32_t& Height) const
    {
        // ソフトウェアカーソルのデフォルトテクスチャサイズ
        Width = 32;
        Height = 32;
    }

    void GenericPlatformSoftwareCursor::GetPosition(Vector2D& OutPosition) const
    {
        OutPosition = m_position;
    }

    void GenericPlatformSoftwareCursor::SetPosition(int32_t X, int32_t Y)
    {
        m_position.X = static_cast<float>(X);
        m_position.Y = static_cast<float>(Y);
        ClampPosition();
    }

    void GenericPlatformSoftwareCursor::Show(bool bShow)
    {
        m_bShow = bShow;
    }

    void GenericPlatformSoftwareCursor::Lock(const PlatformRect* Bounds)
    {
        if (Bounds)
        {
            m_bLocked = true;
            m_lockBounds = *Bounds;
            ClampPosition();
        }
        else
        {
            m_bLocked = false;
        }
    }

    void GenericPlatformSoftwareCursor::ClampPosition()
    {
        if (m_bLocked)
        {
            m_position.X =
                std::clamp(m_position.X, static_cast<float>(m_lockBounds.Left), static_cast<float>(m_lockBounds.Right));
            m_position.Y =
                std::clamp(m_position.Y, static_cast<float>(m_lockBounds.Top), static_cast<float>(m_lockBounds.Bottom));
        }
    }

} // namespace NS

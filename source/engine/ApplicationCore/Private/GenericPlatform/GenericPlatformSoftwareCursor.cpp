/// @file GenericPlatformSoftwareCursor.cpp
/// @brief GenericPlatformSoftwareCursor 実装
#include "GenericPlatform/GenericPlatformSoftwareCursor.h"

#include <algorithm>

namespace NS
{
    GenericPlatformSoftwareCursor::GenericPlatformSoftwareCursor() : m_position{.x=0.0F, .y=0.0F}, m_lockBounds{.left=0, .top=0, .right=0, .bottom=0} {}

    MouseCursor::Type GenericPlatformSoftwareCursor::GetType() const
    {
        return m_currentType;
    }

    void GenericPlatformSoftwareCursor::SetType(MouseCursor::Type inType)
    {
        m_currentType = inType;
    }

    void GenericPlatformSoftwareCursor::GetSize(int32_t& width, int32_t& height) const
    {
        // ソフトウェアカーソルのデフォルトテクスチャサイズ
        width = 32;
        height = 32;
    }

    void GenericPlatformSoftwareCursor::GetPosition(Vector2D& outPosition) const
    {
        outPosition = m_position;
    }

    void GenericPlatformSoftwareCursor::SetPosition(int32_t x, int32_t y)
    {
        m_position.x = static_cast<float>(x);
        m_position.y = static_cast<float>(y);
        ClampPosition();
    }

    void GenericPlatformSoftwareCursor::Show(bool bShow)
    {
        m_bShow = bShow;
    }

    void GenericPlatformSoftwareCursor::Lock(const PlatformRect* bounds)
    {
        if (bounds != nullptr)
        {
            m_bLocked = true;
            m_lockBounds = *bounds;
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
            m_position.x =
                std::clamp(m_position.x, static_cast<float>(m_lockBounds.left), static_cast<float>(m_lockBounds.right));
            m_position.y =
                std::clamp(m_position.y, static_cast<float>(m_lockBounds.top), static_cast<float>(m_lockBounds.bottom));
        }
    }

} // namespace NS

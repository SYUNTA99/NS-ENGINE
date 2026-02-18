/// @file NullApplication.h
/// @brief ヘッドレス/サーバー環境向け Null 実装群
#pragma once

#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/ICursor.h"

namespace NS
{
    // =========================================================================
    // NullCursor
    // =========================================================================

    /// ソフトウェア位置追跡のみの Null カーソル
    class NullCursor : public ICursor
    {
    public:
        [[nodiscard]] MouseCursor::Type GetType() const override { return MouseCursor::Default; }
        void SetType(MouseCursor::Type /*InType*/) override {}
        void GetSize(int32_t& width, int32_t& height) const override
        {
            width = 0;
            height = 0;
        }
        void GetPosition(Vector2D& outPosition) const override { outPosition = m_position; }
        void SetPosition(int32_t x, int32_t y) override
        {
            m_position.x = static_cast<float>(x);
            m_position.y = static_cast<float>(y);
        }
        void Show(bool /*bShow*/) override {}
        void Lock(const PlatformRect* /*Bounds*/) override {}

    private:
        Vector2D m_position;
    };

    // =========================================================================
    // NullApplication
    // =========================================================================

    /// ヘッドレス/サーバー環境向け Null アプリケーション
    class NullApplication : public GenericApplication
    {
    public:
        NullApplication() : GenericApplication(std::make_shared<NullCursor>()) {}

        /// ファクトリ
        static std::shared_ptr<NullApplication> CreateNullApplication() { return std::make_shared<NullApplication>(); }

        bool IsMouseAttached() const override { return false; }
        bool IsGamepadAttached() const override { return false; }

        PlatformRect GetWorkArea(const PlatformRect& /*CurrentWindow*/) const override { return m_workArea; }

    private:
        PlatformRect m_workArea = {.left=0, .top=0, .right=1920, .bottom=1080};
    };

} // namespace NS

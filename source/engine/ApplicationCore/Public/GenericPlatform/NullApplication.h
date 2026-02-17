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
        MouseCursor::Type GetType() const override { return MouseCursor::Default; }
        void SetType(MouseCursor::Type /*InType*/) override {}
        void GetSize(int32_t& Width, int32_t& Height) const override
        {
            Width = 0;
            Height = 0;
        }
        void GetPosition(Vector2D& OutPosition) const override { OutPosition = m_position; }
        void SetPosition(int32_t X, int32_t Y) override
        {
            m_position.X = static_cast<float>(X);
            m_position.Y = static_cast<float>(Y);
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
        PlatformRect m_workArea = {0, 0, 1920, 1080};
    };

} // namespace NS

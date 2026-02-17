/// @file GenericAccessibleMessageHandler.h
/// @brief アクセシビリティメッセージハンドラ (14-01)
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"

#ifndef WITH_ACCESSIBILITY
#define WITH_ACCESSIBILITY 0
#endif

#if WITH_ACCESSIBILITY

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace NS
{
    class IAccessibleWidget;
    class GenericWindow;

    /// アクセシビリティイベント引数
    struct AccessibleEventArgs
    {
        std::shared_ptr<IAccessibleWidget> Widget;
        AccessibleEvent Event = AccessibleEvent::FocusChange;
        AccessibleUserIndex UserIndex = 0;
    };

    /// グローバルアクセシビリティ許可フラグ
    inline bool GAllowAccessibility = false;

    // =========================================================================
    // GenericAccessibleUserRegistry
    // =========================================================================

    /// アクセシブルユーザー登録管理
    class GenericAccessibleUserRegistry
    {
    public:
        /// ユーザー登録
        void RegisterUser(AccessibleUserIndex UserIndex) { m_users[UserIndex] = true; }

        /// ユーザー登録解除
        void UnregisterUser(AccessibleUserIndex UserIndex) { m_users.erase(UserIndex); }

        /// 全ユーザー登録解除
        void UnregisterAllUsers() { m_users.clear(); }

        /// ユーザーが登録されているか
        bool IsUserRegistered(AccessibleUserIndex UserIndex) const { return m_users.find(UserIndex) != m_users.end(); }

        /// 登録ユーザー数を取得 (※UE5 タイポ保存: NumberofUsers)
        int32_t GetNumberofUsers() const { return static_cast<int32_t>(m_users.size()); }

        /// 全ユーザーインデックスを取得
        std::vector<AccessibleUserIndex> GetAllUsers() const
        {
            std::vector<AccessibleUserIndex> result;
            result.reserve(m_users.size());
            for (const auto& pair : m_users)
            {
                result.push_back(pair.first);
            }
            return result;
        }

        /// プライマリユーザーインデックス (常に0)
        static AccessibleUserIndex GetPrimaryUserIndex() { return 0; }

    private:
        std::unordered_map<AccessibleUserIndex, bool> m_users;
    };

    // =========================================================================
    // GenericAccessibleMessageHandler
    // =========================================================================

    /// アクセシビリティメッセージハンドラ
    /// ウィジェットのアクセシビリティ情報をプラットフォームに公開する
    class GenericAccessibleMessageHandler
    {
    public:
        using AccessibleEventDelegate = std::function<void(const AccessibleEventArgs&)>;

        virtual ~GenericAccessibleMessageHandler() = default;

        /// アプリケーションがアクセシブルか
        bool ApplicationIsAccessible() const { return m_bApplicationIsAccessible && GAllowAccessibility; }

        /// アクティブ状態
        bool IsActive() const { return m_bIsActive; }
        void SetActive(bool bActive) { m_bIsActive = bActive; }

        // =================================================================
        // ウィジェットアクセス (サブクラスで実装)
        // =================================================================

        /// ウィンドウIDからアクセシブルウィンドウを取得
        virtual std::shared_ptr<IAccessibleWidget> GetAccessibleWindow(int32_t WindowId) = 0;

        /// ウィジェットからウィンドウIDを取得
        virtual int32_t GetAccessibleWindowId(const std::shared_ptr<IAccessibleWidget>& Widget) = 0;

        /// IDからアクセシブルウィジェットを取得
        virtual std::shared_ptr<IAccessibleWidget> GetAccessibleWidgetFromId(int32_t Id) = 0;

        // =================================================================
        // イベント
        // =================================================================

        /// アクセシビリティイベント発火
        void RaiseEvent(const AccessibleEventArgs& Args)
        {
            if (m_eventDelegate)
            {
                m_eventDelegate(Args);
            }
        }

        /// イベントデリゲート設定
        void SetAccessibleEventDelegate(AccessibleEventDelegate Delegate) { m_eventDelegate = std::move(Delegate); }

        /// イベントデリゲート解除
        void UnbindAccessibleEventDelegate() { m_eventDelegate = nullptr; }

        // =================================================================
        // スレッド安全
        // =================================================================

        /// ゲームスレッドでラムダを実行
        /// @param Lambda 実行する関数
        /// @param bBlock true: 完了を待つ, false: 非同期
        virtual void RunInThread(std::function<void()> Lambda, bool bBlock = false)
        {
            // デフォルト実装: 即時実行
            if (Lambda)
            {
                Lambda();
            }
            (void)bBlock;
        }

        // =================================================================
        // アナウンスメント (Win10/Mac/iOS)
        // =================================================================

        /// アクセシビリティアナウンスメント
        virtual void MakeAccessibleAnnouncement(const wchar_t* /*Text*/) {}

        // =================================================================
        // ユーザー管理
        // =================================================================

        GenericAccessibleUserRegistry& GetUserRegistry() { return m_userRegistry; }
        const GenericAccessibleUserRegistry& GetUserRegistry() const { return m_userRegistry; }

    protected:
        bool m_bApplicationIsAccessible = false;
        bool m_bIsActive = false;
        AccessibleEventDelegate m_eventDelegate;
        GenericAccessibleUserRegistry m_userRegistry;
    };

} // namespace NS

#endif // WITH_ACCESSIBILITY

/// @file IAccessibleWidget.h
/// @brief アクセシブルウィジェットインターフェース (14-02)
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"

#ifndef WITH_ACCESSIBILITY
#define WITH_ACCESSIBILITY 0
#endif

#if WITH_ACCESSIBILITY

#include <cstdint>
#include <functional>
#include <string>

namespace NS
{
    class GenericWindow;

    /// アクセシブルウィジェットインターフェース
    /// スクリーンリーダーやUI Automationが使用する抽象インターフェース
    class IAccessibleWidget
    {
    public:
        virtual ~IAccessibleWidget() = default;

        // =================================================================
        // 基本情報
        // =================================================================

        /// ウィジェットID
        virtual int32_t GetId() const = 0;

        /// ウィジェットタイプ
        virtual AccessibleWidgetType GetWidgetType() const = 0;

        /// ウィジェット名
        virtual std::wstring GetWidgetName() const = 0;

        /// ヘルプテキスト
        virtual std::wstring GetHelpText() const = 0;

        /// クラス名
        virtual std::wstring GetClassName() const = 0;

        /// バウンディングボックス (スクリーン座標)
        virtual void GetBounds(int32_t& OutX, int32_t& OutY, int32_t& OutWidth, int32_t& OutHeight) const = 0;

        // =================================================================
        // 状態
        // =================================================================

        /// 有効か
        virtual bool IsEnabled() const = 0;

        /// 非表示か
        virtual bool IsHidden() const = 0;

        /// 有効なウィジェットか
        virtual bool IsValid() const = 0;

        /// キーボードフォーカスをサポートするか
        virtual bool SupportsFocus() const = 0;

        /// アクセシブルフォーカスをサポートするか
        virtual bool SupportsAccessibleFocus() const = 0;

        /// 現在アクセシブルフォーカスを受け入れ可能か
        virtual bool CanCurrentlyAcceptAccessibleFocus() const = 0;

        /// 指定ユーザーがフォーカスしているか
        virtual bool HasUserFocus(AccessibleUserIndex UserIndex) const = 0;

        /// 指定ユーザーのフォーカスを設定
        virtual void SetUserFocus(AccessibleUserIndex UserIndex) = 0;

        // =================================================================
        // ナビゲーション
        // =================================================================

        /// 親ウィジェット
        virtual IAccessibleWidget* GetParent() const = 0;

        /// 子ウィジェット（インデックス指定）
        virtual IAccessibleWidget* GetChildAt(int32_t Index) const = 0;

        /// 子ウィジェット数
        virtual int32_t GetNumberOfChildren() const = 0;

        /// 次の兄弟
        virtual IAccessibleWidget* GetNextSibling() const = 0;

        /// 前の兄弟
        virtual IAccessibleWidget* GetPreviousSibling() const = 0;

        /// 階層内の次のウィジェット (深さ優先)
        virtual IAccessibleWidget* GetNextWidgetInHierarchy() const = 0;

        /// 階層内の前のウィジェット (深さ優先)
        virtual IAccessibleWidget* GetPreviousWidgetInHierarchy() const = 0;

        /// 所属ウィンドウ
        virtual GenericWindow* GetWindow() const = 0;

        // =================================================================
        // サブインターフェース (as* パターン)
        // =================================================================

        /// ウィンドウとして取得 (非ウィンドウなら nullptr)
        virtual IAccessibleWidget* AsWindow() { return nullptr; }

        /// アクティベート可能として取得
        virtual IAccessibleWidget* AsActivatable() { return nullptr; }

        /// プロパティ取得可能として取得
        virtual IAccessibleWidget* AsProperty() { return nullptr; }

        /// テキスト操作可能として取得
        virtual IAccessibleWidget* AsText() { return nullptr; }

        /// テーブルとして取得
        virtual IAccessibleWidget* AsTable() { return nullptr; }

        /// テーブル行として取得
        virtual IAccessibleWidget* AsTableRow() { return nullptr; }

        // =================================================================
        // 静的検索テンプレート (6種)
        // =================================================================

        /// 祖先を検索
        template <typename Predicate>
        static IAccessibleWidget* SearchForAncestorFrom(IAccessibleWidget* Start, Predicate Pred)
        {
            if (!Start)
                return nullptr;
            IAccessibleWidget* current = Start->GetParent();
            while (current)
            {
                if (Pred(current))
                    return current;
                current = current->GetParent();
            }
            return nullptr;
        }

        /// 次の兄弟を検索
        template <typename Predicate>
        static IAccessibleWidget* SearchForNextSiblingFrom(IAccessibleWidget* Start, Predicate Pred)
        {
            if (!Start)
                return nullptr;
            IAccessibleWidget* current = Start->GetNextSibling();
            while (current)
            {
                if (Pred(current))
                    return current;
                current = current->GetNextSibling();
            }
            return nullptr;
        }

        /// 前の兄弟を検索
        template <typename Predicate>
        static IAccessibleWidget* SearchForPreviousSiblingFrom(IAccessibleWidget* Start, Predicate Pred)
        {
            if (!Start)
                return nullptr;
            IAccessibleWidget* current = Start->GetPreviousSibling();
            while (current)
            {
                if (Pred(current))
                    return current;
                current = current->GetPreviousSibling();
            }
            return nullptr;
        }

        /// 階層内で次のウィジェットを検索
        template <typename Predicate>
        static IAccessibleWidget* SearchForNextWidgetInHierarchyFrom(IAccessibleWidget* Start, Predicate Pred)
        {
            if (!Start)
                return nullptr;
            IAccessibleWidget* current = Start->GetNextWidgetInHierarchy();
            while (current)
            {
                if (Pred(current))
                    return current;
                current = current->GetNextWidgetInHierarchy();
            }
            return nullptr;
        }

        /// 階層内で前のウィジェットを検索
        template <typename Predicate>
        static IAccessibleWidget* SearchForPreviousWidgetInHierarchyFrom(IAccessibleWidget* Start, Predicate Pred)
        {
            if (!Start)
                return nullptr;
            IAccessibleWidget* current = Start->GetPreviousWidgetInHierarchy();
            while (current)
            {
                if (Pred(current))
                    return current;
                current = current->GetPreviousWidgetInHierarchy();
            }
            return nullptr;
        }

        /// 最初の子を検索
        template <typename Predicate>
        static IAccessibleWidget* SearchForFirstChildFrom(IAccessibleWidget* Start, Predicate Pred)
        {
            if (!Start)
                return nullptr;
            int32_t count = Start->GetNumberOfChildren();
            for (int32_t i = 0; i < count; ++i)
            {
                IAccessibleWidget* child = Start->GetChildAt(i);
                if (child && Pred(child))
                    return child;
            }
            return nullptr;
        }
    };

} // namespace NS

#endif // WITH_ACCESSIBILITY

/// @file ITextInputMethodSystem.h
/// @brief テキスト入力メソッドシステムインターフェース (11-01, 11-02)
#pragma once

#include "ApplicationCore/ApplicationCoreTypes.h"
#include <cstdint>
#include <memory>
#include <string>

namespace NS
{
    // 前方宣言
    class GenericWindow;

    // =========================================================================
    // ITextInputMethodContext (11-01)
    // =========================================================================

    /// キャレット位置
    enum class CaretPosition : uint8_t
    {
        Beginning,
        Ending
    };

    /// テキスト入力メソッドコンテキスト
    /// IMEが操作対象とするテキストフィールドの抽象。
    class ITextInputMethodContext
    {
    public:
        virtual ~ITextInputMethodContext() = default;

        // =================================================================
        // コンポジション状態
        // =================================================================

        /// IME コンポジション中か
        virtual bool IsComposing() const = 0;

        /// コンポジション範囲
        virtual bool IsReadOnly() const = 0;

        // =================================================================
        // テキストアクセス
        // =================================================================

        /// テキスト全体を取得
        virtual void GetTextInRange(int32_t BeginIndex, int32_t Length, std::wstring& OutString) const = 0;

        /// 選択範囲を取得
        virtual void GetSelectionRange(int32_t& OutBeginIndex,
                                       int32_t& OutLength,
                                       CaretPosition& OutCaretPosition) const = 0;

        /// 選択範囲を設定
        virtual void SetSelectionRange(int32_t BeginIndex, int32_t Length, CaretPosition InCaretPosition) = 0;

        /// テキスト長
        virtual int32_t GetTextLength() const = 0;

        // =================================================================
        // 座標
        // =================================================================

        /// 文字インデックスからスクリーン座標を取得
        virtual void GetTextBounds(int32_t BeginIndex, int32_t Length, PlatformRect& OutScreenBounds) const = 0;

        /// スクリーン座標から文字インデックスへ変換
        virtual int32_t GetCharacterIndexFromPoint(const Vector2D& Point) const = 0;

        // =================================================================
        // テキスト操作
        // =================================================================

        /// テキスト挿入
        virtual void InsertTextAtCursor(const std::wstring& InString) = 0;

        /// コンポジション開始
        virtual void BeginComposition() = 0;

        /// コンポジション更新
        virtual void UpdateCompositionRange(int32_t BeginIndex, int32_t Length) = 0;

        /// コンポジション終了
        virtual void EndComposition() = 0;

        // =================================================================
        // ウィンドウ関連
        // =================================================================

        /// 関連ウィンドウを取得
        virtual std::shared_ptr<GenericWindow> GetWindow() const = 0;
    };

    // =========================================================================
    // ITextInputMethodChangeNotifier (11-02)
    // =========================================================================

    /// テキスト入力メソッド変更通知
    class ITextInputMethodChangeNotifier
    {
    public:
        virtual ~ITextInputMethodChangeNotifier() = default;

        /// レイアウト変更通知
        virtual void NotifyLayoutChanged() = 0;

        /// 選択変更通知
        virtual void NotifySelectionChanged() = 0;

        /// テキスト変更通知
        virtual void NotifyTextChanged() = 0;

        /// コンポジションキャンセル要求
        virtual void CancelComposition() = 0;
    };

    // =========================================================================
    // ITextInputMethodSystem (11-02)
    // =========================================================================

    /// テキスト入力メソッドシステム
    class ITextInputMethodSystem
    {
    public:
        virtual ~ITextInputMethodSystem() = default;

        /// コンテキスト登録
        virtual std::shared_ptr<ITextInputMethodChangeNotifier> RegisterContext(
            const std::shared_ptr<ITextInputMethodContext>& Context) = 0;

        /// コンテキスト登録解除
        virtual void UnregisterContext(const std::shared_ptr<ITextInputMethodContext>& Context) = 0;

        /// コンテキストをアクティブ化（IME入力開始）
        virtual void ActivateContext(const std::shared_ptr<ITextInputMethodContext>& Context) = 0;

        /// コンテキストを非アクティブ化（IME入力停止）
        virtual void DeactivateContext(const std::shared_ptr<ITextInputMethodContext>& Context) = 0;

        /// デフォルトIMEウィンドウをアプリケーションに適用
        virtual bool ApplyDefaults(const std::shared_ptr<GenericWindow>& InWindow)
        {
            (void)InWindow;
            return false;
        }
    };

} // namespace NS

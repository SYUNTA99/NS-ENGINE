/// @file IConsoleVariable.h
/// @brief コンソール変数インターフェース
#pragma once

#include "HAL/PlatformMacros.h"
#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// コンソール変数フラグ
    ///
    /// ビット演算で組み合わせ可能。
    /// 設定元フラグは数値が大きいほど優先度が高い。
    enum class ConsoleVariableFlags : uint32
    {
        None = 0,

        // 属性フラグ (bit 0-15)
        Unregistered = 1 << 0,     ///< システムに未登録
        ReadOnly = 1 << 2,         ///< 読み取り専用
        Cheat = 1 << 3,            ///< チート扱い
        RenderThreadSafe = 1 << 4, ///< レンダースレッドから安全に読み取り可
        Archive = 1 << 5,          ///< 設定ファイルに保存/読み込み
        ShaderChange = 1 << 6,     ///< 変更時シェーダ再コンパイルをトリガー
        Scalability = 1 << 7,      ///< スケーラビリティグループの一部
        Preview = 1 << 8,          ///< プレビュー用

        // 設定元フラグ (bit 16-23)
        SetByConstructor = 1 << 16,    ///< コンストラクタで設定（優先度1）
        SetByScalability = 2 << 16,    ///< スケーラビリティで設定（優先度2）
        SetByGameSetting = 3 << 16,    ///< ゲーム設定で設定（優先度3）
        SetByProjectSetting = 4 << 16, ///< プロジェクト設定で設定（優先度4）
        SetByCommandline = 5 << 16,    ///< コマンドラインで設定（優先度5）
        SetByConsole = 6 << 16,        ///< コンソールで設定（優先度6）
        SetByCode = 7 << 16,           ///< コードで設定（優先度7、最高）

        SetByMask = 0xFF << 16 ///< 設定元マスク
    };

    // フラグ演算子
    FORCEINLINE ConsoleVariableFlags operator|(ConsoleVariableFlags a, ConsoleVariableFlags b)
    {
        return static_cast<ConsoleVariableFlags>(static_cast<uint32>(a) | static_cast<uint32>(b));
    }

    FORCEINLINE ConsoleVariableFlags operator&(ConsoleVariableFlags a, ConsoleVariableFlags b)
    {
        return static_cast<ConsoleVariableFlags>(static_cast<uint32>(a) & static_cast<uint32>(b));
    }

    FORCEINLINE ConsoleVariableFlags operator~(ConsoleVariableFlags a)
    {
        return static_cast<ConsoleVariableFlags>(~static_cast<uint32>(a));
    }

    FORCEINLINE ConsoleVariableFlags& operator|=(ConsoleVariableFlags& a, ConsoleVariableFlags b)
    {
        a = a | b;
        return a;
    }

    FORCEINLINE bool HasFlag(ConsoleVariableFlags flags, ConsoleVariableFlags test)
    {
        return (static_cast<uint32>(flags) & static_cast<uint32>(test)) != 0;
    }

    /// 設定元優先度を取得（0-255、大きいほど高優先度）
    FORCEINLINE uint32 GetSetByPriority(ConsoleVariableFlags flags)
    {
        return (static_cast<uint32>(flags) >> 16) & 0xFF;
    }

    /// 設定元を比較（aがbより高優先度ならtrue）
    FORCEINLINE bool HasHigherPriority(ConsoleVariableFlags a, ConsoleVariableFlags b)
    {
        return GetSetByPriority(a) > GetSetByPriority(b);
    }

    /// 設定可能か判定
    FORCEINLINE bool CanSetWithPriority(ConsoleVariableFlags currentFlags, ConsoleVariableFlags newSetBy)
    {
        return GetSetByPriority(newSetBy) >= GetSetByPriority(currentFlags);
    }

    // 前方宣言
    class IConsoleVariable;

    /// コンソール変数変更時のコールバック
    using ConsoleVariableDelegate = void (*)(IConsoleVariable* variable);

    /// コールバック登録ハンドル
    using ConsoleVariableCallbackHandle = uint32;

    /// 無効なコールバックハンドル
    constexpr ConsoleVariableCallbackHandle kInvalidCallbackHandle = 0;

    /// コンソールオブジェクト基底インターフェース
    class IConsoleObject
    {
    public:
        IConsoleObject() = default;
        virtual ~IConsoleObject() = default;
        NS_DISALLOW_COPY_AND_MOVE(IConsoleObject);

    public:
        /// ヘルプテキスト取得
        [[nodiscard]] virtual const TCHAR* GetHelp() const = 0;

        /// ヘルプテキスト設定
        virtual void SetHelp(const TCHAR* help) = 0;

        /// フラグ取得
        [[nodiscard]] virtual ConsoleVariableFlags GetFlags() const = 0;

        /// フラグ設定
        virtual void SetFlags(ConsoleVariableFlags flags) = 0;

        /// 変数として取得（変数でなければnullptr）
        virtual IConsoleVariable* AsVariable() { return nullptr; }
    };

    /// コンソール変数インターフェース
    ///
    /// 実行時に変更可能な設定値。
    /// int, float, bool, string型をサポート。
    class IConsoleVariable : public IConsoleObject
    {
    public:
        /// 整数値として取得
        [[nodiscard]] virtual int32 GetInt() const = 0;

        /// 浮動小数点値として取得
        [[nodiscard]] virtual float GetFloat() const = 0;

        /// ブール値として取得
        [[nodiscard]] virtual bool GetBool() const = 0;

        /// 文字列として取得
        [[nodiscard]] virtual const TCHAR* GetString() const = 0;

        /// 整数値を設定
        virtual void Set(int32 value, ConsoleVariableFlags flags = ConsoleVariableFlags::SetByCode) = 0;

        /// 浮動小数点値を設定
        virtual void Set(float value, ConsoleVariableFlags flags = ConsoleVariableFlags::SetByCode) = 0;

        /// 文字列値を設定
        virtual void Set(const TCHAR* value, ConsoleVariableFlags flags = ConsoleVariableFlags::SetByCode) = 0;

        /// 変更コールバック登録（単一、レガシー）
        virtual void SetOnChangedCallback(ConsoleVariableDelegate callback) = 0;

        /// 変更コールバック追加（複数登録対応）
        virtual ConsoleVariableCallbackHandle AddOnChangedCallback(ConsoleVariableDelegate callback) = 0;

        /// 変更コールバック解除
        virtual bool RemoveOnChangedCallback(ConsoleVariableCallbackHandle handle) = 0;

        /// 全コールバック解除
        virtual void ClearOnChangedCallbacks() = 0;

        /// 現在の設定元を取得
        [[nodiscard]] virtual ConsoleVariableFlags GetSetBy() const
        {
            return GetFlags() & ConsoleVariableFlags::SetByMask;
        }

        /// デフォルト値にリセット
        virtual void Reset() = 0;

        IConsoleVariable* AsVariable() override { return this; }
    };

    /// コンソールコマンドインターフェース
    class IConsoleCommand : public IConsoleObject
    {
    public:
        /// コマンド実行
        virtual bool Execute(const TCHAR* args) = 0;
    };
} // namespace NS

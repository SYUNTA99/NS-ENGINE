/// @file ConsoleManager.h
/// @brief コンソール変数・コマンドマネージャー
#pragma once

#include "HAL/IConsoleVariable.h"
#include "HAL/PlatformMacros.h"
#include "common/utility/macros.h"

#include <type_traits>

namespace NS
{
    /// コンソールマネージャーインターフェース
    ///
    /// 全コンソール変数・コマンドの登録・検索・列挙を管理。
    class IConsoleManager
    {
    public:
        virtual ~IConsoleManager() = default;

        /// シングルトン取得
        static IConsoleManager& Get();

        // =========================================================================
        // 変数登録
        // =========================================================================

        /// int変数を登録
        /// @param name 変数名（"r.ShadowQuality"形式）
        /// @param defaultValue 初期値
        /// @param help ヘルプテキスト
        /// @param flags フラグ
        /// @return 登録された変数
        virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* name, int32 defaultValue, const TCHAR* help,
                                                          ConsoleVariableFlags flags = ConsoleVariableFlags::None) = 0;

        /// float変数を登録
        virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* name, float defaultValue, const TCHAR* help,
                                                          ConsoleVariableFlags flags = ConsoleVariableFlags::None) = 0;

        /// string変数を登録
        virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* name, const TCHAR* defaultValue,
                                                          const TCHAR* help,
                                                          ConsoleVariableFlags flags = ConsoleVariableFlags::None) = 0;

        /// int参照変数を登録（既存変数へのバインド）
        virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name, int32& variable, const TCHAR* help,
                                                             ConsoleVariableFlags flags =
                                                                 ConsoleVariableFlags::None) = 0;

        /// float参照変数を登録
        virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name, float& variable, const TCHAR* help,
                                                             ConsoleVariableFlags flags =
                                                                 ConsoleVariableFlags::None) = 0;

        /// bool参照変数を登録
        virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name, bool& variable, const TCHAR* help,
                                                             ConsoleVariableFlags flags =
                                                                 ConsoleVariableFlags::None) = 0;

        // =========================================================================
        // コマンド登録
        // =========================================================================

        /// コマンドを登録
        /// @param name コマンド名
        /// @param help ヘルプテキスト
        /// @param command 実行関数
        /// @param flags フラグ
        virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* name, const TCHAR* help,
                                                        bool (*command)(const TCHAR* args),
                                                        ConsoleVariableFlags flags = ConsoleVariableFlags::None) = 0;

        // =========================================================================
        // 検索・取得
        // =========================================================================

        /// 名前で変数を検索
        /// @param name 変数名
        /// @return 見つかった変数（なければnullptr）
        virtual IConsoleVariable* FindConsoleVariable(const TCHAR* name) const = 0;

        /// 名前でオブジェクト（変数またはコマンド）を検索
        virtual IConsoleObject* FindConsoleObject(const TCHAR* name) const = 0;

        // =========================================================================
        // 登録解除
        // =========================================================================

        /// オブジェクトを登録解除
        virtual void UnregisterConsoleObject(const TCHAR* name) = 0;

        // =========================================================================
        // ユーティリティ
        // =========================================================================

        /// コンソール入力を処理
        /// @param input 入力文字列（"r.ShadowQuality 3"形式）
        /// @return 処理成功したか
        virtual bool ProcessInput(const TCHAR* input) = 0;

        /// 登録済み変数をすべて列挙
        /// @param callback 各変数に対して呼ばれるコールバック
        virtual void ForEachConsoleObject(void (*callback)(const TCHAR* name, IConsoleObject* object)) const = 0;
    };

    /// グローバルコンソールマネージャー取得
    FORCEINLINE IConsoleManager& GetConsoleManager()
    {
        return IConsoleManager::Get();
    }

    // =========================================================================
    // 自動登録ヘルパー
    // =========================================================================

    /// コンソール変数の自動登録クラス
    ///
    /// グローバル/静的変数として宣言すると自動登録。
    /// @tparam T 変数型（int32, float, bool）
    template <typename T> class TAutoConsoleVariable
    {
    public:
        TAutoConsoleVariable(const TCHAR* name, T defaultValue, const TCHAR* help,
                             ConsoleVariableFlags flags = ConsoleVariableFlags::None)
            : m_variable(nullptr), m_name(name), m_defaultValue(defaultValue), m_help(help), m_flags(flags)
        {
        }

        /// 変数取得
        IConsoleVariable* GetVariable()
        {
            EnsureRegistered();
            return m_variable;
        }

        /// 値取得
        T GetValue()
        {
            EnsureRegistered();
            if (!m_variable)
            {
                return m_defaultValue;
            }

            if constexpr (std::is_same_v<T, int32>)
            {
                return m_variable->GetInt();
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                return m_variable->GetFloat();
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return m_variable->GetBool();
            }
            return m_defaultValue;
        }

        /// 暗黙変換
        operator T()
        {
            return GetValue();
        }

        NS_DISALLOW_COPY_AND_MOVE(TAutoConsoleVariable);

    private:
        void EnsureRegistered()
        {
            if (!m_variable)
            {
                if constexpr (std::is_same_v<T, int32>)
                {
                    m_variable = GetConsoleManager().RegisterConsoleVariable(m_name, m_defaultValue, m_help, m_flags);
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    m_variable = GetConsoleManager().RegisterConsoleVariable(m_name, m_defaultValue, m_help, m_flags);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    // boolはintとして登録（0/1）
                    m_variable = GetConsoleManager().RegisterConsoleVariable(m_name, m_defaultValue ? 1 : 0, m_help, m_flags);
                }
            }
        }

        IConsoleVariable* m_variable;
        const TCHAR* m_name;
        T m_defaultValue;
        const TCHAR* m_help;
        ConsoleVariableFlags m_flags;
    };

    /// int型自動登録変数
    using AutoConsoleVariableInt = TAutoConsoleVariable<int32>;

    /// float型自動登録変数
    using AutoConsoleVariableFloat = TAutoConsoleVariable<float>;

    /// bool型自動登録変数
    using AutoConsoleVariableBool = TAutoConsoleVariable<bool>;
} // namespace NS

/// コンソール変数定義マクロ
/// @param Type 型（Int, Float）
/// @param Name 変数名
/// @param Default 初期値
/// @param Help ヘルプ文字列
/// @param Flags フラグ
#define NS_CONSOLE_VARIABLE(Type, Name, Default, Help, Flags)                                                          \
    static NS::AutoConsoleVariable##Type Name(TEXT(#Name), Default, TEXT(Help), Flags)

/// コンソール変数定義（フラグなし）
#define NS_CONSOLE_VARIABLE_SIMPLE(Type, Name, Default, Help)                                                          \
    NS_CONSOLE_VARIABLE(Type, Name, Default, Help, NS::ConsoleVariableFlags::None)

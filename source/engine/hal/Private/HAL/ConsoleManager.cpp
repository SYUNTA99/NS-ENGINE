/// @file ConsoleManager.cpp
/// @brief コンソールマネージャー実装

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "HAL/ConsoleManager.h"
#include "HAL/Platform.h"
#include "HAL/PlatformString.h"

#include <cwchar>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace NS
{
    namespace
    {
        // =================================================================
        // コールバック管理の共通基底
        // =================================================================
        class ConsoleVariableBase : public IConsoleVariable
        {
        public:
            ConsoleVariableBase(const TCHAR* help, ConsoleVariableFlags flags)
                : m_help((help != nullptr) ? help : TEXT("")), m_flags(flags)
            {}

            [[nodiscard]] const TCHAR* GetHelp() const override { return m_help.c_str(); }
            void SetHelp(const TCHAR* help) override { m_help = (help != nullptr) ? help : TEXT(""); }
            [[nodiscard]] ConsoleVariableFlags GetFlags() const override { return m_flags; }
            void SetFlags(ConsoleVariableFlags flags) override { m_flags = flags; }

            void SetOnChangedCallback(ConsoleVariableDelegate callback) override { m_legacyCallback = callback; }

            ConsoleVariableCallbackHandle AddOnChangedCallback(ConsoleVariableDelegate callback) override
            {
                if (callback == nullptr)
                {
                    return kInvalidCallbackHandle;
                }
                ConsoleVariableCallbackHandle const handle = m_nextCallbackHandle++;
                m_callbacks.push_back({.handle = handle, .callback = callback});
                return handle;
            }

            bool RemoveOnChangedCallback(ConsoleVariableCallbackHandle handle) override
            {
                for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
                {
                    if (it->handle == handle)
                    {
                        m_callbacks.erase(it);
                        return true;
                    }
                }
                return false;
            }

            void ClearOnChangedCallbacks() override
            {
                m_callbacks.clear();
                m_legacyCallback = nullptr;
            }

        protected:
            void UpdateSetBy(ConsoleVariableFlags newFlags)
            {
                m_flags = (m_flags & ~ConsoleVariableFlags::SetByMask) | (newFlags & ConsoleVariableFlags::SetByMask);
            }

            void NotifyChanged()
            {
                if (m_legacyCallback != nullptr)
                {
                    m_legacyCallback(this);
                }
                for (const auto& entry : m_callbacks)
                {
                    entry.callback(this);
                }
            }

            struct CallbackEntry
            {
                ConsoleVariableCallbackHandle handle;
                ConsoleVariableDelegate callback;
            };

            std::wstring m_help;
            ConsoleVariableFlags m_flags;
            ConsoleVariableDelegate m_legacyCallback{nullptr};
            std::vector<CallbackEntry> m_callbacks;
            ConsoleVariableCallbackHandle m_nextCallbackHandle{1};
        };

        // =================================================================
        // 値を内部に保持するコンソール変数
        // =================================================================
        template <typename T> class TConsoleVariable : public ConsoleVariableBase
        {
        public:
            TConsoleVariable(T defaultValue, const TCHAR* help, ConsoleVariableFlags flags)
                : ConsoleVariableBase(help, flags), m_value(defaultValue), m_defaultValue(defaultValue)
            {}

            [[nodiscard]] int32 GetInt() const override
            {
                if constexpr (std::is_same_v<T, int32>)
                {
                    return m_value;
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    return static_cast<int32>(m_value);
                }
                else
                {
                    return m_value ? 1 : 0;
                }
            }

            [[nodiscard]] float GetFloat() const override
            {
                if constexpr (std::is_same_v<T, float>)
                {
                    return m_value;
                }
                else if constexpr (std::is_same_v<T, int32>)
                {
                    return static_cast<float>(m_value);
                }
                else
                {
                    return m_value ? 1.0F : 0.0F;
                }
            }

            [[nodiscard]] bool GetBool() const override
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    return m_value;
                }
                else if constexpr (std::is_same_v<T, int32>)
                {
                    return m_value != 0;
                }
                else
                {
                    return m_value != 0.0F;
                }
            }

            [[nodiscard]] const TCHAR* GetString() const override { return TEXT(""); }

            void Set(int32 value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                if constexpr (std::is_same_v<T, int32>)
                {
                    m_value = value;
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    m_value = static_cast<float>(value);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    m_value = value != 0;
                }
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(float value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                if constexpr (std::is_same_v<T, float>)
                {
                    m_value = value;
                }
                else if constexpr (std::is_same_v<T, int32>)
                {
                    m_value = static_cast<int32>(value);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    m_value = value != 0.0F;
                }
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(const TCHAR* value, ConsoleVariableFlags flags) override
            {
                if ((value == nullptr) || !CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                if constexpr (std::is_same_v<T, float>)
                {
                    Set(static_cast<float>(std::wcstod(value, nullptr)), flags);
                }
                else
                {
                    // int32 および bool: 文字列→整数変換（boolは0/1として扱う）
                    Set(static_cast<int32>(std::wcstol(value, nullptr, 10)), flags);
                }
            }

            void Reset() override
            {
                m_value = m_defaultValue;
                m_flags = m_flags & ~ConsoleVariableFlags::SetByMask;
                NotifyChanged();
            }

        private:
            T m_value;
            T m_defaultValue;
        };

        // =================================================================
        // 文字列コンソール変数
        // =================================================================
        class ConsoleVariableString : public ConsoleVariableBase
        {
        public:
            ConsoleVariableString(const TCHAR* defaultValue, const TCHAR* help, ConsoleVariableFlags flags)
                : ConsoleVariableBase(help, flags), m_value((defaultValue != nullptr) ? defaultValue : TEXT("")),
                  m_defaultValue((defaultValue != nullptr) ? defaultValue : TEXT(""))
            {}

            [[nodiscard]] int32 GetInt() const override
            {
                return static_cast<int32>(std::wcstol(m_value.c_str(), nullptr, 10));
            }
            [[nodiscard]] float GetFloat() const override
            {
                return static_cast<float>(std::wcstod(m_value.c_str(), nullptr));
            }
            [[nodiscard]] bool GetBool() const override
            {
                return !m_value.empty() && m_value != TEXT("0") && m_value != TEXT("false");
            }
            [[nodiscard]] const TCHAR* GetString() const override { return m_value.c_str(); }

            void Set(int32 value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                m_value = std::to_wstring(value);
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(float value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                m_value = std::to_wstring(value);
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(const TCHAR* value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                m_value = (value != nullptr) ? value : TEXT("");
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Reset() override
            {
                m_value = m_defaultValue;
                m_flags = m_flags & ~ConsoleVariableFlags::SetByMask;
                NotifyChanged();
            }

        private:
            std::wstring m_value;
            std::wstring m_defaultValue;
        };

        // =================================================================
        // 外部変数への参照を保持するコンソール変数
        // =================================================================
        template <typename T> class TConsoleVariableRef : public ConsoleVariableBase
        {
        public:
            TConsoleVariableRef(T& variable, const TCHAR* help, ConsoleVariableFlags flags)
                : ConsoleVariableBase(help, flags), m_ref(variable), m_defaultValue(variable)
            {}

            [[nodiscard]] int32 GetInt() const override
            {
                if constexpr (std::is_same_v<T, int32>)
                {
                    return m_ref;
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    return static_cast<int32>(m_ref);
                }
                else
                {
                    return m_ref ? 1 : 0;
                }
            }

            [[nodiscard]] float GetFloat() const override
            {
                if constexpr (std::is_same_v<T, float>)
                {
                    return m_ref;
                }
                else if constexpr (std::is_same_v<T, int32>)
                {
                    return static_cast<float>(m_ref);
                }
                else
                {
                    return m_ref ? 1.0F : 0.0F;
                }
            }

            [[nodiscard]] bool GetBool() const override
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    return m_ref;
                }
                else if constexpr (std::is_same_v<T, int32>)
                {
                    return m_ref != 0;
                }
                else
                {
                    return m_ref != 0.0F;
                }
            }

            [[nodiscard]] const TCHAR* GetString() const override { return TEXT(""); }

            void Set(int32 value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                if constexpr (std::is_same_v<T, int32>)
                {
                    m_ref = value;
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    m_ref = static_cast<float>(value);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    m_ref = value != 0;
                }
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(float value, ConsoleVariableFlags flags) override
            {
                if (!CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                if constexpr (std::is_same_v<T, float>)
                {
                    m_ref = value;
                }
                else if constexpr (std::is_same_v<T, int32>)
                {
                    m_ref = static_cast<int32>(value);
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    m_ref = value != 0.0F;
                }
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(const TCHAR* value, ConsoleVariableFlags flags) override
            {
                if ((value == nullptr) || !CanSetWithPriority(m_flags, flags))
                {
                    return;
                }
                if constexpr (std::is_same_v<T, float>)
                {
                    Set(static_cast<float>(std::wcstod(value, nullptr)), flags);
                }
                else
                {
                    // int32 および bool: 文字列→整数変換（boolは0/1として扱う）
                    Set(static_cast<int32>(std::wcstol(value, nullptr, 10)), flags);
                }
            }

            void Reset() override
            {
                m_ref = m_defaultValue;
                m_flags = m_flags & ~ConsoleVariableFlags::SetByMask;
                NotifyChanged();
            }

        private:
            T& m_ref;
            T m_defaultValue;
        };

        // =================================================================
        // コンソールコマンドの内部実装
        // =================================================================
        class ConsoleCommandImpl : public IConsoleCommand
        {
        public:
            ConsoleCommandImpl(const TCHAR* help, bool (*command)(const TCHAR*), ConsoleVariableFlags flags)
                : m_help((help != nullptr) ? help : TEXT("")), m_command(command), m_flags(flags)
            {}

            [[nodiscard]] const TCHAR* GetHelp() const override { return m_help.c_str(); }
            void SetHelp(const TCHAR* help) override { m_help = (help != nullptr) ? help : TEXT(""); }
            [[nodiscard]] ConsoleVariableFlags GetFlags() const override { return m_flags; }
            void SetFlags(ConsoleVariableFlags flags) override { m_flags = flags; }
            bool Execute(const TCHAR* args) override { return (m_command != nullptr) ? m_command(args) : false; }

        private:
            std::wstring m_help;
            bool (*m_command)(const TCHAR*);
            ConsoleVariableFlags m_flags;
        };

        // =================================================================
        // コンソールマネージャーの内部実装
        // =================================================================
        class ConsoleManagerImpl : public IConsoleManager
        {
        public:
            ConsoleManagerImpl() = default;

            ~ConsoleManagerImpl() override = default;
            NS_DISALLOW_COPY_AND_MOVE(ConsoleManagerImpl);

        public:
            IConsoleVariable* RegisterConsoleVariable(const TCHAR* name,
                                                      int32 defaultValue,
                                                      const TCHAR* help,
                                                      ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<TConsoleVariable<int32>>(defaultValue, help, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleVariable* RegisterConsoleVariable(const TCHAR* name,
                                                      float defaultValue,
                                                      const TCHAR* help,
                                                      ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<TConsoleVariable<float>>(defaultValue, help, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleVariable* RegisterConsoleVariable(const TCHAR* name,
                                                      const TCHAR* defaultValue,
                                                      const TCHAR* help,
                                                      ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<ConsoleVariableString>(defaultValue, help, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name,
                                                         int32& variable,
                                                         const TCHAR* help,
                                                         ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<TConsoleVariableRef<int32>>(variable, help, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name,
                                                         float& variable,
                                                         const TCHAR* help,
                                                         ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<TConsoleVariableRef<float>>(variable, help, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name,
                                                         bool& variable,
                                                         const TCHAR* help,
                                                         ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<TConsoleVariableRef<bool>>(variable, help, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleCommand* RegisterConsoleCommand(const TCHAR* name,
                                                    const TCHAR* help,
                                                    bool (*command)(const TCHAR*),
                                                    ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto ptr = std::make_unique<ConsoleCommandImpl>(help, command, flags);
                auto* raw = ptr.get();
                m_objects[name] = std::move(ptr);
                return raw;
            }

            IConsoleVariable* FindConsoleVariable(const TCHAR* name) const override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    return it->second->AsVariable();
                }
                return nullptr;
            }

            IConsoleObject* FindConsoleObject(const TCHAR* name) const override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    return it->second.get();
                }
                return nullptr;
            }

            void UnregisterConsoleObject(const TCHAR* name) override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                m_objects.erase(name);
            }

            bool ProcessInput(const TCHAR* input) override
            {
                if ((input == nullptr) || input[0] == L'\0')
                {
                    return false;
                }

                // 入力を "name value" 形式でパース
                std::wstring inputStr(input);

                // 先頭・末尾の空白を除去
                size_t const start = inputStr.find_first_not_of(L" \t");
                if (start == std::wstring::npos)
                {
                    return false;
                }
                size_t const end = inputStr.find_last_not_of(L" \t");
                inputStr = inputStr.substr(start, end - start + 1);

                // 名前と値を分離
                size_t const spacePos = inputStr.find_first_of(L" \t");
                std::wstring const name = (spacePos != std::wstring::npos) ? inputStr.substr(0, spacePos) : inputStr;
                std::wstring valueStr;
                if (spacePos != std::wstring::npos)
                {
                    size_t const valueStart = inputStr.find_first_not_of(L" \t", spacePos);
                    if (valueStart != std::wstring::npos)
                    {
                        valueStr = inputStr.substr(valueStart);
                    }
                }

                std::lock_guard<std::mutex> const lock(m_mutex);
                auto it = m_objects.find(name);
                if (it == m_objects.end())
                {
                    return false;
                }

                IConsoleObject* obj = it->second.get();

                // コマンドの場合
                IConsoleVariable* var = obj->AsVariable();
                if (var == nullptr)
                {
                    // IConsoleCommandとして実行
                    auto* cmd = dynamic_cast<IConsoleCommand*>(obj);
                    return cmd->Execute(valueStr.c_str());
                }

                // 変数の場合：値なしなら現在値のクエリ（何もしない）
                if (valueStr.empty())
                {
                    return true;
                }

                // 値を設定
                var->Set(valueStr.c_str(), ConsoleVariableFlags::SetByConsole);
                return true;
            }

            void ForEachConsoleObject(void (*callback)(const TCHAR* name, IConsoleObject* object)) const override
            {
                std::lock_guard<std::mutex> const lock(m_mutex);
                for (const auto& pair : m_objects)
                {
                    callback(pair.first.c_str(), pair.second.get());
                }
            }

        private:
            mutable std::mutex m_mutex;
            std::unordered_map<std::wstring, std::unique_ptr<IConsoleObject>> m_objects;
        };
    } // namespace

    IConsoleManager& IConsoleManager::Get()
    {
        static ConsoleManagerImpl instance;
        return instance;
    }
} // namespace NS

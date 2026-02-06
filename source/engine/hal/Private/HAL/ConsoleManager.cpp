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

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace NS
{
    namespace
    {
        /// コンソール変数の内部実装
        template <typename T> class TConsoleVariable : public IConsoleVariable
        {
        public:
            TConsoleVariable(T defaultValue, const TCHAR* help, ConsoleVariableFlags flags)
                : m_value(defaultValue), m_defaultValue(defaultValue), m_help(help ? help : TEXT("")), m_flags(flags),
                  m_legacyCallback(nullptr), m_nextCallbackHandle(1)
            {}

            const TCHAR* GetHelp() const override { return m_help.c_str(); }

            void SetHelp(const TCHAR* help) override { m_help = help ? help : TEXT(""); }

            ConsoleVariableFlags GetFlags() const override { return m_flags; }

            void SetFlags(ConsoleVariableFlags flags) override { m_flags = flags; }

            int32 GetInt() const override
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

            float GetFloat() const override
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
                    return m_value ? 1.0f : 0.0f;
                }
            }

            bool GetBool() const override
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
                    return m_value != 0.0f;
                }
            }

            const TCHAR* GetString() const override { return TEXT(""); }

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
                    m_value = value != 0.0f;
                }
                UpdateSetBy(flags);
                NotifyChanged();
            }

            void Set(const TCHAR* value, ConsoleVariableFlags flags) override
            {
                NS_UNUSED(value);
                NS_UNUSED(flags);
            }

            void SetOnChangedCallback(ConsoleVariableDelegate callback) override { m_legacyCallback = callback; }

            ConsoleVariableCallbackHandle AddOnChangedCallback(ConsoleVariableDelegate callback) override
            {
                if (!callback)
                {
                    return kInvalidCallbackHandle;
                }
                ConsoleVariableCallbackHandle handle = m_nextCallbackHandle++;
                m_callbacks.push_back({handle, callback});
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

            void Reset() override
            {
                m_value = m_defaultValue;
                m_flags = m_flags & ~ConsoleVariableFlags::SetByMask;
                NotifyChanged();
            }

        private:
            void UpdateSetBy(ConsoleVariableFlags newFlags)
            {
                m_flags = (m_flags & ~ConsoleVariableFlags::SetByMask) | (newFlags & ConsoleVariableFlags::SetByMask);
            }

            void NotifyChanged()
            {
                if (m_legacyCallback)
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

            T m_value;
            T m_defaultValue;
            std::wstring m_help;
            ConsoleVariableFlags m_flags;
            ConsoleVariableDelegate m_legacyCallback;
            std::vector<CallbackEntry> m_callbacks;
            ConsoleVariableCallbackHandle m_nextCallbackHandle;
        };

        /// コンソールコマンドの内部実装
        class ConsoleCommandImpl : public IConsoleCommand
        {
        public:
            ConsoleCommandImpl(const TCHAR* help, bool (*command)(const TCHAR*), ConsoleVariableFlags flags)
                : m_help(help ? help : TEXT("")), m_command(command), m_flags(flags)
            {}

            const TCHAR* GetHelp() const override { return m_help.c_str(); }

            void SetHelp(const TCHAR* help) override { m_help = help ? help : TEXT(""); }

            ConsoleVariableFlags GetFlags() const override { return m_flags; }

            void SetFlags(ConsoleVariableFlags flags) override { m_flags = flags; }

            bool Execute(const TCHAR* args) override { return m_command ? m_command(args) : false; }

        private:
            std::wstring m_help;
            bool (*m_command)(const TCHAR*);
            ConsoleVariableFlags m_flags;
        };

        /// コンソールマネージャーの内部実装
        class ConsoleManagerImpl : public IConsoleManager
        {
        public:
            ConsoleManagerImpl() = default;

            ~ConsoleManagerImpl() override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (auto& pair : m_objects)
                {
                    delete pair.second;
                }
                m_objects.clear();
            }

            IConsoleVariable* RegisterConsoleVariable(const TCHAR* name,
                                                      int32 defaultValue,
                                                      const TCHAR* help,
                                                      ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    delete it->second;
                }
                auto* var = new TConsoleVariable<int32>(defaultValue, help, flags);
                m_objects[name] = var;
                return var;
            }

            IConsoleVariable* RegisterConsoleVariable(const TCHAR* name,
                                                      float defaultValue,
                                                      const TCHAR* help,
                                                      ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    delete it->second;
                }
                auto* var = new TConsoleVariable<float>(defaultValue, help, flags);
                m_objects[name] = var;
                return var;
            }

            IConsoleVariable* RegisterConsoleVariable(const TCHAR* name,
                                                      const TCHAR* defaultValue,
                                                      const TCHAR* help,
                                                      ConsoleVariableFlags flags) override
            {
                NS_UNUSED(name);
                NS_UNUSED(defaultValue);
                NS_UNUSED(help);
                NS_UNUSED(flags);
                return nullptr;
            }

            IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name,
                                                         int32& variable,
                                                         const TCHAR* help,
                                                         ConsoleVariableFlags flags) override
            {
                NS_UNUSED(name);
                NS_UNUSED(variable);
                NS_UNUSED(help);
                NS_UNUSED(flags);
                return nullptr;
            }

            IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name,
                                                         float& variable,
                                                         const TCHAR* help,
                                                         ConsoleVariableFlags flags) override
            {
                NS_UNUSED(name);
                NS_UNUSED(variable);
                NS_UNUSED(help);
                NS_UNUSED(flags);
                return nullptr;
            }

            IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* name,
                                                         bool& variable,
                                                         const TCHAR* help,
                                                         ConsoleVariableFlags flags) override
            {
                NS_UNUSED(name);
                NS_UNUSED(variable);
                NS_UNUSED(help);
                NS_UNUSED(flags);
                return nullptr;
            }

            IConsoleCommand* RegisterConsoleCommand(const TCHAR* name,
                                                    const TCHAR* help,
                                                    bool (*command)(const TCHAR*),
                                                    ConsoleVariableFlags flags) override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    delete it->second;
                }
                auto* cmd = new ConsoleCommandImpl(help, command, flags);
                m_objects[name] = cmd;
                return cmd;
            }

            IConsoleVariable* FindConsoleVariable(const TCHAR* name) const override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    return it->second->AsVariable();
                }
                return nullptr;
            }

            IConsoleObject* FindConsoleObject(const TCHAR* name) const override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    return it->second;
                }
                return nullptr;
            }

            void UnregisterConsoleObject(const TCHAR* name) override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_objects.find(name);
                if (it != m_objects.end())
                {
                    delete it->second;
                    m_objects.erase(it);
                }
            }

            bool ProcessInput(const TCHAR* input) override
            {
                NS_UNUSED(input);
                return false;
            }

            void ForEachConsoleObject(void (*callback)(const TCHAR* name, IConsoleObject* object)) const override
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& pair : m_objects)
                {
                    callback(pair.first.c_str(), pair.second);
                }
            }

        private:
            mutable std::mutex m_mutex;
            std::unordered_map<std::wstring, IConsoleObject*> m_objects;
        };
    } // namespace

    IConsoleManager& IConsoleManager::Get()
    {
        static ConsoleManagerImpl instance;
        return instance;
    }
} // namespace NS

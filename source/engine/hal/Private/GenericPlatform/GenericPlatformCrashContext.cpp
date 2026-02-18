/// @file GenericPlatformCrashContext.cpp
/// @brief クラッシュコンテキスト基底クラス実装

#include "GenericPlatform/GenericPlatformCrashContext.h"
#include <cstring>
#include <cwchar>

namespace NS
{
    namespace
    {
        /// ポータブルなワイド文字列安全コピー
        void SafeWcsCopy(wchar_t* dest, size_t destSize, const wchar_t* src)
        {
            if (destSize == 0)
            {
                return;
            }
#if defined(_MSC_VER)
            wcsncpy_s(dest, destSize, src, _TRUNCATE);
#else
            std::wcsncpy(dest, src, destSize - 1);
            dest[destSize - 1] = L'\0';
#endif
        }
    } // namespace

    // 静的メンバ初期化
    TCHAR GenericPlatformCrashContext::s_engineVersion[64] = L"Unknown";
    CrashHandlerFunc GenericPlatformCrashContext::s_crashHandler = nullptr;

    GenericPlatformCrashContext::GenericPlatformCrashContext(CrashContextType type) : m_type(type) {}

    void GenericPlatformCrashContext::CaptureContext()
    {
        // 基底クラスでは何もしない
    }

    void GenericPlatformCrashContext::SetErrorMessage(const TCHAR* message)
    {
        if (message != nullptr)
        {
            SafeWcsCopy(m_errorMessage, sizeof(m_errorMessage) / sizeof(m_errorMessage[0]), message);
        }
    }

    void GenericPlatformCrashContext::SetEngineVersion(const TCHAR* version)
    {
        if (version != nullptr)
        {
            SafeWcsCopy(s_engineVersion, sizeof(s_engineVersion) / sizeof(s_engineVersion[0]), version);
        }
    }

    void GenericPlatformCrashContext::SetCrashHandler(CrashHandlerFunc handler)
    {
        s_crashHandler = handler;
    }

    CrashHandlerFunc GenericPlatformCrashContext::GetCrashHandler()
    {
        return s_crashHandler;
    }
} // namespace NS

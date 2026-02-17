/// @file WindowsPlatformCrashContext.cpp
/// @brief Windows固有のクラッシュコンテキスト実装

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "Windows/WindowsPlatformCrashContext.h"
#include <cstring>

namespace NS
{
    // 静的メンバ初期化
    TCHAR GenericPlatformCrashContext::s_engineVersion[64] = L"Unknown";
    CrashHandlerFunc GenericPlatformCrashContext::s_crashHandler = nullptr;

    GenericPlatformCrashContext::GenericPlatformCrashContext(CrashContextType type) : m_type(type)
    {
        m_errorMessage[0] = L'\0';
    }

    void GenericPlatformCrashContext::CaptureContext()
    {
        // 基底クラスでは何もしない
    }

    void GenericPlatformCrashContext::SetErrorMessage(const TCHAR* message)
    {
        if (message)
        {
            wcsncpy_s(m_errorMessage, message, _TRUNCATE);
        }
    }

    void GenericPlatformCrashContext::SetEngineVersion(const TCHAR* version)
    {
        if (version)
        {
            wcsncpy_s(s_engineVersion, version, _TRUNCATE);
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

    // =========================================================================
    // WindowsPlatformCrashContext
    // =========================================================================

    void WindowsPlatformCrashContext::CaptureContext()
    {
        // スタックバックトレースをキャプチャ（CaptureContext自身 + 呼び出し元を2フレームスキップ）
        m_stackDepth = static_cast<int32>(
            ::RtlCaptureStackBackTrace(2, kCrashMaxStackDepth,
                                        reinterpret_cast<void**>(m_stackTrace), nullptr));
    }

    void WindowsPlatformCrashContext::SetUnhandledExceptionFilter()
    {
        ::SetUnhandledExceptionFilter(
            reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(WindowsPlatformCrashContext::UnhandledExceptionFilter));
    }

    long __stdcall WindowsPlatformCrashContext::UnhandledExceptionFilter(void* exceptionPointers)
    {
        CaptureFromException(exceptionPointers);

        if (s_crashHandler)
        {
            s_crashHandler(exceptionPointers);
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void WindowsPlatformCrashContext::CaptureFromException(void* exceptionPointers)
    {
        EXCEPTION_POINTERS* ep = static_cast<EXCEPTION_POINTERS*>(exceptionPointers);
        if (!ep)
        {
            return;
        }

        // 例外コードから種類を判定
        CrashContextType type = CrashContextType::Crash;

        if (ep->ExceptionRecord)
        {
            switch (ep->ExceptionRecord->ExceptionCode)
            {
            case STATUS_HEAP_CORRUPTION:
                type = CrashContextType::OutOfMemory;
                break;
            case EXCEPTION_ACCESS_VIOLATION:
            case EXCEPTION_STACK_OVERFLOW:
            case EXCEPTION_ILLEGAL_INSTRUCTION:
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
            default:
                type = CrashContextType::Crash;
                break;
            }
        }

        WindowsPlatformCrashContext context(type);

        // 例外情報を保存
        if (ep->ExceptionRecord)
        {
            context.m_exceptionCode = ep->ExceptionRecord->ExceptionCode;
            context.m_exceptionAddress = reinterpret_cast<uint64>(ep->ExceptionRecord->ExceptionAddress);
        }

        context.CaptureContext();
    }
} // namespace NS

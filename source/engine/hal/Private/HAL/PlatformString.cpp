/// @file PlatformString.cpp
/// @brief プラットフォーム共通の文字列操作実装

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "HAL/Char.h"
#include "HAL/Platform.h"
#include "HAL/PlatformString.h"

#include <cstring>
#include <cwchar>

namespace NS
{
    int32 GenericPlatformString::Strlen(const ANSICHAR* str)
    {
        return str ? static_cast<int32>(std::strlen(str)) : 0;
    }

    int32 GenericPlatformString::Strlen(const WIDECHAR* str)
    {
        return str ? static_cast<int32>(std::wcslen(str)) : 0;
    }

    int32 GenericPlatformString::Strcmp(const ANSICHAR* a, const ANSICHAR* b)
    {
        if (!a || !b)
        {
            return a ? 1 : (b ? -1 : 0);
        }
        return std::strcmp(a, b);
    }

    int32 GenericPlatformString::Strcmp(const WIDECHAR* a, const WIDECHAR* b)
    {
        if (!a || !b)
        {
            return a ? 1 : (b ? -1 : 0);
        }
        return std::wcscmp(a, b);
    }

    int32 GenericPlatformString::Stricmp(const ANSICHAR* a, const ANSICHAR* b)
    {
        if (!a || !b)
        {
            return a ? 1 : (b ? -1 : 0);
        }

        while (*a && *b)
        {
            ANSICHAR ca = CharAnsi::ToLower(*a);
            ANSICHAR cb = CharAnsi::ToLower(*b);
            if (ca != cb)
            {
                return ca - cb;
            }
            ++a;
            ++b;
        }
        return CharAnsi::ToLower(*a) - CharAnsi::ToLower(*b);
    }

    int32 GenericPlatformString::Stricmp(const WIDECHAR* a, const WIDECHAR* b)
    {
        if (!a || !b)
        {
            return a ? 1 : (b ? -1 : 0);
        }

        while (*a && *b)
        {
            WIDECHAR ca = CharWide::ToLower(*a);
            WIDECHAR cb = CharWide::ToLower(*b);
            if (ca != cb)
            {
                return ca - cb;
            }
            ++a;
            ++b;
        }
        return CharWide::ToLower(*a) - CharWide::ToLower(*b);
    }

    int32 GenericPlatformString::Strncmp(const ANSICHAR* a, const ANSICHAR* b, SIZE_T count)
    {
        if (count == 0)
        {
            return 0;
        }
        if (!a || !b)
        {
            return a ? 1 : (b ? -1 : 0);
        }
        return std::strncmp(a, b, count);
    }

    int32 GenericPlatformString::Strncmp(const WIDECHAR* a, const WIDECHAR* b, SIZE_T count)
    {
        if (count == 0)
        {
            return 0;
        }
        if (!a || !b)
        {
            return a ? 1 : (b ? -1 : 0);
        }
        return std::wcsncmp(a, b, count);
    }

    ANSICHAR* GenericPlatformString::Strcpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src)
    {
        if (!dest || !src || destSize == 0)
        {
            return dest;
        }

#if PLATFORM_WINDOWS
        strcpy_s(dest, destSize, src);
#else
        std::strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
#endif
        return dest;
    }

    WIDECHAR* GenericPlatformString::Strcpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src)
    {
        if (!dest || !src || destSize == 0)
        {
            return dest;
        }

#if PLATFORM_WINDOWS
        wcscpy_s(dest, destSize, src);
#else
        std::wcsncpy(dest, src, destSize - 1);
        dest[destSize - 1] = L'\0';
#endif
        return dest;
    }

    ANSICHAR* GenericPlatformString::Strncpy(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src, SIZE_T count)
    {
        if (!dest || !src || destSize == 0)
        {
            return dest;
        }

        SIZE_T copyLen = (count < destSize - 1) ? count : destSize - 1;

#if PLATFORM_WINDOWS
        strncpy_s(dest, destSize, src, copyLen);
#else
        std::strncpy(dest, src, copyLen);
        dest[copyLen] = '\0';
#endif
        return dest;
    }

    WIDECHAR* GenericPlatformString::Strncpy(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src, SIZE_T count)
    {
        if (!dest || !src || destSize == 0)
        {
            return dest;
        }

        SIZE_T copyLen = (count < destSize - 1) ? count : destSize - 1;

#if PLATFORM_WINDOWS
        wcsncpy_s(dest, destSize, src, copyLen);
#else
        std::wcsncpy(dest, src, copyLen);
        dest[copyLen] = L'\0';
#endif
        return dest;
    }

    ANSICHAR* GenericPlatformString::Strcat(ANSICHAR* dest, SIZE_T destSize, const ANSICHAR* src)
    {
        if (!dest || !src || destSize == 0)
        {
            return dest;
        }

#if PLATFORM_WINDOWS
        strcat_s(dest, destSize, src);
#else
        SIZE_T destLen = std::strlen(dest);
        if (destLen < destSize - 1)
        {
            std::strncat(dest, src, destSize - destLen - 1);
        }
#endif
        return dest;
    }

    WIDECHAR* GenericPlatformString::Strcat(WIDECHAR* dest, SIZE_T destSize, const WIDECHAR* src)
    {
        if (!dest || !src || destSize == 0)
        {
            return dest;
        }

#if PLATFORM_WINDOWS
        wcscat_s(dest, destSize, src);
#else
        SIZE_T destLen = std::wcslen(dest);
        if (destLen < destSize - 1)
        {
            std::wcsncat(dest, src, destSize - destLen - 1);
        }
#endif
        return dest;
    }

    const ANSICHAR* GenericPlatformString::Strstr(const ANSICHAR* str, const ANSICHAR* find)
    {
        if (!str || !find)
        {
            return nullptr;
        }
        return std::strstr(str, find);
    }

    const WIDECHAR* GenericPlatformString::Strstr(const WIDECHAR* str, const WIDECHAR* find)
    {
        if (!str || !find)
        {
            return nullptr;
        }
        return std::wcsstr(str, find);
    }

    const ANSICHAR* GenericPlatformString::Strchr(const ANSICHAR* str, ANSICHAR c)
    {
        if (!str)
        {
            return nullptr;
        }
        return std::strchr(str, c);
    }

    const WIDECHAR* GenericPlatformString::Strchr(const WIDECHAR* str, WIDECHAR c)
    {
        if (!str)
        {
            return nullptr;
        }
        return std::wcschr(str, c);
    }

    const ANSICHAR* GenericPlatformString::Strrchr(const ANSICHAR* str, ANSICHAR c)
    {
        if (!str)
        {
            return nullptr;
        }
        return std::strrchr(str, c);
    }

    const WIDECHAR* GenericPlatformString::Strrchr(const WIDECHAR* str, WIDECHAR c)
    {
        if (!str)
        {
            return nullptr;
        }
        return std::wcsrchr(str, c);
    }
} // namespace NS

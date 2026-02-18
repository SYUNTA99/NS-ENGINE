/// @file WindowsPlatformFile.cpp
/// @brief Windows固有のファイルI/O実装

// Windows APIマクロとの衝突を避けるため、windows.hを先にインクルード
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Windows APIと衝突するマクロをundef
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#undef CreateDirectory

#include "Windows/WindowsPlatformFile.h"

namespace NS
{
    // =========================================================================
    // WindowsFileHandle
    // =========================================================================

    WindowsFileHandle::WindowsFileHandle(void* handle) : m_handle(handle) {}

    WindowsFileHandle::~WindowsFileHandle()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
        }
    }

    int64 WindowsFileHandle::Tell()
    {
        LARGE_INTEGER const zero = {};
        LARGE_INTEGER pos;
        if (SetFilePointerEx(m_handle, zero, &pos, FILE_CURRENT) == 0)
        {
            return -1;
        }
        return static_cast<int64>(pos.QuadPart);
    }

    bool WindowsFileHandle::Seek(int64 newPosition)
    {
        LARGE_INTEGER pos;
        pos.QuadPart = newPosition;
        return SetFilePointerEx(m_handle, pos, nullptr, FILE_BEGIN) != 0;
    }

    bool WindowsFileHandle::SeekFromEnd(int64 offset)
    {
        LARGE_INTEGER pos;
        pos.QuadPart = offset;
        return SetFilePointerEx(m_handle, pos, nullptr, FILE_END) != 0;
    }

    bool WindowsFileHandle::Read(uint8* dest, int64 bytesToRead)
    {
        while (bytesToRead > 0)
        {
            // 一度に読み取れるのはDWORD_MAXまで
            auto const toRead = static_cast<DWORD>((bytesToRead > MAXDWORD) ? MAXDWORD : bytesToRead);
            DWORD bytesRead = 0;

            if (ReadFile(m_handle, dest, toRead, &bytesRead, nullptr) == 0)
            {
                return false;
            }

            if (bytesRead != toRead)
            {
                return false;
            }

            dest += bytesRead;
            bytesToRead -= bytesRead;
        }
        return true;
    }

    bool WindowsFileHandle::Write(const uint8* src, int64 bytesToWrite)
    {
        while (bytesToWrite > 0)
        {
            auto const toWrite = static_cast<DWORD>((bytesToWrite > MAXDWORD) ? MAXDWORD : bytesToWrite);
            DWORD bytesWritten = 0;

            if (WriteFile(m_handle, src, toWrite, &bytesWritten, nullptr) == 0)
            {
                return false;
            }

            if (bytesWritten != toWrite)
            {
                return false;
            }

            src += bytesWritten;
            bytesToWrite -= bytesWritten;
        }
        return true;
    }

    bool WindowsFileHandle::Flush()
    {
        return FlushFileBuffers(m_handle) != 0;
    }

    int64 WindowsFileHandle::Size()
    {
        LARGE_INTEGER size;
        if (GetFileSizeEx(m_handle, &size) == 0)
        {
            return -1;
        }
        return static_cast<int64>(size.QuadPart);
    }

    // =========================================================================
    // WindowsPlatformFile
    // =========================================================================

    bool WindowsPlatformFile::FileExists(const TCHAR* filename)
    {
        DWORD const attributes = GetFileAttributesW(filename);
        return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0U);
    }

    bool WindowsPlatformFile::DirectoryExists(const TCHAR* directory)
    {
        DWORD const attributes = GetFileAttributesW(directory);
        return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0U);
    }

    int64 WindowsPlatformFile::FileSize(const TCHAR* filename)
    {
        WIN32_FILE_ATTRIBUTE_DATA data;
        if (GetFileAttributesExW(filename, GetFileExInfoStandard, &data) == 0)
        {
            return -1;
        }
        LARGE_INTEGER size;
        size.HighPart = static_cast<LONG>(data.nFileSizeHigh);
        size.LowPart = data.nFileSizeLow;
        return static_cast<int64>(size.QuadPart);
    }

    bool WindowsPlatformFile::DeleteFile(const TCHAR* filename)
    {
        return ::DeleteFileW(filename) != 0;
    }

    bool WindowsPlatformFile::MoveFile(const TCHAR* to, const TCHAR* from)
    {
        return ::MoveFileW(from, to) != 0;
    }

    bool WindowsPlatformFile::CopyFile(const TCHAR* to, const TCHAR* from)
    {
        return ::CopyFileW(from, to, FALSE) != 0;
    }

    bool WindowsPlatformFile::IsReadOnly(const TCHAR* filename)
    {
        DWORD const attributes = GetFileAttributesW(filename);
        return (attributes != INVALID_FILE_ATTRIBUTES) && ((attributes & FILE_ATTRIBUTE_READONLY) != 0U);
    }

    bool WindowsPlatformFile::SetReadOnly(const TCHAR* filename, bool readOnly)
    {
        DWORD attributes = GetFileAttributesW(filename);
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            return false;
        }

        if (readOnly)
        {
            attributes |= FILE_ATTRIBUTE_READONLY;
        }
        else
        {
            attributes &= ~FILE_ATTRIBUTE_READONLY;
        }

        return SetFileAttributesW(filename, attributes) != 0;
    }

    bool WindowsPlatformFile::CreateDirectory(const TCHAR* directory)
    {
        if (::CreateDirectoryW(directory, nullptr) != 0)
        {
            return true;
        }
        // 既存の場合もtrue
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    bool WindowsPlatformFile::DeleteDirectory(const TCHAR* directory)
    {
        return ::RemoveDirectoryW(directory) != 0;
    }

    bool WindowsPlatformFile::CreateDirectoryTree(const TCHAR* directory)
    {
        // パスを走査して各階層を作成
        wchar_t path[MAX_PATH];
        wcsncpy_s(path, directory, MAX_PATH - 1);

        // 末尾のセパレータを削除
        size_t const len = wcslen(path);
        if (len > 0 && (path[len - 1] == L'\\' || path[len - 1] == L'/'))
        {
            path[len - 1] = L'\0';
        }

        // 各階層を順番に作成
        for (wchar_t* p = path + 1; *p != 0U; ++p)
        {
            if (*p == L'\\' || *p == L'/')
            {
                *p = L'\0';
                if (!CreateDirectory(path))
                {
                    DWORD const error = GetLastError();
                    if (error != ERROR_ALREADY_EXISTS)
                    {
                        return false;
                    }
                }
                *p = L'\\';
            }
        }

        return CreateDirectory(path);
    }

    std::unique_ptr<IFileHandle> WindowsPlatformFile::OpenRead(const TCHAR* filename)
    {
        HANDLE handle = CreateFileW(
            filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }
        return std::make_unique<WindowsFileHandle>(handle);
    }

    std::unique_ptr<IFileHandle> WindowsPlatformFile::OpenWrite(const TCHAR* filename, bool append, bool allowRead)
    {
        DWORD access = GENERIC_WRITE;
        if (allowRead)
        {
            access |= GENERIC_READ;
        }

        DWORD const shareMode = allowRead ? FILE_SHARE_READ : 0;
        DWORD const creation = append ? OPEN_ALWAYS : CREATE_ALWAYS;

        HANDLE handle = CreateFileW(filename, access, shareMode, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }

        if (append)
        {
            SetFilePointer(handle, 0, nullptr, FILE_END);
        }

        return std::make_unique<WindowsFileHandle>(handle);
    }

    // =========================================================================
    // グローバル関数
    // =========================================================================

    static WindowsPlatformFile g_sPlatformFile;

    IPlatformFile& GetPlatformFile()
    {
        return g_sPlatformFile;
    }
} // namespace NS

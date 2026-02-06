# 03-05b: Windowsファイル実装

## 目的

Windows固有のファイルI/Oを実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション3「ファイルI/O」

## 依存（commonモジュール）

- `common/utility/macros.h` - `NS_DISALLOW_COPY_AND_MOVE`

## 依存（HAL）

- 01-03 COMPILED_PLATFORM_HEADER
- 03-05a ファイルインターフェース

## 必要なヘッダ（Windows）

- `<windows.h>` - `CreateFileW`, `ReadFile`, `WriteFile`, `CloseHandle`
- `<fileapi.h>` - `GetFileAttributesW`, `CreateDirectoryW`, `RemoveDirectoryW`

## 変更対象

新規作成:
- `source/engine/hal/Public/Windows/WindowsPlatformFile.h`
- `source/engine/hal/Public/HAL/PlatformFile.h`
- `source/engine/hal/Private/Windows/WindowsPlatformFile.cpp`

## TODO

- [ ] `WindowsFileHandle` クラス実装
- [ ] `WindowsPlatformFile` クラス実装
- [ ] ファイル存在チェック実装
- [ ] ファイル読み書き実装
- [ ] ディレクトリ操作実装
- [ ] `GetPlatformFile()` グローバル関数実装

## 実装内容

### WindowsPlatformFile.h

```cpp
#pragma once

#include "GenericPlatform/GenericPlatformFile.h"

namespace NS
{
    class WindowsFileHandle : public IFileHandle
    {
    public:
        explicit WindowsFileHandle(void* handle);
        ~WindowsFileHandle();

        int64 Tell() override;
        bool Seek(int64 newPosition) override;
        bool SeekFromEnd(int64 offset) override;
        bool Read(uint8* dest, int64 bytesToRead) override;
        bool Write(const uint8* src, int64 bytesToWrite) override;
        bool Flush() override;
        int64 Size() override;

    private:
        void* m_handle;

        NS_DISALLOW_COPY_AND_MOVE(WindowsFileHandle);
    };

    class WindowsPlatformFile : public IPlatformFile
    {
    public:
        bool FileExists(const TCHAR* filename) override;
        bool DirectoryExists(const TCHAR* directory) override;
        int64 FileSize(const TCHAR* filename) override;

        bool DeleteFile(const TCHAR* filename) override;
        bool MoveFile(const TCHAR* to, const TCHAR* from) override;
        bool CopyFile(const TCHAR* to, const TCHAR* from) override;
        bool IsReadOnly(const TCHAR* filename) override;
        bool SetReadOnly(const TCHAR* filename, bool readOnly) override;

        bool CreateDirectory(const TCHAR* directory) override;
        bool DeleteDirectory(const TCHAR* directory) override;
        bool CreateDirectoryTree(const TCHAR* directory) override;

        IFileHandle* OpenRead(const TCHAR* filename) override;
        IFileHandle* OpenWrite(const TCHAR* filename, bool append, bool allowRead) override;
    };

    typedef WindowsPlatformFile PlatformFile;
}
```

### WindowsPlatformFile.cpp（抜粋）

```cpp
#include "Windows/WindowsPlatformFile.h"
#include <windows.h>

namespace NS
{
    // WindowsFileHandle
    WindowsFileHandle::WindowsFileHandle(void* handle)
        : m_handle(handle)
    {
    }

    WindowsFileHandle::~WindowsFileHandle()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
        }
    }

    int64 WindowsFileHandle::Tell()
    {
        LARGE_INTEGER pos;
        LARGE_INTEGER zero = {};
        SetFilePointerEx(m_handle, zero, &pos, FILE_CURRENT);
        return pos.QuadPart;
    }

    bool WindowsFileHandle::Read(uint8* dest, int64 bytesToRead)
    {
        DWORD bytesRead;
        return ReadFile(m_handle, dest, static_cast<DWORD>(bytesToRead), &bytesRead, nullptr)
               && bytesRead == static_cast<DWORD>(bytesToRead);
    }

    // WindowsPlatformFile
    bool WindowsPlatformFile::FileExists(const TCHAR* filename)
    {
        DWORD attributes = GetFileAttributesW(filename);
        return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    IFileHandle* WindowsPlatformFile::OpenRead(const TCHAR* filename)
    {
        HANDLE handle = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }
        return new WindowsFileHandle(handle);
    }

    // GetPlatformFile singleton
    static WindowsPlatformFile s_platformFile;

    IPlatformFile& GetPlatformFile()
    {
        return s_platformFile;
    }
}
```

## 検証

- `GetPlatformFile().FileExists(L"C:\\Windows\\notepad.exe")` が true
- ファイル読み書きが正常動作
- `OpenRead()` が存在しないファイルで nullptr を返す

## 注意事項

- `INVALID_HANDLE_VALUE` チェック必須
- 大きなファイルは `int64` で扱う
- `OpenRead`/`OpenWrite` の戻り値は呼び出し側が `delete` する責任

## 次のサブ計画

→ 04-01: premake HALプロジェクト

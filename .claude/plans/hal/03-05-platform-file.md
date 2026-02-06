# 03-05: PlatformFile（統合計画 → 細分化済み）

> **注意**: この計画は細分化されました。以下を参照してください：
> - [03-05a: ファイルインターフェース](03-05a-file-interface.md)
> - [03-05b: Windowsファイル実装](03-05b-windows-file.md)

---

# 03-05: PlatformFile（旧）

## 目的

ファイルI/Oの抽象化レイヤーを実装する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション3「ファイルI/O」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`int64`, `uint8`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformFile.h`
- `source/engine/hal/Public/Windows/WindowsPlatformFile.h`
- `source/engine/hal/Public/HAL/PlatformFile.h`
- `source/engine/hal/Private/Windows/WindowsPlatformFile.cpp`

## TODO

- [ ] `IFileHandle` インターフェース（読み書き操作）
- [ ] `IPlatformFile` インターフェース（ファイルシステム操作）
- [ ] `WindowsPlatformFile` 実装
- [ ] `PlatformFile.h` エントリポイント

## 実装内容

```cpp
// GenericPlatformFile.h
namespace NS
{
    class IFileHandle
    {
    public:
        virtual ~IFileHandle() = default;

        virtual int64 Tell() = 0;
        virtual bool Seek(int64 newPosition) = 0;
        virtual bool SeekFromEnd(int64 offset = 0) = 0;

        virtual bool Read(uint8* dest, int64 bytesToRead) = 0;
        virtual bool Write(const uint8* src, int64 bytesToWrite) = 0;
        virtual bool Flush() = 0;

        virtual int64 Size() = 0;
    };

    class IPlatformFile
    {
    public:
        virtual ~IPlatformFile() = default;

        virtual bool FileExists(const TCHAR* filename) = 0;
        virtual bool DirectoryExists(const TCHAR* directory) = 0;
        virtual int64 FileSize(const TCHAR* filename) = 0;

        virtual bool DeleteFile(const TCHAR* filename) = 0;
        virtual bool MoveFile(const TCHAR* to, const TCHAR* from) = 0;
        virtual bool CopyFile(const TCHAR* to, const TCHAR* from) = 0;

        virtual bool CreateDirectory(const TCHAR* directory) = 0;
        virtual bool DeleteDirectory(const TCHAR* directory) = 0;

        virtual IFileHandle* OpenRead(const TCHAR* filename) = 0;
        virtual IFileHandle* OpenWrite(const TCHAR* filename, bool append = false) = 0;
    };

    IPlatformFile& GetPlatformFile();
}
```

## 検証

- ファイル読み書きが正常動作
- ディレクトリ操作が正常動作
- FileExists が存在/非存在を正しく判定

## 必要なヘッダ（Windows実装）

- `<windows.h>` - `CreateFileW`, `ReadFile`, `WriteFile`, `CloseHandle`
- `<fileapi.h>` - `GetFileAttributesW`, `CreateDirectoryW`, `RemoveDirectoryW`

## 実装詳細

### WindowsFileHandle クラス

```cpp
class WindowsFileHandle : public IFileHandle
{
public:
    WindowsFileHandle(HANDLE handle) : m_handle(handle) {}

    ~WindowsFileHandle()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
        }
    }

    int64 Tell() override
    {
        LARGE_INTEGER pos;
        LARGE_INTEGER zero = {};
        SetFilePointerEx(m_handle, zero, &pos, FILE_CURRENT);
        return pos.QuadPart;
    }

    bool Read(uint8* dest, int64 bytesToRead) override
    {
        DWORD bytesRead;
        return ReadFile(m_handle, dest, (DWORD)bytesToRead, &bytesRead, nullptr) && bytesRead == bytesToRead;
    }

    bool Write(const uint8* src, int64 bytesToWrite) override
    {
        DWORD bytesWritten;
        return WriteFile(m_handle, src, (DWORD)bytesToWrite, &bytesWritten, nullptr) && bytesWritten == bytesToWrite;
    }

private:
    HANDLE m_handle;
};
```

## 注意事項

- `INVALID_HANDLE_VALUE` チェック必須
- 大きなファイルは `int64` で扱う（4GB超対応）
- `CreateFileW` の共有モード（FILE_SHARE_READ等）に注意
- ファイルパスは `TCHAR*`（Windowsでは `wchar_t*`）

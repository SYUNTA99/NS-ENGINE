/// @file WindowsPlatformFile.h
/// @brief Windows固有のファイルI/O実装
#pragma once

#include "GenericPlatform/GenericPlatformFile.h"
#include "common/utility/macros.h"

namespace NS
{
    /// Windows固有のファイルハンドル実装
    class WindowsFileHandle : public IFileHandle
    {
    public:
        explicit WindowsFileHandle(void* handle);
        ~WindowsFileHandle() override;

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

    /// Windows固有のプラットフォームファイルシステム実装
    class WindowsPlatformFile : public IPlatformFile
    {
    public:
        // =====================================================================
        // 存在チェック
        // =====================================================================

        bool FileExists(const TCHAR* filename) override;
        bool DirectoryExists(const TCHAR* directory) override;
        int64 FileSize(const TCHAR* filename) override;

        // =====================================================================
        // ファイル操作
        // =====================================================================

        bool DeleteFile(const TCHAR* filename) override;
        bool MoveFile(const TCHAR* to, const TCHAR* from) override;
        bool CopyFile(const TCHAR* to, const TCHAR* from) override;
        bool IsReadOnly(const TCHAR* filename) override;
        bool SetReadOnly(const TCHAR* filename, bool readOnly) override;

        // =====================================================================
        // ディレクトリ操作
        // =====================================================================

        bool CreateDirectory(const TCHAR* directory) override;
        bool DeleteDirectory(const TCHAR* directory) override;
        bool CreateDirectoryTree(const TCHAR* directory) override;

        // =====================================================================
        // ファイルオープン
        // =====================================================================

        IFileHandle* OpenRead(const TCHAR* filename) override;
        IFileHandle* OpenWrite(const TCHAR* filename, bool append, bool allowRead) override;
    };

    /// 現在のプラットフォームのファイルシステム
    using PlatformFile = WindowsPlatformFile;
} // namespace NS

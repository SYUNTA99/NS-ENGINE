# 03-05a: ファイルインターフェース

## 目的

ファイルI/Oのインターフェースを定義する。

## 参考

[Platform_Abstraction_Layer_Part3.md](docs/Platform_Abstraction_Layer_Part3.md) - セクション3「ファイルI/O」

## 依存（commonモジュール）

- `common/utility/types.h` - 基本型（`int64`, `uint8`）

## 依存（HAL）

- 01-04 プラットフォーム型（`TCHAR`）

## 変更対象

新規作成:
- `source/engine/hal/Public/GenericPlatform/GenericPlatformFile.h`

## TODO

- [ ] `IFileHandle` インターフェース定義
- [ ] `IPlatformFile` インターフェース定義
- [ ] ファイルアクセスモード列挙型

## 実装内容

```cpp
// GenericPlatformFile.h
#pragma once

#include "common/utility/types.h"
#include "HAL/PlatformTypes.h"

namespace NS
{
    /// ファイルハンドル抽象インターフェース
    class IFileHandle
    {
    public:
        virtual ~IFileHandle() = default;

        /// 現在のファイル位置を取得
        virtual int64 Tell() = 0;

        /// ファイル位置を設定
        /// @return 成功した場合true
        virtual bool Seek(int64 newPosition) = 0;

        /// ファイル末尾からの相対位置に設定
        /// @param offset 末尾からのオフセット（通常は0以下）
        virtual bool SeekFromEnd(int64 offset = 0) = 0;

        /// データを読み取る
        /// @param dest 読み取り先バッファ
        /// @param bytesToRead 読み取るバイト数
        /// @return 成功した場合true
        virtual bool Read(uint8* dest, int64 bytesToRead) = 0;

        /// データを書き込む
        /// @param src 書き込むデータ
        /// @param bytesToWrite 書き込むバイト数
        /// @return 成功した場合true
        virtual bool Write(const uint8* src, int64 bytesToWrite) = 0;

        /// バッファをフラッシュ
        virtual bool Flush() = 0;

        /// ファイルサイズを取得
        virtual int64 Size() = 0;
    };

    /// プラットフォームファイルシステム抽象インターフェース
    class IPlatformFile
    {
    public:
        virtual ~IPlatformFile() = default;

        // 存在チェック
        virtual bool FileExists(const TCHAR* filename) = 0;
        virtual bool DirectoryExists(const TCHAR* directory) = 0;
        virtual int64 FileSize(const TCHAR* filename) = 0;

        // ファイル操作
        virtual bool DeleteFile(const TCHAR* filename) = 0;
        virtual bool MoveFile(const TCHAR* to, const TCHAR* from) = 0;
        virtual bool CopyFile(const TCHAR* to, const TCHAR* from) = 0;
        virtual bool IsReadOnly(const TCHAR* filename) = 0;
        virtual bool SetReadOnly(const TCHAR* filename, bool readOnly) = 0;

        // ディレクトリ操作
        virtual bool CreateDirectory(const TCHAR* directory) = 0;
        virtual bool DeleteDirectory(const TCHAR* directory) = 0;
        virtual bool CreateDirectoryTree(const TCHAR* directory) = 0;

        // ファイルオープン
        virtual IFileHandle* OpenRead(const TCHAR* filename) = 0;
        virtual IFileHandle* OpenWrite(const TCHAR* filename, bool append = false, bool allowRead = false) = 0;
    };

    /// グローバルプラットフォームファイル取得
    IPlatformFile& GetPlatformFile();
}
```

## 検証

- ヘッダがコンパイル可能
- インターフェースが正しく定義される

## 次のサブ計画

→ 03-05b: Windowsファイル実装

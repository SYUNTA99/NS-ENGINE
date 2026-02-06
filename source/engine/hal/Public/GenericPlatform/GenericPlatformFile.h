/// @file GenericPlatformFile.h
/// @brief プラットフォーム非依存のファイルI/Oインターフェース
#pragma once

#include "HAL/PlatformTypes.h"
#include "common/utility/types.h"

namespace NS
{
    /// ファイルハンドル抽象インターフェース
    ///
    /// ## 所有権
    ///
    /// OpenRead/OpenWriteで取得したハンドルは呼び出し側がdelete責任を持つ。
    ///
    /// ## スレッドセーフティ
    ///
    /// 単一のIFileHandleインスタンスを複数スレッドから同時に使用することは安全ではない。
    class IFileHandle
    {
    public:
        virtual ~IFileHandle() = default;

        /// 現在のファイル位置を取得
        ///
        /// @return 現在位置（バイト単位）
        virtual int64 Tell() = 0;

        /// ファイル位置を設定
        ///
        /// @param newPosition 新しい位置（バイト単位、先頭から）
        /// @return 成功した場合true
        virtual bool Seek(int64 newPosition) = 0;

        /// ファイル末尾からの相対位置に設定
        ///
        /// @param offset 末尾からのオフセット（0=末尾、負値=末尾より前）
        /// @return 成功した場合true
        virtual bool SeekFromEnd(int64 offset = 0) = 0;

        /// データを読み取る
        ///
        /// @param dest 読み取り先バッファ
        /// @param bytesToRead 読み取るバイト数
        /// @return 要求されたバイト数を完全に読み取れた場合true
        virtual bool Read(uint8* dest, int64 bytesToRead) = 0;

        /// データを書き込む
        ///
        /// @param src 書き込むデータ
        /// @param bytesToWrite 書き込むバイト数
        /// @return 成功した場合true
        virtual bool Write(const uint8* src, int64 bytesToWrite) = 0;

        /// バッファをフラッシュ
        ///
        /// @return 成功した場合true
        virtual bool Flush() = 0;

        /// ファイルサイズを取得
        ///
        /// @return ファイルサイズ（バイト単位）
        virtual int64 Size() = 0;
    };

    /// プラットフォームファイルシステム抽象インターフェース
    ///
    /// ## 使用パターン
    ///
    /// ```cpp
    /// IPlatformFile& fs = GetPlatformFile();
    /// if (fs.FileExists(L"config.ini"))
    /// {
    ///     IFileHandle* file = fs.OpenRead(L"config.ini");
    ///     if (file)
    ///     {
    ///         // ... 読み取り処理 ...
    ///         delete file;
    ///     }
    /// }
    /// ```
    class IPlatformFile
    {
    public:
        virtual ~IPlatformFile() = default;

        // =====================================================================
        // 存在チェック
        // =====================================================================

        /// ファイルが存在するかチェック
        ///
        /// @param filename ファイルパス
        /// @return ファイルが存在すればtrue（ディレクトリはfalse）
        virtual bool FileExists(const TCHAR* filename) = 0;

        /// ディレクトリが存在するかチェック
        ///
        /// @param directory ディレクトリパス
        /// @return ディレクトリが存在すればtrue
        virtual bool DirectoryExists(const TCHAR* directory) = 0;

        /// ファイルサイズを取得
        ///
        /// @param filename ファイルパス
        /// @return ファイルサイズ（バイト単位）、エラー時-1
        virtual int64 FileSize(const TCHAR* filename) = 0;

        // =====================================================================
        // ファイル操作
        // =====================================================================

        /// ファイルを削除
        ///
        /// @param filename ファイルパス
        /// @return 成功した場合true
        virtual bool DeleteFile(const TCHAR* filename) = 0;

        /// ファイルを移動（リネーム）
        ///
        /// @param to 移動先パス
        /// @param from 移動元パス
        /// @return 成功した場合true
        virtual bool MoveFile(const TCHAR* to, const TCHAR* from) = 0;

        /// ファイルをコピー
        ///
        /// @param to コピー先パス
        /// @param from コピー元パス
        /// @return 成功した場合true
        virtual bool CopyFile(const TCHAR* to, const TCHAR* from) = 0;

        /// ファイルが読み取り専用かチェック
        ///
        /// @param filename ファイルパス
        /// @return 読み取り専用ならtrue
        virtual bool IsReadOnly(const TCHAR* filename) = 0;

        /// ファイルの読み取り専用属性を設定
        ///
        /// @param filename ファイルパス
        /// @param readOnly true=読み取り専用、false=書き込み可能
        /// @return 成功した場合true
        virtual bool SetReadOnly(const TCHAR* filename, bool readOnly) = 0;

        // =====================================================================
        // ディレクトリ操作
        // =====================================================================

        /// ディレクトリを作成（単一階層）
        ///
        /// @param directory ディレクトリパス
        /// @return 成功した場合true（既存の場合もtrue）
        virtual bool CreateDirectory(const TCHAR* directory) = 0;

        /// ディレクトリを削除
        ///
        /// @param directory ディレクトリパス
        /// @return 成功した場合true
        /// @note ディレクトリが空でない場合は失敗
        virtual bool DeleteDirectory(const TCHAR* directory) = 0;

        /// ディレクトリツリーを作成（親ディレクトリも含めて作成）
        ///
        /// @param directory ディレクトリパス
        /// @return 成功した場合true
        virtual bool CreateDirectoryTree(const TCHAR* directory) = 0;

        // =====================================================================
        // ファイルオープン
        // =====================================================================

        /// ファイルを読み取り用に開く
        ///
        /// @param filename ファイルパス
        /// @return ファイルハンドル、失敗時nullptr
        /// @note 戻り値は呼び出し側がdeleteする責任
        virtual IFileHandle* OpenRead(const TCHAR* filename) = 0;

        /// ファイルを書き込み用に開く
        ///
        /// @param filename ファイルパス
        /// @param append true=追記モード、false=上書きモード
        /// @param allowRead true=読み取りも許可
        /// @return ファイルハンドル、失敗時nullptr
        /// @note 戻り値は呼び出し側がdeleteする責任
        virtual IFileHandle* OpenWrite(const TCHAR* filename, bool append = false, bool allowRead = false) = 0;
    };

    /// グローバルプラットフォームファイルシステム取得
    ///
    /// @return プラットフォーム固有のファイルシステム実装
    IPlatformFile& GetPlatformFile();
} // namespace NS

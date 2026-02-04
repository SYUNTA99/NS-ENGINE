#pragma once

#include <cstdint>
#include <string>

namespace NS
{
    class Result;
}

//! ファイルエラー情報
struct FileError
{
    //! エラーコード
    enum class Code
    {
        None,           //!< エラーなし
        NotFound,       //!< ファイル/ディレクトリが見つからない
        AccessDenied,   //!< アクセス権限がない
        InvalidPath,    //!< パス形式が不正
        InvalidMount,   //!< マウントが見つからない/不正
        DiskFull,       //!< ディスク容量不足
        AlreadyExists,  //!< 既に存在する
        NotEmpty,       //!< ディレクトリが空でない
        IsDirectory,    //!< ディレクトリに対してファイル操作を行った
        IsNotDirectory, //!< ファイルに対してディレクトリ操作を行った
        PathTooLong,    //!< パスが長すぎる
        ReadOnly,       //!< 読み取り専用
        Cancelled,      //!< 操作がキャンセルされた
        Unknown,        //!< 不明なエラー
    };

    Code code = Code::None;  //!< 抽象エラーコード
    int32_t nativeError = 0; //!< OS固有エラーコード（GetLastError等）
    std::string context;     //!< 追加コンテキスト（パスなど）

    //! エラーがないか確認
    [[nodiscard]] bool isOk() const noexcept { return code == Code::None; }

    //! エラーメッセージを生成
    //! @return 人間が読める形式のエラーメッセージ
    [[nodiscard]] std::string message() const;

    //! 静的ファクトリ: コンテキスト付きエラー生成
    [[nodiscard]] static FileError make(Code code, int32_t nativeError = 0, const std::string& context = {})
    {
        return FileError{code, nativeError, context};
    }

    //! NS::Result に変換
    //! @return 対応するNS::Result値
    [[nodiscard]] NS::Result ToResult() const noexcept;

    //! NS::Result から FileError を生成（互換性用）
    //! @param result 変換元のResult
    //! @return 対応するFileError（contextは空）
    [[nodiscard]] static FileError FromResult(NS::Result result) noexcept;
};

//! エラーコードを文字列に変換
[[nodiscard]] const char* FileErrorToString(FileError::Code code) noexcept;

#include "file_error.h"

#include "common/result/Module/CommonResult.h"
#include "common/result/Module/FileSystemResult.h"

const char* FileErrorToString(FileError::Code code) noexcept
{
    switch (code)
    {
    case FileError::Code::None:
        return "None";
    case FileError::Code::NotFound:
        return "NotFound";
    case FileError::Code::AccessDenied:
        return "AccessDenied";
    case FileError::Code::InvalidPath:
        return "InvalidPath";
    case FileError::Code::InvalidMount:
        return "InvalidMount";
    case FileError::Code::DiskFull:
        return "DiskFull";
    case FileError::Code::AlreadyExists:
        return "AlreadyExists";
    case FileError::Code::NotEmpty:
        return "NotEmpty";
    case FileError::Code::IsDirectory:
        return "IsDirectory";
    case FileError::Code::IsNotDirectory:
        return "IsNotDirectory";
    case FileError::Code::PathTooLong:
        return "PathTooLong";
    case FileError::Code::ReadOnly:
        return "ReadOnly";
    case FileError::Code::Cancelled:
        return "Cancelled";
    case FileError::Code::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

std::string FileError::message() const
{
    std::string msg = FileErrorToString(code);

    if (!context.empty())
    {
        msg += ": ";
        msg += context;
    }

    if (nativeError != 0)
    {
        msg += " (native error: ";
        msg += std::to_string(nativeError);
        msg += ")";
    }

    return msg;
}

NS::Result FileError::ToResult() const noexcept
{
    using namespace NS::FileSystemResult;

    switch (code)
    {
    case Code::None:
        return NS::ResultSuccess();
    case Code::NotFound:
        return ResultPathNotFound();
    case Code::AccessDenied:
        return ResultAccessDenied();
    case Code::InvalidPath:
        return ResultInvalidPathFormat();
    case Code::InvalidMount:
        return ResultMountNotFound();
    case Code::DiskFull:
        return ResultDiskFull();
    case Code::AlreadyExists:
        return ResultPathAlreadyExists();
    case Code::NotEmpty:
        return ResultDirectoryNotEmpty();
    case Code::IsDirectory:
        return ResultIsADirectory();
    case Code::IsNotDirectory:
        return ResultNotADirectory();
    case Code::PathTooLong:
        return ResultPathTooLong();
    case Code::ReadOnly:
        return ResultReadOnly();
    case Code::Cancelled:
        return NS::CommonResult::ResultCancelled();
    case Code::Unknown:
    default:
        return ResultUnknownError();
    }
}

FileError FileError::FromResult(NS::Result result) noexcept
{
    using namespace NS::FileSystemResult;

    if (result.IsSuccess())
    {
        return FileError{Code::None, 0, {}};
    }

    // FileSystemモジュールのエラーをマッピング
    if (result == ResultPathNotFound())
        return FileError{Code::NotFound, 0, {}};
    if (result == ResultPathAlreadyExists())
        return FileError{Code::AlreadyExists, 0, {}};
    if (result == ResultPathTooLong())
        return FileError{Code::PathTooLong, 0, {}};
    if (result == ResultInvalidPathFormat())
        return FileError{Code::InvalidPath, 0, {}};
    if (result == ResultNotADirectory())
        return FileError{Code::IsNotDirectory, 0, {}};
    if (result == ResultIsADirectory())
        return FileError{Code::IsDirectory, 0, {}};
    if (result == ResultDirectoryNotEmpty())
        return FileError{Code::NotEmpty, 0, {}};
    if (result == ResultAccessDenied())
        return FileError{Code::AccessDenied, 0, {}};
    if (result == ResultReadOnly())
        return FileError{Code::ReadOnly, 0, {}};
    if (result == ResultDiskFull())
        return FileError{Code::DiskFull, 0, {}};
    if (result == ResultMountNotFound())
        return FileError{Code::InvalidMount, 0, {}};
    if (result == ResultUnknownError())
        return FileError{Code::Unknown, 0, {}};

    // Commonモジュールのキャンセル
    if (result == NS::CommonResult::ResultCancelled())
        return FileError{Code::Cancelled, 0, {}};

    return FileError{Code::Unknown, 0, {}};
}

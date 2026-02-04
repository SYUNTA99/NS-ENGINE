# 08: Module - System Results

## 目的

システムレイヤーのエラー定義を実装（ファイルシステム、OS、メモリ、ネットワーク）。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Module/FileSystemResult.h` | 新規作成 |
| `Source/common/result/Module/FileSystemResult.cpp` | 新規作成 |
| `Source/common/result/Module/OsResult.h` | 新規作成 |
| `Source/common/result/Module/OsResult.cpp` | 新規作成 |
| `Source/common/result/Module/MemoryResult.h` | 新規作成 |
| `Source/common/result/Module/MemoryResult.cpp` | 新規作成 |
| `Source/common/result/Module/NetworkResult.h` | 新規作成 |
| `Source/common/result/Module/NetworkResult.cpp` | 新規作成 |

## 設計

### FileSystemResult

```cpp
// Source/common/result/Module/FileSystemResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// ファイルシステムエラー
namespace FileSystemResult {

    //=========================================================================
    // パス関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultPathError, Result::ModuleId::FileSystem, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultPathNotFound, Result::ModuleId::FileSystem, 1);
    NS_DEFINE_ERROR_RESULT(ResultPathAlreadyExists, Result::ModuleId::FileSystem, 2);
    NS_DEFINE_ERROR_RESULT(ResultPathTooLong, Result::ModuleId::FileSystem, 3);
    NS_DEFINE_ERROR_RESULT(ResultInvalidPathFormat, Result::ModuleId::FileSystem, 4);
    NS_DEFINE_ERROR_RESULT(ResultInvalidCharacter, Result::ModuleId::FileSystem, 5);
    NS_DEFINE_ERROR_RESULT(ResultNotAFile, Result::ModuleId::FileSystem, 6);
    NS_DEFINE_ERROR_RESULT(ResultNotADirectory, Result::ModuleId::FileSystem, 7);
    NS_DEFINE_ERROR_RESULT(ResultIsADirectory, Result::ModuleId::FileSystem, 8);
    NS_DEFINE_ERROR_RESULT(ResultDirectoryNotEmpty, Result::ModuleId::FileSystem, 9);
    NS_DEFINE_ERROR_RESULT(ResultSymlinkLoop, Result::ModuleId::FileSystem, 10);
    NS_DEFINE_ERROR_RESULT(ResultTooManySymlinks, Result::ModuleId::FileSystem, 11);
    NS_DEFINE_ERROR_RESULT(ResultCrossDevice, Result::ModuleId::FileSystem, 12);
    NS_DEFINE_ERROR_RESULT(ResultRootDirectory, Result::ModuleId::FileSystem, 13);

    //=========================================================================
    // アクセス関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultAccessError, Result::ModuleId::FileSystem, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultAccessDenied, Result::ModuleId::FileSystem, 100);
    NS_DEFINE_ERROR_RESULT(ResultPermissionDenied, Result::ModuleId::FileSystem, 101);
    NS_DEFINE_ERROR_RESULT(ResultFileLocked, Result::ModuleId::FileSystem, 102);
    NS_DEFINE_ERROR_RESULT(ResultSharingViolation, Result::ModuleId::FileSystem, 103);
    NS_DEFINE_ERROR_RESULT(ResultReadOnly, Result::ModuleId::FileSystem, 104);
    NS_DEFINE_ERROR_RESULT(ResultWriteProtected, Result::ModuleId::FileSystem, 105);
    NS_DEFINE_ERROR_RESULT(ResultInUse, Result::ModuleId::FileSystem, 106);

    //=========================================================================
    // 操作関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultOperationError, Result::ModuleId::FileSystem, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultOpenFailed, Result::ModuleId::FileSystem, 200);
    NS_DEFINE_ERROR_RESULT(ResultCloseFailed, Result::ModuleId::FileSystem, 201);
    NS_DEFINE_ERROR_RESULT(ResultReadFailed, Result::ModuleId::FileSystem, 202);
    NS_DEFINE_ERROR_RESULT(ResultWriteFailed, Result::ModuleId::FileSystem, 203);
    NS_DEFINE_ERROR_RESULT(ResultSeekFailed, Result::ModuleId::FileSystem, 204);
    NS_DEFINE_ERROR_RESULT(ResultFlushFailed, Result::ModuleId::FileSystem, 205);
    NS_DEFINE_ERROR_RESULT(ResultTruncateFailed, Result::ModuleId::FileSystem, 206);
    NS_DEFINE_ERROR_RESULT(ResultRenameFailed, Result::ModuleId::FileSystem, 207);
    NS_DEFINE_ERROR_RESULT(ResultCopyFailed, Result::ModuleId::FileSystem, 208);
    NS_DEFINE_ERROR_RESULT(ResultDeleteFailed, Result::ModuleId::FileSystem, 209);
    NS_DEFINE_ERROR_RESULT(ResultCreateDirectoryFailed, Result::ModuleId::FileSystem, 210);
    NS_DEFINE_ERROR_RESULT(ResultRemoveDirectoryFailed, Result::ModuleId::FileSystem, 211);

    //=========================================================================
    // ストレージ関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultStorageError, Result::ModuleId::FileSystem, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultDiskFull, Result::ModuleId::FileSystem, 300);
    NS_DEFINE_ERROR_RESULT(ResultQuotaExceeded, Result::ModuleId::FileSystem, 301);
    NS_DEFINE_ERROR_RESULT(ResultFileTooLarge, Result::ModuleId::FileSystem, 302);
    NS_DEFINE_ERROR_RESULT(ResultNoSpace, Result::ModuleId::FileSystem, 303);
    NS_DEFINE_ERROR_RESULT(ResultDriveNotReady, Result::ModuleId::FileSystem, 304);
    NS_DEFINE_ERROR_RESULT(ResultMediaRemoved, Result::ModuleId::FileSystem, 305);
    NS_DEFINE_ERROR_RESULT(ResultMediaChanged, Result::ModuleId::FileSystem, 306);
    NS_DEFINE_ERROR_RESULT(ResultMediaWriteProtected, Result::ModuleId::FileSystem, 307);

    //=========================================================================
    // データ関連 (400-499)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDataError, Result::ModuleId::FileSystem, 400, 500);
    NS_DEFINE_ERROR_RESULT(ResultEndOfFile, Result::ModuleId::FileSystem, 400);
    NS_DEFINE_ERROR_RESULT(ResultUnexpectedEndOfFile, Result::ModuleId::FileSystem, 401);
    NS_DEFINE_ERROR_RESULT(ResultDataCorrupted, Result::ModuleId::FileSystem, 402);
    NS_DEFINE_ERROR_RESULT(ResultInvalidFileFormat, Result::ModuleId::FileSystem, 403);
    NS_DEFINE_ERROR_RESULT(ResultUnsupportedFormat, Result::ModuleId::FileSystem, 404);

} // namespace FileSystemResult

} // namespace NS
```

### OsResult

```cpp
// Source/common/result/Module/OsResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// OSエラー
namespace OsResult {

    //=========================================================================
    // プロセス関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultProcessError, Result::ModuleId::Os, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultProcessNotFound, Result::ModuleId::Os, 1);
    NS_DEFINE_ERROR_RESULT(ResultProcessExitedAbnormally, Result::ModuleId::Os, 2);
    NS_DEFINE_ERROR_RESULT(ResultProcessKilled, Result::ModuleId::Os, 3);
    NS_DEFINE_ERROR_RESULT(ResultProcessCreateFailed, Result::ModuleId::Os, 4);
    NS_DEFINE_ERROR_RESULT(ResultProcessTerminateFailed, Result::ModuleId::Os, 5);
    NS_DEFINE_ERROR_RESULT(ResultWaitFailed, Result::ModuleId::Os, 6);

    //=========================================================================
    // スレッド関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultThreadError, Result::ModuleId::Os, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultThreadCreateFailed, Result::ModuleId::Os, 100);
    NS_DEFINE_ERROR_RESULT(ResultThreadJoinFailed, Result::ModuleId::Os, 101);
    NS_DEFINE_ERROR_RESULT(ResultThreadDetachFailed, Result::ModuleId::Os, 102);
    NS_DEFINE_ERROR_RESULT(ResultThreadAffinityFailed, Result::ModuleId::Os, 103);
    NS_DEFINE_ERROR_RESULT(ResultThreadPriorityFailed, Result::ModuleId::Os, 104);

    //=========================================================================
    // 同期関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultSyncError, Result::ModuleId::Os, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultMutexCreateFailed, Result::ModuleId::Os, 200);
    NS_DEFINE_ERROR_RESULT(ResultMutexLockFailed, Result::ModuleId::Os, 201);
    NS_DEFINE_ERROR_RESULT(ResultMutexUnlockFailed, Result::ModuleId::Os, 202);
    NS_DEFINE_ERROR_RESULT(ResultSemaphoreCreateFailed, Result::ModuleId::Os, 203);
    NS_DEFINE_ERROR_RESULT(ResultSemaphoreWaitFailed, Result::ModuleId::Os, 204);
    NS_DEFINE_ERROR_RESULT(ResultSemaphoreSignalFailed, Result::ModuleId::Os, 205);
    NS_DEFINE_ERROR_RESULT(ResultEventCreateFailed, Result::ModuleId::Os, 206);
    NS_DEFINE_ERROR_RESULT(ResultEventWaitFailed, Result::ModuleId::Os, 207);
    NS_DEFINE_ERROR_RESULT(ResultEventSignalFailed, Result::ModuleId::Os, 208);
    NS_DEFINE_ERROR_RESULT(ResultDeadlockDetected, Result::ModuleId::Os, 209);

    //=========================================================================
    // システム関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultSystemError, Result::ModuleId::Os, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultSystemCallFailed, Result::ModuleId::Os, 300);
    NS_DEFINE_ERROR_RESULT(ResultNotEnoughPrivilege, Result::ModuleId::Os, 301);
    NS_DEFINE_ERROR_RESULT(ResultTooManyOpenFiles, Result::ModuleId::Os, 302);
    NS_DEFINE_ERROR_RESULT(ResultTooManyProcesses, Result::ModuleId::Os, 303);
    NS_DEFINE_ERROR_RESULT(ResultEnvironmentVariableNotFound, Result::ModuleId::Os, 304);
    NS_DEFINE_ERROR_RESULT(ResultLibraryLoadFailed, Result::ModuleId::Os, 305);
    NS_DEFINE_ERROR_RESULT(ResultSymbolNotFound, Result::ModuleId::Os, 306);

} // namespace OsResult

} // namespace NS
```

### MemoryResult

```cpp
// Source/common/result/Module/MemoryResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// メモリ管理エラー
namespace MemoryResult {

    //=========================================================================
    // アロケーション関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultAllocationError, Result::ModuleId::Memory, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultAllocationFailed, Result::ModuleId::Memory, 1);
    NS_DEFINE_ERROR_RESULT(ResultAlignmentError, Result::ModuleId::Memory, 2);
    NS_DEFINE_ERROR_RESULT(ResultSizeOverflow, Result::ModuleId::Memory, 3);
    NS_DEFINE_ERROR_RESULT(ResultZeroSizeAllocation, Result::ModuleId::Memory, 4);
    NS_DEFINE_ERROR_RESULT(ResultFragmentation, Result::ModuleId::Memory, 5);

    //=========================================================================
    // 解放関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDeallocationError, Result::ModuleId::Memory, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultDoubleFree, Result::ModuleId::Memory, 100);
    NS_DEFINE_ERROR_RESULT(ResultInvalidPointer, Result::ModuleId::Memory, 101);
    NS_DEFINE_ERROR_RESULT(ResultHeapCorruption, Result::ModuleId::Memory, 102);
    NS_DEFINE_ERROR_RESULT(ResultUseAfterFree, Result::ModuleId::Memory, 103);

    //=========================================================================
    // プール・アロケータ関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultPoolError, Result::ModuleId::Memory, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultPoolExhausted, Result::ModuleId::Memory, 200);
    NS_DEFINE_ERROR_RESULT(ResultPoolNotInitialized, Result::ModuleId::Memory, 201);
    NS_DEFINE_ERROR_RESULT(ResultPoolAlreadyInitialized, Result::ModuleId::Memory, 202);
    NS_DEFINE_ERROR_RESULT(ResultWrongPool, Result::ModuleId::Memory, 203);
    NS_DEFINE_ERROR_RESULT(ResultBlockSizeMismatch, Result::ModuleId::Memory, 204);

    //=========================================================================
    // 仮想メモリ関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultVirtualMemoryError, Result::ModuleId::Memory, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultVirtualAllocFailed, Result::ModuleId::Memory, 300);
    NS_DEFINE_ERROR_RESULT(ResultVirtualFreeFailed, Result::ModuleId::Memory, 301);
    NS_DEFINE_ERROR_RESULT(ResultProtectionChangeFailed, Result::ModuleId::Memory, 302);
    NS_DEFINE_ERROR_RESULT(ResultPageFault, Result::ModuleId::Memory, 303);
    NS_DEFINE_ERROR_RESULT(ResultAddressNotMapped, Result::ModuleId::Memory, 304);

} // namespace MemoryResult

} // namespace NS
```

### NetworkResult

```cpp
// Source/common/result/Module/NetworkResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// ネットワークエラー
namespace NetworkResult {

    //=========================================================================
    // 接続関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultConnectionError, Result::ModuleId::Network, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultConnectionFailed, Result::ModuleId::Network, 1);
    NS_DEFINE_ERROR_RESULT(ResultConnectionRefused, Result::ModuleId::Network, 2);
    NS_DEFINE_ERROR_RESULT(ResultConnectionReset, Result::ModuleId::Network, 3);
    NS_DEFINE_ERROR_RESULT(ResultConnectionAborted, Result::ModuleId::Network, 4);
    NS_DEFINE_ERROR_RESULT(ResultConnectionTimeout, Result::ModuleId::Network, 5);
    NS_DEFINE_ERROR_RESULT(ResultNotConnected, Result::ModuleId::Network, 6);
    NS_DEFINE_ERROR_RESULT(ResultAlreadyConnected, Result::ModuleId::Network, 7);
    NS_DEFINE_ERROR_RESULT(ResultConnectionClosed, Result::ModuleId::Network, 8);
    NS_DEFINE_ERROR_RESULT(ResultNoRoute, Result::ModuleId::Network, 9);
    NS_DEFINE_ERROR_RESULT(ResultHostUnreachable, Result::ModuleId::Network, 10);
    NS_DEFINE_ERROR_RESULT(ResultNetworkUnreachable, Result::ModuleId::Network, 11);

    //=========================================================================
    // ソケット関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultSocketError, Result::ModuleId::Network, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultSocketCreateFailed, Result::ModuleId::Network, 100);
    NS_DEFINE_ERROR_RESULT(ResultSocketBindFailed, Result::ModuleId::Network, 101);
    NS_DEFINE_ERROR_RESULT(ResultSocketListenFailed, Result::ModuleId::Network, 102);
    NS_DEFINE_ERROR_RESULT(ResultSocketAcceptFailed, Result::ModuleId::Network, 103);
    NS_DEFINE_ERROR_RESULT(ResultSocketCloseFailed, Result::ModuleId::Network, 104);
    NS_DEFINE_ERROR_RESULT(ResultSocketOptionFailed, Result::ModuleId::Network, 105);
    NS_DEFINE_ERROR_RESULT(ResultAddressInUse, Result::ModuleId::Network, 106);
    NS_DEFINE_ERROR_RESULT(ResultAddressNotAvailable, Result::ModuleId::Network, 107);
    NS_DEFINE_ERROR_RESULT(ResultInvalidAddress, Result::ModuleId::Network, 108);

    //=========================================================================
    // 送受信関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultTransferError, Result::ModuleId::Network, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultSendFailed, Result::ModuleId::Network, 200);
    NS_DEFINE_ERROR_RESULT(ResultReceiveFailed, Result::ModuleId::Network, 201);
    NS_DEFINE_ERROR_RESULT(ResultMessageTooLarge, Result::ModuleId::Network, 202);
    NS_DEFINE_ERROR_RESULT(ResultBufferFull, Result::ModuleId::Network, 203);
    NS_DEFINE_ERROR_RESULT(ResultWouldBlock, Result::ModuleId::Network, 204);
    NS_DEFINE_ERROR_RESULT(ResultPartialSend, Result::ModuleId::Network, 205);
    NS_DEFINE_ERROR_RESULT(ResultPartialReceive, Result::ModuleId::Network, 206);

    //=========================================================================
    // DNS関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDnsError, Result::ModuleId::Network, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultDnsResolutionFailed, Result::ModuleId::Network, 300);
    NS_DEFINE_ERROR_RESULT(ResultHostNotFound, Result::ModuleId::Network, 301);
    NS_DEFINE_ERROR_RESULT(ResultServiceNotFound, Result::ModuleId::Network, 302);
    NS_DEFINE_ERROR_RESULT(ResultDnsTimeout, Result::ModuleId::Network, 303);

    //=========================================================================
    // プロトコル関連 (400-499)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultProtocolError, Result::ModuleId::Network, 400, 500);
    NS_DEFINE_ERROR_RESULT(ResultProtocolNotSupported, Result::ModuleId::Network, 400);
    NS_DEFINE_ERROR_RESULT(ResultInvalidProtocolData, Result::ModuleId::Network, 401);
    NS_DEFINE_ERROR_RESULT(ResultHandshakeFailed, Result::ModuleId::Network, 402);
    NS_DEFINE_ERROR_RESULT(ResultVersionMismatch, Result::ModuleId::Network, 403);
    NS_DEFINE_ERROR_RESULT(ResultTlsError, Result::ModuleId::Network, 404);
    NS_DEFINE_ERROR_RESULT(ResultCertificateError, Result::ModuleId::Network, 405);

} // namespace NetworkResult

} // namespace NS
```

## カテゴリ解決（各.cpp）

各モジュールの.cppファイルでは、CommonResult.cppと同様に以下を実装:
- `ResolveXxxCategory()`: エラーカテゴリの判定
- `ResolveXxxInfo()`: エラー情報の解決
- 自動登録用の静的インスタンス

```cpp
// 例: FileSystemResult.cpp
namespace {
struct FileSystemResultRegistrar {
    FileSystemResultRegistrar() {
        Detail::CategoryRegistry::Register(ModuleId::FileSystem, ResolveFileSystemCategory);
        Detail::InfoRegistry::Register(ModuleId::FileSystem, ResolveFileSystemInfo);
    }
} s_registrar;
}
```

## TODO

- [ ] `FileSystemResult.h` 作成
- [ ] `FileSystemResult.cpp` 作成
- [ ] `OsResult.h` 作成
- [ ] `OsResult.cpp` 作成
- [ ] `MemoryResult.h` 作成
- [ ] `MemoryResult.cpp` 作成
- [ ] `NetworkResult.h` 作成
- [ ] `NetworkResult.cpp` 作成
- [ ] カテゴリ/情報解決実装
- [ ] ビルド確認

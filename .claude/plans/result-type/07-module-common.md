# 07: Module - ModuleId・CommonResult

## 目的

モジュールID定義と共通エラーコードを実装。全モジュールで使用される汎用エラー。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Module/ModuleId.h` | 新規作成 |
| `Source/common/result/Module/CommonResult.h` | 新規作成 |
| `Source/common/result/Module/CommonResult.cpp` | 新規作成 |

## 設計

### ModuleId

```cpp
// Source/common/result/Module/ModuleId.h
#pragma once

namespace NS::Result {

/// モジュールID定義
/// 範囲: 1-511 (9bit, 0は成功用に予約)
namespace ModuleId {
    // Core (1-9)
    constexpr int Common      = 1;   // 共通エラー

    // System (10-49)
    constexpr int FileSystem  = 10;  // ファイルシステム
    constexpr int Os          = 11;  // OS
    constexpr int Memory      = 12;  // メモリ管理
    constexpr int Network     = 13;  // ネットワーク
    constexpr int Thread      = 14;  // スレッド

    // Engine (50-99)
    constexpr int Graphics    = 50;  // グラフィックス
    constexpr int Ecs         = 51;  // ECS
    constexpr int Audio       = 52;  // オーディオ
    constexpr int Input       = 53;  // 入力
    constexpr int Resource    = 54;  // リソース管理
    constexpr int Scene       = 55;  // シーン管理
    constexpr int Physics     = 56;  // 物理

    // Application (100-149)
    constexpr int Application = 100; // アプリケーション
    constexpr int Ui          = 101; // UI
    constexpr int Script      = 102; // スクリプト

    // User defined (200-511)
    constexpr int UserBegin   = 200;
    constexpr int UserEnd     = 512;
}

/// モジュール名取得
[[nodiscard]] const char* GetModuleName(int moduleId) noexcept;

} // namespace NS::Result
```

### CommonResult

```cpp
// Source/common/result/Module/CommonResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// 共通エラーコード
/// 全モジュールで発生しうる汎用的なエラー
namespace CommonResult {

    //=========================================================================
    // 成功・汎用 (0-99)
    //=========================================================================

    // 汎用失敗
    NS_DEFINE_ERROR_RESULT(ResultOperationFailed, Result::ModuleId::Common, 1);
    NS_DEFINE_ERROR_RESULT(ResultNotImplemented, Result::ModuleId::Common, 2);
    NS_DEFINE_ERROR_RESULT(ResultNotSupported, Result::ModuleId::Common, 3);

    //=========================================================================
    // 引数・状態 (100-199)
    //=========================================================================

    // 引数エラー
    NS_DEFINE_ERROR_RANGE_RESULT(ResultInvalidArgument, Result::ModuleId::Common, 100, 150);
    NS_DEFINE_ERROR_RESULT(ResultNullPointer, Result::ModuleId::Common, 100);
    NS_DEFINE_ERROR_RESULT(ResultOutOfRange, Result::ModuleId::Common, 101);
    NS_DEFINE_ERROR_RESULT(ResultInvalidSize, Result::ModuleId::Common, 102);
    NS_DEFINE_ERROR_RESULT(ResultInvalidFormat, Result::ModuleId::Common, 103);
    NS_DEFINE_ERROR_RESULT(ResultInvalidHandle, Result::ModuleId::Common, 104);
    NS_DEFINE_ERROR_RESULT(ResultInvalidId, Result::ModuleId::Common, 105);
    NS_DEFINE_ERROR_RESULT(ResultInvalidState, Result::ModuleId::Common, 106);
    NS_DEFINE_ERROR_RESULT(ResultInvalidOperation, Result::ModuleId::Common, 107);
    NS_DEFINE_ERROR_RESULT(ResultInvalidPath, Result::ModuleId::Common, 108);
    NS_DEFINE_ERROR_RESULT(ResultInvalidName, Result::ModuleId::Common, 109);
    NS_DEFINE_ERROR_RESULT(ResultInvalidType, Result::ModuleId::Common, 110);
    NS_DEFINE_ERROR_RESULT(ResultInvalidData, Result::ModuleId::Common, 111);
    NS_DEFINE_ERROR_RESULT(ResultInvalidVersion, Result::ModuleId::Common, 112);
    NS_DEFINE_ERROR_RESULT(ResultInvalidConfiguration, Result::ModuleId::Common, 113);

    // 状態エラー
    NS_DEFINE_ERROR_RANGE_RESULT(ResultStateError, Result::ModuleId::Common, 150, 200);
    NS_DEFINE_ERROR_RESULT(ResultNotInitialized, Result::ModuleId::Common, 150);
    NS_DEFINE_ERROR_RESULT(ResultAlreadyInitialized, Result::ModuleId::Common, 151);
    NS_DEFINE_ERROR_RESULT(ResultNotStarted, Result::ModuleId::Common, 152);
    NS_DEFINE_ERROR_RESULT(ResultAlreadyStarted, Result::ModuleId::Common, 153);
    NS_DEFINE_ERROR_RESULT(ResultNotFinished, Result::ModuleId::Common, 154);
    NS_DEFINE_ERROR_RESULT(ResultAlreadyFinished, Result::ModuleId::Common, 155);
    NS_DEFINE_ERROR_RESULT(ResultNotOpen, Result::ModuleId::Common, 156);
    NS_DEFINE_ERROR_RESULT(ResultAlreadyOpen, Result::ModuleId::Common, 157);
    NS_DEFINE_ERROR_RESULT(ResultNotClosed, Result::ModuleId::Common, 158);
    NS_DEFINE_ERROR_RESULT(ResultAlreadyClosed, Result::ModuleId::Common, 159);

    //=========================================================================
    // リソース (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultResourceError, Result::ModuleId::Common, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultOutOfMemory, Result::ModuleId::Common, 200);
    NS_DEFINE_ERROR_RESULT(ResultOutOfResources, Result::ModuleId::Common, 201);
    NS_DEFINE_ERROR_RESULT(ResultResourceBusy, Result::ModuleId::Common, 202);
    NS_DEFINE_ERROR_RESULT(ResultResourceNotFound, Result::ModuleId::Common, 203);
    NS_DEFINE_ERROR_RESULT(ResultResourceAlreadyExists, Result::ModuleId::Common, 204);
    NS_DEFINE_ERROR_RESULT(ResultResourceLocked, Result::ModuleId::Common, 205);
    NS_DEFINE_ERROR_RESULT(ResultResourceExhausted, Result::ModuleId::Common, 206);
    NS_DEFINE_ERROR_RESULT(ResultCapacityExceeded, Result::ModuleId::Common, 207);
    NS_DEFINE_ERROR_RESULT(ResultLimitReached, Result::ModuleId::Common, 208);
    NS_DEFINE_ERROR_RESULT(ResultQuotaExceeded, Result::ModuleId::Common, 209);

    //=========================================================================
    // タイミング・同期 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultTimingError, Result::ModuleId::Common, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultTimeout, Result::ModuleId::Common, 300);
    NS_DEFINE_ERROR_RESULT(ResultDeadlock, Result::ModuleId::Common, 301);
    NS_DEFINE_ERROR_RESULT(ResultWouldBlock, Result::ModuleId::Common, 302);
    NS_DEFINE_ERROR_RESULT(ResultInterrupted, Result::ModuleId::Common, 303);
    NS_DEFINE_ERROR_RESULT(ResultCancelled, Result::ModuleId::Common, 304);
    NS_DEFINE_ERROR_RESULT(ResultAborted, Result::ModuleId::Common, 305);
    NS_DEFINE_ERROR_RESULT(ResultRetryLater, Result::ModuleId::Common, 306);
    NS_DEFINE_ERROR_RESULT(ResultPending, Result::ModuleId::Common, 307);
    NS_DEFINE_ERROR_RESULT(ResultInProgress, Result::ModuleId::Common, 308);

    //=========================================================================
    // アクセス・権限 (400-499)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultAccessError, Result::ModuleId::Common, 400, 500);
    NS_DEFINE_ERROR_RESULT(ResultAccessDenied, Result::ModuleId::Common, 400);
    NS_DEFINE_ERROR_RESULT(ResultPermissionDenied, Result::ModuleId::Common, 401);
    NS_DEFINE_ERROR_RESULT(ResultReadOnly, Result::ModuleId::Common, 402);
    NS_DEFINE_ERROR_RESULT(ResultWriteProtected, Result::ModuleId::Common, 403);
    NS_DEFINE_ERROR_RESULT(ResultNotAuthorized, Result::ModuleId::Common, 404);
    NS_DEFINE_ERROR_RESULT(ResultForbidden, Result::ModuleId::Common, 405);

    //=========================================================================
    // データ整合性 (500-599)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDataError, Result::ModuleId::Common, 500, 600);
    NS_DEFINE_ERROR_RESULT(ResultDataCorrupted, Result::ModuleId::Common, 500);
    NS_DEFINE_ERROR_RESULT(ResultChecksumMismatch, Result::ModuleId::Common, 501);
    NS_DEFINE_ERROR_RESULT(ResultVersionMismatch, Result::ModuleId::Common, 502);
    NS_DEFINE_ERROR_RESULT(ResultFormatError, Result::ModuleId::Common, 503);
    NS_DEFINE_ERROR_RESULT(ResultParseError, Result::ModuleId::Common, 504);
    NS_DEFINE_ERROR_RESULT(ResultSerializationError, Result::ModuleId::Common, 505);
    NS_DEFINE_ERROR_RESULT(ResultDeserializationError, Result::ModuleId::Common, 506);
    NS_DEFINE_ERROR_RESULT(ResultEncodingError, Result::ModuleId::Common, 507);
    NS_DEFINE_ERROR_RESULT(ResultDecodingError, Result::ModuleId::Common, 508);
    NS_DEFINE_ERROR_RESULT(ResultValidationFailed, Result::ModuleId::Common, 509);
    NS_DEFINE_ERROR_RESULT(ResultConstraintViolation, Result::ModuleId::Common, 510);

    //=========================================================================
    // 初期化・終了 (600-699)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultLifecycleError, Result::ModuleId::Common, 600, 700);
    NS_DEFINE_ERROR_RESULT(ResultInitializationFailed, Result::ModuleId::Common, 600);
    NS_DEFINE_ERROR_RESULT(ResultShutdownFailed, Result::ModuleId::Common, 601);
    NS_DEFINE_ERROR_RESULT(ResultLoadFailed, Result::ModuleId::Common, 602);
    NS_DEFINE_ERROR_RESULT(ResultUnloadFailed, Result::ModuleId::Common, 603);
    NS_DEFINE_ERROR_RESULT(ResultCreateFailed, Result::ModuleId::Common, 604);
    NS_DEFINE_ERROR_RESULT(ResultDestroyFailed, Result::ModuleId::Common, 605);
    NS_DEFINE_ERROR_RESULT(ResultConnectFailed, Result::ModuleId::Common, 606);
    NS_DEFINE_ERROR_RESULT(ResultDisconnectFailed, Result::ModuleId::Common, 607);
    NS_DEFINE_ERROR_RESULT(ResultBindFailed, Result::ModuleId::Common, 608);
    NS_DEFINE_ERROR_RESULT(ResultUnbindFailed, Result::ModuleId::Common, 609);

    //=========================================================================
    // 内部エラー (700-799)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultInternalError, Result::ModuleId::Common, 700, 800);
    NS_DEFINE_ERROR_RESULT(ResultUnknownError, Result::ModuleId::Common, 700);
    NS_DEFINE_ERROR_RESULT(ResultAssertionFailed, Result::ModuleId::Common, 701);
    NS_DEFINE_ERROR_RESULT(ResultInvariantViolation, Result::ModuleId::Common, 702);
    NS_DEFINE_ERROR_RESULT(ResultUnreachable, Result::ModuleId::Common, 703);
    NS_DEFINE_ERROR_RESULT(ResultBugDetected, Result::ModuleId::Common, 704);

} // namespace CommonResult

} // namespace NS
```

### 実装

```cpp
// Source/common/result/Module/CommonResult.cpp
#include "common/result/Module/CommonResult.h"
#include "common/result/Error/ErrorCategory.h"
#include "common/result/Error/ErrorInfo.h"

namespace NS::Result {

namespace {

// モジュール名テーブル
constexpr const char* kModuleNames[] = {
    "Unknown",      // 0
    "Common",       // 1
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  // 2-9
    "FileSystem",   // 10
    "Os",           // 11
    "Memory",       // 12
    "Network",      // 13
    "Thread",       // 14
};

constexpr std::size_t kModuleNamesSize = sizeof(kModuleNames) / sizeof(kModuleNames[0]);

// Commonモジュールのカテゴリ解決
ErrorCategory ResolveCommonCategory(::NS::Result result) noexcept {
    const int desc = result.GetDescription();

    // リソースエラーは一時的（リトライ可能な場合がある）
    if (desc >= 200 && desc < 300) {
        if (desc == 200 || desc == 201 || desc == 206) {
            // OutOfMemory, OutOfResources, ResourceExhausted
            return {ErrorPersistence::Transient, ErrorSeverity::Recoverable};
        }
        return {ErrorPersistence::Permanent, ErrorSeverity::Recoverable};
    }

    // タイミングエラーは一時的
    if (desc >= 300 && desc < 400) {
        return {ErrorPersistence::Transient, ErrorSeverity::Recoverable};
    }

    // アクセスエラーは永続的
    if (desc >= 400 && desc < 500) {
        return {ErrorPersistence::Permanent, ErrorSeverity::Recoverable};
    }

    // データエラーは永続的
    if (desc >= 500 && desc < 600) {
        return {ErrorPersistence::Permanent, ErrorSeverity::Recoverable};
    }

    // 内部エラーは致命的
    if (desc >= 700 && desc < 800) {
        return {ErrorPersistence::Permanent, ErrorSeverity::Fatal};
    }

    return {ErrorPersistence::Unknown, ErrorSeverity::Unknown};
}

// Commonモジュールの情報解決
ErrorInfo ResolveCommonInfo(::NS::Result result) noexcept {
    const int desc = result.GetDescription();
    ErrorInfo info{
        .moduleName = "Common",
        .category = ResolveCommonCategory(result),
        .module = ModuleId::Common,
        .description = desc
    };

    // エラー名とメッセージの解決
    switch (desc) {
        case 1:   info.errorName = "OperationFailed";    info.message = "Operation failed"; break;
        case 2:   info.errorName = "NotImplemented";     info.message = "Not implemented"; break;
        case 3:   info.errorName = "NotSupported";       info.message = "Not supported"; break;
        case 100: info.errorName = "NullPointer";        info.message = "Null pointer"; break;
        case 101: info.errorName = "OutOfRange";         info.message = "Out of range"; break;
        case 200: info.errorName = "OutOfMemory";        info.message = "Out of memory"; break;
        case 300: info.errorName = "Timeout";            info.message = "Operation timed out"; break;
        case 400: info.errorName = "AccessDenied";       info.message = "Access denied"; break;
        case 500: info.errorName = "DataCorrupted";      info.message = "Data corrupted"; break;
        case 600: info.errorName = "InitializationFailed"; info.message = "Initialization failed"; break;
        case 700: info.errorName = "UnknownError";       info.message = "Unknown internal error"; break;
        default:  info.errorName = "Unknown";            info.message = "Unknown common error"; break;
    }

    return info;
}

// 自動登録
struct CommonResultRegistrar {
    CommonResultRegistrar() {
        Detail::CategoryRegistry::Register(ModuleId::Common, ResolveCommonCategory);
        Detail::InfoRegistry::Register(ModuleId::Common, ResolveCommonInfo);
    }
} s_registrar;

} // namespace

const char* GetModuleName(int moduleId) noexcept {
    if (moduleId >= 0 && moduleId < static_cast<int>(kModuleNamesSize)) {
        const char* name = kModuleNames[moduleId];
        return name ? name : "Unknown";
    }

    // Engine modules (50-99)
    switch (moduleId) {
        case ModuleId::Graphics: return "Graphics";
        case ModuleId::Ecs:      return "Ecs";
        case ModuleId::Audio:    return "Audio";
        case ModuleId::Input:    return "Input";
        case ModuleId::Resource: return "Resource";
        case ModuleId::Scene:    return "Scene";
        case ModuleId::Physics:  return "Physics";
        default: return "Unknown";
    }
}

} // namespace NS::Result
```

## TODO

- [ ] `Source/common/result/Module/` ディレクトリ作成
- [ ] `ModuleId.h` 作成
- [ ] `CommonResult.h` 作成
- [ ] `CommonResult.cpp` 作成
- [ ] カテゴリ解決検証
- [ ] 情報解決検証
- [ ] ビルド確認

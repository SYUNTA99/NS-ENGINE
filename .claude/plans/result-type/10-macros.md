# 10: Utility - Macros

## 目的

Result操作を簡潔に記述するためのヘルパーマクロを実装。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Utility/ResultMacros.h` | 新規作成 |

## 設計

### ResultMacros

```cpp
// Source/common/result/Utility/ResultMacros.h
#pragma once

#include "common/result/Core/Result.h"
#include "common/result/Context/SourceLocation.h"

//=============================================================================
// 基本マクロ
//=============================================================================

/// 失敗時に早期リターン
#define NS_RETURN_IF_FAILED(expr) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            return _ns_result_; \
        } \
    } while (false)

/// 成功時に早期リターン
#define NS_RETURN_IF_SUCCESS(expr) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsSuccess()) { \
            return _ns_result_; \
        } \
    } while (false)

/// 失敗時にgotoでジャンプ
#define NS_GOTO_IF_FAILED(expr, label) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            goto label; \
        } \
    } while (false)

//=============================================================================
// 値取得マクロ
//=============================================================================

/// 失敗時に早期リターンし、成功時は値を取得
/// 使用例: NS_TRY_ASSIGN(value, GetValue())
#define NS_TRY_ASSIGN(var, expr) \
    auto _ns_try_result_ = (expr); \
    if (_ns_try_result_.IsFailure()) { \
        return _ns_try_result_.GetResult(); \
    } \
    var = std::move(_ns_try_result_).GetValue()

/// auto変数宣言付き
/// 使用例: NS_TRY(value, GetValue())
#define NS_TRY(var, expr) \
    auto _ns_try_result_##var = (expr); \
    if (_ns_try_result_##var.IsFailure()) { \
        return _ns_try_result_##var.GetResult(); \
    } \
    auto var = std::move(_ns_try_result_##var).GetValue()

//=============================================================================
// コンテキスト付きマクロ（05-context.mdで定義済み）
//=============================================================================

#ifndef NS_RETURN_IF_FAILED_CTX
#define NS_RETURN_IF_FAILED_CTX(expr) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            ::NS::Result::RecordContext(_ns_result_, NS_CURRENT_SOURCE_LOCATION()); \
            return _ns_result_; \
        } \
    } while (false)
#endif

#ifndef NS_RETURN_IF_FAILED_CTX_MSG
#define NS_RETURN_IF_FAILED_CTX_MSG(expr, msg) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            ::NS::Result::RecordContext(_ns_result_, NS_CURRENT_SOURCE_LOCATION(), msg); \
            return _ns_result_; \
        } \
    } while (false)
#endif

//=============================================================================
// アサーションマクロ
//=============================================================================

/// 成功を期待（失敗時はアサーション）
#define NS_ASSERT_SUCCESS(expr) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            NS_ASSERT(false, "Expected success but got failure: " #expr); \
        } \
    } while (false)

/// 失敗を期待（成功時はアサーション）
#define NS_ASSERT_FAILED(expr) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsSuccess()) { \
            NS_ASSERT(false, "Expected failure but got success: " #expr); \
        } \
    } while (false)

//=============================================================================
// ロギングマクロ
//=============================================================================

/// 失敗時にログ出力して早期リターン
#define NS_RETURN_IF_FAILED_LOG(expr, ...) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            LOG_ERROR(__VA_ARGS__); \
            return _ns_result_; \
        } \
    } while (false)

/// 失敗時に警告ログを出力（リターンしない）
#define NS_WARN_IF_FAILED(expr, ...) \
    do { \
        if (auto _ns_result_ = (expr); _ns_result_.IsFailure()) { \
            LOG_WARN(__VA_ARGS__); \
        } \
    } while (false)

//=============================================================================
// 変換マクロ
//=============================================================================

/// HRESULTからResultへ変換
#define NS_FROM_HRESULT(hr, successResult, failResult) \
    (SUCCEEDED(hr) ? (successResult) : (failResult))

/// Win32エラーコードからResultへ変換
#define NS_FROM_WIN32(err, successResult, failResult) \
    ((err) == ERROR_SUCCESS ? (successResult) : (failResult))

/// boolからResultへ変換
#define NS_FROM_BOOL(b, successResult, failResult) \
    ((b) ? (successResult) : (failResult))

/// nullptrチェック付きResult変換
#define NS_FROM_POINTER(ptr, successResult, failResult) \
    ((ptr) != nullptr ? (successResult) : (failResult))

//=============================================================================
// スコープガードマクロ
//=============================================================================

/// 失敗時のクリーンアップ用
/// 使用例:
///   NS_SCOPE_FAIL { CleanUp(); };
///   NS_RETURN_IF_FAILED(SomeOperation());
#define NS_SCOPE_FAIL \
    auto _ns_scope_fail_guard_ = ::NS::Result::Detail::ScopeFailGuard{} + [&]()

/// 成功時の処理用
#define NS_SCOPE_SUCCESS \
    auto _ns_scope_success_guard_ = ::NS::Result::Detail::ScopeSuccessGuard{} + [&]()

namespace NS::Result::Detail {

template <typename F>
class ScopeFailGuardImpl {
public:
    explicit ScopeFailGuardImpl(F&& f) noexcept : m_func(std::forward<F>(f)) {}
    ~ScopeFailGuardImpl() {
        if (m_active && std::uncaught_exceptions() > 0) {
            m_func();
        }
    }
    void Dismiss() noexcept { m_active = false; }

private:
    F m_func;
    bool m_active = true;
};

struct ScopeFailGuard {
    template <typename F>
    ScopeFailGuardImpl<F> operator+(F&& f) const noexcept {
        return ScopeFailGuardImpl<F>(std::forward<F>(f));
    }
};

template <typename F>
class ScopeSuccessGuardImpl {
public:
    explicit ScopeSuccessGuardImpl(F&& f) noexcept : m_func(std::forward<F>(f)) {}
    ~ScopeSuccessGuardImpl() {
        if (m_active && std::uncaught_exceptions() == 0) {
            m_func();
        }
    }
    void Dismiss() noexcept { m_active = false; }

private:
    F m_func;
    bool m_active = true;
};

struct ScopeSuccessGuard {
    template <typename F>
    ScopeSuccessGuardImpl<F> operator+(F&& f) const noexcept {
        return ScopeSuccessGuardImpl<F>(std::forward<F>(f));
    }
};

} // namespace NS::Result::Detail

//=============================================================================
// 条件付きリターンマクロ
//=============================================================================

/// 条件が真の場合に指定したResultを返す
#define NS_RETURN_IF(cond, result) \
    do { \
        if (cond) { \
            return (result); \
        } \
    } while (false)

/// nullptrの場合にResultを返す
#define NS_RETURN_IF_NULL(ptr, result) \
    NS_RETURN_IF((ptr) == nullptr, result)

/// 範囲外の場合にResultを返す
#define NS_RETURN_IF_OUT_OF_RANGE(val, min, max, result) \
    NS_RETURN_IF((val) < (min) || (val) >= (max), result)

//=============================================================================
// デバッグ用マクロ
//=============================================================================

#ifdef NS_DEBUG

/// デバッグビルドでのみResultを検証
#define NS_DEBUG_VERIFY_SUCCESS(expr) NS_ASSERT_SUCCESS(expr)
#define NS_DEBUG_VERIFY_FAILED(expr) NS_ASSERT_FAILED(expr)

#else

#define NS_DEBUG_VERIFY_SUCCESS(expr) ((void)(expr))
#define NS_DEBUG_VERIFY_FAILED(expr) ((void)(expr))

#endif // NS_DEBUG
```

## 使用例

```cpp
// 基本的な使い方
NS::Result ProcessFile(const char* path) {
    NS_RETURN_IF_NULL(path, CommonResult::ResultNullPointer());

    NS_RETURN_IF_FAILED(ValidatePath(path));
    NS_RETURN_IF_FAILED_CTX(OpenFile(path));
    NS_RETURN_IF_FAILED_CTX_MSG(ReadContents(), "Failed to read file contents");

    return NS::ResultSuccess();
}

// スコープガード
NS::Result CreateResource() {
    Resource* resource = Allocate();
    NS_SCOPE_FAIL { Free(resource); };

    NS_RETURN_IF_FAILED(Initialize(resource));
    NS_RETURN_IF_FAILED(Configure(resource));

    return NS::ResultSuccess();  // 成功時はFreeされない
}

// HRESULTとの連携
NS::Result CreateD3DDevice() {
    HRESULT hr = D3D11CreateDevice(...);
    return NS_FROM_HRESULT(hr,
        NS::ResultSuccess(),
        GraphicsResult::ResultDeviceCreationFailed());
}

// ログ付きリターン
NS::Result LoadConfig() {
    NS_RETURN_IF_FAILED_LOG(
        ReadConfigFile(),
        "Failed to read config file, using defaults");
    return NS::ResultSuccess();
}
```

## TODO

- [ ] `Source/common/result/Utility/` ディレクトリ作成
- [ ] `ResultMacros.h` 作成
- [ ] 各マクロの動作検証
- [ ] スコープガードの例外安全性検証
- [ ] ビルド確認

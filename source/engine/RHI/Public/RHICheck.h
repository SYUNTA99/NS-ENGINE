/// @file RHICheck.h
/// @brief RHIチェック・検証マクロ
/// @details RHI操作のデバッグ検証とプラットフォーム固有エラーチェックを提供。
/// @see 17-01-rhi-result.md
#pragma once

#include "Common/Logging/Logging.h"
#include "Common/Utility/Macros.h"
#include <cassert>
#include <cstdio>

//=============================================================================
// バリデーション有効化フラグ
//=============================================================================

#ifndef NS_RHI_VALIDATION_ENABLED
#if NS_BUILD_DEBUG
#define NS_RHI_VALIDATION_ENABLED 1
#else
#define NS_RHI_VALIDATION_ENABLED 0
#endif
#endif

//=============================================================================
// チェックマクロ
//=============================================================================

#if NS_RHI_VALIDATION_ENABLED

/// 条件チェック（falseならログ+assert）
#define RHI_CHECK(condition)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            char rhiCheckBuf_[512];                                                                                    \
            snprintf(                                                                                                  \
                rhiCheckBuf_, sizeof(rhiCheckBuf_), "[RHI] Check failed: %s (%s:%d)", #condition, __FILE__, __LINE__); \
            LOG_ERROR(rhiCheckBuf_);                                                                                   \
            assert(false && #condition);                                                                               \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

/// フォーマット付き条件チェック
#define RHI_CHECKF(condition, fmt, ...)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            char rhiCheckBuf_[512];                                                                                    \
            snprintf(rhiCheckBuf_,                                                                                     \
                     sizeof(rhiCheckBuf_),                                                                             \
                     "[RHI] Check failed: " fmt " (%s:%d)",                                                            \
                     ##__VA_ARGS__,                                                                                    \
                     __FILE__,                                                                                         \
                     __LINE__);                                                                                        \
            LOG_ERROR(rhiCheckBuf_);                                                                                   \
            assert(false && #condition);                                                                               \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

/// 条件チェック（リリースでも条件を評価、結果を返す）
#define RHI_ENSURE(condition)                                                                                          \
    ([&]() -> bool {                                                                                                   \
        bool result_ = !!(condition);                                                                                  \
        if (!result_)                                                                                                  \
        {                                                                                                              \
            char rhiCheckBuf_[512];                                                                                    \
            snprintf(rhiCheckBuf_,                                                                                     \
                     sizeof(rhiCheckBuf_),                                                                             \
                     "[RHI] Ensure failed: %s (%s:%d)",                                                                \
                     #condition,                                                                                       \
                     __FILE__,                                                                                         \
                     __LINE__);                                                                                        \
            LOG_ERROR(rhiCheckBuf_);                                                                                   \
            assert(false && #condition);                                                                               \
        }                                                                                                              \
        return result_;                                                                                                \
    }())

/// 条件を評価し、成否にかかわらず結果を返す（デバッグ時はログ出力）
#define RHI_VERIFY(condition)                                                                                          \
    ([&]() -> bool {                                                                                                   \
        bool result_ = !!(condition);                                                                                  \
        if (!result_)                                                                                                  \
        {                                                                                                              \
            char rhiCheckBuf_[512];                                                                                    \
            snprintf(rhiCheckBuf_,                                                                                     \
                     sizeof(rhiCheckBuf_),                                                                             \
                     "[RHI] Verify failed: %s (%s:%d)",                                                                \
                     #condition,                                                                                       \
                     __FILE__,                                                                                         \
                     __LINE__);                                                                                        \
            LOG_WARN(rhiCheckBuf_);                                                                                    \
        }                                                                                                              \
        return result_;                                                                                                \
    }())

#else // !NS_RHI_VALIDATION_ENABLED

#define RHI_CHECK(condition) ((void)0)
#define RHI_CHECKF(condition, fmt, ...) ((void)0)
#define RHI_ENSURE(condition) (!!(condition))
#define RHI_VERIFY(condition) (!!(condition))

#endif // NS_RHI_VALIDATION_ENABLED

//=============================================================================
// HRESULT チェック（D3D12バックエンド内部用）
// D3D12バックエンドの Private ヘッダーからインクルードされる想定
//=============================================================================

#ifdef _WIN32

/// HRESULT成功チェック（失敗時ログ+false返却）
#define RHI_CHECK_HRESULT(hr)                                                                                          \
    ([&]() -> bool {                                                                                                   \
        HRESULT hr_ = (hr);                                                                                            \
        if (FAILED(hr_))                                                                                               \
        {                                                                                                              \
            char rhiCheckBuf_[512];                                                                                    \
            snprintf(rhiCheckBuf_,                                                                                     \
                     sizeof(rhiCheckBuf_),                                                                             \
                     "[RHI/D3D12] HRESULT failed: 0x%08lX (%s:%d)",                                                    \
                     static_cast<unsigned long>(hr_),                                                                  \
                     __FILE__,                                                                                         \
                     __LINE__);                                                                                        \
            LOG_ERROR(rhiCheckBuf_);                                                                                   \
            return false;                                                                                              \
        }                                                                                                              \
        return true;                                                                                                   \
    }())

#endif // _WIN32

//=============================================================================
// VkResult チェック（Vulkanバックエンド内部用）
// Vulkanバックエンドの Private ヘッダーからインクルードされる想定
// vulkan.h が先にインクルードされている場合のみ有効
//=============================================================================

#ifdef VK_SUCCESS

/// VkResult成功チェック（失敗時ログ+false返却）
#define RHI_CHECK_VK(vkResult)                                                                                         \
    ([&]() -> bool {                                                                                                   \
        auto vk_ = (vkResult);                                                                                         \
        if (vk_ != VK_SUCCESS)                                                                                         \
        {                                                                                                              \
            char rhiCheckBuf_[512];                                                                                    \
            snprintf(rhiCheckBuf_,                                                                                     \
                     sizeof(rhiCheckBuf_),                                                                             \
                     "[RHI/Vulkan] VkResult failed: %d (%s:%d)",                                                       \
                     static_cast<int>(vk_),                                                                            \
                     __FILE__,                                                                                         \
                     __LINE__);                                                                                        \
            LOG_ERROR(rhiCheckBuf_);                                                                                   \
            return false;                                                                                              \
        }                                                                                                              \
        return true;                                                                                                   \
    }())

#endif // VK_SUCCESS
